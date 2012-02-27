#ifndef _PING_TASK_H_
#define _PING_TASK_H_

#include "talk/xmpp/xmppengine.h"
#include "talk/xmpp/xmpptask.h"
#include "talk/base/sigslot.h"
#include "talk/base/messagehandler.h"

class TalkClient;

namespace buzz {

class PingTask : public XmppTask, talk_base::MessageHandler {
 const static int timeout = 5;//ping timeout seconds
 public:
  PingTask(Task* parent, TalkClient *client) :
    XmppTask(parent, XmppEngine::HL_TYPE), client_(client), done_(false) {}
  
  virtual int ProcessStart();
  virtual int ProcessResponse();

  void OnMessage(talk_base::Message *pmsg);
 protected:
  virtual bool HandleStanza(const XmlElement *stanza);
 private:
  XmlElement* MakePingStanza();

  TalkClient *client_;
  bool        done_;
};

}

#endif
