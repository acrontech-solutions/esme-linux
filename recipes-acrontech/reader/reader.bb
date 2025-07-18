DESCRIPTION = "Example application on how to read images from the camera"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " file://reader.cpp \
			file://lepton_ioctl_header.h \
			file://lepton_rpmsg_datatype.h"

S = "${WORKDIR}"

do_compile() {
	${CXX} reader.cpp ${LDFLAGS} -o reader
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 reader ${D}${bindir}
}