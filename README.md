Herein lies the source for *dslibris*, an EPUB reader for the Nintendo DS.

# Prerequisites

Ubuntu 16.04 LTS is known to work as a build platform.
Other platforms might, as long as the correct toolchain can be obtained.

*   devkitARM r45
*   libnds-1.5.8
*   libfat-1.0.12
*   a media card and a DLDI patcher, but you knew that.

devkitARM r45 is not easily found. If you need to build it,
clone the r45 branch of devkitPro/buildscripts from GitHub
and patch it with the files in tool/buildscripts.
You'll need to obtain the required tarball archives from each
project's release webpage; check out the version numbers in
the main build script.

After installing devkitPro/devkitARM, see

```bash
etc/profile
```

for an example of setting DEVKITPRO and DEVKITARM in your shell.

# Building

To build the dependent libraries,

```shell
cd tool
make
```

The arm-none-eabi-* executables must be in your PATH for the above to work.

Then, to build the program,

```shell
cd ..
make
```

dslibris.nds should show up in your current directory.
for a debugging build,

```shell
DEBUG=1 make
```

# Installation

See INSTALL.

# Screenshots

![UTF-8 Multingual](http://rhaleblian.files.wordpress.com/2007/09/utf8.png)

# Debugging

arm-eabi-gdb, insight-6.8 and desmume-0.9.12-svn5575 have been known to work for debugging. See online forums for means to build an arm-eabi-targeted Insight for your platform.

# More Info

http://github.com/rhaleblian/dslibris
