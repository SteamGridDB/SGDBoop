# SteamGridDB Presents SGDBop
Have fun with SGDBop, the new and improved automatic tool for applying your favorite artwork from [SteamGridDB.com](https://www.steamgriddb.com/) directly to your Steam library!

# Instructions
## Set up for Windows
- Put the program in any directory you prefer
- Run it once with Administrator privileges (can be done via Right Click -> Run as Administrator)

In case you want to completely remove the app from your system:
- Run the program with the 'unregister' argument (For example, in and Admin CMD run `SGDBop.exe unregister`)
- Delete all the files that came with the program

## Set up for Linux
### From source
- Install the following prerequisites via your distros package manager: `make` `gcc` `libcurl4-openssl-dev`
- ```sh
  git clone https://github.com/SteamGridDB/SGDBop.git
  sudo make install -C SGDBop
  ```

### Via install script
- Download the latest [sgdbop-linux64.tar.gz](https://github.com/SteamGridDB/SGDBop/releases/latest)
- ```sh
  mkdir sgdbop-linux64
  tar -zxf sgdbop-linux64.tar.gz -C sgdbop-linux64
  chmod +x sgdbop-linux64/install.sh
  sudo ./sgdbop-linux64/install.sh
  ```

**That's it!** You can now go to SteamGridDB.com, click the "**BOP**" button on any artwork you want and SGDBop will automatically apply it to your Steam client!

# Credits
<a href="https://github.com/SteamGridDB/SGDBop/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=SteamGridDB/SGDBop" />
</a>

# License
[MIT](LICENSE)
