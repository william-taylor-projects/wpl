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

#define MAJOR_VERSION 2
#define MINOR_VERSION 0

bool fileExists(const std::string& filename)
{
	return std::ifstream(filename).good();
}

HRESULT InitializeEVR(IBaseFilter *pEVR, HWND hwnd, IMFVideoDisplayControl ** ppWc);
HRESULT InitWindowlessVMR9(IBaseFilter *pVMR, HWND hwnd, IVMRWindowlessControl9 ** ppWc);
HRESULT InitWindowlessVMR(IBaseFilter *pVMR, HWND hwnd, IVMRWindowlessControl** ppWc);
HRESULT FindConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);

// Helper functions for graphs and filters
HRESULT RemoveUnconnectedRenderer(IGraphBuilder * graphBuilder, IBaseFilter * baseFilter, bool * removed)
{
	IPin * pinPointer = nullptr;
	(*removed) = false;
	auto result = FindConnectedPin(baseFilter, PINDIR_INPUT, &pinPointer);
	SafeRelease(&pinPointer);
	
	if (FAILED(result)) {
		result = graphBuilder->RemoveFilter(baseFilter);
		(*removed) = TRUE;
	}

	return result;
}

HRESULT IsPinConnected(IPin * pinPointer, bool * resultPointer)
{
	IPin * tempPinPointer = nullptr;
	auto hr = pinPointer->ConnectedTo(&tempPinPointer);
	
	if (SUCCEEDED(hr)) 
	{
		*resultPointer = true;
	} 
	else if (hr == VFW_E_NOT_CONNECTED) 
	{
		*resultPointer = true;
		hr = S_OK;
	}

	SafeRelease(&tempPinPointer);
	return hr;
}

HRESULT IsPinDirection(IPin * pinPointer, PIN_DIRECTION dir, BOOL * result)
{
	PIN_DIRECTION pinDirection;
	auto hr = pinPointer->QueryDirection(&pinDirection);

	if (SUCCEEDED(hr))
	{
		(*result) = (pinDirection == dir);
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
		bool bIsConnected;
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
	*ppF = 0;

	IBaseFilter *pFilter = NULL;

	HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pFilter));
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pGraph->AddFilter(pFilter, wszName);
	if (FAILED(hr))
	{
		goto done;
	}

	*ppF = pFilter;
	(*ppF)->AddRef();

done:
	SafeRelease(&pFilter);
	return hr;
}

