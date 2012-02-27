#include "service/jingleinfotask.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/xmppclient.h"


namespace buzz {

class JingleInfoTask::JingleInfoGetTask : public XmppTask {
public:
  JingleInfoGetTask(Task * parent) : XmppTask(parent, XmppEngine::HL_SINGLE),
    done_(false) {}

  virtual int ProcessStart() {
    talk_base::scoped_ptr<XmlElement> get(MakeIq(STR_GET, JID_EMPTY, task_id()));
    get->AddElement(new XmlElement(QN_JINGLE_INFO_QUERY, true));
    if (SendStanza(get.get()) != XMPP_RETURN_OK) {
      return STATE_ERROR;
    }
    return STATE_RESPONSE;
  }
  virtual int ProcessResponse() {
    if (done_)
      return STATE_DONE;
    return STATE_BLOCKED;
  }

protected:
  virtual bool HandleStanza(const XmlElement * stanza) {
    if (!MatchResponseIq(stanza, JID_EMPTY, task_id()))
      return false;

    if (stanza->Attr(QN_TYPE) != STR_RESULT)
      return false;

    // Queue the stanza with the parent so these don't get handled out of order
    JingleInfoTask* parent = static_cast<JingleInfoTask*>(GetParent());
    parent->QueueStanza(stanza);

    // Wake ourselves so we can go into the done state
    done_ = true;
    Wake();
    return true;
  }

  bool done_;
};


void JingleInfoTask::RefreshJingleInfoNow() {
  JingleInfoGetTask* get_task = new JingleInfoGetTask(this);
  get_task->Start();
}

bool
JingleInfoTask::HandleStanza(const XmlElement * stanza) {
  if (!MatchRequestIq(stanza, "set", QN_JINGLE_INFO_QUERY))
    return false;

  // only respect relay push from the server
  Jid from(stanza->Attr(QN_FROM));
  if (from != JID_EMPTY &&
      !from.BareEquals(GetClient()->jid()) &&
      from != Jid(GetClient()->jid().domain()))
    return false;

  QueueStanza(stanza);
  return true;
}

int
JingleInfoTask::ProcessStart() {
  const XmlElement * stanza = NextStanza();
  if (stanza == NULL)
    return STATE_BLOCKED;
  const XmlElement * query = stanza->FirstNamed(QN_JINGLE_INFO_QUERY);
  if (query == NULL)
    return STATE_START;
  const XmlElement *stun = query->FirstNamed(QN_JINGLE_INFO_STUN);
  if (stun) {
    for (const XmlElement *server = stun->FirstNamed(QN_JINGLE_INFO_SERVER);
         server != NULL; server = server->NextNamed(QN_JINGLE_INFO_SERVER)) {
      std::string host = server->Attr(QN_JINGLE_INFO_HOST);
      std::string port = server->Attr(QN_JINGLE_INFO_UDP);
      if (host != STR_EMPTY && host != STR_EMPTY)
        //stun_hosts.push_back(talk_base::SocketAddress(host, atoi(port.c_str())));
        stun_hosts = talk_base::SocketAddress(host, atoi(port.c_str()));
        break;
    }
  }
 
  const XmlElement *relay = query->FirstNamed(QN_JINGLE_INFO_RELAY);
  if (relay) {
    relay_token = relay->TextNamed(QN_JINGLE_INFO_TOKEN);
    for (const XmlElement *server = relay->FirstNamed(QN_JINGLE_INFO_SERVER);
         server != NULL; server = server->NextNamed(QN_JINGLE_INFO_SERVER)) {
      std::string host = server->Attr(QN_JINGLE_INFO_HOST);
      std::string udpport = server->Attr(QN_JINGLE_INFO_UDP);
      std::string tcpport = server->Attr(QN_JINGLE_INFO_TCP);
      std::string sslport = server->Attr(QN_JINGLE_INFO_TCPSSL);
      if (host != STR_EMPTY) {
        //relay_hosts.push_back(host);
        if (udpport != STR_EMPTY) {
          relay_hosts.push_back(talk_base::SocketAddress(host, atoi(udpport.c_str())));
        }
        if (tcpport != STR_EMPTY) {
          relay_hosts.push_back(talk_base::SocketAddress(host, atoi(tcpport.c_str())));
        }
        if (sslport != STR_EMPTY) {
          relay_hosts.push_back(talk_base::SocketAddress(host, atoi(sslport.c_str())));
        }
        break;
      }
    }
  }

  if (relay_hosts[0].IsUnresolved()) {
    resolver_ = new talk_base::AsyncResolver();
    resolver_->set_address(relay_hosts[0]);
    resolver_->SignalWorkDone.connect(this, &JingleInfoTask::OnResolveResult);
    resolver_->Start();
  }
  return STATE_START;
}

void JingleInfoTask::OnResolveResult(talk_base::SignalThread* thread) {
  if (thread != resolver_)
    return;
  int error = resolver_->error();
  if (error == 0) {
    for (size_t i = 0; i < relay_hosts.size(); i++) {
      relay_hosts[i].SetResolvedIP(resolver_->address().ip());
    }
    SignalJingleInfo(relay_token, stun_hosts, relay_hosts);
  }
  resolver_->Destroy(false);
  resolver_ = NULL;
}

}



