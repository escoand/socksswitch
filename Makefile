#
# Makefile
# forward socks5 connections to multiple socks5 proxies on basis of filter rules
# Copyright (C) 2011  Andreas Schönfelder
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
CXX=gcc
CXXFLAGS=-O3 -Wall -fmessage-length=0
LDFLAGS=-Llib -lssh2
MAIN=socksswitch

# windows config
ifeq ($(OS), Windows_NT)
LDFLAGS=-Llib -lssh2 -lws2_32 -lpsapi
SUFFIX=.exe
endif

all: $(MAIN)

$(MAIN):
	$(CXX) -orelease/$(MAIN)$(SUFFIX) $(CXXFLAGS) src/*.c $(LDFLAGS)

debug:
	$(CXX) -orelease/$(MAIN)$(SUFFIX) $(CXXFLAGS) -g -D _DEBUG src/*.c $(LDFLAGS)

static:
	$(CXX) -orelease/$(MAIN)$(SUFFIX) $(CXXFLAGS) -static-libgcc -D _DEBUG src/*.c $(LDFLAGS)

staticdebug:
	$(CXX) -orelease/$(MAIN)$(SUFFIX) $(CXXFLAGS) -static-libgcc -g -D _DEBUG src/*.c $(LDFLAGS)

upx:
	upx release/$(MAIN)$(SUFFIX)

clean:
	$(RM) release/*.o release/$(MAIN)$(SUFFIX)