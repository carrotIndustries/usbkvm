import os
import os.path
import sys
import subprocess
from string import Template
import xml.etree.ElementTree as ET


cpp_template = """#include "version.hpp"
const unsigned int Version::major = $major;
const unsigned int Version::minor = $minor;
const unsigned int Version::micro = $micro;
const char *Version::commit = "$commit";
const char *Version::commit_hash = "$commit_hash";
"""

meson_version = sys.argv[3]
major,minor,micro = [int(x) for x in meson_version.split(".")]

srcdir = sys.argv[2]

tree = ET.parse(os.path.join(srcdir, 'net.carrotindustries.usbkvm.metainfo.xml'))
root = tree.getroot()
release = root.find("releases").find("release")
version_from_xml = release.attrib["version"]

rc = 0
if version_from_xml != meson_version :
	print("Version mismatch %s != %s"%(meson_version, version_from_xml))
	rc = 1

if release.find("url").text.strip() != f"https://github.com/carrotIndustries/usbkvm/releases/tag/v{meson_version}" :
	print("Release URL mismatch")
	rc = 1

if rc != 0 :
	exit(rc)


gitversion = ""
commit_hash = ""
if os.path.isdir(os.path.join(srcdir, "../.git")):
	gitversion = subprocess.check_output(['git', 'log', '-1', '--pretty=format:%h %ci %s'], cwd=srcdir).decode()
	gitversion = gitversion.replace("\"", "\\\"")
	commit_hash = subprocess.check_output(['git', 'log', '-1', '--pretty=format:%h'], cwd=srcdir).decode()
	rc = subprocess.run(["git", "describe", "--tags", "--exact-match"], capture_output=True)
	if rc.returncode == 0 : #have tag
		tag = rc.stdout.decode().strip()
		if tag != f"v{meson_version}" :
			print("Tag mismatch")
			exit(1)

outfile = sys.argv[1]
with open(outfile, "w", encoding="utf-8") as fi:
	tmpl = Template(cpp_template)
	fi.write(tmpl.substitute(major=major, minor=minor, micro=micro, string=meson_version, commit=gitversion, commit_hash=commit_hash))
