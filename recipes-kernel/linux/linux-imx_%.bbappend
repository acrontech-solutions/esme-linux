FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
	file://0001-test-change.patch \
	file://0001-new-device-tree-for-lepton-interface.patch "

KERNEL_DEVICETREE +=  "imx93_11x11-evk-lepton.dtb"

