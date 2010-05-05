#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include "options.h"
#include "defines.h"
#include "dhcp.h"
#include "debug.h"

static debuginfo DEBUG[] = { /* !!! Check cish_defines.h */
	{"acl","kernel: (acl)","Access list service", 0},
	{"bgp","bgpd[","BGP routing service", 0},
	{"bridge","kernel: bridge","Bridge daemon", 0},
	{"chat","chat[","Chat service", 0},
	{"config","config[","Configuration service", 0},
#ifdef OPTION_IPSEC
	{"crypto","pluto[","VPN daemon", 0},
#endif
#ifdef UDHCPD
	{"dhcp","udhcpd[","DHCP service", 0},
#else
	{"dhcp","dhcpd:","DHCP service", 0},
#endif
	{"ethernet0","kernel: ethernet0:","Ethernet daemon", 0},
	{"ethernet1","kernel: ethernet1:","Ethernet daemon", 0},
	{"frelay","kernel: (fr)","Frame-relay daemon", 0},
	{"hdlc","kernel: (cisco)","HDLC daemon", 0},
#ifdef CONFIG_DEVELOPMENT
	{"lapb","kernel: lapb","LAPB events", 0},
#endif
#ifdef OPTION_IPSEC
	{"l2tp","l2tpd","L2TP daemon", 0},
#endif
	{"login","login[","Login daemon", 0},
#ifdef OPTION_NTPD
	{"ntp","ntpd[","NTP daemon", 0},
#endif
	{"ospf","ospfd[","OSPF routing service", 0},
	{"ppp","pppd[","PPP daemon", 0},
	{"rfc1356","rfc1356[","RFC1356 tunnel", 0},
	{"rip","ripd[","RIP routing service", 0},
	{"ppp","kernel: (sppp)","PPP daemon", 0},
#ifdef OPTION_OPENSSH
	{"ssh","sshd","SSH service", 0},
#else
	{"ssh","dropbear[","SSH service", 0},
#endif
	{"systty","systtyd[","System control service", 0},
#ifdef OPTION_VRRP
	{"vrrp","Keepalived_vrrp","VRRP service", 0},
#endif
	{"zebra","zebra[","Routing daemon", 0},
	{NULL, NULL, NULL, 0}
};

static int find_debug(const char *name)
{
	int i;

	for (i=0; DEBUG[i].name; i++)
	{
		if (!strncasecmp(name, DEBUG[i].name, strlen(DEBUG[i].name))) return i;
	}
	return -1;
}

char *find_debug_token(char *logline, char *name, int enabled)
{
	char *p;
	debuginfo *dk;

	for( dk=DEBUG; dk->name; dk++ ) {
		if( (enabled || dk->enabled) && (strncmp(logline, dk->token, strlen(dk->token)) == 0) ) {
			p = strchr(logline+strlen(dk->token), ' ');
			if( p == NULL )
				return NULL;
			strncpy(name, dk->name, 14); /* renamed token! */
			strcat(name, ": ");
			return (p+1); /* log info! */
		}
	}
	return NULL;
}

int set_debug_token(int on_off, const char *token)
{
	int i, j;

	i=find_debug(token);
	if (i >= 0)
	{
		DEBUG[i].enabled=on_off;
#if 0
		cish_cfg->debug[i]=on_off; /* debug persistent */
#endif
		printf("  %s debbuging is %s\n", DEBUG[i].description, on_off ? "on" : "off");
	}
	if (!on_off)
	{
		for (j=0; DEBUG[j].name; j++)
		{ if (DEBUG[j].enabled) return -1; } /* Verifica se ainda ha algum debugging ligado. Se sim, mantem... */
	}
	return i;
}

void set_debug_all(int on_off)
{
	int i;

	for (i=0; DEBUG[i].name; i++) {
		DEBUG[i].enabled=on_off;
#if 0
		cish_cfg->debug[i]=on_off; /* debug persistent */
#endif
	}
	printf ("  All debugging %s\n", on_off ? "enabled" : "disabled");
}

void dump_debug(void)
{
	int i;

	for (i=0; DEBUG[i].name; i++)
	{
		if (DEBUG[i].enabled) printf("  %s debugging is on\n", DEBUG[i].description);
	}
}

int get_debug_state(const char *token)
{
	int i;

	i=find_debug(token);
	if (i >= 0)
		return DEBUG[i].enabled;
	return i;
}

void recover_debug_all(void)
{
#if 0
	int i;

	for (i=0; DEBUG[i].name; i++)
		DEBUG[i].enabled=cish_cfg->debug[i]; /* debug persistent */
#endif
}
