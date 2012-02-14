
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

#define COUNT        2
#define SIZE         6
#define TITLE_APPEND " via socksswitch"

BYTE jmp[COUNT][SIZE];
BYTE bytes[COUNT][SIZE];
FARPROC addr[COUNT];
FARPROC addr_new[COUNT];

INT APIENTRY DllMain(HMODULE hDLL, DWORD Reason, LPVOID Reserved) {
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

    switch (Reason) {

	/* hook */
    case DLL_PROCESS_ATTACH:

	/* init */
	for (i = 0; i < COUNT; i++) {
	    memset(bytes[i], 0, SIZE);
	    memcpy(jmp[i], "\xE9\x90\x90\x90\x90\xC3", SIZE);
	}

	/* get function address */
	addr[0] = GetProcAddress(GetModuleHandle("ws2_32.dll"), "connect");
	addr[1] = GetProcAddress(GetModuleHandle("user32.dll"),
				 "SetWindowTextA");
	addr_new[0] = new_connect;
	addr_new[1] = new_SetWindowText;

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

	hook(0);

	/* window text */
	h = GetTopWindow(0);
	while (h) {
	    GetWindowThreadProcessId(h, (PDWORD) & pid);
	    if (pid == GetCurrentProcessId()) {
		GetWindowText(h, txt, sizeof(txt));
		SetWindowText(h, txt);
	    }
	    h = GetNextWindow(h, GW_HWNDNEXT);
	}

	break;

	/* unhook */
    case DLL_PROCESS_DETACH:
	unhook(0);
	break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
	break;
    }

    DEBUG_LEAVE;

    return TRUE;
}

void hook(int func) {
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

void unhook(int func) {
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

int WINAPI new_connect(SOCKET s, const struct sockaddr *addr, int namelen) {
    int rc, err;
    struct sockaddr_in in_addr;
    struct sockaddr_in out_addr;
    char buf[256];
    fd_set set;
    DEBUG_ENTER;

    /* fd_set */
    FD_ZERO(&set);
    FD_SET(s, &set);

    /* addr */
    memcpy(&in_addr, addr, sizeof(in_addr));
    out_addr.sin_family = AF_INET;
    out_addr.sin_port = htons(1081);
    out_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* connect */
    unhook(1);
    rc = connect(s, (struct sockaddr *) (&out_addr),
		 sizeof(struct sockaddr_in));
    err = WSAGetLastError();
#if defined(_DEBUG) || defined(_DEBUG_)
    TRACE_NO("connect (rc:%i err:%i pid:%i): %s\n", rc, err,
	     GetCurrentProcessId(), socketError());
#endif
    hook(1);
    if (rc != 0 && err != WSAEWOULDBLOCK) {
	closesocket(s);
	DEBUG_LEAVE;
	return rc;
    }

    /* sock5 handshake */
    select(s + 1, NULL, &set, NULL, 0);
    rc = send(s, "\x05\x01\x00", 3, 0);
#if defined(_DEBUG) || defined(_DEBUG_)
    err = WSAGetLastError();
    TRACE_NO("send (rc:%i err:%i pid:%i): %s\n", rc, err,
	     GetCurrentProcessId(), socketError());
#endif
    if (rc != 3) {
	closesocket(s);
	DEBUG_LEAVE;
	return -1;
    }

    /* socks5 ack */
    select(s + 1, &set, NULL, NULL, 0);
    rc = recv(s, buf, 256, 0);
#if defined(_DEBUG) || defined(_DEBUG_)
    err = WSAGetLastError();
    TRACE_NO("recv (rc:%i err:%i pid:%i): %s\n", rc, err,
	     GetCurrentProcessId(), socketError());
    DUMP(buf, rc);
#endif
    if (rc != 2) {
	closesocket(s);
	DEBUG_LEAVE;
	return -1;
    }

    /* socks5 request */
    memcpy(buf, "\x05\x01\x00\x01", 4);
    memcpy(buf + 4, &in_addr.sin_addr.s_addr, 4);
    memcpy(buf + 8, &in_addr.sin_port, 2);
    select(s + 1, NULL, &set, NULL, 0);
    rc = send(s, buf, 10, 0);
#if defined(_DEBUG) || defined(_DEBUG_)
    err = WSAGetLastError();
    TRACE_NO("send (rc:%i err:%i pid:%i): %s\n", rc, err,
	     GetCurrentProcessId(), socketError());
#endif
    if (rc != 10) {
	closesocket(s);
	DEBUG_LEAVE;
	return -1;
    }

    /* socks5 ack */
    select(s + 1, &set, NULL, NULL, 0);
    rc = recv(s, buf, 256, 0);
#if defined(_DEBUG) || defined(_DEBUG_)
    err = WSAGetLastError();
    TRACE_NO("recv (rc:%i err:%i pid:%i): %s\n", rc, err,
	     GetCurrentProcessId(), socketError());
    DUMP(buf, rc);
#endif
    if (rc != 10) {
	closesocket(s);
	DEBUG_LEAVE;
	return -1;
    }

    DEBUG_LEAVE;
    return 0;
}

BOOL WINAPI new_SetWindowText(HWND h, LPCTSTR text) {
    BOOL rc;
    char title[1024];

    DEBUG_ENTER;

    /* get title */
    strcpy(title, text);
    if (IsWindow(h) && strstr(text, TITLE_APPEND) == NULL)
	strcat(title, TITLE_APPEND);

    /* set window text */
    unhook(2);
    rc = SetWindowText(h, title);
    hook(2);

    DEBUG_LEAVE;

    return rc;
}
