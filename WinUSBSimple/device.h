//
// Define below GUIDs
//
#include <initguid.h>

// Device Interface GUID.
// Used by all WinUsb devices that this application talks to.
// Must match "DeviceInterfaceGUIDs" registry value specified in the INF file.
// adf8702c-8857-4204-992f-61a93eb9f0c1
DEFINE_GUID(GUID_DEVINTERFACE_iOS_USB_Capture, 0xadf8702c,0x8857,0x4204,0x99,0x2f,0x61,0xa9,0x3e,0xb9,0xf0,0xc1);

typedef struct _DEVICE_DATA {

    BOOL                    HandlesOpen;
    WINUSB_INTERFACE_HANDLE WinusbHandle;
    HANDLE                  DeviceHandle;
    TCHAR                   DevicePath[MAX_PATH];

} DEVICE_DATA, *PDEVICE_DATA;

HRESULT OpenDevice(OUT PDEVICE_DATA DeviceData, OUT PBOOL FailureDeviceNotFound);
VOID CloseDevice(IN OUT PDEVICE_DATA DeviceData);
