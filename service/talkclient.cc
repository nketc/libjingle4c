#include "service/talkclient.h"

#include <string>
#include <iostream>

#include "talk/base/helpers.h"
#include "talk/base/logging.h"
#include "talk/base/network.h"
#include "talk/base/socketaddress.h"
#include "talk/base/stringencode.h"
#include "talk/base/stringutils.h"
#include "talk/base/thread.h"
#include "service/presencepushtask.h"
#include "service/presenceouttask.h"
#include "service/jingleinfotask.h"
#include "service/rostergettask.h"
#include "service/pingtask.h"
#include "service/muc.h"
#include "service/iceconfig.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/mucroomlookuptask.h"
#include "talk/p2p/base/sessionmanager.h"
#include "talk/p2p/client/basicportallocator.h"
#include "talk/p2p/client/sessionmanagertask.h"
#include "talk/session/tunnel/tunnelsessionclient.h"


TalkClient::TalkClient(buzz::XmppClient* xmpp_client)
    : xmpp_client_(xmpp_client),
      worker_thread_(NULL),
      roster_(new RosterInfoMap),
      session_client_(NULL),
      mesg_(NULL) {
  xmpp_client_->SignalStateChange.connect(this, &TalkClient::OnStateChange);
}

TalkClient::~TalkClient() {
  if (!sessions_.empty()) {
    session_manager_->SignalSessionDestroy.disconnect(this);
    std::list<cricket::Session*>::iterator iter;
    for (iter = sessions_.begin(); iter != sessions_.end(); iter++) {
      session_manager_->DestroySession(*iter);
    }
    sessions_.clear();
  }
  if (mesg_)
    delete mesg_;
  if (session_client_)
    delete session_client_;
  delete roster_;
  if (worker_thread_)
    delete worker_thread_;
}

void TalkClient::OnStateChange(buzz::XmppEngine::State state) {
  switch (state) {
  case buzz::XmppEngine::STATE_START:
    LOG(INFO) << "connecting..." << std::endl;
    break;
  case buzz::XmppEngine::STATE_OPENING:
    LOG(INFO) << "logging in..." << std::endl;
    break;
  case buzz::XmppEngine::STATE_OPEN:
    LOG(INFO) << "logged in..." << std::endl;
    InitClientStage1();
    InitPresence();
    break;
  case buzz::XmppEngine::STATE_CLOSED:
    buzz::XmppEngine::Error error = xmpp_client_->GetError(NULL);
    LOG(INFO) << "logged out.. " << error << std::endl;
    talk_base::Thread::Current()->Quit();
    break;
  }
}

void TalkClient::InitPresence() {
  buzz::RosterGetTask *rgt = new buzz::RosterGetTask(xmpp_client_, this);
  rgt->RefreshRosterInfoNow();
  rgt->Start();

  presence_push_ = new buzz::PresencePushTask(xmpp_client_, this);
  presence_push_->SignalStatusUpdate.connect(
    this, &TalkClient::OnStatusUpdate);
  presence_push_->Start();

  presence_out_ = new buzz::PresenceOutTask(xmpp_client_);
  RefreshStatus();
  presence_out_->Start();

  ICEConfig *config = ICEConfig::getInstance();
  if (config->use_presetting_config()) {
    port_allocator_ = new cricket::BasicPortAllocator(
        network_manager_, config->get_stun_addr(), config->get_relay_addr(),
        talk_base::SocketAddress(), talk_base::SocketAddress());
    InitClientStage2();
  }
  else {
    buzz::JingleInfoTask *jit = new buzz::JingleInfoTask(xmpp_client_);
    jit->RefreshJingleInfoNow();
    jit->SignalJingleInfo.connect(this, &TalkClient::OnJingleInfo);
    jit->Start();
  }
}

void TalkClient::AddRosterInfo(RosterInfo& roster) {
  if (roster.account == xmpp_client_->jid().BareJid().Str())
    return;

  RosterInfoMap::iterator iter = roster_->find(roster.account);
  if (iter == roster_->end()) {
    RosterInfo &r = (*roster_)[roster.account];
    r.account = roster.account;
    r.sub = roster.sub;
    r.name = roster.name;
    r.res = NULL;
  }
}

void TalkClient::OnStatusUpdate(const buzz::Status& status) {
  RosterItem item;
  ResMap *resmap;
  item.jid = status.jid();
  item.show = status.show();
  item.status = status.status();
  item.prio = status.priority();

  std::string reskey = item.jid.resource();
  std::string barekey = item.jid.BareJid().Str();
  
  if (barekey == xmpp_client_->jid().BareJid().Str())
    return;

  RosterInfoMap::iterator iter = roster_->find(barekey);
  if (iter == roster_->end()) {
    RosterInfo &info = (*roster_)[barekey];
    info.res = new ResMap;
    info.account = item.jid.BareJid().Str();
    resmap = info.res;
  } else {
    if (!iter->second.res)
      iter->second.res = new ResMap;
    resmap = iter->second.res;
  }

  if (status.available()) {
    LOG(INFO) << status.jid().Str() << " available" << std::endl;
    (*resmap)[reskey] = item;
  } else {
    LOG(INFO) << status.jid().Str() << " unavailable" << std::endl;
    ResMap::iterator it = resmap->find(reskey);
    if (it != resmap->end())
      resmap->erase(it);
  }
  SignalPresenceUpdate(status);
}

