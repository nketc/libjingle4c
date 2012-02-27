#ifndef _XMPP_MESSAGE_H
#define _XMPP_MESSAGE_H
#include <string>
#include "talk/base/sigslot.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/xmpp/xmppclient.h"
#include "talk/xmllite/xmlelement.h"

class XmppMessage :
  public buzz::XmppStanzaHandler,
  public sigslot::has_slots<>
{
  public:
    XmppMessage(buzz::XmppClient *client) : client_(client) {
      client_->engine()->AddStanzaHandler(this, buzz::XmppEngine::HL_TYPE);
	};
	~XmppMessage() {};
    bool SendMessage(const std::string &mesg, const std::string &jid);
	sigslot::signal2<const std::string&, const std::string&> SignalMessageIncoming;
  protected:
    virtual bool HandleStanza(const buzz::XmlElement *stanza);
  private:
    buzz::XmppClient *client_;
};
#endif