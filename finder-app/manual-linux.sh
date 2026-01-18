#!/bin/bash
# Script outline to install and build kernel.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-
SKIP_KERNEL_BUILD=${SKIP_KERNEL_BUILD:-true}

if env | grep -q "^SKIP_BUILD="; then
    SKIP_KERNEL_BUILD=true
fi

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"



if [ "$SKIP_KERNEL_BUILD" = false ]; then
    if [ ! -d "${OUTDIR}/linux-stable" ]; then
        echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
        git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
    fi

    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    echo "Building kernel"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} Image

    cp arch/${ARCH}/boot/Image ${OUTDIR}/Image
else
    echo "Skipping kernel build (CI mode)"
fi

# Mandatory check (CI expects this)
if [ ! -f "${OUTDIR}/Image" ]; then
    echo "Missing kernel image at ${OUTDIR}/Image"
    exit 1
fi



fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image
if [ ! -f ${OUTDIR}/Image ]; then
    echo "ERROR: Missing kernel Image at ${OUTDIR}/Image"
    exit 1
fi


echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

#Create necessary base directories
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    #Configure busybox
    make distclean
    make defconfig
    sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config

else
    cd busybox
fi

#Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc)
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install
file ${OUTDIR}/rootfs/bin/busybox #checking file type

echo "Library dependencies readelf"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox 
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox 
echo "Library dependencies to rootfs"
#Library dependencies to rootfs
SYSROOT=/usr/aarch64-linux-gnu

echo "Copying Library dependencies to rootfs"

cp -a ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp -a ${SYSROOT}/lib/libc.so.6 ${OUTDIR}/rootfs/lib/
cp -a ${SYSROOT}/lib/libm.so.6 ${OUTDIR}/rootfs/lib/
cp -a ${SYSROOT}/lib/libresolv.so.2 ${OUTDIR}/rootfs/lib/
echo "Make device nodes"
#Make device nodes
cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
echo "Clean and build the writer utility"
#Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
cp writer ${OUTDIR}/rootfs/home/

echo "Copy the finder related scripts and executables to the /home directory on the target rootfs"
#Copy the finder related scripts and executables to the /home directory on the target rootfs
cp finder.sh ${OUTDIR}/rootfs/home/
cp finder-test.sh ${OUTDIR}/rootfs/home/
mkdir ${OUTDIR}/rootfs/home/conf
cp /home/badu/assignments/finder-app/conf/username.txt ${OUTDIR}/rootfs/home/conf
cp /home/badu/assignments/finder-app/conf/assignment.txt ${OUTDIR}/rootfs/home/conf
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/
echo "Chown the root directory"
#Chown the root directory
sudo chown -R root:root ${OUTDIR}/rootfs
echo "Create initramfs.cpio.gz"
#Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root | gzip > ${OUTDIR}/initramfs.cpio.gz
echo "Finished"

