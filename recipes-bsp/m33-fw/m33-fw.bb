SUMMARY = "Deploy m33 firmware to boot partition"
LICENSE = "CLOSED"

SRC_URI = "file://m33_fw.bin"

S = "${WORKDIR}"

do_deploy() {
    install -d ${DEPLOY_DIR_IMAGE}
    install -m 0644 ${S}/m33_fw.bin ${DEPLOY_DIR_IMAGE}/m33_fw.bin
}

do_install() {
    # no files installed to rootfs, intentionally empty
    :
}

addtask deploy after do_fetch before do_build
