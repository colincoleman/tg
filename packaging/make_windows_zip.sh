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

# --- Stage the bundle ------------------------------------------------------
# Standard Windows GTK layout: the exe and all DLLs sit at the root, with
# lib/ and share/ alongside, so GTK derives its install prefix from the exe.
rm -rf win-zip
mkdir -p "$STAGE"
cp tg-timer.exe "$STAGE"/

# Copy every dependent DLL that resolves into the MinGW prefix.  Use ntldd,
# which (unlike ldd) reliably resolves the dependencies of a MinGW PE binary
# and, with -R, walks them recursively so transitive DLLs are included too.
echo "Collecting dependent DLLs..."
# ntldd -R prints "name => C:\...\mingw64\bin\name.dll (0xbase)". Normalise the
# backslashes to slashes, then the resolved path is the third field.
ntldd -R "$STAGE"/tg-timer.exe \
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

# GDK-Pixbuf image loaders (needed for icons/PNGs); regenerate the cache so it
# references the bundled loader location.
mkdir -p "$STAGE"/lib/gdk-pixbuf-2.0
cp -r "$MINGW"/lib/gdk-pixbuf-2.0/* "$STAGE"/lib/gdk-pixbuf-2.0/
GDK_PIXBUF_MODULEDIR="$STAGE"/lib/gdk-pixbuf-2.0/2.10.0/loaders \
	gdk-pixbuf-query-loaders \
	> "$STAGE"/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache

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
