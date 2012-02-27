#ifndef __CIF_CHAT_H_
#define __CIF_CHAT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ENCODING_NONE,
  ENCODING_BASE64,
  DECODING_BASE64,
} ContentEncoding;

/*
 * If sucess return 
 */
int xmpp_chat_send(const char *jid, const char *content, int len, ContentEncoding coding);
void xmpp_chat_register_incoming_handler(void (*incoming)(const char *jid, const char *msg, int len));
void xmpp_chat_register_incoming_handler_on_jid(const char *jid,
	                                     void (*incoming)(const char *jid, const char *msg, int len));
void xmpp_chat_remove_incoming_handler(void (*incoming)(const char *jid, const char *msg, int len));

#ifdef __cplusplus
}
#endif

#endif