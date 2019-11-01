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


#define MAX_LABEL (5)
#define MAX_FORM (4096)

#include <vector>

class TreeCanon {
 public:
	typedef struct node_s {
		char *form; // cannonicalized form for this node
		int form_len;
		char label[MAX_LABEL]; // label for this node
		std::vector<struct node_s*> children;
	} node_t;

	struct ltnode {
		bool operator()(const node_t* n1, const node_t* n2) {
			return n1->form_len < n2->form_len ||
				(n1->form_len == n2->form_len
				 && memcmp(n1->form, n2->form, n1->form_len) < 0);
		}
	};

 private:
	node_t *root;
	void canonicalize(node_t*);
	void canon_form_inter(node_t *n);
	void clear(node_t *n);
 public:
	~TreeCanon();
	node_t *add(node_t* parent, char* label);
	void canon_form (char *&buf, int &len);
	void node_canon_form(node_t* n, char *&buf, int&len);
};
