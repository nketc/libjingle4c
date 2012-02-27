#ifndef _XMPP_ROSTER_H_
#define _XMPP_ROSTER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 凡是jid所指的都是包含资源部分的xmpp帐号。
 */

enum roster_subcription_t {
  XMPP_ROSTER_BOTH, /* 双向订阅，即你是我的好友，我也是你的好友 */
  XMPP_ROSTER_NONE
};
typedef enum roster_subcription_t roster_sub_t;

enum roster_show_t {
  XMPP_SHOW_NONE = 0,
  XMPP_SHOW_OFFLINE = 1, /* 不在线，其他状态(除了XMPP_SHOW_NONE)都可以认为是在线 */
  XMPP_SHOW_XA = 2,
  XMPP_SHOW_AWAY = 3,
  XMPP_SHOW_DND = 4,
  XMPP_SHOW_ONLINE = 5,
  XMPP_SHOW_CHAT = 6
};
typedef enum roster_show_t roster_show_t;

typedef struct jid_resource_t jid_res_t;
struct jid_resource_t {
  const char    *resource;
  int            prio;
  roster_show_t  show;
  char          *status;
  jid_res_t     *next;
};

typedef struct roster_t roster_t;
struct roster_t {
  const char    *account; /*xmpp帐号，不包含resouce部分*/
  const char    *name; //*自定义的一个名字
  const char    *jid;  //*没有在线资源的时候是NULL
  roster_sub_t   sub;
  jid_res_t     *respart;
  roster_t      *next;
};

typedef struct roster_status_t roster_status_t;
struct roster_status_t {
  const char    *jid;
  int            prio;
  roster_show_t  show;
  char          *status;
};

roster_t* xmpp_get_rosters(void);
void      xmpp_free_rosters(roster_t *rosters);
void      xmpp_set_roster_listener(void (*presence_cb)(const roster_status_t *));

#ifdef __cplusplus
}
#endif

#endif
