# Windows Playback Library

WPL is a simple and small C++ library to play back video files inside a normal Win32 window on the Windows operating system from Windows 7 and up.

![alt tag](http://williamsamtaylor.co.uk/images/projects/wpl.png)

## Overview

The library is based on DirectShow and replaces a lot of boiler plate code with just a few simple functions. Below you can view a simple example where SDL is used for providing the basic Win32 window.

All function prefixed with WPL are functions that below to this library. Likewise any function starting with SDL is part of the SDL library.

## Example

```c++
#include <iostream>
#include <SDL.h>
#include "WPL.h"

#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "WPL.lib")

int main(int argc, char * argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_bool exit = SDL_FALSE;
	SDL_Window * window = SDL_CreateWindow("demo",
		SDL_WINDOWPOS_CENTERED,	SDL_WINDOWPOS_CENTERED,
		800, 500, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
	);

	SDL_GL_CreateContext(window);

	WPL_Video * video = WPL_OpenVideo("demo.wmv");
	if (video == NULL)
		std::cout << WPL_GetError(video) << endl;

	while(!exit) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				exit = SDL_TRUE;
				break;
			} else if(event.key.type == SDL_KEYUP)	{
				switch (event.key.keysym.sym) {
					case SDLK_RIGHT: WPL_PlayVideo(video); break;
					case SDLK_LEFT: WPL_PauseVideo(video); break;
					case SDLK_DOWN: WPL_StopVideo(video); break;

					default: break;
				}
			}
		}

		WPL_ShowVideo(video);
	}

	WPL_ExitVideo(&video);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return(0);
}
```

## Installation

If you would like to build the library you can download the project with a simple clone.

```git clone https://github.com/WilliamTaylor/WPL```

Once you have done that you can run the example or build the library yourself for your app.  Remember it’s only for Windows 7 and above.

## Features

* Load Avi/Wmv Video Files
* DirectX based drawing
* The ability to Pause, Stop and Resume Videos.

## Future Features

* Bug fixes regarding to the rendering of videos.
* Adjust the playback speed.
* Disable and control Audio.
* Set drawing region.

## Links

Find below some quick links to the technology used to build the application.

[SDL](https://www.libsdl.org/), [DirectX](https://www.microsoft.com/en-us/download/search.aspx?q=directx), [DirectShow](https://msdn.microsoft.com/en-us/library/windows/desktop/dd390351(v=vs.85).aspx)
