#include "talk/xmpp/xmppclient.h"
#include "talk/base/logging.h"
#include "talk/base/thread.h"
#include "service/talkclient.h"
#include "service/pingtask.h"

namespace buzz {


int
PingTask::ProcessStart() {
  const XmlElement *stanza = MakePingStanza();
  if (stanza == NULL)
    return STATE_BLOCKED;

  if (SendStanza(stanza) != XMPP_RETURN_OK) {
    delete stanza;
    return STATE_ERROR;
  }
  delete stanza;
  set_timeout_seconds(timeout);
  return STATE_RESPONSE;
}

int
PingTask::ProcessResponse() {
  if (done_)
    return STATE_DONE;
  talk_base::ThreadManager::CurrentThread()->PostDelayed(1000, this);
  return STATE_BLOCKED;
}

void
PingTask::OnMessage(talk_base::Message *pmsg) {
  Wake();
}

bool
PingTask::HandleStanza(const XmlElement *stanza) {
  if (!MatchResponseIq(stanza, JID_EMPTY, task_id()))
    return false;
  done_ = true;
  Wake();
  talk_base::ThreadManager::CurrentThread()->PostDelayed(timeout*1000, client_, TalkClient::MSG_PING);
  return true;
}

XmlElement*
PingTask::MakePingStanza() {
  XmlElement *result = MakeIq(STR_GET, JID_EMPTY, task_id());
  result->AddElement(XmlElement::ForStr("<ping xmlns=\'urn:xmpp:ping\'/>"));
  return result;
}

}

