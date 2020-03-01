# Monolinux root folder.
export ML_ROOT=$(readlink -f 3pp/monolinux)

# Sources folder.
export ML_SOURCES=$(readlink -f 3pp)

# Linux kernel configuration.
export ML_LINUX_CONFIG=$(readlink -f configs/linux-x86_64.config)

export PATH=$PATH:$ML_ROOT/bin
export PATH=$(readlink -f x86_64-linux-musl-cross/bin):$PATH

# Cross compilation.

export CROSS_COMPILE=x86_64-linux-musl-
# autotools: The system where built programs and libraries will run.
export ML_AUTOTOOLS_HOST=x86_64-linux-musl

# For ARM.
# export ARCH=arm
# export CROSS_COMPILE=arm-linux-musleabi-
# export ML_AUTOTOOLS_HOST=arm-linux-musleabi