WPL_Version WPL_GetVersion()
{
	WPL_Version v;
	v.majorVersion = MAJOR_VERSION;
	v.minorVersion = MINOR_VERSION;
	return v;
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

WPL_Video * WPL_OpenVideo(std::string filename)
{
	if (fileExists(filename))
	{
		WPL_Video * video			= new WPL_Video();
		BOOL RenderedAnyPin		    = FALSE;
		IFilterGraph2 * Graph2		= nullptr;
		IBaseFilter * AudioRenderer = nullptr;
		IBaseFilter * pEVR			= nullptr;
		IBaseFilter * Source		= nullptr;
		IEnumPins * Enum			= nullptr;
		IPin * Pin				    = nullptr;

		int slength = filename.length() + 1;

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
				RenderedAnyPin = TRUE;
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
	
	return nullptr;
};

void WPL_PauseVideo(WPL_Video * video)
{
	if(video != nullptr)
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
	return "Hiya";
}

void WPL_StopVideo(WPL_Video* video)
{
	if(video != nullptr)
	{
		if(video->playbackState == STATE_RUNNING || video->playbackState == STATE_PAUSED)
		{
			LONGLONG StopTimes = 1;
			LONGLONG Start = 1;

			video->playbackState = STATE_STOPPED;
			video->mediaControl->Stop();
			video->mediaSeeking->GetPositions(nullptr, &StopTimes);
			video->mediaSeeking->SetPositions(&Start, 0x01 | 0x04, &StopTimes, 0x01 | 0x04);
		}
	}
}

void WPL_PlayVideo(WPL_Video* video)
{
	if(video != nullptr)
	{
		PAINTSTRUCT ps;
		RECT rc;
		HDC hdc;

		GetClientRect(video->wndHwnd, &rc);
		video->videoDisplayControl->SetVideoPosition(nullptr, &rc);

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
	if (video != nullptr)
	{
		if (video->show == TRUE)
		{
			if (video->playbackState == STATE_PAUSED || video->playbackState == STATE_RUNNING)
			{
				video->videoDisplayControl->SetVideoPosition(nullptr, &rc);
			}
		}
	}
}

void WPL_ShowVideo(WPL_Video * video)
{
	if (video != nullptr)
	{
		RECT rc;
		GetClientRect(video->wndHwnd, &rc);
		WPL_ShowVideo(video, rc);
	}
} 

void WPL_ExitVideo(WPL_Video ** video)
{
	if((*video) != nullptr)
	{
		(*video)->videoDisplayControl->Release();
		(*video)->evrBaseFilter->Release();

		(*video)->mediaEvents->SetNotifyWindow((OAHWND)nullptr, NULL, NULL);
		(*video)->mediaEvents->Release();

		(*video)->graphBuilder->Release();
		(*video)->mediaControl->Release();
		(*video)->mediaSeeking->Release();

		delete(*video);
		*video = nullptr;
	}
}

/// VMR-7 Wrapper

CVMR7::CVMR7() : m_pWindowless(nullptr)
{

}

CVMR7::~CVMR7()
{
	SafeRelease(&m_pWindowless);
}

BOOL CVMR7::HasVideo() const
{
	return (m_pWindowless != nullptr);
}

HRESULT CVMR7::AddToGraph(IGraphBuilder *pGraph, HWND hwnd)
{
	IBaseFilter *pVMR = nullptr;

	HRESULT hr = AddFilterByCLSID(pGraph, CLSID_VideoMixingRenderer, &pVMR, L"VMR-7");

	if (SUCCEEDED(hr))
	{
		// Set windowless mode on the VMR. This must be done before the VMR
		// is connected.
		hr = InitWindowlessVMR(pVMR, hwnd, &m_pWindowless);
	}
	SafeRelease(&pVMR);
	return hr;
}

HRESULT CVMR7::FinalizeGraph(IGraphBuilder *pGraph)
{
	if (m_pWindowless == nullptr)
	{
		return S_OK;
	}

	IBaseFilter *pFilter = nullptr;

	HRESULT hr = m_pWindowless->QueryInterface(IID_PPV_ARGS(&pFilter));
	if (FAILED(hr))
	{
		goto done;
	}

	BOOL bRemoved;
	hr = RemoveUnconnectedRenderer(pGraph, pFilter, &bRemoved);

	// If we removed the VMR, then we also need to release our 
	// pointer to the VMR's windowless control interface.
	if (bRemoved)
	{
		SafeRelease(&m_pWindowless);
	}

done:
	SafeRelease(&pFilter);
	return hr;
}

HRESULT CVMR7::UpdateVideoWindow(HWND hwnd, const LPRECT prc)
{
	if (m_pWindowless == nullptr)
	{
		return S_OK; // no-op
	}

	if (prc)
	{
		return m_pWindowless->SetVideoPosition(NULL, prc);
	}
	else
	{
		RECT rc;
		GetClientRect(hwnd, &rc);
		return m_pWindowless->SetVideoPosition(NULL, &rc);
	}
}

HRESULT CVMR7::Repaint(HWND hwnd, HDC hdc)
{
	if (m_pWindowless)
	{
		return m_pWindowless->RepaintVideo(hwnd, hdc);
	}
	else
	{
		return S_OK;
	}
}

HRESULT CVMR7::DisplayModeChanged()
{
	if (m_pWindowless)
	{
		return m_pWindowless->DisplayModeChanged();
	}
	else
	{
		return S_OK;
	}
}


// Initialize the VMR-7 for windowless mode. 

HRESULT InitWindowlessVMR(
	IBaseFilter *pVMR,              // Pointer to the VMR
	HWND hwnd,                      // Clipping window
	IVMRWindowlessControl** ppWC    // Receives a pointer to the VMR.
	)
{

	IVMRFilterConfig* pConfig = NULL;
	IVMRWindowlessControl *pWC = NULL;

	// Set the rendering mode.  
	HRESULT hr = pVMR->QueryInterface(IID_PPV_ARGS(&pConfig));
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pConfig->SetRenderingMode(VMRMode_Windowless);
	if (FAILED(hr))
	{
		goto done;
	}

	// Query for the windowless control interface.
	hr = pVMR->QueryInterface(IID_PPV_ARGS(&pWC));
	if (FAILED(hr))
	{
		goto done;
	}

	// Set the clipping window.
	hr = pWC->SetVideoClippingWindow(hwnd);
	if (FAILED(hr))
	{
		goto done;
	}

	// Preserve aspect ratio by letter-boxing
	hr = pWC->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
	if (FAILED(hr))
	{
		goto done;
	}

	// Return the IVMRWindowlessControl pointer to the caller.
	*ppWC = pWC;
	(*ppWC)->AddRef();

done:
	SafeRelease(&pConfig);
	SafeRelease(&pWC);
	return hr;
}


/// VMR-9 Wrapper

CVMR9::CVMR9() : m_pWindowless(NULL)
{

}

BOOL CVMR9::HasVideo() const
{
	return (m_pWindowless != NULL);
}

CVMR9::~CVMR9()
{
	SafeRelease(&m_pWindowless);
}

HRESULT CVMR9::AddToGraph(IGraphBuilder *pGraph, HWND hwnd)
{
	IBaseFilter *pVMR = NULL;

	HRESULT hr = AddFilterByCLSID(pGraph, CLSID_VideoMixingRenderer9,
		&pVMR, L"VMR-9");
	if (SUCCEEDED(hr))
	{
		// Set windowless mode on the VMR. This must be done before the VMR 
		// is connected.
		hr = InitWindowlessVMR9(pVMR, hwnd, &m_pWindowless);
	}
	SafeRelease(&pVMR);
	return hr;
}

HRESULT CVMR9::FinalizeGraph(IGraphBuilder *pGraph)
{
	if (m_pWindowless == NULL)
	{
		return S_OK;
	}

	IBaseFilter *pFilter = NULL;

	HRESULT hr = m_pWindowless->QueryInterface(IID_PPV_ARGS(&pFilter));
	if (FAILED(hr))
	{
		goto done;
	}

	BOOL bRemoved;
	hr = RemoveUnconnectedRenderer(pGraph, pFilter, &bRemoved);

	// If we removed the VMR, then we also need to release our 
	// pointer to the VMR's windowless control interface.
	if (bRemoved)
	{
		SafeRelease(&m_pWindowless);
	}

done:
	SafeRelease(&pFilter);
	return hr;
}


HRESULT CVMR9::UpdateVideoWindow(HWND hwnd, const LPRECT prc)
{
	if (m_pWindowless == NULL)
	{
		return S_OK; // no-op
	}

	if (prc)
	{
		return m_pWindowless->SetVideoPosition(NULL, prc);
	}
	else
	{

		RECT rc;
		GetClientRect(hwnd, &rc);
		return m_pWindowless->SetVideoPosition(NULL, &rc);
	}
}

HRESULT CVMR9::Repaint(HWND hwnd, HDC hdc)
{
	if (m_pWindowless)
	{
		return m_pWindowless->RepaintVideo(hwnd, hdc);
	}
	else
	{
		return S_OK;
	}
}

HRESULT CVMR9::DisplayModeChanged()
{
	if (m_pWindowless)
	{
		return m_pWindowless->DisplayModeChanged();
	}
	else
	{
		return S_OK;
	}
}


// Initialize the VMR-9 for windowless mode. 

HRESULT InitWindowlessVMR9(
	IBaseFilter *pVMR,              // Pointer to the VMR
	HWND hwnd,                      // Clipping window
	IVMRWindowlessControl9** ppWC   // Receives a pointer to the VMR.
	)
{

	IVMRFilterConfig9 * pConfig = NULL;
	IVMRWindowlessControl9 *pWC = NULL;

	// Set the rendering mode.  
	HRESULT hr = pVMR->QueryInterface(IID_PPV_ARGS(&pConfig));
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pConfig->SetRenderingMode(VMR9Mode_Windowless);
	if (FAILED(hr))
	{
		goto done;
	}

	// Query for the windowless control interface.
	hr = pVMR->QueryInterface(IID_PPV_ARGS(&pWC));
	if (FAILED(hr))
	{
		goto done;
	}

	// Set the clipping window.
	hr = pWC->SetVideoClippingWindow(hwnd);
	if (FAILED(hr))
	{
		goto done;
	}

	// Preserve aspect ratio by letter-boxing
	hr = pWC->SetAspectRatioMode(VMR9ARMode_LetterBox);
	if (FAILED(hr))
	{
		goto done;
	}

	// Return the IVMRWindowlessControl pointer to the caller.
	*ppWC = pWC;
	(*ppWC)->AddRef();

done:
	SafeRelease(&pConfig);
	SafeRelease(&pWC);
	return hr;
}


/// EVR Wrapper

CEVR::CEVR() : m_pEVR(NULL), m_pVideoDisplay(NULL)
{

}

CEVR::~CEVR()
{
	SafeRelease(&m_pEVR);
	SafeRelease(&m_pVideoDisplay);
}

BOOL CEVR::HasVideo() const
{
	return (m_pVideoDisplay != NULL);
}

HRESULT CEVR::AddToGraph(IGraphBuilder *pGraph, HWND hwnd)
{
	IBaseFilter *pEVR = NULL;

	HRESULT hr = AddFilterByCLSID(pGraph, CLSID_EnhancedVideoRenderer,
		&pEVR, L"EVR");

	if (FAILED(hr))
	{
		goto done;
	}

	InitializeEVR(pEVR, hwnd, &m_pVideoDisplay);
	if (FAILED(hr))
	{
		goto done;
	}

	// Note: Because IMFVideoDisplayControl is a service interface,
	// you cannot QI the pointer to get back the IBaseFilter pointer.
	// Therefore, we need to cache the IBaseFilter pointer.

	m_pEVR = pEVR;
	m_pEVR->AddRef();

done:
	SafeRelease(&pEVR);
	return hr;
}

HRESULT CEVR::FinalizeGraph(IGraphBuilder *pGraph)
{
	if (m_pEVR == NULL)
	{
		return S_OK;
	}

	BOOL bRemoved;
	HRESULT hr = RemoveUnconnectedRenderer(pGraph, m_pEVR, &bRemoved);
	if (bRemoved)
	{
		SafeRelease(&m_pEVR);
		SafeRelease(&m_pVideoDisplay);
	}
	return hr;
}

HRESULT CEVR::UpdateVideoWindow(HWND hwnd, const LPRECT prc)
{
	if (m_pVideoDisplay == NULL)
	{
		return S_OK; // no-op
	}

	if (prc)
	{
		return m_pVideoDisplay->SetVideoPosition(NULL, prc);
	}
	else
	{

		RECT rc;
		GetClientRect(hwnd, &rc);
		return m_pVideoDisplay->SetVideoPosition(NULL, &rc);
	}
}

HRESULT CEVR::Repaint(HWND hwnd, HDC hdc)
{
	if (m_pVideoDisplay)
	{
		return m_pVideoDisplay->RepaintVideo();
	}
	else
	{
		return S_OK;
	}
}

HRESULT CEVR::DisplayModeChanged()
{
	// The EVR does not require any action in response to WM_DISPLAYCHANGE.
	return S_OK;
}


// Initialize the EVR filter. 
HRESULT InitializeEVR(
	IBaseFilter *pEVR,              // Pointer to the EVR
	HWND hwnd,                      // Clipping window
	IMFVideoDisplayControl** ppDisplayControl
	)
{
	IMFGetService *pGS = NULL;
	IMFVideoDisplayControl *pDisplay = NULL;

	HRESULT hr = pEVR->QueryInterface(IID_PPV_ARGS(&pGS));
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplay));
	if (FAILED(hr))
	{
		goto done;
	}

	// Set the clipping window.
	hr = pDisplay->SetVideoWindow(hwnd);
	if (FAILED(hr))
	{
		goto done;
	}

	// Preserve aspect ratio by letter-boxing
	hr = pDisplay->SetAspectRatioMode(MFVideoARMode_PreservePicture);
	if (FAILED(hr))
	{
		goto done;
	}

	// Return the IMFVideoDisplayControl pointer to the caller.
	*ppDisplayControl = pDisplay;
	(*ppDisplayControl)->AddRef();

done:
	SafeRelease(&pGS);
	SafeRelease(&pDisplay);
	return hr;
}

DShowPlayer::DShowPlayer(HWND hwnd) :
	m_state(STATE_NO_GRAPH),
	m_hwnd(hwnd),
	m_pGraph(NULL),
	m_pControl(NULL),
	m_pEvent(NULL),
	m_pVideo(NULL)
{

}

DShowPlayer::~DShowPlayer()
{
	TearDownGraph();
}

// Open a media file for playback.
HRESULT DShowPlayer::OpenFile(PCWSTR pszFileName)
{
	IBaseFilter *pSource = NULL;

	// Create a new filter graph. (This also closes the old one, if any.)
	HRESULT hr = InitializeGraph();
	if (FAILED(hr))
	{
		goto done;
	}

	// Add the source filter to the graph.
	hr = m_pGraph->AddSourceFilter(pszFileName, NULL, &pSource);
	if (FAILED(hr))
	{
		goto done;
	}

	// Try to render the streams.
	hr = RenderStreams(pSource);

done:
	if (FAILED(hr))
	{
		TearDownGraph();
	}
	SafeRelease(&pSource);
	return hr;
}


// Respond to a graph event.
//
// The owning window should call this method when it receives the window
// message that the application specified when it called SetEventWindow.
//
// Caution: Do not tear down the graph from inside the callback.

HRESULT DShowPlayer::HandleGraphEvent(GraphEventFN pfnOnGraphEvent)
{
	if (!m_pEvent)
	{
		return E_UNEXPECTED;
	}

	long evCode = 0;
	LONG_PTR param1 = 0, param2 = 0;

	HRESULT hr = S_OK;

	// Get the events from the queue.
	while (SUCCEEDED(m_pEvent->GetEvent(&evCode, &param1, &param2, 0)))
	{
		// Invoke the callback.
		pfnOnGraphEvent(m_hwnd, evCode, param1, param2);

		// Free the event data.
		hr = m_pEvent->FreeEventParams(evCode, param1, param2);
		if (FAILED(hr))
		{
			break;
		}
	}
	return hr;
}

HRESULT DShowPlayer::Play()
{
	if (m_state != STATE_PAUSED && m_state != STATE_STOPPED)
	{
		return VFW_E_WRONG_STATE;
	}

	HRESULT hr = m_pControl->Run();
	if (SUCCEEDED(hr))
	{
		m_state = STATE_RUNNING;
	}
	return hr;
}

HRESULT DShowPlayer::Pause()
{
	if (m_state != STATE_RUNNING)
	{
		return VFW_E_WRONG_STATE;
	}

	HRESULT hr = m_pControl->Pause();
	if (SUCCEEDED(hr))
	{
		m_state = STATE_PAUSED;
	}
	return hr;
}

HRESULT DShowPlayer::Stop()
{
	if (m_state != STATE_RUNNING && m_state != STATE_PAUSED)
	{
		return VFW_E_WRONG_STATE;
	}

	HRESULT hr = m_pControl->Stop();
	if (SUCCEEDED(hr))
	{
		m_state = STATE_STOPPED;
	}
	return hr;
}


// EVR/VMR functionality

BOOL DShowPlayer::HasVideo() const
{
	return (m_pVideo && m_pVideo->HasVideo());
}

// Sets the destination rectangle for the video.
HRESULT DShowPlayer::UpdateVideoWindow(const LPRECT prc)
{
	if (m_pVideo)
	{
		return m_pVideo->UpdateVideoWindow(m_hwnd, prc);
	}
	else
	{
		return S_OK;
	}
}

// Repaints the video. Call this method when the application receives WM_PAINT.
HRESULT DShowPlayer::Repaint(HDC hdc)
{
	if (m_pVideo)
	{
		return m_pVideo->Repaint(m_hwnd, hdc);
	}
	else
	{
		return S_OK;
	}
}


// Notifies the video renderer that the display mode changed.
//
// Call this method when the application receives WM_DISPLAYCHANGE.
HRESULT DShowPlayer::DisplayModeChanged()
{
	if (m_pVideo)
	{
		return m_pVideo->DisplayModeChanged();
	}
	else
	{
		return S_OK;
	}
}


// Graph building

// Create a new filter graph. 
HRESULT DShowPlayer::InitializeGraph()
{
	TearDownGraph();

	// Create the Filter Graph Manager.
	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pGraph));

	if (FAILED(hr))
	{
		goto done;
	}

	hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pControl));
	if (FAILED(hr))
	{
		goto done;
	}

	hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pEvent));
	if (FAILED(hr))
	{
		goto done;
	}

	// Set up event notification.
	hr = m_pEvent->SetNotifyWindow((OAHWND)m_hwnd, WM_GRAPH_EVENT, NULL);
	if (FAILED(hr))
	{
		goto done;
	}

	m_state = STATE_STOPPED;

