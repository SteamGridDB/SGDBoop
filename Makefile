ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: all
all: build

.PHONY: build
build:
	gcc sgdbop.c curl-helper.c string-helpers.c -lcurl -o linux-release/SGDBop

ifdef FLATPAK_ID
.PHONY: install
install: build
	install -Dm644 linux-release/com.steamgriddb.SGDBop.desktop -t /app/share/applications
	install -Dm755 linux-release/SGDBop -t /app/bin
else
.PHONY: install
install: build
	install -Dm755 linux-release/SGDBop -t $(DESTDIR)$(PREFIX)/bin
	desktop-file-install com.steamgriddb.SGDBop.desktop --rebuild-mime-info-cache
	@su - $(SUDO_USER)
	xdg-mime default com.steamgriddb.SGDBop.desktop x-scheme-handler/sgdb
endif
