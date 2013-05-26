1. How to test

Using dummy_hcd and g_zero modules in linux kernel,
we could test usb-skeleton.c in PC without udc hardware.

a. Prepare
Make sure you have modules dummy_hcd.ko and g_zero.ko.
Then remember to remove usbtest.ko,
or add usbtest to modprobe blacklist.

Take Ubuntu as example, you can do it through:
#apt-get install linux-image-`uname -r`
#echo 'blacklist usbtest' >> /etc/modprobe.d/blacklist.conf

b. Test
#modprobe g_zero loopdefault=1
#insmod usb-skeleton.ko
write to g_zero loopback
#echo 'abcdefghijklmnopqrstuvwxyz...' > /dev/skel0
read from g_zero loopback
#cat /dev/skel0
