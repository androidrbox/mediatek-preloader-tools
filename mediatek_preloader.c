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

#define bswap_32(num) ((num >> 24) & 0xff) |     \
                      ((num << 8) & 0xff0000) |  \
                      ((num >> 8) & 0xff00) |    \
                      ((num << 24) & 0xff000000)

#define CMD_READ32            0xD1
#define CMD_WRITE32           0xD4
#define MSDC_FIFOCS           (BASE_ADDR + 0x14)
#define MSDC_TXDATA           (BASE_ADDR + 0x18)
#define MSDC_RXDATA           (BASE_ADDR + 0x1c)
#define MSDC_CMD              (BASE_ADDR + 0x34)
#define MSDC_ARG              (BASE_ADDR + 0x38)
#define MSDC_STS              (BASE_ADDR + 0x3c)
#define MSDC_FIFOCS_RXCNT     0x000000FF
#define MSDC_FIFOCS_TXCNT     0x00FF0000
#define MSDC_FIFO_SZ          128
#define MSDC_STS_SDCBUSY      0x1
#define MMC_SEND_EXT_CSD      8
#define MMC_READ_SINGLE_BLOCK 17
#define MMC_WRITE_BLOCK       24
#define MSDC_RESP_R1          1
#define EXT_CSD_SEC_CNT       212

uint32_t BASE_ADDR = 0;

static uint32_t readwrite32(struct sp_port *port, uint8_t cmd, uint32_t addr, uint32_t val)
{
    uint32_t addrlen;
    uint16_t status;

    sp_blocking_write(port, &cmd, sizeof(cmd), 0);
    sp_blocking_read(port, &cmd, sizeof(cmd), 0);

    addrlen = bswap_32(addr);
    sp_blocking_write(port, &addrlen, sizeof(addrlen), 0);
    sp_blocking_read(port, &addrlen, sizeof(addrlen), 0);

    addrlen = 0x01000000;
    sp_blocking_write(port, &addrlen, sizeof(addrlen), 0);
    sp_blocking_read(port, &addrlen, sizeof(addrlen), 0);

    sp_blocking_read(port, &status, sizeof(status), 0);

    if (cmd == CMD_READ32)
    {
        sp_blocking_read(port, &val, sizeof(val), 0);
        val = bswap_32(val);
    }
    else
    {
        val = bswap_32(val);
        sp_blocking_write(port, &val, sizeof(val), 0);
        sp_blocking_read(port, &val, sizeof(val), 0);
    }

    sp_blocking_read(port, &status, sizeof(status), 0);

    return val;
}

static void msdc_do_command(struct sp_port *port, uint8_t opcode, uint32_t arg)
{
    uint32_t cmd;

    cmd = (BLOCK_SIZE << 16) | (1 << 11) | MSDC_RESP_R1 << 7 | opcode;
    if (opcode == MMC_WRITE_BLOCK)
        cmd |= (1 << 13);

    while (readwrite32(port, CMD_READ32, MSDC_STS, 0) & MSDC_STS_SDCBUSY);

    readwrite32(port, CMD_WRITE32, MSDC_ARG, arg);
    readwrite32(port, CMD_WRITE32, MSDC_CMD, cmd);
}

static void msdc_pio_read(struct sp_port *port, uint32_t *buf, uint32_t count)
{
    while (count > 0)
    {
        if ((readwrite32(port, CMD_READ32, MSDC_FIFOCS, 0) & MSDC_FIFOCS_RXCNT) >= MSDC_FIFO_SZ)
        {
            for (int i = 0; i < (MSDC_FIFO_SZ / 4); i++)
                *buf++ = readwrite32(port, CMD_READ32, MSDC_RXDATA, 0);
            count -= MSDC_FIFO_SZ;
        }
    }
}

static void msdc_pio_write(struct sp_port *port, const uint32_t *buf, uint32_t count)
{
    while (count > 0)
    {
        if ((readwrite32(port, CMD_READ32, MSDC_FIFOCS, 0) & MSDC_FIFOCS_TXCNT) == 0)
        {
            for (int i = 0; i < (MSDC_FIFO_SZ / 4); i++)
                readwrite32(port, CMD_WRITE32, MSDC_TXDATA, *buf++);
            count -= MSDC_FIFO_SZ;
        }
    }
}

static int preadwrite(int write, struct sp_port *port, void *buf, uint32_t count, uint64_t offset)
{
    uint32_t blocks = count / BLOCK_SIZE;

#ifndef __APPLE__
    printf("%09lX:   0%%", offset);
#else
    printf("%09llX:   0%%", offset);
#endif
    for (uint32_t block = 1; count > 0; block++)
    {
        msdc_do_command(port, write ? MMC_WRITE_BLOCK : MMC_READ_SINGLE_BLOCK, offset / BLOCK_SIZE);
        if (write)
            msdc_pio_write(port, buf, BLOCK_SIZE);
        else
            msdc_pio_read(port, buf, BLOCK_SIZE);

        count -= BLOCK_SIZE;
        buf += BLOCK_SIZE;
        offset += BLOCK_SIZE;

        printf("\b\b\b\b%3.0f%%", (block / (float)blocks) * 100);
    }
    printf("\n");

    return 0;
}

int mediatek_pread(struct sp_port *port, void *buf, uint32_t count, uint64_t offset)
{
    printf("Reading ");
    return preadwrite(0, port, buf, count, offset);
}

int mediatek_pwrite(struct sp_port *port, void *buf, uint32_t count, uint64_t offset)
{
    printf("Writing ");
    return preadwrite(1, port, buf, count, offset);
}

int64_t mediatek_get_size(struct sp_port *port)
{
    uint32_t buf[BLOCK_SIZE / sizeof(uint32_t)];

    msdc_do_command(port, MMC_SEND_EXT_CSD, 0);
    msdc_pio_read(port, buf, BLOCK_SIZE);

    return (int64_t)buf[EXT_CSD_SEC_CNT / sizeof(uint32_t)] * BLOCK_SIZE;
}
