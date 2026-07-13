ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

USER_LIB_PATH := /usr/lib
ifdef FLATPAK_ID
USER_LIB_PATH := /app/lib
endif

PKG_CONFIG ?= pkg-config

STATIC_DIR := linux-release
OUTPUT_DIR ?= linux-release

.PHONY: all
all: build

.PHONY: build
build:
	$(CC) -O3 -g sgdboop.c curl-helper.c gui-helper-gtk.c string-helpers.c crc.c $(shell $(PKG_CONFIG) --cflags --libs gtk+-3.0 libcurl) -o $(OUTPUT_DIR)/SGDBoop $(LDFLAGS)

.PHONY: build-mac
build-mac:
	mkdir -p SGDBoop.app/Contents/MacOS
	mkdir -p SGDBoop.app/Contents/Resources
	cp res/mac/AppIcon.icns SGDBoop.app/Contents/Resources/AppIcon.icns
	cp res/mac/Info.plist SGDBoop.app/Contents/Info.plist
	$(CC) -Wno-pointer-sign -fobjc-arc -arch arm64 -arch x86_64 -g sgdboop.c curl-helper.c gui-helper-mac.m string-helpers.c crc.c $(shell $(PKG_CONFIG) --cflags --libs libcurl) -framework Cocoa -o SGDBoop.app/Contents/MacOS/SGDBoop $(LDFLAGS)

ifdef FLATPAK_ID
.PHONY: install
install: build
	install -Dm644 $(STATIC_DIR)/com.steamgriddb.SGDBoop-flatpak.desktop /app/share/applications/com.steamgriddb.SGDBoop.desktop
	install -Dm644 com.steamgriddb.SGDBoop.appdata.xml -t /app/share/metainfo
	install -Dm755 $(OUTPUT_DIR)/SGDBoop -t /app/bin
	install -Dm644 res/com.steamgriddb.SGDBoop.svg -t /app/share/icons/hicolor/scalable/apps
else
.PHONY: install
install: build
	install -Dm755 $(OUTPUT_DIR)/SGDBoop -t $(DESTDIR)$(PREFIX)/bin
	desktop-file-install $(STATIC_DIR)/com.steamgriddb.SGDBoop.desktop --rebuild-mime-info-cache
	@su - $(SUDO_USER)
	xdg-mime default com.steamgriddb.SGDBoop.desktop x-scheme-handler/sgdb
endif