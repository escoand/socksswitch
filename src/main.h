
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


#ifndef MAIN_H
#define MAIN_H

#define NAME    "socksswitch"
#define VERSION "20120206"
#define DESC    "forwards socks connection on basis of filter rules"
#define RIGHTS  "Copyright (C) 2011-2012 Andreas Schoenfelder"

typedef struct {
    char host[256];
    int port;
    char user[32];
    char privkeyfile[256];
    char pubkeyfile[256];
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


void showHelp(char *);
void readParams(const int, char *argv[]);
int readConfig();
int matchFilter(FORWARD_DESTINATION **, const char *, const int);
void cleanEnd(const int);
int forward(const int, ssh_channel *, const char *, const int);
int forwardsAdd(FORWARD_TYPE, const int, const int, ssh_channel *);

void handleSSHChannel(char *, const int);
void handleSocksRequest(const int, char *, const int);


#endif				/* MAIN_H */
