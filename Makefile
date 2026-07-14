# Either "linux" or "darwin", windows is built via mstools atm
OS_NAME := $(shell uname -s | tr A-Z a-z)
ARCH := $(shell uname -m)
VERSION := $(shell awk -F'"' '/define VERSION/{print $$2}' sgdboop.c)

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

USER_LIB_PATH := /usr/lib
ifdef FLATPAK_ID
USER_LIB_PATH := /app/lib
endif

PKG_CONFIG ?= pkg-config

all: build

.PHONY: build
build: build-$(OS_NAME)
	@echo "Built for target: $(OS_NAME)"
	
.PHONY: build-linux build-darwin
build-linux:
	$(CC) -O3 -g sgdboop.c curl-helper.c gui-helper-gtk.c string-helpers.c crc.c $(shell $(PKG_CONFIG) --cflags --libs gtk+-3.0 libcurl) -o SGDBoop $(LDFLAGS)
	chmod +x SGDBoop

build-darwin:
	mkdir -p SGDBoop.app/Contents/MacOS
	mkdir -p SGDBoop.app/Contents/Resources
	cp LICENSE SGDBoop.app/Contents/Resources/LICENSE.txt
	cp res/mac/AppIcon.icns SGDBoop.app/Contents/Resources/AppIcon.icns
	cp res/mac/Info.plist SGDBoop.app/Contents/Info.plist
	/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $(VERSION)" SGDBoop.app/Contents/Info.plist
	/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $(VERSION)" SGDBoop.app/Contents/Info.plist
	$(CC) -Wno-pointer-sign -fobjc-arc -arch arm64 -arch x86_64 -g sgdboop.c curl-helper.c gui-helper-mac.m string-helpers.c crc.c $(shell $(PKG_CONFIG) --cflags --libs libcurl) -framework Cocoa -o SGDBoop.app/Contents/MacOS/SGDBoop $(LDFLAGS)
	chmod +x SGDBoop.app/Contents/MacOS/SGDBoop

.PHONY: install install-linux install-flatpak
ifdef FLATPAK_ID
install: install-flatpak
else
install: install-$(OS_NAME)
endif

install-linux: build
	install -Dm755 SGDBoop -t $(DESTDIR)$(PREFIX)/bin
	desktop-file-install res/linux/com.steamgriddb.SGDBoop.desktop --rebuild-mime-info-cache
	@su - $(SUDO_USER)
	xdg-mime default com.steamgriddb.SGDBoop.desktop x-scheme-handler/sgdb

install-flatpak: build
	install -Dm644 res/linux/com.steamgriddb.SGDBoop-flatpak.desktop /app/share/applications/com.steamgriddb.SGDBoop.desktop
	install -Dm644 com.steamgriddb.SGDBoop.appdata.xml -t /app/share/metainfo
	install -Dm755 SGDBoop -t /app/bin
	install -Dm644 res/com.steamgriddb.SGDBoop.svg -t /app/share/icons/hicolor/scalable/apps

.PHONY: dist
dist: build
	mkdir -p dist/
# if mac delete symbols before archiving
ifeq ($(OS_NAME),darwin)
	rm -r SGDBoop.app/Contents/MacOS/*.dSYM
	tar -czvf dist/sgdboop-$(OS_NAME)-universal.tar.gz $(shell $(MAKE) -s print-dists-darwin)
else
	mkdir -p dist/$(OS_NAME)
	cp $(shell $(MAKE) -s print-dists-$(OS_NAME)) dist/$(OS_NAME)/
	tar -czvf dist/sgdboop-$(OS_NAME)-$(ARCH).tar.gz dist/$(OS_NAME)/*
	rm -rf dist/$(OS_NAME)
endif

.PHONY: print-dists print-dists-linux print-dists-darwin
print-dists: print-dists-$(OS_NAME)

print-dists-linux:
	@echo "SGDBoop" "LICENSE" "res/linux/com.steamgriddb.SGDBoop.desktop" "res/linux/install.sh"

print-dists-darwin:
	@echo "SGDBoop.app"

.PHONY: clean
clean:
	rm -rf SGDBoop.app SGDBoop dist