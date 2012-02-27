#ifndef _ROSTER_GET_TASK_H_
#define _ROSTER_GET_TASK_H_

#include "talk/xmpp/xmppengine.h"
#include "talk/xmpp/xmpptask.h"
#include "talk/base/sigslot.h"

class TalkClient;

namespace buzz {

class RosterGetTask : public XmppTask {
 public:
  RosterGetTask(Task* parent, TalkClient *client) :
    XmppTask(parent, XmppEngine::HL_TYPE), client_(client) {}
  
  virtual int ProcessStart();
  void RefreshRosterInfoNow();

  //sigslot::signal1(RosterMap *) SignalRosterRefresh;
 protected:
  class RosterPullTask;
  friend class RosterPullTask;

  virtual bool HandleStanza(const XmlElement *stanza);
 private:
  TalkClient *client_;
};

}

#endif
