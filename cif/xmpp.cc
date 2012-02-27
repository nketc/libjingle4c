#include <iomanip>
#include <errno.h>
#include <cstring>
#include <cstdio>

#include "talk/base/logging.h"
#include "talk/base/physicalsocketserver.h"
#include "talk/base/ssladapter.h"
#include "talk/xmpp/jid.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/base/thread.h"
#include "talk/base/event.h"
#include "talk/xmpp/xmppclientsettings.h"
#include "talk/xmpp/xmppclient.h"

#include "service/xmppthread.h"
#include "service/xmpppump.h"
#include "service/talkclient.h"

#include "cif/xmpp.h"
#include "cif/debuglog.h"
#include "cif/notify.h"
#include "cif/roster.h"

#ifndef ACCOUNT_FILE
#ifdef WIN32
#define ACCOUNT_FILE ".xmpp"
#else
#define ACCOUNT_FILE "/usr/.xmpp"
#endif
#endif

typedef struct {
  buzz::Jid   jid;
  std::string password;
}Account;

static DebugLog        debug_log_;
static TalkClient     *client = NULL;
static XmppNotify      notify;
static Account         account;
static srv_stat_t      client_stat = XMPP_STATE_NONE;
static talk_base::Thread *xmppth = NULL;
static talk_base::Event *client_stat_event = new talk_base::Event(false, false);


static void *xmpp_service(void *param) {
  talk_base::LogMessage::LogToDebug(talk_base::LS_WARNING);
  (void)param;//avoid compiler warning
  talk_base::InsecureCryptStringImpl pass;
  pass.password() = account.password;

  buzz::XmppClientSettings xcs;
  xcs.set_user(account.jid.node());
  xcs.set_resource("jingle.");
  xcs.set_host(account.jid.domain());
  xcs.set_use_tls(true);
  xcs.set_pass(talk_base::CryptString(pass));
  xcs.set_server(talk_base::SocketAddress("talk.google.com", 5222));

  talk_base::InitializeSSL();
  talk_base::Thread *main_thread = talk_base::ThreadManager::CurrentThread();

  notify.Reset();
  XmppPump pump(&notify);
  pump.client()->SignalLogInput.connect(&debug_log_, &DebugLog::Input);
  pump.client()->SignalLogOutput.connect(&debug_log_, &DebugLog::Output);
  notify.SetXmppClient(pump.client());

  client = new TalkClient(pump.client());
  client->SignalPresenceUpdate.connect(&notify, &XmppNotify::OnPresenceUpdate);
  client->SignalReady.connect(&notify, &XmppNotify::OnReady);
  client->SignalP2PIncoming.connect(&notify, &XmppNotify::OnP2PIncoming);

  LOG(WARNING) << account.jid.Str() << " logging in ...";

  XmppSocket *sock = new XmppSocket(true);
  sock->SignalCloseEvent.connect(&notify, &XmppNotify::OnSocketClose);
  pump.DoLogin(xcs, sock, NULL);
  main_thread->Run();
  pump.DoDisconnect();

  LOG(WARNING) << account.jid.Str() << " logout or exit.";

  delete client;
  client = NULL;
  client_stat = XMPP_STATE_NONE;
  return NULL;
}

class XmppService : public talk_base::Runnable {
  virtual void Run(talk_base::Thread *thread) {
    xmpp_service(NULL);
  }
};

int xmpp_service_start(const char *username, const char *password) {
  account.jid = buzz::Jid(username);
  if (!account.jid.IsValid() || account.jid.node() == "") {
    LOG(LERROR) << "Invalid JID: " << username;
    return -1;
  }
  if (!password || !password[0]) {
    LOG(LERROR) << "Password required for login.";
    return -1;
  }
  account.password = std::string(password);
  
  client_stat = XMPP_STATE_INIT;

  if (xmppth) delete xmppth;
  xmppth = new talk_base::Thread();
  xmppth->SetName("xmpp_thread", xmppth);
  if (xmppth) {
    xmppth->Start(new XmppService);
    return 0;
  }

  return -1;
}

void xmpp_service_try_start(void) {
  FILE *fp = fopen(ACCOUNT_FILE, "rb");
  if (fp) {
    char username[512] = {'\0', }, password[512] = {'\0', };
    int ret = fscanf(fp, "%s %s", username, password);
    ret = ret;//avoid compiler warning
    fclose(fp);
    if (username[0] && password[0]) {
      xmpp_service_start(username, password);
    }
    else {
      LOG(WARNING) << "read user and passwd from " << ACCOUNT_FILE << " failed";
    }
  }
  else {
    LOG(WARNING) << "open " << ACCOUNT_FILE << " failed, errno:" << errno
                 <<" ("<< strerror(errno) <<")";
  }
}

srv_stat_t xmpp_service_wait(int timeout) {
  if (client_stat != XMPP_STATE_READY)
    client_stat_event->Wait(timeout);
  return client_stat;
}

void xmpp_set_service_listener(void (*srv_stat_cb)(srv_stat_t)) {
  notify.RegisterStatusListener(srv_stat_cb);
}

int xmpp_lasterror(void) {
  return notify.LastError();
}

const char* xmpp_strerror(int err) {
  switch (err) {
    case  XMPP_ERROR_NONE:
      return "";
    case  XMPP_ERROR_XML:
      return "Malformed XML or encoding error";
    case  XMPP_ERROR_STREAM:
      return "XMPP stream error";
    case  XMPP_ERROR_VERSION:
      return "XMPP version error";
    case  XMPP_ERROR_UNAUTHORIZED:
      return "User is not authorized (Check your username and password)";
    case  XMPP_ERROR_TLS:
      return "TLS could not be negotiated";
    case  XMPP_ERROR_AUTH:
      return "Authentication could not be negotiated";
    case  XMPP_ERROR_BIND:
      return "Resource or session binding could not be negotiated";
    case  XMPP_ERROR_CONNECTION_CLOSED:
      return "Connection closed by output handler.";
    case  XMPP_ERROR_DOCUMENT_CLOSED:
      return "Closed by </stream:stream>";
    case  XMPP_ERROR_SOCKET:
      return "Socket error";
    default:
      return "Unknown error";
    }
}

srv_stat_t xmpp_get_service_state(void) {
  return client_stat;
}

void _xmpp_set_srv_stat(srv_stat_t stat) {
  client_stat = stat;
  client_stat_event->Set();
}

const char* xmpp_get_service_jid(void) {
  if (client) {
    return strdup(client->GetXmppClient()->jid().Str().c_str());
  }
  return NULL;
}

XmppNotify* _get_xmpp_notify(void) {
  return &notify;
}

TalkClient* _get_xmpp_talkclient(void) {
  return client;
}

