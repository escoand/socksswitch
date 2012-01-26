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


#ifndef SOCKS_H
#define SOCKS_H


typedef struct
{
  unsigned char ver;
  unsigned char cmd;
  union
  {
    struct
    {
      unsigned short port;
      unsigned char ip[4];
      unsigned char method;
    } socks4;
    struct
    {
      unsigned char rsv;
      unsigned char atyp;
      union
      {
	struct
	{
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

void getSocksReqHost (char *host, const char *buf, const int len);
unsigned short getSocksReqPort (const char *buf, const int len);

#endif /* SOCKS_H */
