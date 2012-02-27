SUBDIRS = talk service cif


JINGLE_CPPDEFINES = -DLOGGING=1 -DFEATURE_ENABLE_SSL -DHAVE_OPENSSL_SSL_H=1 -DEXPAT_RELATIVE_PATH
JINGLE_CPPDEFINES += -DHASHNAMESPACE=__gnu_cxx -DHASH_NAMESPACE=__gnu_cxx
JINGLE_CPPDEFINES += -DPOSIX -D_REENTRANT -DLINUX
JINGLE_CPPDEFINES += -DDISABLE_DYNAMIC_CAST

JINGLE_CCFLAGS = -Wall -Werror -Wno-switch -fno-exceptions
JINGLE_CCFLAGS += -ffunction-sections -fdata-sections

JINGLE_CXXFLAGS = -Wno-non-virtual-dtor -Wno-ctor-dtor-privacy -fno-rtti -fPIC -DPIC
JINGLE_CXXFLAGS += -I$(prefix)/include

export JINGLE_CPPDEFINES JINGLE_CCFLAGS JINGLE_CXXFLAGS

lib_LTLIBRARIES = libjingle.la
libjingle_la_SOURCES =
libjingle_la_LDFLAGS = \
    -avoid-version \
	-L$(prefix)/lib \
	-export-symbols $(top_srcdir)/jingle.symbols
libjingle_la_LIBADD = \
    service/libservice.la \
	talk/xmllite/libxmllite.la \
    talk/xmpp/libxmpp.la \
   	talk/p2p/libp2p.la	\
	talk/base/libbase.la \
	talk/session/libsession.la \
	cif/libcif.la
libjingle_la_LIBADD += -lssl -lcrypto -lexpat -lpthread -ldl -lrt
