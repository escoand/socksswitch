
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


#ifdef WIN32
#include <windef.h>
#include <winsock2.h>
#else
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#endif
#include <stdio.h>
#include "sockets.h"
#include "trace.h"

/* init winsock */
void socksswitch_init() {
#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;

    DEBUG_ENTER;

    wVersionRequested = MAKEWORD(1, 1);
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
	TRACE_ERROR("winsock init: %i\n", SOCKET_ERROR_CODE);
	DEBUG_LEAVE;
	exit(1);
    }

    DEBUG_LEAVE;
#endif
}

/* get socket address */
const char *socksswitch_addr(const int sock) {
    struct sockaddr_in addr;
    SOCKET_ADDR_LEN addrlen = sizeof(struct sockaddr_in);
    char *addrstr;

    DEBUG_ENTER;

    addrstr = malloc(256 * sizeof(char));
    memset(addrstr, 0, 256);

    getpeername(sock, (struct sockaddr *) &addr, &addrlen);
    sprintf(addrstr, "%s:%i", inet_ntoa(addr.sin_addr),
	    ntohs(addr.sin_port));

    DEBUG_LEAVE;
    return addrstr;
}

/* wrapper for socket accpeting */
int socksswitch_accept(const int sock) {
    SOCKET_ADDR_LEN addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    int rc;

    DEBUG_ENTER;

    rc = accept(sock, (struct sockaddr *) &addr, &addrlen);

    /* socket connected */
    if (rc > 0) {
	TRACE_INFO("connect from %s:%i (socket:%i)\n",
		   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), rc);
    }

    /* error */
    else {
	TRACE_WARNING("failure on connecting (socket:%i err:%i): %s\n",
		      sock, SOCKET_ERROR_CODE, socketError());
	socketError();
    }

    DEBUG_LEAVE;
    return rc;
}

