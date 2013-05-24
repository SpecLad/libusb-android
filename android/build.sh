#!/bin/bash

set -e
dir=$(dirname "$0")

if [ $# -ne 1 ]; then
  echo "Usage: $0 (armeabi | armeabi-v7a | mips | x86)"
  exit 1
fi

build=$(sh $dir/../config.guess)

echo "Guessed the build system as ${build}."

cflags="-O2 -g"

arch=$1

case $arch in
armeabi)
  host=arm-linux-androideabi
  cflags="$cflags -mthumb"
  ldflags="-Wl,--no-undefined"
  ;;
armeabi-v7a)
  host=arm-linux-androideabi
  cflags="$cflags -mthumb -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16"
  ldflags="-march=armv7-a -Wl,--fix-cortex-a8 -Wl,--no-undefined"
  ;;
mips)
  host=mipsel-linux-android
  ldflags=""
  ;;
x86)
  host=i686-linux-android
  ldflags=""
  ;;
*)
  echo "Unsupported architecture: $arch"
  exit 1
esac

mkdir -p $dir/build-$arch
(cd $dir/build-$arch && ../../configure --host=$host --build=$build CFLAGS="$cflags" LDFLAGS="$ldflags" && make)

echo "Copying headers and libraries..."
mkdir -p $dir/ndk-modules/libusb-1.0/{include/libusb-1.0,lib/$arch}
cp $dir/../libusb/libusb.h $dir/ndk-modules/libusb-1.0/include/libusb-1.0
cp $dir/build-$arch/libusb/.libs/libusb-1.0.so $dir/ndk-modules/libusb-1.0/lib/$arch

