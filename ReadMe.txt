What's new in version 1.4.0 ?
Compatibility with version 1.4 of the Symbian application, meaning WiFi support and better resolution.

1. Installation/compilation on PC

Install the .deb file; to install from source: ./configure make & make install

To compile and install the driver: download the src package from sorceforge and 
from the driver_src directory type:

make -C /lib/modules/`uname -r`/build M=`pwd`

After successfully compiling the driver you should have the file smartcam.ko.

2 Installation on phone

Just transfer any of the files from /usr/share/smartcam/phone_installs (SmartCamS602ndEd_v1_4.sis or SmartCamS603rdEd_v1_4.sis)
that match your phone on your mobile and follow the installation instructions.

3. Running

Before running smartcam, the driver must be loaded in the kernel; if not the PC application is 
pretty much useless. To load the driver, open a shell as root in src/driver path and type:

	/sbin/modprobe videodev
	/sbin/insmod smartcam.ko

After this start the application on the PC, start the phone application and connect it to your PC.
You should now see video images on the PC application window.

4. 3rd party applications

SmartCam was tested on Ubuntu 9.04, kernel version 2.6.28-11-generic
SmartCam works with Skype, Ekiga, aMsn and gstreamer (launch gstreamer-properties, go to the video
tab, Default Input, and press Test to see if it is supported by gstreamer). Unfortunately it doesn't
work anymore with Kopete.

Enjoy :)

