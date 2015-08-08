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

#include "WPL.h"
#include <fstream>

#define MAJOR_VERSION 1
#define MINOR_VERSION 0

HRESULT FindConnectedPin(IBaseFilter *, PIN_DIRECTION, IPin **);

bool is_file_exists(const char * filename)
{
	return std::ifstream(filename).good();
}

WPL_Version WPL_GetVersion()
{
	WPL_Version v;

	v.versionString = std::to_string(MAJOR_VERSION) + "." + std::to_string(MINOR_VERSION);
	v.majorVersion = MAJOR_VERSION;
	v.minorVersion = MINOR_VERSION;
	
	return v;
}

void InitializeEVR(IBaseFilter * pEVR, HWND hwnd, IMFVideoDisplayControl ** ppDisplayControl) 
{ 
	IMFVideoDisplayControl * Display = NULL;
	IMFGetService * pGS = NULL;

    pEVR->QueryInterface(IID_PPV_ARGS(&pGS)); 
	pGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&Display));

    Display->SetVideoWindow(hwnd);
    Display->SetAspectRatioMode(0);

    *ppDisplayControl = Display;
    (*ppDisplayControl)->AddRef();

	if(pGS != NULL)
	{
		pGS->Release();
	}

	if (Display != NULL)
	{
		Display->Release();
	}
} 

HRESULT RemoveUnconnectedRenderer(IGraphBuilder *pGraph, IBaseFilter *pRenderer)
{
    IPin *pPin = NULL;

    HRESULT hr = FindConnectedPin(pRenderer, PINDIR_INPUT, &pPin);

    pPin->Release();

    if (FAILED(hr))
    {
        hr = pGraph->RemoveFilter(pRenderer);
    }

    return hr;
}

HRESULT IsPinConnected(IPin *pPin, BOOL *pResult)
{
    IPin *pTmp = NULL;
    HRESULT hr = pPin->ConnectedTo(&pTmp);
    
	if (SUCCEEDED(hr))
    {
        *pResult = TRUE;
    }
    else if (hr == VFW_E_NOT_CONNECTED)
    {
        *pResult = FALSE;
        hr = S_OK;
    }

    pTmp->Release();
    return hr;
}

HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult)
{
    PIN_DIRECTION pinDir;
    HRESULT hr = pPin->QueryDirection(&pinDir);
    if (SUCCEEDED(hr))
    {
        *pResult = (pinDir == dir);
    }
    return hr;
}

HRESULT FindConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    *ppPin = NULL;

    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;

    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }

    BOOL bFound = FALSE;
    while (S_OK == pEnum->Next(1, &pPin, NULL))
    {
        BOOL bIsConnected;
        hr = IsPinConnected(pPin, &bIsConnected);
        if (SUCCEEDED(hr))
        {
            if (bIsConnected)
            {
                hr = IsPinDirection(pPin, PinDir, &bFound);
            }
        }

        if (FAILED(hr))
        {
            pPin->Release();
            break;
        }
        if (bFound)
        {
            *ppPin = pPin;
            break;
        }
        pPin->Release();
    }

    pEnum->Release();

    if (!bFound)
    {
        hr = VFW_E_NOT_FOUND;
    }
    return hr;
}

HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName)
{
    IBaseFilter * pFilter = NULL;
	*ppF = NULL;
    
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFilter));

    hr = pGraph->AddFilter(pFilter, wszName);
    *ppF = pFilter;
    (*ppF)->AddRef();

	pFilter->Release();

    return hr;
}

