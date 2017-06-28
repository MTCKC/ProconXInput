# ProconXInput

A Windows-only XInput USB user mode driver for the Switch Pro Controller using hidapi and XOutput/ScpVBus.

Based upon the work of:

- @ShinyQuagsire [here](https://github.com/shinyquagsire23/HID-Joy-Con-Whispering), which was based upon the work of dekuNukem and other contributors [here](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering). 
- Rumble features from @TheToadKing [here](https://github.com/ToadKing/switch-pro-x).

## Requirements

- Both the ScpVBus driver and XOutput library from [here](https://github.com/nefarius/ScpVBus).
- ScpVBus from the [ScpToolkit installer](https://github.com/nefarius/ScpToolkit/) will probably work, but I haven't tested it and it comes with a bunch of extra stuff.
- Link to XOutput1_2.lib and setupapi.lib
- Supports [HidGuardian](https://github.com/nefarius/ViGEm/tree/master/HidGuardian) via [HidCerberus.Srv](https://github.com/nefarius/ViGEm/tree/master/HidCerberus.Srv). Define NO_CERBERUS to disable support, but it will run fine even without disabling if neither are available
- HidGuardian/HidCerberus.Srv support requires [HidCerberus.Lib](https://github.com/nefarius/ViGEm/tree/master/HidCerberus.Lib).dll which is dynamically loaded at runtime
- A Switch Pro Controller attatched via USB
- &lt;optional&gt; support, compile with /std:c++latest in MSVC or add your own implementation

## Building

You can use the included project for Visual Studio 2017 provided you edit the include and library directories. If you don't have VS2017, you can add the files to an empty C++ project. Then add XOutput headers to your include directories, XOutput1_2.lib's location to your library directories, set C++ Language Standard to /std:c++latest, add XOutput1_2.lib and setupapi.lib to linker input, and build.

## Using

Plug in the Pro Controller, then run ProconXInput.exe. Press CTRL+C or close the window when you're done.

## License

This code is available under the MIT License, see LICENSE.txt

XOutput and ScpVBus are part of the ScpToolkit, licensed under Gnu GPLv3, see LICENSE-Scp.txt

hidapi is licensed under the MIT License, see LICENSE-hidapi.txt
