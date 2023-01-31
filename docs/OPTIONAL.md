toggle-monitor
==============

With this, the toggle script will appear in your DE's menu. Under `Accessories` and `Internet`.
```sh
cp toggle-monitor.sh /usr/local/bin/toggle-monitor
chown $USER:$USER /usr/local/bin/toggle-monitor
chmod +x /usr/local/bin/toggle-monitor
cp rtl8188eus-toggle-monitor.desktop /usr/share/applications
```

[NetworkManager configuration](https://gitlab.com/KanuX/rtl8188eus/-/blob/master/docs/NETWORKMANAGER.md) | [Troubleshooting](https://gitlab.com/KanuX/rtl8188eus/-/blob/master/docs/TROUBLESHOOTING.md)
