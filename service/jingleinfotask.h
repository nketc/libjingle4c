#ifndef _JINGLEINFOTASK_H_
#define _JINGLEINFOTASK_H_

#include "talk/p2p/client/httpportallocator.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/xmpp/xmpptask.h"
#include "talk/base/sigslot.h"
#include "talk/base/socketaddress.h"
#include "talk/base/nethelpers.h"
#include "talk/base/physicalsocketserver.h"
#include "status.h"

namespace buzz {

class JingleInfoTask : public XmppTask {

 public:
  JingleInfoTask(Task * parent) : 
    XmppTask(parent, XmppEngine::HL_TYPE) {}
  
  virtual int ProcessStart();
  void RefreshJingleInfoNow();

  sigslot::signal3<const std::string &, 
                   const talk_base::SocketAddress &, 
                   const std::vector<talk_base::SocketAddress> &> SignalJingleInfo;

 protected:
  class JingleInfoGetTask;
  friend class JingleInfoGetTask;

  virtual bool HandleStanza(const XmlElement * stanza);
  void OnResolveResult(talk_base::SignalThread* thread);

  std::vector<talk_base::SocketAddress> relay_hosts;
  talk_base::SocketAddress              stun_hosts;
  std::string                           relay_token;

  talk_base::AsyncResolver* resolver_;
};

  
}

#endif