void TalkClient::RefreshStatus() {
  my_status_.set_jid(xmpp_client_->jid());
  my_status_.set_available(true);
  my_status_.set_show(buzz::Status::SHOW_ONLINE);
  my_status_.set_priority(0);
  my_status_.set_version("1.0.0");

  presence_out_->Send(my_status_);
}

void TalkClient::InitClientStage1() {
  std::string client_unique = xmpp_client_->jid().Str();
  talk_base::InitRandom(client_unique.c_str(), client_unique.size());

  worker_thread_ = new talk_base::Thread();
  worker_thread_->SetName("worker_thread", worker_thread_);
  worker_thread_->Start();

  network_manager_ = new talk_base::BasicNetworkManager();
}

void TalkClient::OnJingleInfo(const std::string& relay_token,
                    const talk_base::SocketAddress& stun_address,
                    const std::vector<talk_base::SocketAddress>& relay_addresses) {
  LOG(INFO) << "-------------JingleInfo--------------";
  LOG(INFO) << " relay_token ==> " << relay_token << std::endl;
  LOG(INFO) << " relay_address ==> ";
  for (size_t i=0; i<relay_addresses.size(); i++) {
    LOG(INFO) << "    " << relay_addresses[i].ToString() << ", ";
  }
  LOG(INFO) << std::endl;
  LOG(INFO) << " stun_address ==> " << stun_address.ToString() << std::endl;
  LOG(INFO) << "------------------------------------" << std::endl;

  if (relay_addresses.size() == 0) {
    port_allocator_ = new cricket::BasicPortAllocator(
                           network_manager_, stun_address, talk_base::SocketAddress(),
                           talk_base::SocketAddress(), talk_base::SocketAddress());
  } else if (relay_addresses.size() == 1) {
    port_allocator_ = new cricket::BasicPortAllocator(
                           network_manager_, stun_address, relay_addresses[0],
                           talk_base::SocketAddress(), talk_base::SocketAddress());
  } else if (relay_addresses.size() == 2) {
    port_allocator_ = new cricket::BasicPortAllocator(
                           network_manager_, stun_address, relay_addresses[0],
                           relay_addresses[1], talk_base::SocketAddress());
  } else if (relay_addresses.size() == 3) {
    port_allocator_ = new cricket::BasicPortAllocator(
                           network_manager_, stun_address, relay_addresses[0],
                           relay_addresses[1], relay_addresses[2]);
  }

  InitClientStage2();
}

void TalkClient::InitClientStage2() {
  session_manager_ = new cricket::SessionManager(port_allocator_, worker_thread_);
  session_manager_->SignalRequestSignaling.connect(this, &TalkClient::OnRequestSignaling);
  session_manager_->SignalSessionCreate.connect(this, &TalkClient::OnSessionCreate);
  session_manager_->SignalSessionDestroy.connect(this, &TalkClient::OnSessionDestroy);
  session_manager_->OnSignalingReady();

  session_manager_task_ = new cricket::SessionManagerTask(xmpp_client_, session_manager_);
  session_manager_task_->EnableOutgoingMessages();
  session_manager_task_->Start();

  session_client_ = new cricket::TunnelSessionClient(xmpp_client_->jid(), session_manager_);
  session_client_->SignalIncomingTunnel.connect(this, &TalkClient::OnIncomingTunnel);

  talk_base::Thread::Current()->PostDelayed(1500, this, MSG_READY);
}

void TalkClient::OnRequestSignaling() {
  session_manager_->OnSignalingReady();
}

void TalkClient::OnSessionCreate(cricket::Session* session, bool initiate) {
  session->set_allow_local_ips(true);
  session->set_current_protocol(cricket::PROTOCOL_HYBRID);

  sessions_.push_front(session);
}

void TalkClient::OnSessionDestroy(cricket::Session* session) {
  if (!sessions_.empty()) {
    std::list<cricket::Session*>::iterator iter = find(sessions_.begin(), sessions_.end(), session);
    if (iter != sessions_.end()) {
      sessions_.erase(iter);
    }
  }
}

void TalkClient::OnIncomingTunnel(cricket::TunnelSessionClient* session_client, buzz::Jid jid,
                        std::string description, cricket::Session* session) {
  ASSERT(session_client == session_client_);
  LOG(INFO) << "incoming tunnel from " << jid.Str() <<", description=" << description;

  SignalP2PIncoming(jid.Str().c_str(), description.c_str(), session);
}

void TalkClient::OnPingTimeout() {
  LOG(WARNING)<<"server response ping timeout, the connection to internet may be broken. try to relogin";
  talk_base::Thread::Current()->Quit();
}

void TalkClient::OnMessage(talk_base::Message *pmsg) {
  buzz::PingTask *ping;

  switch (pmsg->message_id) {
  case MSG_READY:
	mesg_ = new XmppMessage(xmpp_client_);
    SignalReady();
	/*fall to MSG_PING*/
  case MSG_PING:
	ping = new buzz::PingTask(xmpp_client_, this);//delete by TaskRunner
	ping->Start();
	ping->SignalTimeout.connect(this, &TalkClient::OnPingTimeout);
    break;
  default:
    break;
  }
}

