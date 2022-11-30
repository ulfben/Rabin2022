#pragma comment(lib, "winmm")
#include "custom_time.h"
#include "profile.h"
#include <chrono>
#include <thread>
using namespace std::literals;
bool input()  { std::this_thread::sleep_for(100ms); return false; }
void update() { std::this_thread::sleep_for(200ms); }
void render() { std::this_thread::sleep_for(400ms); }

int main() {
  InitTime();
  Profiler::Init();
  bool exitGame = false;
  while (!exitGame) {
    MarkTimeThisTick();    
    Profiler::Begin("Main Game Loop");    
            exitGame = input();
        Profiler::Begin("Update");
            update();
        Profiler::End("Update");
        Profiler::Begin("Render");
            render();
        Profiler::End("Render");   
    Profiler::End("Main Game Loop");    
    
    Profiler::Profile(); // buffer will be drawn next frame
    Profiler::Draw();
  }
  return 0;
}