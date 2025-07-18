DESCRIPTION = "Kernel module for interfacing with Flir Lepton infrared camera" 
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit module 

SRC_URI = "\
	file://Makefile \
	file://lepton_ioctl_header.h \
	file://lepton_rpmsg_datatype.h \
	file://rpmsg_esme.c "
S = "${WORKDIR}" 

EXTRA_OEMAKE = "'KERNELDIR=${STAGING_KERNEL_DIR}'" 

do_compile() { 
       oe_runmake 
} 

do_install() { 
      install -d ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra 
      install -m 0644 rpmsg_esme.ko ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra 
} 

FILES_${PN} += "${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra/rpmsg_esme.ko"

KERNEL_MODULE_AUTOLOAD += "rpmsg_esme"
