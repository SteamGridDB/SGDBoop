ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: all
all: build

.PHONY: build
build:
	gcc sgdboop.c curl-helper.c string-helpers.c lib/linux/*.so -lcurl -o linux-release/SGDBoop

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
	desktop-file-install com.steamgriddb.SGDBoop.desktop --rebuild-mime-info-cache
	@su - $(SUDO_USER)
	xdg-mime default com.steamgriddb.SGDBoop.desktop x-scheme-handler/sgdb
endif
