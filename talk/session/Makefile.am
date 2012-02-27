AM_CPPFLAGS = $(JINGLE_CPPDEFINES) $(JINGLE_CCFLAGS) $(JINGLE_CXXFLAGS)
AM_CPPFLAGS += -I$(top_srcdir) -I$(top_srcdir)/talk/session/tunner
noinst_LTLIBRARIES = libsession.la
libsession_la_SOURCES = \
               tunnel/tunnelsessionclient.cc \
               tunnel/securetunnelsessionclient.cc \
               tunnel/pseudotcpchannel.cc
