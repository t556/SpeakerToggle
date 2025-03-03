// Small program to toggle sound devices. I have a shortcut on my windows desktop. This is to enable/disable my speakers quickly/easily. There is nothing worse than
// being in the office and my speakers are on and start playing sound, so I like to keep them disabled. However, sometimes I need them enabled for meetings. This program
// allows me to toggle it quickly.

#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <setupapi.h>
#include <cfgmgr32.h>  
#include <devguid.h>   


#pragma comment(lib, "setupapi.lib")

// Get device's 'friendly name/info'
BOOL GetDeviceDescription(
    HDEVINFO hDevInfo,
    SP_DEVINFO_DATA *pDevInfoData,
    char *buffer,
    DWORD bufferSize
)
{
    if (!SetupDiGetDeviceRegistryPropertyA(
            hDevInfo,
            pDevInfoData,
            SPDRP_DEVICEDESC,
            NULL,
            (PBYTE)buffer,
            bufferSize,
            NULL))
    {
        // If the description fails, find another way. 
        if (!SetupDiGetDeviceRegistryPropertyA(
                hDevInfo,
                pDevInfoData,
                SPDRP_FRIENDLYNAME,
                NULL,
                (PBYTE)buffer,
                bufferSize,
                NULL))
        {
            // We will never know
            snprintf(buffer, bufferSize, "Unknown Device");
            return FALSE;
        }
    }
    return TRUE;
}

// See if the device is enabled or disabled.
BOOL IsDeviceEnabled(
    SP_DEVINFO_DATA *pDevInfoData,
    ULONG *pStatus,
    ULONG *pProblem
)
{
    
    CONFIGRET cr = CM_Get_DevNode_Status(
        pStatus,
        pProblem,
        pDevInfoData->DevInst,
        0
    );
    if (cr != CR_SUCCESS)
    {
        return FALSE; // no luck. unknown/disabled.
    }

    BOOL disabled = ((*pStatus) & DN_HAS_PROBLEM) && ((*pProblem) == CM_PROB_DISABLED);
    return !disabled;
}

// This will toggle the device.
BOOL EnableDisableDevice(
    HDEVINFO hDevInfo,
    SP_DEVINFO_DATA *pDevInfoData,
    BOOL enable
)
{
    SP_PROPCHANGE_PARAMS propChangeParams;
    ZeroMemory(&propChangeParams, sizeof(propChangeParams));
    propChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    propChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    propChangeParams.StateChange = (enable ? DICS_ENABLE : DICS_DISABLE);
    propChangeParams.Scope       = DICS_FLAG_GLOBAL;
    propChangeParams.HwProfile   = 0;

    if (!SetupDiSetClassInstallParams(
            hDevInfo,
            pDevInfoData,
            &propChangeParams.ClassInstallHeader,
            sizeof(propChangeParams)))
    {
        printf("SetupDiSetClassInstallParams failed. GetLastError() = %ld\n", GetLastError());
        return FALSE;
    }

    // Apply
    if (!SetupDiCallClassInstaller(
            DIF_PROPERTYCHANGE,
            hDevInfo,
            pDevInfoData))
    {
        printf("SetupDiCallClassInstaller failed. GetLastError() = %ld\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

int main(void)
{
    // Get devices from "Sound, video and game controllers".
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(
        &GUID_DEVCLASS_MEDIA,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_PROFILE
    );

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        printf("SetupDiGetClassDevs failed. GetLastError() = %ld\n", GetLastError());
        return 1;
    }

    // Enumerate
    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    int index = 0; 
    BOOL moreDevices = TRUE;

    typedef struct _DEVICE_ENTRY {
        SP_DEVINFO_DATA devInfo;
        BOOL enabled;
        char name[512];
    } DEVICE_ENTRY;

    DEVICE_ENTRY deviceList[256];
    int deviceCount = 0;

    while (moreDevices)
    {
        if (!SetupDiEnumDeviceInfo(hDevInfo, index, &devInfoData))
        {
            DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_ITEMS)
            {
                moreDevices = FALSE;
                break;
            }
            else
            {
                printf("SetupDiEnumDeviceInfo() failure. Error:  %ld\n", err);
                break;
            }
        }

        // show name/description
        char nameBuf[512];
        GetDeviceDescription(hDevInfo, &devInfoData, nameBuf, sizeof(nameBuf));

        // Check status
        ULONG status = 0, problem = 0;
        BOOL isEnabled = IsDeviceEnabled(&devInfoData, &status, &problem);

       
        DEVICE_ENTRY *entry = &deviceList[deviceCount];
        entry->devInfo = devInfoData;
        entry->enabled = isEnabled;
        strncpy(entry->name, nameBuf, sizeof(entry->name));
        entry->name[sizeof(entry->name) - 1] = '\0'; 

        deviceCount++;
        index++;

        if (deviceCount >= 256)
        {
            break;
        }
    }

    // Show user the devices
    printf("Found %d sound devices':\n", deviceCount);
    for (int i = 0; i < deviceCount; i++)
    {
        printf("%d) %s [Status: %s]\n",
               i + 1,
               deviceList[i].name,
               deviceList[i].enabled ? "ENABLED" : "DISABLED");
    }

    if (deviceCount == 0)
    {        
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return 0;
    }

    // Enter Desired choice
    int choice = 0;
    printf("\nEnter Device# to toggle. 0 to exit.\n> ");
    if (scanf_s("%d", &choice) != 1)
    {
        printf("Invalid input.\n");
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return 0;
    }

    if (choice < 1 || choice > deviceCount)
    {
        printf("Exiting.\n");
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return 0;
    }

    // toggle the device
    DEVICE_ENTRY *chosen = &deviceList[choice - 1];
    BOOL newState = !chosen->enabled;

    printf("Set device %s to: %s\n",
        chosen->name, newState ? "ENABLE" : "DISABLE"
           );

    if (EnableDisableDevice(hDevInfo, &chosen->devInfo, newState))
    {
        printf("Successfully toggled.\n");
    }
    else
    {
        printf("Failed to toggle.\n");
    }

    // Cleanup
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return 0;
}