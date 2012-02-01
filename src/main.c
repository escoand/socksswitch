
/*
 *     Copyright (C) 2011-2012 Andreas Schönfelder
 *     https://github.com/escoand/socksswitch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "main.h"
#include "match.h"
#include "sockets.h"
#include "socks.h"
#include "trace.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#include <libssh/libssh.h>

#if defined WIN32 && !defined MINGW32
#define FD_SET_SIZE(set) (set).fd_count
#define FD_SET_DATA(set, i) (set).fd_array[i]
#else
#define FD_SET_SIZE(set) FD_SETSIZE
#define FD_SET_DATA(set, i) __FDS_BITS(&set)[i]
#endif


fd_set sockets_set, read_set;
FORWARD_DESTINATION destinations[32];
FORWARD_PAIR forwards[32];
int destinations_count = 0;


int main(int argc, char *argv[]) {
    int rc;
    unsigned int i, j, masterport, socket_max, newsock;
    char buf[SOCKET_DATA_MAX], tmp[SOCKET_DATA_MAX];
    ssh_channel channel, channels_in[32], channels_out[32];
    FORWARD_DESTINATION *dst = NULL;
    char dst_host[256] = "";

    /* default */
    putenv((char *) "TRACE=2");

    /* read params and config */
    readParams(argc, argv);
    masterport = readConfig();

    DEBUG;

    /* init socket handling */
    socksswitch_init();
    FD_ZERO(&sockets_set);
    FD_ZERO(&read_set);
    for (i = 0; i < sizeof(forwards) / sizeof(forwards[0]); i++) {
	forwards[i].type = 0;
	forwards[i].left = 0;
	forwards[i].right = 0;
	forwards[i].channel = NULL;
    }

    DEBUG;

    /* init master socket */
    rc = masterSocket(masterport);
    if (rc > 0) {
	FD_SET(rc, &sockets_set);
	socket_max = rc;
    } else
	return 1;

    DEBUG;

    /* init ssh connections */
    for (i = 0; i < destinations_count; i++) {
	if (strlen(destinations[i].user) > 0) {
	    ssh_session session = NULL;
	    rc = sshSocket(destinations[i].host, destinations[i].port,
			   destinations[i].user,
			   destinations[i].privkeyfile,
			   destinations[i].pubkeyfile, &session);
	    if (rc > 0) {
		destinations[i].session = session;
		FD_SET(rc, &sockets_set);
	    } else {
		strcpy(destinations[i].host, "");
		destinations[i].port = 0;
		strcpy(destinations[i].user, "");
		strcpy(destinations[i].privkeyfile, "");
		strcpy(destinations[i].pubkeyfile, "");
		destinations[i].session = NULL;
	    }
	}
    }

    DEBUG;


    /* main loop */
    for (;;) {

	DEBUG;

	/* channels list */
	for (i = j = 0; i < sizeof(forwards) / sizeof(forwards[0]); i++)
	    if (forwards[i].channel != NULL)
		channels_in[j++] = forwards[i].channel;
	channels_in[j] = NULL;

	/* wait for socket actions */
	read_set = sockets_set;
	rc = ssh_select(channels_in, channels_out, socket_max + 1,
			&read_set, 0);

	DEBUG;

	/* master socket action */
	if (FD_ISSET(FD_SET_DATA(sockets_set, 0), &read_set)) {

	    DEBUG;

	    rc = socksswitch_accept(FD_SET_DATA(sockets_set, 0));

	    /* add new socket */
	    if (rc > 0) {
		FD_SET(rc, &sockets_set);
		if (rc > socket_max)
		    socket_max = rc;
	    }

	    continue;
	}

	DEBUG;

	/* ssh channel action */
	for (i = 0; channels_out[i] != NULL; i++) {
	    memset(buf, 0, SOCKET_DATA_MAX);
	    rc = socksswitch_ssh_recv(&channels_out[i], buf);

	    /* forward */
	    if (rc > 0)
		forward(0, &channels_out[i], buf, rc);

	    /* error */
	    else if (rc < 0)
		socksswitch_ssh_close(&channels_out[i]);
	}
	if (i > 0)
	    continue;

	DEBUG;

	/* client socket action */
	for (i = 0; i < FD_SET_SIZE(read_set); i++) {

	    /* skip empty */
	    if (FD_SET_DATA(read_set, i) == 0)
		continue;

	    /* get new data */
	    memset(buf, 0, SOCKET_DATA_MAX);
	    rc = socksswitch_recv(FD_SET_DATA(read_set, i), buf);

	    /* socket closed or error */
	    if (rc <= 0) {
		DEBUG;

		cleanEnd(FD_SET_DATA(read_set, i));
	    }

	    /* socks5 handshake */
	    else if (rc == 3 && memcmp(buf, "\x05\x01\x00", 3) == 0) {
		DEBUG;
		rc = socksswitch_send(FD_SET_DATA(read_set, i),
				      "\x05\x00", 2);
		if (rc < 0)
		    cleanEnd(rc);
	    }

	    /* socks5 request */
	    else if (rc > 3 && memcmp(buf, "\x05\x01\x00", 3) == 0) {

		/* testing filters */
		if (!matchFilter(&dst, buf, rc)) {
		    DEBUG;
		    cleanEnd(FD_SET_DATA(read_set, i));
		    continue;
		}

		/* ssh forwarding */
		if (dst->session != NULL) {
		    DEBUG;

		    getSocksReqHost(dst_host, buf, rc);

		    /* connected */
		    if (socksswitch_ssh_connect(&dst->session,
						dst_host,
						getSocksReqPort
						(buf, rc), &channel)) {

			/* add new socket */
			forwardsAdd(FORWARD_TYPE_SSH,
				    FD_SET_DATA(read_set, i), 0, &channel);

			/* init connection */
			buf[1] = 0x00;
			socksswitch_send(FD_SET_DATA(read_set, i),
					 buf, rc);
		    }

		    /* error */
		    else
			cleanEnd(FD_SET_DATA(read_set, i));

		    DEBUG;
		}

		/* direct forwarding */
		else {
		    DEBUG;
		    newsock = clientSocket(dst->host, dst->port);

		    /* connected */
		    if (newsock > 0) {
			DEBUG;

			/* add new socket */
			FD_SET(newsock, &sockets_set);
			forwardsAdd(FORWARD_TYPE_DIRECT,
				    FD_SET_DATA(read_set, i),
				    newsock, NULL);

			/* init connection */
			if (socksswitch_send
			    (newsock, "\x05\x01\x00", 3) > 0 &&
			    socksswitch_recv(newsock, tmp) > 0)
			    socksswitch_send(newsock, buf, rc);
		    }

		    /* error */
		    else {
			DEBUG;
			cleanEnd(FD_SET_DATA(read_set, i));
			cleanEnd(newsock);
		    }
		}
	    }

	    /* forwarding */
	    else if (!forward(FD_SET_DATA(read_set, i), NULL, buf, rc)) {
		DEBUG;
		cleanEnd(FD_SET_DATA(read_set, i));
	    }
	}
    }

    return 0;
}

