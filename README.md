# <img height="20" src="./res/com.steamgriddb.SGDBoop.svg"> SGDBoop
SGDBoop is a tool that automatically applies assets from [SteamGridDB](https://www.steamgriddb.com/) directly to your Steam library, removing the need to download and set them manually.

# Instructions
## Set up for Windows
- Put the program in any directory you prefer
- Run it once with Administrator privileges (can be done via Right Click -> Run as Administrator)

In case you want to completely remove the app from your system:

- Run the program with the 'unregister' argument (For example, in an Admin CMD run `SGDBoop.exe unregister`)
- Delete all the files that came with the program

## Set up for Linux
### Build from source
- Install the following prerequisites via your distros package manager: `make` `gcc` `libcurl4-openssl-dev`
- ```sh
  git clone https://github.com/SteamGridDB/SGDBoop.git
  sudo make install -C SGDBoop
  ```

### Prebuilt binary via install script
- Download the latest [sgdboop-linux64.tar.gz](https://github.com/SteamGridDB/SGDBoop/releases/latest)
- ```sh
  mkdir sgdboop-linux64
  tar -zxf sgdboop-linux64.tar.gz -C sgdboop-linux64
  chmod +x sgdboop-linux64/install.sh
  sudo ./sgdboop-linux64/install.sh
  ```

### Using AUR for Arch Linux
- Install [the AUR package](https://aur.archlinux.org/packages/sgdboop-bin). This can be done with an AUR manager like `yay`:
  `yay -S sgdboop-bin`


**That's it!** You can now go to https://www.steamgriddb.com/boop to enable the "**BOOP**" button!

# Credits
<a href="https://github.com/SteamGridDB/SGDBoop/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=SteamGridDB/SGDBoop" />
</a>

# License
[MIT](LICENSE)
