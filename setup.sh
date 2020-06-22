export ML_ROOT=$(readlink -f 3pp/monolinux)
export ML_SOURCES=$(readlink -f 3pp)
export ML_LINUX_CONFIG=$(readlink -f app/linux.config)
export PATH=$PATH:$ML_ROOT/bin
export PATH=$(readlink -f x86_64-linux-musl-cross/bin):$PATH
export PATH=/opt/x86_64-linux-musl-cross/bin:$PATH
export CROSS_COMPILE=x86_64-linux-musl-
export ML_AUTOTOOLS_HOST=x86_64-linux-musl
