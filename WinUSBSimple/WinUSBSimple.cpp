#include "stdafx.h"
#include "device.h"

#define USB_DEFAULT_CFG 0
#define MAKE_SETUP_REQUEST_TYPE(dir, type, recipient) ((UCHAR)(((dir & 1) << 7) | ((type & 7) << 4) | (recipient & 0xF)))

BOOL QuerySelectedConfiguration(IN WINUSB_INTERFACE_HANDLE hDevice, OUT BYTE& btSelectedConfiguration) {
	WINUSB_SETUP_PACKET packet = { 0 };
	//Create the setup packet
	packet.RequestType = MAKE_SETUP_REQUEST_TYPE(BMREQUEST_DEVICE_TO_HOST, BMREQUEST_STANDARD, BMREQUEST_TO_DEVICE);
	packet.Request = USB_REQUEST_GET_CONFIGURATION;// Table 9-4. Standard Device Requests;
	packet.Length = sizeof(btSelectedConfiguration);
	ULONG ulBytesProcessed = 0;
	return WinUsb_ControlTransfer(hDevice, packet, &btSelectedConfiguration, sizeof(btSelectedConfiguration), &ulBytesProcessed, 0);
}

BOOL QueryAvailableConfiguration(IN WINUSB_INTERFACE_HANDLE hDevice, IN BYTE index, IN OUT USB_CONFIGURATION_DESCRIPTOR& desc, IN OUT ULONG& ulBytes) {
	WINUSB_SETUP_PACKET packet = { 0 };
	//Create the setup packet
	packet.RequestType = MAKE_SETUP_REQUEST_TYPE(BMREQUEST_DEVICE_TO_HOST, BMREQUEST_STANDARD, BMREQUEST_TO_DEVICE);
	packet.Request = USB_REQUEST_GET_DESCRIPTOR;// Table 9-4. Standard Device Requests;
	packet.Value = ((WORD)USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | index;
	packet.Length = sizeof(desc);
	ULONG ulBytesProcessed = 0;
	if (FALSE == WinUsb_ControlTransfer(hDevice, packet, (PBYTE)&desc, ulBytes, &ulBytesProcessed, 0))
		return FALSE;
	ulBytes = ulBytesProcessed;
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	_ASSERT(FALSE);
	HRESULT		hr;
	DEVICE_DATA deviceData;
	BOOL        noDevice;

	if (FAILED(hr = OpenDevice(&deviceData, &noDevice))) {
		_ASSERT(FALSE);
		if (TRUE == noDevice) {
			printf("Device not connected or driver not installed\n");
		}
		else {
			printf("Failed looking for device, HRESULT 0x%x\n", hr);
		}
		return hr;
	}

	BYTE btSelectedCfg = 0xFF;
	if (FALSE == QuerySelectedConfiguration(deviceData.WinusbHandle, btSelectedCfg)) {// Returns the id of selected CFG, should be identical to the is of the CFG at index '0' queried next
		printf("Failed quering selected configuration, HRESULT 0x%x\n", hr);
		CloseDevice(&deviceData);
		return E_FAIL;
	}

	USB_CONFIGURATION_DESCRIPTOR	desc;
	ULONG							ulBytes = sizeof(desc);
	if (FALSE == QueryAvailableConfiguration(deviceData.WinusbHandle, USB_DEFAULT_CFG, desc, ulBytes)) {// Returns the overriden configuration set by our driver
		printf("Failed quering available configuration, HRESULT 0x%x\n", hr);
		CloseDevice(&deviceData);
		return E_FAIL;
	}

	_ASSERT(btSelectedCfg == desc.bConfigurationValue);// Must be true
	
	return S_OK;
}

