# ProconXInput

A Windows-only XInput USB user mode driver for the Switch Pro Controller using XOutput/ScpVBus.
Based upon the work of @ShinyQuagsire [here](https://github.com/shinyquagsire23/HID-Joy-Con-Whispering), which was based upon the work of dekuNukem and other contributors [here](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering).

## Requirements

- ScpVBus driver installed
- XOutput library, link to XOutput1_2.lib
- Link to setupapi.lib
- Optionally requires HidGuardian and HidCerberus.srv installed. Define NO_CERBERUS to disable, but it should run without it just fine.
- HidCerberus support requires HidCerberus.Lib.dll which is dynamically loaded at runtime
- A Switch Pro Controller attatched via USB
- &lt;optional&gt; support, compile with /std:c++latest in MSVC or add your own implementation

## Building

You can use the included project for Visual Studio 2017 provided you edit the include and library directories. If you don't have VS2017, you can add the files to an empty C++ project. Then add XOutput headers to your include directories, XOutput1_2.lib's location to your library directories, set C++ Language Standard to /std:c++latest, add XOutput1_2.lib and setupapi.lib to linker input, and build.

## Using

Plug in the Pro Controller, then run ProconXInput.exe. Press CTRL+C when you're done.

## License

See LICENSE.txt

XOutput is part of the ScpToolkit. License is under LICENSE-ScpVBus.txt

hidapi license is under LICENSE-hidapi.txt