done:
	return hr;
}

void DShowPlayer::TearDownGraph()
{
	// Stop sending event messages
	if (m_pEvent)
	{
		m_pEvent->SetNotifyWindow((OAHWND)NULL, NULL, NULL);
	}

	SafeRelease(&m_pGraph);
	SafeRelease(&m_pControl);
	SafeRelease(&m_pEvent);

	delete m_pVideo;
	m_pVideo = NULL;

	m_state = STATE_NO_GRAPH;
}


HRESULT DShowPlayer::CreateVideoRenderer()
{
	HRESULT hr = E_FAIL;

	enum { Try_EVR, Try_VMR9, Try_VMR7 };

	for (DWORD i = Try_EVR; i <= Try_VMR7; i++)
	{
		switch (i)
		{
		case Try_EVR:
			m_pVideo = new (std::nothrow) CEVR();
			break;

		case Try_VMR9:
			m_pVideo = new (std::nothrow) CVMR9();
			break;

		case Try_VMR7:
			m_pVideo = new (std::nothrow) CVMR7();
			break;
		}

		if (m_pVideo == NULL)
		{
			hr = E_OUTOFMEMORY;
			break;
		}

		hr = m_pVideo->AddToGraph(m_pGraph, m_hwnd);
		if (SUCCEEDED(hr))
		{
			break;
		}

		delete m_pVideo;
		m_pVideo = NULL;
	}
	return hr;
}


