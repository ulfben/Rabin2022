#define _CRT_SECURE_NO_WARNINGS
#include "profile.h"
#include "custom_time.h"
#include <cassert>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
struct Sample {
  std::string_view szName;
  unsigned int iProfileInstances = 0; // # of times ProfileBegin called
  int iOpenProfiles = 0;              // # of times ProfileBegin w/o ProfileEnd
  float fStartTime = 0.0f;            // The current open profile start time
  float fAccumulator = 0.0f;          // All samples this frame added together
  float fChildrenSampleTime = 0.0f;   // Time taken by all children
  std::size_t iNumParents = 0;        // Number of profile parents
  Sample(std::string_view name) noexcept
      : szName{name}, iOpenProfiles{1}, iProfileInstances{1},
        fStartTime{GetExactTime()} {}
};
struct SampleHistory {
  std::string_view szName; // Name of the sample
  Profiler::TimePerFrame times{};
  SampleHistory(std::string_view name, float percent) noexcept : szName{name} {
    times.setAll(percent);
  }

  void updateTimings(float percent, float newRatio) noexcept {
    const float oldRatio = 1.0f - newRatio;
    const float percentNewRatio = (percent * newRatio);
    times.average = (times.average * oldRatio) + percentNewRatio;
    times.min = (percent < times.min)
                    ? percent
                    : (times.min * oldRatio) + percentNewRatio;
    if (times.min < 0.0f) {
      times.min = 0.0f;
    }
    times.max = (percent > times.max)
                    ? percent
                    : (times.max * oldRatio) + percentNewRatio;
  }
};

std::vector<Sample> samples;
std::vector<SampleHistory> history;
float _startTime = 0.0f;
float _endTime = 0.0f;
std::string textBox = "";

void assert_if_invalid(const Sample &s) noexcept {
  if (s.iOpenProfiles < 0) {
    assert(!"ProfileEnd() called without a ProfileBegin()");
  } else if (s.iOpenProfiles > 0) {
    assert(!"ProfileBegin() called without a ProfileEnd()");
  }
}

bool hasParents(const Sample &s) noexcept { return s.iNumParents > 0; }

bool isParent(const Sample &s) noexcept { return s.iOpenProfiles > 0; }

size_t countParents() noexcept {
  return std::ranges::count_if(samples, isParent);
}

auto findClosestParent() noexcept {
  auto parent = std::ranges::find_if(samples, isParent);
  if (parent != std::end(samples)) {
    const auto isCloser = [](const auto &s1, const auto &s2) noexcept -> bool {
      return s1.fStartTime >= s2.fStartTime;
    };
    for (auto i = parent; i < std::end(samples); ++i) {
      if (isParent(*i) && isCloser(*i, *parent)) {
        parent = i; // Replace with more immediate parent
      }
    }
  }
  return parent;
}


Profiler::Profiler() noexcept { _startTime = GetExactTime(); }
void Profiler::Begin(std::string_view name) {
  auto it = std::ranges::find_if(
      samples, [&name](auto &sample) { return sample.szName == name; });
  if (it == std::end(samples)) {
    samples.push_back(Sample(name));
    return;
  }
  it->iOpenProfiles++;
  it->iProfileInstances++;
  it->fStartTime = GetExactTime();
}

void Profiler::End(std::string_view name) noexcept {
  const auto &it = std::ranges::find_if(
      samples, [name](const auto &sample) { return name == sample.szName; });
  if (it == std::end(samples)) {
    assert(!"End() called without a Begin()");
    return;
  }
  const float fEndTime = GetExactTime();
  const float dt = fEndTime - it->fStartTime;
  it->fAccumulator += dt;
  it->iOpenProfiles--;
  it->iNumParents = countParents();
  if (hasParents(*it)) {
    const auto parent = findClosestParent();
    parent->fChildrenSampleTime += dt; // Record this time in fChildrenSampleTime (add it in)    
  }
}

void Profiler::Profile(void) {
  _endTime = GetExactTime();
  textBox.clear();
  textBox.append("  Ave :   Min :   Max :   # : Profile Name\n");
  textBox.append("--------------------------------------------\n");

  for (const auto &sample : samples) {
    assert_if_invalid(sample);
    const auto sampleTime = sample.fAccumulator - sample.fChildrenSampleTime;
    const auto percentTime = (sampleTime / (_endTime - _startTime)) * 100.0f;

    StoreProfileInHistory(sample.szName, percentTime);
    const auto [aveTime, minTime, maxTime] =
        GetProfileFromHistory(sample.szName);

    std::string ave;
    std::format_to_n(std::back_inserter(ave), 5, "{:.1f}", aveTime);
    const std::string min = std::format("{:.1f}", minTime);
    const std::string max = std::format("{:.1f}", maxTime);
    const std::string num = std::format("{}", sample.iProfileInstances);
    // sprintf(num, "%3d", g_sample.iProfileInstances);

    const std::string indentedName =
        std::string("\t", sample.iNumParents) + std::string(sample.szName);
    const std::string line = std::format("{:5} : {:5} : {:5} : {:3} : {}\n",
                                         ave, min, max, num, indentedName);

    textBox.append(line);
  }
  samples.clear();
  _startTime = GetExactTime();
}

void Profiler::StoreProfileInHistory(std::string_view name, float percent) {
  const float newRatio = std::min(0.8f * GetElapsedTime(), 1.0f); // limit to 1
  const auto it = std::ranges::find_if(
      history, [&name](const auto &sample) { return sample.szName == name; });

  if (it == std::end(history)) {
    history.push_back(SampleHistory(name, percent));
    return;
  }
  it->updateTimings(percent, newRatio);
}

Profiler::TimePerFrame
Profiler::GetProfileFromHistory(std::string_view name) noexcept {
  const auto parent = std::ranges::find_if(
      history, [&name](const auto &sample) { return sample.szName == name; });
  if (parent != std::end(history)) {
    return parent->times;
  }
  return {};
}

void Profiler::Draw(void) {  
    std::cout << textBox << "\n";  
}