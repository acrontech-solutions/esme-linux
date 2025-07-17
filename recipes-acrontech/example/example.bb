DESCRIPTION = "Example application on how to interact with edgesoft"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://example.cpp"

S = "${WORKDIR}"

do_compile() {
	${CC} example.c ${LDFLAGS} -o example
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 example ${D}${bindir}
}