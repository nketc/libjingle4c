#include <stdint.h>
#include "talk/base/logging.h"
#include "talk/base/sigslot.h"
#include "talk/session/tunnel/tunnelsessionclient.h"
#include "talk/base/stream.h"
#include "talk/base/event.h"
#include "talk/p2p/base/constants.h"
#include "service/talkclient.h"
#include "cif/notify.h"
#include "cif/jinglep2p.h"
#include <cstdio>

enum {
  MSG_CREATE_P2P,//in worker thread
  MSG_DESTROY_P2P, //in worker thread
  MSG_CHECK_SESSION_DESTROY,//signaling thread
};
static const uint32_t p2p_session_magic = ('p' << 24) | ('2' << 16) | ('p' << 8) | 's';

enum P2PState {
  STATE_NOT_CONNECT,
  STATE_CONNECTED,
  STATE_CLOSING,
};

enum P2PMode {
  MODE_INITIATIVE,
  MODE_PASSIVE
};

class P2PEvent : public sigslot::has_slots<> {
public:
  P2PEvent(p2p_session_t *_p2p, void (*_event_cb)(p2p_session_t*, int, int) = NULL) :
         p2p(_p2p), event_cb(_event_cb) {}
  void OnEvent(talk_base::StreamInterface *st, int events, int error);
  void setListener(void (*_event_cb)(p2p_session_t*, int, int)) {
    event_cb = _event_cb;
  }
private:
  p2p_session_t *p2p;
  void (*event_cb)(p2p_session_t*, int, int);
};

struct p2p_session_t : public talk_base::MessageHandler {
  p2p_session_t(cricket::TunnelSessionClient *_client, const char *_peerjid, const char *_id) :
         session_client(_client), peerjid(_peerjid), id(_id) {
    magic = p2p_session_magic;
    p2p_event = new P2PEvent(this);
    st = NULL;
    session = NULL;
    state_event = NULL;
  };
  ~p2p_session_t() {
    magic = 0;
    if (st)
      st->SignalEvent.disconnect(p2p_event);
    delete p2p_event;
    if (mode == MODE_INITIATIVE) {
      ASSERT(state_event != NULL);
      state_event->Set();
      delete state_event;
    }
    if (st)
      st->Close();
  };

  uint32_t                      magic;
  talk_base::StreamInterface   *st;
  cricket::TunnelSessionClient *session_client;
  cricket::Session             *session;
  P2PEvent                     *p2p_event;
  std::string                  peerjid;
  std::string                  id;

  /* state_event only used in initiative mode */
  talk_base::Event             *state_event;

  P2PState                      state;
  P2PMode                       mode;
protected:
  void OnMessage(talk_base::Message* pmsg);
private:
};

void P2PEvent::OnEvent(talk_base::StreamInterface *st, int events, int error) {
  ASSERT(st == p2p->st);
  LOG(INFO) << "p2p session to " << p2p->peerjid << " id=" << p2p->id
            << " events: " << events;
  if ((events & P2P_EVENT_OPEN) && (p2p->mode == MODE_INITIATIVE)) {
    p2p->state = STATE_CONNECTED;
    p2p->state_event->Set();
  }
  if (events & P2P_EVENT_CLOSE) {
    if (p2p->state == STATE_CLOSING) {
      delete p2p;
      return;
    }
  }

  if (event_cb)
    event_cb(p2p, events, error);
}

XmppNotify* _get_xmpp_notify(void);
TalkClient* _get_xmpp_talkclient(void);

p2p_session_t* xmpp_p2p_create(const char *jid, const char *id) {
  if (!jid || !id || !jid[0] || !id[0])
    return NULL;
  buzz::Jid ojid(jid);
  if (!ojid.IsValid() || ojid.node() == "") {
    LOG(LERROR) << "For p2p, Invalid JID: " << jid;
    return NULL;
  }

  TalkClient *client = _get_xmpp_talkclient();
  cricket::TunnelSessionClient *session_client = client->GetSessionClient();

  p2p_session_t *p2p = new p2p_session_t(session_client, jid, id);
  p2p->mode  = MODE_INITIATIVE;
  p2p->state = STATE_NOT_CONNECT;
  p2p->state_event = new talk_base::Event(false, false);

  session_client->session_manager()->worker_thread()->Post(p2p, MSG_CREATE_P2P);
  return p2p;
}

p2p_session_t* xmpp_p2p_create_with_callback(const char *jid, const char *id,
                                             void (*event_cb)(p2p_session_t*, int, int)) {
  if (!jid || !jid[0] || !id || !id[0] || !event_cb)
    return NULL;

  p2p_session_t *p2p = xmpp_p2p_create(jid, id);
  if (p2p) {
    xmpp_p2p_event_listener(p2p, event_cb);
  }
  return p2p;
}

int xmpp_p2p_connect(p2p_session_t *p2p, int tmo) {
  if (!p2p) return P2P_SR_ERROR;
  if (p2p->magic != p2p_session_magic) return P2P_SR_ERROR;
  if (p2p->mode != MODE_INITIATIVE) return P2P_SR_ERROR;

  p2p->state_event->Wait(tmo);

  return p2p->state == STATE_CONNECTED ? P2P_SR_SUCCESS : P2P_SR_ERROR;
}

