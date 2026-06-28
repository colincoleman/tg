# tg-timer User Manual

## 1. Introduction

tg-timer is an open source program for timing mechanical watches. It listens to
the tick sound of a watch movement through a microphone and computes:

- **Rate** — accuracy in seconds per day (s/d)
- **Beat Error** — regularity of the tic-toc alternation in milliseconds
- **Amplitude** — angular swing of the balance wheel in degrees

These measurements help watchmakers and enthusiasts evaluate whether a movement
is running well and identify problems that need adjustment.

tg-timer runs on Linux, macOS, and Windows (via MSYS2). It is licensed under the
GNU General Public License, version 2.

Source code and issue tracker:
[https://github.com/xyzzy42/tg](https://github.com/xyzzy42/tg)

For installation instructions, see the
[README](../README.md).

## 2. Getting Started

1. **Launch tg-timer.** Run `tg-timer` from your terminal (or `./tg-timer` from
   the build directory). The main window opens with a blank paperstrip display and
   dashes in the numeric output area.

2. **Position the watch near the microphone.** Place the watch caseback-down on or
   within a few centimetres of your microphone. The caseback transmits the tick
   sound most clearly. A quiet environment and a hard surface help. Adjust position
   until the Signal Strength indicator shows at least 1 bar.

3. **Verify signal detection.** The Signal Strength indicator shows how reliably
   tg-timer is detecting the tick:

   | Bars | Meaning |
   |------|---------|
   | 0 | No signal detected |
   | 1 | Minimal detection — measurements may appear but can be unreliable |
   | 2 | Partial lock — Rate and Beat Error appear but may fluctuate |
   | 3 | Good lock — measurements are stable |
   | 4 | Full lock — all measurements active, including Amplitude |

4. **Select the BPH.** BPH (Beats Per Hour) is the oscillation frequency of the
   movement. Select the correct value from the dropdown, type a custom value
   (range 8100–72000), or choose "guess" to let tg-timer auto-detect it. If you
   don't know your movement's BPH, use "guess" and confirm the result against the
   movement's specification.

5. **Read the outputs.** Once signal is acquired and BPH is set:

   - **Rate** — seconds per day gained (+) or lost (−). A well-adjusted watch
     typically runs within ±10 s/d.
   - **Beat Error** — asymmetry between tic and toc intervals. Under 1.0 ms
     indicates good regulation.
   - **Amplitude** — balance wheel swing in degrees. Healthy range is 180°–315°.
     Requires 4-bar signal strength and correct Lift Angle.

   Allow a few seconds of stable signal for readings to settle.

## 3. Main Interface

### 3.1 BPH Selector

Sets the expected oscillation frequency. The dropdown provides presets:

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
| 72000 | Extremely high-beat movements |

You can also type a custom value or select "guess" for auto-detection.

### 3.2 Lift Angle

The angular displacement of the balance wheel caused by the impulse from the
escapement. It varies by caliber and is required for accurate Amplitude readings.

- **Range:** 10°–90°
- **Default:** 52°

An incorrect lift angle causes proportionally wrong Amplitude values. Rate and
Beat Error are unaffected. Consult the movement's technical documentation or a
caliber database for the correct value.

### 3.3 Calibration Control

Compensates for the difference between your audio device's nominal sample rate and
its actual sample rate. This corrects a constant systematic offset in Rate
readings.

- **Range:** −100.0 to +100.0 s/d
- **Default:** 0.0

See [Calibration](#5-calibration) for the procedure.

### 3.4 Paperstrip Display

The scrolling timing display that plots each beat's deviation from the expected
interval. It produces a visual pattern analogous to the output of a traditional
paper-strip timegrapher (which can scroll either vertically or horizontally).

**Timing dots.** Each dot represents one detected beat. The position of a dot
indicates its timing deviation from the ideal interval. Dots aligned in a straight
line mean the watch is running at a steady rate. A slant to one side indicates
gaining; the other indicates losing. Two parallel lines of dots (tic and toc) are
normal — the spacing between them reflects the Beat Error.

**Wrapping.** The display wraps around — dots that reach one edge reappear at the
opposite edge. This means the entire trace is always visible.

**Controls:**

| Control | Action |
|---------|--------|
| Zoom in/out | Adjust the scale (how many milliseconds of timing are shown across the display width) |
| Shift left/right | Pan the display to recenter the trace |
| Center | Reset position so the current trace is centered |
| Clear | Erase all accumulated dots and start fresh |

### 3.5 Waveform and Period Displays

**Waveforms.** Show the detected pulse shape of the most recent tic and toc events.
Clean, repeatable waveforms indicate a strong signal.

**Period.** Shows the measured time interval between successive beats in
milliseconds.

### 3.6 Snapshot Management

A snapshot captures the current timing display state (paperstrip, waveform, and
numeric readings) into a tab for later review or comparison.

- **Create:** Click the snapshot button (camera icon)
- **Name:** Double-click a snapshot tab to edit its label
- **Reorder:** Drag tabs left or right
- **Close:** Click the close button on a tab, or use "Close all snapshots" from
  the menu

### 3.7 Command Menu

| Menu Item | Action |
|-----------|--------|
| Open | Load snapshots from a `.tgj` file |
| Save current display | Save the active tab to a `.tgj` file |
| Save all snapshots | Save all tabs into a single `.tgj` file |
| Light algorithm | Toggle reduced-precision mode for lower CPU usage |
| Calibrate | Start/stop the calibration procedure |
| Vertical | Toggle vertical paperstrip layout |
| Signal | Open the Signal dialog (level meter and spectrogram) |
| Filter Chain | Open the audio filter configuration dialog |
| Audio setup | Configure input device, sample rate, and high-pass filter |
| Close all snapshots | Remove all snapshot tabs |
| Quit | Exit (settings are saved automatically) |

### 3.8 Numeric Outputs

| Output | Unit | Description |
|--------|------|-------------|
| Rate | s/d | Accuracy. Positive = gaining, negative = losing |
| Beat Error | ms | Asymmetry between tic and toc. Lower is better |
| Amplitude | ° | Balance wheel swing. Requires 4-bar signal + correct Lift Angle |
| Guessed BPH | BPH | Shown when "guess" is selected |

## 4. Measurement Parameters

### 4.1 Rate

Seconds per day gained (+) or lost (−) compared to perfect timekeeping.

| Rate | Interpretation |
|------|----------------|
| Within ±5 s/d | Well-adjusted |
| Within ±10 s/d | Typical good condition |
| Beyond ±10 s/d | May need service or regulation |

Accurate Rate depends on correct BPH selection and calibration.

### 4.2 Beat Error

Asymmetry between the tic-to-toc and toc-to-tic intervals, in milliseconds.
Perfect regulation gives 0 ms. Under 1.0 ms is good. The maximum displayed value
is 99.9 ms.

### 4.3 Amplitude

Angular swing of the balance wheel in degrees. Reflects energy delivered by the
mainspring through the gear train.

- Below 180°: low energy — may indicate unwound mainspring or lubrication issues
- 180°–315°: healthy range
- Above 315°: risk of overbanking

tg-timer displays 0 when the computed value falls below 135° or above 360°,
indicating an unreliable measurement (usually weak signal) rather than a true zero.

Requires 4-bar signal strength and a correct Lift Angle setting.

## 5. Calibration

Calibration compensates for the constant difference between your audio device's
nominal sample rate and its actual sample rate. Since tg-timer derives all timing
from sample counting, even a small constant offset in the real sample rate
translates into a systematic error in Rate readings.

Note: calibration corrects a fixed offset. It does not address instability or
jitter in the audio clock.

### How It Works

Calibration uses a **quartz watch** as a reference. A quartz watch ticks at a
known, precise rate (once per second). tg-timer listens to the quartz tick and
measures how the phase of that tick drifts relative to the audio sample clock over
time. The drift reveals the constant error in the audio device's sample rate.

The BPH selector setting is ignored during calibration — the algorithm always
looks for a 1-second tick period internally.

### Procedure

1. Position a quartz watch (one with an audible seconds tick) near the microphone.
2. Ensure Signal Strength reaches 4 bars.
3. Open the command menu and enable **Calibrate**.
4. Wait approximately 15 minutes while keeping the quartz watch positioned stably.
   A progress indicator advances toward 100%.
5. On completion, the calibration value is automatically applied and saved.

### If Calibration Fails

If signal is interrupted during calibration, the result may be unreliable:

1. Uncheck Calibrate to stop.
2. Re-establish 4-bar signal with the quartz watch.
3. Re-enable Calibrate to restart from scratch.

## 6. Audio Setup

Open from the menu: **Audio setup**.

### Device Selection

Lists all detected audio input devices. Incompatible devices are greyed out. On
first launch, the system default device is selected automatically.

### Sample Rate

| Rate | Hz |
|------|-----------|
| 22.05 kHz | 22050 |
| 44.1 kHz | 44100 |
| 48 kHz | 48000 |
| 96 kHz | 96000 |
| 192 kHz | 192000 |

Unsupported rates are greyed out. You can type a value directly (values below 1000
are interpreted as kHz, e.g. `44.1` = 44100). Higher rates give finer timing
resolution at the cost of more CPU usage. The default 44100 Hz is adequate for
most use.

### High-Pass Filter

Removes low-frequency noise before analysis. Watch tick frequencies are generally
in the kHz range, so the default 3000 Hz cutoff passes useful signal while
rejecting low-frequency noise.

- **Range:** 0 (disabled) to Nyquist frequency (half the sample rate)
- **Step:** 100 Hz
- **Default:** 3000 Hz

The slider becomes disabled if the Filter Chain's first filter is no longer a
high-pass type — manage filtering through the Filter Chain dialog instead.

## 7. Filter Chain

An ordered sequence of biquadratic audio filters applied before beat analysis.
Open from the menu: **Filter Chain**.

### Operations

- **Add:** Select a type from the dropdown and click Add (added disabled by default)
- **Remove:** Select a filter and click Remove
- **Reorder:** Drag up or down (processing order is top to bottom)
- **Enable/disable:** Toggle the checkbox next to each filter

### Filter Types

| Type | Effect |
|------|--------|
| Low-pass | Passes below cutoff, attenuates above |
| High-pass | Passes above cutoff, attenuates below |
| Band-pass | Passes a band around center frequency |
| Notch | Rejects a narrow band around center frequency |
| All-pass | Alters phase without changing magnitude |
| Peaking | Boosts or cuts a band by a specified gain (dB) |

### Parameters

| Parameter | Description |
|-----------|-------------|
| Frequency | Cutoff or center frequency (0–24000 Hz, 100 Hz steps) |
| Q / BW | Width of effect. LP/HP use Q (0–10); others use BW (0–1.0) |
| Gain | Boost/cut in dB (−35 to +35). Only active for Peaking filters |

### Filter Response Graph

When Python support is available (matplotlib, scipy), the dialog includes a
frequency response graph showing the effect of the selected filter or the entire
chain. Enable via the checkbox in the graph frame.

### Auto-tune

The **Auto-tune** button on the Filter Chain toolbar chooses a filter chain for
you. With a watch running on the microphone, it captures a few seconds of audio
with the existing filters bypassed, then searches for the high-pass/low-pass
combination that produces the cleanest, most consistent beat detection, and
replaces the chain with the result.

Auto-tune deliberately favours a wide passband over a tight band-pass, so the
chosen filters stay robust as the watch is moved between positions. If no tick
is detected during the capture, or the current settings already look optimal, it
reports this and leaves the chain unchanged.

## 8. Signal Analysis

Open from the menu: **Signal**.

### 8.1 True Peak Programme Meter

A real-time audio level meter showing peak amplitude in dBFS. Use it to verify
microphone gain is appropriate.

- Near −70 dBFS: signal too low — increase gain or move watch closer
- Clear activity without hitting top: acceptable range
- Hitting +6 dBFS: clipping — reduce gain or move watch farther away

### 8.2 Spectrogram

A frequency-over-time visualization (requires Python with libtfr and matplotlib).
Can show the spectral content of the last tic, last toc, or a configurable time
window (0.1–32.0 seconds). Useful for identifying the tick's frequency signature
or verifying filter effectiveness.

## 9. Positional Test

A positional test measures how a watch's rate, beat error, and amplitude change
across the six standard horological positions. The spread between positions —
especially in rate and amplitude — indicates how strongly the movement is
affected by gravity, which is a key sign of its condition and regulation.

Start a test with the **Full Test** button in the main window. While a test is
running, the same button becomes **Cancel Test**.

### Configuration

The configuration dialog collects:

| Field | Description |
|-------|-------------|
| Watch Name | Optional label stored with the results |
| Position Duration | How long to measure each position (30–600 s, default 60) |
| Averaging Window | Window used to compute each position's averaged result (5–60 s, default 15). Must be shorter than the position duration |

The dialog also shows the current BPH, lift angle, and calibration so you can
confirm they are correct before starting. A live signal is required: if no tick
is detected, the test will not start.

### Running the Test

The test steps through the standard positions in order:

1. Dial up
2. Dial down
3. Crown up
4. Crown down
5. Crown left
6. Crown right

For each position, place the watch as indicated and leave it undisturbed. A
countdown shows the remaining measurement time while the readings are collected.
When a position finishes, the program pauses and prompts you to reposition the
watch; move it to the next position and click **Continue** to resume. Repeat
until all six positions are done.

### Results

Progress and results are shown on a dedicated **Positional Test** tab, with one
lane per position so you can compare them at a glance. When the test completes,
use **Save Report** to write the results out for your records.

## 10. Saving and Loading Data

tg-timer uses the `.tgj` file format (JSON-based) to store snapshots.

- **Save current display:** Saves the active tab to a file
- **Save all snapshots:** Saves all tabs into one file
- **Open:** Loads snapshots from a file as new tabs

Files can also be passed as command-line arguments:
```sh
tg-timer session1.tgj session2.tgj
```

On Linux desktops, the MIME type `application/x-tg-timer-data` is registered for
`.tgj` files.

## 11. Configuration

tg-timer automatically saves settings to `tg-timer.ini`:

| Platform | Location |
|----------|----------|
| Linux | `~/.config/tg-timer.ini` (or `$XDG_CONFIG_HOME`) |
| macOS | `~/.config/tg-timer.ini` (or `$XDG_CONFIG_HOME`) |
| Windows | `%APPDATA%\tg-timer.ini` |

Settings are saved periodically (every 10 seconds if changed) and on exit.

## 12. Light Algorithm Mode

Trades measurement precision for lower CPU usage by decimating the audio (processing
every other sample, halving the effective sample rate). Toggle via the "Light
algorithm" checkbox in the command menu.

Toggling mid-measurement clears all current data — take a snapshot first if you
want to preserve it.

## 13. Troubleshooting

### No Signal (0 bars)

1. Open Audio Setup and confirm the correct input device is selected.
2. Open the Signal dialog and enable the TPPM to verify audio is reaching tg-timer.
3. Check that the OS microphone input is not muted and gain is turned up.
4. Move the watch closer to the microphone, caseback-down.

### Windows: Signal and Filter Chain Dialogs Not Working

This is a known library path issue when running from the build directory. Copy
`tg-timer.exe` to the MSYS2 UCRT64 bin directory and run from there:

```sh
cp tg-timer.exe /ucrt64/bin/
cd /ucrt64/bin
./tg-timer.exe
```

See [issue #8](https://github.com/xyzzy42/tg/issues/8#issuecomment-2351649260)
for details.

### Unstable Readings

1. Verify the BPH setting matches the movement. An incorrect BPH produces erratic
   Rate values.
2. Check that calibration has been performed if you need precise Rate readings.
3. If Amplitude is unstable, verify the Lift Angle matches the movement's
   specification.

### Application Fails to Start

- **Missing libraries:** tg-timer requires gtk+3, portaudio2, and fftw3.
  Install any missing library using your platform's package manager (see README).
- **Audio device unavailable:** Verify the device is connected and not exclusively
  claimed by another application.
- **Permissions (Linux):** Ensure your user is in the `audio` group if required.

### Getting Help

Report issues at:
[https://github.com/xyzzy42/tg/issues](https://github.com/xyzzy42/tg/issues)

Include your OS/version, how tg-timer was installed, the exact error, and steps to
reproduce.

## 14. Glossary

**Amplitude** — Angular swing of the balance wheel (degrees). Healthy: 180°–315°.

**Beat Error** — Asymmetry between tic and toc intervals (ms). Lower is better.

**BPH** — Beats Per Hour. The oscillation frequency of the movement.

**Calibration** — Compensation for the constant offset between nominal and actual
audio sample rate.

**Filter Chain** — Ordered sequence of biquadratic filters applied before analysis.

**Lift Angle** — Angular displacement caused by the escapement impulse. Varies by
caliber. Affects only Amplitude measurement.

**Light Algorithm** — Reduced-precision mode that halves effective sample rate for
lower CPU usage.

**Paperstrip** — The scrolling timing display. Dots show beat deviations; the
display wraps around.

**Rate** — Watch accuracy in seconds per day.

**Signal Strength** — Detection confidence (0–4 bars). Not a volume meter.

**Snapshot** — A captured state of the timing display, saved as a tab.

**TGJ** — Native file format (.tgj) for saving/loading snapshots.