/* wrapper for socket receiving */
SOCKET_DATA_LEN socksswitch_recv(const int sock, char *buf) {
    int rc = 0;

    DEBUG_ENTER;

    if (sock <= 0) {
	DEBUG_LEAVE;
	return 0;
    }

    rc = recv(sock, buf, SOCKET_DATA_MAX, 0);

    /* data received */
    if (rc > 0) {
	TRACE_VERBOSE
	    ("recv from %s (socket:%i length:%i)\n",
	     socksswitch_addr(sock), sock, rc);
	DUMP(buf, rc);
    }

    /* socket closed */
#ifdef WIN32
    else if (rc == 0 || SOCKET_ERROR_CODE == WSAECONNRESET) {
#else
    else if (rc == 0) {
#endif
	TRACE_VERBOSE
	    ("recv disconnect from %s (socket:%i)\n",
	     socksswitch_addr(sock), sock);
    }

    /* error */
    else {
	TRACE_WARNING
	    ("failure on receiving from %s (socket:%i err:%i): %s\n",
	     socksswitch_addr(sock), sock, SOCKET_ERROR_CODE,
	     socketError());
	socketError();
	DEBUG_LEAVE;
	return SOCKET_ERROR;
    }

    DEBUG_LEAVE;
    return rc;
}

/* wrapper for socket sending */
int
socksswitch_send(const int sock,
		 const char *buf, const SOCKET_DATA_LEN len) {
    int rc = 0, bytessend = 0;

    DEBUG_ENTER;

    if (sock <= 0)
	return SOCKET_ERROR;

    /* send data */
    while (bytessend < len) {
	rc = send(sock, buf + bytessend, len - bytessend, 0);

	/* error */
	if (rc <= 0) {
	    TRACE_WARNING
		("failure on sending to %s (socket:%i: err:%i): %s\n",
		 socksswitch_addr(sock), sock, SOCKET_ERROR_CODE,
		 socketError());
	    socketError();
	    DEBUG_LEAVE;
	    return SOCKET_ERROR;
	}
	bytessend += rc;
    }

    /* successful */
    if (rc >= 0) {
	TRACE_VERBOSE
	    ("send to %s (socket:%i length:%i rc:%i)\n",
	     socksswitch_addr(sock), sock, len, rc);
	DUMP(buf, len);
    }

    DEBUG_LEAVE;
    return rc;
}

/* wrapper for socket closing */
int socksswitch_close(const int sock) {
    char addrstr[256];

    DEBUG_ENTER;

    if (sock <= 0) {
	DEBUG_LEAVE;
	return 0;
    }

    strcpy(addrstr, socksswitch_addr(sock));

    /* disconnect */
    if (shutdown(sock, SD_BOTH) == 0 && SOCKET_CLOSE(sock) == 0)
	TRACE_INFO("disconnected from %s (socket:%i)\n", addrstr, sock);

    /* error */
    else {
	TRACE_WARNING
	    ("failure on closing from %s (socket:%i err:%i): %s\n",
	     addrstr, sock, SOCKET_ERROR_CODE, socketError());
	DEBUG_LEAVE;
	return SOCKET_ERROR;
    }

    DEBUG_LEAVE;
    return 1;
}

/* create and bind master socket */
int masterSocket(const int port) {
    int rc;
    struct sockaddr_in addr;

    DEBUG_ENTER;

    /* create master socket */
    rc = socket(AF_INET, SOCK_STREAM, 0);
    if (rc == SOCKET_ERROR) {
	TRACE_ERROR("master socket create (err:%i): %s\n",
		    SOCKET_ERROR_CODE, socketError());
	DEBUG_LEAVE;
	return 0;
    }

    TRACE_VERBOSE("master socket created: %i\n", rc);

    DEBUG;

    /* set socket non blocking */
#ifdef WIN32
    unsigned long nValue = 1;
    ioctlsocket(rc, FIONBIO, &nValue);
#endif

    DEBUG;

    /* bind master socket */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind
	(rc, (struct sockaddr *) &addr,
	 sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
	TRACE_ERROR("failure on binding socket (err:%i): %s\n",
		    SOCKET_ERROR_CODE, socketError());
	DEBUG_LEAVE;
	exit(1);
    }
    TRACE_VERBOSE("master socket bound: %i\n", rc);

    DEBUG;

    /* listen master socket */
    if (listen(rc, 128) == SOCKET_ERROR) {
	TRACE_ERROR("failure on listening (err:%i): %s\n",
		    SOCKET_ERROR_CODE, socketError());
	DEBUG_LEAVE;
	exit(1);
    }

    TRACE_INFO("listening on *:%d (socket:%i)\n", port, rc);
    DEBUG_LEAVE;
    return rc;
}

/* create and connect client socket */
int clientSocket(const char *host, const int port) {
    int rc, sock;
    struct sockaddr_in addr;

    DEBUG_ENTER;

    /* create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    DEBUG;

    /* error */
    if (sock <= 0) {
	TRACE_ERROR("failure on creating socket (err:%i): %s\n",
		    SOCKET_ERROR_CODE, socketError());
	DEBUG_LEAVE;
	return 0;
    }

    DEBUG;

    /* connect to host */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);
    rc = connect(sock, (struct sockaddr *) (&addr),
		 sizeof(struct sockaddr_in));

    DEBUG;

    /* error */
    if (rc != 0) {
	TRACE_ERROR
	    ("failure on connecting to %s:%i (socket:%i err:%i): %s\n",
	     host, port, sock, SOCKET_ERROR_CODE, socketError());
	DEBUG_LEAVE;
	return 0;
    }

    TRACE_INFO("connected to %s (socket:%i)\n", socksswitch_addr(sock),
	       sock);

    DEBUG_LEAVE;
    return sock;
}

/* getting error messages */
const char *socketError() {
    int err = SOCKET_ERROR_CODE;
    char *msg;

    DEBUG_ENTER;

    msg = malloc(1024 * sizeof(char));
    memset(msg, 0, 1024);

    if (err != 0) {
#ifdef WIN32
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		      err, LANG_SYSTEM_DEFAULT, msg, 1023, NULL);
	strcat(msg, "\n");
#else
	strcpy(msg, strerror(err));
	strcat(msg, "\n");
#endif
    }

    DEBUG_LEAVE;
    return msg;
}
