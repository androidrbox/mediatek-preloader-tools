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
#define WANT_DEVICES
#include <fcntl.h>
#include "common.h"

#define MULTIPLE_ROUND(v, m) ((v + (m - 1)) & ~(m - 1))

#define LBA_SIZE 512
typedef struct
{
    char signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved1;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t disk_guid[16];
    uint64_t part_entry_start_lba;
    uint32_t num_part_entries;
    uint32_t part_entry_size;
    uint32_t crc32_part_array;
    uint8_t reserved2[420];
} gpt_header;
typedef struct
{
    uint8_t type[16];
    uint8_t unique[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t flags;
    uint8_t name[72];
} gpt_partition;

static uint64_t locate_partition(struct sp_port *port, char *partition)
{
    char pname[sizeof(((gpt_partition *)0)->name)] = { 0 };
    for (int i = 0; i < strlen(partition); i++)
        pname[i * 2] = partition[i];

    gpt_header gptHeader;
    mediatek_pread(port, &gptHeader, BLOCK_SIZE, LBA_SIZE);
    if (strncmp(gptHeader.signature, "EFI PART", sizeof(gptHeader.signature)) == 0)
    {
        gpt_partition partitions[16];

        mediatek_pread(port, partitions, sizeof(partitions),
                       gptHeader.part_entry_start_lba * LBA_SIZE);

        for (int i = 0; i < gptHeader.num_part_entries; i++)
        {
            if (memcmp(partitions[i].name, pname, sizeof(pname)) == 0)
            {
                return partitions[i].first_lba * LBA_SIZE;
            }
        }
    }

    return 0;
}

static void write_file(struct sp_port *port, char *src, char *dst, char *context)
{
    /* target file must have format /partition/path where partition is the name
     * of the GPT partition to mount and path is the path within the filesystem. */
    char *partition = strtok(dst, "/");
    char *tgtfile = strtok(NULL, "");

    printf("Locating the partition...\n");
    uint64_t partition_offset = locate_partition(port, partition);
    if (partition_offset == 0)
    {
        fprintf(stderr, "error: unable to locate '%s' partition!\n", partition);
        return;
    }

    int srcfile = open(src, O_RDONLY);
    if (srcfile == -1)
    {
        fprintf(stderr, "error: unable to open source file!\n");
        return;
    }

    off_t srcfile_size = lseek(srcfile, 0, SEEK_END);
    off_t srcfile_size_rounded = MULTIPLE_ROUND(srcfile_size, 4096);
    lseek(srcfile, 0, SEEK_SET);
    char *srcfile_buffer = calloc(1, srcfile_size_rounded);
    read(srcfile, srcfile_buffer, srcfile_size);
    close(srcfile);

    if (tgtfile == NULL)
    {
        printf("No target file specified.  Is this a partition image?\n");
        printf("If this is not, DO NOT CONTINUE, otherwise press 'y' to continue.\n");
        printf("Write partition image? <y|n> ");
        if (getchar() != 'y')
        {
            fprintf(stderr, "error: target file malformed!\n");
            return;
        }

        printf("Writing the filesystem image...\n");
        mediatek_pwrite(port, srcfile_buffer, srcfile_size_rounded, partition_offset);
    }
    else
    {
        ext2_filsys fs;
        printf("Opening the filesystem...\n");
        char ext2fs_name[40];
#ifndef __APPLE__
        snprintf(ext2fs_name, sizeof(ext2fs_name), "%p?%ld", port, partition_offset);
#else
        snprintf(ext2fs_name, sizeof(ext2fs_name), "%p?%lld", port, partition_offset);
#endif
        if (ext2fs_open(ext2fs_name, IO_FLAG_RW, 0, 0, mediatek_io_manager, &fs))
        {
            fprintf(stderr, "error: unable to open filesystem!\n");
            return;
        }

        ext2_ino_t ino;
        printf("Locating the target file...\n");
        if (ext2fs_namei(fs, EXT2_ROOT_INO, EXT2_ROOT_INO, tgtfile, &ino))
        {
            fprintf(stderr, "error: unable to find target file!\n");

            ext2fs_close(fs);
            return;
        }

        ext2_file_t file;
        printf("Opening the target file...\n");
        if (ext2fs_file_open(fs, ino, EXT2_FILE_WRITE, &file))
        {
            fprintf(stderr, "error: unable to open target file!\n");

            ext2fs_close(fs);
            return;
        }

        ext2_off_t tgtfile_size = MULTIPLE_ROUND(ext2fs_file_get_size(file), fs->blocksize);
        if (srcfile_size_rounded > tgtfile_size)
        {
            fprintf(stderr, "error: source file is larger than target file!\n");

            ext2fs_file_close(file);
            ext2fs_close(fs);
            return;
        }

        printf("Writing target file...\n");
        ext2fs_file_write(file, srcfile_buffer, srcfile_size_rounded, NULL);

        printf("Setting target file size...\n");
        ext2fs_file_set_size(file, srcfile_size);

        printf("Closing the file...\n");
        ext2fs_file_close(file);

        if (*context != '-')
        {
            struct ext2_xattr_handle *xattrs;
            printf("Reading extended attributes...\n");
            if (ext2fs_xattrs_open(fs, ino, &xattrs) || ext2fs_xattrs_read(xattrs))
            {
                fprintf(stderr, "error: unable to read extended attributes!\n");

                ext2fs_close(fs);
                return;
            }

            printf("Setting SELinux context...\n");
            if (ext2fs_xattr_set(xattrs, "security.selinux", context, strlen(context)) || ext2fs_xattrs_write(xattrs))
            {
                fprintf(stderr, "error: unable to set selinux context!\n");

                ext2fs_xattrs_close(&xattrs);
                ext2fs_close(fs);
                return;
            }
        }
    }
}

int main(int argc, char **argv)
{
    struct sp_port *port = NULL;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Mediatek Inject, git version %s\n", GIT_VERSION);

    if (argc < 6)
    {
        fprintf(stderr, "error: %s <device> <comport> <source file> <target file> <selinux context>\n", argv[0]);
    }
    else if (setBaseAddr(argv[1]) == 0)
    {
        fprintf(stderr, "error: unknown device name!\n");
    }
    else if ((sp_get_port_by_name(argv[2], &port) != SP_OK) || (sp_open(port, SP_MODE_READ_WRITE) != SP_OK))
    {
        fprintf(stderr, "error: unable to open comport!\n");
    }
    else
    {
        write_file(port, argv[3], argv[4], argv[5]);
    }

    sp_close(port);
    sp_free_port(port);

    return 0;
}
