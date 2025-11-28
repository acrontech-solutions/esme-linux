DESCRIPTION = "TCP bridge application which forwards communication between edgesoft application fifos and TCP socket for external communication"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " file://tcpbridge.cpp \
			file://tcpbridge.h \
			file://json.hpp"

S = "${WORKDIR}"

do_compile() {
	${CXX} tcpbridge.cpp ${LDFLAGS} -o tcpbridge
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 tcpbridge ${D}${bindir}
}