#ifndef NVIDIACONTROLLER_H
#define NVIDIACONTROLLER_H

#include <QColor>
#include <QString>
#include <vector>

// Forward declarations for NVAPI types
typedef unsigned int NV_STATUS;
typedef void* NV_PHYSICAL_GPU_HANDLE;
typedef unsigned int NV_U32;
typedef int NV_S32;

// NVAPI Zone Control Types
enum NV_GPU_CLIENT_ILLUM_ZONE_TYPE
{
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB = 0,
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW = 1,
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED = 2,
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR = 3
};

enum NV_GPU_CLIENT_ILLUM_CTRL_MODE
{
    NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL_RGB = 0,
    NV_GPU_CLIENT_ILLUM_CTRL_MODE_AUTO_RGB = 1
};

// NVAPI Structure definitions based on OpenRGB code
struct NV_GPU_CLIENT_ILLUM_ZONE_DATA_RGB
{
    struct
    {
        struct
        {
            unsigned char colorR;
            unsigned char colorG;
            unsigned char colorB;
            unsigned char brightnessPct;
        } rgbParams;
    } data;
};

struct NV_GPU_CLIENT_ILLUM_ZONE_DATA_RGBW
{
    struct
    {
        struct
        {
            unsigned char colorR;
            unsigned char colorG;
            unsigned char colorB;
            unsigned char colorW;
            unsigned char brightnessPct;
        } rgbwParams;
    } data;
};

struct NV_GPU_CLIENT_ILLUM_ZONE_DATA_SINGLE_COLOR
{
    struct
    {
        struct
        {
            unsigned char singleColorParams;
            unsigned char brightnessPct;
        } manualSingleColor;
    } data;
};

struct NV_GPU_CLIENT_ILLUM_ZONE_DATA_COLOR_FIXED
{
    struct
    {
        struct
        {
            unsigned char colorFixedParams;
            unsigned char brightnessPct;
        } manualColorFixed;
    } data;
};

struct NV_GPU_CLIENT_ILLUM_ZONE
{
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE type;
    NV_GPU_CLIENT_ILLUM_CTRL_MODE ctrlMode;
    union
    {
        NV_GPU_CLIENT_ILLUM_ZONE_DATA_RGB rgb;
        NV_GPU_CLIENT_ILLUM_ZONE_DATA_RGBW rgbw;
        NV_GPU_CLIENT_ILLUM_ZONE_DATA_SINGLE_COLOR singleColor;
        NV_GPU_CLIENT_ILLUM_ZONE_DATA_COLOR_FIXED colorFixed;
    } data;
};

struct NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS
{
    NV_U32 version;
    NV_U32 numIllumZonesControl;
    NV_U32 bDefault;
    NV_U32 rsvdField;
    NV_GPU_CLIENT_ILLUM_ZONE zones[8]; // Assuming max 8 zones
};

// NVAPI Constants
#define NVAPI_ZONE_GET_CONTROL 0
#define NVAPI_ZONE_SET_CONTROL 1
#define NVAPI_OK 0

// Mode definitions for RTX 3090 FE
#define NVIDIA_ILLUMINATION_OFF 0
#define NVIDIA_ILLUMINATION_DIRECT 1

class NVIDIAController
{
public:
    NVIDIAController();
    ~NVIDIAController();

    bool isInitialized() const { return initialized; }

    bool initialize();
    void setMode(int mode);
    void setBrightness(int brightness);
    void setRGBColor(const QColor& color);
    void setWhiteBrightness(int brightness);
    void updateLEDs();

    // Get information about zones
    int getNumZones() const { return numZones; }
    bool isRGBWZone(int zoneIndex) const;

private:
    bool initialized;
    bool deviceFound;
    NV_PHYSICAL_GPU_HANDLE gpuHandle;
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS zoneParams;
    int currentMode;
    int currentBrightness;
    int currentWhiteBrightness;
    QColor currentRGBColor;
    int numZones;

    // NVAPI function pointers (using query interface pattern)
    // nvapi_QueryInterface returns a function pointer, so the return type MUST be
    // a pointer type, not a 32-bit handle. Modeling it as NV_STATUS (unsigned int)
    // truncates the returned pointer on 64-bit and yields garbage function pointers.
    typedef void *(*NvAPI_QueryInterface_t)(unsigned int offset);
    NvAPI_QueryInterface_t NvAPI_QueryInterface;

    // Function pointer types for the actual functions we'll get through query interface
    typedef NV_STATUS (*NvAPI_Initialize_t)();
    typedef NV_STATUS (*NvAPI_EnumPhysicalGPUs_t)(NV_PHYSICAL_GPU_HANDLE*, NV_S32*);
    // NVAPI_GPU_GetPCIIdentifiers(out deviceId, out subSystemId, out revisionId, out extDeviceId)
    // NOTE: The 4th parameter is the *external* PCI device ID, NOT the vendor ID.
    typedef NV_STATUS (*NvAPI_GPU_GetPCIIdentifiers_t)(NV_PHYSICAL_GPU_HANDLE, NV_U32*, NV_U32*, NV_U32*, NV_U32*);
    typedef NV_STATUS (*NvAPI_GPU_ClientIllumZonesGetControl_t)(NV_PHYSICAL_GPU_HANDLE, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS*);
    typedef NV_STATUS (*NvAPI_GPU_ClientIllumZonesSetControl_t)(NV_PHYSICAL_GPU_HANDLE, NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS*);

    // Actual function pointers that we'll get through query interface
    NvAPI_Initialize_t NvAPI_Initialize;
    NvAPI_EnumPhysicalGPUs_t NvAPI_EnumPhysicalGPUs;
    NvAPI_GPU_GetPCIIdentifiers_t NvAPI_GPU_GetPCIIdentifiers;
    NvAPI_GPU_ClientIllumZonesGetControl_t NvAPI_GPU_ClientIllumZonesGetControl;
    NvAPI_GPU_ClientIllumZonesSetControl_t NvAPI_GPU_ClientIllumZonesSetControl;

    bool loadNVAPILibrary();
    bool detectRTX3090FE();
    void cleanup();
    void getZoneInfo();
    bool setupQueryInterfaceFunction();
};

#endif // NVIDIACONTROLLER_H