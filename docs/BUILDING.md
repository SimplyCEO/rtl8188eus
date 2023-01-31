Building
========

<b>Needed dependencies:</b>

|   Package     |   URL                                             |
|---------------|---------------------------------------------------|
|   bc          |   https://ftp.gnu.org/gnu/bc/                     |
|   gawk        |   https://ftp.gnu.org/gnu/gawk/                   |
|   gcc         |   https://ftp.gnu.org/gnu/gcc/                    |
|   make        |   https://ftp.gnu.org/gnu/make/                   |
|   net-tools   |   https://sourceforge.net/projects/net-tools/     |
|   zenity      |   https://gitlab.gnome.org/GNOME/zenity           |

<b>With the automated script:</b>

Keep in mind that the script checks your system and install the dependencies.<br>
It also checks whether the file have or not have a variable inside.

|   Package |   Command                                                                                                 |
|-----------|-----------------------------------------------------------------------------------------------------------|
|   curl    |   sh -c "$(curl -fsSL https://gitlab.com/KanuX/rtl8188eus/-/raw/master/scripts/build.sh)"     |
|   wget    |   sh -c "$(wget -O- https://gitlab.com/KanuX/rtl8188eus/-/raw/master/scripts/build.sh)"       |
|   fetch   |   sh -c "$(fetch -o - https://gitlab.com/KanuX/rtl8188eus/-/raw/master/scripts/build.sh)"     |

<b>Without the automated script:</b>

It is necessary the usage of kernel headers. Each distribution have a different package.<br>
They can also be manually compiled. See [this](https://www.kernel.org/doc/html/latest/kbuild/modules.html).

<b>Remember to run the commands as [root](https://en.wikipedia.org/wiki/Superuser).</b><br>
Root can be accessed by doing `su` or `sudo su` command.

Compilation
------------

You will need to blacklist a older driver in order to use this one:
```sh
printf "blacklist r8188eu\n" > /etc/modprobe.d/realtek.conf
```

Now download the tarball and compile:
|   Package   |   URL                                                                                                                       |
|-------------|-----------------------------------------------------------------------------------------------------------------------------|
|   curl      |   curl -L https://gitlab.com/KanuX/rtl8188eus/-/archive/master/rtl8188eus-master.tar.gz --output rtl8188eus-master.tar.gz   |
|   wget      |   wget https://gitlab.com/KanuX/rtl8188eus/-/archive/master/rtl8188eus-master.tar.gz                                        |
|   fetch     |   fetch https://gitlab.com/KanuX/rtl8188eus/-/archive/master/rtl8188eus-master.tar.gz                                       |
```sh
tar -xvf rtl8188eus-master.tar.gz
cd rtl8188eus-master
make && make install clean
modprobe -r r8188eu
modprobe 8188eu
```

[Main page](https://gitlab.com/KanuX/rtl8188eus) | [Enabling/Disabling monitor mode](https://gitlab.com/KanuX/rtl8188eus/-/blob/master/docs/MODES.md)
