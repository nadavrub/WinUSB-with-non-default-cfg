#include "driver.h"
#include "driver.tmh"
#include "Trace.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT  DriverObject,IN PUNICODE_STRING RegistryPath) {
	NTSTATUS				status;
	WDF_DRIVER_CONFIG		config;
    WDF_OBJECT_ATTRIBUTES	attributes;
	
#if defined(_DEBUG) || defined(DBG)
//	if (TRUE == KdRefreshDebuggerNotPresent())
//		return STATUS_UNSUCCESSFUL;// For the time being, don't load the driver if no debugger is attached
//	DbgBreakPoint();
	if (FALSE == KdRefreshDebuggerNotPresent())
		DbgBreakPoint();
#endif
	
    // Initialize WPP Tracing
    WPP_INIT_TRACING( DriverObject, RegistryPath );
	WPP_FLAG_LEVEL_ENABLED(MYDRIVER_ALL_INFO, TRACE_LEVEL_VERBOSE);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "UsbCfgCtrl>> DriverEntry " __DATE__ " " __TIME__ "\r\n");

    // Register a cleanup callback so that we can call WPP_CLEANUP when the framework driver object is deleted during driver unload.
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = [](IN WDFOBJECT DriverObject) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");
		// Stop WPP Tracing
		WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)DriverObject));
	};

	WDF_DRIVER_CONFIG_INIT(&config, UsbCfgDevice::CreateDevice);// UsbCfgCtrlEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, &config, WDF_NO_HANDLE);
    if (NT_FAILED(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!\r\n", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit\r\n");

    return status;
}
