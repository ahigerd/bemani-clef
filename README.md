bemani2wav
============

bemani2wav is a player for Beatmania IIDX (and, eventually, Pop'n Music) arcade music tracks.

Prebuilt plugins are available in the [downloads section](https://bitbucket.org/ahigerd/bemani2wav/downloads/):

* (Windows) Winamp: [in_bemani2wav.dll](https://bitbucket.org/ahigerd/bemani2wav/downloads/in_bemani2wav.dll)
* (Windows) Foobar2000: [foo_input_bemani2wav.dll](https://bitbucket.org/ahigerd/bemani2wav/downloads/foo_input_bemani2wav.dll)
* (Ubuntu) Audacious: [aud_bemani2wav.so](https://bitbucket.org/ahigerd/bemani2wav/downloads/aud_bemani2wav.so)

Building
--------
To build on POSIX platforms or MinGW using GNU Make, simply run `make`. The following make
targets are recognized:

* `cli`: builds the command-line tool. (default)
* `plugins`: builds all plugins supported by the current platform.
* `all`: builds the command-line tool and all plugins supported by the current platform.
* `debug`: builds a debug version of the command-line tool.
* `audacious`: builds just the Audacious plugin, if supported.
* `winamp`: builds just the Winamp plugin, if supported.
* `foobar`: builds just the Foobar2000 plugin, if supported.
* `aud_bemani2wav_d.dll`: builds a debug version of the Audacious plugin, if supported.
* `in_bemani2wav_d.dll`: builds a debug version of the Winamp plugin, if supported.

The following make variables are also recognized:

* `CROSS=mingw`: If building on Linux, use MinGW to build Windows binaries.
* `CROSS=msvc`: Use Microsoft Visual C++ to build Windows binaries, using Wine if the current
  platform is not Windows. (Required to build the Foobar2000 plugin.)
* `WINE=[command]`: Sets the command used to run Wine. (Default: `wine`)

To build using Microsoft Visual C++ on Windows without using GNU Make, run `buildvs.cmd`,
optionally with one or more build targets. The following build targets are supported:

* `cli`: builds the command-line tool. (default)
* `plugins`: builds the Winamp and Foobar2000 plugins.
* `all`: builds the command-line tool and the Winamp and Foobar2000 plugins.
* `winamp`: builds just the Winamp plugin.
* `foobar`: builds just the Foobar2000 plugin.

Separate debug builds are not supported with Microsoft Visual C++, but the build flags may be
edited in `msvc.mak`.

License
-------
bemani2wav is copyright (c) 2020 Adam Higerd and distributed under the terms of the
[MIT license](LICENSE.md).

This project is based upon seq2wav, copyright (c) 2020 Adam Higerd and distributed
under the terms of the [MIT license](LICENSE.md).
