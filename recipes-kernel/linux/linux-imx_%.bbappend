FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
	file://0001-test-change.patch \
	file://0002-added-new-device-tree.patch \
	file://0003-fixed-error.patch "


KERNEL_DEVICETREE +=  "freescale/imx93-11x11-evk-lepton.dtb"

