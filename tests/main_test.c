#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef POSIX
#include <unistd.h>
#include <pthread.h>
#endif
#include "cif/xmpp.h"
#include "cif/roster.h"
#include "cif/jinglep2p.h"
#include "cif/chat.h"
#include "cif/base64.h"
#ifdef WIN32
#include <Windows.h>
#define __func__ __FUNCTION__
void sleep(int s) {
  Sleep(s*1000);
}
#endif

#ifdef POSIX
typedef pthread_t th_handle;
int thread_create(th_handle *handle, void* (*start)(void *param), void *param, int deatch) {
  pthread_attr_t *a = NULL;
  pthread_attr_t attr;
  if (deatch) {
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    a = &attr;
  }
  return pthread_create(handle, a, start, param);
}

void thread_join(th_handle handle) {
  void *ret;
  pthread_join(handle, &ret);
}
#endif
#ifdef WIN32
typedef HANDLE th_handle;
int thread_create(th_handle *handle, void* (*start)(void *param), void *param, int deatch) {
  *handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start, param, 0, NULL);
  return 0;
}

void thread_join(th_handle handle) {
  WaitForSingleObject(handle, 0xFFFFFFFF);
}
#endif

#define MAX_P2P_CONNS  10

typedef struct p2p_conns_list p2p_conns_list;
struct p2p_conns_list {
  p2p_session_t  *p2p;
  p2p_conns_list *next;
};

struct xmpp_resource {
  char                 *res;
  struct xmpp_resource *next;
};
struct xmpp_roster {
  char                 *account;
  struct xmpp_resource *resource;
  struct xmpp_roster   *next;
};

static struct xmpp_roster  *online_roster = NULL;

static p2p_conns_list *passive_p2p_head = NULL;
static int             passive_p2p_cnt  = 0;

static p2p_session_t *initiative_p2p = NULL;

static void           add_passive_p2p(p2p_session_t *p2p);
static int            get_passive_p2p_cnt(void);
static void           del_passive_p2p(p2p_session_t *p2p);
static void           del_passive_p2p_all(void);
static p2p_session_t* find_passive_p2p_by_peerjid(const char *jid);

static void           setup_online_rosters(roster_t *rs);
static void           update_online_rosters(const roster_status_t *s);
static void           clean_online_rosters(void);

static void           xmpp_worker_init(void);

static p2p_session_t* create_p2ptunnel_for_account(struct xmpp_roster *, const char*,
                                                   void (*)(p2p_session_t*, int, int));
static void           initiative_p2p_event(p2p_session_t *p2p, int events, int error);

static const char* strstat(srv_stat_t s) {
  switch(s) {
  case XMPP_STATE_NONE: return "none";
  case XMPP_STATE_START: return "start";
  case XMPP_STATE_OPENING: return "logging";
  case XMPP_STATE_OPEN: return "logged";
  case XMPP_STATE_CLOSED: return "closed";
  case XMPP_STATE_READY: return "ready";
  }
  return "unknown";
}

static void service_stat_listener(srv_stat_t s) {
  if (s == XMPP_STATE_CLOSED) {
    int err = xmpp_lasterror();
    printf("service exit: %d(%s)\n", err, xmpp_strerror(err));
    del_passive_p2p_all();
    clean_online_rosters();

    //service exit, restart waiting for service ready again.
    xmpp_worker_init();
  }
}

static void roster_status_listener(const roster_status_t *s) {
  printf("%s become %s\n", s->jid, s->show == XMPP_SHOW_OFFLINE ? "offline" : "online");
  
  update_online_rosters(s);

  if (s->show == XMPP_SHOW_OFFLINE) {
    p2p_session_t *p2p = find_passive_p2p_by_peerjid(s->jid);
    if (p2p) {
      printf("del p2p from %s\n", s->jid);
      xmpp_p2p_post_event(p2p, P2P_EVENT_CLOSE);
    }
    if (initiative_p2p) {
      const char *jid = xmpp_p2p_get_peer(initiative_p2p);
      if (jid && (strcmp(jid, s->jid) == 0)) {
        printf("del p2p to %s\n", jid);
        xmpp_p2p_post_event(initiative_p2p, P2P_EVENT_CLOSE);
      }
    }
  }
}

