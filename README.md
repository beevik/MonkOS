MonkOS
======

MonkOS is an experimental 64-bit operating system for Intel and AMD
processors. It is a mix of C and assembly language and is a work in progress.
Currently, it consists of a BIOS boot loader, a virtual console system, an
interrupt handling system, a paged memory manager, a heap allocator, a few
simple device drivers, and a primitive interactive shell. The boot loader is
designed to launch the operating system from a cdrom.

## Building

The OS currently builds under linux using a cross-compiler.  There are two
ways to build it: (1) by installing all the necessary tools on your system and
running `make`, or (2) by using a ready-made docker container that contains
all the build tools you'll need.

### Building with local tools

To build with tools installed on your local system, you'll need to have
the following software already installed:

* gcc x86_64 cross-compiler for elf binaries (I have used versions 4.8, 5.3,
  6.3, 7.3, and 8.1 successfully)
* gnu binutils (I used version 2.30)
* nasm assembler
* genisoimage
* exuberant-ctags (optional, for help with editing)
* gdb (optional, for debugging)
* qemu (optional, for testing)
* doxygen (optional)

Most of these tools are available from standard linux package managers. The
cross-compiler, however, is not. To build a cross-compiler, consult the
instructions on [this page](http://wiki.osdev.org/GCC_Cross-Compiler). Make
sure to also follow the [libgcc without
red-zone](http://wiki.osdev.org/Libgcc_without_red_zone) instructions.

Once you've installed all the tools and made sure the cross-compiler is in
your path, run `make`.

```bash
$ make
```

This results in a bootable cdrom ISO file called `monk.iso` in your build
subdirectory.

### Building with docker-ized tools

Because it can be a bit of a hassle to build and install a cross-compiler, a
docker container has been prepared, allowing you to avoid building the cross-compiler
yourself.  To run the docker-based build, make sure you have a recent
version of docker installed on your system, add yourself to your system's
docker user group, and then type the following:

```bash
$ make docker
```

This will pull down the docker container
([`brett/monkos-build`](https://hub.docker.com/r/brett/monkos-build/))
if you don't already have it, run the build inside the container, and generate
the iso file (and all other intermediate output files) in your build
subdirectory.  It behaves almost exactly as if you ran `make` using a cross-
compiler installed locally on your system.

## Running MonkOS

There are several ways to run MonkOS once you have the iso file. The first and
most time-consuming way is to burn it to a CD or DVD ROM using your favorite
burning utility. This is the only way to test MonkOS on a bare-metal system.

Alternatively, you can launch the operating system using virtual machine
software like VMware or virtualbox.

Or you can run the operating system in a linux-based emulator like qemu or
bochs.  The MonkOS makefile makes this alternative particularly easy by
providing a simple build rule to launch the OS in qemu:

```bash
$ make test
```

You can also use the makefile to start a kernel debugging session under qemu
and gdb. First, launch qemu in debugging mode:

```bash
$ make debug
```

Then start a gdb debugger session by attaching gdb to the qemu debugger
endpoint:

```
$ gdb
(gdb) set arch i386:x86-64
(gdb) symbol-file build/monk.sys
(gdb) target remote localhost:8864
(gdb) layout src
```

## Other build options

Run `make docs` to build nicely formatted documentation for MonkOS. You'll
need doxygen installed on your system to do this.

```bash
$ make docs
```

The doxygen documents will appear in the `docs/monk` subdirectory. To view
them, launch them in your browser (in this case firefox):

```bash
$ firefox docs/monk/index.html
```

To build code tags for easy symbol searching within your editor, use the
makefile to run the exuberant-tags utility:

```bash
$ make tags
```

This produces a `.tags` file in your MonkOS directory.

To clean up all intermediate files, use the clean build:

```bash
$ make clean
```

To clean all generated dependencies files, use the cleandeps build:

```bash
$ make cleandeps
```

## Documentation

Please consult the
[Doxygen-formatted documentation](https://beevik.github.io/MonkOS/docs/monk/index.html),
which is part of the [MonkOS documentation set](https://beevik.github.io/MonkOS/).

## Resources

These are some of the resources I have relied on in my attempt to better
understand the numerous and various aspects of bootloader and OS development:

* [Intel 64 and IA-32 Architectures: Software Developer’s Manual](https://www-ssl.intel.com/content/www/us/en/processors/architectures-software-developer-manuals.html)
* [The OsDev wiki](http://wiki.osdev.org/Main_Page)
  * [Bare Bones guide](http://wiki.osdev.org/Bare_Bones)
  * [Setting up long mode](http://wiki.osdev.org/Setting_Up_Long_Mode)
  * [ISO 9660](http://wiki.osdev.org/ISO_9660)
  * [PS/2 Keyboard](http://wiki.osdev.org/PS2_Keyboard)
  * [PCI](http://wiki.osdev.org/PCI)
* [The System V ABI](http://www.sco.com/developers/gabi/latest/contents.html)
  * [AMD64 supplement](http://www.x86-64.org/documentation/abi.pdf)
* [The El Torito specification](http://download.intel.com/support/motherboards/desktop/sb/specscdrom.pdf)
* [Hardware Level VGA and SVGA Video Programming Information Page](http://www.osdever.net/FreeVGA/vga/vgareg.htm)
  * [CRT Controller Registers](http://www.osdever.net/FreeVGA/vga/crtcreg.htm)
  * [Advanced Programmable Interrupt Controller](http://www.osdever.net/tutorials/view/advanced-programming-interrupt-controller)
* [The PCI Database](http://pcidatabase.com/)
* [Write your own operating system](http://geezer.osdevbrasil.net/osd/index.htm)
  * [PC keyboard](http://geezer.osdevbrasil.net/osd/kbd/index.htm)
* [The Xeos project](https://github.com/macmade/XEOS)
* [BareMetal OS](https://github.com/ReturnInfinity/BareMetal)
* [IanOS](http://www.ijack.org.uk/)

## License

Use of this source code is governed by a BSD-style license that can be found
in the [LICENSE](https://github.com/beevik/MonkOS/blob/master/LICENSE) file.
