DESCRIPTION = "Kernel module for interfacing with Flir Lepton infrared camera" 
LICENSE = "GPL-2.0" 
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6" 

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