static void print_rosters(roster_t *rs) {
  while(rs != NULL) {
    jid_res_t *res;
    printf("roster: %s(name:%s, subcriptions:%s, %s)\n",
            rs->account, rs->name ? rs->name : "",
            rs->sub==XMPP_ROSTER_BOTH ? "both" : "none",
            rs->respart ? "online" : "offline");
    res = rs->respart;
    if (res) {
      printf("         online resource:\n");
      while(res) {
        printf("                 %s/%s priority=%d\n", rs->account, res->resource, res->prio);
        res = res->next;
      }
    }
    rs = rs->next;
  }
}

static int p2p_response(const char *jid, const char *id) {
  if (strcmp(id, "just_a_test") == 0) {
    if (get_passive_p2p_cnt() >= MAX_P2P_CONNS) {
      printf("decline p2p from %s id=%s\n", jid, id);
      return XMPP_P2P_DECLINE;
    }
    printf("accept p2p request from %s id=%s\n", jid, id);
    return XMPP_P2P_ACCEPT;
  }
  printf("ignore p2p request from %s id=%s\n", jid, id);
  return XMPP_P2P_DECLINE;
}

static void p2p_session(p2p_session_t *p2p) {
  add_passive_p2p(p2p);
}

static void passive_p2p_event(p2p_session_t *p2p, int events, int detail_error) {
  static int test_cnt = 0;

  if (events & P2P_EVENT_READ) {
    char    buf[1024];
    size_t  read, written, len;
    int     error = 0;
    int ret = xmpp_p2p_read(p2p, buf, sizeof buf, &read, &error);
    if (ret == P2P_SR_SUCCESS) {
      buf[read] = '\0';
      printf("[%s-->%s]:\n\t%s\n", xmpp_p2p_get_peer(p2p), xmpp_get_service_jid(), buf);
      len = sprintf(buf, "%08d hello, %s", test_cnt++, xmpp_p2p_get_peer(p2p));
      ret = xmpp_p2p_write(p2p, buf, len, &written, &error);
      if (ret != P2P_SR_SUCCESS) {
        printf("[%s-->%s]: (failed, ret=%d error=%d)\n",
               xmpp_get_service_jid(), xmpp_p2p_get_peer(p2p), ret, error);
      } else {
        printf("[%s-->%s]:\n\t%s\n", xmpp_get_service_jid(), xmpp_p2p_get_peer(p2p), buf);
      }
    }
    else if (ret == P2P_SR_ERROR) {
      printf("at %s read failed, ret=%d error=%d\n", __func__, ret, error);
    }
  }
  if (events & P2P_EVENT_CLOSE) {
    printf("%s close event\n", __func__);
    del_passive_p2p(p2p);
  }
}

static void add_passive_p2p(p2p_session_t *p2p) {
  p2p_conns_list *node = (p2p_conns_list*)malloc(sizeof(*node));
  node->p2p  = p2p;
  node->next = passive_p2p_head;
  passive_p2p_head = node;
  passive_p2p_cnt += 1;

  xmpp_p2p_event_listener(p2p, passive_p2p_event);
}

static void del_passive_p2p(p2p_session_t *p2p) {
  p2p_conns_list *node, *prev_node;
  for (node = passive_p2p_head; node; node = node->next) {
    if (node->p2p == p2p) {
      if (node == passive_p2p_head) {
        passive_p2p_head = node->next;
      }
      else {
        prev_node->next = node->next;
      }
      passive_p2p_cnt -= 1;
      break;
    }
    prev_node = node;
  }
  free(node);

  xmpp_p2p_close(p2p);
}

static int get_passive_p2p_cnt(void) {
  return passive_p2p_cnt;
}

