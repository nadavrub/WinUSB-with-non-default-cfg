WinUSB-with-non-default-cfg
===========================

Project site @ https://nadavrub.wordpress.com/2014/10/16/winusb-with-non-default-configuration/

While WinUSB was designed to simplify USB development, Comparing to a fully fetched USB Kernel mode driver, As of 
Windows 8.1, WinUSB has few limitations, one, is the fact that while a USB device might expose multiple Configurations 
to choose from, WinUSB supports only the default Configuration ( the first one ).<br />
In this project we use a lower KDMF filter driver to force WinUSB to use the USB configuration we want.

Solution Structure:<br />
UsbCfgCtrl -          The lower filter KDMF driver used to set the desiered USB configuration<br />
UsbCfgCtrl Package -  The Driver deployment package<br />
WinUSBSimple -        A simple Console application demonstrating WinUSB usage with the non-default configuration<br />

Usage:<br />
1. Update USB\VID_nnnn&PID_nnnn at UsbCfgCtrl.inx in accordance with the designated HW.<br />
2. Compile 'UsbCfgCtrl' and 'WinUSBSimple'<br /><br />
3. Open 'UsbCfgCtrl Package' Property pages and check [X] 'Enable Deployment' under 'Driver Install'->'Deployment'<br />
4. Select the Target machine, this machine should be provisioned for driver development, see the following link for details:<br />
   http://msdn.microsoft.com/en-us/library/windows/hardware/dn265573(v=vs.85).aspx<br />
5. Deploy by running 'UsbCfgCtrl Package'<br />
6. On the target machine Device manager verify the driver is properly functioning<br />
7. Remote Debug 'WinUSBSimple.exe' on the target machine.<br />
