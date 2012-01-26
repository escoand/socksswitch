/*
 * main.h
 */

#ifndef MAIN_H
#define MAIN_H

#include <libssh2.h>

#define NAME    "socksswitch"
#define VERSION "20120126"
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
    LIBSSH2_SESSION *session;
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
    LIBSSH2_CHANNEL *channel;
} FORWARD_PAIR;


void showHelp(char *);
void readParams(const int, char *argv[]);
int readConfig();
int matchFilter(FORWARD_DESTINATION **, const char *, const int);
void cleanEnd(const int);
int forward(const int, LIBSSH2_CHANNEL *, const char *, const int);
int forwardsAdd(FORWARD_TYPE, const int, const int, LIBSSH2_CHANNEL **);

void handleSSHChannel(char *, const int);
void handleSocksRequest(const int, char *, const int);


#endif				/* MAIN_H */
