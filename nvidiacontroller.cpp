#include "nvidiacontroller.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>
#include <cstring>

#include <windows.h>
#define LIBRARY_HANDLE HMODULE
#define LOAD_LIBRARY LoadLibraryW
#define GET_PROC_ADDRESS GetProcAddress
#define CLOSE_LIBRARY FreeLibrary

// Helper function to dump exported functions from a DLL
void dumpDLLExports(LIBRARY_HANDLE handle, const QString& dllPath)
{
    qDebug() << "=== DUMPING EXPORTS FROM DLL ===";
    qDebug() << "DLL Path:" << dllPath;

    // Get the DOS header
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)handle;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        qDebug() << "Invalid DOS signature in DLL";
        return;
    }

    // Get the NT headers
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)handle + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        qDebug() << "Invalid NT signature in DLL";
        return;
    }

    // Get the export directory
    PIMAGE_EXPORT_DIRECTORY exportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)handle +
                                                                   ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    DWORD* addressOfNames = (DWORD*)((BYTE*)handle + exportDir->AddressOfNames);
    // DWORD* addressOfFunctions = (DWORD*)((BYTE*)handle + exportDir->AddressOfFunctions);
    // WORD* addressOfNameOrdinals = (WORD*)((BYTE*)handle + exportDir->AddressOfNameOrdinals);

    qDebug() << "Number of exported functions:" << exportDir->NumberOfNames;

    // Only dump first 20 functions to avoid overwhelming output
    const DWORD maxFunctions = qMin(exportDir->NumberOfNames, static_cast<DWORD>(20));
    for (DWORD i = 0; i < maxFunctions; i++) {
        char* functionName = (char*)((BYTE*)handle + addressOfNames[i]);
        qDebug() << "Exported function" << i << ":" << functionName;
    }

    if (exportDir->NumberOfNames > 20) {
        qDebug() << "... and" << (exportDir->NumberOfNames - 20) << "more functions";
    }

    qDebug() << "=== END DUMP ===";
}

NVIDIAController::NVIDIAController()
    : initialized(false)
    , deviceFound(false)
    , gpuHandle(nullptr)
    , currentMode(1) // Direct mode
    , currentBrightness(100)
    , currentWhiteBrightness(100)
    , numZones(0)
{
    // Initialize zoneParams structure
    memset(&zoneParams, 0, sizeof(zoneParams));

    qDebug() << "Initializing NVIDIA controller...";

    if (!initialize())
    {
        qDebug() << "Failed to initialize NVIDIA controller";
    } else {
        qDebug() << "NVIDIA controller initialized successfully";
    }
}

NVIDIAController::~NVIDIAController()
{
    cleanup();
}

bool NVIDIAController::initialize()
{
    qDebug() << "Starting NVIDIA controller initialization...";

    if (initialized)
        return true;

    qDebug() << "Attempting to load NVAPI library...";

    // Load NVAPI library
    if (!loadNVAPILibrary())
    {
        qDebug() << "Failed to load NVAPI library - cannot proceed with initialization";
        return false;
    }

    qDebug() << "NVAPI library loaded successfully. Setting up query interface functions...";

    // Setup the query interface functions
    if (!setupQueryInterfaceFunction())
    {
        qDebug() << "Failed to set up query interface functions";
        return false;
    }

    qDebug() << "Query interface functions set up successfully. Initializing NVAPI...";

    // NVAPI must be fully initialized before any other call (e.g. enumerating
    // GPUs) or those calls fail. Initialize first, then detect the GPU.
    NV_STATUS initStatus = NvAPI_Initialize();
    if (initStatus != NVAPI_OK)
    {
        qDebug() << "Failed to initialize NVAPI - status code:" << initStatus;
        return false;
    }

    qDebug() << "NVAPI initialized successfully. Attempting to detect RTX 3090 FE...";

    // Try to detect RTX 3090 FE
    if (!detectRTX3090FE())
    {
        qDebug() << "RTX 3090 FE not found - this may be expected if the correct GPU isn't detected";
        return false;
    }

    qDebug() << "RTX 3090 FE detected.";

    qDebug() << "Getting zone information...";

    // Get zone information
    getZoneInfo();

    initialized = true;
    qDebug() << "NVIDIA controller initialization completed successfully";
    return true;
}

