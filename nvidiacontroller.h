#ifndef NVIDIACONTROLLER_H
#define NVIDIACONTROLLER_H

#include <QColor>
#include <QString>
#include <vector>
#include <stdint.h>

// NVAPI base types (match the real nvapi64.dll ABI)
typedef int32_t  NV_STATUS;
typedef void*    NV_PHYSICAL_GPU_HANDLE;
typedef uint32_t NV_U32;
typedef int32_t  NV_S32;
typedef uint8_t  NV_U8;
typedef uint16_t NV_U16;

// ---------------------------------------------------------------------------
// NVAPI illumination types and structures.
//
// IMPORTANT: these definitions were copied verbatim from the real NVIDIA NVAPI
// headers and must stay byte-for-byte identical to the ABI exposed by
// nvapi64.dll. The driver reads/writes these structs through system memory, so
// any difference in size or field offset causes a heap buffer overflow:
// NvAPI_GPU_ClientIllumZonesGetControl / SetControl copy the whole
// NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_V1 (~6476 bytes). The driver
// validates the `version` field against the size it was built with, a bad
// version makes GetControl fail (Apply button silently no-ops) and a too-small
// buffer makes SetControl overflow the heap (segfault on exit).
// Sizes verified at compile time: zone=200, params=6476.
// ---------------------------------------------------------------------------

#define NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX 32
#define MAKE_NVAPI_VERSION(STRUCT, VERSION) (((VERSION) << 16) | sizeof(STRUCT))

typedef enum
{
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_INVALID = 0,
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGB,
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_COLOR_FIXED,
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_RGBW,
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE_SINGLE_COLOR,
} NV_GPU_CLIENT_ILLUM_ZONE_TYPE;


typedef enum
{
    NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_TOP_0   = 0x00,
    NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_FRONT_0 = 0x08,
    NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_GPU_BACK_0  = 0x0C,
    NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_SLI_TOP_0   = 0x20,
    NV_GPU_CLIENT_ILLUM_ZONE_LOCATION_INVALID     = 0xFFFFFFFF,
} NV_GPU_CLIENT_ILLUM_ZONE_LOCATION;


typedef enum
{
    NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_HALF_HALT = 0,
    NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_HALT,
    NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_FULL_REPEAT,
    NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_INVALID = 0xFF,
} NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE;

typedef enum
{
    NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL_RGB = 0,       // deprecated
    NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_RGB, // deprecated

    NV_GPU_CLIENT_ILLUM_CTRL_MODE_MANUAL = 0,
    NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR,

    // Strictly add new control modes above this.
    NV_GPU_CLIENT_ILLUM_CTRL_MODE_INVALID = 0xFF,
} NV_GPU_CLIENT_ILLUM_CTRL_MODE;


typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS
{
    NV_U8 colorR;
    NV_U8 colorG;
    NV_U8 colorB;
    NV_U8 brightnessPct;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB
{
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS rgbParams;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB;


typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR
{
    NV_GPU_CLIENT_ILLUM_PIECEWISE_LINEAR_CYCLE_TYPE    cycleType;
    NV_U8    grpCount;
    NV_U16   riseTimems;
    NV_U16   fallTimems;
    NV_U16   ATimems;
    NV_U16   BTimems;
    NV_U16   grpIdleTimems;
    NV_U16   phaseOffsetms;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR;


#define NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS           2

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGB
{
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB_PARAMS rgbParams[NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS];
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR  piecewiseLinearData;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGB;


typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGB
{
    union
    {
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGB            manualRGB;
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGB  piecewiseLinearRGB;
        NV_U8                                         rsvd[64];
    } data;
    NV_U8    rsvd[64];
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGB;


typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED_PARAMS
{
    NV_U8 brightnessPct;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED_PARAMS;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED
{
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED_PARAMS colorFixedParams;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_COLOR_FIXED
{
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED_PARAMS colorFixedParams[NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS];
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR          piecewiseLinearData;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_COLOR_FIXED;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_COLOR_FIXED
{
    union
    {
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_COLOR_FIXED           manualColorFixed;
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_COLOR_FIXED piecewiseLinearColorFixed;
        NV_U8                                         rsvd[64];
    } data;
    NV_U8    rsvd[64];
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_COLOR_FIXED;


typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS
{
    NV_U8 colorR;
    NV_U8 colorG;
    NV_U8 colorB;
    NV_U8 colorW;
    NV_U8 brightnessPct;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW
{
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS rgbwParams;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGBW
{
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW_PARAMS rgbwParams[NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS];
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR  piecewiseLinearRGBW;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGBW;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGBW
{
    union
    {
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_RGBW           manualRGBW;
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_RGBW piecewiseLinearRGBW;
        NV_U8                                         rsvd[64];
    } data;
    NV_U8    rsvd[64];
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGBW;


typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR_PARAMS
{
    NV_U8 brightnessPct;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR_PARAMS;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR
{
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR_PARAMS singleColorParams;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_SINGLE_COLOR
{
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR_PARAMS singleColorParams[NV_GPU_CLIENT_ILLUM_CTRL_MODE_PIECEWISE_LINEAR_COLOR_ENDPOINTS];
    NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR  piecewiseLinearData;
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_SINGLE_COLOR;

typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_SINGLE_COLOR
{
    union
    {
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_MANUAL_SINGLE_COLOR           manualSingleColor;
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_PIECEWISE_LINEAR_SINGLE_COLOR piecewiseLinearSingleColor;
        NV_U8                                         rsvd[64];
    } data;
    NV_U8    rsvd[64];
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_SINGLE_COLOR;

// NV_GPU_CLIENT_ILLUM_ZONE (a.k.a. the old name NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_V1)
typedef struct _NV_GPU_CLIENT_ILLUM_ZONE
{
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE  type;
    NV_GPU_CLIENT_ILLUM_CTRL_MODE  ctrlMode;
    union
    {
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGB           rgb;
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_COLOR_FIXED   colorFixed;
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_RGBW          rgbw;
        NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_DATA_SINGLE_COLOR  singleColor;
        NV_U8                                                rsvd[64];
    } data;
    NV_U8    rsvd[64];
} NV_GPU_CLIENT_ILLUM_ZONE;


typedef struct _NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS
{
    NV_U32                          version;

    // Bit field specifying default (NV_TRUE) vs currently active (NV_FALSE)
    NV_U32                          bDefault : 1;
    NV_U32                          rsvdField : 31;

    NV_U32                          numIllumZonesControl;

    NV_U8                           rsvd[64];

    NV_GPU_CLIENT_ILLUM_ZONE        zones[NV_GPU_CLIENT_ILLUM_ZONE_NUM_ZONES_MAX];
} NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS;

#define NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER   MAKE_NVAPI_VERSION(NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS, 1)
#define NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_SIZE  6476u  // == sizeof (compile-time asserted below)

// NVAPI Status codes
#define NVAPI_OK                      0
#define NVAPI_ERROR                   -1
#define NVAPI_API_NOT_SUPPORTED       -3
#define NVAPI_INVALID_ARGUMENT        -5

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
    NV_GPU_CLIENT_ILLUM_ZONE_TYPE getZoneType(int zoneIndex) const;

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
    typedef void *(*NvAPI_QueryInterface_t)(NV_U32 interfaceID);
    NvAPI_QueryInterface_t NvAPI_QueryInterface;

    // Function pointer types for the actual functions we'll get through query interface
    typedef NV_STATUS (*NvAPI_Initialize_t)();
    typedef NV_STATUS (*NvAPI_EnumPhysicalGPUs_t)(NV_PHYSICAL_GPU_HANDLE*, NV_S32*);
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

// Compile-time guards: if these ever fail, the struct defs above have drifted
// from the real nvapi64.dll ABI and the controller will corrupt memory.
static_assert(sizeof(NV_GPU_CLIENT_ILLUM_ZONE) == 200,
              "NV_GPU_CLIENT_ILLUM_ZONE size mismatch with nvapi64.dll ABI");
static_assert(sizeof(NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS) == 6476,
              "NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS size mismatch with nvapi64.dll ABI");
static_assert(NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS_VER == 72012u,
              "NV_GPU_CLIENT_ILLUM_ZONE_CONTROL_PARAMS version mismatch");

#endif // NVIDIACONTROLLER_H
