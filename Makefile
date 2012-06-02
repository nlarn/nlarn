#
# Makefile
# Copyright (C) 2009, 2010, 2011 Joachim de Groot <jdegroot@web.de>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be usefuse along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
#

.PHONY: help clean dist

ifndef config
  config=debug
endif

OS   := $(shell uname -s)
ARCH := $(shell uname -m)

# check if operating on a git checkout
ifneq ($(wildcard .git),)
  # get hash of current commit
  GITREV := $(shell git show --pretty=oneline | sed -ne '1 s/\(.\{6\}\).*/\1/p')
  # get tag for current commit
  GITTAG := $(shell git tag --contains $(GITREV) | tr '[:upper:]' '[:lower:]')
  ifeq ($(GITTAG),)
    # current commit is not tagged
    # -> include current commit hash in version information
    DEFINES += -DGITREV=\"-g$(GITREV)\"
  endif
endif

VERSION_MAJOR := $(shell awk '/VERSION_MAJOR/ { print $$3 }' inc/nlarn.h)
VERSION_MINOR := $(shell awk '/VERSION_MINOR/ { print $$3 }' inc/nlarn.h)
VERSION_PATCH := $(shell awk '/VERSION_PATCH/ { print $$3 }' inc/nlarn.h)

# Collect version information
ifneq ($(GITTAG),)
  VERSION = $(GITTAG)
else
  # not on a release tag, determine version manually
  VERSION = $(VERSION_MAJOR).$(VERSION_MINOR)

  ifneq ($(VERSION_PATCH),0)
	VERSION := $(VERSION).$(VERSION_PATCH)
  endif

  ifneq ($(GITREV),)
    date     = $(shell date "+%Y%m%d")
    VERSION := $(VERSION)-$(date)-g$(GITREV)
  endif
endif

# Definitions required regardless of host OS
DEFINES += -DG_DISABLE_DEPRECATED
CFLAGS  += -MMD -MP -std=c99 -Wextra -Iinc
LDFLAGS += -lz

# Make sure warnings do not pass unoticed.
# Make an exception for OpenBSD as the libc headers generate some
# bogus warings there when using strlen and similar functions.
ifneq ($(OS),OpenBSD)
  CFLAGS += -Werror
endif

