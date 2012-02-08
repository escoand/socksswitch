
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

/* wrapper for socket connecting */
int socksswitch_ssh_connect(ssh_session * session, const char *host,
			    const SOCKET_DATA_LEN port,
			    ssh_channel * channel) {
    DEBUG_ENTER;

    if (!ssh_is_connected(*session))
	return 0;

    TRACE_VERBOSE("connecting to %s:%i (session:%i channel:%i)\n",
		  host, port, *session, *channel);

    /* create channel */
    *channel = ssh_channel_new(*session);
    if (channel == NULL) {
	TRACE_WARNING
	    ("failure on creating channel (session:%i err:%i): %s\n",
	     *session, ssh_get_error_code(*session),
	     ssh_get_error(*session));
	DEBUG_LEAVE;
	return 0;
    }

    /* connect */
    if (ssh_channel_open_forward(*channel, host, port, "", 0) != SSH_OK) {
	TRACE_WARNING
	    ("failure on connect to %s:%i (session:%i err:%i): %s\n",
	     host, port, *session, ssh_get_error_code(*session),
	     ssh_get_error(*session));
	if (channel != NULL)
	    ssh_channel_free(*channel);
	DEBUG_LEAVE;
	return 0;
    }

    TRACE_INFO("connected to %s:%i (session:%i channel:%i)\n", host,
	       port, *session, *channel);

    DEBUG_LEAVE;
    return 1;
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

SOCKET_DATA_LEN socksswitch_ssh_recv(ssh_channel * channel, char *buf) {
    int rc;

    DEBUG_ENTER;

    rc = ssh_channel_read_nonblocking(*channel, buf, SOCKET_DATA_MAX, 0);

    DEBUG;

    /* data received */
    if (rc > 0) {
	TRACE_VERBOSE
	    ("recv from ssh (channel:%i length:%i)\n", *channel, rc);
	DUMP(buf, rc);
    }

    /* socket closed */
    else if (rc == SSH_EOF) {
	TRACE_VERBOSE("recv disconnect from ssh (channel:%i)\n", *channel);
    }

    /* error */
    else if (rc == SSH_ERROR) {
	TRACE_WARNING
	    ("failure on receiving (channel:%i err:%i): %s\n",
	     *channel, rc,
	     ssh_get_error(ssh_channel_get_session(*channel)));
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

int
socksswitch_ssh_send(ssh_channel * channel,
		     const char *buf, const SOCKET_DATA_LEN len) {
    int rc = 0, bytessend = 0;

    DEBUG_ENTER;

    /* send data */
    while (bytessend < len) {
	rc = ssh_channel_write(*channel, buf + bytessend, len - bytessend);

	/* error */
	if (rc <= 0) {
	    TRACE_WARNING
		("failure on sending (channel:%i: err:%i): %s\n",
		 *channel,
		 ssh_get_error_code(ssh_channel_get_session(*channel)),
		 ssh_get_error(ssh_channel_get_session(*channel)));
	    DEBUG_LEAVE;
	    return SOCKET_ERROR;
	}
	bytessend += rc;
    }

    /* successfull */
    if (rc > 0) {
	TRACE_VERBOSE
	    ("send to ssh (channel:%i length:%i rc:%i)\n",
	     *channel, len, rc);
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

int socksswitch_ssh_close(ssh_channel * channel) {
    int rc = 1;

    DEBUG_ENTER;

    if (channel == NULL || !channel_is_open(*channel)) {
	DEBUG_LEAVE;
	return 0;
    }

    /* disconnect */
    if (ssh_channel_send_eof(*channel) == SSH_OK &&
	ssh_channel_close(*channel) == SSH_OK) {
	ssh_channel_free(*channel);
	TRACE_INFO("disconnected from ssh (channel:%i)\n", *channel);
    }

    /* error */
    else {
	TRACE_WARNING("failure on closing (channel:%i err:%i): %s\n",
		      *channel,
		      ssh_get_error_code(ssh_channel_get_session
					 (*channel)),
		      ssh_get_error(ssh_channel_get_session(*channel)));
	ssh_channel_free(*channel);
	DEBUG_LEAVE;
	return SOCKET_ERROR;
    }

    DEBUG_LEAVE;
    return rc;
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
	TRACE_ERROR("failure on binding socket: %i\n", SOCKET_ERROR_CODE);
	DEBUG_LEAVE;
	exit(1);
    }
    TRACE_VERBOSE("master socket bound: %i\n", rc);

    DEBUG;

    /* listen master socket */
    if (listen(rc, 128) == SOCKET_ERROR) {
	TRACE_ERROR("failure on listening: %i\n", SOCKET_ERROR_CODE);
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
	TRACE_ERROR("failure on creating socket (err: %i): ",
		    SOCKET_ERROR_CODE);
	socketError();
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

/* create and connect ssh socket */
int
sshSocket(const char *host, const int port,
	  const char *username,
	  const char *privkeyfile,
	  const char *pubkeyfile, ssh_session * session) {
    int hlen;
    unsigned char *hash = NULL;

    DEBUG_ENTER;

    /* trace */
    TRACE_VERBOSE("connecting to %s@%s:%i\n", username, host, port);

    DEBUG;

    /* startup session */
    *session = ssh_new();
    if (*session == NULL) {
	DEBUG_LEAVE;
	return 0;
    }

    /* connect */
    ssh_options_set(*session, SSH_OPTIONS_HOST, host);
    ssh_options_set(*session, SSH_OPTIONS_PORT, &port);
    if (ssh_connect(*session) != SSH_OK) {
	TRACE_ERROR
	    ("failure on connecting to %s:%i (session:%i): %s\n",
	     host, port, *session, ssh_get_error(*session));
	ssh_disconnect(*session);
	ssh_free(*session);
	DEBUG_LEAVE;
	return 0;
    }

    /* verify host */
    hlen = ssh_get_pubkey_hash(*session, &hash);
    switch (ssh_is_server_known(*session)) {
    case SSH_SERVER_KNOWN_OK:
	break;
    case SSH_SERVER_KNOWN_CHANGED:
    case SSH_SERVER_FOUND_OTHER:
    case SSH_SERVER_FILE_NOT_FOUND:
    case SSH_SERVER_NOT_KNOWN:
    case SSH_SERVER_ERROR:
	TRACE_WARNING
	    ("ssh fingerprint %s invalid for %s\n",
	     ssh_get_hexa(hash, hlen), host);
	//return SOCKET_ERROR;
    }

    DEBUG;

    /* auth */
    if (ssh_userauth_privatekey_file(*session, username, privkeyfile, "")
	!= SSH_AUTH_SUCCESS) {
	TRACE_WARNING
	    ("failure on ssh auth to %s:%i: (session:%i err:%i) %s\n",
	     host, port, *session, ssh_get_error_code(*session),
	     ssh_get_error(*session));
	DEBUG_LEAVE;
	return 0;
    }

    TRACE_INFO
	("connected to %s:%i (socket:%i session:%i)\n",
	 host, port, ssh_get_fd(*session), *session);

    DEBUG_LEAVE;
    return ssh_get_fd(*session);
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
