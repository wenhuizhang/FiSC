
// DO NOT EDIT -- automatically generated by ../gen-opt.pl
// from crash.default.options

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <assert.h>

#include "options.h"

using namespace std;

int __ekm__choice_ioctl = 0;

int __ekm__disk_choice_ioctl = 0;

int __ekm__extra_kernel_failures = 0;

int __ekm__log_ioctl = 1;

int __ekm__max_kernel_choicepoints = 255;

int __ekm__net = 0;

const char* __ekm__net_server = NULL;

int __ekm__run_ioctl = 0;

int __fsck__cache = 0;

int __fsck__crash = 0;

int __fsck__deterministic_threshold = 3;

int __fsck__random_tries = 10;

int __permuter__cluster = 0;

int __permuter__kernel_writes = 3;

const char* __permuter__type = NULL;

int __rdd__sectors = 4096;

const char* __recovery_permuter__type = NULL;

int __simulator__default_time = 0;



namespace {

struct _options: public options {

    virtual ~_options() {






    free((void*)__ekm__net_server);
	__ekm__net_server = NULL;








    free((void*)__permuter__type);
	__permuter__type = NULL;


    free((void*)__recovery_permuter__type);
	__recovery_permuter__type = NULL;









    }

    virtual void init() {






    __ekm__net_server = strdup("172.16.97.128");





    __permuter__type = strdup("versioned_buffer");    __recovery_permuter__type = strdup("none");
    }

    virtual int load(const char *dom, const char *opt, const char *val) {
    if (!strcmp (dom, "ekm")) {
        if (!strcmp (opt, "choice_ioctl"))
            { __ekm__choice_ioctl = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "disk_choice_ioctl"))
            { __ekm__disk_choice_ioctl = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "extra_kernel_failures"))
            { __ekm__extra_kernel_failures = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "log_ioctl"))
            { __ekm__log_ioctl = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "max_kernel_choicepoints"))
            { __ekm__max_kernel_choicepoints = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "net"))
            { __ekm__net = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "net_server"))
         {
             char *v = (char *)val;
             if(__ekm__net_server) free((void*)__ekm__net_server);
             if(v[0] == '\"') v++;
             if(v[strlen(v)-1] == '\"') v[strlen(v)-1]=0;
             __ekm__net_server = strdup(val);
             return 1;
        }
        if (!strcmp (opt, "run_ioctl"))
            { __ekm__run_ioctl = (int)strtoul(val, 0, 0); return 1; }

    }
    if (!strcmp (dom, "fsck")) {
        if (!strcmp (opt, "cache"))
            { __fsck__cache = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "crash"))
            { __fsck__crash = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "deterministic_threshold"))
            { __fsck__deterministic_threshold = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "random_tries"))
            { __fsck__random_tries = (int)strtoul(val, 0, 0); return 1; }

    }
    if (!strcmp (dom, "permuter")) {
        if (!strcmp (opt, "cluster"))
            { __permuter__cluster = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "kernel_writes"))
            { __permuter__kernel_writes = (int)strtoul(val, 0, 0); return 1; }
        if (!strcmp (opt, "type"))
         {
             char *v = (char *)val;
             if(__permuter__type) free((void*)__permuter__type);
             if(v[0] == '\"') v++;
             if(v[strlen(v)-1] == '\"') v[strlen(v)-1]=0;
             __permuter__type = strdup(val);
             return 1;
        }

    }
    if (!strcmp (dom, "rdd")) {
        if (!strcmp (opt, "sectors"))
            { __rdd__sectors = (int)strtoul(val, 0, 0); return 1; }

    }
    if (!strcmp (dom, "recovery_permuter")) {
        if (!strcmp (opt, "type"))
         {
             char *v = (char *)val;
             if(__recovery_permuter__type) free((void*)__recovery_permuter__type);
             if(v[0] == '\"') v++;
             if(v[strlen(v)-1] == '\"') v[strlen(v)-1]=0;
             __recovery_permuter__type = strdup(val);
             return 1;
        }

    }
    if (!strcmp (dom, "simulator")) {
        if (!strcmp (opt, "default_time"))
            { __simulator__default_time = (int)strtoul(val, 0, 0); return 1; }

    }

        return 0;
    }
    
    virtual void print(std::ostream &o) {
    o << "ekm::choice_ioctl    " << __ekm__choice_ioctl << endl;
    o << "ekm::disk_choice_ioctl    " << __ekm__disk_choice_ioctl << endl;
    o << "ekm::extra_kernel_failures    " << __ekm__extra_kernel_failures << endl;
    o << "ekm::log_ioctl    " << __ekm__log_ioctl << endl;
    o << "ekm::max_kernel_choicepoints    " << __ekm__max_kernel_choicepoints << endl;
    o << "ekm::net    " << __ekm__net << endl;
    o << "ekm::net_server    " << __ekm__net_server << endl;
    o << "ekm::run_ioctl    " << __ekm__run_ioctl << endl;    o << "fsck::cache    " << __fsck__cache << endl;
    o << "fsck::crash    " << __fsck__crash << endl;
    o << "fsck::deterministic_threshold    " << __fsck__deterministic_threshold << endl;
    o << "fsck::random_tries    " << __fsck__random_tries << endl;    o << "permuter::cluster    " << __permuter__cluster << endl;
    o << "permuter::kernel_writes    " << __permuter__kernel_writes << endl;
    o << "permuter::type    " << __permuter__type << endl;    o << "rdd::sectors    " << __rdd__sectors << endl;    o << "recovery_permuter::type    " << __recovery_permuter__type << endl;    o << "simulator::default_time    " << __simulator__default_time << endl;        
    }
    
};

struct register_options x(new _options);

}

