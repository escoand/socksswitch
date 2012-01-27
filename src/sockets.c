
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
#include <winsock.h>
#include <io.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libssh/libssh.h>
#include "sockets.h"
#include "trace.h"


  /* init winsock */
int socksswitch_init()
{
#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);
    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
	TRACE_ERROR("winsock init: %i\n", SOCKET_ERROR_CODE);
	return SOCKET_ERROR;
    }
#endif

    return 0;
}

/* get socket address */
void socksswitch_addr(const int sock, char *addrstr)
{
    struct sockaddr_in addr;
#if defined WIN32 && !defined MINGW32
    int addrlen = sizeof(struct sockaddr_in);
#else
    unsigned int addrlen = sizeof(struct sockaddr_in);
#endif

    getpeername(sock, (struct sockaddr *) &addr, &addrlen);
    sprintf(addrstr, "%s:%i", inet_ntoa(addr.sin_addr),
	    ntohs(addr.sin_port));
}

/* wrapper for socket accpeting */
int socksswitch_accept(const int sock)
{
    struct sockaddr_in addr;
#if defined WIN32 && !defined MINGW32
    int addrlen = sizeof(struct sockaddr_in);
#else
    unsigned int addrlen = sizeof(struct sockaddr_in);
#endif
    int rc = accept(sock, (struct sockaddr *) &addr, &addrlen);
    char addrstr[256];

    /* socket connected */
    if (rc > 0) {
	socksswitch_addr(sock, addrstr);
	TRACE_INFO("connect from %s (socket:%i)\n", addrstr, rc);
    }

    /* error */
    else {
	TRACE_WARNING("failure on connecting (socket:%i err:%i): ",
		      sock, SOCKET_ERROR_CODE);
	socketError();
    }

    return rc;
}

/* wrapper for socket receiving */
int socksswitch_recv(const int sock, char *buf)
{
    DEBUG;

    int rc = 0;
    char addrstr[256];

    if (sock <= 0)
	return 0;

    socksswitch_addr(sock, addrstr);
    rc = recv(sock, buf, SOCKET_DATA_MAX, 0);

    /* data received */
    if (rc > 0) {
	TRACE_VERBOSE("recv from %s (socket:%i length:%i)\n",
		      addrstr, sock, rc);
	DUMP(buf, rc);
    }

    /* socket closed */
#ifdef WIN32
    else if (rc == 0 || SOCKET_ERROR_CODE == WSAECONNRESET) {
#else
    else if (rc == 0) {
#endif
	TRACE_VERBOSE("recv disconnect from %s (socket:%i)\n", addrstr,
		      sock);
    }

    /* error */
    else {
	TRACE_WARNING("failure on receiving (socket:%i err:%i): ",
		      sock, SOCKET_ERROR_CODE);
	socketError();
	return SOCKET_ERROR;
    }

    DEBUG;

    return rc;
}

int socksswitch_ssh_recv(ssh_channel * channel, char *buf)
{
    DEBUG;

    int rc = ssh_channel_read(*channel, buf,
			      SOCKET_DATA_MAX, 0);
    DEBUG;

    /* data received */
    if (rc > 0 /*&& rc != LIBSSH2_ERROR_EAGAIN */ ) {
	TRACE_VERBOSE("recv from ssh (channel:%i length:%i)\n", *channel,
		      rc);
	DUMP(buf, rc);
    }

    /* error */
    else if (rc != 0) {
	TRACE_WARNING
	    ("failure on receiving (channel:%i err:%i): %s\n", *channel,
	     rc, ssh_get_error(ssh_channel_get_session(*channel)));
	return SOCKET_ERROR;
    }

    DEBUG;

    return rc;
}

/* wrapper for socket sending */
int
socksswitch_send(const int sock, const char *buf,
		 const SOCKET_DATA_LEN len)
{
    DEBUG;

    int rc = 0, bytessend = 0;
    char addrstr[256];

    /* send data */
    while (bytessend < len) {
	rc = send(sock, buf + bytessend, len - bytessend, 0);

	/* error */
	if (rc <= 0) {
	    TRACE_WARNING("failure on sending (socket:%i: err:%i): ", sock,
			  SOCKET_ERROR_CODE);
	    socketError();
	    return SOCKET_ERROR;
	}
	bytessend += rc;
    }

    /* successful */
    if (rc >= 0) {
	socksswitch_addr(sock, addrstr);
	TRACE_VERBOSE("send to %s (socket:%i length:%i rc:%i)\n",
		      addrstr, sock, len, rc);
	DUMP(buf, len);
    }

    DEBUG;

    return rc;
}

int
socksswitch_ssh_send(ssh_channel * channel, const char *buf,
		     const SOCKET_DATA_LEN len)
{
    DEBUG;

    int rc = 0, bytessend = 0;
    //char addrstr[256] = "";

    /* send data */
    while (bytessend < len) {
	rc = ssh_channel_write(*channel, buf + bytessend, len - bytessend);

	/* error */
	if (rc <= 0) {
	    TRACE_WARNING("failure on sending (channel:%i: err:%i): %s\n",
			  *channel, rc,
			  ssh_get_error(ssh_channel_get_session
					(*channel)));
	    return SOCKET_ERROR;
	}
	bytessend += rc;
    }

    /* successfull */
    if (rc > 0) {
	TRACE_VERBOSE("send to ssh (channel:%i length:%i rc:%i)\n",
		      *channel, len, rc);

	DUMP(buf, len);
    }

    DEBUG;

    return rc;
}

/* wrapper for socket closing */
int socksswitch_close(const int sock)
{
    int rc = 0;
    char addrstr[256];

    if (sock <= 0)
	return 0;

    shutdown(sock, 2);
    close(sock);

    socksswitch_addr(sock, addrstr);
    TRACE_INFO("disconnected from %s (socket:%i rc:%i err:%i)\n", addrstr,
	       sock, rc, SOCKET_ERROR_CODE);

    return rc;
}

int socksswitch_ssh_close(ssh_channel * channel)
{
    int rc = ssh_channel_close(*channel);

    TRACE_VERBOSE("disconnected from ssh (channel:%i length:%i)\n",
		  *channel, rc);

    return rc;
}

/* create and bind master socket */
int masterSocket(const int port)
{
    int rc;
    struct sockaddr_in addr;

    DEBUG;

    /* create master socket */
    rc = socket(AF_INET, SOCK_STREAM, 0);
    if (rc == SOCKET_ERROR) {
	TRACE_ERROR("master socket create: %i\n", SOCKET_ERROR_CODE);
	return SOCKET_ERROR;
    }
    TRACE_VERBOSE("master socket created: %i\n", rc);

    DEBUG;

    /* set socket non blocking */
#ifdef _WIN32
    unsigned long nValue = 1;
    ioctlsocket(rc, FIONBIO, &nValue);
#endif

    DEBUG;

    /* bind master socket */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(rc, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) ==
	SOCKET_ERROR) {
	TRACE_ERROR("master socket bind: %i\n", SOCKET_ERROR_CODE);
	return SOCKET_ERROR;
    }
    TRACE_VERBOSE("master socket bound: %i\n", rc);

    DEBUG;

    /* listen master socket */
    if (listen(rc, 128) == SOCKET_ERROR) {
	TRACE_ERROR("master socket listen: %i\n", SOCKET_ERROR_CODE);
	return SOCKET_ERROR;
    }

    TRACE_INFO("listening on *:%d (socket:%i)\n", port, rc);

    DEBUG;

    return rc;
}

