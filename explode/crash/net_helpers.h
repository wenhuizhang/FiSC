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


#ifndef __NET_HELPERS_H
#define __NET_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

int net_send_buffer(int s, unsigned int cmd, const void *data, int size);
int net_send_int(int s, unsigned int cmd, const int val);
int net_recv_buffer(int s, unsigned int cmd, void *data, int size);
int Write(int fd, const void *buf, ssize_t count);
int Read(int fd, void *buf, ssize_t count);
int cmd_local_to_net(int cmd);
int send_cmd(int s, int cmd);
int net_get_buffer_log(int s, struct ekm_log_request *req);
int listen_for_client(int port);
int Read(int fd, void *buf, ssize_t count);
int Write(int fd, const void *buf, ssize_t count);
int net_send_buffer_two_args(int s, unsigned int cmd, const void *data, int size, const void *data2, int size2);
int cmd_net_to_local(int cmd);

#ifdef __cplusplus
}
#endif

#endif
