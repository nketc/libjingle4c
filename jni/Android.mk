LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

TARGET_PLATFORM := android-8

LOCAL_C_INCLUDES += $(LOCAL_PATH)/.. \
                    sources/cxx-stl/stlport/stlport \
                    $(ANDROID_SRC_ROOT)/external/openssl/include \
                    $(ANDROID_SRC_ROOT)/external/expat/lib

LOCAL_CPPFLAGS := -DLOGGING=1 -DFEATURE_ENABLE_SSL -DHAVE_OPENSSL_SSL_H=1 -DEXPAT_RELATIVE_PATH \
                  -DHASHNAMESPACE=__gnu_cxx -DHASH_NAMESPACE=__gnu_cxx \
                  -DPOSIX -D_REENTRANT -DLINUX \
                  -DDISABLE_DYNAMIC_CAST \
                  -DANDROID \
                  -Wno-non-virtual-dtor -Wno-ctor-dtor-privacy -fno-rtti

LOCAL_CPP_EXTENSION := .cc
#base
LOCAL_SRC_FILES := \
            ../talk/base/asyncfile.cc \
            ../talk/base/asynchttprequest.cc \
            ../talk/base/asyncsocket.cc \
            ../talk/base/asynctcpsocket.cc \
            ../talk/base/asyncudpsocket.cc \
            ../talk/base/autodetectproxy.cc \
            ../talk/base/base64.cc \
            ../talk/base/basicpacketsocketfactory.cc \
            ../talk/base/bytebuffer.cc \
            ../talk/base/checks.cc \
            ../talk/base/common.cc \
            ../talk/base/diskcache.cc \
            ../talk/base/event.cc \
            ../talk/base/fileutils.cc \
            ../talk/base/firewallsocketserver.cc \
            ../talk/base/flags.cc \
            ../talk/base/helpers.cc \
            ../talk/base/host.cc \
            ../talk/base/httpbase.cc \
            ../talk/base/httpclient.cc \
            ../talk/base/httpcommon.cc \
            ../talk/base/httprequest.cc \
            ../talk/base/logging.cc \
            ../talk/base/md5.cc \
            ../talk/base/messagehandler.cc \
            ../talk/base/messagequeue.cc \
            ../talk/base/nethelpers.cc \
            ../talk/base/network.cc \
            ../talk/base/openssladapter.cc \
            ../talk/base/pathutils.cc \
            ../talk/base/physicalsocketserver.cc \
            ../talk/base/proxydetect.cc \
            ../talk/base/proxyinfo.cc \
            ../talk/base/ratetracker.cc \
            ../talk/base/signalthread.cc \
            ../talk/base/socketadapters.cc \
            ../talk/base/socketaddress.cc \
            ../talk/base/socketaddresspair.cc \
            ../talk/base/socketpool.cc \
            ../talk/base/socketstream.cc \
            ../talk/base/ssladapter.cc \
            ../talk/base/sslsocketfactory.cc \
            ../talk/base/stream.cc \
            ../talk/base/stringdigest.cc \
            ../talk/base/stringencode.cc \
            ../talk/base/stringutils.cc \
            ../talk/base/task.cc \
            ../talk/base/taskparent.cc \
            ../talk/base/taskrunner.cc \
            ../talk/base/thread.cc \
            ../talk/base/time.cc \
            ../talk/base/urlencode.cc \
            ../talk/base/worker.cc \
            ../talk/base/unixfilesystem.cc \
            ../talk/base/opensslidentity.cc \
            ../talk/base/opensslstreamadapter.cc \
            ../talk/base/sslidentity.cc \
            ../talk/base/sslstreamadapter.cc

LOCAL_SRC_FILES += \
            ../talk/xmllite/xmlprinter.cc \
            ../talk/xmllite/xmlparser.cc \
            ../talk/xmllite/xmlnsstack.cc \
            ../talk/xmllite/xmlelement.cc \
            ../talk/xmllite/xmlconstants.cc \
            ../talk/xmllite/xmlbuilder.cc \
            ../talk/xmllite/qname.cc

LOCAL_SRC_FILES += \
            ../talk/xmpp/xmpptask.cc \
            ../talk/xmpp/xmppstanzaparser.cc \
            ../talk/xmpp/xmpplogintask.cc \
            ../talk/xmpp/xmppengineimpl_iq.cc \
            ../talk/xmpp/xmppengineimpl.cc \
            ../talk/xmpp/xmppclient.cc \
            ../talk/xmpp/saslmechanism.cc \
            ../talk/xmpp/ratelimitmanager.cc \
            ../talk/xmpp/mucroomlookuptask.cc \
            ../talk/xmpp/jid.cc \
            ../talk/xmpp/iqtask.cc \
            ../talk/xmpp/constants.cc

LOCAL_SRC_FILES += \
            ../talk/session/tunnel/tunnelsessionclient.cc \
            ../talk/session/tunnel/securetunnelsessionclient.cc \
            ../talk/session/tunnel/pseudotcpchannel.cc

LOCAL_SRC_FILES += \
            ../talk/p2p/base/constants.cc \
            ../talk/p2p/base/p2ptransport.cc \
            ../talk/p2p/base/p2ptransportchannel.cc \
            ../talk/p2p/base/parsing.cc \
            ../talk/p2p/base/port.cc \
            ../talk/p2p/base/pseudotcp.cc \
            ../talk/p2p/base/relayport.cc \
            ../talk/p2p/base/relayserver.cc \
            ../talk/p2p/base/rawtransport.cc \
            ../talk/p2p/base/rawtransportchannel.cc \
            ../talk/p2p/base/session.cc \
            ../talk/p2p/base/sessiondescription.cc \
            ../talk/p2p/base/sessionmanager.cc \
            ../talk/p2p/base/sessionmessages.cc \
            ../talk/p2p/base/stun.cc \
            ../talk/p2p/base/stunport.cc \
            ../talk/p2p/base/stunrequest.cc \
            ../talk/p2p/base/stunserver.cc \
            ../talk/p2p/base/tcpport.cc \
            ../talk/p2p/base/transport.cc \
            ../talk/p2p/base/transportchannel.cc \
            ../talk/p2p/base/transportchannelproxy.cc \
            ../talk/p2p/base/udpport.cc \
            ../talk/p2p/client/basicportallocator.cc \
            ../talk/p2p/client/httpportallocator.cc \
            ../talk/p2p/client/socketmonitor.cc

LOCAL_SRC_FILES += \
            ../service/xmppauth.cc \
            ../service/xmpppump.cc \
            ../service/xmppsocket.cc \
            ../service/xmppthread.cc \
            ../service/talkclient.cc \
            ../service/presenceouttask.cc \
            ../service/presencepushtask.cc \
            ../service/jingleinfotask.cc \
            ../service/rostergettask.cc

LOCAL_SRC_FILES += \
            ../cif/xmpp.cc \
            ../cif/debuglog.cc \
            ../cif/notify.cc \
            ../cif/roster.cc \
            ../cif/jinglep2p.cc

LOCAL_LDLIBS := -lssl -lexpat -lcrypto -llog
LOCAL_MODULE := libjingle
include $(BUILD_SHARED_LIBRARY)
