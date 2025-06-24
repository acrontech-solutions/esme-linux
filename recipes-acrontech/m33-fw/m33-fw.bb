SUMMARY = "Copy m33 fw to the boot partition"
LICENSE = "CLOSED"

SRC_URI += "file://m33_fw.bin"

do_install() {
    install -d ${D}/boot/
    cp ${WORKDIR}/m33_fw.bin ${D}/boot
}
FILES_${PN} += "/boot"