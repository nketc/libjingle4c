#include "talk/base/logging.h"
#include "service/talkclient.h"
#include "cif/notify.h"
#include "cif/roster.h"

TalkClient* _get_xmpp_talkclient(void);

/*Override*/
void XmppNotify::OnStateChange(buzz::XmppEngine::State state) {
  LOG(INFO) << "LoginState: " << state;
  state_ = static_cast<srv_stat_t>(state);
  if (state_ == XMPP_STATE_CLOSED) {
    error_ = xclient_->GetError(NULL);
    LOG(INFO) << "xmpp stream closed ... " << xmpp_strerror(error_);
    talk_base::Thread::Current()->Quit();
  }
  _xmpp_set_srv_stat(state_);

  for (size_t i=0; i < xmpp_srv_listeners.size(); i++) {
    xmpp_srv_listeners[i](state_);
  }
}

void XmppNotify::OnSocketClose(int err) {
  sockerr_ = err;
  error_   = XMPP_ERROR_SOCKET;

  _xmpp_set_srv_stat(XMPP_STATE_CLOSED);

  for (size_t i=0; i < xmpp_srv_listeners.size(); i++) {
    xmpp_srv_listeners[i](XMPP_STATE_CLOSED);
  }
  talk_base::Thread::Current()->Quit();
}

int XmppNotify::LastError(void) {
  int err = xclient_->GetError(NULL);
  if (err == XMPP_ERROR_NONE)
    return error_;
  return err;
}

void XmppNotify::RegisterStatusListener(void (*listener)(const srv_stat_t)) {
  size_t i;
  for (i=0; i < xmpp_srv_listeners.size(); i++) {
    if (xmpp_srv_listeners[i] == listener) return;
  }
  xmpp_srv_listeners.push_back(listener);
}

void XmppNotify::RegisterPresenceListener(void (*listener)(const roster_status_t*)) {
  size_t i;
  for (i=0; i < presence_listeners.size(); i++) {
    if (presence_listeners[i] == listener) return;
  }
  presence_listeners.push_back(listener);
}

void XmppNotify::RegisterP2PListener(int (*p2p_ack)(const char*, const char*),
                           void(*p2p_session)(p2p_session_t*)) {
  p2p_listener_t li;
  li.p2p_ack = p2p_ack;
  li.p2p_session = p2p_session;
  size_t i;
  for (i=0; i < p2p_listeners.size(); i++) {
    if (p2p_listeners[i].p2p_ack == li.p2p_ack) return;
  }
  p2p_listeners.push_back(li);
}

void XmppNotify::RegisterMessageListener(buzz::Jid *jid, void (*incoming)(const char*, const char*, int)) {
  message_listener_t ml;
  ml.jid = jid;
  ml.listener = incoming;
  size_t i;
  for (i=0; i < message_listeners.size(); i++) {
    if (message_listeners[i].listener == ml.listener) return;
  }
  message_listeners.push_back(ml);
}

void XmppNotify::RemoveMessageListener(void (*incoming)(const char*, const char*, int)) {
  std::vector<message_listener_t>::iterator iter = message_listeners.begin();
  for (; iter != message_listeners.end(); iter++) {
    if (iter->listener == incoming) break;
  }
  if (iter->listener == incoming) {
    delete iter->jid;
    message_listeners.erase(iter);
  }
}

void XmppNotify::OnPresenceUpdate(const buzz::Status& status) {
  roster_status_t s;

  s.jid    = strdup(status.jid().Str().c_str());
  if (!s.jid) {
    return;
  }
  s.prio   = status.priority();
  s.status =  strdup(status.status().c_str());
  s.show   = (roster_show_t) status.show();
  if (!status.available()) s.show = XMPP_SHOW_OFFLINE;

  for (size_t i=0; i < presence_listeners.size(); i++) {
    presence_listeners[i](&s);
  }
  free((void*)s.jid);
  free(s.status);
}

void XmppNotify::OnP2PIncoming(const char *jid, const char *id, cricket::Session *session) {
  TalkClient *client = _get_xmpp_talkclient();
  cricket::TunnelSessionClient *session_client = client->GetSessionClient();
  size_t i;
  int    r;
  p2p_session_t *p2p;
  for (i=0; i < p2p_listeners.size(); i++) {
    r = p2p_listeners[i].p2p_ack(jid, id);
    switch (r) {
    case XMPP_P2P_CONTINUE:
      continue;
    case XMPP_P2P_ACCEPT:
      p2p = xmpp_p2p_accept(session_client, session, jid, id);
      p2p_listeners[i].p2p_session(p2p);
      return;
    case XMPP_P2P_DECLINE:
      xmpp_p2p_decline(session_client, session);
      return;
    }
  }

  if (i == p2p_listeners.size()) {
    LOG(INFO) << "decline tunnel to: " << jid << " id: " << id;
    xmpp_p2p_decline(session_client, session);
  }
}

void XmppNotify::OnMessageIncoming(const std::string &msg, const std::string &fromJid) {
  buzz::Jid from(fromJid);
  size_t i;
  for (i=0; i < message_listeners.size(); i++) {
    message_listener_t ml = message_listeners[i];
    if ((ml.jid->Compare(buzz::JID_EMPTY) == 0)
        || (ml.jid->IsBare() && ml.jid->BareEquals(from))
        || (ml.jid->Compare(from) == 0)) {
      ml.listener(fromJid.c_str(), msg.c_str(), msg.length());
    }
  }
}
