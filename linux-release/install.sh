#!/bin/bash
cd "$(dirname "$0")"

if [ "$EUID" -ne 0 ]; then
    echo "Please run this install script with elevated permissions."
    exit 1
fi

install -Dm755 SGDBoop -t /usr/local/bin
chmod +x /usr/local/bin/SGDBoop
desktop-file-install com.steamgriddb.SGDBoop.desktop --delete-original --rebuild-mime-info-cache

su - $SUDO_USER
xdg-mime default com.steamgriddb.SGDBoop.desktop x-scheme-handler/sgdb

echo "Installation complete! You may delete this install script."
exit 0
