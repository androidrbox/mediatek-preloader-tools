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
#include <termios.h>
#include <nbdkit-plugin.h>
#include "common.h"

#define THREAD_MODEL NBDKIT_THREAD_MODEL_SERIALIZE_CONNECTIONS
#define mp_name "mtkpreloader"
#define mp_version "1.0"
#define mp_longname "MediaTek Preloader Plugin"
#define mp_description \
	"This plugin allows nbd to connect to the MMC of a\n" \
    "MediaTek Android device while in the preloader.\n"
#define mp_config_help \
	"tty=<NODE>      [required] The device node of the preloader tty.\n" \
	"                This is most likely going to be /dev/ttyACM0.\n\n" \
	"device=<DEVICE> [required] The name of the device being connected to.\n" \
    "                Valid choices: firetv2"

static const char *ttyNode = NULL;

static int mp_config(const char *key, const char *value)
{
	if (!strcmp(key, "tty"))
	{
		ttyNode = value;
		return 0;
	}
	else if (!strcmp(key, "device"))
	{
		for (int i = 0; i < (sizeof(devices) / sizeof(*devices)); i++)
		{
			if (!strcmp(value, devices[i].name))
			{
				BASE_ADDR = devices[i].base_addr;
				return 0;
			}
		}

		nbdkit_error("unknown device '%s'", value);
		return -1;
	}
	else
	{
		nbdkit_error("unknown parameter '%s'", key);
		return -1;
	}
}

static int mp_config_complete(void)
{
	if (ttyNode == NULL || BASE_ADDR == 0)
	{
		nbdkit_error("ERROR: Required options missing!");
		return -1;
	}

    setvbuf(stdout, NULL, _IONBF, 0);

	return 0;
}

static void *mp_open(int readonly)
{
    struct sp_port *port = NULL;

    if ((sp_get_port_by_name(ttyNode, &port) != SP_OK)
            || (sp_open(port, SP_MODE_READ_WRITE) != SP_OK))
    {
        nbdkit_error("open: unable to open '%s': %m", ttyNode);
	}

	return port;
}

static void mp_close(void *port)
{
    sp_close(port);
    sp_free_port(port);
}

static int64_t mp_get_size(void *port)
{
    return mediatek_get_size(port);
}

static int mp_is_rotational(void *port)
{
	return 0;
}

static int mp_pread(void *port, void *buf, uint32_t count, uint64_t offset)
{
    return mediatek_pread(port, buf, count, offset);
}

static int mp_pwrite(void *port, const void *buf, uint32_t count, uint64_t offset)
{
    return mediatek_pwrite(port, (void *)buf, count, offset);
}

static struct nbdkit_plugin plugin = {
	.name = mp_name,
	.version = mp_version,
	.longname = mp_longname,
	.description = mp_description,
	.config = mp_config,
	.config_complete = mp_config_complete,
	.config_help = mp_config_help,
	.open = mp_open,
	.close = mp_close,
	.get_size = mp_get_size,
	.is_rotational = mp_is_rotational,
	.pread = mp_pread,
	.pwrite = mp_pwrite,
};
NBDKIT_REGISTER_PLUGIN(plugin)
