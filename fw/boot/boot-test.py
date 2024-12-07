import sys
import time
import itertools

sys.path.append("../../app/build")
import usbkvm_py as usbkvm

def load_flash(filename) :
    data = list(open(filename, "rb").read())
    page_size = 1024
    n_pages = (len(data) + page_size-1)//page_size
    print(f"{n_pages} pages")
    m.boot_flash_unlock()
    print("erasing")
    m.boot_flash_erase(8, n_pages)
    print("erased")
    chunk_size = 256
    for chunk in range(len(data)//chunk_size) :
        file_offset = chunk*chunk_size
        flash_offset = 0x8002000 + file_offset
        print(f"{chunk}/{len(data)//chunk_size}")
        print(m.boot_flash_write(flash_offset, data[file_offset:file_offset+chunk_size]))

d=usbkvm.Device("USBKVM")
m = d.mcu()
info = m.get_info()
print(info.version, info.in_bootloader)
