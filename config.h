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
#ifndef _CONFIG_H
#define _CONFIG_H

#ifdef LIBSERIALPORT

    #define SP_API
    #define SP_PRIV

#else /* LIBSERIALPORT */

    #ifdef WIN32

        #include <unistd.h>

        #define F_GETFD 1
        #define F_SETFD 2
        #define FD_CLOEXEC 1
        #define HAVE_SYS_STAT_H 1

        static inline unsigned short getuid(void) { return 0; }
        static inline unsigned short geteuid(void) { return 0; }
        static inline unsigned short getgid(void) { return 0; }
        static inline unsigned short getegid(void) { return 0; }
        static inline int fcntl(int __fd, int __cmd, ...) { return -1; }
        static inline int fsync(int fd) { return -1; }

    #else /* WIN32 */

        #ifndef __APPLE__
            #define HAVE_MNTENT_H 1
            #define HAVE_SETMNTENT 1
        #endif
        #define HAVE_NETINET_IN_H 1
        #define HAVE_SYS_STAT_H 1
        #define HAVE_UNISTD_H 1

    #endif

#endif

#endif
