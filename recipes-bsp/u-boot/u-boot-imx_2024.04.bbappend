FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-custom-u-boot-config.patch \
            file://0002-autoload-m33-binary.patch \
            "
