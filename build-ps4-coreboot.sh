#!/bin/bash

# Build script for PS4 Fat Coreboot

set -e

# Configuration
COREBOT_DIR="coreboot-ps4"
PS4_MODEL="ps4-fat"
CROSS_COMPILE="x86_64-linux-gnu-"

echo "Building Coreboot for PS4 Fat (CUH-1004A)"

# Create directory structure
mkdir -p $COREBOT_DIR/src/mainboard/sony/ps4-fat
mkdir -p $COREBOT_DIR/src/southbridge/sony/aeolia
mkdir -p $COREBOT_DIR/src/northbridge/amd/ps4_belize
mkdir -p $COREBOT_DIR/src/cpu/amd/jaguar
mkdir -p $COREBOT_DIR/payloads/ps4_linux
mkdir -p $COREBOT_DIR/configs

# Copy source files
# (Assume files are in current directory)

# Set up cross-compiler
export CC="${CROSS_COMPILE}gcc"
export LD="${CROSS_COMPILE}ld"
export OBJCOPY="${CROSS_COMPILE}objcopy"

# Configure Coreboot
cd $COREBOT_DIR
make distclean
cp configs/config.ps4_fat .config
make oldconfig

# Build
make -j$(nproc)

echo "Build complete. Coreboot ROM: build/coreboot.rom"