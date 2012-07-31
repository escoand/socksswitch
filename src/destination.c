
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


#include <libssh/libssh.h>
#include "destination.h"
#include "request.h"
#include "ssh.h"
#include "sockets.h"
#include "trace.h"

FORWARD_DESTINATION destinations[32];
int destinations_count = 0;

int _matching(char *text, char *pattern) {
    char *orig_pattern = pattern;

    /* matching beginning */
    if (*pattern == '^' || *pattern == '!')
	pattern++;

    /* run through text */
    for (; *text != '\0'; text++) {

	/* beginning */
	if (*pattern == '^')
	    return 0;

	/* ending */
	else if (*pattern == '\0')
	    return 1;
	else if (*pattern == '$' && *(pattern + 1) != '\0')
	    return 0;

	/* any char */
	else if (*pattern == '?')
	    pattern++;

	/* any char */
	else if (*pattern == '*') {
	    pattern++;
	    orig_pattern = pattern;
	}

	/* matching */
	else if (*text == *pattern)
	    pattern++;

	/* not matching */
	else if (*text != *pattern && pattern != orig_pattern) {
	    pattern = orig_pattern;
	    text--;
	}
    }

    if (*pattern == '\0' || (*pattern == '$' && *(pattern + 1) == '\0'))
	return 1;
    else
	return 0;
}

void socksswitch_client_thread(void *params) {
    int rc, port;
    SOCKET_DATA_LEN sock;
    struct timeval to;
    fd_set set;
    char buf[SOCKET_DATA_MAX], host[256];
    SOCKS_REQUEST req;
    FORWARD_DESTINATION *dst = NULL;
    ssh_channel channel;

    DEBUG_ENTER;

    sock = ((SOCKSSWITCH_THREAD_DATA *) params)->sock;
    TRACE_VERBOSE("thread (socket:%i)\n", sock);

    to.tv_sec = 10;
    to.tv_usec = 0;

    FD_ZERO(&set);
    FD_SET(sock, &set);

    DEBUG;

    /* socks5 handshake */
    rc = select(sock + 1, &set, NULL, NULL, &to);
    rc = socksswitch_socket_recv(sock, buf);
    if (rc != 3 || memcmp(buf, "\x05\x01\x00", 3) != 0) {
	TRACE_VERBOSE("socks5 handshake (rc:%i err:%i): %s\n", rc,
		      WSAGetLastError(), socketError());
	socksswitch_socket_close(sock);
	DEBUG_LEAVE;
	return;
    }

    DEBUG;

    /* sock5 ack */
    rc = select(sock + 1, NULL, &set, NULL, &to);
    rc = socksswitch_socket_send(sock, "\x05\x00", 2);
    if (rc != 2) {
	TRACE_VERBOSE("socks5 ack (rc:%i err:%i): %s\n", rc,
		      WSAGetLastError(), socketError());
	socksswitch_socket_close(sock);
	DEBUG_LEAVE;
	return;
    }

    DEBUG;

    /* socks5 request */
    rc = select(sock + 1, &set, NULL, NULL, &to);
    rc = socksswitch_socket_recv(sock, buf);
    if (rc <= 3 || memcmp(buf, "\x05\x01\x00", 3) != 0) {
	TRACE_VERBOSE("socks5 request (rc:%i err:%i): %s\n", rc,
		      WSAGetLastError(), socketError());
	socksswitch_socket_close(sock);
	DEBUG_LEAVE;
	return;
    }

    DEBUG;

    /* testing filters */
    if (!socksswitch_destination_match(&dst, buf, rc)) {
	DEBUG;
	return;
    }

    DEBUG;

    memcpy(&req, buf, sizeof(req));
    memset(host, 0, sizeof(host));
    socksswitch_request_host(host, buf, rc);
    port = socksswitch_request_port(buf, rc);

    TRACE_VERBOSE("connecting to %s:%i (session:%i)\n",
		  host, port, dst->session);

    /* test session */
    if (!ssh_is_connected(dst->session)) {
	TRACE_WARNING("session not connected (session:%i)\n",
		      dst->session);
	socksswitch_socket_close(sock);
	DEBUG_LEAVE;
	return;
    }

    DEBUG;

    /* create channel */
    channel = ssh_channel_new(dst->session);
    if (channel == NULL) {
	TRACE_WARNING
	    ("failure on creating channel (session:%i err:%i): %s\n",
	     dst->session, ssh_get_error_code(dst->session),
	     ssh_get_error(dst->session));

	/* send negative feedback */
	req.cmd = 0x01;
	socksswitch_socket_send(sock, (char *) &req,
				socksswitch_request_length(&req));

	/* exit */
	socksswitch_socket_close(sock);
	DEBUG_LEAVE;
	return;
    }

    DEBUG;

    /* start forwarding */
    rc = ssh_channel_open_forward(channel, host, port, "", 0);
    if (rc != SSH_OK) {
	TRACE_WARNING
	    ("failure on connect to %s:%i (session:%i err:%i): %s\n",
	     host, port, dst->session,
	     ssh_get_error_code(dst->session),
	     ssh_get_error(dst->session));

	/* send negative feedback */
	req.cmd = 0x01;
	socksswitch_socket_send(sock, (char *) &req,
				socksswitch_request_length(&req));

	if (channel != NULL)
	    ssh_channel_free(channel);
	socksswitch_socket_close(sock);
	DEBUG_LEAVE;
	return;
    }

    DEBUG;

    /* send positive feedback */
    req.cmd = 0x00;
    socksswitch_socket_send(sock, (char *) &req,
			    socksswitch_request_length(&req));

    DEBUG;

    TRACE_INFO("connected to %s:%i (session:%i channel:%i)\n", host,
	       port, dst->session, channel);

    while (TRUE) {
	memset(buf, 0, SOCKET_DATA_MAX);

	DEBUG;

	rc = socksswitch_ssh_select(&channel, sock);

	TRACE_DEBUG("socksswitch_ssh_select (rc:%i)\n", rc);

	/* error */
	if (rc < 0) {
	    socksswitch_ssh_close(&channel);
	    //ssh_channel_free(channel[0]);
	    socksswitch_socket_close(sock);
	    TRACE_VERBOSE("close thread\n");
	    DEBUG_LEAVE;
	    return;
	}

	/* channel action */
	else if (rc == 1) {
	    DEBUG;

	    rc = socksswitch_ssh_recv(&channel, buf);
	    if (rc <= 0) {
		socksswitch_ssh_close(&channel);
		//ssh_channel_free(channel[0]);
		socksswitch_socket_close(sock);
		TRACE_VERBOSE("close thread\n");
		DEBUG_LEAVE;
		return;
	    } else if (rc > 0)
		socksswitch_socket_send(sock, buf, rc);
	}

	/* socket action */
	else if (rc == 2) {
	    DEBUG;

	    rc = socksswitch_socket_recv(sock, buf);
	    if (rc <= 0) {
		socksswitch_ssh_close(&channel);
		//ssh_channel_free(channel[0]);
		socksswitch_socket_close(sock);
		TRACE_VERBOSE("close thread\n");
		DEBUG_LEAVE;
		return;
	    } else if (rc > 0)
		socksswitch_ssh_send(&channel, buf, rc);
	}
    }

    DEBUG_LEAVE;
    return;
}

