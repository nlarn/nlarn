#
# Makefile
# Copyright (C) 2009-2018 Joachim de Groot <jdegroot@web.de>
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
  GITREV := $(shell git log -n1 --format="%h")
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
  # Remove "nlarn-" from the tag name as it will be prepended later.
  VERSION := $(patsubst nlarn-%,%,$(GITTAG))
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
CFLAGS  += -std=c99 -Wall -Wextra -Iinc -Iinc/external
LDFLAGS += -lz -lm

ifneq (,$(findstring MINGW, $(MSYSTEM)))
  # Settings specific to Windows.
  ifneq (Y, $(SDLPDCURSES))
    $(error Building without SDLPDCURSES is not supported on Windows.)
  endif

  DLLS := libbz2-1.dll libfreetype-6.dll libgcc_s_dw2-1.dll libglib-2.0-0.dll
  DLLS += libgraphite2.dll libharfbuzz-0.dll libiconv-2.dll libintl-8.dll
  DLLS += libpcre-1.dll libpng16-16.dll libstdc++-6.dll libwinpthread-1.dll
  DLLS += lua53.dll SDL.dll SDL_ttf.dll zlib1.dll
  LIBFILES := lib/nlarn-128.bmp

  # Fake the content of the OS var to make it more common
  # (otherwise packages would have silly names)
  OS := win32

  RESOURCES := $(patsubst %.rc,%.res,$(wildcard resources/*.rc))
  RESDEFINE := -DVERSION_MAJOR=$(VERSION_MAJOR)
  RESDEFINE += -DVERSION_MINOR=$(VERSION_MINOR)
  RESDEFINE += -DVERSION_PATCH=$(VERSION_PATCH)
  # Escape-O-Rama! Required in all it's ugliness.
  RESDEFINE += -DVINFO=\\\"$(VERSION)\\\"

  # and finally the dreaded executable suffix from the eighties
  SUFFIX = .exe
  ARCHIVE_CMD = zip -r
  ARCHIVE_SUFFIX = zip
  INSTALLER := nlarn-$(VERSION).exe
else
  # executables on other plattforms do not have a funny suffix
  SUFFIX =
  ARCHIVE_CMD = tar czf
  ARCHIVE_SUFFIX = tar.gz
endif

# Configuration for glib-2
CFLAGS  += $(shell pkg-config --cflags glib-2.0)
LDFLAGS += $(shell pkg-config --libs glib-2.0)

# Determine the name of the Lua 5.3 library
# Debian and derivates use lua5.3, the rest of the world lua
ifneq ($(wildcard /etc/debian_version),)
  lua = lua5.3
else ifneq ($(filter $(OS), FreeBSD NetBSD),)
  lua = lua-5.3
else
  lua = lua
endif

# Configure Lua
CFLAGS  += $(shell pkg-config --cflags $(lua))
LDFLAGS += $(shell pkg-config --libs $(lua))

# Unless requested otherwise build with curses.
ifneq ($(SDLPDCURSES),Y)
	LDFLAGS += -lcurses -lpanel
else
	PDCLIB   := PDCurses/sdl1/libpdcurses.a
	CFLAGS   += $(shell pkg-config --cflags SDL_ttf) -IPDCurses -DSDLPDCURSES
	LDFLAGS  += $(shell pkg-config --libs SDL_ttf )
	LIBFILES += lib/FiraMono-Medium.otf
endif

# System-wide installation on *nix
ifeq ($(SETGID),Y)
	CFLAGS += -DSETGID
endif

# Enable creating packages when working on a git checkout
ifneq ($(GITREV),)
  DIRNAME   = nlarn-$(VERSION)
  SRCPKG    = nlarn-$(VERSION).tar.gz
  PACKAGE   = $(DIRNAME)_$(OS).$(ARCH).$(ARCHIVE_SUFFIX)
  MAINFILES = nlarn$(SUFFIX) nlarn.ini-sample README.html LICENSE Changelog.html
  LIBFILES += lib/fortune lib/maze lib/maze_doc.txt lib/nlarn.* lib/*.lua
endif

ifeq ($(OS),Darwin)
  OSXIMAGE := nlarn-$(VERSION).dmg
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
OBJECTS += $(patsubst %.c,%.o,$(wildcard src/wrappers/*.c))
OBJECTS += $(patsubst %.c,%.o,$(wildcard src/external/*.c))

INCLUDES := $(wildcard inc/*.h)
INCLUDES += $(wildcard inc/external/*.h)

all: nlarn$(SUFFIX)

nlarn$(SUFFIX): $(PDCLIB) $(OBJECTS) $(RESOURCES)
	$(CC) -o $@ $(OBJECTS) $(PDCLIB) $(LDFLAGS) $(RESOURCES)

%.o: %.c ${INCLUDES}
	$(CC) $(CFLAGS) -o $@ -c $<

%.html: %.md
	makepage $< > $@

.SECONDEXPANSION:
$(DLLS): $$(patsubst %, /mingw32/bin/%, $$@)
	cp $< $@

$(RESOURCES): %.res: %.rc
	windres -v $(RESDEFINE) $< -O coff -o $@

$(PDCLIB):
	@git submodule init
	@git submodule update --recommend-shallow
	$(MAKE) -C PDCurses/sdl1 WIDE=Y UTF8=Y libs

dist: clean $(SRCPKG) $(PACKAGE) $(INSTALLER) $(OSXIMAGE)

$(SRCPKG):
	@echo -n Packing source archive $(SRCPKG)
	@git archive --prefix $(DIRNAME)/ --format=tar $(GITREV) | gzip > $(SRCPKG)
	@echo " - done."

$(PACKAGE): $(MAINFILES) $(DLLS)
	@echo -n Packing $(PACKAGE)
	@mkdir -p $(DIRNAME)/lib
	@cp -p $(MAINFILES) $(DLLS) $(DIRNAME)
	@cp -p $(LIBFILES) $(DIRNAME)/lib
	@$(ARCHIVE_CMD) $(PACKAGE) $(DIRNAME)
	@rm -rf $(DIRNAME)
	@echo " - done."

mainfiles.nsh:
	@touch $@
	@for FILE in $(MAINFILES) $(DLLS) ; \
	do echo "  File \"$$FILE\"" >> $@; \
	done

libfiles.nsh:
	@touch $@
	@for FILE in $(LIBFILES) ; \
	do echo "  File \"$$FILE\"" | sed -e 's|/|\\|' >> $@; \
	done

# The Windows installer
$(INSTALLER): $(MAINFILES) $(DLLS) mainfiles.nsh libfiles.nsh nlarn.nsi
	@echo -n Packing $(PACKAGE)
	@makensis //DVERSION="$(VERSION)" \
		//DVERSION_MAJOR=$(VERSION_MAJOR) \
		//DVERSION_MINOR=$(VERSION_MINOR) \
		//DVERSION_PATCH=$(VERSION_PATCH) nlarn.nsi
	@echo " - done."
	@rm mainfiles.nsh libfiles.nsh

# The OSX installer
$(OSXIMAGE): $(MAINFILES)
	@mkdir -p dmgroot/NLarn.app/Contents/{Frameworks,MacOS,Resources}
	@cp -p nlarn dmgroot/NLarn.app/Contents/MacOS
# Copy local libraries into the app folder and instruct the linker.
	@for lib in $$(otool -L nlarn | awk '/local/ {print $$1}'); do\
		cp $$lib dmgroot/NLarn.app/Contents/MacOS/; \
		chmod 0644 dmgroot/NLarn.app/Contents/MacOS/$${lib##*/}; \
		install_name_tool -change $$lib @executable_path/$${lib##*/} \
			dmgroot/NLarn.app/Contents/MacOS/nlarn; \
		for llib in dmgroot/NLarn.app/Contents/MacOS/*.dylib; do\
			for libpath in $$(otool -L $$llib | awk '/local/ {print $$1}'); do\
				install_name_tool -change $$libpath \
					@executable_path/$${libpath##*/} $$llib; \
            done; \
		done; \
	done
# Copy required files
	@cp -p $(MAINFILES) dmgroot
	@cp -p $(LIBFILES) dmgroot/Nlarn.app/Contents/Resources
	@cp -p resources/NLarn.icns dmgroot/NLarn.app/Contents/Resources
	@cp -p resources/Info.plist dmgroot/NLarn.app/Contents
# Update the version information in the plist
	@/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $(VERSION)" \
		dmgroot/NLarn.app/Contents/Info.plist
	@/usr/libexec/PlistBuddy -c \
		"Add :CFBundleShortVersionString string $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)" \
		dmgroot/NLarn.app/Contents/Info.plist
# Use the same icons for the dmg file
	@cp -p resources/NLarn.icns dmgroot/.VolumeIcon.icns
	@SetFile -c icnC dmgroot/.VolumeIcon.icns
# Create a pseudo-installer disk image
	@mkdir dmgroot/.background
	@cp resources/dmg_background.png dmgroot/.background
	@echo hdiutil requires superuser rights.
	@hdiutil create -srcfolder dmgroot -volname "NLarn $(VERSION)" \
		-uid 99 -gid 99 -format UDRW -ov "raw-$(DIRNAME).dmg" || rm -rf dmgroot
	@rm -rf dmgroot
	@hdiutil attach -noautoopen -readwrite "raw-$(DIRNAME).dmg"
	@SetFile -a C "/Volumes/NLarn $(VERSION)"
	@sed -e 's/##VOLNAME##/NLarn $(VERSION)/' resources/dmg_settings.scpt | osascript
	@hdiutil detach "/Volumes/NLarn $(VERSION)"
	@hdiutil convert "raw-$(DIRNAME).dmg" -format UDZO -o "$(DIRNAME).dmg"
	@rm "raw-$(DIRNAME).dmg"

clean:
	@echo Cleaning nlarn
	rm -f $(OBJECTS) $(DLLS)
	rm -f nlarn$(SUFFIX) $(RESOURCES) $(SRCPKG) $(PACKAGE) $(INSTALLER) $(OSXIMAGE) README.html
	@if \[ -n "$(PDCLIB)" \]; then \
		$(MAKE) -C PDCurses/sdl1 clean; \
	fi

help:
	@echo "Usage: make [config=name] [options] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "   debug"
	@echo "   release"
	@echo ""
	@echo "OPTIONS:"
	@echo "   SDLPDCURSES=Y - compile with PDCurses for SDL (instead of ncurses)"
	@echo "   SETGID=Y      - compile for system-wide installations on *nix platforms"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default) - builds nlarn$(SUFFIX)"
	@echo "   clean         - cleans the working directory"
	@if \[ -n "$(GITREV)" \]; then \
		echo "   dist          - create source and binary packages for distribution"; \
		echo "                   ($(SRCPKG) and"; \
		echo "                   $(PACKAGE))"; \
	fi
	@echo ""