bool NVIDIAController::loadNVAPILibrary()
{
    // Try to load nvapi64.dll first (64-bit version)
    const QString libName = "nvapi64.dll";
    qDebug() << "Attempting to load library:" << libName;
    LIBRARY_HANDLE handle = LOAD_LIBRARY(libName.toStdWString().c_str());

    if (!handle) {
        qDebug() << "Failed to load" << libName << "with error code:" << GetLastError();
    } else {
        qDebug() << "Successfully loaded" << libName << "at address:" << handle;
    }

    // If that fails, try nvapi.dll (32-bit version)
    if (!handle)
    {
        const QString altLibName = "nvapi.dll";
        qDebug() << "Attempting to load alternative library:" << altLibName;
        handle = LOAD_LIBRARY(altLibName.toStdWString().c_str());
        if (!handle) {
            qDebug() << "Failed to load" << altLibName << "with error code:" << GetLastError();
        } else {
            qDebug() << "Successfully loaded" << altLibName << "at address:" << handle;
        }
    }

    // If still no success, let's also try common locations
    if (!handle)
    {
        // Try common NVIDIA installation paths
        QStringList commonPaths;
        commonPaths << "C:/Windows/System32/nvapi64.dll"
                    << "C:/Windows/SysWOW64/nvapi.dll"
                    << "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.0/bin/nvapi64.dll";

        qDebug() << "Trying common paths for NVAPI library...";

        for (const QString& path : commonPaths)
        {
            qDebug() << "Attempting to load from:" << path;
            handle = LOAD_LIBRARY(path.toStdWString().c_str());
            if (handle)
            {
                qDebug() << "Found NVAPI library at:" << path;
                qDebug() << "DLL loaded successfully at address:" << handle;
                break;
            } else {
                qDebug() << "Failed to load from" << path << "with error code:" << GetLastError();
            }
        }
    }

    // Additional check: Try to get the full path of loaded library for debugging
    if (handle) {
        wchar_t buffer[1024];
        DWORD result = GetModuleFileNameW((HMODULE)handle, buffer, 1024);
        if (result > 0) {
            qDebug() << "Full path of loaded library:" << QString::fromWCharArray(buffer);

            // Add extra debugging - check what this file actually is
            qDebug() << "Checking if this is a real NVAPI DLL by examining file size...";
            WIN32_FILE_ATTRIBUTE_DATA fileInfo;
            if (GetFileAttributesExW(buffer, GetFileExInfoStandard, &fileInfo)) {
                qDebug() << "DLL file size:" << fileInfo.nFileSizeLow << "bytes";
            }
        } else {
            qDebug() << "Could not get full path of loaded library";
        }
    }

    if (!handle)
    {
        qDebug() << "Failed to load NVAPI library - no valid handle returned";
        qDebug() << "This is likely due to missing or incorrect NVIDIA drivers.";
        return false;
    }

    // Debug: Check what functions are actually available in this library
    qDebug() << "Library loaded successfully. Checking available functions...";
    qDebug() << "Library handle:" << handle;

    dumpDLLExports(handle, "");

    // Let's also check for common alternative names that might be used
    const char* functionNames[] = {
        "nvapi_Direct_GetMethod",
        "nvapi_QueryInterface"
    };

    int numFunctionNames = sizeof(functionNames) / sizeof(functionNames[0]);
    bool foundAtLeastOneFunction = false;

    qDebug() << "Checking for all possible function names...";
    for (int i = 0; i < numFunctionNames; i++) {
        const char* funcName = functionNames[i];
        FARPROC proc = GET_PROC_ADDRESS(handle, funcName);
        if (proc) {
            qDebug() << "Found function:" << funcName;
            foundAtLeastOneFunction = true;
        } else {
            // Only log missing functions for the primary names, not the decorated ones
            if (i < 5) {
                qDebug() << "Missing function:" << funcName;
            }
        }
    }

    // If we loaded a library but it has no expected NVAPI functions, this is definitely the problem
    if (!foundAtLeastOneFunction) {
        qDebug() << "CRITICAL: Loaded library contains NO expected NVAPI functions!";
        return false;
    }

    // Get the nvapi_QueryInterface function pointer which is the key to accessing all other functions
    qDebug() << "Attempting to get nvapi_QueryInterface function pointer...";
    NvAPI_QueryInterface = reinterpret_cast<NvAPI_QueryInterface_t>(GET_PROC_ADDRESS(handle, "nvapi_QueryInterface"));
    if (!NvAPI_QueryInterface) {
        qDebug() << "Failed to get nvapi_QueryInterface function pointer";
        CLOSE_LIBRARY(handle);
        return false;
    } else {
        qDebug() << "Successfully got nvapi_QueryInterface function pointer";
    }

    // Note: We don't try to get the other functions directly since they're accessed through query interface
    qDebug() << "Successfully loaded NVAPI library using COM-like query interface pattern";
    return true;
}

