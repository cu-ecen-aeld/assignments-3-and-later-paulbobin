#!/bin/bash
# build kernel, busybox and rootfs

set -e
set -u
echo "Checking finder app files..."

FILES_TO_COPY=(
    "${FINDER_APP_DIR}/writer"
    "${FINDER_APP_DIR}/finder.sh"
    "${FINDER_APP_DIR}/finder-test.sh"
    "${FINDER_APP_DIR}/conf/username.txt"
    "${FINDER_APP_DIR}/autorun-qemu.sh"
)

echo "Files list:"
for f in "${FILES_TO_COPY[@]}"; do
    echo " - $f"
done

for f in "${FILES_TO_COPY[@]}"; do
    if [ ! -e "$f" ]; then
        echo "ERROR: File missing -> $f"
        exit 1
    fi
done

echo "All required files exist"

sudo apt-get update
sudo apt-get install -y \
gcc-aarch64-linux-gnu \
bc bison flex \
libssl-dev \
libncurses5-dev \
git cpio \
qemu-system-aarch64 \
build-essential

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-
FINDER_APP_DIR=$(realpath $(dirname $0))

if [ $# -ge 1 ]; then
    OUTDIR=$1
fi

mkdir -p ${OUTDIR}
cd ${OUTDIR}

# kernel
if [ ! -d linux-stable ]; then
    git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

cd linux-stable
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
make -j2 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} Image

cp arch/${ARCH}/boot/Image ${OUTDIR}/Image

# rootfs
cd ${OUTDIR}
sudo rm -rf rootfs
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin

# busybox
cd ${OUTDIR}
if [ ! -d busybox ]; then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    make distclean
    make defconfig
else
    cd busybox
fi

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=${OUTDIR}/rootfs install

# libs
#SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
SYSROOT=/usr/aarch64-linux-gnu
cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp ${SYSROOT}/lib/libc.so.6 ${OUTDIR}/rootfs/lib/
cp ${SYSROOT}/lib/libm.so.6 ${OUTDIR}/rootfs/lib/
cp ${SYSROOT}/lib/libresolv.so.2 ${OUTDIR}/rootfs/lib/

# device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1

# writer app
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

echo "Copying finder app files..."

cd ${OUTDIR}/rootfs/home
mkdir -p conf

cp -a ${FINDER_APP_DIR}/writer .
cp -a ${FINDER_APP_DIR}/finder.sh .
cp -a ${FINDER_APP_DIR}/finder-test.sh .
cp -a ${FINDER_APP_DIR}/conf/username.txt conf/
cp -a ${FINDER_APP_DIR}/autorun-qemu.sh .

echo "Copy done"
echo "Verifying copied files in rootfs..."

ls -l ${OUTDIR}/rootfs/home
ls -l ${OUTDIR}/rootfs/home/conf
# permissions
cd ${OUTDIR}/rootfs
sudo chown -R root:root .

# initramfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
echo "Done Manual Linux SH"
