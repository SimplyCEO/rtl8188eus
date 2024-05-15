Building
========

<b>Needed dependencies:</b>

|   Package     |                      URL                          |
|---------------|---------------------------------------------------|
|   bc          |   https://ftp.gnu.org/gnu/bc/                     |
|   gcc         |   https://ftp.gnu.org/gnu/gcc/                    |
|   make        |   https://www.gnu.org/software/make/              |

<details><summary>Building with the automated script</summary>

<b>Needed dependencies:</b>

|   Package     |                      URL                          |
|---------------|---------------------------------------------------|
|   tar         |   https://ftp.gnu.org/gnu/tar/                    |
|   gawk        |   https://ftp.gnu.org/gnu/gawk/                   |
|   net-tools   |   https://sourceforge.net/projects/net-tools/     |
|   zenity      |   https://gitlab.gnome.org/GNOME/zenity           |

Keep in mind that the script checks your system and install the dependencies.<br>
It also checks whether the file have or not have a variable inside.

|  Package  |                                          Command                                                  |
|-----------|---------------------------------------------------------------------------------------------------|
|   curl    |   sh -c "$(curl -fsSL https://gitlab.com/SimplyCEO/rtl8188eus/-/raw/master/scripts/build.sh)"     |
|   wget    |   sh -c "$(wget -O- https://gitlab.com/SimplyCEO/rtl8188eus/-/raw/master/scripts/build.sh)"       |
|   fetch   |   sh -c "$(fetch -o - https://gitlab.com/SimplyCEO/rtl8188eus/-/raw/master/scripts/build.sh)"     |

</details>

<details><summary>Building like a true hacker</summary>

It is necessary the usage of kernel headers. Each distribution have a different package.<br>
They can also be manually compiled. See [this](https://www.kernel.org/doc/html/latest/kbuild/modules.html).

Compilation
------------

<b>Remember to run the commands as [root](https://en.wikipedia.org/wiki/Superuser).</b><br>
Root can be accessed by doing `su` / `doas su` / `sudo su` command.

You will need to blacklist a older driver in order to use this one:
```sh
printf "blacklist r8188eu\n" > /etc/modprobe.d/realtek.conf
```

Now download the tarball and compile:

|   Package   |                                                           URL                                                                   |
|-------------|---------------------------------------------------------------------------------------------------------------------------------|
|    curl     |   curl -L https://gitlab.com/SimplyCEO/rtl8188eus/-/archive/master/rtl8188eus-master.tar.gz --output rtl8188eus-master.tar.gz   |
|    wget     |   wget https://gitlab.com/SimplyCEO/rtl8188eus/-/archive/master/rtl8188eus-master.tar.gz                                        |
|    fetch    |   fetch https://gitlab.com/SimplyCEO/rtl8188eus/-/archive/master/rtl8188eus-master.tar.gz                                       |

```shell
tar -xvf rtl8188eus-master.tar.gz
cd rtl8188eus-master/
make && make install clean
modprobe --remove rtl8xxxu && modprobe 8188eu
```

Note: You can also use `git` if your system allows you to.

</details>

<b>Important note</b>: From kernel version `6.3+` the driver changed from `r8188eu` to `rtl8xxxu`.

[Main page](/README.md) | [Enabling/Disabling monitor mode](./MODES.md)
