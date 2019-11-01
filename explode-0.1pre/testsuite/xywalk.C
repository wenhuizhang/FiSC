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


/* A simple test case simulating walking in a 2D plane.  Each step, we can
 *  either walk horizontally to the left (increment x coordinate by 1) or
 *  vertically to the top (increment y coordinate by 1).  When we go off
 *  bound (MAX), we wrap around.  Traps exist at coordinate (x, y) where x
 *  + y = 2 * MAX - 2.  Once we hit a trap, we will store a trace by
 *  calling eXplode-provided error(...) method.
 *
 * To replay the error trace, simple run
 *
 *   ./xywalk -r trace/0/trace
 *
 * This implementation does stateless checking: each state is represented
 * by the sequence of choices (xe_random() returns) that created the
 * state.
 *
 */


#include "mcl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

using namespace std;

const int MAX = 5;

class Test: public TestHarness {
    int x, y;
    int ntrans;
    
public:
    Test() {
        ntrans = 0;
    }


    virtual void init_state(void)
    {
        x = 0;
        y = 0;
    }

    virtual void run_one_transition(void) {
	int op = xe_random(2);

	// run transition here
	printf ("(%d, %d)", x, y);
		
	switch (op) {
	case 0:
		x += 1;
		x %= MAX;
		break;
	case 1:
		y += 1;
		y %= MAX;
		break;
	}
	if (x + y == 2 * MAX - 2) {
		printf (" -> bug\n");
		fflush(NULL);
		error("bug", "bug");
	}
	ntrans ++;
	printf (" -> (%d, %d)\n", x, y);
	return;
    }

    virtual void before_transitions(void) {
        printf("new execution\n");
    }

    void exit(void) {
	int expected =  MAX*MAX*2;
	if (ntrans == expected)
            printf ("total trans %d is expected.\n", ntrans);
	else
            printf ("Something wrong with the modelchecker(actual transitions %d != expectd %d!!!\n", ntrans, expected);
    }
    
    signature_t get_sig (void)
    {
	//cout << "sig = " << (x<<16)+y << endl;
	return (x<<16) + y;
    }
};

int main(int argc, char *argv[])
{
    xe_process_cmd_args(argc, argv);
    xe_check(new Test);
}
