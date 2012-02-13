
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
#include "drv.h"

#define SIZE 6

BYTE JMP[SIZE] = { 0xE9, 0x90, 0x90, 0x90, 0x90, 0xC3 };
BYTE org_bytes[SIZE] = { 0 };

DWORD org_protect;
org_connect org_address = NULL;

INT APIENTRY DllMain(HMODULE hDLL, DWORD Reason, LPVOID Reserved) {
    HWND h;
    int pid;
    char txt[1024] = "";

    switch (Reason) {

	/* hook */
    case DLL_PROCESS_ATTACH:

	org_address = (org_connect)
	    GetProcAddress(GetModuleHandle("ws2_32.dll"), "connect");

	if (org_address != NULL) {

	    /* get access */
	    VirtualProtect((LPVOID) org_address, SIZE,
			   PAGE_EXECUTE_READWRITE, &org_protect);

	    /* save bytes */
	    memcpy(org_bytes, org_address, SIZE);

	    /* get jmp */
	    DWORD JMPSize = (DWORD) new_connect - (DWORD) org_address - 5;
	    memcpy(&JMP[1], &JMPSize, 4);

	    /* hook */
	    memcpy(org_address, JMP, SIZE);
	    VirtualProtect((LPVOID) org_address, SIZE, org_protect, NULL);

	    /* window text */
	    h = GetTopWindow(0);
	    while (h) {
		GetWindowThreadProcessId(h, (PDWORD) & pid);
		if (pid == GetCurrentProcessId()) {
		    GetWindowText(h, txt, sizeof(txt));
		    strcat(txt, " via socksswitch");
		    SetWindowText(h, txt);
		}
		h = GetNextWindow(h, GW_HWNDNEXT);
	    }
	}
	break;

	/* unhook */
    case DLL_PROCESS_DETACH:
	memcpy(org_address, org_bytes, SIZE);

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
	break;
    }
    return TRUE;
}

int WINAPI new_connect(SOCKET s, const struct sockaddr *addr, int namelen) {
    int rc, err;
    struct sockaddr_in in_addr;
    struct sockaddr_in out_addr;
    char buf[256];
    fd_set set;

    /* fd_set */
    FD_ZERO(&set);
    FD_SET(s, &set);

    /* addr */
    memcpy(&in_addr, addr, sizeof(in_addr));
    out_addr.sin_family = AF_INET;
    out_addr.sin_port = htons(1080);
    out_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* unhook */
    VirtualProtect((void *) org_address, SIZE, PAGE_EXECUTE_READWRITE,
		   NULL);
    memcpy(org_address, org_bytes, SIZE);

    /* connect */
    rc = connect(s, (struct sockaddr *) (&out_addr),
		 sizeof(struct sockaddr_in));
    err = WSAGetLastError();

    /* rehook */
    memcpy(org_address, JMP, SIZE);
    VirtualProtect((LPVOID) org_address, SIZE, org_protect, NULL);

    if (rc != 0 && err != WSAEWOULDBLOCK) {
	closesocket(s);
	return rc;
    }

    /* sock5 handshake */
    select(s + 1, NULL, &set, NULL, 0);
    rc = send(s, "\x05\x01\x00", 3, 0);
    if (rc != 3) {
	closesocket(s);
	return -1;
    }
    select(s + 1, &set, NULL, NULL, 0);
    rc = recv(s, buf, 256, 0);
    if (rc != 2) {
	closesocket(s);
	return -1;
    }

    /* socks5 request */
    memcpy(buf, "\x05\x01\x00\x01", 4);
    memcpy(buf + 4, &in_addr.sin_addr.s_addr, 4);
    memcpy(buf + 8, &in_addr.sin_port, 2);
    select(s + 1, NULL, &set, NULL, 0);
    rc = send(s, buf, 10, 0);
    if (rc != 10) {
	closesocket(s);
	return -1;
    }
    select(s + 1, &set, NULL, NULL, 0);
    rc = recv(s, buf, 256, 0);
    if (rc != 10) {
	closesocket(s);
	return -1;
    }

    return 0;
}
