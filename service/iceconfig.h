#ifndef _ICE_CONFIG_H_
#define _ICE_CONFIG_H_

#include <stdio.h>
#include <string.h>
#include "talk/base/socketaddress.h"

#define ICECONFIG_FILE  "ice.cfg"
//TODO:
class ICEConfig {
public:
  static ICEConfig* getInstance() {
    static ICEConfig *config;
    if (!config)
      config = new ICEConfig;
    return config;
  };
  bool use_presetting_config() {
    return true;
  };
  talk_base::SocketAddress& get_stun_addr() {
    return stunAddr;
  };
  talk_base::SocketAddress& get_relay_addr() {
    return relayAddr;
  };
protected:
  ICEConfig() {
    use_presetting = true;
    if (!fromFile()) {
      stunAddr.FromString("stun.l.google.com:19302");
      relayAddr.FromString("relay.google.com:19295");
    }
  };
private:
  bool fromFile() {
    char line[1024];
    FILE *fp = fopen(ICECONFIG_FILE, "rb");
    if (!fp)
      return false;
    char *p;
    while(!feof(fp)) {
      if (!fgets(line, sizeof(line), fp))
        break;
      if (line[0] == '#')
        continue;
      p = strchr(line, '=');
      if (p) {
        *p++ = '\0';
        if (strcmp(line, "use_presetting") == 0) {
          if (strcmp(p, "true") == 0)
            use_presetting = true;
          else
            use_presetting = false;
        }
        else if (strcmp(line, "stun_addr") == 0) {
          stunAddr.FromString(p);
        }
        else if (strcmp(line, "relay_addr") == 0) {
          relayAddr.FromString(p);
        }
      }
    }
    fclose(fp);
    return true;
  }
private:
  talk_base::SocketAddress  stunAddr;
  talk_base::SocketAddress  relayAddr;
  bool                      use_presetting;
};

#endif