ifeq ($(MSYSTEM),MINGW32)
  # Settings specific to Windows.

  # Fake the content of the OS var to make it more common
  # (otherwise packages would have silly names)
  OS := win32

  # Ensure make doesn't try to run cc
  CC = gcc

  RESOURCES := $(patsubst %.rc,%.res,$(wildcard resources/*.rc))
  RESDEFINE := -DVERSION_MAJOR=$(VERSION_MAJOR)
  RESDEFINE += -DVERSION_MINOR=$(VERSION_MINOR)
  RESDEFINE += -DVERSION_PATCH=$(VERSION_PATCH)
  # Escape-O-Rama! Required in all it's ugliness.
  RESDEFINE += -DVINFO=\\\"$(VERSION)\\\"

  # Libraries specific to Windows
  LDFLAGS += -static -lpdcurses -llua

  # Configuration for glib-2
  # Funny enough, build breaks if these are set as ususal..
  CFLAGS  += `pkg-config --cflags glib-2.0`
  LDFLAGS += `pkg-config --libs glib-2.0`

  # Defines specific to Windows
  DEFINES += -DWIN32_LEAN_AND_MEAN -DNOGDI

  # and finally the dreaded executable suffix from the eighties
  SUFFIX = .exe
  ARCHIVE_CMD = zip -r
  ARCHIVE_SUFFIX = zip
else
  # Settings specific to Un*x-like operating systems

  # Use clang on OS X.
  ifeq ($(OS), Darwin)
    CC = clang
  endif

  # Configuration for glib-2
  CFLAGS  += $(shell pkg-config --cflags glib-2.0)
  LDFLAGS += $(shell pkg-config --libs glib-2.0)

  # Configuration for ncurses
  ifeq ($(filter Darwin DragonFly OpenBSD,$(OS)),)
    CFLAGS  += $(shell ncurses5-config --cflags)
    LDFLAGS += $(shell ncurses5-config --libs) -lpanel
  else
    # 1) there is no ncurses5-config on OS X and
    #    the available ncurses5.4-config returns garbage
    # 2) DragonFly has ncurses in base
    # 3) as has OpenBSD
    ifneq ($(filter -DSDLPDCURSES,$(DEFINES)),)
	  # build with SDL PDCurses on OS X
      CFLAGS  += $(shell sdl-config --cflags) -Dmain=SDL_main
      LDFLAGS += -lpdcurses $(shell sdl-config --static-libs)
    else
      LDFLAGS += -lncurses -lpanel
    endif
  endif

  # Determine the name of the Lua 5.1 library
  # Debian and derivates use lua5.1, the rest of the world lua
  ifneq ($(wildcard /etc/debian_version),)
    lua = lua5.1
  else
    lua = lua
  endif

  # Configure Lua
  CFLAGS  += $(shell pkg-config --cflags $(lua))
  LDFLAGS += $(shell pkg-config --libs $(lua))

  # executables on other plattforms do not have a funny suffix
  SUFFIX =
  ARCHIVE_CMD = tar czf
  ARCHIVE_SUFFIX = tar.gz
endif

# Enable creating packages when working on a git checkout
ifneq ($(GITREV),)
  DIRNAME   = nlarn-$(VERSION)
  SRCPKG    = nlarn-$(VERSION).tar.gz
  PACKAGE   = $(DIRNAME)_$(OS).$(ARCH).$(ARCHIVE_SUFFIX)
  MAINFILES = nlarn$(SUFFIX) nlarn.ini-sample README.txt LICENSE
  LIBFILES  = lib/fortune lib/maze lib/maze_doc.txt lib/nlarn.* lib/*.lua
endif

ifeq ($(MSYSTEM),MINGW32)
  INSTALLER := nlarn-$(VERSION).exe
endif

ifeq ($(config),debug)
  DEFINES   += -DDEBUG -DLUA_USE_APICHECK
  CFLAGS    += $(DEFINES) -g
  RESFLAGS  += $(DEFINES) $(INCLUDES)
endif

ifeq ($(config),release)
  DEFINES   += -DG_DISABLE_ASSERT
  CFLAGS    += $(DEFINES) -O2
  RESFLAGS  += $(DEFINES) $(INCLUDES)
endif

OBJECTS := $(patsubst %.c,%.o,$(wildcard src/*.c))

all: nlarn$(SUFFIX)

nlarn$(SUFFIX): $(OBJECTS) $(RESOURCES)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(RESOURCES)

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(RESOURCES): %.res: %.rc
	windres -v $(RESDEFINE) $< -O coff -o $@

dist: clean $(SRCPKG) $(PACKAGE) $(INSTALLER)

$(SRCPKG):
	@echo -n Packing source archive $(SRCPKG)
	@git archive --prefix $(DIRNAME)/ --format=tar $(GITREV) | gzip > $(SRCPKG)
	@echo " - done."

$(PACKAGE): nlarn$(SUFFIX)
	@echo -n Packing $(PACKAGE)
	@mkdir -p $(DIRNAME)/lib
	@cp -p $(MAINFILES) $(DIRNAME)
	@cp -p $(LIBFILES) $(DIRNAME)/lib
	@$(ARCHIVE_CMD) $(PACKAGE) $(DIRNAME)
	@rm -rf $(DIRNAME)
	@echo " - done."

$(INSTALLER): nlarn$(SUFFIX) nlarn.nsi
	@echo -n Packing $(PACKAGE)
	@makensis //DVERSION="$(VERSION)" \
		//DVERSION_MAJOR=$(VERSION_MAJOR) \
		//DVERSION_MINOR=$(VERSION_MINOR) \
		//DVERSION_PATCH=$(VERSION_PATCH) nlarn.nsi
	@echo " - done."

clean:
	@echo Cleaning nlarn
	rm -f nlarn
	rm -f $(RESOURCES) $(OBJECTS) $(patsubst %.o,%.d,$(OBJECTS))
	rm -f $(SRCPKG) $(PACKAGE) $(INSTALLER)

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "   debug"
	@echo "   release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default) - builds nlarn$(SUFFIX)"
	@echo "   clean         - cleans the working directory"
	@if \[ -n "$(GITREV)" \]; then \
		echo "   dist          - create source and binary packages for distribution"; \
		echo "                   ($(SRCPKG) and $(PACKAGE))"; \
	fi
	@echo ""

-include $(OBJECTS:%.o=%.d)
