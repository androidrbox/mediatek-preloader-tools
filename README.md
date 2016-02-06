# Mediatek Preloader Tools

There are 2 tools provided here.  The first is mediatek_inject and the second
is nbdkit-mtkpreloader.  mediatek_inject will compile for both Windows and
Linux while nbdkit-mtkpreloader will only compile for Linux.

## Prerequisites

To compile for Windows i686-w64-mingw32-gcc is needed.  To compile for Linux
nbdkit-plugin-dev is needed to compile the nbdkit plugin.

To use both of these tools,
handshake.py (https://gitlab.com/zeroepoch/aftv2-tools/blob/master/handshake.py)
is required to handshake with the preloader.

## mediatek_inject

This tool can inject a file in to the target filesystem specified.  It is
restricted to only overwriting files with a file that is less than or equal to
in size.  Additionally, this tool can also overwrite a partition with a full
filesystem image.  Do note that this should be as small as possible as writing
large amounts of data can be quite slow.

## nbdkit-mtkpreloader

This tool can export the flash storage as a network block device within Linux.
Additionally, xnbd-client is required.  This must be xnbd-client as nbd-client
will not work.  The nbd kernel module must be loaded with max_part=16.

To start nbdkit use:

`nbdkit ./nbdkit-mtkpreloader-plugin.so -f tty=NODE device=DEVICE`

To create the block device, run xnbd-client:

`xnbd-client --blocksize 4096 localhost 10809 /dev/nbd0`

After some time, xnbd-client will return and /dev/nbd0 will be usable.
To mount a partition type:

`mount <NBD PARTITION> <MOUNT POINT> -t ext4 -o inode_readahead_blks=0`

After using the mount, unmount it and disconnect xnbd-client:

xnbd-client -d <NBD DEVICE>

You can use CTRL+C to end nbdkit.

### Notes

It is very important to use `-t ext4` when mounting to speed up the mount
process.  Without specifying this, the kernel has to figure out the filesystem
type, which means reading a lot more blocks.  It is also very important to use
`-o inode_readahead_blks=0` when mounting to speed up general partition use.
Without this, the kernel will read extra and most likely unnecessary blocks.
Finally, you can add the the `ro` mount option if you just need read-only
access.  This will speed it up because it won't need to write the superblock.

I have tested this and it works for me.  It is extremely important that you do
not CTRL-C any operations as they are in progress, as this may corrupt the
flash.  If it seems as if something stopped working and you weren't doing a
write, it should be okay to disconnect the device and try again.  Note that if
you didn't properly unmount/disconnect the nbd, you may need to use another
nbd device, such as nbd1.
