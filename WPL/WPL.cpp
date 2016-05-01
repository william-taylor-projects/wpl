
#include "WPL.h"
#include <fstream>

using namespace wpl;

#define MAJOR_VERSION 2
#define MINOR_VERSION 0

template<typename T> void safeRelease(T ** comPtr) {
	if (comPtr != nullptr && *comPtr) {
		(*comPtr)->Release();
		(*comPtr) = nullptr;
	}
}

template<typename T> void safeDelete(T ** savePointer) {
	if(savePointer != nullptr && savePointer) {
		delete (*savePointer);
		(*savePointer) = nullptr;
	}
}

template<typename T> bool fileExists(T filename) {
	return std::ifstream(filename).good();
}

HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName);
HRESULT InitializeEVR(IBaseFilter *pEVR, HWND hwnd, IMFVideoDisplayControl ** ppWc);
HRESULT FindConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT RemoveUnconnectedRenderer(IGraphBuilder *pGraph, IBaseFilter *pRenderer);

// Helper functions for graphs and filters
HRESULT RemoveUnconnectedRenderer(IGraphBuilder * graphBuilder, IBaseFilter * baseFilter, bool * removed)
{
	IPin * pinPointer = nullptr;
	(*removed) = false;
	auto result = FindConnectedPin(baseFilter, PINDIR_INPUT, &pinPointer);
	safeRelease(&pinPointer);
	
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

	safeRelease(&tempPinPointer);
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
	safeRelease(&pFilter);
	return hr;
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

Version WPL_GetVersion()
{
	Version v;
	v.majorVersion = MAJOR_VERSION;
	v.minorVersion = MINOR_VERSION;
	return v;
}

EVR::EVR() 
	: evr(nullptr), videoDisplay(nullptr)
{
}

EVR::~EVR()
{
	safeRelease(&evr);
	safeRelease(&videoDisplay);
}

BOOL EVR::hasVideo() const
{
	return (videoDisplay != nullptr);
}

HRESULT EVR::addToGraph(IGraphBuilder *pGraph, HWND hwnd)
{
	IBaseFilter *pEVR = nullptr;

	HRESULT hr = AddFilterByCLSID(pGraph, CLSID_EnhancedVideoRenderer, &pEVR, L"EVR");

	if (FAILED(hr))
	{
		goto done;
	}

	InitializeEVR(pEVR, hwnd, &videoDisplay);
	if (FAILED(hr))
	{
		goto done;
	}

	evr = pEVR;
	evr->AddRef();

done:
	safeRelease(&pEVR);
	return hr;
}

HRESULT EVR::finalizeGraph(IGraphBuilder *pGraph)
{
	if (evr == nullptr)
	{
		return S_OK;
	}

	bool bRemoved;
	HRESULT hr = RemoveUnconnectedRenderer(pGraph, evr, &bRemoved);
	if (bRemoved)
	{
		safeRelease(&evr);
		safeRelease(&videoDisplay);
	}
	return hr;
}

HRESULT EVR::updateVideoWindow(HWND hwnd, const LPRECT prc)
{
	if (videoDisplay == nullptr)
	{
		return S_OK; // no-op
	}

	if (prc)
	{
		return videoDisplay->SetVideoPosition(nullptr, prc);
	}
	else
	{

		RECT rc;
		GetClientRect(hwnd, &rc);
		return videoDisplay->SetVideoPosition(nullptr, &rc);
	}
}

HRESULT EVR::repaint()
{
	return videoDisplay ? videoDisplay->RepaintVideo() : S_OK;
}

// Initialize the EVR filter. 
HRESULT InitializeEVR(IBaseFilter *pEVR, HWND hwnd, IMFVideoDisplayControl** ppDisplayControl)
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

	hr = pDisplay->SetVideoWindow(hwnd);
	
	if (FAILED(hr))
	{
		goto done;
	}

	hr = pDisplay->SetAspectRatioMode(MFVideoARMode_None);

	if (FAILED(hr))
	{
		goto done;
	}

	// Return the IMFVideoDisplayControl pointer to the caller.
	*ppDisplayControl = pDisplay;
	(*ppDisplayControl)->AddRef();

done:
	safeRelease(&pGS);
	safeRelease(&pDisplay);
	return hr;
}



/* */
VideoPlayer::VideoPlayer(HWND hwnd)
  :	state(PlaybackState::NoVideo),
	windowHandle(hwnd),
	m_pGraph(nullptr),
	m_pControl(nullptr),
	m_pEvent(nullptr),
	videoRenderer(nullptr)
{
}

