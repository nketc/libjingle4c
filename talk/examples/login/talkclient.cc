#include "talk/examples/login/talkclient.h"

#include <string>
#include <iostream>

#include "talk/base/helpers.h"
#include "talk/base/logging.h"
#include "talk/base/network.h"
#include "talk/base/socketaddress.h"
#include "talk/base/stringencode.h"
#include "talk/base/stringutils.h"
#include "talk/base/thread.h"

#include "talk/examples/login/presencepushtask.h"
#include "talk/examples/login/presenceouttask.h"

TalkClient::TalkClient(buzz::XmppClient* xmpp_client)
    : xmpp_client_(xmpp_client) {
  xmpp_client_->SignalStateChange.connect(this, &TalkClient::OnStateChange);
}

TalkClient::~TalkClient() {
}

void TalkClient::OnStateChange(buzz::XmppEngine::State state) {
  switch (state) {
  case buzz::XmppEngine::STATE_START:
    std::cout << "connecting..." << std::endl;
	break;
  case buzz::XmppEngine::STATE_OPENING:
    std::cout << "logging in..." << std::endl;
	break;
  case buzz::XmppEngine::STATE_OPEN:
    std::cout << "logged in..." << std::endl;
	InitPresence();
	break;
  case buzz::XmppEngine::STATE_CLOSED:
    buzz::XmppEngine::Error error = xmpp_client_->GetError(NULL);
    std::cout << "logged out.. " << error << std::endl;
	talk_base::Thread::Current()->Quit();
	break;
  }
}

void TalkClient::InitPresence() {
}

