/*
 * socks.h
 */


#ifndef SOCKS_H
#define SOCKS_H


typedef struct {
    unsigned char ver;
    unsigned char cmd;
    union {
	struct {
	    unsigned short port;
	    unsigned char ip[4];
	    unsigned char method;
	} socks4;
	struct {
	    unsigned char rsv;
	    unsigned char atyp;
	    union {
		struct {
		    unsigned char len;
		    char name[];
		} str;
		unsigned char ipv4[4];
		unsigned char ipv6[16];
	    } data;
	    unsigned short port;
	} socks5;
    } data;
} SOCKS_REQUEST;

void getSocksReqHost(char *host, const char *buf, const int len);
unsigned short getSocksReqPort(const char *buf, const int len);

#endif				/* SOCKS_H */
