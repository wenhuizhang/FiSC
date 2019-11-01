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


#include "Fsck.h"
#include "BlockDev.h"
#include "Permuter.h"
#include "CrashSim.h"
#include "ChunkDB.h"
#include "BufferCache.h"
#include "crash-options.h"

//#define DEBUG

using namespace std;
Fsck fsck;

void Fsck::clear(node *n)
{
    if(n == NULL)
        return;

    Sgi::hash_map<signature_t, node*>::iterator it;
    for_all_iter(it, n->next) 
        clear((*it).second);
    delete n;
}

Fsck::~Fsck(void)
{
    clear(root);
    root = NULL;
}

void Fsck::add_to_cache(list<sector_loc_t> rd_ord,
                        wr_set_t& rd_set,
                        wr_set_t& wr_set)
{
    node *p = root, *prev = NULL;
    signature_t sig=0;
    list<sector_loc_t>::iterator it;
    sector_loc_t loc;

#ifdef DEBUG
    cout << "rd_set: ";
    for_all_iter(it, rd_ord)
        cout << "("<<*it << "," << rd_set[*it] << ") ";
    cout << endl;
	
    wr_set_t::iterator sit;
#if 0
    cout << "rd_set: ";
    for_all_iter(sit, rd_set)
        cout << "(" << (*sit).first << "," << (*sit).second << ") ";
    cout << endl;
#endif

    cout << "wr_set: ";
    for_all_iter(sit, wr_set)
        cout << "(" << (*sit).first << "," << (*sit).second << ") ";
    cout << endl;
#endif

    assert(rd_ord.size() > 0);

    for_all_iter(it, rd_ord) {

        loc = (*it);

        if(p != NULL) {
            eq_sector_loc eq;
            if(p->leaf_p || !eq(p->loc, loc)) {
                /*if(db.is_valid(rd_set[loc])) */
                xi_panic("fsck not deterministic. " \
                         "read different locations with identical inputs!");
				
                // the block at p->loc read by the cached fsck
                // execution appeared to be valid, therefore we
                // added it to the cache.  however, for this new
                // fsck execution, the block at p->loc is invlaid
                // (i.e., uninitialized, and fsck shouldn't depend
                // on it). 

                // the subtree rooted from p->loc must all be
                // identical.  so, we free all of them except one,
                // and chain it to the previous node.
				
                // will this optimization introduce false
                // negatives?  e.g., fsck will do something stupid
                // if it reads an uninitialized block.
				
            }
            //explode_assert(!p->leaf_p && eq(p->loc, loc),
        }
        else {
            num ++;
            p = new node; // new an internal node
            assert(p);
            p->loc = loc;
            if(prev == NULL)
                root = p;  // this must be root. update root
            else
                prev->next[sig] = p; // update previous pointer
        }
		
        assert(exists(*it, rd_set));
        sig = rd_set[*it];
        prev = p;
        p = prev->next[sig]; // this will insert a NULL if sig
        // isn't in prev->next
    }

    // this must be a new fsck trace
    assert(prev && p == NULL);
    p = new node;
    p->leaf_p = true;
    p->wr_set.swap(wr_set);
    prev->next[sig] = p;

    // TODO: how to merge? hard
}

int Fsck::run_cached(BlockDevDataVec& disks)
{
    node *n = root;
    sector_loc_t loc;
    int dev, dev_index, i;
    long sector;
    char buf[SECTOR_SIZE];
    signature_t sig;

#ifdef DEBUG
    cout << "run_cached start\n";
#endif
    while(n) {

        if(n->leaf_p) {
            // found cached fsck
            // apply n->wr_set;
            disks.write_set(n->wr_set);
            disks.set_dev_data();

            sim_stat.fsck_hits ++;
#ifdef DEBUG
            cout << "hit\n";
#endif
            return 1;
        }

        disks.read(n->loc, sig);
#ifdef DEBUG
        cout << "(" << n->loc << "," << sig << ") ";
#endif

        if(exists(sig, n->next))
            // follow the path
            n = n->next[sig];
        else
            break;
    }
#ifdef DEBUG
    cout << "miss\n";
#endif
    sim_stat.fsck_misses ++;
    return 0;
}

void Fsck::run(BlockDevDataVec& disks)
{
    if(!get_option(fsck, cache)) {
        disks.set_dev_data();
        sim()->recover();
        sim()->umount();
        return;
    }

    if(!run_cached(disks))
        run_real(disks);
	
    //print_cache(cout, root, 0);
    cout << "FSCK CACHE "
         << "hits=" << sim_stat.fsck_hits << " "
         << "misses=" << sim_stat.fsck_misses << " " 
         << "nodes="<<num << "\n";
}

