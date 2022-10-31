#!/bin/bash -ex

SCRIPT_NAME=$(basename ${BASH_SOURCE[0]})
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
STONEADM_SRC_DIR=${SCRIPT_DIR}/../../../src/stoneadm
CORPUS_COMMIT=9cd9ad020d93b0b420924fec55da307aff8bd422

[ -z "$SUDO" ] && SUDO=sudo
if [ -z "$STONEADM" ]; then
    STONEADM=${STONEADM_SRC_DIR}/stoneadm
fi

# at this point, we need $STONEADM set
if ! [ -x "$STONEADM" ]; then
    echo "stoneadm not found. Please set \$STONEADM"
    exit 1
fi

# respawn ourselves with a shebang
if [ -z "$PYTHON_KLUDGE" ]; then
    # see which pythons we should test with
    PYTHONS=""
    which python3 && PYTHONS="$PYTHONS python3"
    echo "PYTHONS $PYTHONS"
    if [ -z "$PYTHONS" ]; then
	echo "No PYTHONS found!"
	exit 1
    fi

    TMPBINDIR=$(mktemp -d)
    trap "rm -rf $TMPBINDIR" EXIT
    ORIG_STONEADM="$STONEADM"
    STONEADM="$TMPBINDIR/stoneadm"
    for p in $PYTHONS; do
	echo "=== re-running with $p ==="
	ln -s `which $p` $TMPBINDIR/python
	echo "#!$TMPBINDIR/python" > $STONEADM
	cat $ORIG_STONEADM >> $STONEADM
	chmod 700 $STONEADM
	$TMPBINDIR/python --version
	PYTHON_KLUDGE=1 STONEADM=$STONEADM $0
	rm $TMPBINDIR/python
    done
    rm -rf $TMPBINDIR
    echo "PASS with all of: $PYTHONS"
    exit 0
fi

# combine into a single var
STONEADM_BIN="$STONEADM"
STONEADM="$SUDO $STONEADM_BIN"

## adopt
CORPUS_GIT_SUBMOD="stoneadm-adoption-corpus"
TMPDIR=$(mktemp -d)
git clone https://github.com/stonepros/$CORPUS_GIT_SUBMOD $TMPDIR
trap "$SUDO rm -rf $TMPDIR" EXIT

git -C $TMPDIR checkout $CORPUS_COMMIT
CORPUS_DIR=${TMPDIR}/archive

for subdir in `ls ${CORPUS_DIR}`; do
    for tarfile in `ls ${CORPUS_DIR}/${subdir} | grep .tgz`; do
	tarball=${CORPUS_DIR}/${subdir}/${tarfile}
	FSID_LEGACY=`echo "$tarfile" | cut -c 1-36`
	TMP_TAR_DIR=`mktemp -d -p $TMPDIR`
	$SUDO tar xzvf $tarball -C $TMP_TAR_DIR
	NAMES=$($STONEADM ls --legacy-dir $TMP_TAR_DIR | jq -r '.[].name')
	for name in $NAMES; do
            $STONEADM adopt \
                     --style legacy \
                     --legacy-dir $TMP_TAR_DIR \
                     --name $name
            # validate after adopt
            out=$($STONEADM ls | jq '.[]' \
                      | jq 'select(.name == "'$name'")')
            echo $out | jq -r '.style' | grep 'stoneadm'
            echo $out | jq -r '.fsid' | grep $FSID_LEGACY
	done
	# clean-up before next iter
	$STONEADM rm-cluster --fsid $FSID_LEGACY --force
	$SUDO rm -rf $TMP_TAR_DIR
    done
done

echo "OK"
