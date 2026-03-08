# Clone Coreboot
git clone https://codeberg.org/Abdulghani121/coreboot-ps4.git 
cd coreboot-ps4

# Copy PS4-specific files to appropriate directories
cp -r ../sony/ src/mainboard/
cp -r ../aeolia/ src/southbridge/sony/
cp -r ../ps4_belize/ src/northbridge/amd/
cp -r ../jaguar/ src/cpu/amd/

# Configure
make menuconfig
# Select: Mainboard -> Sony -> PS4 Fat (CUH-1004A)

# Build
make