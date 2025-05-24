#!/usr/bin/python3
import pyudev
import time
import subprocess
from termcolor import cprint
import sys
import itertools

sys.path.append("../app/build")
import usbkvm_py as usbkvm

print("Welcome to the USBKVM mass production tool")

context = pyudev.Context()

def count_product(product) :
	devs = context.list_devices(subsystem='usb').match_attribute("idVendor", "534d").match_attribute("idProduct", "2109").match_attribute("product", product)
	ndevs = len(list(devs))
	return ndevs

def count_bootloader() :
	devs = context.list_devices(subsystem='usb').match_attribute("idVendor", "0483").match_attribute("idProduct", "df11").match_attribute("product", "STM32  BOOTLOADER")
	ndevs = len(list(devs))
	return ndevs

def count_hid() :
	devs = context.list_devices(subsystem='usb').match_attribute("idVendor", "cafe").match_attribute("idProduct", "4008").match_attribute("product", "TinyUSB Device")
	ndevs = len(list(devs))
	return ndevs

def count_unprogrammed_ms2109() :
	return count_product("USB Video")

def count_programmed_ms2109() :
	return count_product("USBKVM")

if count_programmed_ms2109() != 1 :
	while True:
		ndevs = count_unprogrammed_ms2109()
		if ndevs == 1 :
			cprint("Found unprogrammed MS2109", "green", attrs=["bold"])
			break
		elif ndevs == 0 :
			print("waiting for unprogrammed MS2109")
			time.sleep(1)
		else :
			print("connect only one MS2109")
			time.sleep(1)

	print("programming MS2109 EEPROM")
	subprocess.check_call(["../app/ms-tools/cli/cli", "write-file", "EEPROM", "0", "../app/src/ms2109-eeprom.bin"])
	cprint("EEPROM programming done", "green", attrs=["bold"])
	print("reconnect USBKVM")

while True:
	ndevs = count_programmed_ms2109()
	if ndevs == 1 :
		cprint("Found programmed MS2109", "green", attrs=["bold"])
		break
	elif ndevs == 0 :
		print("waiting for programmed MS2109")
		time.sleep(1)
	else :
		print("connect only one MS2109")
		time.sleep(1)

if count_hid() != 1 :
	while True:
		ndevs = count_bootloader()
		if ndevs == 1 :
			cprint("Found MCU in bootloader", "green", attrs=["bold"])
			break
		elif ndevs == 0 :
			print("waiting for MCU in bootloader")
			time.sleep(1)
		else :
			print("connect only one MCU in bootloader")
			time.sleep(1)

	print("programming MCU")
	subprocess.check_call(["make", "dfu"])
	cprint("MCU programming done", "green", attrs=["bold"])

	print("programming option bytes")
	subprocess.check_call(["dfu-util", "-a", "1", "-s", "0x1FFFF800:will-reset", "-D", "option.bin"])

time.sleep(1)
dev = usbkvm.Device("USBKVM")
mcu = dev.mcu()
dev_info = mcu.get_info()

if dev_info.version == 5 and dev_info.model == usbkvm.Model.USBKVM :
	cprint("MCU version and model okay", "green", attrs=["bold"])
else :
	cprint(f"MCU version and model mismatch, have version:{dev_info.version}, model:{dev_info.model.name}", "red", attrs=["bold"])
	#print("entering bootloader")
	#dev.enter_bootloader()
	exit()

if dev_info.in_bootloader :
	print("found MCU in bootloader, starting app")
	mcu.boot_start_app()
	time.sleep(.1)
	info2 = mcu.get_info()
	assert(not info2.in_bootloader)


while True:
	ndevs = count_hid()
	if ndevs == 1 :
		cprint("Found programmed MCU", "green", attrs=["bold"])
		break
	elif ndevs == 0 :
		print("waiting for programmed MCU")
		time.sleep(1)
	else :
		print("connect only one MCU")
		time.sleep(1)

led_seq = [usbkvm.Led.NONE, usbkvm.Led.USB, usbkvm.Led.HID, usbkvm.Led.HDMI]

while True :
	print("blinking LEDs")
	for led in itertools.islice(itertools.cycle(led_seq), 10) :
		mcu.set_led(usbkvm.Led.ALL, led)
		time.sleep(.2)

	mcu.set_led(usbkvm.Led.ALL, usbkvm.Led.NONE)

	led_result = input("Did LEDs blink in sequence [Y]?")
	if led_result.lower() in ("Y", "") :
		break

mcu.set_led(usbkvm.Led.NONE, usbkvm.Led.NONE)
