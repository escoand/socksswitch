
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


#ifndef TREAD_H
#define TREAD_H

#include "sockets.h"

typedef struct {
    char host[256];
    int port;
    char user[32];
    char privkeyfile[256];
    int filters_count;
    char filters[32][256];
    ssh_session session;
} FORWARD_DESTINATION;

typedef enum {
    FORWARD_TYPE_NONE,
    FORWARD_TYPE_DIRECT,
    FORWARD_TYPE_SSH
} FORWARD_TYPE;

typedef struct {
    FORWARD_TYPE type;
    int left;
    int right;
    ssh_channel channel;
    int recv;
} FORWARD_PAIR;

typedef struct {
    SOCKET_DATA_LEN sock;
} SOCKSSWITCH_THREAD_DATA;

void socksswitch_client_thread(void *);
FORWARD_DESTINATION* socksswitch_destination_get(const int);
void socksswitch_destination_add(FORWARD_DESTINATION);
int socksswitch_destination_match(FORWARD_DESTINATION **, const char *,
				  const int);

#endif				/* TREAD_H */
