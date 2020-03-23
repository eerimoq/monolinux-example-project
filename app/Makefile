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
PACKAGES += curl
PACKAGES += heatshrink
PACKAGES += xz
PACKAGES += detools
PACKAGES += zlib
PACKAGES += async
PACKAGES += mbedtls
PACKAGES += monolinux-c-library
PACKAGES += bitstream
PACKAGES += humanfriendly
PACKAGES += dbg-macro

.PHONY: default

default:
	$(MAKE) all

build: build/mldisk.img

build/mldisk.img: mldisk.img
	mkdir -p build
	cp $< $@

mldisk:
	create_file_storage.sh $@ detools

include $(ML_ROOT)/make/app.mk
