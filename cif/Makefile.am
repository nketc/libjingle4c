AM_CPPFLAGS = $(JINGLE_CPPDEFINES) $(JINGLE_CCFLAGS) $(JINGLE_CXXFLAGS)
AM_CPPFLAGS += -I$(top_srcdir)

noinst_LTLIBRARIES = libcif.la
libcif_la_SOURCES = \
				xmpp.cc \
				debuglog.cc \
				notify.cc \
				roster.cc \
				jinglep2p.cc \
				base64.cc \
				chat.cc

cifincludedir = $(includedir)/xmpp
cifinclude_HEADERS = \
    xmpp.h \
	roster.h \
	jinglep2p.h
