import os
import sys
import subprocess
from string import Template

rc_template = """2 ICON "src/icons/usbkvm.ico"

1 VERSIONINFO
FILEVERSION     $major,$minor,$micro,0
PRODUCTVERSION  $major,$minor,$micro,0
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "040904E4"
    BEGIN
      VALUE "FileDescription", "USBKVM"
      VALUE "InternalName", "usbkvm"
      VALUE "OriginalFilename", "usbkvm.exe"
      VALUE "ProductName", "USBKVM"
      VALUE "ProductVersion", "$major.$minor.$micro"
    END
  END
  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x809, 1252
  END
END
"""

outfile = sys.argv[1]
major,minor,micro = sys.argv[2].split(".")
with open(outfile, "w", encoding="utf-8") as fi:
	tmpl = Template(rc_template)
	fi.write(tmpl.substitute(major=major, minor=minor, micro=micro))
