NetworkManager Configuration
============================

Copy "NetworkManager.conf" to "NetworkManager.conf.bak" to create a backup.<br>
Add these lines below to "NetworkManager.conf" and ADD YOUR ADAPTER MAC below `[keyfile]`.<br>
This will make the Network-Manager ignore the device, and therefore don't cause problems.

```sh
[device]
wifi.scan-rand-mac-address=no

[ifupdown]
managed=false

[connection]
wifi.powersave=0

[main]
plugins=keyfile

[keyfile]
unmanaged-devices=A0:B1:C2:D3:E4:F5
```

[Enabling/Disabling monitor mode](./MODES.md) | [Scripts](./OPTIONAL.md)
