/* Copyright (C) Steve Rabin, 2000.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Steve Rabin, 2000"
 */
#include <time.h>
#include <windows.h>
float g_StartTime = -1.0f;
float g_CurrentTime = -1.0f;
float g_TimeLastTick = -1.0f;

void InitTime(void) noexcept {
  g_StartTime = static_cast<float>(timeGetTime()) / 1000.0f;
  g_CurrentTime = 0.0f;
  g_TimeLastTick = 0.001f;
}

void MarkTimeThisTick(void) noexcept {
  const float newTime =
      (static_cast<float>(timeGetTime()) / 1000.0f) - g_StartTime;
  g_TimeLastTick = newTime - g_CurrentTime;
  g_CurrentTime = newTime;
  if (g_TimeLastTick <= 0.0f) {
    g_TimeLastTick = 0.001f;
  }
}

float GetElapsedTime(void) noexcept { return g_TimeLastTick; }

float GetExactTime(void) noexcept {
  return static_cast<float>(timeGetTime()) / 1000.0f;
}