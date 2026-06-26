#!/bin/bash
#
# Build a Debian/Ubuntu .deb package for tg-timer.
#
# The package version is derived from the top-level `version` file.  That
# string can contain hyphens (e.g. "0.7.0-tpiepho-cc.5"), but Debian treats
# the last hyphen as the boundary between the upstream version and the Debian
# revision, so we sanitise it into a single hyphen-free upstream token and
# append "-1" as the Debian revision.

set -e

DIR=`dirname "${BASH_SOURCE[0]}"`
ABSDIR=`cd "$DIR"; pwd`

cd "$DIR"/..

VERSION=`cat version`
# Debian-safe upstream version: turn every '-' into '+' so dpkg sees one token.
UPSTREAM=`echo "$VERSION" | sed 's/-/+/g'`
DEBVER="$UPSTREAM-1"

./autogen.sh
./configure --without-python
make dist

rm -rf deb
mkdir deb
# make dist names the tarball with the raw (hyphenated) version; rename the
# orig tarball and source directory to the Debian-safe upstream token.
cp tg-timer-"$VERSION".tar.gz deb/tg-timer_"$UPSTREAM".orig.tar.gz

cd deb
tar xzf tg-timer_"$UPSTREAM".orig.tar.gz
mv tg-timer-"$VERSION" tg-timer-"$UPSTREAM"
cp -r "$ABSDIR"/debian tg-timer-"$UPSTREAM"

# Regenerate the changelog top entry (in the build copy only, leaving the
# tracked file untouched) so debuild's version matches the orig tarball.
cd tg-timer-"$UPSTREAM"
( echo "tg-timer ($DEBVER) unstable; urgency=medium"
  echo
  echo "  * Automated build of tg-timer $VERSION."
  echo
  echo " -- Colin Coleman <cco@capraconsulting.no>  $(date -R)"
  echo
  cat "$ABSDIR"/debian/changelog
) > debian/changelog

debuild -us -uc

echo
echo "Built packages:"
ls -1 ../tg-timer_"$UPSTREAM"*.deb
