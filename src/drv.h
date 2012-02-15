
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
#include <winsock2.h>

void hook();
void unhook();

int WINAPI new_connect(SOCKET, const struct sockaddr *, int);
int WINAPI new_WSAConnect(SOCKET,
			  const struct sockaddr *,
			  int, LPWSABUF, LPWSABUF, LPQOS, LPQOS);
BOOL WINAPI new_SetWindowText(HWND, LPCTSTR);
