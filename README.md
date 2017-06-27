# ProconXInput

A Windows-only XInput USB user mode driver for the Switch Pro Controller using ViGEm.
Based upon the work of @ShinyQuagsire [here](https://github.com/shinyquagsire23/HID-Joy-Con-Whispering), which was based upon the work of dekuNukem and other contributors [here](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering).

## Requirements

- ViGEm driver installed
- ViGEmUM library. Link to ViGEmUM.lib
- Link to setupapi.lib
- Optionally requires HidGuardian and HidCerberus.srv installed, remove Cerberus._pp and some marked code from main() to disable
- HidCerberus support requires HidCerberus.Lib.dll which is loaded at runtime
- A Switch Pro Controller attatched via USB
- &lt;optional&gt; support, compile with /std:c++latest in MSVC or add your own implementation

## Building

You can use the included project for Visual Studio 2017 provided you edit the include and library directories. If you don't have VS2017, you can add the files to an empty C++ project. Then add ViGEmUM headers to your include directories, ViGEmUM.lib's location to your library directories, set C++ Language Standard to /std:c++latest, add ViGEmUM.lib and setupapi.lib to linker input, and build.

## Using

Plug in the Pro Controller, then run ProconXInput.exe. Press CTRL+C when you're done.

## License

See LICENSE.txt

ViGEm license is under LICENSE-ViGEm.txt

hidapi license is under LICENSE-hidapi.txt
