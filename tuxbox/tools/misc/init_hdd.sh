#! /bin/sh

# This script creates a new partition on the disk $HDD,
# partition 1 is a 100 MB swap partition (presently not used),
# partition 2 is the rest of the disk.
# The partitions will be properly initialized.

# Needless to say, this script will nuke all data on the disc.

echo "This command will initialize you hard disk. It will irrevocably"
echo "ERASE ALL DATA on the disk!! If this is what you want to do,"
echo "enter 3.1415926 to continue."

read ans
if [ $ans = "3.1415926" ] ; then
    echo Continuing...
else
    echo "Bye!"
    exit 1
fi

umount /hdd

HDD=/dev/ide/host0/bus0/target0/lun0

# Create the partition label
fdisk $HDD/disc << EOF
o
n
p
1
1
+100M
n
p
2



t
1
82
p
w
q
EOF

if [ $? -ne 0 ] ; then
    echo "Partitioning failed, aborting"
    exit 1
fi

# Initialize the swap partition
# mkswap /dev/ide/host0/bus0/target0/lun0/part1

# Create the files system (often incorrectly called "formatting").
echo "Now creating the file system. This may take a few minutes."
mkfs.ext3 -T largefile -m0 $HDD/part2
