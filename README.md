# A program for timing mechanical watches

The program tg is distributed under the GNU GPL license version 2. The full
source code of tg is available at
[https://github.com/xyzzy42/tg](https://github.com/xyzzy42/tg) and its
copyright belongs to the respective contributors.

This repository is a community-maintained fork that carries additional fixes
and features on top of the upstream
[xyzzy42/tg](https://github.com/xyzzy42/tg) tree, with pre-built macOS
binaries published via a Homebrew tap (see [Macintosh](#macintosh) below).

A comprehensive [user manual](docs/user-manual.md) is available covering
installation, configuration, operation, and troubleshooting. Additional
discussion can be found in this
[thread at WUS](http://forums.watchuseek.com/f6/open-source-timing-software-2542874.html),
in particular the calibration procedure is described at
[this post](http://forums.watchuseek.com/f6/open-source-timing-software-2542874-post29970370.html).

## Install instructions

Tg is known to work under Microsoft Windows, OS X, and Linux. Moreover it
should be possible to compile the source code under most modern UNIX-like
systems. See the sub-sections below for the details.

### Windows

Binaries can be found at https://tg.ciovil.li

Unfortunately, these packages have not been updated since 2017.  Help from
someone who can build the Windows installer version would be appreciated.
You'll need to [build from source](#compiling-on-windows) to get any features
from the last five plus years.

### Macintosh

This fork is distributed as a [Homebrew](http://brew.sh) tap, built
automatically for each release.  You need Homebrew installed first
(instructions on http://brew.sh).

Add the tap, trust it, and install:

	brew tap colincoleman/tg
	brew trust colincoleman/tg
	brew install tg-timer

The `brew trust` step is required by recent versions of Homebrew before it
will install a formula from a third-party tap.

You can now launch tg by typing:

	tg-timer &

To upgrade later, run `brew update && brew upgrade tg-timer`.

The original formula was prepared by GitHub user
[dmnc](https://github.com/dmnc) and later updated by
[xyzzy42](https://github.com/xyzzy42); the tap above tracks this fork.

### Debian or Debian-based (e.g. Mint, Ubuntu)

A binary `.deb` package is built for each release of this fork and attached to
the [GitHub Releases page](https://github.com/colincoleman/tg/releases), for
both `amd64` (Intel/AMD PCs) and `arm64` (Apple Silicon, Raspberry Pi, ARM
servers).  Check your architecture with `dpkg --print-architecture`, download
the matching package, and install it with `apt`, for example:

```sh
# amd64 (Intel/AMD):
sudo apt install ./tg-timer_*_amd64.deb
# arm64 (Apple Silicon, Raspberry Pi, ...):
sudo apt install ./tg-timer_*_arm64.deb
```

`apt` will pull in the required GTK, PortAudio, and FFTW runtime libraries
automatically.  The packages are built on Ubuntu 22.04, so they install on
22.04 and newer (including Debian-based derivatives).  You can then launch tg
by typing `tg-timer &`.

### Fedora, CentOS or other Redhat-based

Binary RPM packages are available from https://copr.fedorainfracloud.org/coprs/tpiepho/tg-timer/

This COPR repository can be added to dnf's list with:
```sh
dnf copr enable tpiepho/tg-timer
```
Then tg-timer can be installed with `dnf install tg-timer`, or with any dnf
based GUI package installer.

## Compiling from sources

The source code of tg can probably be built by any C99 compiler, however only
gcc and clang have been tested.  You need the following libraries:  gtk+3,
portaudio2, fftw3 (all available as open source).

The optional plotting features for audio filter response graphs and audio
spectrograms require Python.  The follow Python modules are needed for all
features:  numpy, scipy, matplotlib, libtfr.

### Normal build
Get source:
```sh
git clone https://github.com/xyzzy42/tg.git
cd tg

# Optional, check out a specific branch other than the default
git checkout new-stuff
```

Build it:
```sh
./autogen.sh
./configure
make
```

### Debug build
After the steps of a normal build, above, run:
```sh
make tg-timer-dbg
```

### Compiling on Windows

It is suggested to use the MSYS2 platform. First install MSYS2 according
to the instructions at [http://www.msys2.org](http://www.msys2.org).  The
[UCRT64 environment](https://www.msys2.org/docs/environments/) appears to work
best.

From the MSYS2 UCRT64 terminal issue the following commands to install dependencies:

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc pkg-config mingw-w64-ucrt-x86_64-gtk3 mingw-w64-ucrt-x86_64-portaudio mingw-w64-ucrt-x86_64-fftw make git autoconf automake libtool
pacman -S mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python-scipy mingw-w64-ucrt-x86_64-python-matplotlib mingw-w64-ucrt-x86_64-lapack mingw-w64-ucrt-x86_64-cython mingw-w64-ucrt-x86_64-python-pip
```

Add the folder "C:\msys64\ucrt64\bin" to the system path variable

Install the libtfr package with:

```sh
pip install libtfr
```

If that doesn't work, try building it manually:

```sh
git clone https://github.com/melizalab/libtfr.git
cd libtfr
python setup.py install
```

Then follow the [normal build](#normal-build) instructions to clone the TG
repository and build it.

If the "Signal" or "Filter Chain" dialogs do not work, this appears to be some
sort of [library issue with Python on Windows](https://github.com/xyzzy42/tg/issues/8#issuecomment-2351649260).
Try copying the `tg-timer.exe` file to `C:\msys64\ucrt64\bin` and run it from there.

### Compiling on Debian

To compile tg on Debian, install these dependencies:

```sh
sudo apt-get install libgtk-3-dev libjack-jackd2-dev portaudio19-dev libfftw3-dev git autoconf automake libtool
```

Additional software is necessary for the optional Python graphing system, see
Fedora section for more information.  Debian specific instructions welcome.

Then follow the [normal build](#normal-build) instructions to clone the
repository and build it.

The package libjack-jackd2-dev is not necessary, it only works around a
known bug (https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=718221).

### Compiling on Fedora

To compile tg on Fedora, install these dependencies:

```sh
sudo dnf install fftw-devel portaudio-devel gtk3-devel autoconf automake libtool
```

To use the Python graphing code (filter response and audio spectrogram) one
should also install:
```sh
sudo dnf install python3-devel python3-numpy python3-scipy python3-matplotlib
pip install --user libtfr
```

Then follow the [normal build](#normal-build) instructions to clone the
repository and build it.

To build an RPM on Fedora or another RPM based distro, install the build
prerequisites and checkout the source as for compiling (above), then run
`rpmbuild` to create the RPM:

```sh
rpmbuild --build-in-place -bb packaging/tg-timer.spec
```


### Compiling on Macintosh

To build on MacOS, install the follow dependencies with
[Homebrew](http://brew.sh).  If you do not have Homebrew, the link above will
explain the install.

```sh
brew install pkg-config autoconf automake libtool gtk+3 portaudio fftw gnome-icon-theme
```

To use the Python graphing code (filter response and audio spectrogram) one
should also install:

```sh
brew install python numpy scipy
pip3 install matplotlib libtfr
```

If you have multiple versions of python3 installed, you might need to use a
specific version of pip, e.g. `pip3.11` in the above command.

Then follow the [normal build](#normal-build) instructions to clone the
repository and build it.

If you have multiple versions of Python installed and the configure script does
not detect the correct one, i.e. the one for which numpy, matplotlib, libtfr,
etc. have been installed for, then run the configure script as:

```sh
PYTHON=python3.11 ./configure
```

Where `python3.11` is the correct version to use.
