ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: all
all: build

.PHONY: build
build:
	gcc sgdbop.c curl-helper.c string-helpers.c -lcurl -o linux-release/SGDBoop

ifdef FLATPAK_ID
.PHONY: install
install: build
	install -Dm644 linux-release/com.steamgriddb.SGDBoop.desktop -t /app/share/applications
	install -Dm755 linux-release/SGDBoop -t /app/bin
	install -Dm644 res/com.steamgriddb.SGDBoop.svg -t export/share/icons/hicolor/scalable/apps
	mkdir generated_icons
	for size in 16 24 32 48 64 128 256 512 ; do \
		rsvg-convert -w $$size -h $$size -f png -o generated_icons/$$size.png res/com.steamgriddb.SGDBoop.svg ; \
		install -Dm644 generated_icons/$$size.png /app/share/icons/hicolor/$${size}x$${size}/apps/com.steamgriddb.SGDBoop.png ; \
	done
else
.PHONY: install
install: build
	install -Dm755 linux-release/SGDBoop -t $(DESTDIR)$(PREFIX)/bin
	desktop-file-install com.steamgriddb.SGDBoop.desktop --rebuild-mime-info-cache
	@su - $(SUDO_USER)
	xdg-mime default com.steamgriddb.SGDBoop.desktop x-scheme-handler/sgdb
endif
