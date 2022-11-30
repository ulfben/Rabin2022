/* Copyright (C) Steve Rabin, 2000.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Steve Rabin, 2000"
 */

#pragma once
#include <string_view>

class Profiler {
public:
  struct TimePerFrame {
    float average = .0f; // Average time per frame (percentage)
    float min = .0f;     // Minimum time per frame (percentage)
    float max = .0f;     // Maximum time per frame (percentage)
    void setAll(float v) noexcept { average = min = max = v; }
  };

  Profiler() noexcept;
  void Begin(std::string_view name);
  void End(std::string_view name) noexcept;
  void Profile(void);
  void Draw();

private:
  void StoreProfileInHistory(std::string_view name, float percent);
  TimePerFrame GetProfileFromHistory(std::string_view name) noexcept;
};