// Render the streams from a source filter. 

HRESULT DShowPlayer::RenderStreams(IBaseFilter *pSource)
{
	BOOL bRenderedAnyPin = FALSE;

	IFilterGraph2 *pGraph2 = NULL;
	IEnumPins *pEnum = NULL;
	IBaseFilter *pAudioRenderer = NULL;
	HRESULT hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&pGraph2));
	if (FAILED(hr))
	{
		goto done;
	}

	// Add the video renderer to the graph
	hr = CreateVideoRenderer();
	if (FAILED(hr))
	{
		goto done;
	}

	// Add the DSound Renderer to the graph.
	hr = AddFilterByCLSID(m_pGraph, CLSID_DSoundRender,
		&pAudioRenderer, L"Audio Renderer");
	if (FAILED(hr))
	{
		goto done;
	}

	// Enumerate the pins on the source filter.
	hr = pSource->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		goto done;
	}

	// Loop through all the pins
	IPin *pPin;
	while (S_OK == pEnum->Next(1, &pPin, NULL))
	{
		// Try to render this pin. 
		// It's OK if we fail some pins, if at least one pin renders.
		HRESULT hr2 = pGraph2->RenderEx(pPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);

		pPin->Release();
		if (SUCCEEDED(hr2))
		{
			bRenderedAnyPin = TRUE;
		}
	}

	hr = m_pVideo->FinalizeGraph(m_pGraph);
	if (FAILED(hr))
	{
		goto done;
	}

	// Remove the audio renderer, if not used.
	BOOL bRemoved;
	hr = RemoveUnconnectedRenderer(m_pGraph, pAudioRenderer, &bRemoved);

done:
	SafeRelease(&pEnum);
	SafeRelease(&pAudioRenderer);
	SafeRelease(&pGraph2);

	// If we succeeded to this point, make sure we rendered at least one 
	// stream.
	if (SUCCEEDED(hr))
	{
		if (!bRenderedAnyPin)
		{
			hr = VFW_E_CANNOT_RENDER;
		}
	}
	return hr;
}