bool NVIDIAController::detectRTX3090FE()
{
    // Properly detect RTX 3090 FE by enumerating GPUs and checking their identifiers
    if (!NvAPI_EnumPhysicalGPUs || !NvAPI_GPU_GetPCIIdentifiers)
        return false;

    // Enumerate all physical GPUs
    NV_PHYSICAL_GPU_HANDLE gpuHandles[16] = {0};  // Maximum number of GPUs we might encounter
    NV_S32 gpuCount = 0;

    // NVAPI returns NVAPI_OK (0) on success and a nonzero value on error.
    NV_STATUS enumStatus = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);
    if (enumStatus != NVAPI_OK)
    {
        qDebug() << "Failed to enumerate GPUs - status code:" << enumStatus;
        return false;
    }

    if (gpuCount <= 0)
    {
        qDebug() << "No GPUs found";
        return false;
    }

    // Look for RTX 3090 FE specifically by PCI identifiers.
    // NVAPI returns deviceId/subSystemId as packed 32-bit values where the
    // high 16 bits are the PCI ID and the low 16 bits are the vendor ID
    // (e.g. 0x220410de -> RTX 3090 device ID 0x2204, 0x147d10de -> FE
    // subsystem ID 0x147d).
    const NV_U32 NVIDIA_VENDOR_ID = 0x10DE;    // NVIDIA vendor ID
    const NV_U32 RTX3090_DEV_ID = 0x2204;      // RTX 3090 PCI device ID
    const NV_U32 RTX3090_FE_SUB_DEV_ID = 0x147d; // RTX 3090 FE PCI subsystem ID

    for (int i = 0; i < gpuCount; i++)
    {
        NV_U32 deviceId = 0;
        NV_U32 subSystemId = 0;
        NV_U32 revisionId = 0;
        NV_U32 extDeviceId = 0;

        // 4th out-parameter is the *external* PCI device ID, not a vendor ID
        if (NvAPI_GPU_GetPCIIdentifiers(gpuHandles[i], &deviceId, &subSystemId, &revisionId, &extDeviceId) != NVAPI_OK)
        {
            continue;
        }

        // NVAPI returns these as 32-bit values with the 16-bit PCI ID in the
        // high word and the vendor ID (0x10DE) in the low word, e.g.:
        //   deviceId    = 0x220410de  -> PCI device ID 0x2204
        //   subSystemId = 0x147d10de  -> PCI subsystem ID 0x147d
        const NV_U32 vendorId = deviceId & 0xFFFF;
        if (vendorId != NVIDIA_VENDOR_ID)
            continue;

        const NV_U32 devId16 = (deviceId >> 16) & 0xFFFF;
        const NV_U32 subId16 = (subSystemId >> 16) & 0xFFFF;

        qDebug() << "Found NVIDIA GPU: dev=0x" << QString::number(devId16, 16)
                 << "sub=0x" << QString::number(subId16, 16);

        // Check for RTX 3090 FE specifically (device ID + FE subsystem ID)
        if (devId16 == RTX3090_DEV_ID && subId16 == RTX3090_FE_SUB_DEV_ID)
        {
            gpuHandle = gpuHandles[i];
            deviceFound = true;
            qDebug() << "RTX 3090 FE detected successfully";
            return true;
        }
    }

    qDebug() << "RTX 3090 FE not found";
    return false;
}

void NVIDIAController::getZoneInfo()
{
    if (!deviceFound)
        return;

    if (!NvAPI_GPU_ClientIllumZonesGetControl)
        return;

    memset(&zoneParams, 0, sizeof(zoneParams));
    zoneParams.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
    zoneParams.bDefault = 0;

    NV_STATUS status = NvAPI_GPU_ClientIllumZonesGetControl(gpuHandle, &zoneParams);
    if (status != NVAPI_OK)
    {
        qDebug() << "NvAPI_GPU_ClientIllumZonesGetControl failed:" << status;
        return;
    }

    numZones = zoneParams.numIllumZonesControl;
    qDebug() << "Found" << numZones << "illumination zones";
    for (int i = 0; i < numZones && i < NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX; ++i)
    {
        qDebug() << "  zone" << i
                 << "type=" << static_cast<int>(zoneParams.zones[i].type)
                 << "ctrlMode=" << static_cast<int>(zoneParams.zones[i].ctrlMode);
    }
}

void NVIDIAController::setMode(int mode)
{
    currentMode = mode;
}

void NVIDIAController::setBrightness(int brightness)
{
    currentBrightness = brightness;
}

void NVIDIAController::setRGBColor(const QColor& color)
{
    currentRGBColor = color;
}

