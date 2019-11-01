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


#ifndef __OUTPUT_H
#define __OUTPUT_H

#include <iostream>

#include "options.h"

#define _vn_one_line_out(lvl) \
	if(get_option(mcl, verbose)<=lvl){} else std::cout<<"("<<lvl<<"):"

#define status_out _vn_one_line_out(0)
// verbose
#define v_out _vn_one_line_out(1)
// very verbose
#define vv_out _vn_one_line_out(2)
// very very verbose
#define vvv_out _vn_one_line_out(3)

#define vn_out(lvl) _vn_one_line_out(lvl)

/* this is way too hackish
#define _lvl_multi_line_out_begin(lvl) \
	if(get_option(mcl, verbose)<=lvl){} else {std::cout
#define out_end } 

#define status_out_begin _lvl_multi_line_out_begin(0)
#define v_out_begin _lvl_multi_line_out_begin(1)
#define vv_out_begin _lvl_multi_line_out_begin(2)
#define vvv_out_begin _lvl_multi_line_out_begin(3) */

#define output_status (get_option(mcl, verbose) > 0)
#define output_v (get_option(mcl, verbose) > 1)
#define output_vv (get_option(mcl, verbose) > 2)
#define output_vvv (get_option(mcl, verbose) > 3)


#endif
