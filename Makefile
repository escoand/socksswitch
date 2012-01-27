#
#     Copyright (C) 2011-2012 Andreas Schönfelder
#     https://github.com/escoand/socksswitch
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#


# default config
CC       ?= gcc
CCFLAGS   = -O3 -Wall -Isrc/ -Iinclude/
LDFLAGS   = -Llib/ -lssh
BINDIR    = bin
MAIN      = socksswitch
OBJS      = main.o match.o sockets.o socks.o trace.o


# windows config
ifeq ($(OS), Windows_NT)
CC        = gcc
LDFLAGS  += -lws2_32 -lpsapi
MAIN     := $(MAIN).exe
endif


OBJS     := $(addprefix $(BINDIR)/,$(OBJS))
MAIN     := $(addprefix $(BINDIR)/,$(MAIN))


debug: CCFLAGS += -g -D_DEBUG -D_DEBUG_
debug: all

static: LDFLAGS := -Llib/static/ $(LDFLAGS) -static -static-libgcc
static: all

mingw32: CC = i586-mingw32msvc-cc
mingw32: all


all: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) -o $(MAIN) $(OBJS) $(LDFLAGS)

$(BINDIR)/%.o: src/%.c
	$(CC) -o $@ -c $(CCFLAGS) $<

$(OBJS): | $(BINDIR)

$(BINDIR):
	mkdir $@

upx: $(MAIN)
	upx $(MAIN)

clean:
	$(RM) $(MAIN) $(OBJS)

