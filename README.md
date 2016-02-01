MonkOS
======

MonkOS is an experimental 64-bit operating system for Intel and AMD
processors. It is a work in progress. Currently, it consists of a BIOS boot
loader, a virtual console system, and a keyboard interrupt handler. The boot
loader is designed to launch the operating system from a cdrom. The code
is a mix of C and assembly language.

###Prerequisites

The OS currently builds under linux.  To build it, you must have the following
tools installed:
* gcc x86_64 cross-compiler (I used version 5.3)
* gnu binutils (I used version 2.26)
* NASM assembler
* genisoimage
* gdb (for debugging)
* qemu (optional, for testing)
* doxygen (optional)

See [this page](http://wiki.osdev.org/GCC_Cross-Compiler) for instructions
on installing a gcc cross-compiler.

When building libgcc, make sure to follow the [libgcc without red-
zone](http://wiki.osdev.org/Libgcc_without_red_zone) instructions. Note that
these instructions are a bit out of date. If you are using a more recent
version of gcc (such as 5.3), you'll need to modify the gcc/config.gcc file
(not gcc/config.cc).

###Resources

These are some of the resources I have relied on in my attempt to better
understand the numerous and various aspects of bootloader and OS development:
* [Intel 64 and IA-32 Architectures: Software Developer’s Manual]
  (https://www-ssl.intel.com/content/www/us/en/processors/architectures-software-developer-manuals.html)
* [The OsDev wiki](http://wiki.osdev.org/Main_Page)
  * [Bare Bones guide](http://wiki.osdev.org/Bare_Bones)
  * [Setting up long mode](http://wiki.osdev.org/Setting_Up_Long_Mode)
  * [IOS 9660](http://wiki.osdev.org/ISO_9660)
  * [PS/2 Keyboard](http://wiki.osdev.org/PS2_Keyboard)
* [The System V ABI](http://www.sco.com/developers/gabi/latest/contents.html)
  * [AMD64 supplement](http://www.x86-64.org/documentation/abi.pdf)
* [The El Torito specification]
  (http://download.intel.com/support/motherboards/desktop/sb/specscdrom.pdf)
* [Hardware Level VGA and SVGA Video Programming Information Page]
  (http://www.osdever.net/FreeVGA/vga/vgareg.htm)
  * [CRT Controller Registers](http://www.osdever.net/FreeVGA/vga/crtcreg.htm)
* [Write your own operating system](http://geezer.osdevbrasil.net/osd/index.htm)
  * [PC keyboard](http://geezer.osdevbrasil.net/osd/kbd/index.htm)
* [The Xeos project](https://github.com/macmade/XEOS)
* [BareMetal OS](https://github.com/ReturnInfinity/BareMetal)
* [IanOS](http://www.ijack.org.uk/)

###License

Use of this source code is governed by a BSD-style license that can be found
in the [LICENSE](https://github.com/beevik/MonkOS/blob/master/LICENSE) file.
