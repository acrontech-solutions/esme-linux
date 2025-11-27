FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://0001-new-device-tree-for-FLIR-lepton-interaction.patch"
SRC_URI += "file://mywifi.cfg"

KERNEL_DEVICETREE +=  "freescale/imx93-11x11-evk-lepton.dtb"
