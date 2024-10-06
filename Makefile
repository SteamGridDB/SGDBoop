ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

USER_LIB_PATH := /usr/lib
ifdef FLATPAK_ID
    USER_LIB_PATH := /app/lib
endif

PKG_CONFIG ?= pkg-config

.PHONY: all
all: build

.PHONY: build
build:
	$(CC) -g sgdboop.c curl-helper.c gui-helper-gtk.c string-helpers.c crc.c $(shell $(PKG_CONFIG) --cflags --libs gtk+-3.0 libcurl) -o linux-release/SGDBoop $(LDFLAGS)

ifdef FLATPAK_ID
.PHONY: install
install: build
	install -Dm644 linux-release/com.steamgriddb.SGDBoop.desktop -t /app/share/applications
	install -Dm644 com.steamgriddb.SGDBoop.appdata.xml -t /app/share/metainfo
	install -Dm755 linux-release/SGDBoop -t /app/bin
	install -Dm644 res/com.steamgriddb.SGDBoop.svg -t /app/share/icons/hicolor/scalable/apps
else
.PHONY: install
install: build
	install -Dm755 linux-release/SGDBoop -t $(DESTDIR)$(PREFIX)/bin
	desktop-file-install linux-release/com.steamgriddb.SGDBoop.desktop --rebuild-mime-info-cache
	@su - $(SUDO_USER)
	xdg-mime default com.steamgriddb.SGDBoop.desktop x-scheme-handler/sgdb
endif