/* show help */
void showHelp(char *binary) {
    printf("%s v%s\n", NAME, VERSION);
    printf("%s\n", DESC);
    printf("\n");
    printf("Usage: %s [OPTION]...\n", binary);
    printf("\n");
    printf("  -d         Dump datagram data (huge output).\n");
    printf("  -h         Display this help message.\n");
    printf("  -l <file>  Log to <file> instead STDOUT/STDERR.\n");
    printf("  -qq        Decrease verbosity to QUIET.\n");
    printf("  -q         Decrease verbosity to ERROR.\n");
    printf("  -v         Increase verbosity to INFO.\n");
    printf("  -vv        Increase verbosity to VERBOSE.\n");
    printf("             (Default verbosity is WARNING)\n");
    printf("\n");
    printf("%s\n", RIGHTS);
}

/* read parameters */
void readParams(const int argc, char *argv[]) {
    unsigned int i;
    char tmp[1025];

    /* read arguments */
    for (i = 1; i < argc; i++) {

	/* show help */
	if (strcmp(argv[i], "-h") == 0) {
	    showHelp(argv[0]);
	    exit(1);
	}

	/* logfile */
	else if (strcmp(argv[i], "-l") == 0) {
	    sprintf(tmp, "LOGFILE=%s", argv[++i]);
	    putenv(tmp);
	}

	/* tacelevel */
	else if (strcmp(argv[i], "-qq") == 0)
	    putenv((char *) "TRACE=0");
	else if (strcmp(argv[i], "-q") == 0)
	    putenv((char *) "TRACE=1");
	else if (strcmp(argv[i], "-v") == 0)
	    putenv((char *) "TRACE=3");
	else if (strcmp(argv[i], "-vv") == 0)
	    putenv((char *) "TRACE=4");

	/* dump */
	else if (strcmp(argv[i], "-d") == 0)
	    putenv((char *) "DUMP=1");

	/* unsupported argument */
	else {
	    showHelp(argv[0]);
	    exit(1);
	}
    }
}

