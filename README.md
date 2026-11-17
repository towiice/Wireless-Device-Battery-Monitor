# Wireless Battery

Battery levels of your wireless mouse, headset, keyboard and controller, right on the Windows taskbar.

![Wireless Battery on the taskbar](docs/screenshot.png)

> **Just want it working?** Download **[WirelessBattery-Setup](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor/releases/latest)** from the latest release, run it and click through. That's it.
>
> **Experienced users** can grab the portable zip, use the [command-line options](#command-line), or [build it from source](#build) with `make` or `build.bat`.

- **Tiny**: a ~350 KB app using about 2 MB of RAM. It's plain C++/Win32, with no runtime and no background service, and installs in seconds without admin rights.
- **No vendor software needed**: it talks to the devices directly. G HUB, Synapse, iCUE, GG and NGenuity can stay closed (or keep running; they don't conflict).
- **Many brands**: Logitech, Razer, SteelSeries, Corsair, HyperX, Glorious, Kysona, Smailwolf, PlayStation and Xbox controllers, any Bluetooth device Windows knows a battery level for, and every headset the bundled [HeadsetControl](https://github.com/Sapd/HeadsetControl) supports.

## Display

**On the taskbar (default):** `🎧 30%  🖱 93%` next to the notification area, drawn with the Windows icon font.
- **Low battery:** a red number means 15% or less.
- **Charging:** a blue ⚡ appears next to the number.
- **Asleep:** dimmed text means the device is asleep and the last known value is shown.
- **Theme:** it follows light/dark mode and DPI scaling.

**Tray icons:** one icon per device with the number drawn in it, plus a "Low battery" notification at 15%. The app switches to this automatically when the taskbar is vertical or can't be found.

Hover for details. Click for the menu: every device, display mode, *Refresh now*, *Start with Windows*, and *Exit*. Levels refresh every 60 seconds, and right away when a device is plugged in or the PC wakes up.

## Supported devices

| Vendor | Devices | How |
|---|---|---|
| **Logitech** | Mice and keyboards on Unifying, Bolt, Lightspeed and Nano receivers, over USB or Bluetooth; G headsets (G533, G535, G633/G635/G733/G933/G935, G PRO/PRO X) | HID++ 1.0 / 2.0 with feature discovery: unified battery, battery level, voltage, ADC |
| **Razer** | Wireless mice, keyboards and dongles; BlackShark V3 / V3 Pro headsets | Razer feature-report protocol; headset envelope protocol (queried every 5 min) |
| **SteelSeries** | Arctis 1/7/7+/9/Pro/Pro Wireless, Arctis Nova 3P/5/7/7X/7P/Pro Wireless, GameBuds; Aerox 3/5/9 Wireless, Prime Wireless, Rival 3 Wireless (Gen 2), Rival 650, Rival Pro | Vendor HID reports |
| **Corsair** | Void / Void Pro / Void Elite / HS70 / HS80 wireless; Bragi devices (Virtuoso, Void V2, HS80 MAX, Slipstream mice and keyboards) | Vendor HID reports, Bragi `GET` (read-only) |
| **HyperX** | Cloud III Wireless, Cloud II Wireless (HP and Kingston), Cloud Alpha Wireless, Cloud Alpha 2 Wireless, Cloud Flight | Vendor HID reports |
| **Glorious** | Model O / O- / D / D- Wireless, Model O 2 / D 2 PRO | Vendor feature report |
| **Hitscan** | Hyperlight X, Hyperlight N | Vendor feature report (same protocol as Glorious) |
| **Kysona / Pulsar / ATK (VXE) / Hitscan** | Kysona M600 / Mercury, Pulsar and ATK VXE wireless mice (e.g. VXE X3 Pro, ATK F1 Extreme), Hitscan Hyperlight | Shared vendor protocol with checksummed replies |
| **2.4 GHz dongles of the qmk.top family** | Kysona Jupiter Pro (dongle 3151:402D) and other 8K mice and keyboards whose dongle is listed by the [qmk.top](https://qmk.top/) web configurator (VIDs 3151, 39AB, 25AA, 342D, 374A, 347A, 0DB0, 379A, 38A9, 145F, 38EE, 391D, 14A5). Only 3151:402D has been tested | Vendor feature report `F7` (battery and online state of the paired mouse/keyboard) |
| **SMAILWOLF** | RS7 over USB cable (A8A4:2255) or its 2.4 GHz receiver (A8A5:2255, untested). No battery over Bluetooth: the mouse does not expose it and the vendor's own software does not read it either | Vendor HID report `55 30 A5 0B 2E` (percent and charging) |
| **ATK headsets** | ATK N9 Pro | Virtual COM port protocol (the ATK desktop app must be closed) |
| **G-Wolves** | Fenrir, Fenrir Pro / Max / Asym, HTX, HTS, HSK, HTR, HT-S2, Lycan, WARG, VUK and their 8K dongles | Glorious or Kysona protocol, depending on the model |
| **Keychron mice** | M4 and other Keychron mice on the 2.4 GHz receiver | Vendor report from Keychron Launcher |
| **ASUS ROG** | Strix Scope II 96 (RX), Azoth, Falchion keyboards; Keris, Harpe, Gladius, Chakram, Spatha, Pugio, Strix Impact mice; the Omni Receiver | ROG HID battery query |
| **ZOWIE** | EC1/EC2/EC3-CW and DW series (U2-DW, S2-DW ...) | Community-documented read-only vendor commands |
| **Motospeed / Delux / Ajazz** | Motospeed X6, Delux M900 Pro, Ajazz AJ179 family | Vendor reports (Delux: passive status broadcasts) |
| **Sony** | DualShock 4, DualSense, DualSense Edge (USB, Bluetooth, wireless adapter); INZONE H9 II and H5 headsets | Input report; INZONE vendor protocol |
| **Xbox** | Controllers and headsets on the Xbox Wireless adapter or USB | XInput (coarse levels: Low / Medium / Full) |
| **Bluetooth** | Anything that shows a battery in *Settings → Bluetooth & devices* (earbuds, headphones, BLE mice and keyboards). Earbuds that keep their battery on the Hands-Free node, such as the QCY MeloBuds Pro, are included | Windows device property |
| **Generic HID** | Devices reporting the standard HID *Battery Strength* usage | HID usage page 0x06 / 0x85 |
| **HeadsetControl** | [Every headset it supports](https://github.com/Sapd/HeadsetControl#supported-headsets) (Audeze, Jabra, Sony INZONE, newer Logitech and more) | Bundled; runs `headsetcontrol -o json` |

**Tested on real hardware:** Glorious Model O Wireless, HyperX Cloud III Wireless, Kysona Jupiter Pro (2.4 GHz receiver 3151:402D), Smailwolf RS7 (USB cable) and QCY MeloBuds Pro (Bluetooth).
**Implemented from protocol documentation and other projects, not yet tested:** everything else. If you own one of these devices, please report whether it works by [opening an issue](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor/issues).

## Install

[![Latest version](https://img.shields.io/github/v/release/krisgfx9/Wireless-Device-Battery-Monitor?label=latest%20version&style=for-the-badge)](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor/releases/latest)

1. Download **`WirelessBattery-Setup-<version>.exe`** from the [latest release](https://github.com/krisgfx9/Wireless-Device-Battery-Monitor/releases/latest).
2. Run it and click through. It needs **no administrator rights** and **nothing else installed**: no
   runtimes, no drivers, no vendor apps. HeadsetControl comes bundled.

The installer adds a Start menu entry and an uninstaller (*Settings → Apps*), and can start the app
when you sign in. Prefer no installer? Download `WirelessBattery-<version>-portable.zip`, unzip it
anywhere and run `WirelessBattery.exe`.

**Notes:**
- **SmartScreen warning:** the exe is not code-signed, so Windows may say "Windows protected your PC". Click *More info → Run anyway*, or build it yourself.
- **Tray-icon mode on Windows 11:** new tray icons go to the `^` overflow. Drag them onto the taskbar or enable them in *Settings → Personalization → Taskbar → Other system tray icons*.

## Command line

```
WirelessBattery.exe --list   # print every device and its battery level, then exit
WirelessBattery.exe --dump   # print every HID collection (useful when asking for a new device)
WirelessBattery.exe --quit   # close the running instance
```

## Build

You need [MinGW-w64](https://www.mingw-w64.org/), e.g. from [MSYS2](https://www.msys2.org/) (`pacman -S mingw-w64-x86_64-gcc make`).

```
make            # MSYS2 / any shell with make
build.bat       # plain cmd with g++ and windres on PATH
```

To build the installer and portable zip, install [Inno Setup](https://jrsoftware.org/isinfo.php) and run
`powershell -File installer\build-installer.ps1 -Version 1.0.0`. It downloads the pinned
HeadsetControl release and verifies its checksum. GitHub Actions does all of this automatically:
pushing a tag like `v1.0.0` publishes a release with the installer, the portable zip and the
HeadsetControl source (as its GPL license requires).

`WirelessBattery.exe` on its own also works. Without `headsetcontrol.exe` next to it (or on `PATH`) you
just lose the extra headsets.

## Limitations

- **Primary monitor only:** the taskbar widget sits on the primary monitor's taskbar.
- **Crowded taskbars:** with a very full centered Windows 11 taskbar, the app buttons can reach the widget. On Windows 10 the task-button area is narrowed automatically.
- **Sleeping devices:** a device that is asleep may not answer. Its last value stays visible, dimmed. Logitech receivers drop sleeping devices from the link, so their last value is kept for 30 minutes.
- **Keychron keyboards:** they only report their battery over Bluetooth (shown via the Bluetooth support). Over the 2.4 GHz receiver the firmware does not expose it. Keychron mice are supported.
- **Smailwolf RS7:** the mouse does not expose a battery over Bluetooth, and neither does the vendor's own software, so only the USB cable and the 2.4 GHz receiver are supported. Over the cable it reports 100% while charging; that value has not been checked against a partly discharged battery. The receiver path follows the vendor's web configurator and is untested.
- **Kysona Jupiter Pro:** the battery is read through the 2.4 GHz receiver. Over the USB cable (3151:4026) no battery command was found.
- **Unknown protocols:** some 2.4 GHz devices use undocumented protocols and are not supported yet. See [CONTRIBUTING.md](CONTRIBUTING.md) for how to help add one.

## How it works

Each vendor driver in [`src/drivers/`](src/drivers) recognises its devices by USB vendor and product ID, and only ever sends read-only battery queries. Unknown devices are never written to. The generic fallbacks (HID battery usage, Bluetooth, XInput, HeadsetControl) only look at devices no vendor driver has claimed.

The taskbar widget is a layered child window of `Shell_TrayWnd`, placed left of `TrayNotifyWnd`. If Explorer restarts, it is re-created.

## Credits

The protocols come from these projects' research:

- [HeadsetControl](https://github.com/Sapd/HeadsetControl): SteelSeries, Corsair, HyperX and Logitech headsets
- [OpenRazer](https://github.com/openrazer/openrazer): Razer
- [Solaar](https://github.com/pwr-Solaar/Solaar) and the Logitech HID++ 2.0 documents: Logitech
- [rivalcfg](https://github.com/flozz/rivalcfg): SteelSeries mice (Rival Pro from its pull request #294)
- [glorious-indicator](https://github.com/ikajdan/glorious-indicator) and [glorious-ctl](https://github.com/louis4craft/glorious-ctl): Glorious
- Hitscan's web configurator ([control.hitscan.com](https://control.hitscan.com)): Hitscan
- Linux `hid-kysona` (Lode Willems): Kysona, Pulsar and ATK
- ATK's web configurator ([hub.atk.pro](https://hub.atk.pro)): ATK mice and the N9 Pro headset
- G-Wolves' web driver ([mouse.xyz](https://mouse.xyz)): G-Wolves models and their protocols
- Keychron Launcher ([launcher.keychron.com](https://launcher.keychron.com)): Keychron mice
- The qmk.top web configurator ([qmk.top](https://qmk.top/)): 2.4 GHz dongles of its family, including the Kysona Jupiter Pro
- Smailwolf's web driver ([smailwolf.com/qdxz/wyqd](https://smailwolf.com/qdxz/wyqd/)): Smailwolf RS7
- [peripheral-battery-tray](https://github.com/SteveWang92/peripheral-battery-tray) / [G-Helper](https://github.com/seerge/g-helper): ASUS ROG
- [zowie-battery](https://github.com/yleshchuk/zowie-battery) and [zowie-battery-tray](https://github.com/IgnacioBarb/zowie-battery-tray): ZOWIE EC-CW and DW
- [LGSTrayBattery-MotospeedX6](https://github.com/Jacobz888/LGSTrayBattery-MotospeedX6), [omarchy-delux-m900](https://github.com/AeonArcHQ/omarchy-delux-m900), [aj179-linux](https://github.com/Rockeyxx/aj179-linux): Motospeed, Delux, Ajazz
- OpenRazer PR #2794 and [omarchy-blackshark-v3](https://github.com/ReedME/omarchy-blackshark-v3): BlackShark V3
- [HyperXWM](https://github.com/Medprj/HyperXWM): HyperX Cloud III Wireless
- Linux `hid-playstation`: DualShock 4 / DualSense report layout

## License

[GNU General Public License v3.0 or later](LICENSE). You may use, modify and redistribute this
software, but anything you distribute that is based on it must be released under the same license,
with its source code.