/* create and connect client socket */
int clientSocket(const char *host, const int port)
{
    int rc, sock;
    struct sockaddr_in addr;
    char addrstr[256];

    DEBUG;

    /* create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    DEBUG;

    /* error */
    if (sock <= 0) {
	TRACE_ERROR("socket create\n");
	return SOCKET_ERROR;
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
	TRACE_ERROR("socket connect: %i\n", rc);
	return SOCKET_ERROR;
    }

    socksswitch_addr(sock, addrstr);
    TRACE_INFO("connected to %s (socket:%i)\n", addrstr, sock);

    DEBUG;

    return sock;
}

/* create and connect ssh socket */
int
sshSocket(const char *host, const int port,
	  const char *username, const char *privkeyfile,
	  const char *pubkeyfile, ssh_session * session)
{
    int hlen;
    unsigned char *hash = NULL;

    DEBUG;

    /* trace */
    TRACE_VERBOSE("ssh host: %s\n", host);
    TRACE_VERBOSE("ssh port: %i\n", port);
    TRACE_VERBOSE("ssh username: %s\n", username);
    TRACE_VERBOSE("ssh private keyfile: %s\n", privkeyfile);
    TRACE_VERBOSE("ssh public keyfile: %s\n", pubkeyfile);

    DEBUG;

    /* startup session */
    *session = ssh_new();
    if (*session == NULL)
	return 0;

    DEBUG;

    /* connect */
    ssh_options_set(*session, SSH_OPTIONS_HOST, host);
    ssh_options_set(*session, SSH_OPTIONS_PORT, &port);
    if (ssh_connect(*session) != SSH_OK) {
	TRACE_ERROR("ssh session connect (session:%i): %s\n", *session,
		    ssh_get_error(*session));
	ssh_disconnect(*session);
	ssh_free(*session);
	return 0;
    }

    DEBUG;

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
	TRACE_WARNING("ssh fingerprint %s invalid for %s\n",
		      ssh_get_hexa(hash, hlen), host);
	/*free(hash);
	   return SOCKET_ERROR; */
    }

    DEBUG;

    /* auth */
    if (ssh_userauth_privatekey_file(*session, username, privkeyfile, "")
	!= SSH_AUTH_SUCCESS) {
	TRACE_WARNING("ssh auth to %s:%i failed: %s\n", host, port,
		      ssh_get_error(*session));
	return SOCKET_ERROR;
    }

    TRACE_VERBOSE("ssh connected: %i\n", *session);

    free(hash);

    DEBUG;

    return ssh_get_fd(*session);
}

/* getting error messages */
void socketError()
{
    char msg[256] = "\n";
    if (SOCKET_ERROR_CODE != 0) {
#ifdef WIN32
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
		      SOCKET_ERROR_CODE, LANG_SYSTEM_DEFAULT, msg,
		      sizeof(msg), NULL);
#else
	strcpy(msg, strerror(SOCKET_ERROR_CODE));
	strcat(msg, "\n");
#endif
    }
    printf("%s", msg);
}
