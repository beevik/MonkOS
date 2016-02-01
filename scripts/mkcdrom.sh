#!/bin/bash

# Create a directory structure to hold the ISO image files.
rm -rf build/iso
mkdir -p build/iso
mkdir -p build/iso/boot

# Copy the stage 1 boot loader
cp build/boot.sys build/iso/boot/boot.sys

# Copy the stage 2 boot loader
cp build/loader.sys build/iso/loader.sys

# Copy the kernel and strip it
cp build/monk.sys build/iso/monk.sys
strip build/iso/monk.sys

# Generate the ISO file (with a boot catalog)
genisoimage -R -J \
	-c boot/bootcat \
	-b boot/boot.sys \
	-no-emul-boot \
	-boot-load-size 4 \
	-o build/monk.iso \
	./build/iso