static void del_passive_p2p_all(void) {
  p2p_conns_list *node = passive_p2p_head, *tmp;
  passive_p2p_head = NULL;
  while(node) {
    tmp = node->next;
    xmpp_p2p_close(node->p2p);
    free(node);
    passive_p2p_cnt -= 1;

    node = tmp;
  }
}

static p2p_session_t* find_passive_p2p_by_peerjid(const char *jid) {
  p2p_conns_list *walk;
  for (walk = passive_p2p_head; walk; walk = walk->next) {
    const char *p2pjid = xmpp_p2p_get_peer(walk->p2p);
    if (p2pjid && (strcmp(p2pjid, jid) == 0))
      return walk->p2p;
  }
  return NULL;
}

static void setup_online_rosters(roster_t *rs) {
  while(rs != NULL) {
    struct xmpp_roster *roster;
    jid_res_t *jidres;
    jidres = rs->respart;
    roster = NULL;
    while (jidres) {
      struct xmpp_resource *resource;
      if (roster == NULL) {
        roster = (struct xmpp_roster *)malloc(sizeof(*roster));
        roster->account = strdup(rs->account);
        roster->resource = NULL;
      }
      resource = (struct xmpp_resource *)malloc(sizeof(*resource));
      resource->res = strdup(jidres->resource);
      resource->next = roster->resource;
      roster->resource = resource;

      jidres = jidres->next;
    }
    if (roster) {
      roster->next = online_roster;
      online_roster = roster;
    }

    rs = rs->next;
  }
}

static void update_online_rosters(const roster_status_t *s) {
  char *account, *res;

  account = strdup(s->jid);
  res     = strdup(s->jid);

  char *c = strchr(account, '/');
  if (c) {
    *c = '\0';
    c += 1;
    char *tmp = res;
    while(*c) {
      *tmp++ = *c++;
    }
    *tmp = '\0';
  }

  struct xmpp_roster *walk, *prev_walk = NULL;
  for (walk = online_roster; walk; walk = walk->next) {
    if (strcmp(walk->account, account) == 0) {
      struct xmpp_resource *resource = walk->resource, *prev_resource = NULL;
      while(resource) {
        if (strcmp(resource->res, res) == 0) {
          if (s->show == XMPP_SHOW_OFFLINE) {
            if (prev_resource == NULL) walk->resource = resource->next;
            else prev_resource->next = resource->next;
            free(resource->res);
            free(resource);

            if (walk->resource == NULL) {
              if (prev_walk == NULL) online_roster = walk->next;
              else prev_walk->next = walk->next;
              free(walk->account);
              free(walk);
            }
          }
          goto __done;
        }
        prev_resource = resource;
        resource = resource->next;
      }
      if (resource == NULL) {
        if (s->show != XMPP_SHOW_OFFLINE) {
          struct xmpp_resource *resource = (struct xmpp_resource*)malloc(sizeof(*resource));
          resource->res = strdup(res);
          resource->next = walk->resource;
          walk->resource = resource;
        }
      }
      goto __done;
    }
    prev_walk = walk;
  }
  if (walk == NULL) {
    if (s->show != XMPP_SHOW_OFFLINE) {
      walk = (struct xmpp_roster*)malloc(sizeof(*walk));
      walk->account = strdup(account);
      walk->resource = NULL;

      struct xmpp_resource *resource = (struct xmpp_resource*)malloc(sizeof(*resource));
      resource->res = strdup(res);
      resource->next = walk->resource;
      walk->resource = resource;

      walk->next = online_roster;
      online_roster = walk;
    }
  }

__done:
  free(account);
  free(res);
}

static void clean_online_rosters(void) {
  struct xmpp_roster   *walk, *tw;
  struct xmpp_resource *resource, *tr;
  walk = online_roster;
  online_roster = NULL;
  for (;walk;) {
    for (resource = walk->resource; resource;) {
      tr = resource->next;
      free(resource->res);
      free(resource);
      resource = tr;
    }
    tw = walk->next;
    free(walk->account);
    free(walk);
    walk = tw;
  }
}