/* read config file */
int readConfig() {
    FILE *cfgfile = NULL;
    char cfgline[1024];
    char cfgtype[256];
    int rc, masterport = 0;

    DEBUG_ENTER;

    /* open config file */
    cfgfile = fopen("socksswitch.cfg", "r");
    if (!cfgfile) {
	TRACE_ERROR("unable to open config file\n");
	exit(1);
    }

    /* read config */
    while (fgets(cfgline, sizeof(cfgline), cfgfile)) {

	/* skip comments */
	if (strlen(cfgline) == 1 || memcmp(cfgline, "#", 1) == 0)
	    continue;

	/* get cfg type */
	sscanf(cfgline, "%s ", cfgtype);

	/* get master port */
	if (strcmp(cfgtype, "listen") == 0)
	    sscanf(cfgline, "%*s %i", &masterport);

	/* get forward conenction */
	else if (strcmp(cfgtype, "forward") == 0) {
	    char host[256];
	    int port;
	    char user[32];
	    char privkeyfile[256];
	    char pubkeyfile[256];

	    /* read forward config */
	    rc = sscanf(cfgline,
			"%*s %s %i %s \"%[^\"]\" \"%[^\"]\"", host,
			&port, user, pubkeyfile, privkeyfile);
	    if (rc > 0) {
		strcpy(destinations[destinations_count].host, host);
		destinations[destinations_count].port = port;
	    }
	    if (rc == 2) {
		strcpy(destinations[destinations_count].user, "");
		strcpy(destinations[destinations_count].privkeyfile, "");
		strcpy(destinations[destinations_count].pubkeyfile, "");
	    } else if (rc == 5) {
		strcpy(destinations[destinations_count].user, user);
		strcpy(destinations[destinations_count].privkeyfile,
		       privkeyfile);
		strcpy(destinations[destinations_count].pubkeyfile,
		       pubkeyfile);
	    }
	    destinations_count++;
	}

	/* get filter rules */
	else if (strcmp(cfgtype, "filter") == 0) {
	    char filter[256] = "";
	    rc = sscanf(cfgline, "%*s %s", filter);
	    strcpy(destinations[destinations_count - 1].filters
		   [destinations
		    [destinations_count - 1].filters_count++], filter);
	}
    }

    /* close config file */
    fclose(cfgfile);

    DEBUG_LEAVE;
    return masterport;
}

/* get matching destination */
int matchFilter(FORWARD_DESTINATION ** fo, const char *buf, const int len) {
    unsigned int i, j;
    char host[256] = "";

    DEBUG_ENTER;

    getSocksReqHost(host, buf, len);

    for (i = 0; i < destinations_count; i++)
	for (j = 0; j < destinations[i].filters_count; j++)
	    if (matching(host, destinations[i].filters[j])) {
		TRACE_INFO
		    ("request %s:%i matches filter %s for %s:%i\n",
		     host, getSocksReqPort(buf, len),
		     destinations[i].filters[j],
		     destinations[i].host, destinations[i].port);
		*fo = &destinations[i];
		return 1;
	    }

    TRACE_WARNING("request %s:%i matches no filter\n",
		  host, getSocksReqPort(buf, len));

    DEBUG_LEAVE;
    return 0;
}

/* add partners to forward list */
int forwardsAdd(FORWARD_TYPE type, const int left,
		const int sock, ssh_channel * channel) {
    unsigned int i;

    DEBUG_ENTER;

    for (i = 0; i < sizeof(forwards) / sizeof(forwards[0]); i++) {
	if (forwards[i].left == 0) {
	    forwards[i].type = type;
	    forwards[i].left = left;
	    forwards[i].right = sock;
	    if (channel != NULL) {
		forwards[i].channel = *channel;
		TRACE_VERBOSE
		    ("forward added (socket:%i channel:%i)\n",
		     left, *channel);
	    } else
		TRACE_VERBOSE
		    ("forward added (socket:%i socket:%i)\n", left, sock);
	    return 1;
	}
    }

    TRACE_WARNING("too much forwards\n");

    DEBUG_LEAVE;
    return 0;
}

