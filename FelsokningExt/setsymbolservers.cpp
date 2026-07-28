// setsymbolservers.cpp — Add downstream symbol servers via .sympath -p srve<cache;url>;... form
// Implements Windbg extension command: !setsymbolservers <absolute-cache-path>

#include "pch.h"
#include "helpers.h"
#include <wrl/client.h>
#include <string>
#include <vector>
#include <format>

using namespace std;
using Microsoft::WRL::ComPtr;

static constexpr const wchar_t* kServers[] = {
    L"https://msdl.microsoft.com/download/symbols",
    L"https://chromium-browser-symsrv.commondatastorage.googleapis.com/msdl",
    L"https://symbols.mozilla.org/",
    L"https://symbolserver.unity3d.com/",
    L"https://ctxsym.citrix.com/symbols/download",
    L"https://software.intel.com/sites/downloads/symbols/microsoft",
    L"https://driver-symbols.nvidia.com/symbolCache",
    L"https://download.amd.com/dir/bin",
    L"https://symbols.nuget.org/download/symbols",
    L"http://symbols.autodesk.com/symbols",
    L"http://symbolserver.unity3d.com/"
};

inline static std::string TrimArgs(PCSTR args) {
    if (!args || !*args) return {};

    // Skip leading whitespace
    size_t start = strspn(args, " \t\r\n");
    if (!args[start]) return {}; // Only whitespace or empty

    // Find end (trim trailing)
    size_t end = strlen(args);
    while (end > start && isspace(static_cast<unsigned char>(args[end - 1]))) --end;

    return std::string(args + start, end - start);
}

HRESULT __stdcall setsymbolservers(_In_ PDEBUG_CLIENT8 pDebugClient, _In_opt_ PCSTR args) {
    if (!pDebugClient) return E_POINTER;

    helpers h{};
    helpers* internalHelper = &h;

    if (SUCCEEDED(internalHelper->IsUserMode(pDebugClient)))
    {
        ComPtr<IDebugControl7> pDebugControl;
        HRESULT hr = pDebugClient->QueryInterface(__uuidof(IDebugControl7), &pDebugControl);
        if (FAILED(hr)) return hr;

        // 1. Build compound path string (.sympath natively supports semicolon-delimited arrays)
        std::string cmdBuf = ".sympath ";
        std::string trimmedArgs = TrimArgs(args);
        std::string urlSuffix;
        for (size_t i = 0; i < _countof(kServers); ++i) {
            if (i > 0) urlSuffix += ';';

            std::wstring wUrl(kServers[i]);
            int bytes = WideCharToMultiByte(CP_UTF8, 0, wUrl.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (bytes <= 0) { hr = E_FAIL; break; }

            std::string narrow(bytes, '\0');
            WideCharToMultiByte(CP_UTF8, 0, wUrl.c_str(), -1, &narrow[0], bytes, nullptr, nullptr);
            if (!narrow.empty() && narrow.back() == '\0') narrow.pop_back();
            if (!trimmedArgs.empty())
            {
				auto result = std::format("srv*{}*{}", trimmedArgs, narrow);
				urlSuffix += result;
            }
            else
            {
                urlSuffix += narrow;
            }
        }

        cmdBuf += urlSuffix;

        // 2. Execute as a single debug command. 
        // DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_NOT_LOGGED keeps it silent & session-bound.
        if (SUCCEEDED(hr)) {
            hr = pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_NOT_LOGGED, cmdBuf.c_str(), 0);
        }

        return hr; // S_OK = path set successfully. Symbol download status is irrelevant here.
    }
    else
    {
		return S_FALSE; // Not user-mode, command not applicable.
    }
}
