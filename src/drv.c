
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


#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include "drv.h"
#include "sockets.h"
#include "trace.h"

#define COUNT        3
#define SIZE         6
#define TITLE_APPEND " via socksswitch"

BYTE jmp[COUNT][SIZE];
BYTE bytes[COUNT][SIZE];
FARPROC addr[COUNT];
FARPROC addr_new[COUNT];

BOOL WINAPI DllMain(HINSTANCE hinstDLL,
		    DWORD fdwReason, LPVOID lpvReserved) {
    DWORD protect, jmp_size;
    HWND h;
    int i, pid;
    char txt[1024] = "";

#if defined(_DEBUG) || defined(_DEBUG_)
    putenv("LOGFILE=c:\\socksswitchdrv.log");
    putenv("TRACE=4");
    putenv("DUMP=1");
#endif

    DEBUG_ENTER;

    switch (fdwReason) {

	/* hook */
    case DLL_PROCESS_ATTACH:

	/* init */
	for (i = 0; i < COUNT; i++) {
	    memset(bytes[i], 0, SIZE);
	    memcpy(jmp[i], "\xE9\x90\x90\x90\x90\xC3", SIZE);
	}

	/* function address */
	addr[0] = GetProcAddress(GetModuleHandle("ws2_32.dll"), "connect");
	addr[1] =
	    GetProcAddress(GetModuleHandle("ws2_32.dll"), "WSAConnect");
	addr[2] =
	    GetProcAddress(GetModuleHandle("user32.dll"),
			   "SetWindowTextA");
	addr_new[0] = new_connect;
	addr_new[1] = new_WSAConnect;
	addr_new[2] = new_SetWindowText;

#if defined(_DEBUG) || defined(_DEBUG_)
	for (i = 0; i < COUNT; i++) {
	    TRACE_VERBOSE("pid:%i org:%p new:%p\n", GetCurrentProcessId(),
			  addr[i], addr_new[i]);
	}
#endif

	/* save bytes */
	for (i = 0; i < COUNT; i++) {
	    if (addr[i] != NULL) {
		VirtualProtect(addr[i], SIZE,
			       PAGE_EXECUTE_READWRITE, &protect);
		memcpy(bytes[i], addr[i], SIZE);
		VirtualProtect(addr[i], SIZE, protect, NULL);
		jmp_size = (DWORD) addr_new[i] - (DWORD) addr[i] - 5;
		memcpy(&jmp[i][1], &jmp_size, 4);
	    }
	}

	_hook(0);

	/* window text */
	h = GetTopWindow(0);
	while (h) {
	    GetWindowThreadProcessId(h, (PDWORD) & pid);
	    if (pid == GetCurrentProcessId() && IsWindow(h)
		&& IsWindowVisible(h)) {
		GetWindowText(h, txt, sizeof(txt));
		SetWindowText(h, txt);
	    }
	    h = GetNextWindow(h, GW_HWNDNEXT);
	}

	for (i = 0; i < COUNT; i++) {
	    DUMP(bytes[i], SIZE);
	    DUMP(jmp[i], SIZE);
	}

	break;

	/* unhook */
    case DLL_PROCESS_DETACH:
	_unhook(0);
	break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
	break;
    }

    DEBUG_LEAVE;

    return TRUE;
}

void _hook(int func) {
    int i;
    DWORD protect;

    DEBUG_ENTER;

    for (i = 0; i < COUNT; i++) {
	if (addr[i] != NULL && (func == 0 || func == i + 1)) {
	    VirtualProtect(addr[i], SIZE, PAGE_EXECUTE_READWRITE,
			   &protect);
	    memcpy(addr[i], jmp[i], SIZE);
	    VirtualProtect(addr[i], SIZE, protect, NULL);
	}
    }

    DEBUG_LEAVE;
}

void _unhook(int func) {
    int i;
    DWORD protect;

    DEBUG_ENTER;

    for (i = 0; i < COUNT; i++) {
	if (addr[i] != NULL && (func == 0 || func == i + 1)) {
	    VirtualProtect(addr[i], SIZE, PAGE_EXECUTE_READWRITE,
			   &protect);
	    memcpy(addr[i], bytes[i], SIZE);
	    VirtualProtect(addr[i], SIZE, protect, NULL);
	}
    }

    DEBUG_LEAVE;
}

int WINAPI new_connect(SOCKET s, const struct sockaddr *name, int namelen) {
    return new_WSAConnect(s, name, namelen, NULL, NULL, NULL, NULL);
}

