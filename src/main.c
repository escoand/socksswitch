
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


#ifndef WIN32
#include <stdlib.h>
#include <string.h>
#endif
#include <stdio.h>
#include <libssh/libssh.h>
#include "destination.h"
#include "inject.h"
#include "main.h"
#include "request.h"
#include "sockets.h"
#include "ssh.h"
#include "trace.h"

#if defined WIN32 && !defined MINGW32
#define FD_SET_SIZE(set) (set).fd_count
#define FD_SET_DATA(set, i) (set).fd_array[i]
#else
#define FD_SET_SIZE(set) FD_SETSIZE
#define FD_SET_DATA(set, i) __FDS_BITS(&set)[i]
#endif

char *cfgpath = NULL;
unsigned int masterport = 0;

int main(int argc, char *argv[]) {
    int rc, i;
    fd_set sockets_set, read_set;
    SOCKET_DATA_LEN sock;
    SOCKSSWITCH_THREAD_DATA thread_data;
    FORWARD_DESTINATION *dest;

    /* default */
    putenv("TRACE=2");

    /* read params and config */
    readParams(argc, argv);
    readConfig();

    DEBUG;

    /* init socket handling */
    socksswitch_socket_init();
    FD_ZERO(&sockets_set);
    FD_ZERO(&read_set);

    DEBUG;

    /* init master socket */
    sock = masterSocket(masterport);
    if (sock > 0)
	FD_SET(sock, &sockets_set);
    else
	return 1;

    DEBUG;

    /* init ssh connections */
    i = 0;
    dest = socksswitch_destination_get(i);
    while (dest != NULL) {
	if (strlen(dest->user) > 0) {
	    ssh_session session = NULL;
	    TRACE_VERBOSE("##### host:%s port:%i\n",dest->host, dest->port);
	    rc = socksswitch_ssh_connect(dest->host, dest->port,
					 dest->user,
					 dest->privkeyfile, &session);
	    if (rc > 0) {
		dest->session = session;
	    } else {
		strcpy(dest->host, "");
		dest->port = 0;
		strcpy(dest->user, "");
		strcpy(dest->privkeyfile, "");
		dest->session = NULL;
	    }
	}
	dest = socksswitch_destination_get(++i);
    }

    DEBUG;

/* inject thread */
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)
		 socksswitch_inject_thread, NULL, 0, NULL);

    /* main loop */
    for (;;) {

	DEBUG;

	/* wait for socket actions */
	read_set = sockets_set;
	rc = select(sock + 1, &read_set, NULL, NULL, NULL);
	TRACE_VERBOSE("select (rc:%i)\n", rc);

	DEBUG;

	/* master socket action */
	if (FD_ISSET(sock, &read_set)) {

	    rc = socksswitch_socket_accept(sock);
	    TRACE_VERBOSE("socksswitch_accept (rc:%i)\n", rc);

	    DEBUG;

	    /* new client thread */
	    if (rc > 0) {
		thread_data.sock = rc;
		TRACE_INFO("starting new thread\n", rc);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)
			     socksswitch_client_thread, &thread_data, 0,
			     NULL);
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
    printf
	("  -c <file>  Read config from <file> instead socksswitch.cfg.\n");
    printf("  -d         Dump datagram data (huge output).\n");
    printf("  -D <file>  Inject DLL <file> instead socksswitchdrv.dll.\n");
    printf("  -h         Display this help message.\n");
    printf("  -l <file>  Log to <file> instead STDOUT/STDERR.\n");
    printf("  -qq        Decrease verbosity to QUIET (no output).\n");
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
    char tmp[MAX_PATH + 1];

    /* read arguments */
    for (i = 1; i < argc; i++) {

	/* show help */
	if (strcmp(argv[i], "-h") == 0) {
	    showHelp(argv[0]);
	    exit(1);
	}

	/* config file */
	else if (strcmp(argv[i], "-c") == 0) {
	    i++;
	    strcpy(cfgpath, argv[i]);
	}

	/* log file */
	else if (strcmp(argv[i], "-l") == 0) {
	    i++;
	    sprintf(tmp, "LOGFILE=%s", argv[i]);
	    putenv(tmp);
	}

	/* dll file */
	else if (strcmp(argv[i], "-D") == 0) {
	    i++;
	    if (strchr(argv[i], ':') == NULL) {
		getcwd(tmp, MAX_PATH);
		strcat(tmp, "\\");
		strcat(tmp, argv[i]);
	    } else
		strcpy(tmp, argv[i]);
	    sprintf(tmp, "DLLPATH=%s", argv[i]);
	    putenv(tmp);
	}

	/* tacelevel */
	else if (strcmp(argv[i], "-qq") == 0)
	    putenv("TRACE=0");
	else if (strcmp(argv[i], "-q") == 0)
	    putenv("TRACE=1");
	else if (strcmp(argv[i], "-v") == 0)
	    putenv("TRACE=3");
	else if (strcmp(argv[i], "-vv") == 0)
	    putenv("TRACE=4");

	/* dump */
	else if (strcmp(argv[i], "-d") == 0)
	    putenv("DUMP=1");

	/* unsupported argument */
	else {
	    showHelp(argv[0]);
	    exit(1);
	}
    }
}

/* read config file */
void readConfig() {
    FILE *cfgfile = NULL;
    char cfgline[1024];
    char cfgtype[256];
    int rc;
    FORWARD_DESTINATION dest;

    DEBUG_ENTER;

    if (cfgpath == NULL)
	cfgpath = "socksswitch.cfg";
    memset(&dest, 0, sizeof(dest));

    /* open config file */
    cfgfile = fopen(cfgpath, "r");
    if (!cfgfile) {
	TRACE_ERROR("unable to open config file \"%s\".\n", cfgpath);
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

	    /* add destination */
	    if (strlen(dest.host) > 0)
		socksswitch_destination_add(dest);

	    /* clean */
	    memset(&dest, 0, sizeof(dest));

	    /* read forward config */
	    rc = sscanf(cfgline,
			"%*s %s %i %s \"%[^\"]\"", host,
			&port, user, privkeyfile);
	    if (rc > 0) {
		strcpy(dest.host, host);
		dest.port = port;
	    }
	    if (rc == 2) {
		strcpy(dest.user, "");
		strcpy(dest.privkeyfile, "");
	    } else if (rc == 4) {
		strcpy(dest.user, user);
		strcpy(dest.privkeyfile, privkeyfile);
	    }
	}

	/* get filter rules */
	else if (strcmp(cfgtype, "filter") == 0) {
	    char filter[256] = "";
	    rc = sscanf(cfgline, "%*s %s", filter);
	    strcpy(dest.filters[dest.filters_count++], filter);
	}

	/* get capture file */
	else if (strcmp(cfgtype, "capture") == 0) {
	    char path[MAX_PATH + 1] = "";
	    sscanf(cfgline, "%*s \"%[^\"]\"", path);
	    socksswitch_inject_add(path);
	}
    }

    /* add destination */
    if (strlen(dest.host) > 0)
	socksswitch_destination_add(dest);

    /* close config file */
    fclose(cfgfile);

    DEBUG_LEAVE;
}
