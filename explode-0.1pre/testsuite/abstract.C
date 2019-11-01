/*
 *
 * Copyright (C) 2008 Can Sar (csar@cs.stanford.edu)
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


#include "driver/AbstFS.h"
#include "options.h"

using namespace std;

int main(int argc, char **argv) {
	AbstFS fs1, fs2;
	if(argc != 2) {
		cout << "usage: " << argv[0] << " <path-to-abstract>" << endl;
		return 0;
	}
	
	int len;
	char *buffer;
	set_option(mcl, verbose, 10);

	fs1.abstract(argv[1]);
	
	printf("ABSTRACTED:\n");
	fs1.print();
	
	fs1.marshal(buffer, len);
	fs2.unmarshal(buffer, len);

	printf("UNMARSHALLED:\n");
	fs2.print();
	
	return 0;
}
