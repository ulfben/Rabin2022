#include "profile.h"
#include "custom_time.h"
#include <cassert>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
using namespace std::string_view_literals;
struct TimePerFrame {
    float average = .0f; // Average time per frame (percentage)
    float min = .0f;     // Minimum time per frame (percentage)
    float max = .0f;     // Maximum time per frame (percentage)   
  };  
struct Sample {
  std::string_view name;
  unsigned int count = 1; // # of times Profile::Begin called
  std::size_t parentCount = 0;        // Number of profile parents
  int openProfiles = 1;              // # of times Profile::Begin w/o ProfileEnd
  float startTime = 0.0f;            
  float accumulator = 0.0f;          // All samples this frame added together
  float childrensTime = 0.0f;       // Time taken by all nestled profiler calls  
  Sample(std::string_view name) noexcept : name{name}, startTime{GetExactTime()} {}
};
struct SampleHistory {
  std::string_view name; // Name of the sample
  TimePerFrame times{};
  SampleHistory(std::string_view name, float percent) noexcept : name{name}, times{percent, percent, percent} {}

  void updateTimings(float percent, float newRatio) noexcept {
    const float oldRatio = 1.0f - newRatio;
    const float percentNewRatio = (percent * newRatio);
    auto &[average, min, max] = times;
    average = (average * oldRatio) + percentNewRatio;
    min = (percent < min) ? percent : (min * oldRatio) + percentNewRatio;
    min = (min < 0.0f) ? 0.0f : min;  //don't allow negative values
    max = (percent > max) ? percent : (max * oldRatio) + percentNewRatio;
  }
};

constexpr std::string_view header(" Ave  :  Min  :  Max  : #   : Profile Name\n------------------------------------------\n");
std::vector<Sample> samples;
std::vector<SampleHistory> history;
float _startTime = 0.0f;
float _endTime = 0.0f;
std::string textBox = ""; //stand-in for the original codes' GUI window.

void assert_if_invalid(const Sample &s) noexcept {
  if (s.openProfiles < 0) {
    assert(!"Profile::End() called without a Profile::Begin()");
  } else if (s.openProfiles > 0) {
    assert(!"Profile::Begin() called without a Profile::End()");
  }
}
std::string indent_n(size_t levels, std::string_view s) {        
    return std::string("\t", levels) + std::string(s);
}
auto findSample(std::string_view name) noexcept {
    return std::ranges::find_if(samples, [name](const auto &sample) { return sample.name == name; });
}
auto findHistory(std::string_view name) noexcept {
    return std::ranges::find_if(history, [name](const auto &sample) { return sample.name == name; });
}    
bool hasParents(const Sample &s) noexcept { return s.parentCount > 0; }
bool isParent(const Sample &s) noexcept { return s.openProfiles > 0; }
size_t countParents() noexcept { return std::ranges::count_if(samples, isParent); }

auto findClosestParent() noexcept {
  auto parent = std::ranges::find_if(samples, isParent);
  if (parent != std::end(samples)) {
    const auto isCloser = [](const auto &s1, const auto &s2) noexcept -> bool {
      return s1.startTime >= s2.startTime;
    };
    for (auto i = parent; i < std::end(samples); ++i) {
      if (isParent(*i) && isCloser(*i, *parent)) {
        parent = i; // Replace with more immediate parent
      }
    }
  }
  return parent;
}

void storeProfile(std::string_view name, float percent) { 
  const auto profile = findHistory(name);
  if (profile == std::end(history)) {
    history.push_back(SampleHistory(name, percent));
    return;
  }
  const float newRatio = std::min(0.8f * GetElapsedTime(), 1.0f); // limit to 1
  profile->updateTimings(percent, newRatio);
}

TimePerFrame getTimings(std::string_view name) noexcept {
  const auto profile = findHistory(name);
  if (profile != std::end(history)) {
    return profile->times;
  }
  return {};
}

void Profiler::Init() noexcept { _startTime = GetExactTime(); }
void Profiler::Begin(std::string_view name) {
  assert(_startTime && "Profile::Begin() called without calling Profiler::Init() first!");
  auto it = findSample(name);
  if (it == std::end(samples)) {
    samples.push_back(Sample(name));
    return;
  }
  it->openProfiles++;
  it->count++;
  it->startTime = GetExactTime();
}

void Profiler::End(std::string_view name) noexcept {
  const auto sample = findSample(name);
  if (sample == std::end(samples)) {
    assert(!"End() called without a Begin()");
    return;
  }
  const float endTime = GetExactTime();
  const float dt = endTime - sample->startTime;
  sample->accumulator += dt;
  sample->openProfiles--;
  sample->parentCount = countParents();
  if (hasParents(*sample)) {
    const auto parent = findClosestParent();
    parent->childrensTime += dt; // Record this time in fChildrenSampleTime (add it in)    
  }
}

void Profiler::Profile(void) {
  _endTime = GetExactTime();
  textBox.clear();
  textBox.append(header);
  for (const auto &sample : samples) {
    assert_if_invalid(sample);
    const auto sampleTime = sample.accumulator - sample.childrensTime;
    const auto percentTime = (sampleTime / (_endTime - _startTime)) * 100.0f;
    storeProfile(sample.name, percentTime);
    const auto [aveTime, minTime, maxTime] = getTimings(sample.name);          
    textBox += std::format("{:5.1f} : {:5.1f} : {:5.1f} : {:3} : {}\n"sv,
                            aveTime,  minTime,  maxTime, sample.count, indent_n(sample.parentCount, sample.name));    
  }
  samples.clear();
  _startTime = GetExactTime();
}

void Profiler::Draw(void) {  
    std::cout << textBox << "\n";  
}