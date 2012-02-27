
AM_CPPFLAGS = $(JINGLE_CPPDEFINES) $(JINGLE_CCFLAGS) $(JINGLE_CXXFLAGS)
AM_CPPFLAGS += -I$(top_srcdir) -I$(prefix)/include -I$(top_builddir)

DEFAULT_INCLUDES =

noinst_LTLIBRARIES = libbase.la
libbase_la_SOURCES = \
	        asyncfile.cc 	\
            asynchttprequest.cc	\
            asyncsocket.cc \
            asynctcpsocket.cc \
            asyncudpsocket.cc \
            autodetectproxy.cc \
            base64.cc \
            basicpacketsocketfactory.cc \
            bytebuffer.cc \
            checks.cc \
            common.cc \
            diskcache.cc \
            event.cc \
            fileutils.cc \
            firewallsocketserver.cc \
            flags.cc \
            helpers.cc \
            host.cc \
            httpbase.cc \
            httpclient.cc \
            httpcommon.cc \
            httprequest.cc \
            logging.cc \
            md5.cc \
            messagehandler.cc \
            messagequeue.cc \
            nethelpers.cc \
            network.cc \
            openssladapter.cc \
            pathutils.cc \
            physicalsocketserver.cc \
            proxydetect.cc \
            proxyinfo.cc \
            ratetracker.cc \
            signalthread.cc \
            socketadapters.cc \
            socketaddress.cc \
            socketaddresspair.cc \
            socketpool.cc \
            socketstream.cc \
            ssladapter.cc 	\
            sslsocketfactory.cc \
            stream.cc \
            stringdigest.cc \
            stringencode.cc \
            stringutils.cc \
            task.cc \
            taskparent.cc \
            taskrunner.cc \
            thread.cc \
            time.cc \
            urlencode.cc \
            worker.cc
libbase_la_SOURCES += \
            unixfilesystem.cc \
			opensslidentity.cc \
			opensslstreamadapter.cc \
			sslidentity.cc \
			sslstreamadapter.cc
