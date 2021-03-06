
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
#include "ssh.h"
#include "trace.h"

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