/* find a partner to forward data */
int forward(const int sock, ssh_channel * channel,
	    const char *buf, const int len) {
    unsigned int i;

    DEBUG_ENTER;

    for (i = 0; i < sizeof(forwards) / sizeof(forwards[0]); i++) {

	/* from local to direct */
	if (sock > 0
	    && forwards[i].type == FORWARD_TYPE_DIRECT
	    && forwards[i].left == sock) {

	    TRACE_VERBOSE
		("forward to remote (socket:%i to socket:%i)\n",
		 forwards[i].left, forwards[i].right);
	    if (socksswitch_send(forwards[i].right, buf, len)
		<= 0)
		cleanEnd(sock);
	    DEBUG;
	    return 1;
	}

	/* from local to ssh */
	else if (sock > 0
		 && forwards[i].type == FORWARD_TYPE_SSH
		 && forwards[i].left == sock) {

	    /*if (memcmp(buf, "SSH-2.0-", 8) == 0)
	       forwards[i].recv = -1;
	       else
	       forwards[i].recv = 0; */

	    TRACE_VERBOSE
		("forward to remote (socket:%i to channel:%i)\n",
		 forwards[i].left, forwards[i].channel);
	    socksswitch_ssh_send(&forwards[i].channel, buf, len);
	    DEBUG;
	    return 1;
	}

	/* from direct to local */
	else if (sock > 0
		 && forwards[i].type == FORWARD_TYPE_DIRECT
		 && forwards[i].right == sock) {

	    TRACE_VERBOSE
		("forward to local (socket:%i to socket:%i)\n",
		 forwards[i].right, forwards[i].left);
	    if (socksswitch_send(forwards[i].left, buf, len) <= 0)
		cleanEnd(sock);
	    DEBUG;
	    return 1;
	}

	/* from ssh to local */
	else if (channel != NULL
		 && forwards[i].type == FORWARD_TYPE_SSH
		 && forwards[i].channel == *channel) {

	    /*if (memcmp(buf, "SSH-2.0-", 8) == 0)
	       forwards[i].recv = -1;
	       else
	       forwards[i].recv = 0; */

	    TRACE_VERBOSE
		("forward to local (channel:%i to socket:%i)\n",
		 forwards[i].channel, forwards[i].left);
	    if (socksswitch_send(forwards[i].left, buf, len) <= 0)
		cleanEnd(sock);
	    DEBUG;
	    return 1;
	}
    }

    TRACE_VERBOSE("no forward (socket:%i channel:%i)\n", sock, channel);

    DEBUG_LEAVE;
    return 0;
}

/* clean shutdown of sockets */
void cleanEnd(const int sock) {
    unsigned int i;

    DEBUG_ENTER;

    /* disconnect forwardings */
    for (i = 0; i < sizeof(forwards) / sizeof(forwards[0]); i++) {
	if (forwards[i].left == sock || forwards[i].right == sock) {

	    /* close sockets */
	    if (sock != forwards[i].left && sock > 0) {
		FD_CLR(forwards[i].left, &sockets_set);
		FD_CLR(forwards[i].left, &read_set);
		socksswitch_close(forwards[i].left);
	    }
	    if (sock != forwards[i].right && sock > 0) {
		FD_CLR(forwards[i].right, &sockets_set);
		FD_CLR(forwards[i].right, &read_set);
		socksswitch_close(forwards[i].right);
	    }
	    if (forwards[i].channel != NULL)
		socksswitch_ssh_close(&forwards[i].channel);

	    /* clear forwards */
	    forwards[i].type = FORWARD_TYPE_NONE;
	    forwards[i].left = 0;
	    forwards[i].right = 0;
	    forwards[i].channel = NULL;
	}
    }

    /* disconnect unforwarded */
    if (FD_ISSET(sock, &sockets_set)) {
	FD_CLR(sock, &sockets_set);
	FD_CLR(sock, &read_set);
	socksswitch_close(sock);
    }

    DEBUG_LEAVE;
}
