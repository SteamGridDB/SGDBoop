# <img height="20px" src="./res/com.steamgriddb.SGDBoop.svg"> SGDBoop
SGDBoop is a tool that automatically applies assets from [SteamGridDB](https://www.steamgriddb.com/) directly to your Steam library, removing the need to download and set them manually.

# Instructions
## Set up for Windows
- Download the latest [sgdboop-win64.zip](https://github.com/SteamGridDB/SGDBoop/releases/latest/download/sgdboop-win64.zip)
- Extract the program in any directory you prefer
- Run it once with Administrator privileges (can be done via Right Click -> Run as Administrator)

In case you want to completely remove the app from your system:

- Run the program with the 'unregister' argument (For example, in an Admin CMD run `SGDBoop.exe unregister`)
- Delete all the files that came with the program

## Set up for Linux
<details>
  <summary>Flatpak</summary>
  
  <a href="https://flathub.org/apps/details/com.steamgriddb.SGDBoop" target="_blank" rel="noreferrer">
    <img height="56px" alt="Download on Flathub" src="https://flathub.org/assets/badges/flathub-badge-en.svg" />
  </a>  

  Or run `flatpak install flathub com.steamgriddb.SGDBoop`
</details>

<details>
  <summary>Build from source</summary>
  
  1. Install the following prerequisites via your distros package manager: `make` `gcc` `libcurl4-openssl-dev`
  2. ```sh
     git clone https://github.com/SteamGridDB/SGDBoop.git
     sudo make install -C SGDBoop
     ```
</details>

<details>
  <summary>Prebuilt binary via install script</summary>
  
  1. Download the latest [sgdboop-linux64.tar.gz](https://github.com/SteamGridDB/SGDBoop/releases/latest)
  2. ```sh
     mkdir sgdboop-linux64
     tar -zxf sgdboop-linux64.tar.gz -C sgdboop-linux64
     chmod +x sgdboop-linux64/install.sh
     sudo ./sgdboop-linux64/install.sh
     ```
</details>

<details>
  <summary>AUR for Arch Linux</summary>
  
  Install [the AUR package](https://aur.archlinux.org/packages/sgdboop). This can be done with an AUR manager like `yay`:
    `yay -S sgdboop`
</details>

### Once installed, head over to https://www.steamgriddb.com/boop to enable the "**BOOP**" buttons!

# Credits
<a href="https://github.com/SteamGridDB/SGDBoop/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=SteamGridDB/SGDBoop" />
</a>

# License
[zlib](LICENSE)