int WINAPI new_WSAConnect(SOCKET s,
			  const struct sockaddr *name,
			  int namelen,
			  LPWSABUF lpCallerData,
			  LPWSABUF lpCalleeData,
			  LPQOS lpSQOS, LPQOS lpGQOS) {
    int rc, err;
    struct sockaddr_in in_addr;
    struct sockaddr_in out_addr;
    char buf[256];
    fd_set set;

    DEBUG_ENTER;

    memcpy(&in_addr, name, sizeof(in_addr));

    /* change non-local */
    if (in_addr.sin_addr.s_addr != inet_addr("127.0.0.1")) {
	DEBUG;
	out_addr.sin_family = AF_INET;
	out_addr.sin_port = htons(1080);
	out_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }

    DEBUG;

    /* connect */
    _unhook(2);
    rc = WSAConnect(s, (struct sockaddr *) (&out_addr),
		    sizeof(struct sockaddr_in), lpCallerData,
		    lpCalleeData, lpSQOS, lpGQOS);
    err = WSAGetLastError();
#if defined(_DEBUG) || defined(_DEBUG_)
    TRACE_NO("connect (rc:%i err:%i pid:%i): %s\n", rc, err,
	     GetCurrentProcessId(), socketError());
#endif
    _hook(2);
    if (rc != 0 && err != WSAEWOULDBLOCK) {
	closesocket(s);
	DEBUG_LEAVE;
	return rc;
    }

    /* pass through local */
    if (in_addr.sin_addr.s_addr == inet_addr("127.0.0.1")) {
	DEBUG_LEAVE;
	return rc;
    }

    /* fd_set */
    FD_ZERO(&set);
    FD_SET(s, &set);
    /* sock5 handshake */
    select(s + 1, NULL, &set, NULL, 0);
    rc = send(s, "\x05\x01\x00", 3, 0);
#if defined(_DEBUG) || defined(_DEBUG_)
    TRACE_VERBOSE("send (rc:%i err:%i pid:%i): %s\n", rc,
		  WSAGetLastError(), GetCurrentProcessId(), socketError());
#endif
    if (rc != 3) {
	closesocket(s);
	DEBUG_LEAVE;
	return SOCKET_ERROR;
    }

    /* socks5 ack */
    select(s + 1, &set, NULL, NULL, 0);
    rc = recv(s, buf, 256, 0);
#if defined(_DEBUG) || defined(_DEBUG_)
    TRACE_VERBOSE("recv (rc:%i err:%i pid:%i): %s\n", rc,
		  WSAGetLastError(), GetCurrentProcessId(), socketError());
    DUMP(buf, rc);
#endif
    if (rc != 2) {
	closesocket(s);
	DEBUG_LEAVE;
	return SOCKET_ERROR;
    }

    /* socks5 request */
    memcpy(buf, "\x05\x01\x00\x01", 4);
    memcpy(buf + 4, &in_addr.sin_addr.s_addr, 4);
    memcpy(buf + 8, &in_addr.sin_port, 2);
    select(s + 1, NULL, &set, NULL, 0);
    rc = send(s, buf, 10, 0);
#if defined(_DEBUG) || defined(_DEBUG_)
    TRACE_VERBOSE("send (rc:%i err:%i pid:%i): %s\n", rc,
		  WSAGetLastError(), GetCurrentProcessId(), socketError());
#endif
    if (rc != 10) {
	closesocket(s);
	DEBUG_LEAVE;
	return SOCKET_ERROR;
    }

    /* socks5 ack */
    select(s + 1, &set, NULL, NULL, 0);
    rc = recv(s, buf, 256, 0);
#if defined(_DEBUG) || defined(_DEBUG_)
    TRACE_VERBOSE("recv (rc:%i err:%i pid:%i): %s\n", rc,
		  WSAGetLastError(), GetCurrentProcessId(), socketError());
    DUMP(buf, rc);
#endif
    if (rc != 10) {
	closesocket(s);
	DEBUG_LEAVE;
	return SOCKET_ERROR;
    }

    DEBUG_LEAVE;
    return 0;
}

BOOL WINAPI new_SetWindowText(HWND h, LPCTSTR text) {
    BOOL rc;
    DEBUG_ENTER;
#if defined(_DEBUG) || defined(_DEBUG_)
    TRACE_VERBOSE("org title: %s\n", text);
#endif
    /* update title */
    if (strstr((char *) text, TITLE_APPEND) == NULL)
	text = strcat((char *) text, TITLE_APPEND);
#if defined(_DEBUG) || defined(_DEBUG_)
    TRACE_VERBOSE("new title: %s\n", text);
#endif
    /* set window text */
    _unhook(3);
    rc = SetWindowText(h, text);
    _hook(3);
    DEBUG_LEAVE;
    return rc;
}
