DESCRIPTION = "Edgesensor application FW"
LICENSE = "CLOSED"

inherit systemd
SYSTEMD_AUTO_ENABLE = "enable"
SYSTEMD_SERVICE:${PN} = "edgesoft.service"

RDEPENDS:${PN} += "bash"
RDEPENDS:${PN} += "libcrypto"
RDEPENDS:${PN} += "libssl"
RDEPENDS:${PN} += "libgpiod"
RDEPENDS:${PN} += "imagemagick"
RDEPENDS:${PN} += "zlib"
RDEPENDS:${PN} += "i2c-tools"


SRC_URI += "file://backup \
            file://current \
            file://factory \
            file://hardware_version.txt \
            file://edgesoft.service \
            file://edge_launcher.sh"
do_install() {
    install -d ${D}/${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/edgesoft.service ${D}/${systemd_unitdir}/system
    install -d ${D}${ROOT_HOME}/current
    install -d ${D}${ROOT_HOME}/backup
    install -d ${D}${ROOT_HOME}/factory
    cp -r ${WORKDIR}/current/ ${D}${ROOT_HOME}
    cp -r ${WORKDIR}/backup/ ${D}${ROOT_HOME}
    cp -r ${WORKDIR}/factory/ ${D}${ROOT_HOME}
    chmod 744 ${D}${ROOT_HOME}/current/edgesoft
    chmod 744 ${D}${ROOT_HOME}/backup/edgesoft
    chmod 744 ${D}${ROOT_HOME}/factory/edgesoft
    install -m 0755 ${WORKDIR}/edge_launcher.sh ${D}${ROOT_HOME}
    install -m 0644 ${WORKDIR}/hardware_version.txt ${D}${ROOT_HOME}
}
FILES:${PN} = "/root/ ${systemd_unitdir}/system/edgesoft.service"
