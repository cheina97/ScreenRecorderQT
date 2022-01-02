#include "GetAudioDevices.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#ifdef __linux__
using namespace std;
#elif defined _WIN32
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>

#include <dshow.h>
#include <objbase.h>
#include <msxml.h>
#pragma comment(lib, "strmiids.lib")
#endif

#ifdef _WIN32
HRESULT EnumerateDevices(REFGUID category, IEnumMoniker** ppEnum)
{
    // Create the System Device Enumerator.
    ICreateDevEnum* pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
        hr = pDevEnum->CreateClassEnumerator(category, ppEnum, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
        pDevEnum->Release();
    }
    return hr;
}

void DisplayDeviceInformation(IEnumMoniker* pEnum, std::vector<std::string>* devices)
{
    IMoniker* pMoniker = NULL;

    while (pEnum->Next(1, &pMoniker, NULL) == S_OK)
    {
        IPropertyBag* pPropBag;
        HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
        if (FAILED(hr))
        {
            pMoniker->Release();
            continue;
        }

        VARIANT var;
        VariantInit(&var);

        // Get description or friendly name.
        hr = pPropBag->Read(L"Description", &var, 0);
        if (FAILED(hr))
        {
            hr = pPropBag->Read(L"FriendlyName", &var, 0);
        }
        if (SUCCEEDED(hr))
        {
            std::wstring ws(var.bstrVal, SysStringLen(var.bstrVal));
            std::string str(ws.begin(), ws.end());
            devices->push_back(str);
            VariantClear(&var);
        }

        hr = pPropBag->Write(L"FriendlyName", &var);

        pPropBag->Release();
        pMoniker->Release();
    }
}
#endif

std::vector<std::string> getAudioDevices() {
    std::vector<std::string> devices;
#ifdef __linux__
    std::cout << "Starting" << std::endl;
    filesystem::directory_iterator alsaDir{"/proc/asound"};
    regex findCard{".*card(0|[1-9]?[0-9]*)"};
    regex findPcm{".*pcm.*"};
    std::string value, field, card, device, streamType;

    for (auto i : alsaDir) {
        if (regex_match(i.path().c_str(), findCard)) {
            filesystem::directory_iterator cardDir{i.path()};
            for (auto const& j : cardDir) {
                if (regex_match(j.path().c_str(), findPcm)) {
                    ifstream info{(std::string)j.path() + "/info"};
                    if (info.is_open()) {
                        while (!info.eof()) {
                            info >> field;
                            info >> value;
                            if (field == "card:") {
                                card = value;
                            } else if (field == "device:") {
                                device = value;
                            } else if (field == "stream:") {
                                streamType = value;
                            }
                        }
                        if (streamType == "CAPTURE") {
                            devices.emplace_back("hw:" + card + "," + device);
                        }
                    } else {
                        throw runtime_error{"Error opening device file"};
                    }
                }
            }
        }
    }

#elif  defined _WIN32
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        IEnumMoniker* pEnum;

        hr = EnumerateDevices(CLSID_AudioInputDeviceCategory, &pEnum);
        if (SUCCEEDED(hr))
        {
            DisplayDeviceInformation(pEnum, &devices);
            pEnum->Release();
        }
        CoUninitialize();
    }

#endif
    return devices;
}
