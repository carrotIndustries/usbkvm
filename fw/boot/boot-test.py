import sys
import time
import itertools

sys.path.append("../../app/build")
import usbkvm_py as usbkvm

d=usbkvm.Device("USBKVM")
m = d.mcu()
info = m.get_info()
print(info.version, info.in_bootloader)
