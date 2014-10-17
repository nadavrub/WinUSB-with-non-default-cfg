#pragma once

#define INITGUID

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <usbdlib.h>
#include <wdfusb.h>

#include <winerror.h>
#include <minwindef.h>

#include "device.h"
#include "trace.h"

#ifndef ZeroMemory
#define ZeroMemory RtlZeroMemory
#endif

#ifndef CopyMemory
#define CopyMemory RtlCopyMemory
#endif

#define NT_SUCCEEDED NT_SUCCESS
#define NT_FAILED !NT_SUCCESS

#define _ASSERT_BREAK(expr) \
	if (FALSE == (expr))	\
		DbgBreakPoint();


// WDFDRIVER Events
extern "C" {
	DRIVER_INITIALIZE DriverEntry;
}