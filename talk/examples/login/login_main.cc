/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products 
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>
#include <iostream>
#include <iomanip>

#include "talk/base/logging.h"
#include "talk/base/physicalsocketserver.h"
#include "talk/base/ssladapter.h"
#include "talk/xmpp/jid.h"

#include "talk/base/thread.h"
#include "talk/xmpp/xmppclientsettings.h"
#include "talk/examples/login/xmppthread.h"
#include "talk/examples/login/talkclient.h"

class DebugLog : public sigslot::has_slots<> {
 public:
  DebugLog() :
    debug_input_buf_(NULL), debug_input_len_(0), debug_input_alloc_(0),
    debug_output_buf_(NULL), debug_output_len_(0), debug_output_alloc_(0),
    censor_password_(false)
      {}
  char * debug_input_buf_;
  int debug_input_len_;
  int debug_input_alloc_;
  char * debug_output_buf_;
  int debug_output_len_;
  int debug_output_alloc_;
  bool censor_password_;

  void Input(const char * data, int len) {
    if (debug_input_len_ + len > debug_input_alloc_) {
      char * old_buf = debug_input_buf_;
      debug_input_alloc_ = 4096;
      while (debug_input_alloc_ < debug_input_len_ + len) {
        debug_input_alloc_ *= 2;
      }
      debug_input_buf_ = new char[debug_input_alloc_];
      memcpy(debug_input_buf_, old_buf, debug_input_len_);
      delete[] old_buf;
    }
    memcpy(debug_input_buf_ + debug_input_len_, data, len);
    debug_input_len_ += len;
    DebugPrint(debug_input_buf_, &debug_input_len_, false);
  }

  void Output(const char * data, int len) {
    if (debug_output_len_ + len > debug_output_alloc_) {
      char * old_buf = debug_output_buf_;
      debug_output_alloc_ = 4096;
      while (debug_output_alloc_ < debug_output_len_ + len) {
        debug_output_alloc_ *= 2;
      }
      debug_output_buf_ = new char[debug_output_alloc_];
      memcpy(debug_output_buf_, old_buf, debug_output_len_);
      delete[] old_buf;
    }
    memcpy(debug_output_buf_ + debug_output_len_, data, len);
    debug_output_len_ += len;
    DebugPrint(debug_output_buf_, &debug_output_len_, true);
  }

  static bool IsAuthTag(const char * str, size_t len) {
    if (str[0] == '<' && str[1] == 'a' &&
                         str[2] == 'u' &&
                         str[3] == 't' &&
                         str[4] == 'h' &&
                         str[5] <= ' ') {
      std::string tag(str, len);

      if (tag.find("mechanism") != std::string::npos)
        return true;
    }
    return false;
  }

  void DebugPrint(char * buf, int * plen, bool output) {
    int len = *plen;
    if (len > 0) {
      time_t tim = time(NULL);
      struct tm * now = localtime(&tim);
      char *time_string = asctime(now);
      if (time_string) {
        size_t time_len = strlen(time_string);
        if (time_len > 0) {
          time_string[time_len-1] = 0;    // trim off terminating \n
        }
      }
      LOG(INFO) << (output ? "SEND >>>>>>>>>>>>>>>>" : "RECV <<<<<<<<<<<<<<<<")
                << " : " << time_string;

      bool indent;
      int start = 0, nest = 3;
      for (int i = 0; i < len; i += 1) {
        if (buf[i] == '>') {
          if ((i > 0) && (buf[i-1] == '/')) {
            indent = false;
          } else if ((start + 1 < len) && (buf[start + 1] == '/')) {
            indent = false;
            nest -= 2;
          } else {
            indent = true;
          }

          // Output a tag
          LOG(INFO) << std::setw(nest) << " "
                    << std::string(buf + start, i + 1 - start);

          if (indent)
            nest += 2;

          // Note if it's a PLAIN auth tag
          if (IsAuthTag(buf + start, i + 1 - start)) {
            censor_password_ = true;
          }

          // incr
          start = i + 1;
        }

        if (buf[i] == '<' && start < i) {
          if (censor_password_) {
            LOG(INFO) << std::setw(nest) << " " << "## TEXT REMOVED ##";
            censor_password_ = false;
          } else {
            LOG(INFO) << std::setw(nest) << " "
                      << std::string(buf + start, i - start);
          }
          start = i;
        }
      }
      len = len - start;
      memcpy(buf, buf + start, len);
      *plen = len;
    }
  }
};

static DebugLog debug_log_;


int main(int argc, char **argv) {
  talk_base::LogMessage::LogToDebug(talk_base::LS_VERBOSE);

  talk_base::InitializeSSL();

  talk_base::PhysicalSocketServer ss;
  //talk_base::AutoThread main_thread(&ss);
  talk_base::Thread main_thread(&ss);
  talk_base::ThreadManager::SetCurrent(&main_thread);

  XmppPump pump;
  buzz::Jid jid;
  buzz::XmppClientSettings xcs;
  talk_base::InsecureCryptStringImpl pass;
  std::string username;

  //std::cout << "JID: ";
  //std::cin >> username;
  jid = buzz::Jid(username);
  if (!jid.IsValid() || jid.node() == "") {
    printf("Invalid JID.\n");
	return 1;
  }
  //std::cout << "Password: ";
  //std::cin >> pass.password();
  //std::cout << std::endl;
  pass.password() = "password";

  xcs.set_user(jid.node());
  xcs.set_resource("pcp");
  xcs.set_host(jid.domain());
  xcs.set_use_tls(true);
  xcs.set_pass(talk_base::CryptString(pass));
  xcs.set_server(talk_base::SocketAddress("talk.google.com", 5222));

  pump.client()->SignalLogInput.connect(&debug_log_, &DebugLog::Input);
  pump.client()->SignalLogInput.connect(&debug_log_, &DebugLog::Output);

  TalkClient *talkclient = new TalkClient(pump.client());

  pump.DoLogin(xcs, new XmppSocket(true), NULL);

  main_thread.Run();
  pump.DoDisconnect();

  delete talkclient;

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "quit")
      break;
  }

  return 0;
}

#if 0
int main(int argc, char **argv) {
  std::cout << "Auth Cookie: ";
  std::string auth_cookie;
  std::getline(std::cin, auth_cookie);

  std::cout << "User Name: ";
  std::string username;
  std::getline(std::cin, username);

  // Start xmpp on a different thread
  XmppThread thread;
  thread.Start();

  buzz::XmppClientSettings xcs;
  xcs.set_user(username.c_str());
  xcs.set_host("gmail.com");
  xcs.set_use_tls(false);
  xcs.set_auth_cookie(auth_cookie.c_str());
  xcs.set_server(talk_base::SocketAddress("talk.google.com", 5222));
  thread.Login(xcs);

  // Use main thread for console input
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "quit")
      break;
  }
  return 0;
}
#endif