void NVIDIAController::setWhiteBrightness(int brightness)
{
    currentWhiteBrightness = brightness;
}

NV_GPU_CLIENT_ILLUM_ZONE_TYPE NVIDIAController::getZoneType(int zoneIndex) const
{
    if (!deviceFound || zoneIndex < 0 || zoneIndex >= NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX)
        return NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID;

    return zoneParams.zones[zoneIndex].type;
}

void NVIDIAController::updateLEDs()
{
    if (!initialized || !deviceFound)
        return;

    if (!NvAPI_GPU_ClientIllumZonesGetControl || !NvAPI_GPU_ClientIllumZonesSetControl)
    {
        qDebug() << "updateLEDs: illumination zone control functions not available";
        return;
    }

    // Query the live control state so we preserve any fields the driver requires
    // and learn the type / ctrlMode of each zone.
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS params;
    memset(&params, 0, sizeof(params));
    params.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
    params.bDefault = 0;

    NV_STATUS getStatus = NvAPI_GPU_ClientIllumZonesGetControl(gpuHandle, &params);
    unsigned numZones = params.numIllumZonesControl;
    if (getStatus != NVAPI_OK)
    {
        // Fall back to a single manual RGB/RGBW zone if we can't read current state.
        qDebug() << "updateLEDs: GetControl failed:" << getStatus;
        memset(&params, 0, sizeof(params));
        params.version = NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER;
        params.bDefault = 0;
        params.numIllumZonesControl = 1;
        params.zones[0].type     = NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW;
        params.zones[0].ctrlMode = NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL_RGB;
        numZones = 1;
    }

    for (unsigned i = 0; i < numZones && i < NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX; ++i)
    {
        // Pick the largest colour-capable variant the zone reports so we can
        // write R/G/B (and W when available).
        NV_GPU_CLIENT_ILLUM_ZONE* z = &params.zones[i];
        z->ctrlMode = NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL_RGB;

        // The "White Brightness" value is the master percentage (brightnessPct)
        // for every zone. The picked colour drives R/G/B, and for RGBW zones the
        // white *content* of that colour (its minimum channel) is routed to the
        // dedicated white diode (colorW) so white renders at full quality.
        // `on` is 0 when the mode is Off, so the whole zone is dark.
        const unsigned int r  = static_cast<unsigned int>(currentRGBColor.red());
        const unsigned int g  = static_cast<unsigned int>(currentRGBColor.green());
        const unsigned int b  = static_cast<unsigned int>(currentRGBColor.blue());
        const unsigned int on = (currentMode == NVIDIA_ILLUMINATION_OFF)
                                    ? 0u : static_cast<unsigned int>(currentBrightness);

        if (z->type == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW)
        {
            // data.rgbw.data.manualRGBW.rgbwParams.{colorR..}
            NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS* p =
                &z->data.rgbw.data.manualRGBW.rgbwParams;
            if (currentMode == NVIDIA_ILLUMINATION_OFF)
            {
                p->colorR = 0; p->colorG = 0; p->colorB = 0; p->colorW = 0;
                p->brightnessPct = 0;
            }
            else
            {
                unsigned int w = r;   // white content = min(R,G,B) -> dedicated W diode
                if (g < w) w = g;
                if (b < w) w = b;
                p->colorR = static_cast<unsigned char>(r - w);
                p->colorG = static_cast<unsigned char>(g - w);
                p->colorB = static_cast<unsigned char>(b - w);
                p->colorW = static_cast<unsigned char>(w);
                p->brightnessPct = static_cast<unsigned char>(on);
            }
        }
        else if (z->type == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB)
        {
            // data.rgb.data.manualRGB.rgbParams.{colorR..}
            NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS* p =
                &z->data.rgb.data.manualRGB.rgbParams;
            if (currentMode == NVIDIA_ILLUMINATION_OFF)
            {
                p->colorR = 0; p->colorG = 0; p->colorB = 0;
                p->brightnessPct = 0;
            }
            else
            {
                p->colorR = static_cast<unsigned char>(r);
                p->colorG = static_cast<unsigned char>(g);
                p->colorB = static_cast<unsigned char>(b);
                p->brightnessPct = static_cast<unsigned char>(on);
            }
        }
        else
        {
            // SINGLE_COLOR / COLOR_FIXED: only brightness is meaningful.
            if (z->type == NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR)
            {
                z->data.singleColor.data.manualSingleColor.singleColorParams.brightnessPct =
                    (currentMode == NVIDIA_ILLUMINATION_OFF) ? 0u : static_cast<unsigned char>(currentBrightness);
            }
            else
            {
                z->data.colorFixed.data.manualColorFixed.colorFixedParams.brightnessPct =
                    (currentMode == NVIDIA_ILLUMINATION_OFF) ? 0u : static_cast<unsigned char>(currentBrightness);
            }
        }
    }

    params.numIllumZonesControl = numZones;

    NV_STATUS setStatus = NvAPI_GPU_ClientIllumZonesSetControl(gpuHandle, &params);
    if (setStatus != NVAPI_OK)
    {
        qWarning() << "updateLEDs: NvAPI_GPU_ClientIllumZonesSetControl failed:" << setStatus;
    }
    else
    {
        qDebug() << "updateLEDs: applied" << numZones << "zone(s) in mode" << currentMode;
    }
}

