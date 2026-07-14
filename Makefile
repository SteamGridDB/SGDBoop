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

# ---------------------------------------------------------------------------
# macOS targets
# ---------------------------------------------------------------------------
MACOS_OUTPUT_DIR := macos-release
MACOS_BUNDLE_DIR := $(MACOS_OUTPUT_DIR)/SGDBoop.app
MACOS_CC         ?= clang

.PHONY: build-macos
build-macos:
	$(MACOS_CC) -O3 -g \
		sgdboop.c curl-helper.c gui-helper-mac.m string-helpers.c crc.c \
		-framework Cocoa \
		-framework ApplicationServices \
		-lcurl \
		-o $(MACOS_OUTPUT_DIR)/SGDBoop $(LDFLAGS)

# Convert res/icon.ico → SGDBoop.icns using built-in macOS tools.
# -s format png is required so sips actually converts (without it sips
# writes an ICO with a .png suffix instead of a real PNG).
.PHONY: icon-macos
icon-macos:
	@mkdir -p SGDBoop.iconset
	sips -s format png -z 16   16   res/icon.ico --out SGDBoop.iconset/icon_16x16.png    2>/dev/null || true
	sips -s format png -z 32   32   res/icon.ico --out SGDBoop.iconset/icon_32x32.png    2>/dev/null || true
	sips -s format png -z 32   32   res/icon.ico --out SGDBoop.iconset/icon_16x16@2x.png 2>/dev/null || true
	sips -s format png -z 64   64   res/icon.ico --out SGDBoop.iconset/icon_32x32@2x.png 2>/dev/null || true
	sips -s format png -z 128  128  res/icon.ico --out SGDBoop.iconset/icon_128x128.png  2>/dev/null || true
	sips -s format png -z 256  256  res/icon.ico --out SGDBoop.iconset/icon_128x128@2x.png 2>/dev/null || true
	sips -s format png -z 256  256  res/icon.ico --out SGDBoop.iconset/icon_256x256.png  2>/dev/null || true
	sips -s format png -z 512  512  res/icon.ico --out SGDBoop.iconset/icon_256x256@2x.png 2>/dev/null || true
	sips -s format png -z 512  512  res/icon.ico --out SGDBoop.iconset/icon_512x512.png  2>/dev/null || true
	sips -s format png -z 1024 1024 res/icon.ico --out SGDBoop.iconset/icon_512x512@2x.png 2>/dev/null || true
	iconutil -c icns SGDBoop.iconset -o $(MACOS_OUTPUT_DIR)/SGDBoop.icns && rm -rf SGDBoop.iconset || (rm -rf SGDBoop.iconset; echo "Warning: iconutil failed, bundle will have no .icns icon.")

# Assemble the .app bundle inside macos-release/, mirroring the linux-release/ pattern.
.PHONY: bundle-macos
bundle-macos: build-macos icon-macos
	@mkdir -p $(MACOS_BUNDLE_DIR)/Contents/MacOS
	@mkdir -p $(MACOS_BUNDLE_DIR)/Contents/Resources
	cp $(MACOS_OUTPUT_DIR)/Contents/Info.plist $(MACOS_BUNDLE_DIR)/Contents/Info.plist
	cp $(MACOS_OUTPUT_DIR)/SGDBoop $(MACOS_BUNDLE_DIR)/Contents/MacOS/SGDBoop
	@chmod +x $(MACOS_BUNDLE_DIR)/Contents/MacOS/SGDBoop
	@if [ -f $(MACOS_OUTPUT_DIR)/SGDBoop.icns ]; then \
		cp $(MACOS_OUTPUT_DIR)/SGDBoop.icns $(MACOS_BUNDLE_DIR)/Contents/Resources/SGDBoop.icns; \
	fi
	cp LICENSE $(MACOS_OUTPUT_DIR)/LICENSE
	@echo "Bundle assembled: $(MACOS_BUNDLE_DIR)"

# Package everything in macos-release/ as a zip for distribution.
.PHONY: release-macos
release-macos: bundle-macos
	@rm -f SGDBoop-macOS.zip
	cd $(MACOS_OUTPUT_DIR) && zip -r ../SGDBoop-macOS.zip SGDBoop.app install.sh LICENSE
	@echo "Release archive created: SGDBoop-macOS.zip"

.PHONY: clean-macos
clean-macos:
	@rm -rf $(MACOS_BUNDLE_DIR) $(MACOS_OUTPUT_DIR)/SGDBoop $(MACOS_OUTPUT_DIR)/SGDBoop.icns \
	         $(MACOS_OUTPUT_DIR)/LICENSE SGDBoop.iconset SGDBoop-macOS.zip \
	         SGDBoop-macos SGDBoop.icns  # remove stale root-level artifacts if present