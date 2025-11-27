SUMMARY = "Realtek RTL8821CU USB WiFi driver (morrownr 8821cu‑20210916)"
DESCRIPTION = "Out-of-tree Linux kernel module for RTL8821CU / RTL8811CU / compatible USB WiFi adapters"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://LICENSE;md5=ab842b299d0a92fb908d6eb122cd6de9"

SRC_URI = "git://github.com/morrownr/8821cu-20210916.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit module

# Extra make parameters to compile for Yocto kernel
EXTRA_OEMAKE = "KSRC=${STAGING_KERNEL_DIR} \
                ARCH=${ARCH} \
                CROSS_COMPILE=${TARGET_PREFIX}"

do_compile:append() {
    # Example: disable warnings-as-errors to avoid build break on some kernels
    export EXTRA_CFLAGS="-Wno-implicit-fallthrough -Wno-unused-but-set-variable -Wno-unused-variable"
    oe_runmake
}

do_install() {
    install -d ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra
    # find the built module and install it
    find ${B} -name '8821cu.ko' -exec install -m 0644 {} ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra/ \;
}

FILES_${PN} += "${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra/8821cu.ko"
FILES_${PN} += "${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra"

RPROVIDES_${PN} += "kernel-module-8821cu"

BB_NO_NETWORK = "0"
