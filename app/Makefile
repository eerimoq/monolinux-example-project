INC += src
SRC += src/lzma.c
SRC += src/async_main.c
SRC += src/publisher.c
SRC += src/network_filter.c
INITRAMFS_FILES += build/linux/fs/ext4/ext4.ko
INITRAMFS_FILES += build/linux/fs/jbd2/jbd2.ko
INITRAMFS_FILES += build/linux/fs/mbcache.ko
QEMU_DISKS += build/mldisk.img
QEMU_DISKS += disk2/fs.img
QEMU_DISKS += disk2/table.img

.PHONY: default

default:
	$(MAKE) all

build: build/mldisk.img

build/mldisk.img: mldisk.img
	mkdir -p build
	cp $< $@

mldisk:
	create_file_storage.sh $@ detools

include $(ML_ROOT)/make/common.mk
include $(ML_ROOT)/make/packages/curl.mk
include $(ML_ROOT)/make/packages/heatshrink.mk
include $(ML_ROOT)/make/packages/xz.mk
include $(ML_ROOT)/make/packages/detools.mk
include $(ML_ROOT)/make/packages/zlib.mk
include $(ML_ROOT)/make/packages/async.mk
include $(ML_ROOT)/make/packages/mbedtls.mk
include $(ML_ROOT)/make/packages/monolinux-c-library.mk
include $(ML_ROOT)/make/packages/bitstream.mk
include $(ML_ROOT)/make/packages/humanfriendly.mk
include $(ML_ROOT)/make/app.mk
