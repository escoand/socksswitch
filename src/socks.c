
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
#include "socks.h"
#include "trace.h"

#ifdef WIN32
#include <winsock.h>
#else
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#endif

/* return destination host of socks request */
void getSocksReqHost(char *host, const char *buf, const int len) {
    SOCKS_REQUEST *req;

    DEBUG_ENTER;

    req = (SOCKS_REQUEST *) malloc(len);
    memcpy(req, buf, len);

    /* socks4 */
    if (req->ver == 4) {
	sprintf(host, "%i.%i.%i.%i", req->data.socks4.ip[0],
		req->data.socks4.ip[1], req->data.socks4.ip[2],
		req->data.socks4.ip[3]);
    }

    /* socks5 ipv4 */
    else if (req->ver == 5 && req->data.socks5.atyp == 1) {
	sprintf(host, "%i.%i.%i.%i", req->data.socks5.data.ipv4[0],
		req->data.socks5.data.ipv4[1],
		req->data.socks5.data.ipv4[2],
		req->data.socks5.data.ipv4[3]);
    }

    /* socks5 domain */
    else if (req->ver == 5 && req->data.socks5.atyp == 3) {
	memcpy(host, req->data.socks5.data.str.name,
	       req->data.socks5.data.str.len);
    }

    /* socks5 ipv6 */
    else if (req->ver == 5 && req->data.socks5.atyp == 4) {
	sprintf(host, "%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x",
		req->data.socks5.data.ipv6[0],
		req->data.socks5.data.ipv6[1],
		req->data.socks5.data.ipv6[2],
		req->data.socks5.data.ipv6[3],
		req->data.socks5.data.ipv6[4],
		req->data.socks5.data.ipv6[5],
		req->data.socks5.data.ipv6[6],
		req->data.socks5.data.ipv6[7],
		req->data.socks5.data.ipv6[8],
		req->data.socks5.data.ipv6[9],
		req->data.socks5.data.ipv6[10],
		req->data.socks5.data.ipv6[11],
		req->data.socks5.data.ipv6[12],
		req->data.socks5.data.ipv6[13],
		req->data.socks5.data.ipv6[14],
		req->data.socks5.data.ipv6[15]);
    }

    free(req);

    DEBUG_LEAVE;
}

/* return destination port of socks request */
unsigned short getSocksReqPort(const char *buf, const int len) {
    SOCKS_REQUEST *req;
    unsigned short port = 0;

    DEBUG_ENTER;

    req = (SOCKS_REQUEST *) malloc(len);
    memcpy(req, buf, len);

    /* socks4 */
    if (req->ver == 4)
	port = req->data.socks4.port;

    /* socks5 ipv4 */
    else if (req->ver == 5 && req->data.socks5.atyp == 1) {
	memcpy(&port,
	       &req->data.socks5.atyp +
	       sizeof(&req->data.socks5.data.ipv4) + 1, 2);
	port = ntohs(port);
    }

    /* socks5 domain */
    else if (req->ver == 5 && req->data.socks5.atyp != 4) {
	memcpy(&port,
	       &req->data.socks5.data.str + req->data.socks5.data.str.len +
	       1, 2);
	port = ntohs(port);
    }

    /* socks5 ipv6 */
    else if (req->ver == 5 && req->data.socks5.atyp == 4) {
	memcpy(&port,
	       &req->data.socks5.atyp +
	       sizeof(&req->data.socks5.data.ipv6) + 1, 2);
	port = ntohs(port);
    }

    free(req);

    DEBUG_LEAVE;
    return port;
}

/* return the length of th socks request */
int getSocksReqLen(const SOCKS_REQUEST * req) {
    /* socks4 */
    if (req->ver == 4)
	return 9;

    /* socks5 ipv4 */
    else if (req->ver == 5 && req->data.socks5.atyp == 1)
	return 10;

    /* socks5 domain */
    else if (req->ver == 5 && req->data.socks5.atyp == 3)
	return 7 + req->data.socks5.data.str.len;

    /* socks5 ipv6 */
    else if (req->ver == 5 && req->data.socks5.atyp == 4)
	return 22;

    return 0;
}
