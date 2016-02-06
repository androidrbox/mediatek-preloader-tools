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
#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <unistd.h>
#include "ext2fs/ext2fs.h"
#include "libserialport.h"

#define BLOCK_SIZE 512

extern uint32_t BASE_ADDR;

#ifdef WANT_DEVICES
static struct
{
    const char *name;
    uint32_t base_addr;
} devices[] =
{
    { "firetv2", 0x11230000 },
};

static inline int setBaseAddr(char *device)
{
    for (int i = 0; i < (sizeof(devices) / sizeof(*devices)); i++)
    {
        if (!strcmp(devices[i].name, device))
        {
            BASE_ADDR = devices[i].base_addr;
            return 1;
        }
    }

    return 0;
}
#endif

extern io_manager mediatek_io_manager;

extern int mediatek_pread(struct sp_port *port, void *buf, uint32_t count, uint64_t offset);
extern int mediatek_pwrite(struct sp_port *port, void *buf, uint32_t count, uint64_t offset);
extern int64_t mediatek_get_size(struct sp_port *port);

#endif
