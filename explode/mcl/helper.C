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


/*
  all the explode accessories go here
*/

#include "mcl.h"
#include "ModelChecker.h"
#include <cstdio>
#include <stdarg.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int xe_random(int ceil) 
{
    explode->check_valid_choose();
    return explode->get_rand()->random(ceil);
}

int unix_random(int ceil)
{
    return explode->get_unix_rand()->random(ceil);
}

int xe_random_fail (void)
{
    explode->check_valid_choose();
    return explode->get_rand()->random_fail();
}

void xe_random_skip_rest(void)
{
    EnumRandom *r = dynamic_cast<EnumRandom*>(explode->get_rand());
    r->skip_rest();
}

int xe_mode(void) 
{
    return explode->mode();
}

const char* xe_trace_dir(void)
{
    return explode->trace_dir();
}

void xe_schedule_restart(void)
{
    // schedule a restart
    explode->restart(0);
}

void xe_set_score(int score)
{
    explode->set_score(score);
}

#ifndef CONF_USE_EXCEPTION
jmp_buf run_test_env;
#endif

void error_(const char *file, int line, 
	    const char *type, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stdout, "ERROR:%s:%d: [type=%s] [trace=%s] ", 
            file, line, type, explode->trace_dir());
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
    /*
      if(get_option(explode, simulation)) {
      explode->prfocess_test_return(TEST_BUG);
      return;
      }
    */
		
#ifdef CONF_USE_EXCEPTION
    static char buf[4096];
    va_start(ap, fmt);
    sprintf(buf, "ERROR:%s:%d: [type=%s] [trace=%s] ", 
            file, line, type, explode->trace_dir());
    vsprintf(buf, fmt, ap);
    va_end(ap);

    throw CheckerException(TEST_BUG, buf);
#else
    longjmp(run_test_env, TEST_BUG);
#endif
}

void xe_msg(int code, const char* file, int line,
                 const char* fmt, ...)
{
    static char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    sprintf(buf, "FiSC %s:%s:%d: ", 
            (code==TEST_ABORT)?"ABORT":"EXIT",	file, line);
    vsprintf(buf, fmt, ap);
    va_end(ap);

    fprintf(stdout, "%s\n", buf);

#ifdef CONF_USE_EXCEPTION
    throw CheckerException(code, buf);
#else
    longjmp(run_test_env, code);
#endif
}


void xi_assert_failed(const char* file, 
                      int line, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Assertion failed at %s, %d: ", file, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "!\n");
    va_end(ap);
    abort();
}

void xi_warn_(const char* file,
              int line, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Warn at %s, %d: ", file, line);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