VideoPlayer::~VideoPlayer()
{
	releaseGraph();
}

// Open a media file for playback.
HRESULT VideoPlayer::openVideo(PCWSTR pszFileName)
{
	IBaseFilter *pSource = nullptr;

	// Create a new filter graph. (This also closes the old one, if any.)
	HRESULT hr = setupGraph();
	if (FAILED(hr))
	{
		goto done;
	}

	// Add the source filter to the graph.
	hr = m_pGraph->AddSourceFilter(pszFileName, nullptr, &pSource);
	if (FAILED(hr))
	{
		goto done;
	}

	// Try to render the streams.
	hr = renderStreams(pSource);

done:
	if (FAILED(hr))
	{
		releaseGraph();
	}
	safeRelease(&pSource);
	return hr;
}

HRESULT VideoPlayer::play()
{
	if (state != PlaybackState::Paused && state != PlaybackState::Stopped)
	{
		return VFW_E_WRONG_STATE;
	}

	updateVideoWindow();

	HRESULT hr = m_pControl->Run();

	if (SUCCEEDED(hr))
	{
		state = PlaybackState::Playing;
	}
	return hr;
}

HRESULT VideoPlayer::pause()
{
	if (state != PlaybackState::Playing)
	{
		return VFW_E_WRONG_STATE;
	}

	HRESULT hr = m_pControl->Pause();
	if (SUCCEEDED(hr))
	{
		state = PlaybackState::Paused;
	}
	return hr;
}

HRESULT VideoPlayer::stop()
{
	if (state != PlaybackState::Playing && state != PlaybackState::Paused)
	{
		return VFW_E_WRONG_STATE;
	}

	HRESULT hr = m_pControl->Stop();
	if (SUCCEEDED(hr))
	{
		state = PlaybackState::Stopped;
	}
	return hr;
}

BOOL VideoPlayer::hasVideo() const
{
	return (videoRenderer && videoRenderer->hasVideo());
}

HRESULT VideoPlayer::updateVideoWindow() const
{
	RECT rc;
	GetClientRect(this->windowHandle, &rc);
	return videoRenderer ? videoRenderer->updateVideoWindow(windowHandle, &rc) : S_OK;
}

HRESULT VideoPlayer::repaint() const
{
	return videoRenderer ? videoRenderer->repaint() : S_OK;
}

HRESULT VideoPlayer::setupGraph()
{
	releaseGraph();

	auto hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pGraph));

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

	state = PlaybackState::Stopped;
done:
	return hr;
}

void VideoPlayer::releaseGraph()
{
	safeRelease(&m_pGraph);
	safeRelease(&m_pControl);
	safeRelease(&m_pEvent);
	safeDelete(&videoRenderer);

	state = PlaybackState::NoVideo;
}

PlaybackState VideoPlayer::playbackState() const 
{
	return state;
}

HRESULT VideoPlayer::createVideoRenderer()
{
	auto hr = E_FAIL;
	videoRenderer = new (std::nothrow) EVR();

	if (videoRenderer == nullptr) {
		hr = E_OUTOFMEMORY;
		return E_FAIL;
	}

	hr = videoRenderer->addToGraph(m_pGraph, windowHandle);
	
	if (FAILED(hr)) {
		safeDelete(&videoRenderer);
	}

	return hr;
}

HRESULT VideoPlayer::renderStreams(IBaseFilter *pSource)
{
	BOOL bRenderedAnyPin = FALSE;

	IFilterGraph2 * pGraph2 = NULL;
	IEnumPins * pEnum = NULL;
	IBaseFilter * pAudioRenderer = NULL;
	HRESULT hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&pGraph2));

	if (FAILED(hr))
	{
		goto done;
	}

	hr = createVideoRenderer();
	if (FAILED(hr))
	{
		goto done;
	}

	// Add the DSound Renderer to the graph.
	hr = AddFilterByCLSID(m_pGraph, CLSID_DSoundRender, &pAudioRenderer, L"Audio Renderer");

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

	hr = videoRenderer->finalizeGraph(m_pGraph);
	if (FAILED(hr))
	{
		goto done;
	}

	// Remove the audio renderer, if not used.
	bool bRemoved;
	hr = RemoveUnconnectedRenderer(m_pGraph, pAudioRenderer, &bRemoved);

done:
	safeRelease(&pEnum);
	safeRelease(&pAudioRenderer);
	safeRelease(&pGraph2);

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