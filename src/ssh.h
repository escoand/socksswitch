
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


#ifndef SSH_H
#define SSH_H

#include <libssh/libssh.h>
#include "sockets.h"

int socksswitch_ssh_connect(ssh_session *, const char *,
			    const SOCKET_DATA_LEN, ssh_channel *);
SOCKET_DATA_LEN socksswitch_ssh_recv(ssh_channel *, char *);
int socksswitch_ssh_send(ssh_channel *, const char *,
			 const SOCKET_DATA_LEN);
int socksswitch_ssh_close(ssh_channel *);

int sshSocket(const char *, const int, const char *, const char *,
	      const char *, ssh_session *);

#endif				/* SSH_H */
