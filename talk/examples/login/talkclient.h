#ifndef TALK_APP_TALKCLIENT_H_
#define TALK_APP_TALKCLIENT_H_

#include <map>
#include <string>
#include <vector>

#include "talk/p2p/base/session.h"
#include "talk/xmpp/xmppclient.h"
#include "talk/examples/login/status.h"

namespace buzz {
class PresencePushTask;
class PresenceOutTask;
class Status;
class XmlElement;
}

namespace talk_base {
class Thread;
class NetworkManager;
}

class TalkClient: public sigslot::has_slots<> {
public:
  explicit TalkClient(buzz::XmppClient* xmpp_client);
  ~TalkClient();
private:
  void OnStateChange(buzz::XmppEngine::State state);

  void InitPresence();

  buzz::XmppClient* xmpp_client_;
  buzz::XmppTask* presence_out_;
};

#endif
