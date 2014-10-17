WinUSB-with-non-default-cfg
===========================

While WinUSB was designed to simplify USB development, Comparing to a fully fetched USB Kernel mode driver, As of 
Windows 8.1, WinUSB has few limitations, one, is the fact that while a USB device might expose multiple Configurations 
to choose from, WinUSB supports only the default Configuration ( the first one ).
In this project we use a lower KDMF filter driver to force WinUSB to use the USB configuration we want.

Solution Structure:
UsbCfgCtrl -          The lower filter KDMF driver used to set the desiered USB configuration
UsbCfgCtrl Package -  The Driver deployment package
WinUSBSimple -        A simple Console application demonstrating WinUSB usage with the non-default configuration
