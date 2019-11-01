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


#ifndef __CLUSTERBUILDER_H
#define __CLUSTERBUILDER_H

#include <map>
#include <set>
#include <ext/hash_set>

#include "hash_map_compat.h"
#include "Permuter.h"

typedef Sgi::hash_set<sector_loc_t, hash_sector_loc, eq_sector_loc> cluster_t;
/* maps from a syscall lsn to the list of sectors it touches */
typedef std::map<int, cluster_t*> clusters_t;
typedef std::set<sector_loc_t, lt_sector_loc> ordered_sectors_t;

class ClusterBuilder
{
 public:
	void build_syscall_clusters(clusters_t& clusters);
	void build(clusters_t& clusters);
};

#endif