bool NVIDIAController::setupQueryInterfaceFunction()
{
    // Use the nvapi_QueryInterface function to get pointers to actual NVAPI functions
    // This is the key difference from the original code - we must use the query interface pattern

    if (!NvAPI_QueryInterface) {
        qDebug() << "nvapi_QueryInterface not available for setting up other functions";
        return false;
    }

    // Get function pointer for NvAPI_Initialize
    void* funcPtr = (void*)NvAPI_QueryInterface(0x0150e828);  // NvAPI_Initialize_ID
    NvAPI_Initialize = reinterpret_cast<NvAPI_Initialize_t>(funcPtr);
    if (!NvAPI_Initialize) {
        qDebug() << "Failed to get NvAPI_Initialize through query interface";
        return false;
    } else {
        qDebug() << "Successfully got NvAPI_Initialize through query interface";
    }

    // Get function pointer for NvAPI_EnumPhysicalGPUs
    funcPtr = (void*)NvAPI_QueryInterface(0xe5ac921f);  // NvAPI_EnumPhysicalGPUs_ID
    NvAPI_EnumPhysicalGPUs = reinterpret_cast<NvAPI_EnumPhysicalGPUs_t>(funcPtr);
    if (!NvAPI_EnumPhysicalGPUs) {
        qDebug() << "Failed to get NvAPI_EnumPhysicalGPUs through query interface";
        return false;
    } else {
        qDebug() << "Successfully got NvAPI_EnumPhysicalGPUs through query interface";
    }

    // Get function pointer for NvAPI_GPU_GetPCIIdentifiers
    funcPtr = (void*)NvAPI_QueryInterface(0x2ddfb66e);  // NvAPI_GPU_GetPCIIdentifiers_ID
    NvAPI_GPU_GetPCIIdentifiers = reinterpret_cast<NvAPI_GPU_GetPCIIdentifiers_t>(funcPtr);
    if (!NvAPI_GPU_GetPCIIdentifiers) {
        qDebug() << "Failed to get NvAPI_GPU_GetPCIIdentifiers through query interface";
        return false;
    } else {
        qDebug() << "Successfully got NvAPI_GPU_GetPCIIdentifiers through query interface";
    }

    // Get function pointer for NvAPI_GPU_ClientIllumZonesGetControl
    funcPtr = (void*)NvAPI_QueryInterface(0x3dbf5764);  // NvAPI_GPU_ClientIllumZonesGetControl_ID
    NvAPI_GPU_ClientIllumZonesGetControl = reinterpret_cast<NvAPI_GPU_ClientIllumZonesGetControl_t>(funcPtr);
    if (!NvAPI_GPU_ClientIllumZonesGetControl) {
        qDebug() << "Failed to get NvAPI_GPU_ClientIllumZonesGetControl through query interface";
        return false;
    } else {
        qDebug() << "Successfully got NvAPI_GPU_ClientIllumZonesGetControl through query interface";
    }

    // Get function pointer for NvAPI_GPU_ClientIllumZonesSetControl
    funcPtr = (void*)NvAPI_QueryInterface(0x197d065e);  // NvAPI_GPU_ClientIllumZonesSetControl_ID
    NvAPI_GPU_ClientIllumZonesSetControl = reinterpret_cast<NvAPI_GPU_ClientIllumZonesSetControl_t>(funcPtr);
    if (!NvAPI_GPU_ClientIllumZonesSetControl) {
        qDebug() << "Failed to get NvAPI_GPU_ClientIllumZonesSetControl through query interface";
        return false;
    } else {
        qDebug() << "Successfully got NvAPI_GPU_ClientIllumZonesSetControl through query interface";
    }

    return true;
}

void NVIDIAController::cleanup()
{
    // Cleanup resources if needed
    initialized = false;
    deviceFound = false;
    numZones = 0;
}