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


#ifndef __MCL_RANDOM_H
#define __MCL_RANDOM_H

#include "mcl_types.h"

typedef struct choice_vec_t {
	choice_t *choices; /* Stores the choice taken at each choice point */
	choice_t *ceils; /* Stroes the ceiling of each choice point. 
			    Only used for ENUM_RAND */
	int num; /* Number of choice_t used */
	int capacity; /* Number of choice_t allocated */
	int cur; /* Current choice */
	
	void (*reset)(struct choice_vec_t* c);
	int (*choose)(struct choice_vec_t* c, int ceil);
	int (*repeat)(struct choice_vec_t* c);

} choice_vec_t;

choice_vec_t* choice_alloc(int type);
void choice_set(choice_vec_t *c, choice_t *choices, int num);
void choice_free(choice_vec_t *c);
int choose_fail(choice_vec_t* c);

enum {UNIX_RAND, ENUM_RAND, REPLAY_RAND};

#endif
