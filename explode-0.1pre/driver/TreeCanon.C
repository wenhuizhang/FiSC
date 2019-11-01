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
  Canonicalizes a tree.  

  TODO: add code to handle hard links.  Re-label files with unique
  labels first, then canonicalizes the file system.  Then replaces
  the first appearance of a unique file label with the file's
  content signature, and other apperances with this file's
  depth-first traversal id.
 */
#include "TreeCanon.h"
#include <assert.h>
#include <algorithm>
#include <list>
#include <iostream>
#include <iterator>
#include <string>

using namespace std;

//#define DEBUG_TREECANON

// '\0' shows up as ^@
void print_char_array(ostream& o, const char* buf, int len)
{
	copy(buf, buf+len, ostream_iterator<char>(o, ""));
}

void TreeCanon::canonicalize(node_t *n)
{
	assert(n);
	for(unsigned i = 0; i < n->children.size(); i++)
		canon_form_inter(n->children[i]);

	sort(n->children.begin(), n->children.end(), ltnode());
#ifdef DEBUG_TREECANON
	for(int i = 0; i < n->children.size(); i++) {
		cout << "child " << i 
		     << " has canonical form ";
		print_char_array(cout, n->children[i]->form, n->children[i]->form_len);
		cout << endl;
	}
#endif
}

void TreeCanon::canon_form_inter(node_t *n)
{
	assert(n);
	canonicalize(n);

	list<node_t *> q;
	q.push_back(n);
	q.push_back(NULL);
	char canon_form[MAX_FORM], *buf = canon_form;
	while(!q.empty()) {
		node_t *cur = q.front();
		q.pop_front();
		if(!cur) {
			assert(buf < canon_form + MAX_FORM);
			*(buf++) = '$';
			continue;
		}
		assert(buf + sizeof(cur->label) 
		       < canon_form + MAX_FORM);
		memcpy(buf, cur->label, sizeof(cur->label));
		buf += sizeof(cur->label);
		for(unsigned i = 0; i < cur->children.size(); i++) {
			q.push_back(cur->children[i]);
		}
		if(cur->children.size())
			q.push_back(NULL);
	}
	assert(buf < canon_form + MAX_FORM);
	*(buf++) = '#';
	n->form_len = buf - canon_form;
	n->form = (char*)malloc(n->form_len);
	assert(n->form);
	memcpy(n->form, canon_form, n->form_len);
#ifdef DEBUG_TREECANON
	cout << "form is ";
	print_char_array(cout, n->form, n->form_len);
	cout << endl;
#endif	
}

TreeCanon::node_t* TreeCanon::add(node_t *parent, char* label)
{
	node_t* n = new node_t;
	n->form = NULL;
	n->form_len = 0;
	memcpy(n->label, label, sizeof(n->label));
#ifdef DEBUG_TREECANON
	cout << "adding node with label ";
	print_char_array(cout, n->label, sizeof(n->label));
	cout << endl;
#endif	
	if(parent)
		parent->children.push_back(n);
	else
		root = n;
	return n;
}

void TreeCanon::node_canon_form(node_t* n, char *&form, int&len)
{
	canon_form_inter(n);
	form = n->form;
	len = n->form_len;
}

void TreeCanon::canon_form(char *&form, int &len)
{
	canon_form_inter(root);
	form = root->form;
	len = root->form_len;
}

void TreeCanon::clear(node_t *n)
{
	for(unsigned i = 0; i < n->children.size(); i++) {
		clear(n->children[i]);
	}
	if(n->form) free(n->form);
	delete n;
}

TreeCanon::~TreeCanon()
{
	clear(root);
}

#if 0
//#ifdef DEBUG_TREECANON
int main(void)
{
	char label[MAX_LABEL];
	memset(label, 0, sizeof(label));

	TreeCanon tc;
	strcpy(label, "D*");
	TreeCanon::node_t *r = tc.add(NULL, label);

	TreeCanon::node_t *d1 = tc.add(r, label);
	TreeCanon::node_t *d2 = tc.add(r, label);

	strcpy(label, "F1*");
	tc.add(d1, label);
	strcpy(label, "F2*");
	tc.add(d1, label);

	strcpy(label, "F3*");
	tc.add(d2, label);

	char *form;
	int len;
	tc.canon_form(form, len);

	cout << string(form, len) << endl;

	return 1;
}
#endif

