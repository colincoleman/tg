---
title: "tg-timer User Manual"
description: "Comprehensive user manual for tg-timer, an open-source mechanical watch timing application"
license: "GPL-2.0-only"
---

# tg-timer User Manual

## 1. Introduction

tg-timer is an open-source program for timing mechanical watches. It listens to the
tick sound of a watch movement through a microphone and computes key timing parameters:
Rate (accuracy in seconds per day), Beat Error (regularity of the tic-toc alternation
in milliseconds), and Amplitude (the angular swing of the balance wheel in degrees).
These measurements help watchmakers and enthusiasts evaluate whether a movement is
running well and identify problems that need adjustment.

tg-timer runs on Windows (via MSYS2), macOS, and Linux. It is licensed under the
[GNU General Public License, version 2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)
(GPL v2).

Source code and issue tracker:
[https://github.com/vacaboja/tg](https://github.com/vacaboja/tg)

## Table of Contents

- [1. Introduction](#1-introduction)
- [2. Installation](#2-installation)
  - [2.1 Windows (MSYS2 UCRT64)](#21-windows-msys2-ucrt64)
  - [2.2 macOS (Homebrew)](#22-macos-homebrew)
  - [2.3 Debian / Ubuntu](#23-debian--ubuntu)
  - [2.4 Fedora / RPM](#24-fedora--rpm)
- [3. Getting Started](#3-getting-started)
- [4. Main Interface](#4-main-interface)
  - [4.1 BPH Selector](#41-bph-selector)
  - [4.2 Lift Angle](#42-lift-angle)
  - [4.3 Calibration Control](#43-calibration-control)
  - [4.4 Paperstrip Display](#44-paperstrip-display)
  - [4.5 Waveform and Period Displays](#45-waveform-and-period-displays)
  - [4.6 Snapshot Management](#46-snapshot-management)
  - [4.7 Command Menu](#47-command-menu)
  - [4.8 Numeric Outputs](#48-numeric-outputs)
- [5. Measurement Parameters](#5-measurement-parameters)
- [6. Calibration](#6-calibration)
- [7. Audio Setup](#7-audio-setup)
- [8. Filter Chain](#8-filter-chain)
- [9. Signal Analysis](#9-signal-analysis)
  - [9.1 True Peak Programme Meter](#91-true-peak-programme-meter)
  - [9.2 Spectrogram](#92-spectrogram)
  - [9.3 Signal Strength](#93-signal-strength)
- [10. Saving and Loading Data](#10-saving-and-loading-data)
- [11. Configuration Persistence](#11-configuration-persistence)
- [12. Light Algorithm Mode](#12-light-algorithm-mode)
- [13. Troubleshooting](#13-troubleshooting)
- [14. Glossary](#14-glossary)

## 2. Installation

This section provides platform-specific instructions for installing tg-timer. Each
subsection lists the required dependencies, exact package manager commands, build
steps, and a verification procedure. Optional dependencies for the plotting features
(filter response graphs and spectrograms — frequency-over-time visualizations of the
audio signal) are listed separately.

### 2.1 Windows (MSYS2 UCRT64)

tg-timer on Windows is built from source using the MSYS2 platform with the UCRT64
environment. MSYS2 provides a Unix-like shell and package manager (pacman) on Windows.

#### Prerequisites

Install MSYS2 following the instructions at [https://www.msys2.org](https://www.msys2.org).
Once installed, open the **MSYS2 UCRT64** terminal for all subsequent steps.

#### Install Required Dependencies

From the MSYS2 UCRT64 terminal, install the compiler, build tools, and required
libraries:

```sh
pacman -S mingw-w64-ucrt-x86_64-gcc pkg-config mingw-w64-ucrt-x86_64-gtk3 mingw-w64-ucrt-x86_64-portaudio mingw-w64-ucrt-x86_64-fftw make git autoconf automake libtool
```

This installs:

- **Build tools:** gcc, make, git, autoconf, automake, libtool, pkg-config
- **Required libraries:** gtk3, portaudio, fftw3

#### Install Optional Python/Plotting Dependencies

For filter response graphs and audio spectrograms, install the Python packages:

```sh
pacman -S mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python-scipy mingw-w64-ucrt-x86_64-python-matplotlib mingw-w64-ucrt-x86_64-lapack mingw-w64-ucrt-x86_64-cython mingw-w64-ucrt-x86_64-python-pip
```

Then install the libtfr package:

```sh
pip install libtfr
```

If that fails, build libtfr manually:

```sh
git clone https://github.com/melizalab/libtfr.git
cd libtfr
python setup.py install
```

#### PATH Setup

Add the UCRT64 bin directory to the Windows system PATH so that tg-timer and its
libraries can be found outside the MSYS2 terminal:

```text
C:\msys64\ucrt64\bin
```

To set this, open **System Properties → Environment Variables** and append the path
above to the `Path` variable.

#### Build from Source

Clone the repository and build:

```sh
git clone https://github.com/xyzzy42/tg.git
cd tg
./autogen.sh
./configure
make
```

#### Verification

Launch tg-timer from the MSYS2 UCRT64 terminal:

```sh
./tg-timer
```

The application window should appear. If the window opens and no errors are printed,
the installation is successful.

#### Troubleshooting: pkg-config Errors

If `./configure` fails with a message like:

```text
Package xyz was not found in the pkg-config search path
```

This means a required library is missing. Install the corresponding MSYS2 package
for the missing library. For example, if `fftw3` is not found:

```sh
pacman -S mingw-w64-ucrt-x86_64-fftw
```

Then re-run `./configure`.

### 2.2 macOS (Homebrew)

tg-timer on macOS can be installed via a Homebrew formula or built from source.
[Homebrew](https://brew.sh) is required for both approaches.

#### Option A: Install via Homebrew Formula

The simplest method is to install tg-timer directly from the tap:

```sh
brew install --HEAD xyzzy42/horology/tg-timer
```

This builds tg-timer from the latest source with all required dependencies resolved
automatically.

#### Option B: Build from Source

If you prefer to build manually or need to customize the build, follow the steps
below.

##### Install Required Dependencies

Install the build tools and required libraries:

```sh
brew install pkg-config autoconf automake libtool gtk+3 portaudio fftw gnome-icon-theme
```

This installs:

- **Build tools:** pkg-config, autoconf, automake, libtool
- **Required libraries:** gtk+3, portaudio, fftw
- **Icon theme:** gnome-icon-theme (required for UI icons on macOS)

##### Install Optional Python/Plotting Dependencies

For filter response graphs and audio spectrograms, install Python and the required
modules:

```sh
brew install python numpy scipy
pip3 install matplotlib libtfr
```

If you have multiple versions of Python 3 installed, use the version-specific pip
command (e.g., `pip3.11`) to ensure packages are installed for the correct version.

##### Build from Source

Clone the repository and build:

```sh
git clone https://github.com/xyzzy42/tg.git
cd tg
./autogen.sh
./configure
make
```

##### Multiple Python Versions Workaround

If the configure script does not detect the correct Python version (i.e., the one
where numpy, matplotlib, and libtfr are installed), specify it explicitly:

```sh
PYTHON=python3.11 ./configure
```

Replace `python3.11` with the version that has the required packages installed, then
run `make` as normal.

#### Verification

Launch tg-timer in the background:

```sh
tg-timer &
```

The application window should appear. If the window opens and no errors are printed
to the terminal, the installation is successful.

#### Troubleshooting: pkg-config Errors

If `./configure` fails with a message like:

```text
Package xyz was not found in the pkg-config search path
```

This means a required library is missing. Install the corresponding Homebrew package
for the missing library. For example, if `fftw3` is not found:

```sh
brew install fftw
```

Then re-run `./configure`.

### 2.3 Debian / Ubuntu

On Debian, Ubuntu, Mint, and other Debian-based distributions, tg-timer is built from
source after installing dependencies through apt.

#### Install Required Dependencies

Install the compiler, build tools, and required libraries:

```sh
sudo apt-get install libgtk-3-dev libjack-jackd2-dev portaudio19-dev libfftw3-dev git autoconf automake libtool
```

This installs:

- **Build tools:** git, autoconf, automake, libtool
- **Required libraries:** libgtk-3-dev (gtk+3), portaudio19-dev (portaudio2), libfftw3-dev (fftw3)
- **Workaround:** libjack-jackd2-dev is not strictly necessary but works around a
  [known Debian bug](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=718221) that
  prevents portaudio from building without a JACK development package installed.

#### Optional Python/Plotting Dependencies

For the filter response graphs and audio spectrogram features, Python 3 and several
modules are required. See the [Fedora / RPM](#24-fedora--rpm) section for the list of
Python packages needed (python3-devel, numpy, scipy, matplotlib, libtfr). On Debian,
install the equivalent packages via apt and pip:

```sh
sudo apt-get install python3-dev python3-numpy python3-scipy python3-matplotlib
pip install --user libtfr
```

Debian-specific packaging instructions for the Python dependencies are welcome as
community contributions.

#### Build from Source

Clone the repository and build:

```sh
git clone https://github.com/xyzzy42/tg.git
cd tg
./autogen.sh
./configure
make
```

#### Verification

Launch tg-timer:

```sh
./tg-timer &
```

The application window should appear. If the window opens and no errors are printed,
the installation is successful.

#### Troubleshooting: pkg-config Errors

If `./configure` fails with a message like:

```text
Package xyz was not found in the pkg-config search path
```

This means a required development library is missing. Install the corresponding
package for the missing library. Common mappings:

| pkg-config name | Debian package |
|-----------------|----------------|
| gtk+-3.0 | libgtk-3-dev |
| portaudio-2.0 | portaudio19-dev |
| fftw3 | libfftw3-dev |

Install the missing package and re-run `./configure`.

### 2.4 Fedora / RPM

On Fedora and other RPM-based distributions (CentOS, RHEL), tg-timer can be installed
from a pre-built RPM package via the COPR repository, or compiled from source.

#### Option A: Install from COPR Repository

The simplest method is to enable the COPR repository and install the pre-built package:

```sh
sudo dnf copr enable tpiepho/tg-timer
sudo dnf install tg-timer
```

This installs tg-timer and all required dependencies automatically. You can also use
any dnf-based GUI package installer after enabling the repository.

#### Option B: Build from Source

##### Install Required Dependencies

Install the build tools and required libraries:

```sh
sudo dnf install fftw-devel portaudio-devel gtk3-devel autoconf automake libtool
```

This provides:

- **Build tools:** autoconf, automake, libtool (gcc, make, git, and pkg-config are
  typically pre-installed on Fedora)
- **Required libraries:** fftw3 (via fftw-devel), portaudio2 (via portaudio-devel),
  gtk+3 (via gtk3-devel)

##### Install Optional Python/Plotting Dependencies

For filter response graphs and audio spectrograms, install the Python packages:

```sh
sudo dnf install python3-devel python3-numpy python3-scipy python3-matplotlib
pip install --user libtfr
```

The `--user` flag installs libtfr into your home directory without requiring root.

##### Build Steps

Clone the repository and build:

```sh
git clone https://github.com/xyzzy42/tg.git
cd tg
./autogen.sh
./configure
make
```

##### RPM Build Option

To build an installable RPM package instead of a plain binary, install the build
prerequisites and clone the source as described above, then run:

```sh
rpmbuild --build-in-place -bb packaging/tg-timer.spec
```

The resulting RPM can be installed with `dnf install` on the local system or
distributed to other compatible machines.

#### Verification

Launch tg-timer:

```sh
tg-timer &
```

If installed from source (Option B), run from the build directory:

```sh
./tg-timer &
```

The application window should appear without errors.

#### Troubleshooting: pkg-config Errors

If `./configure` fails with a message like:

```text
Package xyz was not found in the pkg-config search path
```

This means a required development library is missing. Install the corresponding
`-devel` package for the missing library. For example, if `fftw3` is not found:

```sh
sudo dnf install fftw-devel
```

Then re-run `./configure`.

## 3. Getting Started

This section walks you through timing a watch for the first time. By the end of
these steps you will see live measurements of your watch's accuracy, beat regularity,
and balance wheel swing.

### Quick-Start Steps

1. **Launch tg-timer.** Run `tg-timer` from your terminal (or `./tg-timer` if built
   from source). The main window opens with a blank paperstrip display (the scrolling
   timing graph that plots each beat's deviation) and the numeric output area showing
   dashes.

2. **Position the watch near the microphone.** Place the watch caseback-down directly
   on or within a few centimetres of your computer's built-in microphone (or an
   external microphone). The caseback transmits the tick sound most clearly. A quiet
   environment helps — hard surfaces carry vibrations well, while soft padding can
   muffle the signal. Adjust the position until the Signal Strength indicator (the
   bar display in the main window) shows at least 1 bar.

3. **Verify signal detection.** Check the Signal Strength indicator. This shows how
   reliably tg-timer is detecting the tick sound:

   | Bars | Meaning |
   |------|---------|
   | 0 | No signal detected — the application cannot hear the watch |
   | 1 | Minimal detection — tick is audible but noisy |
   | 2 | Partial lock — measurements are appearing but may be unstable |
   | 3 | Good lock — measurements are reliable |
   | 4 | Full acquisition — maximum accuracy, amplitude measurement enabled |

   Signal Strength is the confidence level of tick detection: higher bars mean more
   consistent identification of each beat. At 4 bars the application has full lock on
   the tick pattern and can compute all measurements including amplitude.

   If the indicator stays at 0 bars, see [Troubleshooting](#13-troubleshooting) for
   microphone and audio device verification steps.

4. **Select the BPH (Beats Per Hour).** BPH is the oscillation frequency of the
   watch movement — it determines how many ticks occur per hour and is essential for
   accurate rate calculation. Select the correct value using one of three methods:

   - **Preset list:** Choose from the dropdown which includes common movement
     frequencies: 12000, 14400, 17280, 18000, 19800, 21600, 25200, 28800, 36000,
     43200, and 72000.
   - **Custom entry:** Type any value between 8100 and 72000 directly into the BPH
     field if your movement uses a non-standard frequency.
   - **"guess" auto-detection:** Select "guess" from the dropdown and tg-timer will
     determine the BPH automatically from the detected tick period once signal is
     acquired. The guessed value appears in the output area. This is useful when you
     do not know the movement's BPH, but selecting the correct preset is preferred
     for best accuracy.

   If you are unsure of the BPH, check the movement's specification sheet or use
   "guess" mode to identify it.

5. **Read the outputs.** Once signal is acquired and BPH is set, tg-timer displays
   three primary measurements:

   - **Rate** — the watch's accuracy expressed in seconds per day (s/d). A positive
     value means the watch is gaining time (running fast); a negative value means it
     is losing time (running slow). For example, +5.2 s/d means the watch gains
     about 5 seconds each day. A well-adjusted mechanical watch typically runs within
     ±10 s/d.

   - **Beat Error** — the asymmetry between the tic and toc intervals, expressed in
     milliseconds (ms). In a perfectly regulated movement the tic-to-toc and
     toc-to-tic intervals are equal, giving a beat error of 0 ms. Lower values are
     better; under 1.0 ms indicates good regulation.

   - **Amplitude** — the angular extent of the balance wheel's swing, measured in
     degrees. This reflects the energy in the mainspring and the health of the gear
     train. Healthy values typically range from 180° to 315°. Amplitude measurement
     requires 4-bar signal strength and a correct Lift Angle setting (default 52°).

   These values update continuously as tg-timer collects more data. Allow a few
   seconds of stable signal for the readings to settle.

## 4. Main Interface

This section describes the controls and displays in the main tg-timer window. Each
subsection covers one group of interface elements, their parameters, and how to
interact with them.

### 4.1 BPH Selector

The BPH (Beats Per Hour) selector sets the expected oscillation frequency of the watch
movement being timed. tg-timer needs this value to compute Rate and Beat Error from
the detected tick intervals.

**Presets.** The dropdown provides commonly used BPH values:

| BPH | Typical use |
|-------|-------------|
| 12000 | Older pocket watches |
| 14400 | Vintage wristwatches |
| 17280 | Some vintage calibers |
| 18000 | Common vintage frequency |
| 19800 | Mid-century movements |
| 21600 | Common modern frequency (6 beats/sec) |
| 25200 | Some Seiko movements |
| 28800 | Common modern frequency (8 beats/sec) |
| 36000 | High-beat movements (10 beats/sec) |
| 43200 | Ultra-high-beat movements |
| 72000 | Spring Drive and similar |

**Custom entry.** If your movement's BPH is not in the preset list, type the value
directly into the selector. Valid range: 8100 to 72000.

**Guess mode.** Select "guess" from the dropdown to let tg-timer auto-detect the BPH
from the measured tick period. Once signal is acquired, the application determines the
BPH automatically and displays the guessed value in the numeric output area. Guess
mode is useful when you do not know the movement's frequency — acquire a signal first,
then read the detected BPH and confirm or adjust it.

### 4.2 Lift Angle

The Lift Angle is the angular displacement of the balance wheel caused by the impulse
from the escapement. It is specific to each movement caliber and is required for
accurate Amplitude readings.

- **Range:** 10° to 90°
- **Default:** 52°

The lift angle value affects only the Amplitude measurement. Rate and Beat Error are
independent of the lift angle setting. If the lift angle is incorrect, the displayed
amplitude will be proportionally wrong, but timing measurements remain accurate.

Set the lift angle to match your movement's specification. If the exact value is
unknown, the 52° default is a reasonable approximation for many modern calibers.
Consult the movement's technical documentation or a watch caliber database for the
precise value.

### 4.3 Calibration Control

The Calibration spin button compensates for the difference between your audio device's
nominal sample rate and its actual sample rate. Small deviations in sample rate cause
a systematic offset in Rate readings; calibration corrects this.

- **Display format:** ±X.X s/d (seconds per day)
- **Range:** −100.0 to +100.0 s/d
- **Default:** 0.0 s/d (no compensation)

The value represents how many seconds per day of offset to subtract from the raw
measurement. A positive calibration value means the audio device's actual sample rate
is slightly faster than nominal, causing readings to appear more positive than reality.

The calibration value can be entered manually or set automatically by running the
calibration procedure (see [6. Calibration](#6-calibration)).

### 4.4 Paperstrip Display

The Paperstrip is the scrolling timing display in the center of the main window. It
plots each beat's deviation from the expected interval, producing a visual pattern
analogous to the paper-strip output of a traditional mechanical timegrapher.

**Timing dots.** Each dot represents one detected beat (a tic or toc event). The
horizontal position of a dot indicates its timing deviation from the ideal interval.
Dots aligned vertically mean the watch is running at a steady rate. A slant to the
right indicates gaining; a slant to the left indicates losing. Two parallel lines of
dots (one for tic, one for toc) are normal — the spacing between the lines reflects
the Beat Error.

**Scrolling.** New beats appear at the top (or right in vertical mode) and older beats
scroll away as new data arrives.

**Controls:**

| Control | Action |
|---------|--------|
| Zoom in / out | Adjust the horizontal scale to show more or less timing detail per dot |
| Shift left / right | Pan the display horizontally to inspect dots that have drifted off-center |
| Center | Reset the horizontal position so the current trace is centered in the display |
| Clear | Erase all accumulated timing dots and start fresh |

Zoom is useful when the dots are tightly clustered (zoom in to see detail) or spread
beyond the visible area (zoom out to see the overall pattern). Shift and center help
when the trace has drifted to one side of the display due to a large rate offset.

### 4.5 Waveform and Period Displays

**Tic and toc waveforms.** The waveform displays show the detected pulse shape of the
most recent tic and toc events. Each display renders the audio waveform of a single
beat, letting you visually inspect the impulse and escapement sounds. Clean, repeatable
waveforms indicate a strong signal; noisy or irregular shapes suggest the microphone
pickup needs improvement.

**Period display.** Below the waveforms, the period display shows the measured time
interval between successive beats. This is the raw timing measurement from which Rate
is derived. The period is displayed in milliseconds and updates with each new beat
detection.

### 4.6 Snapshot Management

A **snapshot** is a captured state of the timing display — it preserves the paperstrip
data, waveform, and numeric readings at the moment of capture so you can review or
compare measurements later.

**Creating a snapshot.** Click the snapshot button (camera icon) to capture the current
timing display into a new tab. Each snapshot appears as a tab in the tab bar below the
main display. You can create multiple snapshots during a timing session to record
readings at different times or in different positions.

**Naming a snapshot.** Double-click a snapshot tab to edit its name. Type a descriptive
label (e.g., "dial up", "crown left") and press Enter to confirm. Named snapshots are
easier to identify when you have several open or when saving to a file.

**Reordering snapshots.** Drag a snapshot tab left or right to rearrange the tab order.
This lets you organize tabs in a sequence that makes sense for your workflow (e.g.,
grouping by position or chronological order).

**Closing a snapshot.** Click the close button on a snapshot tab to remove it. The tab
and its captured data are discarded. To close all snapshot tabs at once, use the
"Close all snapshots" command in the menu (see [4.7 Command Menu](#47-command-menu)).

### 4.7 Command Menu

The command menu provides access to file operations, display modes, dialogs, and
application control. The menu items are listed below in order:

| Menu Item | Action |
|-----------|--------|
| **Open** | Load snapshots from a `.tgj` file (tg-timer's native data format). Each snapshot in the file is opened as a new tab. |
| **Save current display** | Save the active tab's snapshot to a `.tgj` file. |
| **Save all snapshots** | Save all open snapshot tabs into a single `.tgj` file. |
| **Light algorithm** | Toggle Light Algorithm mode (checkbox). When checked, tg-timer uses reduced-precision analysis for lower CPU usage. See [12. Light Algorithm Mode](#12-light-algorithm-mode). |
| **Calibrate** | Start the calibration procedure (checkbox). When checked, tg-timer begins collecting calibration data. See [6. Calibration](#6-calibration). |
| **Vertical** | Toggle vertical paperstrip layout (checkbox). When checked, the paperstrip scrolls horizontally with new beats appearing on the right, resembling a traditional paper-tape timegrapher. |
| **Signal** | Open the Signal dialog, which provides the True Peak Programme Meter (TPPM) for monitoring audio input levels and the spectrogram for frequency analysis. See [9. Signal Analysis](#9-signal-analysis). |
| **Filter Chain** | Open the Filter Chain dialog for configuring biquadratic audio filters applied to the input signal. See [8. Filter Chain](#8-filter-chain). |
| **Audio setup** | Open the Audio Setup dialog for selecting the input device, sample rate, and high-pass filter cutoff. See [7. Audio Setup](#7-audio-setup). |
| **Close all snapshots** | Close all open snapshot tabs at once, discarding their captured data. |
| **Quit** | Exit the application. Current settings are saved to the configuration file on exit. |

Checkbox items (Light algorithm, Calibrate, Vertical) show a check mark when active
and can be toggled on and off by selecting them again.

### 4.8 Numeric Outputs

The numeric output area displays the live measurement values computed from the
detected tick signal:

| Output | Unit | Description |
|--------|------|-------------|
| **Rate** | s/d (seconds per day) | The watch's accuracy. Positive values indicate gaining (running fast); negative values indicate losing (running slow). |
| **Beat Error** | ms (milliseconds) | The asymmetry between tic and toc intervals. Lower is better; 0 ms means perfectly symmetric beats. |
| **Amplitude** | ° (degrees) | The angular swing of the balance wheel. Requires 4-bar signal strength and a correct Lift Angle setting to display. |
| **Guessed BPH** | BPH | Shown when "guess" is selected in the BPH selector. Displays the auto-detected beats-per-hour value once tg-timer determines the tick frequency from the signal. |

When no signal is present or data is insufficient, the outputs display dashes (—)
instead of numeric values. As signal is acquired, values appear and update
continuously with each new beat detection. Allow a few seconds of stable signal for
readings to settle to reliable values.

The guessed BPH value only appears when the BPH selector is set to "guess" and
tg-timer has acquired enough signal to determine the tick frequency. Once displayed,
you can confirm the guessed value by selecting the matching preset from the BPH
dropdown for best accuracy.

## 5. Measurement Parameters

This section explains how to interpret the three primary measurement values displayed
by tg-timer — Rate, Beat Error, and Amplitude — including reference ranges for
evaluating a watch's condition and the factors that affect measurement accuracy.

### 5.1 Rate

Rate is the watch's accuracy expressed in seconds per day (s/d). It tells you how much
time the watch gains or loses over a 24-hour period compared to perfect timekeeping.

- **Positive values** mean the watch is **gaining** time (running fast). For example,
  +6.0 s/d means the watch gains about 6 seconds each day.
- **Negative values** mean the watch is **losing** time (running slow). For example,
  −4.0 s/d means the watch loses about 4 seconds each day.

**Reference ranges:**

| Rate | Interpretation |
|------|----------------|
| Within ±5 s/d | Well-adjusted mechanical watch |
| Within ±10 s/d | Typical performance for a mechanical watch in good condition |
| Beyond ±10 s/d | May indicate a need for service or regulation |

Rate is derived from the measured period between beats compared to the expected period
(determined by the BPH setting). A correct BPH selection and calibration value are
important for an accurate Rate reading.

### 5.2 Beat Error

Beat Error measures the asymmetry between the tic and toc intervals, expressed in
milliseconds (ms). In a perfectly regulated movement, the time from tic-to-toc equals
the time from toc-to-tic, giving a Beat Error of 0 ms. Any difference between these
two half-periods appears as Beat Error.

- **Under 1.0 ms** indicates a well-regulated movement with good symmetry between
  the tic and toc impulses.
- **Above 1.0 ms** suggests the impulse pin or regulator may benefit from adjustment.

The maximum displayed value is **99.9 ms**. Values at or near this limit typically
indicate a severely misaligned impulse pin or a signal detection issue rather than a
true measurement.

Beat Error is unaffected by the Lift Angle setting — it depends only on the relative
timing of the tic and toc events.

### 5.3 Amplitude

Amplitude is the angular extent of the balance wheel's swing, measured in degrees. It
reflects how much energy the mainspring is delivering through the gear train to the
balance wheel. A declining amplitude often indicates a mainspring that needs servicing
or dried lubricants increasing friction.

**Healthy range:** 180° to 315°

- **Below 180°** indicates the movement is running with very low energy. This often
  accompanies an unwound or nearly exhausted mainspring, heavy lubrication issues, or
  worn components.
- **Above 315°** (sometimes called "overbanking") means the balance wheel is swinging
  so far that it risks interfering with the pallet fork on the return stroke. This
  can happen when a freshly wound mainspring delivers excessive torque.

**Display behavior:** tg-timer shows **0** for Amplitude when the computed value falls
below 135° or above 360°. These values are outside the physically valid range for
normal operation and indicate either an unreliable measurement (e.g., weak signal) or
an extraordinary condition. A reading of 0 typically means the signal is not strong
enough for amplitude computation rather than a true zero-degree swing.

Amplitude measurement requires 4-bar signal strength to produce reliable results.

### 5.4 Lift Angle and Amplitude Accuracy

Amplitude calculation depends on the Lift Angle setting matching the specific watch
movement. The lift angle is the angular displacement of the balance wheel caused by
the impulse from the escapement, and it varies by caliber (typically between 44° and
56° for modern movements).

**Key points:**

- An **incorrect lift angle** causes the displayed amplitude to be **proportionally
  wrong**. For example, if the true lift angle is 50° but tg-timer is set to 52°, the
  displayed amplitude will be off by roughly 4%. The error scales linearly with the
  mismatch.
- **Rate and Beat Error are unaffected** by the lift angle setting. These measurements
  depend only on beat timing, not on the energy/swing computation.
- If you see amplitude values that seem inconsistent with the watch's condition (e.g.,
  a freshly wound watch showing low amplitude), check whether the lift angle matches
  the movement's specification.

To find the correct lift angle for your movement, consult:

- The movement manufacturer's technical documentation
- A watch caliber database (several are available online)
- Watchmaker reference books listing common caliber specifications

The default value of 52° is a reasonable approximation for many modern movements, but
using the exact value for your specific caliber produces the most accurate amplitude
readings.

### 5.5 Signal Strength and Stabilization

Measurements require adequate signal strength to be reliable. tg-timer needs to
consistently detect individual tick events before it can compute meaningful values.

- **Rate and Beat Error** begin appearing once Signal Strength reaches 1 bar or
  higher, but readings are most stable at 3–4 bars.
- **Amplitude** requires 4-bar signal strength. Below this level the display shows 0
  or dashes.

**Stabilization time:** When you first position a watch or change settings, allow
several seconds of continuous detection for the measurements to stabilize. tg-timer
averages over multiple beats to reduce noise, so initial readings may fluctuate before
settling to reliable values. The paperstrip display is a good visual indicator —
when the dots form a consistent pattern, the numeric readings have stabilized.

If readings remain unstable after 10–15 seconds of 4-bar signal, verify that the BPH
setting is correct for your movement and consider running a
[calibration](#6-calibration) to compensate for any sample-rate drift in your audio
device.

## 6. Calibration

Calibration compensates for the difference between your audio device's nominal sample
rate and its actual sample rate. This matters because tg-timer derives all timing
measurements from sample counting — if the actual rate at which the audio hardware
captures samples differs from the rate it reports (the nominal rate), every Rate
reading will carry a systematic offset. Even a small discrepancy of a few parts per
million translates into a measurable error in seconds per day.

For example, if your audio device's actual sample rate is slightly faster than the
nominal 44100 Hz, tg-timer will perceive beat intervals as slightly shorter than they
really are, causing Rate readings to appear more positive (faster) than the watch
actually runs. Calibration measures this drift and corrects for it.

### Prerequisites

Before starting calibration, ensure:

1. **Signal Strength is at 4 bars (full lock).** The calibration procedure requires
   a continuous, uninterrupted stream of accurately detected beats. Anything less than
   full signal strength risks corrupting the dataset.

2. **BPH is set to the correct known value.** The BPH selector must show the actual
   frequency of the watch movement — do not use "guess" mode during calibration. The
   calibration algorithm compares detected beat periods against the expected period
   derived from BPH, so an incorrect BPH produces a meaningless result.

### Procedure

1. **Start calibration.** Open the command menu and enable the **Calibrate** checkbox.
   tg-timer begins collecting calibration samples immediately.

2. **Monitor progress.** A progress indicator advances from 0% toward 100% as samples
   are collected. The calibration uses a dataset of 900 samples (defined by the
   internal `CAL_DATA_SIZE` constant), which takes approximately 15 minutes to
   collect at typical BPH values. During this time, keep the watch positioned stably
   near the microphone and avoid disturbing the setup.

3. **Read the result.** When the progress indicator reaches 100% and the dataset is
   complete, tg-timer computes the calibration value and displays it. The result is
   expressed in seconds per day (s/d) and represents the systematic offset caused by
   sample rate drift.

### What Happens on Completion

When calibration completes successfully:

- The computed calibration value is **automatically applied** to the calibration spin
  button in the main interface. You do not need to enter it manually.
- The value is **persisted to the configuration file** (tg-timer.ini) along with your
  other settings, so it remains in effect across application restarts.

From this point forward, all Rate measurements are corrected by subtracting the
calibration offset, giving you readings that reflect the watch's true accuracy rather
than a combination of watch accuracy and audio device drift.

### Failure and Recovery

If the signal is interrupted or becomes unstable during calibration (for example, if
the watch is bumped, the microphone is disconnected, or background noise spikes), the
collected data may be unreliable and the result inaccurate.

If calibration produces a suspect result or fails to complete cleanly:

1. Uncheck the **Calibrate** checkbox to stop the current calibration.
2. Re-establish a stable 4-bar signal with the watch positioned correctly.
3. Re-enable the **Calibrate** checkbox to restart the procedure from scratch with a
   fresh dataset.

A successful calibration requires an uninterrupted 15-minute collection period with
consistent full-strength signal throughout. If your environment makes this difficult,
try timing in a quieter location or using a contact microphone that is less sensitive
to ambient noise.

## 7. Audio Setup

The Audio Setup dialog configures the audio input device, sample rate, and high-pass
filter used for tick detection. Open it from the command menu: select **Audio setup**.

### Device Selection

The dialog lists all detected audio input devices on your system. Select the device
you want to use for timing (typically a built-in microphone, USB microphone, or
contact pickup).

Devices that are unsuitable for timing — for example, those that do not support the
required sample format or have incompatible channel configurations — are displayed as
**insensitive** (grayed out and not selectable). Only active, compatible devices can
be selected.

On first launch, when no configuration file exists, tg-timer selects the system's
default audio input device automatically. This selection is then persisted to the
configuration file for subsequent sessions.

### Sample Rate

The sample rate determines how many audio samples per second are captured from the
input device. Higher sample rates provide finer timing resolution, which translates
to more precise Rate and Beat Error measurements.

**Available rates:**

| Rate | Value (Hz) |
|------|-----------|
| 22.05 kHz | 22050 |
| 44.1 kHz | 44100 |
| 48 kHz | 48000 |
| 96 kHz | 96000 |
| 192 kHz | 192000 |

Rates that the selected device does not support are displayed as **insensitive**
(grayed out, not selectable). The available rates depend on the device hardware and
its driver capabilities.

**Custom entry.** You can type a sample rate value directly into the rate field. Values
below 1000 are interpreted as kHz — for example, entering `44.1` is equivalent to
entering `44100`. This shorthand avoids typing large numbers for common rates.

**Trade-offs.** Higher sample rates increase timing resolution because each sample
represents a shorter time interval, allowing tg-timer to pinpoint beat edges more
precisely. This results in more stable and accurate Rate and Beat Error readings.
However, higher rates also increase CPU load and memory usage proportionally — a
192 kHz rate processes roughly 4× the data of a 48 kHz rate.

**Default recommendation.** The default rate of **44100 Hz** is adequate for most
timing scenarios. It provides sufficient resolution for reliable measurements with
moderate resource usage. Most common audio devices support this rate, making it a safe
default. Higher rates (96 or 192 kHz) are beneficial when you need maximum precision
for fine regulation work or when timing high-beat movements, but are not necessary for
typical use.

### High-Pass Filter

The Audio Setup dialog includes a high-pass filter cutoff slider that removes
low-frequency noise from the audio signal before analysis. Low-frequency content
(such as handling noise, room rumble, or electrical hum) does not contain useful
tick information and can interfere with beat detection.

**Slider parameters:**

| Parameter | Value |
|-----------|-------|
| Range | 0 Hz to half the selected sample rate (Nyquist frequency) |
| Step size | 100 Hz |
| Off position | 0 (filter disabled) |
| Default | 3000 Hz |

The slider adjusts in 100 Hz increments. Setting the slider to **0** disables the
high-pass filter entirely (the "Off" position). The upper limit is the Nyquist
frequency — half the selected sample rate. For example, at 44100 Hz the slider
maximum is 22050 Hz; at 96000 Hz it extends to 48000 Hz.

The default cutoff of **3000 Hz** works well for typical timing setups. Watch tick
frequencies are generally in the range of a few kHz, so a 3000 Hz cutoff passes the
tick signal while rejecting lower-frequency noise.

**When the slider is disabled.** The high-pass filter slider becomes disabled (grayed
out, not adjustable) when the Filter Chain has been modified so that its first filter
is no longer a high-pass type. In this case, the slider no longer controls the filter
chain's first element, and you should manage filtering through the
[Filter Chain](#8-filter-chain) dialog instead.

### Failure Modes

If the audio device fails to start or stops working, tg-timer cannot acquire signal
and measurements will not appear. The following table lists common causes and
corrective actions:

| Problem | Cause | Resolution |
|---------|-------|------------|
| Device fails to open | Another application has exclusive access to the device | Close the other application that is using the audio device, then re-select the device in Audio Setup or restart tg-timer |
| Device not available | The audio device has been physically disconnected (e.g., USB microphone unplugged) | Reconnect the device, then re-open Audio Setup and select it again |
| Rate not supported | A sample rate was selected that the device hardware does not support | Select a different rate from the supported (non-grayed-out) options, or use the default 44100 Hz |
| No devices listed | Audio driver is not installed, or the application lacks permission to access audio devices | Install the appropriate audio driver for your operating system; on Linux, check that the user has permission to access audio devices (e.g., membership in the `audio` group); on macOS, grant microphone access in System Settings → Privacy & Security → Microphone |

If the issue persists after trying the steps above, see
[Troubleshooting](#13-troubleshooting) for additional diagnostic procedures including
using the True Peak Programme Meter to verify audio input levels.

## 8. Filter Chain

The Filter Chain is an ordered sequence of biquadratic audio filters applied to the
input signal before beat analysis. By configuring the chain you can isolate the tick
frequency of a watch movement from background noise, electrical interference, or
other unwanted sounds that degrade detection quality. Open the Filter Chain dialog from
the command menu: select **Filter Chain**.

> **Note:** The filter response graph feature requires Python support (Python 3,
> matplotlib, and scipy) compiled into tg-timer. If Python support is not available,
> the graph area displays "Not Supported" with a tooltip explaining the dependency.

### Managing Filters

The Filter Chain dialog presents a list of all filters in the chain. You manage the
chain using the following operations:

| Operation | How |
|-----------|-----|
| **Add a filter** | Select a filter type from the dropdown at the bottom of the list, then click Add. The new filter is appended to the end of the chain with default parameters and starts in a disabled state. |
| **Remove a filter** | Select a filter in the list and click Remove. The filter is deleted from the chain immediately. |
| **Reorder filters** | Drag a filter entry up or down in the list to change its position in the processing order. Filters are applied in the order shown — top to bottom. |
| **Enable / disable a filter** | Toggle the checkbox next to a filter in the list. Disabled filters remain in the chain but are bypassed during audio processing (their effect is removed without changing the chain structure). |

Filters are processed in sequence: the output of the first filter feeds into the
second, and so on through the entire chain. The order matters — placing a high-pass
filter before a notch filter produces a different result than the reverse.

### Filter Types

Each filter type shapes the audio signal differently by passing, attenuating, or
boosting specific frequency ranges:

| Type | Effect on frequency content |
|------|----------------------------|
| **Low-pass (LP)** | Passes frequencies below the cutoff frequency and progressively attenuates frequencies above it. Useful for removing high-frequency noise while preserving lower-frequency content. |
| **High-pass (HP)** | Passes frequencies above the cutoff frequency and progressively attenuates frequencies below it. Useful for removing low-frequency rumble, handling noise, or electrical hum. |
| **Band-pass (BP)** | Passes frequencies within a band centered on the center frequency and attenuates frequencies outside that band. Useful for isolating the tick frequency when you know its approximate range. |
| **Notch** | Rejects a narrow band of frequencies centered on the center frequency and passes everything else. Useful for removing a specific interference tone (e.g., 50/60 Hz mains hum or a resonant frequency). |
| **All-pass (AP)** | Passes all frequencies at equal amplitude but alters the phase relationship between frequencies. Does not change the magnitude spectrum — used for phase correction in specific signal processing scenarios. |
| **Peaking** | Boosts or cuts a band of frequencies around the center frequency by a specified gain (in dB). Positive gain boosts the band; negative gain cuts it. Useful for selectively emphasizing or suppressing a frequency region without fully removing it. |

### Filter Parameters

When you select a filter in the list, the editing controls below the list become
active. Each filter has the following adjustable parameters:

| Parameter | Range | Description |
|-----------|-------|-------------|
| **Frequency Center** | 0 – 24000 Hz | The cutoff frequency (for LP, HP) or center frequency (for BP, Notch, AP, Peaking). This is the frequency around which the filter acts. Adjusted in 100 Hz steps. |
| **Q / BW** | 0 – 10 (Q) or 0 – 1.0 (BW) | Controls how narrow or wide the filter's effect is. Low-pass and high-pass filters use **Q** (range 0 – 10); band-pass, notch, all-pass, and peaking filters use **BW** (range 0 – 1.0). Lower values produce a narrower effect; higher values produce a wider transition band. Adjusted in 0.001 steps. |
| **Gain** | −35 to +35 dB | The boost or cut applied at the center frequency. **Only active for peaking filters.** Positive values boost; negative values cut. For all other filter types, gain is fixed at 0 dB and the control is inactive. Adjusted in 0.01 dB steps. |

#### Default Values by Filter Type

When a new filter is added, it receives default parameter values based on its type:

| Filter type | Default frequency (Hz) | Default Q / BW |
|-------------|----------------------|----------------|
| Low-pass | 23000 | 0.707 (√½) |
| High-pass | 3000 | 0.707 (√½) |
| Band-pass | 15000 | 0.150 |
| Notch | 22000 | 0.100 |
| All-pass | 7800 | 0.167 |
| Peaking | 15000 | 0.228 |

Peaking filters also start with a gain of 0 dB (no boost or cut). All new filters are
added in a **disabled** state — enable the checkbox to activate them.

### Filter Response Graph

When Python support is available, the Filter Chain dialog includes a frequency
response graph that visualizes how the filter(s) affect the audio signal. The graph
plots magnitude (amplitude) versus frequency, showing which frequencies are passed,
attenuated, or boosted.

**Enabling the graph.** Check the **Filter Response Graph** checkbox in the graph
frame header to activate the visualization. Uncheck it to hide the graph and reduce
CPU usage.

**Graph source options:**

| Option | What it shows |
|--------|---------------|
| **Selected Filter Only** | The frequency response of the single filter currently selected in the list. Useful for understanding one filter's behavior in isolation. |
| **Entire Chain** | The combined frequency response of all enabled filters in the chain. Shows the net effect of the complete filter chain on the audio signal. |

The graph updates automatically when you change filter parameters, add or remove
filters, or enable/disable filters. It requires Python 3, matplotlib, and scipy to be
installed (see [Installation](#2-installation) for platform-specific instructions on
installing these optional dependencies).

### Relationship with Audio Setup High-Pass Slider

The high-pass filter cutoff slider in the [Audio Setup](#7-audio-setup) dialog
controls the **first filter in the chain**, provided that filter is a high-pass type.
On a fresh installation, the filter chain starts with a single high-pass filter at
3000 Hz — this is the same filter controlled by the Audio Setup slider.

The relationship works as follows:

- **Slider active:** When the first filter in the chain is a high-pass filter, the
  Audio Setup slider directly sets its cutoff frequency. Moving the slider to 0
  disables the filter (equivalent to unchecking it in the Filter Chain list).
- **Slider disabled:** If you modify the chain so that the first filter is no longer
  a high-pass type (for example, by reordering filters or removing the initial
  high-pass), the Audio Setup slider becomes grayed out and non-adjustable. A tooltip
  indicates that you should manage filtering through the Filter Chain dialog instead.

This design lets casual users adjust the high-pass cutoff quickly from Audio Setup,
while advanced users who build custom filter chains retain full control through the
Filter Chain dialog. If you need the slider back, ensure the first filter in the
chain is a high-pass type.

## 9. Signal Analysis

The Signal dialog provides tools for inspecting the audio input signal: a level meter
for checking microphone gain and a frequency-over-time visualization for diagnosing
mechanical characteristics. Open the Signal dialog from the command menu: select
**Signal**.

### 9.1 True Peak Programme Meter

The True Peak Programme Meter (TPPM) is a real-time audio level meter that displays the
peak amplitude of the input signal in dBFS (decibels relative to full scale). It helps
you verify that your microphone gain is set appropriately — strong enough for reliable
detection but not so high that the signal clips.

**Enable / disable.** The TPPM has an on/off switch in the Signal dialog. Enable it to
begin monitoring input levels; disable it when you no longer need the meter (reducing
CPU usage slightly).

**dBFS readout.** The numeric readout displays the current peak level to one decimal
place (e.g., −12.3 dBFS). Full scale (0.0 dBFS) represents the maximum level the
audio device can capture without distortion. More negative values indicate a quieter
signal.

**Level bar.** The graphical bar shows the signal level within the range **−70 to
+6 dBFS**. The bar responds to peaks in the audio input, giving a visual indication of
how loud the watch's tick is relative to the device's maximum input capacity.

#### Interpreting the Meter: Gain Assessment

Use the TPPM to decide whether your microphone gain needs adjustment:

| Meter behavior | Interpretation | Action |
|----------------|----------------|--------|
| Bar barely moves, stays near −70 dBFS | Signal is **too low** — the microphone is not picking up the tick adequately | Increase microphone gain in your operating system's sound settings, or move the watch closer to the microphone |
| Bar shows clear activity without hitting the top | Signal level is **acceptable** — the tick is well above the noise floor with headroom to spare | No adjustment needed; this is the target operating range |
| Bar hits +6 dBFS (top of scale) | Signal is **clipping** — the input is overdriving the audio device, causing distortion | Reduce microphone gain or move the watch slightly farther from the microphone |

A clipping signal distorts the waveform shape that tg-timer analyzes, which can cause
erratic measurements or failed beat detection. Aim for a level that produces clear
meter activity in the middle region of the bar without reaching the top.

### 9.2 Spectrogram

The spectrogram is a frequency-over-time visualization that shows the spectral content
of the audio signal — it plots which frequencies are present and how they change over
time. This is useful for identifying the characteristic frequency signature of a watch
movement, spotting mechanical anomalies, or verifying that the filter chain is
correctly isolating the tick signal.

> **Note:** The spectrogram requires Python support with the **libtfr** and
> **matplotlib** libraries compiled into tg-timer. If these are not available, the
> spectrogram controls are not shown. See [Installation](#2-installation) for
> platform-specific instructions on installing the optional Python dependencies.

**Source options.** The spectrogram can be generated for one of three sources:

| Option | What it shows |
|--------|---------------|
| **Last tic** | The spectral content of the most recently detected tic event |
| **Last toc** | The spectral content of the most recently detected toc event |
| **Time duration** | A configurable window of the live audio signal, showing the frequency content over a continuous time span |

**Duration setting.** When using the time duration option, the window length is
configurable from **0.1 to 32.0 seconds** in **0.1-second increments**. The default
duration is **1.0 second**. Shorter durations give a more responsive display that
updates frequently; longer durations show more temporal context at the cost of slower
refresh.

The spectrogram is most useful for advanced diagnostics — for example, checking whether
a notch filter is effectively suppressing an interference frequency, or examining the
harmonic structure of the tick impulse.

### 9.3 Signal Strength

The Signal Strength indicator (displayed in the main window as 0 to 4 bars) represents
how reliably tg-timer is detecting the watch's tick pattern in the audio input. It is
not a simple volume meter — it reflects the confidence level of the beat detection
algorithm.

**Level interpretation:**

| Bars | Meaning |
|------|---------|
| 0 | **No signal** — tg-timer cannot identify any periodic tick pattern in the audio. No measurements are produced. |
| 1 | **Minimal detection** — a periodic signal is detected but with low confidence. Measurements may appear but can be unreliable. |
| 2 | **Partial lock** — the tick pattern is being tracked with moderate confidence. Rate and Beat Error readings appear but may fluctuate. |
| 3 | **Good lock** — the application is tracking the tick reliably. Measurements are stable and usable. |
| 4 | **Full lock** — maximum detection confidence. All measurements are active, including amplitude (which requires the highest signal quality). |

Signal strength depends on several factors: the loudness of the tick relative to
background noise, the consistency of the tick interval, and how well the audio signal
matches the expected BPH. Improving any of these (moving the watch closer, reducing
ambient noise, selecting the correct BPH) increases signal strength.

At lower signal strengths (1–2 bars), amplitude is not available because computing the
balance wheel's swing angle requires precise identification of the impulse waveform
shape. Rate and Beat Error can be computed at lower strengths but stabilize and become
most accurate at 3–4 bars.

## 10. Saving and Loading Data

tg-timer can save and load timing measurements using its native file format, the
**TGJ file** (tg-timer JSON data, `.tgj` extension). A TGJ file stores one or more
snapshots with optional names, allowing you to preserve measurements for later review
or comparison across timing sessions.

### TGJ File Format

The TGJ file is tg-timer's native save format. Each `.tgj` file contains one or more
snapshots — the same snapshots you see as tabs in the main window. Snapshots include
the paperstrip data, waveform, and numeric readings captured at the moment they were
created. If a snapshot has been given a name (by double-clicking the tab), that name
is preserved in the file.

The `.tgj` extension identifies these files on disk. tg-timer automatically appends
the `.tgj` extension when saving if you do not include it in the filename.

### Save Operations

tg-timer provides two save operations, both available from the command menu:

**Save current display.** Saves the active tab's snapshot to a `.tgj` file. This is
useful when you want to preserve a single measurement — for example, the current
timing reading for a specific watch position. A file dialog prompts you to choose a
location and filename.

**Save all snapshots.** Saves all open snapshot tabs into a single `.tgj` file. Each
snapshot is stored in the file with its name and data, so you can capture an entire
timing session (multiple positions, before/after adjustments, etc.) in one file.

**Overwrite confirmation.** If you save to a filename that already exists, tg-timer
displays a confirmation dialog asking whether you want to overwrite the existing file.
This prevents accidental data loss.

**Auto-appended extension.** If the filename you enter does not end with `.tgj`,
tg-timer appends it automatically. For example, entering `my-watch` results in the
file being saved as `my-watch.tgj`.

### Open

Select **Open** from the command menu to load snapshots from a `.tgj` file. A file
dialog lets you browse to and select the file. Each snapshot stored in the file
appears as a new tab in the tab bar, preserving its original name and measurement
data. You can open multiple files in succession — each file's snapshots are added as
additional tabs.

### Command-Line File Arguments

One or more `.tgj` file paths can be passed as command-line arguments when launching
tg-timer. Each file is opened automatically at startup, and every snapshot within each
file appears as a new tab:

```sh
tg-timer session1.tgj session2.tgj
```

This is convenient for quickly reviewing previously saved measurements without
navigating through file dialogs. It also supports shell scripting workflows — for
example, opening a batch of timing sessions recorded over several days.

### MIME Type Registration

On freedesktop.org-compatible systems (most Linux desktop environments), tg-timer
registers the MIME type **`application/x-tg-timer-data`** with the **`.tgj`** glob
pattern. This registration enables the desktop to recognize `.tgj` files and associate
them with tg-timer — for example, double-clicking a `.tgj` file in a file manager can
open it directly in tg-timer, and the file manager displays the correct icon for the
file type.

This registration happens automatically during installation on systems that support
the freedesktop.org shared MIME database (GNOME, KDE, XFCE, and other
XDG-compliant desktops). On Windows and macOS, file association is handled through
the platform's native mechanisms.

### Error Handling

If a file cannot be opened (e.g., the file does not exist, is corrupted, or
permissions are insufficient) or cannot be written (e.g., the destination is read-only
or the disk is full), tg-timer displays an **error dialog** indicating the failure.
The dialog describes what went wrong so you can take corrective action — for example,
choosing a different save location or checking file permissions.

## 11. Configuration Persistence

tg-timer automatically saves your settings to a configuration file so that preferences
persist between sessions. You do not need to manually save settings — the application
handles this transparently.

### Configuration File Location

Settings are stored in a file named `tg-timer.ini`, placed in the platform's standard
user configuration directory:

| Platform | Location |
|----------|----------|
| **Linux** | `$XDG_CONFIG_HOME/tg-timer.ini` (typically `~/.config/tg-timer.ini`) |
| **Windows** | `%APPDATA%\tg-timer.ini` |
| **macOS** | `$XDG_CONFIG_HOME/tg-timer.ini` (typically `~/.config/tg-timer.ini`) |

The path is determined by the GLib `g_get_user_config_dir()` function, which respects
the `XDG_CONFIG_HOME` environment variable on Unix-like systems and uses `%APPDATA%`
on Windows. If `XDG_CONFIG_HOME` is not set, it defaults to `~/.config`.

### Persisted Settings

The following settings are saved to the configuration file:

| Setting | Description |
|---------|-------------|
| **BPH** | The selected beats-per-hour value (0 when set to "guess" mode) |
| **Lift angle** | The balance wheel lift angle in degrees |
| **Calibration** | The calibration compensation value in 0.1 s/d units |
| **Light algorithm** | Whether light algorithm mode is enabled (on/off) |
| **Vertical paperstrip** | Whether the vertical paperstrip layout is active (on/off) |
| **Audio device** | The selected audio input device index |
| **Audio rate** | The selected sample rate in Hz |
| **High-pass cutoff frequency** | The high-pass filter cutoff in Hz (0 = disabled) |
| **Filter chain** | The complete filter chain configuration (see below) |

**Filter chain details.** When the filter chain contains more than a single high-pass
filter, the full chain is persisted. For each filter in the chain, the following
parameters are stored:

- **Type** — the filter type (low-pass, high-pass, band-pass, notch, all-pass, or peaking)
- **Frequency** — the center or cutoff frequency in Hz
- **Q** — the bandwidth / Q factor
- **Gain** — the gain in dB (relevant for peaking filters)
- **Enabled** — whether the filter is active or bypassed

If the chain consists of only a single high-pass filter (the default configuration),
the filter chain group is omitted from the file and the high-pass cutoff frequency
field alone controls the filter.

### Save Behavior

tg-timer saves settings in two ways:

1. **Periodic save.** A background timer checks every **10 seconds** whether any
   setting has changed since the last save. If a change is detected, the configuration
   file is written immediately. If nothing has changed, no write occurs.

2. **Save on exit.** When you quit the application (via the Quit menu item or closing
   the window), all current settings are saved to the configuration file regardless of
   whether the periodic timer has fired recently.

This dual approach ensures that settings are not lost even if the application
terminates unexpectedly (at most 10 seconds of changes could be lost in a crash), while
avoiding unnecessary disk writes when settings are stable.

### First-Launch Defaults

When no configuration file exists (for example, on first launch after installation),
tg-timer uses built-in default values for all settings:

| Setting | Default value |
|---------|--------------|
| BPH | 0 (no preset selected; equivalent to unset) |
| Lift angle | 52° |
| Calibration | 0.0 s/d |
| Light algorithm | Off |
| Vertical paperstrip | On |
| Audio device | System default (auto-selected) |
| Audio rate | 44100 Hz |
| High-pass cutoff | 3000 Hz |
| Filter chain | Single high-pass filter at 3000 Hz, enabled |

After first launch, the configuration file is created on the first periodic save or
when you exit the application. Subsequent launches read settings from this file,
restoring your previous session's configuration automatically.

## 12. Light Algorithm Mode

**Light Algorithm** mode is an alternative analysis mode that trades measurement
precision for lower CPU usage. It uses simple decimation — processing every other
audio sample — to halve the effective sample rate seen by the analysis engine. This
makes tg-timer usable on less powerful hardware or in situations where CPU resources
are constrained.

### How It Works

In normal mode, tg-timer processes every audio sample at the full sample rate (e.g.,
all 44100 samples per second at the default rate). In Light Algorithm mode, the
application applies decimation by discarding every other sample, effectively cutting
the sample rate in half (e.g., from 44100 Hz down to an effective 22050 Hz). This
reduces the number of computations the analysis engine must perform per second.

The trade-off is reduced measurement precision. Because the effective sample rate is
lower, the analysis starts at a smaller initial window size, which means fewer data
points are available for computing Rate, Beat Error, and Amplitude. The result is
slightly less precise readings compared to normal mode, though still sufficient for
most timing tasks.

**Summary of trade-offs:**

| Mode | Precision | CPU Usage |
|------|-----------|-----------|
| Normal (default) | Higher — full sample rate, larger initial analysis window | Higher |
| Light Algorithm | Lower — halved effective sample rate, smaller initial analysis window | Lower |

### Toggling Light Algorithm Mode

Light Algorithm mode is controlled by the **"Light algorithm"** checkbox in the
command menu (see [4.7 Command Menu](#47-command-menu)):

- **Checked** — Light Algorithm mode is enabled (decimation active, reduced precision,
  lower CPU).
- **Unchecked** — Normal mode (full precision, higher CPU usage).

The setting is persisted to the configuration file and restored on next launch (see
[11. Configuration Persistence](#11-configuration-persistence)).

### Warning: Data Loss When Toggling Mid-Measurement

If you toggle Light Algorithm mode while a measurement is in progress, the audio
buffer is cleared and all current measurement data is lost. The paperstrip, numeric
outputs, and waveform displays reset as if you had just started the application. You
must wait for new data to accumulate before readings become available again.

This happens because the analysis engine cannot mix samples processed at different
effective rates — the existing buffer becomes invalid when the decimation state
changes. To avoid losing data, finish reviewing your current measurement or take a
snapshot before toggling the mode.

## 13. Troubleshooting

This section covers common problems you may encounter when using tg-timer and
provides step-by-step solutions for each. Work through the relevant steps in order
before escalating to the project's issue tracker.

### 13.1 No Signal (0 Bars)

If the Signal Strength indicator shows 0 bars and no timing data appears, follow
these steps in order:

1. **Verify the audio input device is selected.** Open the Audio Setup dialog (menu →
   Audio setup) and confirm that the correct microphone or audio input device is
   selected. Devices shown as insensitive (greyed out) are unsuitable — select a
   different one. If no devices appear, check that your audio hardware is connected
   and recognized by the operating system.

2. **Confirm audio input level using the TPPM.** Open the Signal dialog (menu →
   Signal) and enable the True Peak Programme Meter. The dBFS readout and level bar
   show whether any audio is reaching tg-timer. If the meter shows no activity (stays
   at the bottom of the range or reads very low dBFS values), the problem is upstream
   of tg-timer — the OS or hardware is not delivering audio.

3. **Check that the operating system audio input is not muted.** Open your OS audio
   settings and verify that the microphone input is not muted and the input volume is
   turned up. On Windows, check the Recording tab in Sound settings. On macOS, check
   System Preferences → Sound → Input. On Linux, use your desktop environment's sound
   settings or `pavucontrol` / `alsamixer`.

4. **Adjust the microphone gain.** Increase the microphone gain in your OS audio
   settings until the TPPM in the Signal dialog shows activity. Position the watch
   close to the microphone — caseback down, on or within a few centimetres of the mic.
   You should see the level bar respond to the ticking. Once the meter shows signal,
   the Signal Strength indicator should begin climbing above 0 bars.

### 13.2 Windows: Signal and Filter Chain Dialogs Not Working

On Windows, the Signal and Filter Chain dialogs may fail to open or display
incorrectly. This is a known issue caused by a Python library loading conflict in the
MSYS2 environment — the dialogs depend on Python (for plotting), and certain library
paths are not resolved correctly when running from the build directory.

**Workaround:** Copy `tg-timer.exe` to the MSYS2 UCRT64 bin directory and run it
from there:

```sh
cp tg-timer.exe /ucrt64/bin/
cd /ucrt64/bin
./tg-timer.exe
```

On a default MSYS2 installation, this directory is `C:\msys64\ucrt64\bin`. Running
tg-timer from this location ensures that all shared libraries and Python modules are
found correctly.

For more details, see:
[https://github.com/xyzzy42/tg/issues/8#issuecomment-2351649260](https://github.com/xyzzy42/tg/issues/8#issuecomment-2351649260)

### 13.3 Unstable Readings

If the Rate value fluctuates by more than ±10 s/d from the expected value, or Beat
Error and Amplitude readings jump around instead of settling to steady values, check
the following:

1. **Verify the BPH setting matches the watch movement.** An incorrect BPH causes
   the Rate calculation to be based on the wrong expected period, producing erratic or
   very large values. If you are unsure, use "guess" mode to let tg-timer auto-detect
   the BPH, then confirm the result against the movement's specification.

2. **Perform the calibration procedure.** An uncalibrated audio device can introduce
   a systematic drift that appears as wandering Rate values. Follow the calibration
   steps in [6. Calibration](#6-calibration) — ensure you have 4-bar signal strength
   and allow the full ~15-minute calibration period to complete.

3. **Check the Lift Angle.** While an incorrect Lift Angle does not affect Rate or
   Beat Error, it causes the displayed Amplitude to be proportionally wrong. If
   Amplitude is the unstable reading, verify that the Lift Angle matches the specific
   movement's caliber specification. Consult the movement's technical documentation or
   a watch caliber database for the correct value.

### 13.4 Application Fails to Start

If tg-timer does not launch or exits immediately with an error, check the following:

**Missing required libraries.** tg-timer depends on gtk+3, portaudio2, and fftw3. If
any of these are missing or cannot be found, the application fails at startup. Typical
error output for each case:

```text
error while loading shared libraries: libgtk-3.so: cannot open shared object file
```

```text
error while loading shared libraries: libportaudio.so: cannot open shared object file
```

```text
error while loading shared libraries: libfftw3.so: cannot open shared object file
```

On Windows (MSYS2), the equivalent errors reference `.dll` files (e.g.,
`libgtk-3-0.dll`). Install the missing library using the package manager commands in
[2. Installation](#2-installation) for your platform.

**Audio device not available.** If the selected audio device is disconnected or in use
by another application, tg-timer may fail to open the audio stream and exit or display
an error dialog. Verify that your audio input device is plugged in, not claimed
exclusively by another application, and recognized by the operating system. Try
selecting a different device in Audio Setup if available.

**Permissions issues (Linux).** On some Linux systems, the user must be in the `audio`
group to access audio devices directly. If you see permission-denied errors related to
ALSA or PulseAudio, verify your group membership:

```sh
groups | grep audio
```

If `audio` is not listed, add yourself to the group and log out/in:

```sh
sudo usermod -aG audio $USER
```

### 13.5 Getting Further Help

If the steps above do not resolve your issue, report it on the project's GitHub
issues page:

[https://github.com/xyzzy42/tg/issues](https://github.com/xyzzy42/tg/issues)

When reporting a problem, include:

- Your operating system and version
- How tg-timer was installed (package manager, built from source, etc.)
- The exact error message or unexpected behavior observed
- Steps to reproduce the issue

The community and maintainers can provide additional guidance or confirm whether the
issue is a known bug with a pending fix.

## 14. Glossary

This glossary defines the technical terms used throughout this manual. Each entry
includes a cross-reference to the section where the term is first introduced or
explained in detail.

**Amplitude**
The angular extent of the balance wheel's swing, measured in degrees. Healthy values
typically range from 180° to 315°. Amplitude reflects the energy delivered by the
mainspring through the gear train. Requires 4-bar signal strength and a correct Lift
Angle setting to display accurately.
→ See [3. Getting Started](#3-getting-started), [5. Measurement Parameters](#5-measurement-parameters)

**Beat Error**
The asymmetry between the tic and toc intervals, expressed in milliseconds (ms). In a
perfectly regulated movement, the tic-to-toc and toc-to-tic intervals are equal,
giving a Beat Error of 0 ms. Values under 1.0 ms indicate well-regulated movements.
→ See [3. Getting Started](#3-getting-started), [5. Measurement Parameters](#5-measurement-parameters)

**BPH (Beats Per Hour)**
The oscillation frequency of a watch movement, expressed as the number of balance
wheel beats per hour. Common values include 18000, 21600, 28800, and 36000. tg-timer
needs the correct BPH to compute Rate and Beat Error from the detected tick intervals.
→ See [3. Getting Started](#3-getting-started), [4.1 BPH Selector](#41-bph-selector)

**Calibration**
The process of compensating for the difference between an audio device's nominal sample
rate and its actual sample rate. Small deviations in sample rate cause systematic
offsets in Rate readings; calibration corrects this by collecting a 900-sample dataset
over approximately 15 minutes.
→ See [4.3 Calibration Control](#43-calibration-control), [6. Calibration](#6-calibration)

**Filter Chain**
An ordered sequence of biquadratic audio filters (low-pass, high-pass, band-pass,
notch, all-pass, peaking) applied to the input signal before analysis. Filters can be
added, removed, reordered, and individually enabled or disabled to improve signal
detection in noisy environments.
→ See [8. Filter Chain](#8-filter-chain)

**Lift Angle**
The angular displacement of the balance wheel caused by the impulse from the
escapement, specific to each movement caliber. Required for accurate Amplitude
readings; an incorrect Lift Angle causes proportionally wrong amplitude values while
Rate and Beat Error remain unaffected. Range: 10° to 90°, default 52°.
→ See [4.2 Lift Angle](#42-lift-angle)

**Light Algorithm**
An alternative analysis mode that uses simple decimation to halve the effective sample
rate, processing every other audio sample. This reduces CPU usage at the cost of
reduced measurement precision (smaller initial analysis window). Toggled via the
command menu.
→ See [12. Light Algorithm Mode](#12-light-algorithm-mode)

**Paperstrip**
The scrolling timing display that plots each beat's deviation from the expected
interval. Each dot represents one detected beat; the horizontal position indicates
timing deviation. Two parallel lines of dots (tic and toc) are normal, with the
spacing between them reflecting the Beat Error. Analogous to a traditional paper-strip
timegrapher output.
→ See [4.4 Paperstrip Display](#44-paperstrip-display)

**Rate**
The accuracy of a watch movement, expressed in seconds per day (s/d). A positive Rate
means the watch is gaining time (running fast); a negative Rate means it is losing time
(running slow). A well-adjusted mechanical watch typically runs within ±5 s/d.
→ See [3. Getting Started](#3-getting-started), [5. Measurement Parameters](#5-measurement-parameters)

**Signal Strength**
An indicator (0 to 4 bars) of how reliably tg-timer is detecting the watch's ticking
sound. Zero bars means no signal detected; 4 bars means full acquisition with maximum
measurement accuracy and amplitude measurement enabled. Higher bars indicate more
consistent identification of each beat.
→ See [3. Getting Started](#3-getting-started)

**Snapshot**
A captured state of the timing display — preserving the paperstrip data, waveform, and
numeric readings at the moment of capture. Snapshots appear as tabs and can be named,
reordered, and saved to TGJ files for later comparison.
→ See [4.6 Snapshot Management](#46-snapshot-management)

**Spectrogram**
A frequency-over-time visualization of the watch's audio signal, useful for diagnosing
mechanical issues. Shows frequency content for the last tic, last toc, or a
configurable time duration (0.1 to 32.0 seconds). Requires Python with libtfr and
matplotlib.
→ See [9.2 Spectrogram](#92-spectrogram)

**TGJ File**
The native file format (.tgj extension) used by tg-timer to save and load timing
snapshots. Each file stores one or more snapshots with optional names. The MIME type
`application/x-tg-timer-data` is registered for system file-type recognition on
freedesktop.org-compatible systems.
→ See [10. Saving and Loading Data](#10-saving-and-loading-data)

**True Peak Programme Meter**
A tool within the Signal dialog for measuring audio input levels. Displays a dBFS
readout (to one decimal place) and a level bar ranging from −70 to +6 dBFS. Used to
assess whether microphone gain is too low, acceptable, or clipping, and to verify that
the audio device is receiving signal.
→ See [9.1 True Peak Programme Meter](#91-true-peak-programme-meter)
