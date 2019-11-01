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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ModelChecker.h"
#include "CrashSim.h"
#include "Permuter.h"
#include "ekm/ekm_log.h"
#include "ekm_lib.h"
#include "util.h"
#include "Powerset.h"
#include "marshal.h"
#include "Cluster.h"

void ClusterBuilder::build(clusters_t& clustebrs)
{
}

void ClusterBuilder::build_syscall_clusters(clusters_t& clusters)
{
#if 0
    log_t::iterator it;
    struct ekm_record_hdr *rec = NULL;
    struct ekm_bh_record *br;
    struct ekm_bio_record *bir;
    int cluster=-1, lsn, before_lsn;
    sector_loc_t loc;

    while(ekm_log_next(rec)) {
        if(EKM_LOG_SYSCALL_P(rec)) {
            lsn = rec->lsn;
            if(_syscall_results.find(lsn) != _syscall_results.end())
                cluster = rec->lsn;
            else {
                // XXX should have only 1 pending syscall at a
                // time for now.
                // XXX hack
                memcpy(&before_lsn, ((char*)rec)+sizeof*rec, 
                       sizeof before_lsn);
                assert(before_lsn == cluster);
                cluster = -1;
            }
        } else if(EKM_LOG_BH_P(rec) && rec->type != EKM_BH_wait) {
            br = (struct ekm_bh_record*)rec;
            loc.first = br->br_device;
            loc.second = br->br_block*(br->br_size/SECTOR_SIZE);
        add_sector:
            if(cluster == -1) 
                cout << "Permuter:  " << loc << " outside a syscall" << endl;
            if(_syscall_clusters.find(cluster) == _syscall_clusters.end()) {
                _syscall_clusters[cluster] = new sectors_t;
                assert(_syscall_clusters[cluster]);
            }
            _syscall_clusters[cluster]->insert(loc);
        } else if(EKM_LOG_BIO_P(rec) && rec->type != EKM_BIO_wait) {
            bir = (struct ekm_bio_record *)rec;
            loc.first = bir->bir_device;
            loc.second = bir->bir_sector;
            goto add_sector;
        }
    }

    syscall_sectors_t::iterator sit;
    for(sit = _syscall_clusters.begin(); sit != _syscall_clusters.end(); sit ++) {
        cout << (*sit).first << ": ";
        sectors_t *sec = (*sit).second;
        sectors_t::iterator sec_it;
        for(sec_it = sec->begin(); sec_it != sec->end(); sec_it++)
            cout << (*sec_it) << " ";
        cout << endl;
    }
#endif
}