struct login_account {
  char *account;
  char *passwd;
};

static void* xmpp_moniter(void *param) {
  srv_stat_t state;
  int        count = 0;
  struct login_account *la = (struct login_account*)param;
  char account[128];
  char passwd[128];

  strncpy(account, la->account, 127);
  strncpy(passwd, la->passwd, 127);
  account[127] = '\0';
  passwd[127]  = '\0';
  free(la->account);
  free(la->passwd);
  free(la);

  xmpp_service_start(account, passwd);
  sleep(4);

  while(1) {
    state = xmpp_get_service_state();
    if (state == XMPP_STATE_NONE) {
      xmpp_service_start(account, passwd);
      count = 0;
    }
    else if (state == XMPP_STATE_READY) {
      if (count == 0) {
        const char *jid = xmpp_get_service_jid();
        printf("xmpp service login as: %s\n", jid);
        free((void*)jid);
      }
      count += 1;
    }
    sleep(4);
  }
  return NULL;
}

static void* xmpp_worker(void *param) {
  srv_stat_t  stat;
  roster_t   *rs;
  int         cnt = 0;

  printf("waiting for xmpp service ready ...\n");
  while((stat = xmpp_service_wait(1000)) != XMPP_STATE_READY) {
    cnt += 1;
    printf("xmpp service stat(cnt=%d): %s\n", cnt, strstat(stat));
  }
  printf("xmpp service ready!\n");
  xmpp_set_service_listener(service_stat_listener);

  rs = xmpp_get_rosters();
  print_rosters(rs);
  setup_online_rosters(rs);
  xmpp_free_rosters(rs);

  xmpp_set_roster_listener(roster_status_listener);

  xmpp_p2p_listener(p2p_response, p2p_session);

  return NULL;
}

static void xmpp_worker_init(void) {
  th_handle worker_thread;
  thread_create(&worker_thread, xmpp_worker, NULL, 1);
}

#ifdef WIN32
long exception_handle(void *exception) {
  exit(1);
}
#endif

static void incoming_message(const char *jid, const char *text, int len) {
  char decoded[1024];
  int  dlen = sizeof(decoded);
  if (base64_encoded_test(text)) {
    if (base64_decode_with_buffer(text, decoded, &dlen)) {
      decoded[dlen] = '\0';
      printf("%s:: %s\n", jid, decoded);
    }
    else 
      printf("base64 decode \"%s\" failed\n", text);
  }
  else
    printf("%s: %s\n", jid, text);
}

static void base64_test(void) {
  char *encoded = base64_encode("hello", strlen("hello"));
  char buf[1024];
  int  buflen = sizeof(buf);
  base64_decode_with_buffer(encoded, buf, &buflen);
  buf[buflen] = '\0';
  printf("buf=%s buflen=%d\n", buf, buflen);
  if (strcmp(buf, "hello") == 0) {
    printf("base64 text sucess\n");
  } else {
    printf("base64 text failed\n");
    getchar();
  }
  free(encoded);

  char toencode[] = { '\0', 1, 2, 'a', 'b', 'c' };
  encoded = base64_encode(toencode, sizeof(toencode));
  printf("encoded=%s\n", encoded);
  int decoded_len = 0;
  char *decoded = base64_decode(encoded, &decoded_len);
  printf("decoded_len=%d\n", decoded_len);
  if (memcmp(decoded, toencode, 6) != 0) {
    printf("base64 binary failed\n");
    getchar();
  } else {
    printf("base64 binary sucess\n");
  }
  free(encoded);
  free(decoded);
}

int main(int argc, char *argv[]) {
  th_handle  moniter_thread;
  int        ret;
#ifdef WIN32
  SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)exception_handle);
#endif
  struct login_account *la = (struct login_account*)malloc(sizeof(*la));
  la->account = strdup(argv[1]);
  la->passwd  = strdup(argv[2]);
  ret = thread_create(&moniter_thread, xmpp_moniter, la, 0);  

  xmpp_worker(NULL);
