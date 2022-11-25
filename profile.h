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
  void StoreProfileInHistory(std::string_view name, float percent);
  void GetProfileFromHistory(std::string_view name, float *ave, float *min,
                             float *max);

public:
  Profiler();
  void Begin(std::string_view name);
  void End(std::string_view name);
  void Profile(void);

  void Draw();
};
