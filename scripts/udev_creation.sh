#!/bin/sh
/sbin/start_udev
udevadm trigger --action=add --subsystem-match=pci --attr-match=class="0x0c03*"
udevadm trigger --action=add --subsystem-match=block
udevadm settle &
udevadm trigger --action=add --subsystem-match=mem --subsystem-match=input --subsystem-match=tty --subsystem-match=acpi --subsystem-match=rtc --subsystem-match=hid
udevadm trigger --action=add --subsystem-match=pci --attr-match=class="0x060000*"
udevadm settle &

/sbin/udevadm trigger --action=add --subsystem-nomatch=tty --subsystem-nomatch=block --subsystem-nomatch=mem --subsystem-nomatch=input --subsystem-nomatch=acpi --subsystem-nomatch=rtc --subsystem-nomatch=hid
/sbin/udevadm settle --timeout=5 > /dev/null 2>&1
/sbin/udevadm control --env=STARTUP=
mknod /dev/root b 8 2
