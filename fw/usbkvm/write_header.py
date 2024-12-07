#!/usr/bin/python3

import sys
import struct
import binascii
import datetime

magic = 0x1337

revbits = lambda z, bits: sum(((z&(1<<i))!=0)*(1<<(bits-1-i)) for i in range(bits))
rev32 = lambda z: revbits(z, 32)
rev8 = lambda z: revbits(z, 8)

def calc_crc(data):
    return rev32(binascii.crc32(bytes(rev8(x) for x in data))^0xffffffff)

header_fmt = "HHIII"

with open(sys.argv[1], "rb") as infile:
    rom = infile.read()
    magic, app_version, app_size, app_crc, header_crc = struct.unpack(header_fmt, rom[:struct.calcsize(header_fmt)])
    if magic != 0x1337 :
        raise ValueError("wrong magic")
    app_size = len(rom) - 0x20 #header
    app_crc = calc_crc(rom[0x20:])
    header_without_crc = struct.pack(header_fmt[:-1], magic, app_version, app_size, app_crc)
    header_crc = calc_crc(header_without_crc)
    rom_list = list(rom)
    header = list(struct.pack(header_fmt, magic, app_version, app_size, app_crc, header_crc))
    rom_list[:len(header)] = header
    rom_list += [0]*(256-len(rom)%256)
    with open(sys.argv[2], "wb") as outfile :
        outfile.write(bytes(rom_list))
"""
inf = list(struct.pack("II22s", sig_expected, len(rom), date.encode()))
rom = list(rom)
rom[info_base:info_base+len(inf)] = inf
rom = bytes(rom)
crc = rev32(binascii.crc32(bytes(rev8(x) for x in rom))^0xffffffff)
print(hex(crc))
with open(sys.argv[2], "wb") as outfile :
    outfile.write(rom)
    outfile.write(struct.pack("I", crc))
"""
