#include "stdafx.h"
#include <SetupAPI.h>
#include "device.h"

HRESULT RetrieveDevicePath(OUT LPTSTR DevicePath, IN ULONG BufLen, OUT PBOOL FailureDeviceNotFound) {
	HRESULT                          hr;
	ULONG                            length;
	HDEVINFO                         deviceInfo;
	SP_DEVICE_INTERFACE_DATA         interfaceData;
	ULONG                            requiredLength = 0;
	BOOL                             bResult		= FALSE;
	PSP_DEVICE_INTERFACE_DETAIL_DATA detailData		= NULL;

	if (NULL != FailureDeviceNotFound)
		*FailureDeviceNotFound = FALSE;

	// Enumerate all devices exposing the interface
	if (INVALID_HANDLE_VALUE == (deviceInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_iOS_USB_Capture, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE))) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	// Get the first interface (index 0) in the result set
	if (FALSE == (bResult = SetupDiEnumDeviceInterfaces(deviceInfo, NULL, &GUID_DEVINTERFACE_iOS_USB_Capture, 0, &interfaceData))) {
		// We would see this error if no devices were found
		if ((ERROR_NO_MORE_ITEMS == GetLastError()) && (NULL != FailureDeviceNotFound))
			*FailureDeviceNotFound = TRUE;

		hr = HRESULT_FROM_WIN32(GetLastError());
		SetupDiDestroyDeviceInfoList(deviceInfo);
		return hr;
	}

	// Get the size of the path string We expect to get a failure with insufficient buffer
	bResult = SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, NULL, 0, &requiredLength, NULL);
	if (FALSE == bResult && ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		SetupDiDestroyDeviceInfoList(deviceInfo);
		return hr;
	}

	// Allocate temporary space for SetupDi structure
	if (NULL == (detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED, requiredLength))) {
		hr = E_OUTOFMEMORY;
		SetupDiDestroyDeviceInfoList(deviceInfo);
		return hr;
	}

	detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	length = requiredLength;

	// Get the interface's path string
	if (FALSE == (bResult = SetupDiGetDeviceInterfaceDetail(deviceInfo, &interfaceData, detailData, length, &requiredLength, NULL))) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		LocalFree(detailData);
		SetupDiDestroyDeviceInfoList(deviceInfo);
		return hr;
	}

	// Give path to the caller. SetupDiGetDeviceInterfaceDetail ensured DevicePath is NULL-terminated.
	hr = StringCbCopy(DevicePath, BufLen, detailData->DevicePath);
	LocalFree(detailData);
	SetupDiDestroyDeviceInfoList(deviceInfo);
	return hr;
}

HRESULT OpenDevice(OUT PDEVICE_DATA DeviceData, OUT PBOOL FailureDeviceNotFound) {
    HRESULT hr = S_OK;
    BOOL    bResult;

    DeviceData->HandlesOpen = FALSE;

	if (FAILED(hr = RetrieveDevicePath(DeviceData->DevicePath, sizeof(DeviceData->DevicePath), FailureDeviceNotFound)))
        return hr;

    DeviceData->DeviceHandle = CreateFile(DeviceData->DevicePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
    if (INVALID_HANDLE_VALUE == DeviceData->DeviceHandle)
		return HRESULT_FROM_WIN32(GetLastError());

	if (FALSE == (bResult = WinUsb_Initialize(DeviceData->DeviceHandle, &DeviceData->WinusbHandle))) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CloseHandle(DeviceData->DeviceHandle);
        return hr;
    }

    DeviceData->HandlesOpen = TRUE;
    return hr;
}

VOID CloseDevice(IN OUT PDEVICE_DATA DeviceData) {
    if (FALSE == DeviceData->HandlesOpen)
        return;

    WinUsb_Free(DeviceData->WinusbHandle);
    CloseHandle(DeviceData->DeviceHandle);
    DeviceData->HandlesOpen = FALSE;
}