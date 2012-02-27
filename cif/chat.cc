#include "talk/base/logging.h"
#include "talk/base/base64.h"
#include "talk/xmpp/jid.h"
#include "talk/base/criticalsection.h"
#include "service/message.h"
#include "service/talkclient.h"
#include "cif/notify.h"
#include "cif/chat.h"

XmppNotify* _get_xmpp_notify(void);
TalkClient* _get_xmpp_talkclient(void);

static talk_base::CriticalSection chat_crit;

int
xmpp_chat_send(const char *jid, const char *content, int len, ContentEncoding coding) {
  buzz::Jid jjid(jid);
  if (!jjid.IsValid() || jjid.node() == "") return -1;
  XmppMessage *mesg = _get_xmpp_talkclient()->GetXmppMessage();
  if (!mesg) return -1;

  bool sret;
  if (coding == ENCODING_BASE64) {
    std::string encoded;
    talk_base::Base64::EncodeFromArray(content, len, &encoded);
    sret = mesg->SendMessage(encoded, jjid.Str());
  } else {
    sret = mesg->SendMessage(std::string(content), jjid.Str());
  }
  return sret ? len : 0;
}

void
xmpp_chat_register_incoming_handler(void (*incoming)(const char *jid, const char *msg, int len)) {
  if (!incoming) return;

  talk_base::CritScope cs(&chat_crit);
  XmppNotify* notify = _get_xmpp_notify();
  notify->RegisterMessageListener(new buzz::Jid(), incoming);
}

void
xmpp_chat_register_incoming_handler_on_jid(const char *jid,
        void (*incoming)(const char *jid, const char *msg, int len)) {
  if (!incoming) return;

  talk_base::CritScope cs(&chat_crit);
  XmppNotify* notify = _get_xmpp_notify();
  notify->RegisterMessageListener(new buzz::Jid(jid), incoming);
}

void
xmpp_chat_remove_incoming_handler(void (*incoming)(const char *jid, const char *msg, int len)) {
  talk_base::CritScope cs(&chat_crit);
  XmppNotify* notify = _get_xmpp_notify();
  notify->RemoveMessageListener(incoming);
}
