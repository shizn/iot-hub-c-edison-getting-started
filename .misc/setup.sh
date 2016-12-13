#!/bin/bash

# This script updates a fresh Ubuntu installation with all the dependent
# components necessary to use the IoT Client SDK for C.

# install yocto sysroot for edison
# pls refer to https://github.com/intel-iot-devkit/mraa/wiki/Cross-compiling-for-Yocto-using-a-Toolchain-SDK
scriptdir=$(cd "$(dirname "$0")" && pwd)

push_dir () { pushd $1 > /dev/null; }
pop_dir () { popd $1 > /dev/null; }

sysroot_install ()
{
    push_dir ~/Downloads
    if [ ! -f "edison-sdk-linux64-ww25.5-15.zip" ]; then
        wget http://downloadmirror.intel.com/25028/eng/edison-sdk-linux64-ww25.5-15.zip
    fi

    if [ ! -d "/opt/poky-edison/1.7.2" ]; then
        unzip -o edison-sdk-linux64-ww25.5-15.zip
        echo -ne '\n Y' | sudo ./poky-edison-glibc-x86_64-edison-image-core2-32-toolchain-1.7.2.sh
    fi
    pop_dir
}

sysroot_install