# <img height="20" src="./res/com.steamgriddb.SGDBop.svg"> SGDBop 
SGDBop is a tool that automatically applies assets from [SteamGridDB](https://www.steamgriddb.com/) directly to your Steam library, removing the need to download and set them manually.

# Instructions
## Set up for Windows
- Put the program in any directory you prefer
- Run it once with Administrator privileges (can be done via Right Click -> Run as Administrator)

In case you want to completely remove the app from your system:
- Run the program with the 'unregister' argument (For example, in an Admin CMD run `SGDBop.exe unregister`)
- Delete all the files that came with the program

## Set up for Linux
### Build from source
- Install the following prerequisites via your distros package manager: `make` `gcc` `libcurl4-openssl-dev`
- ```sh
  git clone https://github.com/SteamGridDB/SGDBop.git
  sudo make install -C SGDBop
  ```

### Prebuilt binary via install script
- Download the latest [sgdbop-linux64.tar.gz](https://github.com/SteamGridDB/SGDBop/releases/latest)
- ```sh
  mkdir sgdbop-linux64
  tar -zxf sgdbop-linux64.tar.gz -C sgdbop-linux64
  chmod +x sgdbop-linux64/install.sh
  sudo ./sgdbop-linux64/install.sh
  ```

**That's it!** You can now go to https://www.steamgriddb.com/bop to enable the "**BOP**" button!

# Credits
<a href="https://github.com/SteamGridDB/SGDBop/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=SteamGridDB/SGDBop" />
</a>

# License
[MIT](LICENSE)
