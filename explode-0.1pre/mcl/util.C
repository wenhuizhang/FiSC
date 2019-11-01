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
#include "options.h"

using namespace std;

#define MAX_CMD (256)
int execv_interposed(const char* path, int verbose, char *const cmd[])
{
    static const char* envp[] = {
        "LD_PRELOAD=./interpose.so", 
        0};
    if(verbose) {
        int fd = open("/dev/null", O_WRONLY);
        dup2(fd, 1);
    }
    execve(path, cmd, (char**)envp);
    // we shouldn't come back here if execv succeeds...
    perror("execv_interposed");
    return 0;
}


int track_cmd = 0;
int systemf_return = 0;

/*
  extern int track_cmd;
  extern int systemf_return; 
*/

int systemf(const char *fmt, ...)
{
    static char cmd[1024];

    va_list ap;
    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    va_end(ap);

    vv_out << (track_cmd?"INIT:":"")
           << "running cmd \"" << cmd  << "\""
           << endl;

    int ret = system(cmd);
    systemf_return = WEXITSTATUS(ret);
    if(systemf_return != 0)
        vv_out << "command \"" << cmd 
               << "\" returns "<< systemf_return 
               << endl;

    return systemf_return;
}

static const char hexdigits[] = "0123456789abcdef";
const char* to_hex(char c)
{
    static char buf[3];
    int low = c & 0xf;
    int high = (c & 0xf0)>>4;
    buf[0] = hexdigits[high];
    buf[1] = hexdigits[low];
    buf[2] = 0;
    return buf;
}

const char* buf_to_hex(const char* buf, int len)
{
    int i, j;
    static char tmp[4096 + 1];
    if(len > (int) sizeof tmp/2)
        len = sizeof tmp/2;

    for(i=0, j=0; i < len; i++, j+=2) {
        char c = buf[i];
        tmp[j] = hexdigits[(c&0xf0)>>4];
        tmp[j+1] = hexdigits[c&0xf];
    }
    tmp[j] = 0;
    return tmp;
}

int memcmp_long(const char* d1, const char* d2, size_t size)
{
    int ret = memcmp(d1, d2, size);
#if 0
    if(ret) {
        size_t i;
        for(i = 0; i < size; i ++)
            if(d1[i] != d2[i]) {
                cout << i/512<<","<<i%512<< ":\t"
                     << to_hex(d1[i]);
                cout << "\t"
                     << to_hex(d2[i]) << endl;
            }
    }
#endif
    return ret;
}

char *xstrdup(const char* s)
{
    int len = strlen(s);
    return xbufdup(s, len+1);
}

char *xbufdup(const char* buf, int len)
{
    char *p = new char[len];
    assert(p);
    memcpy(p, buf, len);
    return p;
}

void xwrite(int fd, const void* buf, size_t count)
{
    int ret;

    while(count > 0) {
        ret = write(fd, buf, count);
        if(ret < 0) {
            perror("write");
            assert(0);
        }
        buf = (char*)buf + ret;
        count -= ret;
    }
}


// recv until (1) we receive @count bytes, (2) fd is closed, or
// (3) there is an error.  i.e. recv returns -1
ssize_t xrecv (int fd, void* b, size_t count, int flags)
{
    int ret=0;
    char* buf = (char*)b;
    ssize_t cnt = count;
    while (cnt && (ret=recv(fd, buf, cnt, flags)) > 0) {
        //dprintf ("received %d\n", ret);
        buf += ret;
        cnt -= ret;
    }
    return (ret>0)?count:ret;
}

// send until (1) we send @count bytes, (2) fd is closed, or (3)
// there is an error.  i.e. send returns -1
ssize_t xsend (int fd, const void* b, size_t count, int flags)
{
    int ret=0;
    char* buf = (char*)b;
    ssize_t cnt = count;
    while (cnt && (ret=send (fd, buf, cnt, flags)) > 0) {
        //dprintf ("sent %d\n", ret);
        buf += ret;
        cnt -= ret;
    }
    return (ret>0)?count:ret;
}

int xinet_aton(const char* hostname, struct in_addr *inp)
{
    // try x.x.x.x format first
    if(inet_aton(hostname, inp)) // inet_aton returns 0 upon error????
        return 0;
	
    // now query the dns server
    struct hostent *he;
    if((he = gethostbyname(hostname)) == NULL)
        return -1;
    bcopy(inp, he->h_addr, he->h_length);
    return 0;
}

#if defined(HAVE_BACKTRACE) && defined(HAVE_BACKTRACE_SYMBOLS)
/* Obtain a backtrace and print it to stdout. */
void print_trace (void)
{
    void *array[10];
    size_t size;
    char **strings;
    size_t i;
	
    size = backtrace (array, 10);
    strings = backtrace_symbols (array, size);
	
    printf ("Obtained %zd stack frames.\n", size);
	
    for (i = 0; i < size; i++)
        printf ("%s\n", strings[i]);
	
    free (strings);
}
#else
void print_trace(void)
{
    cout<<"not supported\n";
}
#endif
