#!/usr/bin/python3

with open("edid.bin", "rb") as fi:
	for i in range(8) :
		d = fi.read(16)
		print(f"sudo i2ctransfer -y 7 w17@0x50 0x{i*16:02x} {' '.join(hex(x) for x in d)}")

