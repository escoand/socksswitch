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
BUILDDIR  = build
BINDIR    = bin
INSTDIR   = $(ProgramFiles)/socksswitch/
MAIN      = socksswitch
DRV       = socksswitchdrv
OBJS      = destination.o inject.o main.o request.o sockets.o ssh.o trace.o


# windows config
ifeq ($(OS), Windows_NT)
CC        = gcc
LDFLAGS  += -lws2_32 -lpsapi
MAIN     := $(MAIN).exe
DRV      := $(DRV).dll
endif


OBJS     := $(addprefix $(BUILDDIR)/,$(OBJS))
MAIN     := $(addprefix $(BINDIR)/,$(MAIN))
DRV      := $(addprefix $(BINDIR)/,$(DRV))


all: $(MAIN) $(DRV)
drv: $(DRV)

debug: CCFLAGS += -g3 -D_DEBUG -D_DEBUG_
debug: all

static: CCFLAGS += -DLIBSSH_STATIC
static: LDFLAGS := -Llib/static/ $(LDFLAGS) -lzlibstat -llibeay32 -lssleay32 -static
static: $(OBJS) all

mingw32: CC = i586-mingw32msvc-cc
mingw32: all


$(MAIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(DRV): $(BUILDDIR)/drv.o $(BUILDDIR)/sockets.o $(BUILDDIR)/trace.o
	$(CC) -o $@ $^ $(LDFLAGS) -shared

$(BUILDDIR)/%.o: src/%.c
	$(CC) -o $@ -c $(CCFLAGS) $<

$(OBJS): | $(BUILDDIR) $(BINDIR)

$(BUILDDIR):
	mkdir "$@"
$(BINDIR):
	mkdir "$@"
$(INSTDIR):
	mkdir "$@"

install: $(MAIN) $(DRV)
	cp $(MAIN) "$(INSTDIR)"
	cp $(DRV) "$(INSTDIR)"

upx: $(MAIN) $(DRV)
	upx $^

clean:
	$(RM) $(MAIN) $(DRV) $(OBJS)
