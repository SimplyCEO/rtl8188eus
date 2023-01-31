# Troubleshooting
· If your system is old, you can check your device's interface by running `iw dev <interface> info` or `ip address | grep <interface>`.<br>
· "NetworkManager.conf" is normally located under `/etc/NetworkManager/NetworkManager.conf`.<br>
· Check your if your GCC compiler is well installed. Otherwise, [this](https://github.com/KanuX-14/rtl8188eus/issues/1) could happen.<br>
· You **need** your linux headers installed in order to build this driver.<br>

[Scripts](./OPTIONAL.md) | [Main page](../../..)
