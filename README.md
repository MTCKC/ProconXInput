ProconXInput
============

A Windows-only XInput USB user-mode driver for the Switch Pro Controller using
hidapi, ScpVBus, and XOutput.

Based upon the work of:

- @ShinyQuagsire
[here](https://github.com/shinyquagsire23/HID-Joy-Con-Whispering), which was
based upon the work of dekuNukem and other contributors
[here](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering). 
- Rumble features (Currently unsupported) from @TheToadKing
[here](https://github.com/ToadKing/switch-pro-x).

Read RELEASE.md for release notes.


Build Requirements
------------------

- Link to setupapi.lib
- A C++ compiler with &lt;optional&gt; support. For MSVC, compile with
/std:c++latest or add your own implementation


Run Requirements
----------------

- [ScpVBus](https://github.com/nefarius/ScpToolkit/tree/master/ScpControl/ScpVBus)
driver installed. If you have the ScpToolkit installed, this is already
installed

- [XOutput1_1.dll](https://github.com/nefarius/ScpToolkit/tree/master/ScpControl/XOutput)
in the same directory as the program, dynamically loaded at runtime

- Supports
[HidGuardian](https://github.com/nefarius/ViGEm/tree/master/HidGuardian) via
[HidCerberus.Srv](https://github.com/nefarius/ViGEm/tree/master/HidCerberus.Srv).
Define NO_CERBERUS to disable support, but it will run fine even without
disabling if neither are available

- HidGuardian/HidCerberus.Srv support requires
[HidCerberus.Lib](https://github.com/nefarius/ViGEm/tree/master/HidCerberus.Lib).dll
which is dynamically loaded at runtime. This library is optional, and the
driver will work without it

- A Switch Pro Controller attatched via USB


Installing/Uninstalling ScpVBus
-------------------------------

0. The release has a .bat file and devcon.exe included to automate this, use
that if you'd like

1. Get devcon.exe, and put it in the PATH or next to the driver to install.
Make sure you install the amd64 driver on 64 bit Windows, and the x86 driver
on 32 bit

2. Launch an Administrator command prompt and navigate to the driver folder.
devcon will fail if you do not launch it as an Administrator!

3. Run `devcon install ScpVBus.inf Root\ScpVBus` in the same folder as the
driver

4. Press "Install" when the confirmation box shows up, optionally untick
"Always trust software from '...'"

5. Good to go! Try running ProconXInput.exe

6. To uninstall ScpVBus, run `devcon remove Root\ScpVBus` or in Device
Manager right click System Devices/Scp Virtual Bus Driver and hit "Uninstall"

Or, you can install the ScpToolkit if you don't want to do it manually. This
installs a bunch of extra software that isn't required, however. Especially
make sure to not install the Bluetooth driver if you aren't going to use
Dualshock 3 controllers via Bluetooth, as they can interfere with normal
Bluetooth usage.


Building
--------

You can use the included project for Visual Studio 2017. If you don't have
VS2017, you can add the files to an empty C++ project. Then, set C++
Language Standard to /std:c++latest, add setupapi.lib to linker input, and
build.


Using
-----

Plug in the Pro Controller, then run ProconXInput.exe. Press CTRL+C when
you're done.

If the controller is plugged in and the driver doesn't detect it or doesn't
work properly, try replugging it before running the driver. Sometimes the
controller can get 'stuck' and won't reinitialize properly after computer
reboots or driver crashes/unexpected closes.


License
-------

All code except for XOutput and hidapi is dual licensed under the the MIT (Expat)
License, see Licenses/LICENSE.txt, and the GPLv3, see Licenses/LICENSE-GPL3.txt

XOutput and ScpVBus are part of the ScpToolkit, licensed under GNU GPLv3, see
Licenses/LICENSE-Scp.txt

HidGuardian and HidCerberus.Lib are part of the ViGEm library, licensed under
the MIT (Expat) License, see Licenses/LICENSE-ViGEm.txt

hidapi is licensed under a BSD 3-clause license,
see Licenses/LICENSE-hidapi.txt
