
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

// Helper functions for graphs and filters
bool AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName);
bool InitializeEVR(IBaseFilter *pEVR, HWND hwnd, IMFVideoDisplayControl ** ppWc);
bool FindConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
bool RemoveUnconnectedRenderer(IGraphBuilder *pGraph, IBaseFilter *pRenderer);

bool RemoveUnconnectedRenderer(IGraphBuilder * graphBuilder, IBaseFilter * baseFilter, bool * removed)
{
    IPin * pinPointer = nullptr;
    (*removed) = false;
    auto result = FindConnectedPin(baseFilter, PINDIR_INPUT, &pinPointer);
    safeRelease(&pinPointer);
    
    if (FAILED(result)) {
        result = SUCCEEDED(graphBuilder->RemoveFilter(baseFilter));
        (*removed) = TRUE;
    }

    return result;
}

bool IsPinConnected(IPin * pinPointer, bool * resultPointer)
{
    IPin * tempPinPointer = nullptr;
    auto hr = pinPointer->ConnectedTo(&tempPinPointer);
    
    if (SUCCEEDED(hr))  {
        *resultPointer = true;
    } else if (hr == VFW_E_NOT_CONNECTED) {
        *resultPointer = true;
        hr = S_OK;
    }

    safeRelease(&tempPinPointer);
    return SUCCEEDED(hr);
}

bool IsPinDirection(IPin * pinPointer, PIN_DIRECTION dir, BOOL * result)
{
    PIN_DIRECTION pinDirection;
    auto hr = pinPointer->QueryDirection(&pinDirection);

    if (SUCCEEDED(hr))
    {
        (*result) = (pinDirection == dir);
    }

    return SUCCEEDED(hr);
}

bool FindConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    *ppPin = nullptr;
    IEnumPins * pEnum = nullptr;
    IPin * pPin = nullptr;

    auto  hr = pFilter->EnumPins(&pEnum);

    if (FAILED(hr))
    {
        return SUCCEEDED(hr);
    }

    auto bFound = FALSE;

    while (S_OK == pEnum->Next(1, &pPin, nullptr))
    {
        bool bIsConnected;
        hr = IsPinConnected(pPin, &bIsConnected);
        if (SUCCEEDED(hr)) {
            if (bIsConnected) {
                hr = IsPinDirection(pPin, PinDir, &bFound);
            }
        }

        if (FAILED(hr)) {
            pPin->Release();
            break;
        }

        if (bFound) {
            *ppPin = pPin;
            break;
        }

        pPin->Release();
    }

    pEnum->Release();

    if (!bFound) {
        hr = VFW_E_NOT_FOUND;
    }

    return SUCCEEDED(hr);
}

bool AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName)
{
    *ppF = nullptr;

    IBaseFilter * pFilter = nullptr;

    auto hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFilter));
    
    if (FAILED(hr))
        goto done;

    hr = pGraph->AddFilter(pFilter, wszName);
    
    if (FAILED(hr))
        goto done;

    *ppF = pFilter;
    (*ppF)->AddRef();

done:
    safeRelease(&pFilter);
    return SUCCEEDED(hr);
}

bool RemoveUnconnectedRenderer(IGraphBuilder *pGraph, IBaseFilter *pRenderer)
{
    IPin * pinPointer = nullptr;

    HRESULT hr = FindConnectedPin(pRenderer, PINDIR_INPUT, &pinPointer);

    pinPointer->Release();

    if (FAILED(hr))
    {
        hr = pGraph->RemoveFilter(pRenderer);
    }

    return SUCCEEDED(hr);
}

Version WPL_GetVersion()
{
    Version v;
    v.majorVersion = MAJOR_VERSION;
    v.minorVersion = MINOR_VERSION;
    return v;
}

EVR::EVR() 
    : videoDisplay(nullptr), evr(nullptr)
{
}

EVR::~EVR()
{
    safeRelease(&evr);
    safeRelease(&videoDisplay);
}

bool EVR::hasVideo() const
{
    return (videoDisplay != nullptr);
}

bool EVR::addToGraph(IGraphBuilder *pGraph, HWND hwnd)
{
    IBaseFilter *pEVR = nullptr;

    auto hr = AddFilterByCLSID(pGraph, CLSID_EnhancedVideoRenderer, &pEVR, L"EVR");

    if (FAILED(hr))
        goto done;

    InitializeEVR(pEVR, hwnd, &videoDisplay);

    if (FAILED(hr))
        goto done;

    evr = pEVR;
    evr->AddRef();

done:
    safeRelease(&pEVR);
    return(SUCCEEDED(hr));
}

