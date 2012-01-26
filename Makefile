#
# Makefile
# forward socks5 connections to multiple socks5 proxies on basis of filter rules
# Copyright (C) 2011-2012  Andreas Schönfelder
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# default config
CC		:= gcc
CCFLAGS		:= -O3 -Wall
LDFLAGS		:= -lssh2
DEBUGFLAGS	:= -g -D _DEBUG
STATICFLAGS	:= -static-libgcc
BUILDDIR	:= build/
MAIN		:= socksswitch
OBJS		:= $(BUILDDIR)main.o $(BUILDDIR)match.o $(BUILDDIR)sockets.o $(BUILDDIR)socks.o $(BUILDDIR)trace.o

# windows config
ifeq ($(OS), Windows_NT)
LDFLAGS		:= $(LDFLAGS) -lws2_32 -lpsapi
MAIN		:= $(MAIN).exe
endif

all: $(MAIN)
	
$(MAIN): $(OBJS)
	$(CC) -o $(BUILDDIR)$(MAIN) $(OBJS) $(LDFLAGS)

$(BUILDDIR)%.o: src/%.c
	$(CC) -o $@ -c $(CCFLAGS) $<

debug: $(OBJS)
	$(CC) -o $(BUILDDIR)$(MAIN) $(CCFLAGS) $(DEBUGFLAGS) $(OBJS) $(LDFLAGS)

static: $(OBJS)
	$(CC) -o $(BUILDDIR)$(MAIN) $(CCFLAGS) $(OBJS) $(LDFLAGS) $(STATICFLAGS)

staticdebug: $(OBJS)
	$(CC) -o $(BUILDDIR)$(MAIN) $(CCFLAGS) $(DEBUGFLAGS) $(OBJS) $(LDFLAGS) $(STATICFLAGS)

upx: $(MAIN)
	upx $(BUILDDIR)$(MAIN)

clean:
	$(RM) $(BUILDDIR)*.o $(BUILDDIR)$(MAIN)