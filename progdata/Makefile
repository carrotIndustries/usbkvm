../fw/usbkvm/build/usbkvm-header.bin:
	make -C ../fw/usbkvm build/usbkvm-header.bin

../fw/boot/build/usbkvm-boot.bin:
	make -C ../fw/boot

usbkvm-combined.dfu: ../fw/boot/build/usbkvm-boot.bin ../fw/usbkvm/build/usbkvm-header.bin
	../fw/common/dfuse-pack.py -b 0x8000000:../fw/boot/build/usbkvm-boot.bin -b 0x8002000:../fw/usbkvm/build/usbkvm-header.bin $@

dfu: usbkvm-combined.dfu
	dfu-util -D $<