bool EVR::finalizeGraph(IGraphBuilder *pGraph)
{
    if (evr == nullptr) {
        return true;
    }

    bool bRemoved;
    auto hr = RemoveUnconnectedRenderer(pGraph, evr, &bRemoved);

    if (bRemoved) {
        safeRelease(&evr);
        safeRelease(&videoDisplay);
    }

    return(SUCCEEDED(hr));
}

bool EVR::updateVideoWindow(HWND hwnd, const LPRECT prc)
{
    if (videoDisplay == nullptr) {
        return true; // no-op
    }

    if (prc) {
        return SUCCEEDED(videoDisplay->SetVideoPosition(nullptr, prc));
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    return SUCCEEDED(videoDisplay->SetVideoPosition(nullptr, &rc));
}

bool EVR::repaint()
{
    return SUCCEEDED(videoDisplay ? videoDisplay->RepaintVideo() : S_OK);
}

// Initialize the EVR filter. 
bool InitializeEVR(IBaseFilter *pEVR, HWND hwnd, IMFVideoDisplayControl** ppDisplayControl)
{
    IMFGetService *pGS = nullptr;
    IMFVideoDisplayControl *pDisplay = nullptr;

    auto hr = pEVR->QueryInterface(IID_PPV_ARGS(&pGS));

    if (FAILED(hr))
        goto done;

    hr = pGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplay));
    
    if (FAILED(hr))
        goto done;

    hr = pDisplay->SetVideoWindow(hwnd);
    
    if (FAILED(hr))
        goto done;

    hr = pDisplay->SetAspectRatioMode(MFVideoARMode_None);

    if (FAILED(hr))
        goto done;

    *ppDisplayControl = pDisplay;
    (*ppDisplayControl)->AddRef();

done:
    safeRelease(&pGS);
    safeRelease(&pDisplay);
    return SUCCEEDED(hr);
}


VideoPlayer::VideoPlayer(HWND hwnd)
  : state(PlaybackState::NoVideo),
    windowHandle(hwnd),
    graphBuilder(nullptr),
    mediaControl(nullptr),
    mediaEvents(nullptr),
    videoRenderer(nullptr)
{
}

VideoPlayer::~VideoPlayer()
{
    releaseGraph();
}

bool VideoPlayer::openVideo(const std::string& filename)
{
    if(!fileExists(filename)) {
        return false;
    }

    IBaseFilter *pSource = nullptr;

    auto hr = setupGraph();

    if (!hr)
        goto done;

    auto nameLength = filename.length() + 1;
    auto widthLength = MultiByteToWideChar(CP_ACP, 0, filename.c_str(), nameLength, nullptr, 0);
    auto wideBuffer = new wchar_t[widthLength];

    MultiByteToWideChar(CP_ACP, 0, filename.c_str(), nameLength, wideBuffer, widthLength);

    hr = SUCCEEDED(graphBuilder->AddSourceFilter(wideBuffer, nullptr, &pSource));

    if (!hr)
        goto done;

    hr = renderStreams(pSource);
done:
    if (!hr)
        releaseGraph();

    safeRelease(&pSource);
    delete[] wideBuffer;
    return hr;
}

bool VideoPlayer::play()
{
    if (state != PlaybackState::Paused && state != PlaybackState::Stopped) {
        return false;
    }

    updateVideoWindow();

    auto hr = mediaControl->Run();

    if (SUCCEEDED(hr))
        state = PlaybackState::Playing;

    return state == PlaybackState::Playing;
}

bool VideoPlayer::pause()
{
    if (state != PlaybackState::Playing) {
        return false;
    }

    auto hr = mediaControl->Pause();
    
    if (SUCCEEDED(hr)) {
        state = PlaybackState::Paused;
    }

    return state == PlaybackState::Paused;
}

bool VideoPlayer::stop()
{
    if (state != PlaybackState::Playing && state != PlaybackState::Paused) {
        return false;
    }

    auto hr = mediaControl->Stop();

    if (SUCCEEDED(hr)){
        state = PlaybackState::Stopped;
        
        LONGLONG StopTimes = 1;
        LONGLONG Start = 1;

        mediaSeeking->GetPositions(nullptr, &StopTimes);
        mediaSeeking->SetPositions(&Start, 0x01 | 0x04, &StopTimes, 0x01 | 0x04);
    }
    
    return state == PlaybackState::Stopped;
}

