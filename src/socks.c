#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "socks.h"
#include "trace.h"

/* return destination host of socks request */
void getSocksReqHost(char *host, const char *buf, const int len) {
    SOCKS_REQUEST *req = (SOCKS_REQUEST *) malloc(len);

    DEBUG;

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
}

/* return destination port of socks request */
unsigned short getSocksReqPort(const char *buf, const int len) {
    SOCKS_REQUEST *req = (SOCKS_REQUEST *) malloc(len);
    unsigned short port = 0;

    DEBUG;

    memcpy(req, buf, len);

    /* socks4 */
    if (req->ver == 4)
	port = req->data.socks4.port;

    /* socks5 */
    else if (req->ver == 5) {
	memcpy(&port,
	       &req->data.socks5.data.str + req->data.socks5.data.str.len +
	       2, 1);
	memcpy(&port + 1,
	       &req->data.socks5.data.str + req->data.socks5.data.str.len +
	       1, 1);
    }

    free(req);
    return port;
}
