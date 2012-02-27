#ifndef __JINGLE_P2P_H_
#define __JINGLE_P2P_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct p2p_session_t p2p_session_t;

enum {
  XMPP_P2P_CONTINUE = 0,
  XMPP_P2P_ACCEPT   = 1,
  XMPP_P2P_DECLINE  = 2,
};

enum P2PSessionEvent {
  P2P_EVENT_OPEN   = 1,
  P2P_EVENT_READ   = 2,
  P2P_EVENT_WRITE  = 4,
  P2P_EVENT_CLOSE  = 8
};

enum P2PStreamResult {
  P2P_SR_ERROR,
  P2P_SR_SUCCESS,
  P2P_SR_BLOCK,
  P2P_SR_EOS,
};

p2p_session_t* xmpp_p2p_create(const char *jid, const char *id);
p2p_session_t* xmpp_p2p_create_with_callback(const char *jid, const char *id,
                                             void (*event_cb)(p2p_session_t*, int, int));
void           xmpp_p2p_close(p2p_session_t *p2p);
void           xmpp_p2p_listener(int (*p2p_ack)(const char*, const char*),
                                 void (*p2p_session)(p2p_session_t*));
int            xmpp_p2p_read(const p2p_session_t *p2p, void *buf, size_t buf_len,
                             size_t *read, int *error);
int            xmpp_p2p_write(const p2p_session_t *p2p, const void *buf, size_t buf_len,
                             size_t *written, int *error);
int            xmpp_p2p_connect(p2p_session_t *p2p, int tmo);

void           xmpp_p2p_event_listener(p2p_session_t *p2p,
                                       void (*event_cb)(p2p_session_t*, int, int));

const char*    xmpp_p2p_get_peer(p2p_session_t *p2p);
const char*    xmpp_p2p_get_id(p2p_session_t *p2p);

void           xmpp_p2p_post_event(p2p_session_t *p2p, int event);

/* private use */
p2p_session_t* xmpp_p2p_accept(void *session_client, void *session,
                               const char *jid, const char* id);
void           xmpp_p2p_decline(void *session_client, void *session);

#ifdef __cplusplus
}
#endif

#endif
