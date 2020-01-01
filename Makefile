 # Copyright (c) 2016 rbox
 #
 # This program is free software: you can redistribute it and/or modify it
 # under the terms of the GNU General Public License as published by the Free
 # Software Foundation, either version 3 of the License, or (at your option)
 # any later version.
 #
 # This program is distributed in the hope that it will be useful, but WITHOUT
 # ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 # FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 # more details.
 #
 # You should have received a copy of the GNU General Public License along
 # with this program.  If not, see <http://www.gnu.org/licenses/>.

GIT_VERSION := $(shell git describe --abbrev=6 --dirty --always --tags)
E2FSPROGS_LIB = e2fsprogs/lib
SERIALPORT = libserialport
CFLAGS = -std=gnu99 -Wall -Werror -Wno-error=deprecated-declarations -DGIT_VERSION=\"$(GIT_VERSION)\" \
		 -I. -I$(E2FSPROGS_LIB) -I$(SERIALPORT)
LDLIBS = -lpthread

ifeq ($(OS),windows)
    CC = i686-w64-mingw32-gcc
    CFLAGS += -Wno-format -Wno-cpp
    LDFLAGS = -static
    LDLIBS += -lsetupapi -lws2_32
else ifeq ($(OS),osx)
    CC = x86_64-apple-darwin15-clang
	CFLAGS += -DHAVE_SYS_TYPES_H -Wno-\#warnings
	LDFLAGS += -framework IOKit -framework CoreFoundation
else
    CFLAGS += -fPIC
endif

$(SERIALPORT)/%: CFLAGS += -DLIBSERIALPORT

DEPS = mediatek_inject.o mediatek_io.o mediatek_preloader.o \
	$(E2FSPROGS_LIB)/et/com_err.o \
	$(E2FSPROGS_LIB)/et/error_message.o \
	$(E2FSPROGS_LIB)/et/et_name.o \
	$(E2FSPROGS_LIB)/ext2fs/alloc.o \
	$(E2FSPROGS_LIB)/ext2fs/alloc_sb.o \
	$(E2FSPROGS_LIB)/ext2fs/alloc_stats.o \
	$(E2FSPROGS_LIB)/ext2fs/badblocks.o \
	$(E2FSPROGS_LIB)/ext2fs/bitmaps.o \
	$(E2FSPROGS_LIB)/ext2fs/bitops.o \
	$(E2FSPROGS_LIB)/ext2fs/blkmap64_ba.o \
	$(E2FSPROGS_LIB)/ext2fs/blkmap64_rb.o \
	$(E2FSPROGS_LIB)/ext2fs/blknum.o \
	$(E2FSPROGS_LIB)/ext2fs/block.o \
	$(E2FSPROGS_LIB)/ext2fs/bmap.o \
	$(E2FSPROGS_LIB)/ext2fs/closefs.o \
	$(E2FSPROGS_LIB)/ext2fs/crc16.o \
	$(E2FSPROGS_LIB)/ext2fs/crc32c.o \
	$(E2FSPROGS_LIB)/ext2fs/csum.o \
	$(E2FSPROGS_LIB)/ext2fs/dir_iterate.o \
	$(E2FSPROGS_LIB)/ext2fs/dirblock.o \
	$(E2FSPROGS_LIB)/ext2fs/ext_attr.o \
	$(E2FSPROGS_LIB)/ext2fs/extent.o \
	$(E2FSPROGS_LIB)/ext2fs/fallocate.o \
	$(E2FSPROGS_LIB)/ext2fs/fileio.o \
	$(E2FSPROGS_LIB)/ext2fs/freefs.o \
	$(E2FSPROGS_LIB)/ext2fs/gen_bitmap.o \
	$(E2FSPROGS_LIB)/ext2fs/gen_bitmap64.o \
	$(E2FSPROGS_LIB)/ext2fs/get_num_dirs.o \
	$(E2FSPROGS_LIB)/ext2fs/i_block.o \
	$(E2FSPROGS_LIB)/ext2fs/ind_block.o \
	$(E2FSPROGS_LIB)/ext2fs/inline.o \
	$(E2FSPROGS_LIB)/ext2fs/inline_data.o \
	$(E2FSPROGS_LIB)/ext2fs/inode.o \
	$(E2FSPROGS_LIB)/ext2fs/io_manager.o \
	$(E2FSPROGS_LIB)/ext2fs/ismounted.o \
	$(E2FSPROGS_LIB)/ext2fs/lookup.o \
	$(E2FSPROGS_LIB)/ext2fs/mkjournal.o \
	$(E2FSPROGS_LIB)/ext2fs/mmp.o \
	$(E2FSPROGS_LIB)/ext2fs/namei.o \
	$(E2FSPROGS_LIB)/ext2fs/openfs.o \
	$(E2FSPROGS_LIB)/ext2fs/punch.o \
	$(E2FSPROGS_LIB)/ext2fs/rbtree.o \
	$(E2FSPROGS_LIB)/ext2fs/read_bb.o \
	$(E2FSPROGS_LIB)/ext2fs/rw_bitmaps.o \
	$(SERIALPORT)/serialport.o

TARGETS = $(TARGET)
ifeq ($(OS),windows)
DEPS += $(SERIALPORT)/windows.o
TARGET = mediatek_inject.exe
else ifeq ($(OS),osx)
DEPS += $(SERIALPORT)/macosx.o
TARGET = mediatek_inject.osx
else
DEPS += $(SERIALPORT)/linux.o $(SERIALPORT)/linux_termios.o
TARGET = mediatek_inject.linux
TARGETS = $(TARGET) nbdkit-mtkpreloader-plugin.so
endif

all: $(TARGETS)
$(TARGET): $(DEPS)
	$(LINK.o) $(CFLAGS) $^ $(LDLIBS) -o $@

nbdkit-mtkpreloader-plugin.so: nbdkit-mtkpreloader.o mediatek_preloader.o \
	$(SERIALPORT)/serialport.o \
	$(SERIALPORT)/linux.o \
	$(SERIALPORT)/linux_termios.o
	$(CC) -shared -o $@ $^

clean:
	find -name '*.o' -delete
