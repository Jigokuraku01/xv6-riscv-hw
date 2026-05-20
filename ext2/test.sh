#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-normal}"

IMG=ext2.img
MNT=mnt
EXTRACT=extracted
HASHES=hashes.txt

IMG_SIZE=${IMG_SIZE:-8G}
BLOCK_SIZE=${BLOCK_SIZE:-2048}

GETINFO=./getinodeinfo
GETDATA=./getinodedata
PARSEDIR=./parsedirent

if [ "$MODE" = "valgrind" ]; then
    VG="valgrind --error-exitcode=99 --leak-check=full -q"
else
    VG=""
fi

cleanup() {
    if mountpoint -q "$MNT" 2>/dev/null; then
        sudo umount "$MNT" || true
    fi
}
trap cleanup EXIT

echo "=== step 1: create sparse image $IMG_SIZE ==="
rm -f "$IMG"
truncate --size "$IMG_SIZE" "$IMG"

echo "=== step 2: mkfs.ext2 (block size $BLOCK_SIZE) ==="
mkfs.ext2 -F -b "$BLOCK_SIZE" -N 4096 -t ext2 "$IMG" >/dev/null

echo "=== step 3: mount ==="
mkdir -p "$MNT"
sudo mount -t ext2 -o loop "$IMG" "$MNT"
sudo chown "$(id -u):$(id -g)" "$MNT"

echo "=== step 4: populate fs ==="
mkdir -p "$MNT/dir1" "$MNT/dir2" "$MNT/dir2/sub"

dd if=/dev/urandom of="$MNT/small.bin" bs=1K count=4 status=none

dd if=/dev/urandom of="$MNT/medium.bin" bs=1K count=512 status=none

truncate -s 5G "$MNT/sparse.bin"
echo "hello-at-start" | dd of="$MNT/sparse.bin" conv=notrunc bs=1 \
    seek=0 count=15 status=none
echo "hello-at-end" | dd of="$MNT/sparse.bin" conv=notrunc bs=1 \
    seek=$((5 * 1024 * 1024 * 1024 - 13)) count=13 status=none

dd if=/dev/urandom of="$MNT/dir1/inside1.txt" bs=1K count=8 status=none
dd if=/dev/urandom of="$MNT/dir2/inside2.txt" bs=1K count=16 status=none
dd if=/dev/urandom of="$MNT/dir2/sub/deep.txt" bs=1K count=2 status=none

sync

echo "=== step 5: collect inode numbers and hashes ==="
> "$HASHES"
declare -A INODES
declare -A HASHFILES
for f in small.bin medium.bin sparse.bin \
         dir1/inside1.txt dir2/inside2.txt dir2/sub/deep.txt; do
    ino=$(stat -c '%i' "$MNT/$f")
    h=$(sha512sum "$MNT/$f" | awk '{print $1}')
    INODES["$f"]=$ino
    HASHFILES["$f"]=$h
    echo "$ino $h $f" >> "$HASHES"
    echo "  $f -> inode $ino"
done

DIR1_INO=$(stat -c '%i' "$MNT/dir1")
DIR2_INO=$(stat -c '%i' "$MNT/dir2")
DIR2SUB_INO=$(stat -c '%i' "$MNT/dir2/sub")
ROOT_INO=$(stat -c '%i' "$MNT")

echo "  dir1 -> inode $DIR1_INO"
echo "  dir2 -> inode $DIR2_INO"
echo "  dir2/sub -> inode $DIR2SUB_INO"
echo "  / -> inode $ROOT_INO"

echo "=== step 6: umount ==="
sudo umount "$MNT"

echo "=== step 7: getinodeinfo for all ==="
mkdir -p "$EXTRACT"
for f in "${!INODES[@]}"; do
    ino=${INODES[$f]}
    echo "--- info: $f (inode $ino) ---"
    $VG $GETINFO "$IMG" "$ino" | head -n 40
done

echo "=== step 8: extract file data and verify sha512 ==="
fails=0
for f in "${!INODES[@]}"; do
    ino=${INODES[$f]}
    want=${HASHFILES[$f]}
    got=$($VG $GETDATA "$IMG" "$ino" | sha512sum | awk '{print $1}')
    if [ "$got" = "$want" ]; then
        echo "  OK    $f (inode $ino)"
    else
        echo "  FAIL  $f (inode $ino)"
        echo "    expected $want"
        echo "    got      $got"
        fails=$((fails + 1))
    fi
done

echo "=== step 9: parse dir entries ==="
for d in "/" "dir1" "dir2" "dir2/sub"; do
    case "$d" in
        "/")        ino=$ROOT_INO ;;
        "dir1")     ino=$DIR1_INO ;;
        "dir2")     ino=$DIR2_INO ;;
        "dir2/sub") ino=$DIR2SUB_INO ;;
    esac
    echo "--- dir $d (inode $ino) ---"
    $VG $GETDATA "$IMG" "$ino" | $VG $PARSEDIR -b "$BLOCK_SIZE"
done

echo "=== step 10: test on loop device ==="
LOOP=$(sudo losetup -f)
sudo losetup "$LOOP" "$IMG"
echo "attached to $LOOP"
losetup -a | grep "$IMG" || true
lsblk -o name,size,fstype "$LOOP" || true

ino=${INODES["small.bin"]}
want=${HASHFILES["small.bin"]}
got=$($VG $GETDATA "$LOOP" "$ino" | sha512sum | awk '{print $1}')
if [ "$got" = "$want" ]; then
    echo "  OK loop: small.bin"
else
    echo "  FAIL loop: small.bin"
    fails=$((fails + 1))
fi

sudo losetup -d "$LOOP"

echo
if [ "$fails" -eq 0 ]; then
    echo "=== ALL TESTS PASSED ==="
else
    echo "=== $fails TESTS FAILED ==="
    exit 1
fi
