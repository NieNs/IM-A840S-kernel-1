#!/bin/bash
###############################################################################
#
#                           Kernel Build Script 
#
###############################################################################
# 2010-12-29 allydrop     : created
###############################################################################


export ARCH=arm
export CROSS_COMPILE=../../../../../../prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-

make 

