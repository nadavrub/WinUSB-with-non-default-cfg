#include "driver.h"
#include "device.tmh"

#include "Usbioctl.h"

NTSTATUS UsbCfgDevice::CreateDevice(IN WDFDRIVER /*Driver*/, _Inout_ PWDFDEVICE_INIT DeviceInit) {
	NTSTATUS                 status;
	WDF_OBJECT_ATTRIBUTES    wdfObjectAttr;
	WDFDEVICE                wdfDevice;
	UsbCfgDevice*			 pContext;

	WdfFdoInitSetFilter(DeviceInit);

	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&wdfObjectAttr, UsbCfgDevice);
	wdfObjectAttr.EvtCleanupCallback = [](IN WDFOBJECT Object) {
		DeviceGetSingleton(Object)->~UsbCfgDevice(); // Destruct
	};

	if (!NT_SUCCESS(status = WdfDeviceCreate(&DeviceInit, &wdfObjectAttr, &wdfDevice))) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDeviceCreate failed - 0x%x\n", status);
		return status;
	}

	pContext = DeviceGetSingleton(wdfDevice);
	pContext->UsbCfgDevice::UsbCfgDevice(wdfDevice);

	if (NT_FAILED(status = pContext->Initialize())) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDeviceCreate, UsbCfgDevice::Initialize() failed - 0x%x\n", status);
		return status;
	}

	return STATUS_SUCCESS;
}

UsbCfgDevice::UsbCfgDevice(IN WDFDEVICE hDevice)
		 : m_hUsbDevice(0)
		 , m_hDevice(hDevice)
		 , m_hIOTarget(0)
		 , m_btZeroCfgSwitch(1)
{
	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::UsbCfgDevice\n");
}

UsbCfgDevice::~UsbCfgDevice() {
	WdfObjectDelete(m_hUsbDevice);
	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::~UsbCfgDevice\n");
}

void __cdecl UsbCfgDevice::operator delete(void*) {
	// The Kdmf framework is responsible of freeing memory allocated for our Object, 'delete' should never get called
	if (FALSE == KdRefreshDebuggerNotPresent())
		DbgBreakPoint();
}

void UsbCfgDevice::ForwardRequest(IN WDFREQUEST Request) {
	WDF_REQUEST_SEND_OPTIONS options;

	WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

	if (TRUE == WdfRequestSend(Request, m_hIOTarget, &options))
		return;

	NTSTATUS status = WdfRequestGetStatus(Request);
	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestSend failed - 0x%x\n", status);
	WdfRequestComplete(Request, status);
	return;
}

void UsbCfgDevice::ForwardRequestWithCompletion(IN WDFREQUEST Request, IN PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine, IN WDFCONTEXT CompletionContext) {
	WdfRequestFormatRequestUsingCurrentType(Request);// Setup the request for the next driver
	WdfRequestSetCompletionRoutine(Request, CompletionRoutine, CompletionContext);// Set the completion routine...

	if (TRUE == WdfRequestSend(Request, m_hIOTarget, NULL))// And send it!
		return;
		
	NTSTATUS status = WdfRequestGetStatus(Request);
	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfRequestSend failed - 0x%x\n", status);

	WdfRequestComplete(Request, status);
}

NTSTATUS UsbCfgDevice::Initialize() {
	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "->UsbCfgDevice::Initialize\n");
	NTSTATUS				status = STATUS_SUCCESS;
	WDF_IO_QUEUE_CONFIG		ioQueueConfig;
	WDFKEY					hRegKey = 0;
	const UNICODE_STRING	usDefCfgIdx = { 16*2, 16*2, L"DefaultCfgIndex" };

	if (0 == (m_hIOTarget = WdfDeviceGetIoTarget(m_hDevice))) {
		status = STATUS_UNSUCCESSFUL;
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "  UsbCfgDevice::Initialize, WdfDeviceGetIoTarget FAILED with 0x%x\n", status);
		goto Finalization;
	}

	if (NT_FAILED(status = WdfDeviceOpenRegistryKey(m_hDevice, PLUGPLAY_REGKEY_DEVICE, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &hRegKey))) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "  UsbCfgDevice::Initialize, WdfDeviceOpenRegistryKey FAILED with %d\n", status);
		goto Finalization;
	}

	ULONG ulCfgIndex = 0xFFFFFFFF;
	if (NT_FAILED(status = WdfRegistryQueryULong(hRegKey, &usDefCfgIdx, &ulCfgIndex))) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "  UsbCfgDevice::Initialize, WdfRegistryQueryULong FAILED with %d\n", status);
		goto Finalization;
	}
	m_btZeroCfgSwitch = (BYTE)ulCfgIndex;
	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "  UsbCfgDevice::Initialize, 'm_btZeroCfgSwitch' = %d\n", m_btZeroCfgSwitch);

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
	ioQueueConfig.EvtIoInternalDeviceControl = [](IN WDFQUEUE q, IN WDFREQUEST r, IN size_t obl, IN size_t ibl, IN ULONG ioctl) {
		UsbCfgDevice* pThis = DeviceGetSingleton(WdfIoQueueGetDevice(q));
		if (TRUE == pThis->OnFilterInternalDeviceControl(q, r, obl, ibl, ioctl))
			pThis->ForwardRequest(r);// TRUE returned, forward the request
	};
	//ioQueueConfig.EvtIoWrite = __wdfFltrWrite;// We don't want Writes for now
	if (NT_FAILED(status = WdfIoQueueCreate(m_hDevice, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, NULL)))// No need to delete the WDFDEVICE we created, the framework will clean that up for us.
		goto Finalization;
	WDF_USB_DEVICE_CREATE_CONFIG	createParams;
	WDF_USB_DEVICE_CREATE_CONFIG_INIT(&createParams, USBD_CLIENT_CONTRACT_VERSION_602);
	if (NT_FAILED(status = WdfUsbTargetDeviceCreateWithParameters(m_hDevice, &createParams, WDF_NO_OBJECT_ATTRIBUTES, &m_hUsbDevice)))
		goto Finalization;
