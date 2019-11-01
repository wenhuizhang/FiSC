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


#include "crash.h"

#include <errno.h>

// unload all kernel modules
static void unload_modules(int, void*)
{
    systemf("sudo ./unload");
}

void assemble(Component *&test, TestDriver *&driver);
int main(int argc, char *argv[])
{
    unload_modules(0, NULL);
    on_exit(unload_modules, NULL);

#if defined(__linux__)
    // don't allow oom_killer kills us. -16 is the minimum
    systemf("sudo ./set_oom_adj %d -16", getpid());
#endif

    // call this before you do anything: this will read in explode.options
    xe_process_cmd_args(argc, argv);

    Component *st;
    TestDriver *driver;

    assemble(st, driver);
    sim()->driver(driver);
    sim()->stack(st);

    xe_check(sim());
}
