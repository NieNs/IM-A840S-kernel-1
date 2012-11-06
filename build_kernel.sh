export CROSS_COMPILE=$HOME/arm-eabi-4.4.3/bin/arm-eabi-
export PATH=$PATH:~/arm-eabi-4.4.3/bin

# Define KERNEL Configuration (depending on the SKY MODEL)
KERNEL_DEFCONFIG=msm8960_ef44s_tp20_defconfig

# Build LINUX KERNEL
make CONFIG_DEBUG_SECTION_MISMATCH=y ARCH=arm CROSS_COMPILE=arm $KERNEL_DEFCONFIG
make CONFIG_DEBUG_SECTION_MISMATCH=y ARCH=arm

# Copy compressed linux kernel (zImage)
cp -f ./arch/arm/boot/zImage .

MODEL_NAME=IM-A840S
KERNEL_BASE_ADDR=80200000
KERNEL_PAGE_SIZE=2048
KERNEL_CMDLINE='console=ttyHSL0,115200,n8 androidboot.hardware=qcom androidboot.carrier=SKT-KOR user_debug=31 msm_rtb.filter=0x3F kgsl.mmutype=gpummu'

BUILD_OUT_NAME=~/kernel/IM-A840S-kernel
BOOT_IMAGE_NAME=boot.img

# Etc informations.
KERNEL_FLASH_CMD="fastboot flash boot boot.img"

# Ramdisk Image, Image build tools dir & name.
IMAGE_TOOLS_DIR=./tools/pantech
RAMDISK_IMAGE_NAME=$IMAGE_TOOLS_DIR/ramdisk.gz
IMAGE_TOOL_NAME=$IMAGE_TOOLS_DIR/mkbootimg 

echo "###############################################"
echo " PanTech kernel build script for $MODEL_NAME"
echo "###############################################"
echo " Start Build kernel..."

echo "###############################################"
echo " Make boot image..." 
echo "###############################################"
$IMAGE_TOOL_NAME --kernel $BUILD_OUT_NAME/zImage --ramdisk $RAMDISK_IMAGE_NAME --base $KERNEL_BASE_ADDR --pagesize $KERNEL_PAGE_SIZE --output $BUILD_OUT_NAME/$BOOT_IMAGE_NAME --cmdline "$KERNEL_CMDLINE"

echo " Completed."

echo "###############################################"
echo " Build completed."
echo " Boot Image file name : $BOOT_IMAGE_NAME "
echo " Image flash command  : $KERNEL_FLASH_CMD "
echo "###############################################"

