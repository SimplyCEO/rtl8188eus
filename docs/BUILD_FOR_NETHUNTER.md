## Build Kernel Headers

```
cd [your kernel source directory]
make module_prepare
make modules_install INSTALL_MOD_PATH=../
```

## Build RTL8188EUS driver/modules

```
cd ../
curl -L https://gitlab.com/KanuX/rtl8188eus/-/archive/master/rtl8188eus-master.tar.gz --output rtl8188eus-master.tar.gz
tar -xvf rtl8188eus-master.tar.gz
cd rtl8188eus-master/
```

That command places this driver behind your kernel source directory (RECOMMENDED).
If you put it anywhere you might need to set the Makefile in this driver, but i won't explain it.
Now, do:

```
export ARCH=arm64
export SUBARCH=arm64
export CROSS_COMPILE=../toolchain/toolchain64/bin/aarch64-linux-android-
export KBUILD_KVER=3.10.73-NetHunter-something
```

arm64 is the device architecture.
CROSS_COMPILE is your toolchain directory.
KBUILD_KVER is your kernel build version, you can search for it in ../lib/modules (the place of your modules_install when you build kernel headers).

Now, do:
```
make
```

If there is no error or success you will see a file named 8188eu.ko in this driver directory.


## Load the driver (8188eu.ko)
```
su
cd /system/lib/modules
insmod 8188eu.ko
```