FORWARD_DESTINATION *socksswitch_destination_get(const int i) {
    if (i <= destinations_count) {
	return (FORWARD_DESTINATION *) & destinations[i];
    }
    return NULL;
}

void socksswitch_destination_add(FORWARD_DESTINATION dest) {
    destinations[destinations_count++] = dest;
}

int socksswitch_destination_match(FORWARD_DESTINATION ** fo,
				  const char *buf, const int len) {
    unsigned int i, j;
    char host[256] = "";

    DEBUG_ENTER;

    socksswitch_request_host(host, buf, len);
    for (i = 0; i < destinations_count; i++)
	for (j = 0; j < destinations[i].filters_count; j++)
	    if (*destinations[i].filters[j] == '!' &&
		_matching(host, destinations[i].filters[j])) {
		("request %s:%i matches neg filter %s for %s:%i\n",
		 host, socksswitch_request_port(buf, len),
		 destinations[i].filters[j],
		 destinations[i].host, destinations[i].port);
		break;
	    } else if (_matching(host, destinations[i].filters[j])) {
		TRACE_INFO
		    ("request %s:%i matches filter %s for %s:%i\n",
		     host, socksswitch_request_port(buf, len),
		     destinations[i].filters[j],
		     destinations[i].host, destinations[i].port);
		*fo = &destinations[i];
		return 1;
	    }

    TRACE_WARNING("request %s:%i matches no filter\n",
		  host, socksswitch_request_port(buf, len));

    DEBUG_LEAVE;
    return 0;
}