void Fsck::run_real(BlockDevDataVec& disks)
{
    disks.set_dev_data();
	
    sim()->start_trace(WANT_READ);

    sim()->recover();
    sim()->umount();

    RawLogPtr raw_log = sim()->stop_trace();
    LogPtr log(LogMgr::the()->parse(raw_log.get()));

#if 0
    // why do we need this??
    for_all(i, disks)
        reset_time_for_disk(disks[i]);
#endif

    // FIXME: this isn't efficient since we translate the log then
    // immediatelly throw away the translated log.  however there
    // are other things that have higher priorities
    Log::iterator it;
    wr_set_t rd_set, wr_set;
    list<sector_loc_t> rd_ord;
    SectorRec *s;

    // TODO: filter out uninitialized sectors.  it's tricky, since
    // an uninitialized sector in one fsck trace many contain
    // garbage value in another trace.  how should we
    // differentiate them? or should we differentiate them?
    for_all_iter(it, *log) {
        if((*it)->type() == EKM_READ_SECTOR) {
            s = static_cast<SectorRec*>(*it);
            if(!exists(s->loc(), wr_set) && !exists(s->loc(), rd_set)) {
                /*if(db.is_valid(s->s_sig))*/ {
                    // fsck shouldn't depend on unintialized
                    // blocks.  remove them
                    rd_ord.push_back(s->loc());
                    rd_set[s->loc()] = s->sig();
                }
            }
        } else if((*it)->type() == EKM_WRITE_SECTOR) {
            s = static_cast<SectorRec*>(*it);
            // update unconditionally
            wr_set[s->loc()] = s->sig();
            break;
        }
    }

    // TODO: should check the case when we write the same values
    // to disk.  is it sound though?
    add_to_cache(rd_ord, rd_set, wr_set);
}

void Fsck::test(void)
{
    wr_set_t rd_set, wr_set;
    list<sector_loc_t> rd_ord;
    sector_loc_t loc1, loc2, loc3, loc4, loc5;
    loc1 = sector_loc_t(0,1);
    loc2 = sector_loc_t(0,3);
    loc3 = sector_loc_t(0,4);
    loc4 = sector_loc_t(0,8);
    loc5 = sector_loc_t(0,10);

    rd_set[loc1] = 1;
    rd_set[loc2] = 4;
    rd_set[loc3] = 10;
    rd_ord.push_back(loc1);
    rd_ord.push_back(loc2);
    rd_ord.push_back(loc3);
    add_to_cache(rd_ord, rd_set, wr_set);
    print_cache(cout, root, 0);
    rd_set.clear();
    rd_ord.clear();

    rd_set[loc1] = 8;
    rd_ord.push_back(loc1);
    add_to_cache(rd_ord, rd_set, wr_set);
    print_cache(cout, root, 0);
    rd_set.clear();
    rd_ord.clear();

    rd_set[loc1] = 1;
    rd_set[loc2] = 7;
    rd_set[loc3] = 20;
    rd_ord.push_back(loc1);
    rd_ord.push_back(loc2);
    rd_ord.push_back(loc3);
    add_to_cache(rd_ord, rd_set, wr_set);
    print_cache(cout, root, 0);
    rd_set.clear();
    rd_ord.clear();

    rd_set[loc1] = 1;
    rd_set[loc2] = 7;
    rd_set[loc3] = 40;
    rd_set[loc4] = 80;
    rd_ord.push_back(loc1);
    rd_ord.push_back(loc2);
    rd_ord.push_back(loc3);
    rd_ord.push_back(loc4);
    add_to_cache(rd_ord, rd_set, wr_set);
    print_cache(cout, root, 0);
    rd_set.clear();
    rd_ord.clear();

    rd_set[loc1] = 1;
    rd_set[loc2] = 4;
    rd_set[loc3] = 40;
    rd_set[loc4] = 80;
    rd_ord.push_back(loc1);
    rd_ord.push_back(loc2);
    rd_ord.push_back(loc3);
    rd_ord.push_back(loc4);
    add_to_cache(rd_ord, rd_set, wr_set);
    print_cache(cout, root, 0);
    rd_set.clear();
    rd_ord.clear();

    // more test here

    exit(1);
}

std::ostream& Fsck::print_cache(std::ostream& o, node* n, int level)
{
    if(!n) return o;

    int i;
    for(i = 0; i < level; i++)
        o << "    ";
    o << n->loc << endl;

    Sgi::hash_map<signature_t, node*>::iterator it;
    for_all_iter(it, n->next) {
        for(i = 0; i < level; i++)
            o << "    ";
        o << (*it).first << endl;
        print_cache(o, (*it).second, level+1);
    }
	
    return o;
}
