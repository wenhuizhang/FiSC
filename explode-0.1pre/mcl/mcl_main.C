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


#include <string>
#include <iostream>

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <getopt.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "mcl.h"
#include "ModelChecker.h"


using namespace std;

// TODO: all these crazy messy command line options are directly
// adapted from old system. need to clean them sometime.
static int run_trace = 0;
static int set_opt = 0;
static int dump_opt = 0;
static int redirect_output_opt = 0;
static int no_default_opt = 0;
static int start_from_ckpt = 0;

static string trace_file("explode.trace");
static string opt_fname;
static string output_fname;
static string dump_dname;
static string cmd_opt;
static string ckpt_file;

static const struct option long_opts[] = {
    {"no-default-opt",  0, 0, 0},
    {"replay",  1, 0, 'r'},
    {"use-options",  1, 0, 'o'},
    {"dump-options",  1, 0, 'd'},
    {"resume-from-checkpoint",  1, 0, 'c'},
};

void parse_options (int argc, char *argv[])
{
    int c, long_opt_ind;
    struct stat st;
    while ((c = getopt_long(argc, argv, "f:r:o:d:s:c:", 
                            long_opts, &long_opt_ind)) != -1) {
        switch (c) {
        case 0:
            no_default_opt = 1;
            break;
        case 'f':
            redirect_output_opt = 1;
            output_fname = string(optarg);
            break;
        case 'r':
            run_trace = 1;
            trace_file = string (optarg);
            if (!set_opt)
                opt_fname = trace_file + ".options";
            break;
        case 'o':
            set_opt = 1;
            opt_fname = string(optarg);
            break;
        case 'd':
            dump_opt = 1;
            dump_dname = string(optarg);
            break;
        case 'c':
            start_from_ckpt = 1;
            ckpt_file = string(optarg);
            if(stat(ckpt_file.c_str(), &st) < 0)
                // file not there
                start_from_ckpt = 0;
            break;
        default:
            assert(0);
        }
    }
}

void xe_process_cmd_args(int argc, char *argv[])
{
    parse_options(argc, argv);

    // NB this needs to be done as early as possible
    if (redirect_output_opt) {
        const char *fname = output_fname.c_str();
        int flags = O_CREAT | O_TRUNC | O_WRONLY;
        int fd = open(fname, flags, S_IRUSR | S_IWUSR);
        if (fd == -1) {
            printf("cannot open output file: %s\n", fname);
            printf("reason: %s\n", strerror(errno));
            exit(1);
        }

        // XXX: to be strictly kosher we should close stdout and stderr
        // and check for errors here. Also, if we have signal
        // handlers we should not write anything out until this
        // operation is complete.
        if (dup2(fd, 1) == -1) {
            printf("cannot tie stdout to %s\n", fname);
            printf("reason: %s\n", strerror(errno));
            exit(1);
        }
        if (dup2(fd, 2) == -1) {
            printf("cannot tie stderr to %s\n", fname);
            printf("reason: %s\n", strerror(errno));
            exit(1);
        }
    }

    // priority: -o opt_file > -r trace > explode.options
    if (!no_default_opt)
        load_options("explode.options");

    if (run_trace || set_opt)
        load_options(opt_fname.c_str());
	
    // need a better way to set random seed
    if(get_option(mcl, random_seed) == 0)
        set_option(mcl, random_seed, rand());
    srand(get_option(mcl, random_seed));

    if (dump_opt) {
        mkdir (dump_dname.c_str(), 0777);
        opt_fname = dump_dname + "/explode.options";
        print_options(opt_fname.c_str());
        return;
    }

    print_options("unified.options");

    // create the trace dir
    mkdir(get_option(mcl, trace_dir), 0777);

    if(run_trace)
        explode = new ModelChecker(MODE_REPLAY);
    else {
        if(!start_from_ckpt)
            explode = new ModelChecker(MODE_MODEL_CHECK);
        else
            explode = ModelChecker::self_restore(ckpt_file.c_str());
    }
}


void xe_check(void* test_harness)
{
    TestHarness* test = (TestHarness*)test_harness;

    explode->register_test(test);
	
    if (explode->mode() == MODE_REPLAY)
        explode->replay_trace_file(trace_file.c_str());
    else // explode->mode() == MODE_MODEL_CHECK
        explode->model_check();
    delete explode;
}
