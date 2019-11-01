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


#include "random.h"

#include "assert.h"

/* Use kernel-specific allocators */
#include <malloc.h>
#include <string.h>
#define ALLOC malloc
#define FREE free
#define ASSERT assert

choice_vec_t* choice_alloc(int type) 
{
	switch(type) {
	case UNIX_RAND:
		break;
	case ENUM_RAND:
		break;
	case REPLAY_RAND:
		break;
	}
}

void choice_free(choice_vec_t *c)
{
	if(c->choices)
		FREE(c->choices);
	FREE(c);
}

void choice_set(choice_vec_t *c, choice_t *choices, int num)
{
	ASSERT(!c->choices);
	c->num = num;
	c->choices = (choice_t*)ALLOC(sizeof c->choices[0]*num);
	memcpy(c->choices, choices, num);
}

int choose_fail(choice_vec_t* c)
{
	return c->choose(c, 2);
}

void enum_reset(choice_vec_t* c) 
{
	c->cur = -1;
	c->num = 0;
}

static void check_capacity(choice_vec_t *c, int capacity_needed)
{
	choice_t *new_choices, *new_ceils;
	if(capacity_needed < c->capacity)
		return;

	/* make sure we have enough space */
	c->capacity = 2 * capacity_needed + 10;

	/* copy choices over */
	new_choices = (choice_t*)ALLOC(sizeof c->choices[0] * c->capacity);
	memcpy(new_choices, c->choices, sizeof c->choices[0] * c->num);
	c->choices = new_choices;

	/* copy ceils over if necessary */
	if(c->ceils) {
		new_ceils = (choice_t*)ALLOC(sizeof c->ceils[0]*c->capacity);
		memcpy(new_choices, c->choices, sizeof c->ceils[0]*c->num);
		c->ceils = new_ceils;
	}
}

int enum_choose(choice_vec_t* c, int ceil) 
{
	ASSERT(ceil>0 && ceil<= MAX_CEIL);

	/* If ceil == 1, no choice, immediately return. 
	   Note that we need to skip this case, too, in replay */
	if(ceil == 1) return 0;

	c->cur ++;

	/* If this is an old choice point, return the choice value */
	if(c->cur < c->num)
		return c->choices[c->cur];

	/* Otherwise, push the choice point to the top of the choices stack */
	c->num++;
	check_capacity(c, c->num);
	c->ceils[c->num] = ceil;
	c->choices[c->num] = 0;
	c->num ++;
}

void enum_repeat(choice_vec_t* c)
{
}
