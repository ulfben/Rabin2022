#define _CRT_SECURE_NO_WARNINGS
#include "profile.h"
#include "custom_time.h"
#include <iostream>
#include <string>

#include <assert.h>
#include <stdio.h>
#include <string.h>
struct ProfileSample {
  bool bValid = false;            // Whether this data is valid
  unsigned int iProfileInstances; // # of times ProfileBegin called
  int iOpenProfiles;              // # of times ProfileBegin w/o ProfileEnd
  std::string szName;             // Name of sample
  float fStartTime;               // The current open profile start time
  float fAccumulator;             // All samples this frame added together
  float fChildrenSampleTime;      // Time taken by all children
  unsigned int iNumParents;       // Number of profile parents
};

struct ProfileSampleHistory{
  bool bValid;        // Whether the data is valid
  std::string szName; // Name of the sample
  Profiler::TimePerFrame times;
} ;
#define NUM_PROFILE_SAMPLES 50
ProfileSample g_samples[NUM_PROFILE_SAMPLES];
ProfileSampleHistory g_history[NUM_PROFILE_SAMPLES];
float g_startProfile = 0.0f;
float g_endProfile = 0.0f;
std::string textBox = "";

Profiler::Profiler() {
  unsigned int i;
  for (i = 0; i < NUM_PROFILE_SAMPLES; i++) {
    g_samples[i].bValid = false;
    g_history[i].bValid = false;
  }
  g_startProfile = GetExactTime();
}
void Profiler::Begin(std::string_view name) {

  unsigned int i = 0;
  while (i < NUM_PROFILE_SAMPLES && g_samples[i].bValid == true) {
    if (name == g_samples[i].szName) {
      // Found the sample
      g_samples[i].iOpenProfiles++;
      g_samples[i].iProfileInstances++;
      g_samples[i].fStartTime = GetExactTime();
      assert(g_samples[i].iOpenProfiles == 1); // max 1 open at once
      return;
    }
    i++;
  }

  if (i >= NUM_PROFILE_SAMPLES) {
    assert(!"Exceeded Max Available Profile Samples");
    return;
  }

  auto &sample = g_samples[i];
  sample.szName = name;
  sample.bValid = true;
  sample.iOpenProfiles = 1;
  sample.iProfileInstances = 1;
  sample.fAccumulator = 0.0f;
  sample.fStartTime = GetExactTime();
  sample.fChildrenSampleTime = 0.0f;
}
void Profiler::End(std::string_view name) {
  unsigned int i = 0;
  unsigned int numParents = 0;

  while (i < NUM_PROFILE_SAMPLES && g_samples[i].bValid == true) {
    if (name == g_samples[i].szName) {
      unsigned int inner = 0;
      int parent = -1;
      float fEndTime = GetExactTime();
      g_samples[i].iOpenProfiles--;

      // Count all parents and find the immediate parent
      while (g_samples[inner].bValid == true) {
        if (g_samples[inner].iOpenProfiles >
            0) { // Found a parent (any open profiles are parents)
          numParents++;
          if (parent < 0) { // Replace invalid parent (index)
            parent = inner;
          } else if (g_samples[inner].fStartTime >=
                     g_samples[parent]
                         .fStartTime) { // Replace with more immediate parent
            parent = inner;
          }
        }
        inner++;
      }

      // Remember the current number of parents of the sample
      g_samples[i].iNumParents = numParents;

      if (parent >= 0) { // Record this time in fChildrenSampleTime (add it in)
        g_samples[parent].fChildrenSampleTime +=
            fEndTime - g_samples[i].fStartTime;
      }

      // Save sample time in accumulator
      g_samples[i].fAccumulator += fEndTime - g_samples[i].fStartTime;
      return;
    }
    i++;
  }
}
void Profiler::Profile(void) {
  unsigned int i = 0;

  g_endProfile = GetExactTime();

  textBox.clear();

  textBox.append("  Ave :   Min :   Max :   # : Profile Name\n");
  textBox.append("--------------------------------------------\n");

  while (i < NUM_PROFILE_SAMPLES && g_samples[i].bValid == true) {
    unsigned int indent = 0;
    float sampleTime, percentTime;
    char line[256];
    std::string name, indentedName;
    char ave[16], min[16], max[16], num[16];

    if (g_samples[i].iOpenProfiles < 0) {
      assert(!"ProfileEnd() called without a ProfileBegin()");
    } else if (g_samples[i].iOpenProfiles > 0) {
      assert(!"ProfileBegin() called without a ProfileEnd()");
    }

    sampleTime = g_samples[i].fAccumulator - g_samples[i].fChildrenSampleTime;
    percentTime = (sampleTime / (g_endProfile - g_startProfile)) * 100.0f;

    // Add new measurement into the history and get the ave, min, and max
    StoreProfileInHistory(g_samples[i].szName, percentTime);
   
    auto [aveTime, minTime, maxTime] = GetProfileFromHistory(g_samples[i].szName);
    sprintf(ave, "%3.1f", aveTime);
    sprintf(min, "%3.1f", minTime);
    sprintf(max, "%3.1f", maxTime);
    sprintf(num, "%3d", g_samples[i].iProfileInstances);

    indentedName = g_samples[i].szName;
    for (indent = 0; indent < g_samples[i].iNumParents; indent++) {
      name = "     " + indentedName;
      indentedName = name;
    }
    
    sprintf(line, "%5s : %5s : %5s : %3s : %s\n", ave, min, max, num,
            indentedName.c_str());
    textBox.append(line); // Send the line to text buffer
    i++;
  }

  { // Reset samples for next frame
    unsigned int i;
    for (i = 0; i < NUM_PROFILE_SAMPLES; i++) {
      g_samples[i].bValid = false;
    }
    g_startProfile = GetExactTime();
  }
}
void Profiler::StoreProfileInHistory(std::string_view name, float percent) {
  unsigned int i = 0;
  float oldRatio;
  float newRatio = 0.8f * GetElapsedTime();
  if (newRatio > 1.0f) {
    newRatio = 1.0f;
  }
  oldRatio = 1.0f - newRatio;

  while (i < NUM_PROFILE_SAMPLES && g_history[i].bValid == true) {
    if (name == g_history[i].szName) { // Found the sample
      auto &sample = g_history[i];
        sample.times.average = (sample.times.average * oldRatio) + (percent * newRatio);
      if (percent < sample.times.min) {
        sample.times.min = percent;
      } else {
        sample.times.min =
            (sample.times.min * oldRatio) + (percent * newRatio);
      }

      if (sample.times.min < 0.0f) {
        sample.times.min = 0.0f;
      }

      if (percent > sample.times.max) {
        sample.times.max = percent;
      } else {
        sample.times.max =
            (sample.times.max * oldRatio) + (percent * newRatio);
      }
      return;
    }
    i++;
  }

  if (i < NUM_PROFILE_SAMPLES) { // Add to history
    g_history[i].szName = name;
    g_history[i].bValid = true;
    g_history[i].times.setAll(percent);
  } else {
    assert(!"Exceeded Max Available Profile Samples!");
  }
}
Profiler::TimePerFrame Profiler::GetProfileFromHistory(std::string_view name) {
  unsigned int i = 0;
  while (i < NUM_PROFILE_SAMPLES && g_history[i].bValid == true) {
    if (name == g_history[i].szName) { // Found the sample
      return g_history[i].times;      
    }
    i++;
  }
  return {};
}
void Profiler::Draw(void) {
  if (!textBox.empty()) {
    std::cout << textBox << "\n";
  }
}