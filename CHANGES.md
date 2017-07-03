Changelog
=========

All #n are issue numbers. Tracker is
[here](https://github.com/MTCKC/ProconXInput/issues).

v0.1.0-alpha2
-------------

#### Features

- Added basic calibration, implementing #3

- Added versioning info to program

- Added installation improvements to release such as including devcon.exe so 
you don't need to search for it, including the compiled ScpVBus drivers, and 
ScpVBus install/uninstall scripts

- Added RELEASE.md to the repo.

#### Fixes

- Fixed #5, The Procon's ABXY buttons now correspond to the X360 buttons that
are in the same physical location (Procon A == X360 B, Procon X = X360 Y)

- Made all .md documentation less than 80 characters per line, except for some
unfortunate links which are longer than 80 characters...

- Modified line 232 in hid.c to change device opening to unshared mode,
possibly fixing #2


v0.1.0-alpha
------------

Inital release.
