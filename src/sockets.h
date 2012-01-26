/*
 * sockets.h
 */


#ifndef SOCKETS_H
#define SOCKETS_H

#include <libssh2.h>


#ifdef _WIN32
#define SOCKET_ADDR_LEN int
#define SOCKET_DATA_LEN int
#define SOCKET_ERROR_CODE WSAGetLastError()
#else
#define SOCKET_ADDR_LEN socklen_t
#define SOCKET_DATA_LEN size_t
#define SOCKET_ERROR -1
#define SOCKET_ERROR_CODE errno
#endif
#define SOCKET_DATA_MAX 16384


typedef enum {
    SOCKET_TYPE_LISTEN,
    SOCKET_TYPE_CLIENT,
    SOCKET_TYPE_SOCKS,
    SOCKET_TYPE_SSH
} SOCKET_TYPE;

int socksswitch_init();
void socksswitch_addr(const int, char *);
int socksswitch_accept(const int);
int socksswitch_recv(const int, char *);
int socksswitch_ssh_recv(LIBSSH2_CHANNEL **, char *);
int socksswitch_send(const int, const char *, const SOCKET_DATA_LEN);
int socksswitch_ssh_send(LIBSSH2_CHANNEL **, const char *,
			 const SOCKET_DATA_LEN);
int socksswitch_close(const int);
int socksswitch_ssh_close(LIBSSH2_CHANNEL **);

int masterSocket(const int);
int clientSocket(const char *, const int);
int sshSocket(const char *, const int, const char *, const char *,
	      const char *, LIBSSH2_SESSION **);
int sshForward(LIBSSH2_SESSION **, const char *, const int,
	       LIBSSH2_CHANNEL **);
int sshRead(LIBSSH2_CHANNEL *, char *, int);

void socketError();
void sshError(LIBSSH2_SESSION *);

#endif				/* SOCKETS_H */
