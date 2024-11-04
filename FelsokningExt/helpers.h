#pragma once

#include <winapifamily.h>
#include <Windows.h>
#include <DbgEng.h>

class helpers
{
public:
    helpers() = default;
    virtual ~helpers() = default;
    virtual HRESULT IsUserMode(THIS_ _In_ PDEBUG_CLIENT8 DebugClient)
    {
        auto Status = S_OK;
        ULONG Class;
        ULONG Qualifier;

        IDebugControl7* DebugControl = nullptr;
        __try {
            if ((Status = DebugClient->QueryInterface(__uuidof(IDebugControl7), (PVOID*)&DebugControl)) != S_OK)
            {
                __leave;
            }

            if ((Status = DebugControl->GetDebuggeeType(&Class, &Qualifier)) != S_OK)
            {
                DebugControl->Output(DEBUG_OUTPUT_NORMAL, "Could not get debuggee type.\n");
                __leave;
            }

            if (Class != DEBUG_CLASS_USER_WINDOWS)
            {

                DebugControl->Output(DEBUG_OUTPUT_WARNING, "FelsokningExt: Only works in user-mode debugging mode.\n");
                Status = S_FALSE;

                __leave;
            }
        }
        __finally {

            if (DebugControl) {

                DebugControl->Release();
            }
        }

        return Status;
    }
};