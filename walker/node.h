/*
 * node.h -- node utils
 * Copyright (C) 2022  Jacob Koziej <jacobkoziej@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NODE_H
#define NODE_H


#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <sys/stat.h>


#define NODE_CFG_LNK (1 << 0)
#define NODE_CFG_MTM (1 << 1)
#define NODE_CFG_UID (1 << 2)
#define NODE_CFG_VOL (1 << 3)


typedef struct node_s {
	char           mode[11];
	struct passwd *passwd;
	struct group  *group;
	const char    *path;
	const char    *slpath;
	struct stat    stat;
} node_t;

typedef struct node_cfg_s {
	uint_fast8_t  flags;
	dev_t         sl_dev;
	ino_t         sl_ino;
	long          mtime_s;
	uid_t         uid;
	dev_t         vol;
} node_cfg_t;


int         node_parse(node_t *node);
int         node_fprint(FILE *stream, const node_t *node);
const char *slpath(const char *path);
void        strmode(char buf[11], const mode_t mode);


#endif /* NODE_H */