Finalization:
	if (0 != hRegKey)
		WdfRegistryClose(hRegKey);
	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "<-UsbCfgDevice::Initialize, returns 0x%.8x\n", status);
	return status;
}

// see http://www.codeproject.com/Articles/335364/Simple-URB-USB-Request-Block-Monitor for details
BOOL UsbCfgDevice::OnFilterInternalDeviceControl(IN WDFQUEUE /*Queue*/, IN WDFREQUEST Request, IN size_t /*OutputBufferLength*/, IN size_t /*InputBufferLength*/, IN ULONG IoControlCode) {
	if (IOCTL_INTERNAL_USB_SUBMIT_URB != IoControlCode)
		return TRUE;// This is not what we are looking for, FWD the request

	PIRP				pIrp	= WdfRequestWdmGetIrp(Request);
	PIO_STACK_LOCATION  pStack	= IoGetCurrentIrpStackLocation(pIrp);
	if (IRP_MJ_INTERNAL_DEVICE_CONTROL != pStack->MajorFunction) {
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, pStack->MajorFunction:%xh\r\n", pStack->MajorFunction);
		return TRUE;
	}

	_URB_SELECT_CONFIGURATION*	pCfg = 0;
	PURB						pUrb = (PURB)pStack->Parameters.Others.Argument1;

	switch (pUrb->UrbHeader.Function) {
		case URB_FUNCTION_CONTROL_TRANSFER:
			if ((USB_REQUEST_GET_DESCRIPTOR != pUrb->UrbControlTransfer.SetupPacket[1]) || (USB_CONFIGURATION_DESCRIPTOR_TYPE != pUrb->UrbControlTransfer.SetupPacket[3])) {
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, URB_FUNCTION_CONTROL_TRANSFER, Request:%d, ValueType:%d, Value:%d.\r\n" 
						, pUrb->UrbControlTransfer.SetupPacket[1], pUrb->UrbControlTransfer.SetupPacket[3], pUrb->UrbControlTransfer.SetupPacket[2]);
				break;
			}

			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, URB_FUNCTION_CONTROL_TRANSFER, Request:USB_REQUEST_GET_DESCRIPTOR, ValueType:USB_CONFIGURATION_DESCRIPTOR_TYPE, Value:%d.\r\n", pUrb->UrbControlTransfer.SetupPacket[2]);
			if (USB_DEFAULT_CFG_INDEX == pUrb->UrbControlTransfer.SetupPacket[2])
				pUrb->UrbControlTransfer.SetupPacket[2] = m_btZeroCfgSwitch;
			else if (m_btZeroCfgSwitch == pUrb->UrbControlTransfer.SetupPacket[2])
				pUrb->UrbControlTransfer.SetupPacket[2] = USB_DEFAULT_CFG_INDEX;
			break;
		case URB_FUNCTION_GET_CONFIGURATION:
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, URB_FUNCTION_GET_CONFIGURATION.\r\n");
			break;
		case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
			if(USB_CONFIGURATION_DESCRIPTOR_TYPE != pUrb->UrbControlDescriptorRequest.DescriptorType)
				return TRUE;// This is not what we are looking for...
			if (USB_DEFAULT_CFG_INDEX == pUrb->UrbControlDescriptorRequest.Index) {
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, Get USB_CONFIGURATION_DESCRIPTOR_TYPE, cfg:%d changed to cfg:%d\r\n", pUrb->UrbControlDescriptorRequest.Index, m_btZeroCfgSwitch);
				pUrb->UrbControlDescriptorRequest.Index = m_btZeroCfgSwitch;
			} else if (m_btZeroCfgSwitch == pUrb->UrbControlDescriptorRequest.Index) {
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, Get USB_CONFIGURATION_DESCRIPTOR_TYPE, cfg:%d changed to Zero\r\n", pUrb->UrbControlDescriptorRequest.Index);
				pUrb->UrbControlDescriptorRequest.Index = USB_DEFAULT_CFG_INDEX;
			} else {
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, Get USB_CONFIGURATION_DESCRIPTOR_TYPE, cfg:%d Un-changed\r\n", pUrb->UrbControlDescriptorRequest.Index);
			}
			break;
		case URB_FUNCTION_SELECT_CONFIGURATION:
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, URB_FUNCTION_SELECT_CONFIGURATION, bConfigurationValue:%xh\r\n", pUrb->UrbSelectConfiguration.ConfigurationDescriptor->bConfigurationValue);
			pCfg = &pUrb->UrbSelectConfiguration;
			break;
		default:
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "UsbCfgDevice::OnFilterInternalDeviceControl, Function:%xh\r\n", pUrb->UrbHeader.Function);
			break;
	}

	return TRUE;
}



