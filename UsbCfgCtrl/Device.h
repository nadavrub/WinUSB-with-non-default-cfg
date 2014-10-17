#pragma once

class UsbCfgDevice 
{
protected:
	enum { USB_DEFAULT_CFG_INDEX = 0 };
	WDFDEVICE		m_hDevice;
	WDFIOTARGET		m_hIOTarget;
	WDFUSBDEVICE	m_hUsbDevice;
	BYTE			m_btZeroCfgSwitch;

//	NTSTATUS SendStrangePacket();

	void ForwardRequest(IN WDFREQUEST Request);
	void ForwardRequestWithCompletion(IN WDFREQUEST Request, IN PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine, IN WDFCONTEXT CompletionContext);

	BOOL OnFilterInternalDeviceControl(IN WDFQUEUE Queue, IN WDFREQUEST Request, IN size_t OutputBufferLength, IN size_t InputBufferLength, IN ULONG IoControlCode);

	NTSTATUS Initialize();

	UsbCfgDevice(IN WDFDEVICE hDevice);
	~UsbCfgDevice();

public:
	void __cdecl operator delete(void*);

	inline WDFUSBDEVICE GetDevice() { return m_hUsbDevice; }
	inline BOOL IsReady() { return 0 != m_hUsbDevice; }

	static NTSTATUS CreateDevice(IN WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit);
};

// KDMF Filter driver sample: http://www.osronline.com/article.cfm?article=446
extern "C" {
	// This macro will generate an inline function called DeviceGetContext which will be used to get a pointer to the device context memory in a type safe manner.
	WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UsbCfgDevice, DeviceGetSingleton)
}