AC_INIT([libjingle], [0.5.8], [google-talk-open@googlegroups.com])
AC_CANONICAL_SYSTEM
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([dist-zip])
AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_CANONICAL_HOST

PKG_PROG_PKG_CONFIG

AC_DISABLE_STATIC

AC_PROG_RANLIB
##AC_CONFIG_MACRO_DIR([m4])


AC_CONFIG_FILES([
Makefile
service/Makefile
talk/Makefile
talk/xmllite/Makefile
talk/xmpp/Makefile
talk/p2p/Makefile
talk/session/Makefile
talk/base/Makefile
cif/Makefile
tests/Makefile
])

AC_OUTPUT


