#ifndef __XMPP_NOTIFY_H_
#define __XMPP_NOTIFY_H_

#include <vector>
#include "talk/base/sigslot.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/session/tunnel/tunnelsessionclient.h"
#include "service/xmpppump.h"
#include "service/status.h"
#include "service/message.h"
#include "service/talkclient.h"
#include "cif/xmpp.h"
#include "cif/roster.h"
#include "cif/jinglep2p.h"

TalkClient* _get_xmpp_talkclient(void);

class XmppNotify : public XmppPumpNotify, public sigslot::has_slots<> {
public:
  XmppNotify():
    state_(XMPP_STATE_NONE),
    error_(XMPP_ERROR_NONE),
    sockerr_(XMPP_ERROR_NONE) {}

  void SetXmppClient(buzz::XmppClient *client) {
    xclient_ = client;
  }
  void Reset(void) {
    state_ = XMPP_STATE_NONE;
    error_ = XMPP_ERROR_NONE;
    sockerr_ = XMPP_ERROR_NONE;

    presence_listeners.clear();
    xmpp_srv_listeners.clear();
    p2p_listeners.clear();
    for (size_t i = 0; i < message_listeners.size(); i++) {
      delete message_listeners[i].jid;
    }
    message_listeners.clear();
  }

  int  LastError(void);
  void RegisterStatusListener(void (*listener)(srv_stat_t));
  void RegisterPresenceListener(void (*listener)(const roster_status_t*));
  void RegisterP2PListener(int (*p2p_ack)(const char*, const char*),
                           void(*p2p_session)(p2p_session_t*));
  void RegisterMessageListener(buzz::Jid *jid, void (*incoming)(const char *jid, const char *msg, int len));
  void RemoveMessageListener(void (*incoming)(const char *jid, const char *msg, int len));
  /*Override*/
  virtual void OnStateChange(buzz::XmppEngine::State state);

  void OnSocketClose(int err);
  void OnPresenceUpdate(const buzz::Status& status);
  void OnReady(void) {
    XmppMessage *mesg = _get_xmpp_talkclient()->GetXmppMessage();
    ASSERT(mesg != NULL);
    mesg->SignalMessageIncoming.connect(this, &XmppNotify::OnMessageIncoming);
    _xmpp_set_srv_stat(XMPP_STATE_READY);
  }
  void OnP2PIncoming(const char *jid, const char *id, cricket::Session *session);
  void OnMessageIncoming(const std::string&, const std::string&);
private:
  srv_stat_t  state_;
  int  error_;
  int  sockerr_;
  buzz::XmppClient *xclient_;
  
  typedef struct {
    int (*p2p_ack)(const char*, const char*);
    void(*p2p_session)(p2p_session_t*);
  }p2p_listener_t;

  typedef struct {
    buzz::Jid *jid;
    void (*listener)(const char *, const char*, int len);
  }message_listener_t;

  std::vector<void (*)(const roster_status_t*)> presence_listeners;
  std::vector<void (*)(const srv_stat_t)> xmpp_srv_listeners;
  std::vector<p2p_listener_t> p2p_listeners;
  std::vector<message_listener_t> message_listeners;
};

#endif
