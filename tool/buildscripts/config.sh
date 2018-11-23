#!/bin/sh
#---------------------------------------------------------------------------------
# variables for unattended script execution
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# Select package
#---------------------------------------------------------------------------------
#   0: User selects manually
#   1: devkitARM
#   2: devkitPPC
#   3: devkitPSP
#---------------------------------------------------------------------------------
BUILD_DKPRO_PACKAGE=1

#---------------------------------------------------------------------------------
# Toolchain installation directory, comment if not specified
#---------------------------------------------------------------------------------
BUILD_DKPRO_INSTALLDIR=$HOME/Library/devkitpro/r45

#---------------------------------------------------------------------------------
# Path to previously downloaded source packages, comment if not specified
#---------------------------------------------------------------------------------
BUILD_DKPRO_SRCDIR=$(realpath $(dirname $0)/../archive)

#---------------------------------------------------------------------------------
# Automated script execution
#---------------------------------------------------------------------------------
#  0: Ask to delete build folders & patched sources
#  1: Use defaults, don't pause for answers
#---------------------------------------------------------------------------------
BUILD_DKPRO_AUTOMATED=0
