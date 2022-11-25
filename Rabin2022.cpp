#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "winmm")
#include "custom_time.h"
#include "profile.h"
#include <chrono>
#include <thread>
using namespace std::literals;

Profiler profiler;

void input() {  }
void update() { std::this_thread::sleep_for(200ms); }
void render() {
  std::this_thread::sleep_for(400ms);
  profiler.Draw(); // output profiling text from last frame
}

int main() {
  bool exitGame = false;
  while (!exitGame) {
    profiler.Begin("Main Game Loop");
        input();
        update();
        profiler.Begin("Graphics Draw Routine");
            render();
        profiler.End("Graphics Draw Routine");
    profiler.End("Main Game Loop");
    
    profiler.Profile(); // buffer will be drawn next frame
  }
  return 0;
}