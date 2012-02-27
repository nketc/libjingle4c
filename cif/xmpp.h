#ifndef _TX_XMPP_H_
#define _TX_XMPP_H_

#ifdef __cplusplus
extern "C" {
#endif

enum srv_stat_t {
  XMPP_STATE_NONE  = 0,/*the same as XmppEngine::State::STATE_NONE*/
  XMPP_STATE_START, /*the same as XmppEngine::State::STATE_START*/
  XMPP_STATE_OPENING, /*the same as XmppEngine::State::STATE_OPENING*/
  XMPP_STATE_OPEN, /*the same as XmppEngine::State::STATE_OPEN*/
  XMPP_STATE_CLOSED, /*the same as XmppEngine::State::STATE_CLOSED*/
  XMPP_STATE_READY, /*???*/
  XMPP_STATE_INIT,
};
typedef enum srv_stat_t srv_stat_t;

enum {
  XMPP_ERROR_NONE = 0,         //!< No error
  XMPP_ERROR_XML,              //!< Malformed XML or encoding error
  XMPP_ERROR_STREAM,           //!< XMPP stream error - see GetStreamError()
  XMPP_ERROR_VERSION,          //!< XMPP version error
  XMPP_ERROR_UNAUTHORIZED,     //!< User is not authorized (rejected credentials)
  XMPP_ERROR_TLS,              //!< TLS could not be negotiated
  XMPP_ERROR_AUTH,             //!< Authentication could not be negotiated
  XMPP_ERROR_BIND,             //!< Resource or session binding could not be negotiated
  XMPP_ERROR_CONNECTION_CLOSED,//!< Connection closed by output handler.
  XMPP_ERROR_DOCUMENT_CLOSED,  //!< Closed by </stream:stream>
  XMPP_ERROR_SOCKET,           //!< Socket error
  XMPP_ERROR_NETWORK_TIMEOUT,  //!< Some sort of timeout (eg., we never got the roster)
  XMPP_ERROR_MISSING_USERNAME  //!< User has a Google Account but no nickname};
};

srv_stat_t           xmpp_service_wait(int timeout/*seconds*/);
void                 xmpp_set_service_listener(void (*srv_stat_cb)(srv_stat_t stat));

const char* xmpp_strerror(int err);
int xmpp_lasterror(void);
srv_stat_t xmpp_get_service_state(void);

int xmpp_service_start(const char* username, const char *password);
void xmpp_service_try_start(void);
const char* xmpp_get_service_jid(void);

/* private use */
void _xmpp_set_srv_stat(srv_stat_t stat);

#ifdef __cplusplus
}
#endif

#endif
