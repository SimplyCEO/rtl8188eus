Monitor Mode
============

Interface can be both identified as `wlan0` or `wlp1s0`. It may depend on how much wireless devices are connected.<br>
Use the command to list all the available interfaces:
```sh
ip address
```

Use these steps to enter monitor mode.

|   Init System   |   Command                                   |
|-----------------|---------------------------------------------|
|   OpenRC        |   rc-service NetworkManager stop            |
|   SystemD       |   systemctl stop NetworkManager.service     |

```sh
ip link set <interface> down
iw dev <interface> set type monitor
ip link set <interface> name <new_interface_name> # optional
```
Note: `new_interface_name` can be anything such as `[wlan0mon, mon0, monitor0]`.

Frame injection test may be performed with.<br>
(after kernel v5.2 scanning is slow, run a scan or simply an airodump-ng first!)

```sh
aireplay-ng -9 <interface>
```

Managed Mode
============

Use these steps to disable monitor mode. (not possible if your device's MAC address is added to `unmanaged-devices` variable under "NetworkManager.conf")

|   Init System   |   Command                                   |
|-----------------|---------------------------------------------|
|   OpenRC        |   rc-service NetworkManager start           |
|   SystemD       |   systemctl start NetworkManager.service    |

```sh
iw dev <interface> set type managed
ip link set <interface> name <old_interface_name> # optional
ip link set <interface> up
```
Note: Most of the cases the `old_interface_name` is wlan0.

If the adapter still refuses to go back, try:

|   Init System   |   Command                                     |
|-----------------|-----------------------------------------------|
|   OpenRC        |   rc-service NetworkManager restart           |
|   SystemD       |   systemctl restart NetworkManager.service    |

[Building](./BUILDING.md) | [NetworkManager configuration](./NETWORKMANAGER.md)
