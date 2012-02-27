#include "talk/xmpp/constants.h"
#include "talk/xmpp/xmppclient.h"
#include "talk/base/logging.h"
#include "service/rostergettask.h"
#include "service/talkclient.h"

namespace buzz {

class RosterGetTask::RosterPullTask : public XmppTask {
public:
  RosterPullTask(Task *parent) : XmppTask(parent, XmppEngine::HL_SINGLE), done_(false) {}
  virtual int ProcessStart() {
    talk_base::scoped_ptr<XmlElement> get(MakeIq(STR_GET, JID_EMPTY, task_id()));
    get->AddElement(new XmlElement(QN_ROSTER_QUERY, true));
    if (SendStanza(get.get()) != XMPP_RETURN_OK) {
      return STATE_ERROR;
    }
    return STATE_RESPONSE;
  }
  virtual int ProcessResponse() {
    if (done_)
      return STATE_DONE;
    return STATE_BLOCKED;
  }
protected:
  virtual bool HandleStanza(const XmlElement *stanza) {
    if (!MatchResponseIq(stanza, JID_EMPTY, task_id()))
      return false;
    if (stanza->Attr(QN_TYPE) != STR_RESULT)
      return false;
    
    RosterGetTask *parent = static_cast<RosterGetTask*>(GetParent());
    parent->QueueStanza(stanza);

    done_ = true;
    Wake();
    return true;
  }

  bool done_;
};

void RosterGetTask::RefreshRosterInfoNow() {
  RosterPullTask *pull_task = new RosterPullTask(this);
  pull_task->Start();
}

bool RosterGetTask::HandleStanza(const XmlElement *stanza) {
  if (!MatchRequestIq(stanza, STR_RESULT, QN_ROSTER_QUERY))
    return false;

  QueueStanza(stanza);
  return true;
}

int RosterGetTask::ProcessStart() {
  const XmlElement *stanza = NextStanza();
  if (stanza == NULL)
    return STATE_BLOCKED;
  const XmlElement *query = stanza->FirstNamed(QN_ROSTER_QUERY);
  if (query == NULL)
    return STATE_START;

  const XmlElement *item = query->FirstNamed(QN_ROSTER_ITEM);
  for (; item != NULL; item = item->NextNamed(QN_ROSTER_ITEM)) {
    RosterInfo  info;
    info.account = item->Attr(QN_JID);
    info.sub     = item->Attr(QN_SUBSCRIPTION);
    info.name    = item->Attr(QN_NAME);
	info.res  = NULL;
    LOG(INFO) << "roster(" << info.account << ", "
              << info.sub << ", " << info.name <<")";
    client_->AddRosterInfo(info);
  }
  return STATE_START;
}

}
