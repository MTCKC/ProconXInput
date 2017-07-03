ProconXInput v0.1-alpha2
========================

**THIS IS ALPHA SOFTWARE**

It should be stable enough for playing games, but it's not super user friendly
or bug free and it isn't pretty. This release should theoretically support all
platforms ScpToolkit does, so Windows Vista/7/8/10.

It currently theoretically supports up to 4 Switch Pro Controllers via USB,
but I only own one so I can't test past that.

Rumble is unsupported currently, and it might be forever unless I get a
reliable way to test it. I don't have an XInput controller that supports
rumble, and there's no reliable testing app to send rumble commands. 

The controller will sometimes get cranky and decide to not work. Some things
that cause this are being plugged in while Windows reboots or if the driver
is killed unexpectedly. The driver will hang if you launch it while the
controller is like this, you can press Ctrl+C to kill it. If this happens, try
replugging the controller then relaunching the driver.

Read [README.md](https://github.com/MTCKC/ProconXInput/blob/master/README.md)
for license details and other stuff.


Changelog
---------

See CHANGES.md


Installation
------------

1. Download the release, get the 64-bit version if you have 64-bit Windows,
likewise for the 32-bit version.

2. Unzip it where you want it

3. If you don't have ScpVBus installed from a previous installation of this or
ScpToolkit, right click `install ScpVBus.bat` and click `Run as administrator`

4. Plug the Switch Pro Controller into your computer

5. Run ProconXInput.exe, ignore the warnings about HidCerberus

6. Calibrate your controllers by moving the joysticks some, let them reset to
neutral, then press the Share button on each controller.

7. Hit Windows Key+R, enter `joy.cpl` into the Run box, hit enter (Or launch
the 'Set up USB Game Controllers' panel however you want)

8. Check to see if your controller exists

9. Calibrate your controller if you want.

10. Game on!


HidGuardian and HidCerberus
---------------------------

If your games are picking up on the plugged-in Switch Controller that doesn't
work properly and not the XInput device, you'll need to hide it using
HidGuardian.

See [HidGuardian](https://github.com/nefarius/ViGEm/tree/master/HidGuardian),
[HidCerberus.Lib](https://github.com/nefarius/ViGEm/tree/master/HidCerberus.Lib)
and
[HidCerberus.Srv](https://github.com/nefarius/ViGEm/tree/master/HidCerberus.Srv)

Simply place HidCerberus.Lib.dll in the same folder as ProconXInput.exe and it
gains support for HidCerberus and HidGuardian, as long as both HidCerbeus.Srv
and HidGuardian are installed.

Known Issues
------------

**See the Github [tracker](https://github.com/MTCKC/ProconXInput/issues) for
the most up-to-date list.**

- If the Pro Controller is not hidden via HidCerberus, Windows may interpret
the HID commands it sends as various inputs, causing strange occurences such
as random clicking, rapidly opening and closing Magnifier, changing system
volume, and more. The only current workaround for this is installing
HidGuardian and HidCerberus.Srv, hiding the Procon, and adding HidCerberus.Lib
next to ProconXInput. See issue #2.

- Motion controls are unsupported. See issue #4.

- Rumble is currently unsupported. See issue #1.
