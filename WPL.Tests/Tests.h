
#pragma once

#include <windows.h>//different header file in linux
#include <future>

#include "../WPL/WPL.h"
#include "../WPL.Sample/SDL2/SDL.h"
#include "../WPL.Sample/SDL2/SDL_syswm.h"

#pragma comment(lib, "../WPL.Sample/SDL2/SDL2main.lib")
#pragma comment(lib, "../WPL.Sample/SDL2/SDL2.lib")
#pragma comment(lib, "WPL.lib")

#define PLAYBACK_TIMEOUT 15000

template <typename... ParamTypes>
void setTimeout(int milliseconds, std::function<void()> func)
{
    std::async(std::launch::async, [=]()
    {
        Sleep(milliseconds);
        func();
    });
};

const auto timeout = [](Uint32 interval, void * ptr) -> Uint32 {
    SDL_Event e;
    e.type = SDL_QUIT;
    SDL_PushEvent(&e);
    return 0;
};