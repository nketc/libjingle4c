#include "talk/base/logging.h"
#include "talk/base/scoped_ptr.h"
#include "talk/xmpp/constants.h"
#include "service/message.h"

//all types are "chat", "error", "groupchat", "headline", "normal".
//now we use "chat".
static const std::string MESG_CHAT("chat");

bool
XmppMessage::SendMessage(const std::string &mesg, const std::string &jid) {
  talk_base::scoped_ptr<buzz::XmlElement> stanza(new buzz::XmlElement(buzz::QN_MESSAGE));
  stanza->AddAttr(buzz::QN_TO, jid);
  stanza->AddAttr(buzz::QN_ID, client_->NextId());
  stanza->AddAttr(buzz::QN_TYPE, MESG_CHAT);

  buzz::XmlElement *body = new buzz::XmlElement(buzz::QN_BODY);
  body->SetBodyText(mesg);
  stanza->AddElement(body);

  if (client_->SendStanza(stanza.get()) == buzz::XMPP_RETURN_OK)
    return true;
  return false;
}

bool
XmppMessage::HandleStanza(const buzz::XmlElement *stanza) {
  if (stanza->Name() != buzz::QN_MESSAGE)
    return false;
  if (stanza->Attr(buzz::QN_TYPE) == MESG_CHAT) {
    std::string from(stanza->Attr(buzz::QN_FROM));
	const buzz::XmlElement *body = stanza->FirstNamed(buzz::QN_BODY);
	const std::string text(body->BodyText());

	SignalMessageIncoming(text, from);
  }
  return true;
}
