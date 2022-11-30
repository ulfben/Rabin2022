#define _CRT_SECURE_NO_WARNINGS
#include "profile.h"
#include "custom_time.h"
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include <cassert>
#include <stdio.h>
#include <string.h>
struct Sample {
  std::string szName;
  unsigned int iProfileInstances = 0; // # of times ProfileBegin called
  int iOpenProfiles = 0;              // # of times ProfileBegin w/o ProfileEnd
  float fStartTime = 0.0f;            // The current open profile start time
  float fAccumulator = 0.0f;          // All samples this frame added together
  float fChildrenSampleTime = 0.0f;   // Time taken by all children
  unsigned int iNumParents = 0;       // Number of profile parents
  Sample(std::string_view name)
      : szName{name}, iOpenProfiles{1}, iProfileInstances{1},
        fStartTime{GetExactTime()} {}
};

struct SampleHistory {
  std::string szName; // Name of the sample
  Profiler::TimePerFrame times{};
  SampleHistory(std::string_view name, float percent) : szName{name} {
    times.setAll(percent);
  }

  void updateTimings(float percent, float newRatio) {
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

Profiler::Profiler() { _startTime = GetExactTime(); }
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

void Profiler::End(std::string_view name) {
  unsigned int i = 0;
  unsigned int numParents = 0;

  while (i < std::size(samples)) {
    if (name == samples[i].szName) {
      unsigned int inner = 0;
      int parent = -1;
      float fEndTime = GetExactTime();
      samples[i].iOpenProfiles--;

      // Count all parents and find the immediate parent
      while (inner < std::size(samples)) {
        if (samples[inner].iOpenProfiles >
            0) { // Found a parent (any open profiles are parents)
          numParents++;
          if (parent < 0) { // Replace invalid parent (index)
            parent = inner;
          } else if (samples[inner].fStartTime >=
                     samples[parent]
                         .fStartTime) { // Replace with more immediate parent
            parent = inner;
          }
        }
        inner++;
      }

      // Remember the current number of parents of the sample
      samples[i].iNumParents = numParents;

      if (parent >= 0) { // Record this time in fChildrenSampleTime (add it in)
        samples[parent].fChildrenSampleTime += fEndTime - samples[i].fStartTime;
      }

      // Save sample time in accumulator
      samples[i].fAccumulator += fEndTime - samples[i].fStartTime;
      return;
    }
    i++;
  }
}
void Profiler::Profile(void) {
  unsigned int i = 0;

  _endTime = GetExactTime();

  textBox.clear();

  textBox.append("  Ave :   Min :   Max :   # : Profile Name\n");
  textBox.append("--------------------------------------------\n");

  while (i < std::size(samples)) {
    if (samples[i].iOpenProfiles < 0) {
      assert(!"ProfileEnd() called without a ProfileBegin()");
    } else if (samples[i].iOpenProfiles > 0) {
      assert(!"ProfileBegin() called without a ProfileEnd()");
    }
    const auto sampleTime =
        samples[i].fAccumulator - samples[i].fChildrenSampleTime;
    const auto percentTime = (sampleTime / (_endTime - _startTime)) * 100.0f;

    // Add new measurement into the history and get the ave, min, and max
    StoreProfileInHistory(samples[i].szName, percentTime);

    auto [aveTime, minTime, maxTime] = GetProfileFromHistory(samples[i].szName);

    std::string ave;
    std::format_to_n(std::back_inserter(ave), 5, "{:.1f}", aveTime);
    const std::string min = std::format("{:.1f}", minTime);
    const std::string max = std::format("{:.1f}", maxTime);
    const std::string num = std::format("{}", samples[i].iProfileInstances);
    // sprintf(num, "%3d", g_samples[i].iProfileInstances);

    const std::string indentedName =
        std::string("\t", samples[i].iNumParents) + samples[i].szName;
    const std::string line = std::format("{:5} : {:5} : {:5} : {:3} : {}\n",
                                         ave, min, max, num, indentedName);

    textBox.append(line); // Send the line to text buffer
    i++;
  }

  { // Reset samples for next frame
    samples.clear();
    _startTime = GetExactTime();
  }
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
Profiler::TimePerFrame Profiler::GetProfileFromHistory(std::string_view name) {
  const auto it = std::ranges::find_if(
      history, [&name](const auto &sample) { return sample.szName == name; });

  if (it != std::end(history)) {
    return it->times;
  }

  return {};
}
void Profiler::Draw(void) {
  if (!textBox.empty()) {
    std::cout << textBox << "\n";
  }
}