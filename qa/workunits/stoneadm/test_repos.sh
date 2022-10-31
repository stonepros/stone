#!/bin/bash -ex

SCRIPT_NAME=$(basename ${BASH_SOURCE[0]})
SCRIPT_DIR=$(dirname ${BASH_SOURCE[0]})
STONEADM_SRC_DIR=${SCRIPT_DIR}/../../../src/stoneadm
STONEADM=${STONEADM_SRC_DIR}/stoneadm

# this is a pretty weak test, unfortunately, since the
# package may also be in the base OS.
function test_install_uninstall() {
    ( sudo apt update && \
	  sudo apt -y install stoneadm && \
	  sudo $STONEADM install && \
	  sudo apt -y remove stoneadm ) || \
	( sudo yum -y install stoneadm && \
	      sudo $STONEADM install && \
	      sudo yum -y remove stoneadm ) || \
	( sudo dnf -y install stoneadm && \
	      sudo $STONEADM install && \
	      sudo dnf -y remove stoneadm )
}

sudo $STONEADM -v add-repo --release octopus
test_install_uninstall
sudo $STONEADM -v rm-repo

sudo $STONEADM -v add-repo --dev master
test_install_uninstall
sudo $STONEADM -v rm-repo

sudo $STONEADM -v add-repo --release 15.2.7
test_install_uninstall
sudo $STONEADM -v rm-repo

echo OK.
