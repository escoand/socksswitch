
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

#define SIZE 6
#define WINDOW_TITLE_APPEND " via socksswitch"

DWORD org_protect;

BYTE jmp_connect[SIZE] = { 0xE9, 0x90, 0x90, 0x90, 0x90, 0xC3 };
BYTE jmp_SetWindowText[SIZE] = { 0xE9, 0x90, 0x90, 0x90, 0x90, 0xC3 };

BYTE org_bytes_connect[SIZE] = { 0 };
BYTE org_bytes_SetWindowText[SIZE] = { 0 };

typedef int (WINAPI * type_connect) (SOCKET, const struct sockaddr *, int);
typedef BOOL(WINAPI * type_SetWindowText) (HWND, LPCTSTR);

type_connect addr_connect = NULL;
type_SetWindowText addr_SetWindowText = NULL;

INT APIENTRY DllMain(HMODULE hDLL, DWORD Reason, LPVOID Reserved) {
    HWND h;
    int pid;
    char txt[1024] = "";

#if defined(_DEBUG) || defined(_DEBUG_)
    putenv("LOGFILE=c:\\socksswitchdrv.log");
    putenv("TRACE=4");
    putenv("DUMP=1");
#endif

    switch (Reason) {

	/* hook */
    case DLL_PROCESS_ATTACH:

	/* get address of connect */
	addr_connect =
	    (type_connect) GetProcAddress(GetModuleHandle("ws2_32.dll"),
					  "connect");

	/* save bytes */
	if (addr_connect != NULL) {
	    VirtualProtect((LPVOID) addr_connect, SIZE,
			   PAGE_EXECUTE_READWRITE, &org_protect);
	    memcpy(org_bytes_connect, addr_connect, SIZE);
	    VirtualProtect((LPVOID) addr_connect, SIZE, org_protect, NULL);
	}

	/* get address of SetWindowText */
	addr_SetWindowText = (type_SetWindowText)
	    GetProcAddress(GetModuleHandle("user32.dll"),
			   "SetWindowTextA");

	/* save bytes */
	if (addr_SetWindowText != NULL) {
	    VirtualProtect((LPVOID) addr_SetWindowText, SIZE,
			   PAGE_EXECUTE_READWRITE, &org_protect);
	    memcpy(org_bytes_SetWindowText, addr_SetWindowText, SIZE);
	    VirtualProtect((LPVOID) addr_SetWindowText, SIZE, org_protect,
			   NULL);
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

    return TRUE;
}

void hook(int func) {
    DWORD jmp_size;

    if (addr_connect != NULL && (func == 0 || func == 1)) {
	VirtualProtect((LPVOID) addr_connect, SIZE,
		       PAGE_EXECUTE_READWRITE, &org_protect);
	jmp_size = (DWORD) new_connect - (DWORD) addr_connect - 5;
	memcpy(&jmp_connect[1], &jmp_size, 4);
	memcpy(addr_connect, jmp_connect, SIZE);
	VirtualProtect((LPVOID) addr_connect, SIZE, org_protect, NULL);
    }

    if (addr_SetWindowText != NULL && (func == 0 || func == 2)) {
	VirtualProtect((LPVOID) addr_SetWindowText, SIZE,
		       PAGE_EXECUTE_READWRITE, &org_protect);
	jmp_size =
	    (DWORD) new_SetWindowText - (DWORD) addr_SetWindowText - 5;
	memcpy(&jmp_SetWindowText[1], &jmp_size, 4);
	memcpy(addr_SetWindowText, jmp_SetWindowText, SIZE);
	VirtualProtect((LPVOID) addr_SetWindowText, SIZE, org_protect,
		       NULL);
    }
}

void unhook(int func) {
    if (addr_connect != NULL && (func == 0 || func == 1)) {
	VirtualProtect((LPVOID) addr_connect, SIZE,
		       PAGE_EXECUTE_READWRITE, &org_protect);
	memcpy(addr_connect, org_bytes_connect, SIZE);
	VirtualProtect((LPVOID) addr_connect, SIZE, org_protect, NULL);
    }

    if (addr_SetWindowText != NULL && (func == 0 || func == 2)) {
	VirtualProtect((LPVOID) addr_SetWindowText, SIZE,
		       PAGE_EXECUTE_READWRITE, &org_protect);
	memcpy(addr_SetWindowText, org_bytes_SetWindowText, SIZE);
	VirtualProtect((LPVOID) addr_SetWindowText, SIZE, org_protect,
		       NULL);
    }
}

int WINAPI new_connect(SOCKET s, const struct sockaddr *addr, int namelen) {
    int rc, err;
    struct sockaddr_in in_addr;
    struct sockaddr_in out_addr;
    char buf[256];
    fd_set set;

#if defined(_DEBUG) || defined(_DEBUG_)
    DEBUG_ENTER;
#endif

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
#if defined(_DEBUG) || defined(_DEBUG_)
	DEBUG_LEAVE;
#endif
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
#if defined(_DEBUG) || defined(_DEBUG_)
	DEBUG_LEAVE;
#endif
	return -1;
    }
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
#if defined(_DEBUG) || defined(_DEBUG_)
	DEBUG_LEAVE;
#endif
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
#if defined(_DEBUG) || defined(_DEBUG_)
	DEBUG_LEAVE;
#endif
	return -1;
    }
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
#if defined(_DEBUG) || defined(_DEBUG_)
	DEBUG_LEAVE;
#endif
	return -1;
    }

#if defined(_DEBUG) || defined(_DEBUG_)
	DEBUG_LEAVE;
#endif

    return 0;
}

BOOL WINAPI new_SetWindowText(HWND h, LPCTSTR text) {
    BOOL rc;
    char title[1024];

    /* get title */
    strcpy(title, text);
    if (IsWindow(h) && strstr(text, WINDOW_TITLE_APPEND) == NULL)
	strcat(title, WINDOW_TITLE_APPEND);

    /* set window text */
    unhook(2);
    rc = SetWindowText(h, title);
    hook(2);

    return rc;
}
