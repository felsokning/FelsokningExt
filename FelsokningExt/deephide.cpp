#include "pch.h"
#include "helpers.h"
#include <wrl/client.h> // For Microsoft::WRL::ComPtr

#pragma warning(disable : 4996) // _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

using namespace std;
using Microsoft::WRL::ComPtr;

const char deepHideUsage[] = "FelsokningExt by John Bailey\n\tUsage: !deephide <size> [-q] [-s symbol]\n\tExample:\n\t\t!deephide 43 -s hostfxr!execute_app\n\t\t!deephide 43 -q -s hostfxr!execute_app\n\n";

HRESULT CALLBACK deephide(_In_ PDEBUG_CLIENT8 pDebugClient, _In_opt_ PCSTR args)
{
    auto status = S_OK;
    helpers h{};
    helpers* internalHelper = &h;
    wchar_t wideArgs[128] = { 0 };
    size_t convertedChars = 0;
    wstring target;
    bool quietMode = false;
    ULONG targetSize = 0;

    if (SUCCEEDED(internalHelper->IsUserMode(pDebugClient)))
    {
        ComPtr<IDebugControl7> pDebugControl;
        if (!SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl7), &pDebugControl)))
        {
            return E_FAIL;
        }

        // Parse arguments
        if (SUCCEEDED(mbstowcs_s(&convertedChars, wideArgs, _countof(wideArgs), args, _TRUNCATE)))
        {
            if (convertedChars > 0)
            {
                std::wistringstream wiss(wideArgs);
                std::vector<std::wstring> wv(std::istream_iterator<std::wstring, wchar_t>(wiss), {});
                size_t wvCapacity = wv.capacity();

                if (wvCapacity < 2)
                {
                    pDebugControl->Output(DEBUG_OUTPUT_NORMAL, deepHideUsage);
                    return S_OK;
                }

                // First argument should be the size
                targetSize = _wtoi(wv[0].c_str());
                if (targetSize == 0)
                {
                    pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "Invalid size parameter.\n");
                    return S_OK;
                }

                // Parse options
                for (size_t i = 1; i < wvCapacity; i++)
                {
                    if (wv[i] == L"-q")
                    {
                        quietMode = true;
                    }
                    else if (wv[i] == L"-s" && i + 1 < wvCapacity)
                    {
                        target = wv[i + 1];
                        i++; // Skip the next argument as it's the symbol
                    }
                }

                if (target.empty())
                {
                    pDebugControl->Output(DEBUG_OUTPUT_NORMAL, "Symbol parameter is required.\n");
                    pDebugControl->Output(DEBUG_OUTPUT_NORMAL, deepHideUsage);
                    return S_OK;
                }
            }
            else
            {
                pDebugControl->Output(DEBUG_OUTPUT_NORMAL, deepHideUsage);
                return S_OK;
            }
        }

        ComPtr<IDebugSymbols5> pDebugSymbols;
        ComPtr<IDebugSystemObjects4> pDebugSystemObjects;

        if (!SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugSymbols5), &pDebugSymbols)) ||
            !SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugSystemObjects4), &pDebugSystemObjects)))
        {
            return E_FAIL;
        }

        ULONG numberOfThreads = 0;
        if (SUCCEEDED(pDebugSystemObjects->GetNumberThreads(&numberOfThreads)))
        {
            std::vector<ULONG> Ids(numberOfThreads);
            std::vector<ULONG> SysIds(numberOfThreads);
            
            if (SUCCEEDED(pDebugSystemObjects->GetThreadIdsByIndex(0, numberOfThreads, Ids.data(), SysIds.data())))
            {
                ULONG currentThreadId = 0;
                if (SUCCEEDED(pDebugSystemObjects->GetCurrentThreadId(&currentThreadId)))
                {
                    for (ULONG iterationThreads = 0; iterationThreads < numberOfThreads; iterationThreads++)
                    {
                        if (SUCCEEDED(pDebugSystemObjects->SetCurrentThreadId(Ids[iterationThreads])))
                        {
                            ULONG maxStackDepth = 250;
                            ULONG frames = 0;
                            std::vector<DEBUG_STACK_FRAME> stackFrames(maxStackDepth);
                            
                            if (SUCCEEDED(pDebugControl->GetStackTrace(0, 0, 0, stackFrames.data(), maxStackDepth, &frames)))
                            {
                                if (frames > targetSize)
                                {
                                    // Search for symbol in stack frames
                                    char symName[4096];
                                    ULONG symSize = 0;
                                    ULONG64 displacement = 0;
                                    bool foundSymbol = false;

                                    for (const DEBUG_STACK_FRAME& frame : stackFrames)
                                    {
                                        if (SUCCEEDED(pDebugSymbols->GetNameByOffset(frame.InstructionOffset, symName, 4096, &symSize, &displacement)))
                                        {
                                            wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
                                            wstring ws = converter.from_bytes(symName);
                                            if (ws.find(target) != std::string::npos)
                                            {
                                                foundSymbol = true;
                                                break;
                                            }
                                        }
                                    }

                                    // If we didn't find the symbol, output the stack trace
                                    if (!foundSymbol)
                                    {
                                        if (!quietMode)
                                        {
                                            pDebugControl->ControlledOutput(DEBUG_OUTCTL_DML, DEBUG_OUTPUT_NORMAL, 
                                                "Thread Id: <link cmd=\"~%llus\">%llu</link> (Stack depth: %lu)\n", 
                                                Ids[iterationThreads], Ids[iterationThreads], frames);
                                        }
                                        pDebugControl->OutputStackTrace(DEBUG_OUTPUT_NORMAL, stackFrames.data(), frames, 
                                            DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FRAME_ADDRESSES | DEBUG_STACK_SOURCE_LINE | DEBUG_STACK_FRAME_NUMBERS);
                                    }
                                }
                            }
                        }
                    }

                    // Reset thread context to original
                    if (!SUCCEEDED(pDebugSystemObjects->SetCurrentThreadId(currentThreadId)))
                    {
                        pDebugControl->Output(DEBUG_OUTPUT_WARNING, "Unable to return to original thread context.\n");
                    }
                }
            }
        }
    }

    return status;
}