# A vncserver for Sailfish devices
## Build in Sailfish OS SDK for an armv7hl device
* you need libVNCServer in the SDK, follow the instructions in the [libVNCServer README](https://github.com/mer-qa/libvncserver/blob/master/README.md)

* Check out the code (if not done already)
```
git clone https://github.com/mer-qa/lipstick2vnc.git
```

* Start the Sailfish OS SDK Qt-Creator and open the file ```lipstick2vnc.pro```

* In kit selection make sure you enable **MerSDK-SailfishOS-armv7hl**

* In the build settings (on the left side, the Sailfish OS icon above the *play* button) configure **MerSDK-SailfishOS-armv7hl** -> **Release** -> **Deploy By Building An RPM Package**

* In the **Build** menu choose **Deploy All**, the created RPM file can now be installed on the device

## Run on device
*Systemd* ensures that the VNC server is launched when a client connects to port 5900. So it's logs go into the *systemd journal*. To spare resources the VNC server just runs while clients are connected.

There is no authendication! So connections are just accepted from usb network. So e.g. on Jolla phone you have to enable developer mode and when asked choose "Developer mode", default ip address is then 192.168.2.15 to connect to with the VNC client.

## Debug on device
If you want to run the server in the forground, to follow any output, you need to stop the *systemd* socket listener, see below.

For all these commands you must be user **root**.

To verify the *systemd* socket listener is active:
```
systemctl status vnc.socket
```

To restart the *systemd* socket listener:
```
systemctl restart vnc.socket
```

To stop the *systemd* socket listener:
```
systemctl stop vnc.socket
```

To start the *systemd* socket listener:
```
systemctl start vnc.socket
```

Follow log entries in the *systemd* journal, just for the VNC server:
```
journalctl -f -a /usr/bin/lipstick2vnc
```

Verify if the server is running (just running/active when a client is connected):
```
systemctl status vnc.service
```
