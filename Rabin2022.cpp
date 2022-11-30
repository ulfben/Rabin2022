#pragma comment(lib, "winmm")
#include "custom_time.h"
#include "profile.h"
#include <chrono>
#include <thread>
using namespace std::literals;
bool input() noexcept { return false; }
void update() { std::this_thread::sleep_for(200ms); }
void render() {
  std::this_thread::sleep_for(400ms);
  Profiler::Draw(); // output profiling text from last frame
}

int main() {
  InitTime();
  Profiler::Init();
  bool exitGame = false;
  while (!exitGame) {
    Profiler::Begin("Main Game Loop");
        exitGame = input();
        update();
        Profiler::Begin("Graphics Draw Routine");
            render();
        Profiler::End("Graphics Draw Routine");
    Profiler::End("Main Game Loop");
    
    Profiler::Profile(); // buffer will be drawn next frame
  }
  return 0;
}