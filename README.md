# Windows Playback Library

WPL is a C++ library to playback video files inside a normal Window on the Win32 operating system.

![alt tag](http://williamsamtaylor.co.uk/images/projects/wpl.png)

## Overview

The library is a simple wrapper on top of DirectShow and replaces a lot of boiler plate code with just a few simple classes. Below you can view a simple example where SDL2 is used for providing the basic Win32 window.

## Example

```c++

#include "SDL2/SDL.h"
#include "SDL2/SDL_syswm.h"
#include "../WPL/WPL.h"

using namespace std;
using namespace wpl;

int main(int argc, char * argv[])
{
	Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window * window = SDL_CreateWindow("Playback Demo", 100, 100, 800, 500, flags);
	SDL_bool exit = SDL_FALSE;

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);

	VideoPlayer videoPlayer(wmInfo.info.win.window);
	videoPlayer.openVideo(L"demo.wmv");
	videoPlayer.play();

	while(!exit) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				exit = SDL_TRUE;
				break;
			}

			if(event.key.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
					case SDLK_RIGHT: videoPlayer.play(); break;
					case SDLK_LEFT: videoPlayer.pause(); break;
					case SDLK_DOWN: videoPlayer.stop(); break;

					default: break;
				}
			}
		}

		videoPlayer.updateVideoWindow();
		videoPlayer.repaint();
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

```

## Installation

If you would like to build the library you can download the project with a simple clone.

```git clone https://github.com/WilliamTaylor/WPL```

Once you have done that you can run the example or build the library yourself from inside Visual Studio.

## Features

* Load Avi/Wmv Video Files
* DirectX based drawing
* The ability to Pause, Stop and Resume Videos.

## Future Features

* Adjust the playback speed.
* Disable and control Audio.
* Set drawing region.

## Links

Find below some quick links to the technology used to build the application.

[SDL](https://www.libsdl.org/), [DirectX](https://www.microsoft.com/en-us/download/search.aspx?q=directx), [DirectShow](https://msdn.microsoft.com/en-us/library/windows/desktop/dd390351(v=vs.85).aspx)
