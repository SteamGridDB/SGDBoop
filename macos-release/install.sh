#!/bin/bash
# SGDBoop macOS installer
# Copies SGDBoop.app to /Applications and registers the sgdb:// URL scheme.

set -e
cd "$(dirname "$0")"

APP_NAME="SGDBoop.app"
INSTALL_DIR="/Applications"

echo "Installing $APP_NAME to $INSTALL_DIR ..."

# Remove any previous installation
if [ -d "$INSTALL_DIR/$APP_NAME" ]; then
    rm -rf "$INSTALL_DIR/$APP_NAME"
fi

cp -R "$APP_NAME" "$INSTALL_DIR/"
echo "Copied $APP_NAME to $INSTALL_DIR"

# Open the app once so Launch Services registers the sgdb:// URL scheme
# from the Info.plist CFBundleURLTypes declaration.
open "$INSTALL_DIR/$APP_NAME"

echo "Installation complete!"
echo "SGDBoop is now registered as the sgdb:// URL handler."
echo "Head over to https://www.steamgriddb.com/boop to get started."
