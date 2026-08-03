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
	// Microsoft symbol servers
    L"https://msdl.microsoft.com/download/symbols",
	// Chromium symbol servers
    L"https://chromium-browser-symsrv.commondatastorage.googleapis.com/msdl",
	// Mozilla symbol servers
    L"https://symbols.mozilla.org/",
	// Unity symbol servers
    L"https://symbolserver.unity3d.com/",
	// Citrix symbol servers
    L"https://ctxsym.citrix.com/symbols/download",
	// Intel symbol servers
    L"https://software.intel.com/sites/downloads/symbols/microsoft",
	// NVIDIA symbol servers
    L"https://driver-symbols.nvidia.com/symbolCache",
	// AMD symbol servers
    L"https://download.amd.com/dir/bin",
	// NuGet symbol servers
    L"https://symbols.nuget.org/download/symbols",
	// Autodesk symbol servers
    L"http://symbols.autodesk.com/symbols"
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

        // 1. Build compound path string (.sympath+ will append rather than overwrite)
        // + Specifies that the new locations will be appended to (rather than replace) the previous symbol search path.
        // See: https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/-sympath--set-symbol-path-
        std::string symPathCmdBuf = ".sympath+ ";
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

        symPathCmdBuf += urlSuffix;
		std::string symNoisyCmdBuffer = "!sym noisy";
		std::string reloadCmdBuffer = ".reload";
        // 2. Execute as a single debug command. 
        // DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_NOT_LOGGED keeps it silent & session-bound.
        if (SUCCEEDED(hr)) {
            hr = pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_NOT_LOGGED, symPathCmdBuf.c_str(), 0);
            if (SUCCEEDED(hr)) {
				hr = pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_NOT_LOGGED, symNoisyCmdBuffer.c_str(), 0);
                if (SUCCEEDED(hr)) {
                    hr = pDebugControl->Execute(DEBUG_OUTCTL_THIS_CLIENT | DEBUG_OUTCTL_NOT_LOGGED, reloadCmdBuffer.c_str(), 0);
                }
            }
        }

        return hr; // S_OK = path set successfully. Symbol download status is irrelevant here.
    }
    else
    {
		return S_FALSE; // Not user-mode, command not applicable.
    }
}
