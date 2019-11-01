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
#include <fcntl.h>
#include <stdio.h>

#include "NetLib.h"
#include "driver/AbstFS.h"

int main(int argc, char **argv) {
  const char *host;
  if(argc > 2) {
    fprintf(stderr, "Usage: %s [host]\n", argv[0]);
    return 1;
  }
  if(argc == 1)
    host = "localhost";
  else
    host = argv[1];

  NetLib *netLib = new NetLib(host);
  AbstFS fs;

  int fd;
  char buf[7];

  netLib->init();

  if(netLib->systemf("echo test") < 0) {
    fprintf(stderr, "Couldn't echo\n");
    return 3;
  }

  netLib->abstract_path("/home/csar/testdir", fs);

  fs.print();

  if((fd = netLib->creat("/home/csar/testdir/a", 0666)) < 0) {
    fprintf(stderr, "Couldn't creat\n");
    return 3;
  }

  if(netLib->write(fd, (void *) "test", 4) != 4) { 
    fprintf(stderr, "Couldn't write\n");
    return 3;
  }

  if(netLib->pwrite(fd, (void *) "sting", 5, 2) != 5) { 
    fprintf(stderr, "Couldn't pwrite\n");
    return 3;
  }

  if(netLib->close(fd) != 0) {
    fprintf(stderr, "Couldn't close\n");
    return 3;
  }

  if((fd = netLib->open("/home/csar/testdir/a", O_RDONLY)) < 0) {
    fprintf(stderr, "Couldn't creat\n");
    return 3;
  }

  if(netLib->read(fd, buf, 7) != 7) { 
    fprintf(stderr, "Couldn't read\n");
    return 3;
    }

  if(memcmp(buf, "testing", 7) != 0) {
    fprintf(stderr, "Bbuffers didn't match\n");
    return 3;
    }

  if(netLib->close(fd) != 0) {
    fprintf(stderr, "Couldn't close\n");
    return 3;
  }

  if(netLib->mkdir("/home/csar/testdir/b", 0777) < 0) {
    fprintf(stderr, "Couldn't mkdir\n");
    return 3;
  }

  if(netLib->symlink("/home/csar/testdir/a", "/home/csar/testdir/c") < 0) {
    fprintf(stderr, "Couldn't unlink\n");
    return 3;
  }

  if(netLib->link("/home/csar/testdir/c", "/home/csar/testdir/d") < 0) {
    fprintf(stderr, "Couldn't unlink\n");
    return 3;
  }

  if(netLib->rename("/home/csar/testdir/d", "/home/csar/testdir/e") < 0) {
    fprintf(stderr, "Couldn't unlink\n");
    return 3;
  }

  if(netLib->unlink("/home/csar/testdir/a") < 0) {
    fprintf(stderr, "Couldn't unlink\n");
    return 3;
  }

  if(netLib->rmdir("/home/csar/testdir/b") < 0) {
    fprintf(stderr, "Couldn't rmdir\n");
    return 3;
  }

  return 0;
}
