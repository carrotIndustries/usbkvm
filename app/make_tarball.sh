#!/bin/bash
set -euo pipefail

rm -f usbkvm.tar.gz
rm -rf fw
mkdir fw
cp ../fw/boot/Core/Inc/i2c_msg_boot.h fw
cp ../fw/usbkvm/Core/Inc/i2c_msg_app.h fw
cp ../fw/common/Inc/i2c_msg_common.h fw
cp ../fw/usbkvm/build/usbkvm-header.bin fw

{ git ls-files --recurse-submodules && ls fw/* ;} | grep -v keycodemapdb/tests | grep -v ms-tools/board/ | \
 tar caf usbkvm.tar.gz  --xform s:^:usbkvm/: -T -

rm -rf fw
