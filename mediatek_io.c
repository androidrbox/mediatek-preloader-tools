/*
 * Copyright (c) 2016 rbox
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "common.h"

struct mediatek_private_data
{
    struct sp_port *port;
    uint64_t offset;
};

static errcode_t mediatek_open(const char *name, int flags, io_channel *channel)
{
    io_channel io = NULL;
    struct mediatek_private_data *data = NULL;

    ext2fs_get_mem(sizeof(struct struct_io_channel), &io);
    memset(io, 0, sizeof(struct struct_io_channel));
    io->magic = EXT2_ET_MAGIC_IO_CHANNEL;
    io->manager = mediatek_io_manager;
    ext2fs_get_mem(strlen(name) + 1, &io->name);
    strcpy(io->name, name);
    io->block_size = 0;
    io->read_error = 0;
    io->write_error = 0;
    io->refcount = 1;

    ext2fs_get_mem(sizeof(struct mediatek_private_data), &data);
    io->private_data = data;
    memset(data, 0, sizeof(struct mediatek_private_data));
    data->port = (struct sp_port *)strtoul(name, NULL, 16);

    *channel = io;
    return 0;
}

static errcode_t mediatek_close(io_channel channel)
{
    if (--channel->refcount > 0)
        return 0;

    ext2fs_free_mem(&channel->private_data);
    ext2fs_free_mem(&channel->name);
    ext2fs_free_mem(&channel);

    return 0;
}

static errcode_t mediatek_set_blksize(io_channel channel, int blksize)
{
    channel->block_size = blksize;
    return 0;
}

static errcode_t mediatek_read_blk(io_channel channel, unsigned long block, int count, void *buf)
{
    struct mediatek_private_data *data = (struct mediatek_private_data *)channel->private_data;
    size_t size = (count < 0) ? -count : (count * channel->block_size);

    mediatek_pread(data->port, buf, size, data->offset + (block * channel->block_size));

    return 0;

}

static errcode_t mediatek_write_blk(io_channel channel, unsigned long block, int count, const void *buf)
{
    struct mediatek_private_data *data = (struct mediatek_private_data *)channel->private_data;
    size_t size = (count < 0) ? -count : (count * channel->block_size);

    mediatek_pwrite(data->port, (void *)buf, size, data->offset + (block * channel->block_size));

    return 0;

}

static errcode_t mediatek_set_option(io_channel channel, const char *option, const char *arg)
{
    struct mediatek_private_data *data = (struct mediatek_private_data *)channel->private_data;

    data->offset = strtoul(option, NULL, 10);

    return 0;
}


static struct struct_io_manager struct_mediatek_manager =
{
    .magic           = EXT2_ET_MAGIC_IO_MANAGER,
    .name            = "Mediatek Preloader I/O Manager",
    .open            = mediatek_open,
    .close           = mediatek_close,
    .set_blksize     = mediatek_set_blksize,
    .read_blk        = mediatek_read_blk,
    .write_blk       = mediatek_write_blk,
    .flush           = NULL,
    .write_byte      = NULL,
    .set_option      = mediatek_set_option,
    .get_stats       = NULL,
    .read_blk64      = NULL,
    .write_blk64     = NULL,
    .discard         = NULL,
    .cache_readahead = NULL,
    .zeroout         = NULL,
};

io_manager mediatek_io_manager = &struct_mediatek_manager;
