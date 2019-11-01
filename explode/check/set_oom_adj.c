/*
 *
 * Copyright (C) 2008 Junfeng Yang (junfeng@cs.columbia.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */


#include <stdlib.h>
#include <stdio.h>

static void
set_oom_adj (int pid, int val)
{
        FILE *fp;
	char name[4096];

	sprintf(name, "/proc/%d/oom_adj", pid);

        fp = fopen(name, "w");

        if (!fp)
                return;

        fprintf(fp, "%i", val);
        fclose(fp);
}


int main(int argc, char* argv[])
{
	int pid, val;
	if(argc != 3) {
		printf("USAGE: %s <pid> <value>\n", argv[0]);
		return 0;
	}

	pid = atoi(argv[1]);
	val = atoi(argv[2]);
	set_oom_adj(pid, val);
	return 0;
}
