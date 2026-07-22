# <img height="20px" src="./res/com.steamgriddb.SGDBoop.svg"> SGDBoop

SGDBoop is a tool that automatically applies assets from [SteamGridDB](https://www.steamgriddb.com/) directly to your Steam library, removing the need to download and set them manually.

# Instructions

## Set up for Windows

* Download the latest [sgdboop-win64.zip](https://github.com/SteamGridDB/SGDBoop/releases/latest/download/sgdboop-win64.zip)
* Extract the program in any directory you prefer
* Run it once with Administrator privileges (can be done via Right Click -> Run as Administrator)

To remove from your system and unregister the URL handler:

1. Run `unregister.bat` as Administrator
2. Delete all the files that came with the program

## Set up for Linux

<details>
  <summary>Flatpak</summary>

  <a href="https://flathub.org/apps/details/com.steamgriddb.SGDBoop" target="_blank" rel="noreferrer">
    <img height="56px" alt="Get it on Flathub" src="https://flathub.org/api/badge?svg&locale=en" />
  </a>  

Or run `flatpak install flathub com.steamgriddb.SGDBoop`

</details>

<details>
  <summary>Build from source</summary>

1. Install the following prerequisites via your distros package manager: `make` `gcc` `libcurl4-openssl-dev` `libgtk-3-dev`
2.
   ```Shell
   git clone https://github.com/SteamGridDB/SGDBoop.git
   sudo make install -C SGDBoop
   ```

</details>

<details>
  <summary>Prebuilt binary via install script</summary>

1. Download the latest [.tar.gz](https://github.com/SteamGridDB/SGDBoop/releases/latest) matching your architecture
2.
   ```Shell
   mkdir sgdboop
   tar -zxf sgdboop-linux-*.tar.gz -C sgdboop
   chmod +x sgdboop/install.sh
   sudo ./sgdboop/install.sh
   ```

</details>

<details>
  <summary>AUR for Arch Linux</summary>

Install the AUR package [sgdboop](https://aur.archlinux.org/packages/sgdboop) or [sgdboop-bin](https://aur.archlinux.org/packages/sgdboop-bin). This can be done with an AUR manager like `yay`:
`yay -S sgdboop`  
These packages are maintained and updated by the community.

</details>

## Set up for MacOS

<details>
  <summary>brew</summary>

</details>

### Once installed, head over to <https://www.steamgriddb.com/boop> to enable the "**BOOP**" buttons!

# Credits

<a href="https://github.com/SteamGridDB/SGDBoop/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=SteamGridDB/SGDBoop" />
</a>

# License

[zlib](LICENSE)
