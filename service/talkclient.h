#ifndef TALK_APP_TALKCLIENT_H_
#define TALK_APP_TALKCLIENT_H_

#include <map>
#include <string>
#include <vector>
#include <list>

#include "talk/p2p/base/session.h"
#include "talk/xmpp/xmppclient.h"
#include "talk/base/messagehandler.h"
#include "service/status.h"
#include "service/message.h"

namespace buzz {
class PresencePushTask;
class PresenceOutTask;
class PingTask;
class Muc;
class Status;
class MucStatus;
class XmlElement;
}

namespace talk_base {
class Thread;
class NetworkManager;
}

namespace cricket {
class PortAllocator;
class SessionManagerTask;
class TunnelSessionClient;
}

struct RosterItem {
  buzz::Jid jid;
  buzz::Status::Show show;
  std::string status;
  int  prio;
};
typedef std::map<std::string, RosterItem> ResMap;

struct RosterInfo {
  std::string  account;
  std::string  sub;
  std::string  name;
  ResMap      *res;
  RosterInfo() : res(NULL) {}
  ~RosterInfo() {if (res) delete res;}
};
typedef std::map<std::string, RosterInfo> RosterInfoMap;

class TalkClient: public sigslot::has_slots<>,
                  public talk_base::MessageHandler {
public:
  static const uint32 MSG_READY  = 1;
  static const uint32 MSG_PING   = 2;

  explicit TalkClient(buzz::XmppClient* xmpp_client);
  ~TalkClient();
  virtual void OnMessage(talk_base::Message *pmsg);

  typedef std::map<buzz::Jid, buzz::Muc*> MucMap;

  const MucMap& mucs() const {
    return mucs_;
  }
  void AddRosterInfo(RosterInfo& roster);

  RosterInfoMap* GetRosterInfo(void) {
    return roster_;
  }
  cricket::TunnelSessionClient* GetSessionClient(void) {
    return session_client_;
  }
  buzz::XmppClient* GetXmppClient(void) {
    return xmpp_client_;
  }
  XmppMessage* GetXmppMessage(void) {
    return mesg_;
  }

  sigslot::signal1<const buzz::Status&> SignalPresenceUpdate;
  sigslot::signal0<> SignalReady;
  sigslot::signal3<const char*, const char*, cricket::Session*> SignalP2PIncoming;
private:
  void OnStateChange(buzz::XmppEngine::State state);

  void InitClientStage1();
  void InitClientStage2();
  void InitPresence();
  void RefreshStatus();
  void OnStatusUpdate(const buzz::Status& status);

  void OnJingleInfo(const std::string& relay_token,
                    const talk_base::SocketAddress& stun_address,
					const std::vector<talk_base::SocketAddress>& relay_addresses);

  void OnRequestSignaling();
  void OnSessionCreate(cricket::Session* session, bool initiate);
  void OnSessionDestroy(cricket::Session* session);
  void OnIncomingTunnel(cricket::TunnelSessionClient* session_client, buzz::Jid jid,
                        std::string description, cricket::Session* session);
  void OnPingTimeout();

  buzz::XmppClient*            xmpp_client_;
  talk_base::Thread*           worker_thread_;
  talk_base::NetworkManager*   network_manager_;
  cricket::PortAllocator*      port_allocator_;
  cricket::SessionManager*     session_manager_;
  cricket::SessionManagerTask* session_manager_task_;

  MucMap mucs_;

  buzz::Status my_status_;
  buzz::PresencePushTask* presence_push_;
  buzz::PresenceOutTask*  presence_out_;
  RosterInfoMap  *roster_;

  cricket::TunnelSessionClient* session_client_;
  std::list<cricket::Session*> sessions_;

  XmppMessage* mesg_;
};

#endif
