<p align="center">
<img src="/.media/rtl8188eus_logo.png" alt="rtl8188eus logo" width="50%"/>
</p>

Realtek rtl8188eu(s) / rtl8188etv wireless drivers
==================================================

[![Monitor mode](https://img.shields.io/badge/monitor%20mode-supported-brightgreen.svg)](#)
[![Frame Injection](https://img.shields.io/badge/frame%20injection-supported-brightgreen.svg)](#)
[![MESH Mode](https://img.shields.io/badge/mesh%20mode-supported-brightgreen.svg)](#)
[![GitHub issues](https://img.shields.io/github/issues/SimplyCEO/rtl8188eus.svg)](https://gitlab.com/SimplyCEO/rtl8188eus/-/issues)
[![GitHub forks](https://img.shields.io/github/forks/SimplyCEO/rtl8188eus.svg)](https://gitlab.com/SimplyCEO/rtl8188eus/-/forks)
[![GitHub stars](https://img.shields.io/github/stars/SimplyCEO/rtl8188eus.svg)](https://gitlab.com/SimplyCEO/rtl8188eus/-/starrers)
[![GitHub license](https://img.shields.io/badge/License-GPL--2.0-informational)](https://gitlab.com/SimplyCEO/rtl8188eus/-/blob/master/LICENSE)<br>
[![Android](https://img.shields.io/badge/android%20(8)-supported-brightgreen.svg)](#)
[![aircrack-ng](https://img.shields.io/badge/aircrack--ng-supported-blue.svg)](#)

Trying to find a solution? See [troubleshooting](/docs/TROUBLESHOOTING.md).

|   Support         |   Tested  |   Status  |   Description                                     |
|-------------------|-----------|-----------|---------------------------------------------------|
|   Android 7+      |   ❌      |   🟡      |   Depends on which kernel version is installed.   |
|   MESH            |   ❌      |   🟠      |   Not tested yet.                                 |
|   Monitor Mode    |   ✅      |   🔵      |   Tested and working.                             |
|   Frame injection |   ✅      |   🔵      |   Tested and working.                             |
|   Kernel 5.8+     |   ✅      |   🟢      |   Kernel 5.15+ tested.                            |

Building
--------

The quickest compile can presume:
```shell
git clone --depth 1 https://gitlab.com/SimplyCEO/rtl8188eus.git
cd rtl8188eus/
make -j$(nproc)
su -c "make install clean"
su -c "modprobe --remove rtl8xxxu && modprobe 8188eu"
```

The old driver will be kept, but it need to be deactivated.<br>
Verify if your kernel is equal or newer than '6.3.x'.<br>
If it is, then the driver is called `rtl8xxxu`. Otherwise it is `r8188eu`.

All the instructions and explanations can be found by<br>
[reading the documentation](/docs/BUILDING.md) or by accessing the topics:

- [Building for Kali Nethuner](/docs/BUILD_FOR_NETHUNTER.md);
- [Available Modes](/docs/MODES.md);
- [Configuring NetworkManager](/docs/NETWORKMANAGER.md);
- [Managed/Monitor Mode: toggle-script](/docs/OPTIONAL.md).

Credits
-------

Realtek       - https://www.realtek.com<br>
Alfa Networks - https://www.alfa.com.tw<br>
aircrack-ng  - https://www.aircrack-ng.org<br>
Project contributors - https://gitlab.com/SimplyCEO/rtl8188eus/-/graphs/master?ref_type=heads<br>

And all those who are using, requesting support, or teaching. Thanks!
