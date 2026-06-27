#!/bin/bash
#
# Build a portable Windows (x86_64) ZIP for tg-timer.
#
# Must be run inside an MSYS2 MINGW64 shell with the mingw-w64-x86_64 GTK3,
# PortAudio and FFTW toolchain installed.  Produces a self-contained folder
# (the .exe plus every dependent DLL and the GTK runtime support files) and
# zips it.  The ARM build of Windows runs this x86_64 binary via emulation, so
# a single 64-bit package covers all Windows users.

set -e

DIR=`dirname "${BASH_SOURCE[0]}"`
cd "$DIR"/..

VERSION=`cat version`
MINGW=/mingw64
STAGE=win-zip/tg-timer

# --- Build -----------------------------------------------------------------
./autogen.sh
./configure --without-python
make
# Also build the console (-mconsole) debug variant so that, if the GUI build
# fails to start, running tg-timer-dbg.exe from a terminal surfaces the actual
# GLib/GTK error instead of a silent abort.
make tg-timer-dbg.exe

# --- Stage the bundle ------------------------------------------------------
# Standard Windows GTK layout: the exe and all DLLs sit at the root, with
# lib/ and share/ alongside, so GTK derives its install prefix from the exe.
rm -rf win-zip
mkdir -p "$STAGE"
cp tg-timer.exe "$STAGE"/
# Diagnostic console build; harmless to ship and lets users capture errors.
cp tg-timer-dbg.exe "$STAGE"/

# Copy the GDK-Pixbuf image loaders into the bundle first, so their own
# dependencies (e.g. the SVG loader needs librsvg/libxml2) are picked up by the
# DLL scan below -- the loaders are dlopen()ed at runtime, not linked into the
# exe, so scanning only the exe would miss them.
mkdir -p "$STAGE"/lib/gdk-pixbuf-2.0
cp -r "$MINGW"/lib/gdk-pixbuf-2.0/* "$STAGE"/lib/gdk-pixbuf-2.0/

# Collect every dependent DLL that resolves into the MinGW prefix.  Use ntldd
# (-R, recursive); unlike ldd it reliably resolves MinGW PE dependencies.  Scan
# the exe and every pixbuf loader.  ntldd prints "name => C:\...\mingw64\bin\
# name.dll (0xbase)"; normalise backslashes, then the path is the third field.
echo "Collecting dependent DLLs..."
for bin in "$STAGE"/tg-timer.exe \
	   "$STAGE"/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll; do
	ntldd -R "$bin"
done \
	| tr '\\' '/' \
	| awk 'tolower($0) ~ /mingw64\/bin\// {print $3}' \
	| sort -u \
	| while read -r dll; do
		cp -u "$dll" "$STAGE"/ 2>/dev/null || true
	done

ndll=$(ls -1 "$STAGE"/*.dll 2>/dev/null | wc -l)
echo "Bundled $ndll DLLs."
if [ "$ndll" -lt 10 ]; then
	echo "ERROR: too few DLLs bundled ($ndll); dependency collection failed." >&2
	exit 1
fi

# Regenerate the loader cache, then rewrite its entries to be RELATIVE to the
# bundle root so it works wherever the user extracts the ZIP.  gdk-pixbuf-query-
# loaders always writes absolute paths (canonicalised against the build dir), so
# we strip that build-dir prefix afterwards.  At runtime MSYS2's gdk-pixbuf
# resolves relative loader paths against the directory holding the gdk_pixbuf
# DLL (the bundle root).  The previous absolute cache baked in the CI path.
cache="$STAGE"/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
( cd "$STAGE" \
	&& gdk-pixbuf-query-loaders lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll \
		> lib/gdk-pixbuf-2.0/2.10.0/loaders.cache )
stage_abs=$(cd "$STAGE" && pwd -W)
sed -i "s|\"$stage_abs/|\"|g" "$cache"
echo "Loader cache entries:"
grep -E '^"' "$cache" || true

# Compiled GSettings schemas (GTK aborts at startup without these).
mkdir -p "$STAGE"/share/glib-2.0/schemas
cp "$MINGW"/share/glib-2.0/schemas/gschemas.compiled \
	"$STAGE"/share/glib-2.0/schemas/

# Icon themes so stock GTK icons render.
mkdir -p "$STAGE"/share/icons
cp -r "$MINGW"/share/icons/Adwaita "$STAGE"/share/icons/
cp -r "$MINGW"/share/icons/hicolor "$STAGE"/share/icons/

# --- Zip -------------------------------------------------------------------
ZIP="tg-timer-${VERSION}-windows-x64.zip"
rm -f "$ZIP"
( cd win-zip && zip -qr "../$ZIP" tg-timer )

echo
echo "Built $ZIP"
du -h "$ZIP"