WPL_Video * WPL_OpenVideo(std::string filename)
{
	if (is_file_exists(filename.c_str()))
	{
		WPL_Video * video			= new WPL_Video();
		BOOL RenderedAnyPin		    = FALSE;
		IFilterGraph2 * Graph2		= NULL;
		IBaseFilter * AudioRenderer = NULL;
		IBaseFilter * pEVR			= NULL;
		IBaseFilter * Source		= NULL;
		IEnumPins * Enum			= NULL;
		IPin * Pin				    = NULL;

		int slength = (int)filename.length() + 1;
		
		int Length = MultiByteToWideChar(CP_ACP, 0, filename.c_str(), slength, 0, 0); 
		
		wchar_t * wideBuffer = new wchar_t[Length];
		
		MultiByteToWideChar(CP_ACP, 0, filename.c_str(), slength, wideBuffer, Length);

		video->playbackState = STATE_STOPPED;
		video->filename = std::wstring(wideBuffer);
		video->wndHwnd = GetActiveWindow();
		
		CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&video->graphBuilder));

		video->graphBuilder->QueryInterface(IID_IMediaControl, (void**)&video->mediaControl);
		video->graphBuilder->QueryInterface(IID_IMediaEventEx, (void**)&video->mediaEvents);
		video->graphBuilder->QueryInterface(IID_IMediaSeeking, (void**)&video->mediaSeeking);

		video->graphBuilder->AddSourceFilter(video->filename.c_str(), NULL, &Source);
		video->graphBuilder->QueryInterface(IID_PPV_ARGS(&Graph2));
		video->show = TRUE;

		AddFilterByCLSID(video->graphBuilder, CLSID_EnhancedVideoRenderer, &pEVR, L"EVR");

		InitializeEVR(pEVR, video->wndHwnd, &video->videoDisplayControl);

		video->evrBaseFilter = pEVR;
		video->evrBaseFilter->AddRef();

		AddFilterByCLSID(video->graphBuilder, CLSID_DSoundRender, &AudioRenderer, L"Audio Renderer");

		Source->EnumPins(&Enum);

		while(S_OK == Enum->Next(1, &Pin, NULL))
		{           
			HRESULT Result = Graph2->RenderEx(Pin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);

			Pin->Release();
       
			if(SUCCEEDED(Result)) 
			{
				RenderedAnyPin = TRUE;
			}
		}

		RemoveUnconnectedRenderer(video->graphBuilder, video->evrBaseFilter);
		RemoveUnconnectedRenderer(video->graphBuilder, AudioRenderer);

		AudioRenderer->Release();
		Graph2->Release();
		Source->Release();
		Enum->Release();
		pEVR->Release();

		delete[] wideBuffer;
		
		return video;
	}
	else
	{
		return (int)FILE_NOT_FOUND;
	}
};

void WPL_PauseVideo(WPL_Video* video)
{
	if(video != NULL)
	{
		if(video->playbackState == STATE_RUNNING)
		{
			video->playbackState = STATE_PAUSED;
			video->mediaControl->Pause();
		}
	}
}

WPL_API std::string WPL_GetError(WPL_Video * video)
{
	if (video == (int)FILE_NOT_FOUND)
	{
		return "The file could not be found";
	}
	else
	{
		return "File is OK";
	}
}

void WPL_StopVideo(WPL_Video* video)
{
	if(video != NULL)
	{
		if(video->playbackState == STATE_RUNNING || video->playbackState == STATE_PAUSED)
		{
			LONGLONG StopTimes = 1;
			LONGLONG Start = 1;

			video->playbackState = STATE_STOPPED;
			video->mediaControl->Stop();
			video->mediaSeeking->GetPositions(NULL, &StopTimes);
			video->mediaSeeking->SetPositions(&Start, 0x01 | 0x04, &StopTimes, 0x01 | 0x04);
		}
	}
}

void WPL_PlayVideo(WPL_Video* video)
{
	if(video != NULL)
	{
		PAINTSTRUCT ps;
		RECT rc;
		HDC hdc;

		GetClientRect(video->wndHwnd, &rc);
		video->videoDisplayControl->SetVideoPosition(NULL, &rc);

		hdc = BeginPaint(video->wndHwnd, &ps);

		if(video->playbackState != STATE_NO_GRAPH)
		{
			 video->videoDisplayControl->RepaintVideo();
		}
	
		EndPaint(video->wndHwnd, &ps);	

		if(video->playbackState == STATE_PAUSED || video->playbackState == STATE_STOPPED)
		{
			video->playbackState = STATE_RUNNING; 
			video->mediaControl->Run();
		}
	}
}

void WPL_ShowVideo(WPL_Video * video, RECT rc)
{
	if (video != NULL)
	{
		if (video->show == TRUE)
		{
			if (video->playbackState == STATE_PAUSED || video->playbackState == STATE_RUNNING)
			{
				video->videoDisplayControl->SetVideoPosition(NULL, &rc);
			}
		}
	}
}

void WPL_ShowVideo(WPL_Video * video)
{
	if (video != NULL)
	{
		RECT rc;
		GetClientRect(video->wndHwnd, &rc);
		WPL_ShowVideo(video, rc);
	}
}

void WPL_ExitVideo(WPL_Video ** video)
{
	if((*video) != NULL)
	{
		(*video)->videoDisplayControl->Release();
		(*video)->evrBaseFilter->Release();

		(*video)->mediaEvents->SetNotifyWindow((OAHWND)NULL, NULL, NULL);
		(*video)->mediaEvents->Release();

		(*video)->graphBuilder->Release();
		(*video)->mediaControl->Release();
		(*video)->mediaSeeking->Release();

		delete(*video);
		*video = NULL;
	}
}
