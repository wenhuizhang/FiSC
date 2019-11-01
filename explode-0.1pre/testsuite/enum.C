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


/*
  test if we can enumerate all choices.
*/

#include "mcl.h"

const int MAX = 5;

class Test: public TestHarness {

    int r[20], n, num;

public:
    Test() {
        n = 0;
        num = sizeof r/sizeof r[0];
        for(int i=0; i<num; i++)
            r[i] = MAX;
    }

    void init_state(void) {
	r[0] = MAX-1;
    }

    void run_one_transition(void) {
	int i;
	
	for(i = 1; i < num; i ++) 
            r[i] = xe_random(r[i-1]+1);

	printf("%5dth: ", n++);
	for(i = 1; i < num; i ++) 
            printf("%d", r[i]);
	
	printf("\n");
    }

    signature_t get_sig (void) {
	return 0;
    }
};

int main(int argc, char *argv[])
{
    xe_process_cmd_args(argc, argv);
    xe_check(new Test);
}
