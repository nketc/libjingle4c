#include <cstdlib>
#include <cstring>
#include "service/talkclient.h"
#include "cif/notify.h"
#include "cif/roster.h"

XmppNotify* _get_xmpp_notify(void);
TalkClient* _get_xmpp_talkclient(void);
static void free_one_roster(roster_t *r);

roster_t* xmpp_get_rosters(void)
{
  TalkClient *client = _get_xmpp_talkclient();
  if (!client)
    return NULL;
  RosterInfoMap *rosters = client->GetRosterInfo();

  roster_t *h = NULL, *r;
  for (RosterInfoMap::iterator iter = rosters->begin();
       iter != rosters->end(); iter++) {
	RosterInfo *ri = &iter->second;
    r = (roster_t *) malloc(sizeof(*r));
    r->account = strdup(ri->account.c_str());
	r->name    = ri->name != "" ? strdup(ri->name.c_str()) : NULL;
	r->sub     = ri->sub == "both" ? XMPP_ROSTER_BOTH : XMPP_ROSTER_NONE;//TODO
	r->jid     = NULL;
	r->respart = NULL;
    if (ri->res) {
	  jid_resource_t *rh = NULL, *res;
	  for (ResMap::iterator rr = ri->res->begin();
	       rr != ri->res->end(); rr++) {
	    res = (jid_resource_t *)malloc(sizeof(*res));
		res->resource = strdup(rr->second.jid.resource().c_str());
		res->prio     = rr->second.prio;
		res->show     = (roster_show_t)rr->second.show;
		res->status   = strdup(rr->second.status.c_str());
		res->next = rh;
		rh = res;
	  }
	  r->respart = rh;
      if (rh) {
	    r->jid = strdup((ri->account + rh->resource).c_str());
	  }
	}
	r->next = h;
	h = r;
  }
  return h;
}

void xmpp_free_rosters(roster_t *rosters)
{
  if (!rosters)
    return;
  roster_t *r = rosters, *t;
  do {
    t = r->next;
    free_one_roster(r);
	r = t;
  } while(r);
}

void xmpp_set_roster_listener(void (*presence_cb)(const roster_status_t *))
{
  if (!presence_cb)
    return;
  XmppNotify *notify = _get_xmpp_notify();
  notify->RegisterPresenceListener(presence_cb);
}

static void free_one_roster(roster_t *r)
{
  jid_resource_t *res = r->respart, *rt;;
  free((void*)r->account);
  if (r->name) free((void*)r->name);
  if (r->jid)  free((void*)r->jid);
  free(r);

  if (res) {
    do {
	  rt = res->next;
	  if (res->resource) free((void*)res->resource);
	  if (res->status)   free(res->status);
	  free(res);
	  res = rt;
	} while(res);
  }
}

