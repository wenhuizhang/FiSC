/*
 *
 * Copyright (C) 2008 Can Sar (csar@cs.stanford.edu)
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


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <HostLib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  HostLib hostLib;

  long size;

  if(argc != 1) {
    fprintf(stderr, "Usage: %s\n", argv[0]);
    return 1;
  }

  hostLib.init();

  if(hostLib.ekm_get_log_size(&size) < 0) {
    fprintf(stderr, "Couldn't get log size\n");
    return 3;
  }

  printf("Log Size %ld\n", size);

  return 0;
}
