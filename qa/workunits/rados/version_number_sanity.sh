#!/bin/bash -ex
#
# test that stone RPM/DEB package version matches "stone --version"
# (for a loose definition of "matches")
#
source /etc/os-release
case $ID in
debian|ubuntu)
    RPMDEB='DEB'
    dpkg-query --show stone-common
    PKG_NAME_AND_VERSION=$(dpkg-query --show stone-common)
    ;;
centos|fedora|rhel|opensuse*|suse|sles)
    RPMDEB='RPM'
    rpm -q stone
    PKG_NAME_AND_VERSION=$(rpm -q stone)
    ;;
*)
    echo "Unsupported distro ->$ID<-! Bailing out."
    exit 1
esac
PKG_STONE_VERSION=$(perl -e '"'"$PKG_NAME_AND_VERSION"'" =~ m/(\d+(\.\d+)+)/; print "$1\n";')
echo "According to $RPMDEB package, the stone version under test is ->$PKG_STONE_VERSION<-"
test -n "$PKG_STONE_VERSION"
stone --version
BUFFER=$(stone --version)
STONE_STONE_VERSION=$(perl -e '"'"$BUFFER"'" =~ m/stone version (\d+(\.\d+)+)/; print "$1\n";')
echo "According to \"stone --version\", the stone version under test is ->$STONE_STONE_VERSION<-"
test -n "$STONE_STONE_VERSION"
test "$PKG_STONE_VERSION" = "$STONE_STONE_VERSION"
