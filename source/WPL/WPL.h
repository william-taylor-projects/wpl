
/**
 *
 * Copyright (c) 2014 : William Taylor : wi11berto@yahoo.co.k
 *
 * This software is provided 'as-is', without any
 * express or implied warranty. In no event will
 * the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented;
 *    you must not claim that you wrote the original software.
 *    If you use this software in a product, an acknowledgment
 *    in the product documentation would be appreciated but
 *    is not required.
 *
 * 2. Altered source versions must be plainly marked as such,
 *    and must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef __WPL_VIDEO_H__
#define __WPL_VIDEO_H__

// Win32 only supported
#ifdef WIN32
	// DLL Export import statement which is found in every dynamic library
	#ifdef WPL_API_EXPORT
		#define WPL_API __declspec(dllexport)
	#else
		#define WPL_API __declspec(dllimport)
	#endif

/** Required Windows librarys */
#include <windows.h>
#include <dshow.h>
#include <string>
#include <d3d9.h>
#include <Evr.h>

#pragma comment(lib, "strmiids.lib")

/** An enum for telling the videos state when being played */
enum WPL_Video_State : unsigned int {
    STATE_NO_GRAPH,
    STATE_RUNNING,
    STATE_STOPPED,
	STATE_PAUSED,
};

/* The video itself and all the controls that govern it */
struct WPL_Video {
	/** The controller for the display which the video is rendered on */
	IMFVideoDisplayControl * videoDisplayControl;
	/** The constructor class which builds the graph/display */
	IGraphBuilder * graphBuilder;
	/** The media control for the video stream */
	IMediaControl * mediaControl;
	/** The media stream that controls the videos position */
	IMediaSeeking * mediaSeeking;
	/** The controller for media events being sent to the window */
	IMediaEventEx * mediaEvents;
	/** The filter being user for the video renderer */
	IBaseFilter * evrBaseFilter;
	/** The current state of the video */
	WPL_Video_State playbackState;
	/** The play it was loaded from */
	std::wstring filename;
	/** A handle to the active window */
	HWND wndHwnd;
	/** not used yet */
	BOOL show;
};

enum WPL_ERROR {
	FILE_NOT_FOUND = 0
};

/** Data type which holds all the data on the library's version */
struct WPL_Version {
	/** The major/release number for the library */
	unsigned int majorVersion;
	/** The minor/patch number for the library */
	unsigned int minorVersion;
	/** The librarys version as a string */
	std::string versionString;
};

/** This function opens a WMV or AVI file to be rendered any other files will be rejected */
WPL_API WPL_Video * WPL_OpenVideo(std::string filename);
/** This function simply returns the librarys version */
WPL_API WPL_Version WPL_GetVersion();
/** This function pauses the video */
WPL_API void WPL_PauseVideo(WPL_Video* video);
/** This function removes and deletes the video */
WPL_API void WPL_ExitVideo(WPL_Video** video);
/** This function stops the stream */
WPL_API void WPL_StopVideo(WPL_Video* video);
/** This function starts the video */
WPL_API void WPL_PlayVideo(WPL_Video* video);
/** This function should be called in your main loop */
WPL_API void WPL_ShowVideo(WPL_Video* video);
/** This function will return the error as a string which you can print */
WPL_API std::string WPL_GetError(WPL_Video* video);

#endif

#endif