//base64_test();
  if (strcmp(argv[3], "not") == 0)
    goto __block;
  else {
__recreate:
    initiative_p2p = create_p2ptunnel_for_account(online_roster, argv[3],
                                                  initiative_p2p_event);
  }
  if (initiative_p2p) {
    int error;
    error = xmpp_p2p_connect(initiative_p2p, 5000);
    if (error != P2P_SR_SUCCESS) {
      xmpp_p2p_close(initiative_p2p);
      initiative_p2p = NULL;
    }
    else {
      while(1) {
        if (initiative_p2p) {
          xmpp_p2p_post_event(initiative_p2p, P2P_EVENT_WRITE);
        }
        else
          break;
        sleep(2);
      }
    }
  }
  if (initiative_p2p == NULL) {
    sleep(3);
    printf("trying to create a p2p tunnel to %s ...\n", argv[3]);
    goto __recreate;
  }

__block:
  if (strcmp(argv[4], "chat") == 0) {
    xmpp_chat_register_incoming_handler(incoming_message);
    while(1) {
      xmpp_chat_send(argv[5], "hello, haha!", 12, ENCODING_BASE64);
      sleep(2);
      xmpp_chat_send(argv[5], "hello, haha!", 12, ENCODING_NONE);
      sleep(2);
    }
  }
  thread_join(moniter_thread);

  return 0;
}

static p2p_session_t* create_p2ptunnel_for_account(struct xmpp_roster *rosters,
              const char* account, void (*event_cb)(p2p_session_t *, int, int)) {
  while(rosters) {
    if (rosters->resource) {
      if (strcmp(rosters->account, account) == 0) {
        struct xmpp_resource *resource = rosters->resource;
        while(resource) {
          printf("account: %s, resource: %s\n", rosters->account, resource->res);
          if (strncmp(resource->res, "jingle", 6) == 0)
            break;
          resource = resource->next;
        }
        if (resource) {
          char *jid = (char *)malloc(strlen(account) + strlen(resource->res) + 2);
          sprintf(jid, "%s/%s", account, resource->res);
          printf("make a p2ptunnel request to %s ...\n", jid);
          p2p_session_t *p2p = xmpp_p2p_create_with_callback(jid, "just_a_test", event_cb);
          free(jid);
          return p2p;
        }
      }
    }
    rosters = rosters->next;
  }
  return NULL; 
}

static void initiative_p2p_event(p2p_session_t *p2p, int events, int detail_error) {
  char   buf[1024];
  size_t read, written;
  int    error = 0;
  int    ret;
  int    len;

  if (events & P2P_EVENT_READ) {
    ret = xmpp_p2p_read(p2p, buf, sizeof buf, &read, &error);
    if (ret == P2P_SR_SUCCESS) {
      buf[read] = '\0';
      printf("[%s-->%s]:\n\t%s\n", xmpp_p2p_get_peer(p2p), xmpp_get_service_jid(), buf);
    }
    else if (ret == P2P_SR_ERROR) {
      printf("at %s read failed, ret=%d error=%d\n", __func__, ret, error);
    }
  }
  if (events & P2P_EVENT_WRITE) {
    static int cnt = 0;
    len = sprintf(buf, "hello, %s. cnt=%d", xmpp_p2p_get_peer(p2p), cnt++);
    ret = xmpp_p2p_write(p2p, buf, len, &written, &error);
    if (ret == P2P_SR_SUCCESS) {
      printf("[%s-->%s]:\n\t%s\n", xmpp_get_service_jid(), xmpp_p2p_get_peer(p2p), buf);
    }
    else {
      printf("[%s-->%s]: (failed, ret=%d error=%d)\n",
             xmpp_get_service_jid(), xmpp_p2p_get_peer(p2p), ret, error);
    }
  }
  if (events & P2P_EVENT_CLOSE) {
    printf("%s close event, p2p peer: %s\n", __func__, xmpp_p2p_get_peer(p2p));
    initiative_p2p = NULL;
    xmpp_p2p_close(p2p);
  }
}

