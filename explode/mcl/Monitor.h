/* -*- Mode: C++ -*- */

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


#ifndef __MONITOR_H
#define __MONITOR_H

#define MAX_MONITOR_MSG_LEN (4096)		

class Monitor {
public:
    enum cmd_t {
        heart_beat,
        want_restart,
        prog_done,
        prog_dead,
    };

private:
    struct hb_msg {
        int len;
        char msg[MAX_MONITOR_MSG_LEN];
    };

    static bool cmd_complete(const char* buf, int len, 
                             cmd_t *cmd, struct hb_msg* msg);
    static void get_hb_msg(const char* buf, struct hb_msg *msg);
	
public:
	
    static int connect(const char* mon, int port);
    static void send(int skt, cmd_t cmd);
    static void send_heart_beat(int skt, const char* msg, int msg_len);
    static void listen(int port);
};

#endif