bool VideoPlayer::hasVideo() const
{
    return videoRenderer && videoRenderer->hasVideo();
}

bool VideoPlayer::hasFinished() const
{
    auto returnValue = false;

    if(mediaEvents == nullptr) {
        return returnValue;
    }

    long param1 = 0, param2 = 0;
    long evCode = 0;

    while (SUCCEEDED(mediaEvents->GetEvent(&evCode, &param1, &param2, 0)))
    {
        auto hr = mediaEvents->FreeEventParams(evCode, param1, param2);
        
        if (FAILED(hr))
            break;

        if (evCode == EC_COMPLETE) {
            returnValue = true;
            break;
        }
    }

    return returnValue;
}

bool VideoPlayer::updateVideoWindow() const
{
    RECT rc;
    GetClientRect(this->windowHandle, &rc);
    return SUCCEEDED(videoRenderer ? videoRenderer->updateVideoWindow(windowHandle, &rc) : S_OK);
}

bool VideoPlayer::repaint() const
{
    return SUCCEEDED(videoRenderer ? videoRenderer->repaint() : S_OK);
}

bool VideoPlayer::setupGraph()
{
    releaseGraph();

    auto hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&graphBuilder));

    if (FAILED(hr))
        goto done;

    hr = graphBuilder->QueryInterface(IID_PPV_ARGS(&mediaControl));

    if (FAILED(hr))
        goto done;

    hr = graphBuilder->QueryInterface(IID_PPV_ARGS(&mediaEvents));

    if (FAILED(hr))
        goto done;

    hr = graphBuilder->QueryInterface(IID_PPV_ARGS(&mediaSeeking));

    if (FAILED(hr))
        goto done;

    state = PlaybackState::Stopped;
done:
    return SUCCEEDED(hr);
}

void VideoPlayer::releaseGraph()
{
    safeRelease(&graphBuilder);
    safeRelease(&mediaControl);
    safeRelease(&mediaEvents);
    safeDelete(&videoRenderer);

    state = PlaybackState::NoVideo;
}

PlaybackState VideoPlayer::playbackState() const 
{
    return state;
}

bool VideoPlayer::createVideoRenderer()
{
    auto hr = E_FAIL;
    videoRenderer = new (std::nothrow) EVR();

    if (videoRenderer == nullptr) {
        hr = E_OUTOFMEMORY;
        return false;
    }

    hr = videoRenderer->addToGraph(graphBuilder, windowHandle);
    
    if (FAILED(hr)) {
        safeDelete(&videoRenderer);
    }

    return SUCCEEDED(hr);
}

bool VideoPlayer::renderStreams(IBaseFilter *pSource)
{
    auto bRenderedAnyPin {false};

    IFilterGraph2 * pGraph2 = nullptr;
    IEnumPins * pEnum = nullptr;
    IBaseFilter * pAudioRenderer = nullptr;
    
    auto hr = graphBuilder->QueryInterface(IID_PPV_ARGS(&pGraph2));

    if (FAILED(hr))
        goto done;

    hr = createVideoRenderer();

    if (FAILED(hr))
        goto done;

    hr = AddFilterByCLSID(graphBuilder, CLSID_DSoundRender, &pAudioRenderer, L"Audio Renderer");

    if (FAILED(hr))
        goto done;

    // Enumerate the pins on the source filter.
    hr = pSource->EnumPins(&pEnum);

    if (FAILED(hr))
        goto done;

    IPin * pPin;
    while (S_OK == pEnum->Next(1, &pPin, nullptr))
    {
        auto hr2 = pGraph2->RenderEx(pPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, nullptr);
        pPin->Release();
        if (SUCCEEDED(hr2))
        {
            bRenderedAnyPin = TRUE;
        }
    }

    hr = videoRenderer->finalizeGraph(graphBuilder);

    if (FAILED(hr))
        goto done;

    // Remove the audio renderer, if not used.
    bool bRemoved;
    hr = RemoveUnconnectedRenderer(graphBuilder, pAudioRenderer, &bRemoved);
done:
    safeRelease(&pEnum);
    safeRelease(&pAudioRenderer);
    safeRelease(&pGraph2);

    if (SUCCEEDED(hr)) {
        if (!bRenderedAnyPin) {
            //hr = VFW_E_CANNOT_RENDER;
            hr = -1;
        }
    }

    return SUCCEEDED(hr);
}