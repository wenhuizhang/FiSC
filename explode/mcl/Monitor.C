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


#include <iostream>

#include "sysincl.h"
#include "util.h"
#include "Monitor.h"
#include "options.h"

using namespace std;
int Monitor::connect(const char* mon, int port)
{
    int skt;
    struct sockaddr_in mon_addr;
    bzero(&mon_addr, sizeof mon_addr);

    if((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        abort_on(socket);
	
    if(xinet_aton(mon, &mon_addr.sin_addr) < 0)
        abort_on(xinet_aton);

    mon_addr.sin_family = AF_INET;
    mon_addr.sin_port = htons(port);

    if(::connect(skt, (struct sockaddr *)&mon_addr, 
                 sizeof mon_addr) < 0)
        abort_on(connect);
    return skt;
}

void Monitor::send(int skt, cmd_t cmd)
{
    //cout << "skt = " << skt << endl;
    if(xsend(skt, &cmd, sizeof cmd, 0) < 0)
        abort_on(xsend);
}

void Monitor::send_heart_beat(int skt, const char* msg, int msg_len)
{
    cmd_t cmd = heart_beat;
    if(xsend(skt, &cmd, sizeof cmd, 0) < 0)
        abort_on(xsend);

    if(xsend(skt, &msg_len, sizeof msg_len, 0) < 0)
        abort_on(xsend);

    if(msg_len > 0) {
        if(msg_len > MAX_MONITOR_MSG_LEN)
            msg_len = MAX_MONITOR_MSG_LEN;
        if(xsend(skt, msg, msg_len, 0) < 0)
            abort_on(xsend);
    }
}

// FIXME: ignore any extra bytes after the first complete
// command. could cause problems
bool Monitor::cmd_complete(const char* buf, int len, 
                           cmd_t *cmd, struct hb_msg* msg)
{
    const char *p = buf;

    if(len < sizeof *cmd)
        return false;

    memcpy(cmd, p, sizeof *cmd);
    len -= sizeof *cmd;
    p += sizeof *cmd;

    if(*cmd != heart_beat)
        return true;

    if(len < sizeof msg->len)
        return false;
    memcpy(&msg->len, p, sizeof msg->len);
    assert(msg->len < MAX_MONITOR_MSG_LEN);
    len -= sizeof msg->len;
    p += sizeof msg->len;

    if(len < msg->len)
        return false;

    get_hb_msg(buf, msg);
    return true;
}

void Monitor::get_hb_msg(const char* buf, struct hb_msg *msg)
{
    cmd_t cmd;
    const char* p = buf;
    memcpy(&cmd, p, sizeof cmd);
    p += sizeof cmd;
	
    memcpy(&msg->len, p, sizeof msg->len);
    p += sizeof msg->len;

    memcpy(&msg->msg, p, msg->len);
    // make it zero terminated
    msg->msg[msg->len] = '\0';
}

// ideally the monitor and the model checker should run on a
// separate machines.  however vmware workstation lacks a
// programmable interface.  so, just call system("reboot") here
void Monitor::listen(int port)
{
    int skt, fd, yes=1, avail=0, ret, interval = 10, restarted=0;
    struct sockaddr_in myaddr;
    enum cmd_t cmd;
    struct hb_msg msg;
    char cmd_buf[4096];
    fd_set rfds;
    struct timeval tv;

    if((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        abort_on(socket);

    // get rid of the address already in use error
    if(setsockopt(skt, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0)
        abort_on(setsockopt);

    bzero(&myaddr, sizeof myaddr);
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    //myaddr.sin_addr.s_addr = inet_addr(get_option(monitor, addr));

    if(bind(skt, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
        abort_on(bind);

    if(::listen(skt, 10) < 0)
        abort_on(listen);

    for(;;) {
        if((fd=accept(skt, NULL, NULL)) < 0)
            abort_on(accept);
        status_out << "MONITOR: accepted " << fd << endl;

        restarted = 0;
        while(!restarted) {
            tv.tv_sec = get_option(monitor, timeout);
            tv.tv_usec = 0;
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            ret = select(fd+1, &rfds, NULL, NULL, &tv);

            if(ret < 0 && errno == EINTR)
                continue;
            if(ret < 0)
                abort_on(select);

            if(FD_ISSET(fd, &rfds)) { // fd has data to read
                if((ret=recv(fd,cmd_buf+avail,(sizeof cmd_buf)-avail,0))<=0) {
                    status_out << "MONITOR: recv error or socket closed. "\
                        "declare the other side dead" << endl;
                    perror("recv");
                    cmd = prog_dead;
                } else {
                    avail+= ret;
                    if(!Monitor::cmd_complete(cmd_buf, avail, &cmd, &msg))
                        continue;
                    avail = 0;
                    status_out << "MONITOR: received " << cmd << endl;
                }
            } else { // timeout
                status_out << "MONITOR: recv timeouts. "\
                    "declare the other side dead" << endl;
                cmd = prog_dead;
            }

            switch(cmd) {
            case heart_beat:
                status_out << "MONITOR: heart beat message\n" << msg.msg;
                break;

            case want_restart:
            case prog_dead:
                // reboot
                close(fd);
                if(get_option(monitor, use_vm)) {
                    // wait for the other side to close before we reboot
                    sleep(1);
                    systemf("vmrun reset %s", get_option(monitor, vm_path));
                }
                else
                    system("reboot");
                restarted = 1; // restarted. needs to get a new fd
                break;
            case prog_done: 
                // don't need to do anything, actuall.  just quit
                close(fd);
#if 0
                if(get_option(monitor, use_vm))
                    systemf("vmrun stop %s", get_option(monitor, vm_path));
                else
                    system("halt");
                break;
#else
                return;
#endif
            }
        }
    }
}
