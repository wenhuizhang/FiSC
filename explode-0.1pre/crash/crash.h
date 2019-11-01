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


// libcrash.a client interface

#ifndef __CRASH_H
#define __CRASH_H

#include "mcl.h"
#include "CrashSim.h"
#include "Log.h"
#include "ekm_lib.h"
#include "crash-options.h"

// shortcuts
CrashSim* sim(void);
Permuter* permuter(void);
HostLib* ekm(void);

#include "Permuter.h"
#include "SyscallPermuter.h"

static inline void forget_old_crashes(void)
{
    permuter()->forget_old_crashes();
}

#define CHECK_CRASHES_PREFIX \
    if(xe_random(2) == 1) { \
        forget_old_crashes(); \
        break; /* breaking out the do-while loop */ \
    }

#define check_all_crashes(args...) \
do{ \
    CHECK_CRASHES_PREFIX \
    permuter()->check_all_crashes_ex(this, ##args); \
    /* end this path */\
    xe_skip("finished checking crashes. exit this path."); \
} while(0)

#define check_now_crashes(args...) \
do{ \
    CHECK_CRASHES_PREFIX \
    permuter()->check_now_crashes_ex(this, ##args); \
    /* end this path */\
    xe_skip("finished checking crashes. exit this path."); \
} while(0)

#define begin_commit(args...) \
do{ \
    CHECK_CRASHES_PREFIX \
    permuter()->begin_commit_ex(this, ##args); \
    /* end this path */\
    xe_skip("finished checking crashes. exit this path."); \
} while(0)

#define end_commit(args...) \
do{ \
    CHECK_CRASHES_PREFIX \
    permuter()->end_commit_ex(this, ##args); \
    /* end this path */\
    xe_skip("finished checking crashes. exit this path."); \
} while(0)


#endif