void xmpp_p2p_close(p2p_session_t *p2p) {
  if (!p2p)
    return;
  if (p2p->magic != p2p_session_magic)
    return;
  p2p->magic = 0;
  if (p2p->state == STATE_NOT_CONNECT) {
    p2p->session_client->session_manager()->signaling_thread()->Post(p2p, MSG_CHECK_SESSION_DESTROY);
  }
  else {
    p2p->session_client->session_manager()->worker_thread()->Post(p2p, MSG_DESTROY_P2P);
  }
}

void xmpp_p2p_listener(int (*p2p_ack)(const char*, const char*),
                       void (*p2p_session)(p2p_session_t*)) {
  if (!p2p_ack || !p2p_session)
    return;

  XmppNotify* notify = _get_xmpp_notify();
  notify->RegisterP2PListener(p2p_ack, p2p_session);
}

int xmpp_p2p_read(const p2p_session_t *p2p, void *buf, size_t buf_len,
                  size_t *read, int *error) {
  if (!p2p || !p2p->st || !buf || !read)
    return P2P_SR_ERROR;
  if (p2p->magic != p2p_session_magic)
    return P2P_SR_ERROR;

  talk_base::StreamResult ret;
  ret = p2p->st->Read(buf, buf_len, read, error);
  return ret;
}

int xmpp_p2p_write(const p2p_session_t *p2p, const void *buf, size_t buf_len,
                   size_t *written, int *error) {
  if (!p2p || !p2p->st || !buf || !written)
    return P2P_SR_ERROR;
  if (p2p->magic != p2p_session_magic)
    return P2P_SR_ERROR;

  talk_base::StreamResult ret;
  ret = p2p->st->Write(buf, buf_len, written, error);
  return ret;
}

void xmpp_p2p_event_listener(p2p_session_t *p2p,
                             void (*event_cb)(p2p_session_t*, int, int)) {
  if (!p2p || !event_cb)
    return;
  if (p2p->magic != p2p_session_magic)
    return;
  
  ASSERT(p2p->p2p_event != NULL);
  p2p->p2p_event->setListener(event_cb);
}

const char* xmpp_p2p_get_peer(p2p_session_t *p2p) {
  if (!p2p) return NULL;
  if (p2p->magic != p2p_session_magic)
    return NULL;

  return p2p->peerjid.c_str();
}

const char* xmpp_p2p_get_id(p2p_session_t *p2p) {
  if (!p2p) return NULL;
  if (p2p->magic != p2p_session_magic)
    return NULL;

  return p2p->id.c_str();
}

p2p_session_t* xmpp_p2p_accept(void *session_client, void *session,
                               const char *jid, const char *id) {
  cricket::TunnelSessionClient *client = static_cast<cricket::TunnelSessionClient*>(session_client);
  p2p_session_t* p2p = new p2p_session_t(client, jid, id);
  p2p->session = static_cast<cricket::Session*>(session);
  p2p->mode  = MODE_PASSIVE;
  p2p->state = STATE_CONNECTED;

  p2p->st = client->AcceptTunnel(p2p->session);
  p2p->st->SignalEvent.connect(p2p->p2p_event, &P2PEvent::OnEvent);
  return p2p;
}

void xmpp_p2p_decline(void *session_client, void *session) {
  cricket::TunnelSessionClient *session_client_ = 
              static_cast<cricket::TunnelSessionClient*>(session_client);
  cricket::Session             *session_ =
              static_cast<cricket::Session*>(session);

  session_client_->DeclineTunnel(session_);
}

void xmpp_p2p_post_event(p2p_session_t *p2p, int event) {
  if (!p2p || (event == 0))
    return;
  if (p2p->magic != p2p_session_magic)
    return;

  p2p->st->PostEvent(p2p->session_client->session_manager()->worker_thread(), event, 0);
}

void p2p_session_t::OnMessage(talk_base::Message* pmsg) {
  if (pmsg->message_id == MSG_CREATE_P2P) {
    buzz::Jid ojid(peerjid);
    st = session_client->CreateTunnel(ojid, std::string(id));
    st->SignalEvent.connect(p2p_event, &P2PEvent::OnEvent);
  }
  else if (pmsg->message_id == MSG_DESTROY_P2P) {
    delete this;
  }
  else if (pmsg->message_id == MSG_CHECK_SESSION_DESTROY) {
    cricket::TunnelSession* tunnelsession = session_client->FindSessionByStream(st);
    if (tunnelsession) {
      state = STATE_CLOSING;
      cricket::Session *tmp_session = tunnelsession->GetSession();
      tmp_session->TerminateWithReason(cricket::STR_TERMINATE_SUCCESS);
      session_client->session_manager()->DestroySession(tmp_session);
    }
    else {
      session_client->session_manager()->worker_thread()->Post(this, MSG_DESTROY_P2P);
    }
  }
}
