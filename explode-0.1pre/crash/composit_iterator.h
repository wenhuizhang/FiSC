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


#ifndef __COMPOSITE_ITERATOR_H
#define __COMPOSITE_ITERATOR_H

#include <stack>

// STL style iterators that support pre-order and post-order
// traversal of a composite type.  A composite type is defined as
// a type T which contains is a collection of items of the type T*.  
// e.g.
//
//      class T { 
//          std::list<T*> children; 
//      };
//
// In other words, T is a recursive, tree-like type.
//
// The implementaiton of these Iterators assume that the
// collection of items is named "children"
//
// The iterators take one template argument, _Container, which is
// the type for the member variable children.


template <typename _Container, 
          typename _Begin, 
          typename _End>
struct _Composite_iterator_base
{
    typedef _Composite_iterator_base<_Container, _Begin, _End> _Self;

    typedef ptrdiff_t                          difference_type;
    typedef std::forward_iterator_tag          iterator_category;
    typedef typename _Container::value_type    value_type;
    typedef typename _Container::iterator      child_iterator;
    typedef value_type*                        pointer;
    typedef value_type&                        reference;

    _Composite_iterator_base(value_type __root, bool done,
                             _Begin __b, _End __e)
        : _M_root(__root), _M_done(done), _begin(__b), _end(__e)
    { _M_root_ptr = &_M_root; }

    _Composite_iterator_base() : _M_root(NULL)
    { _M_root_ptr = &_M_root; }
    
    _Composite_iterator_base(const _Self &__x)
    {
        _M_root = __x._M_root;
        _M_root_ptr = &_M_root;
        _M_done = __x._M_done;
        _M_it_stack = __x._M_it_stack;
        _M_end_stack = __x._M_end_stack;
        _begin = __x._begin;
        _end = __x._end;
    }

    reference
    operator*() const
    {
        return *(_M_it_stack.top());
    }

    pointer
    operator->() const
    { return &(operator*());}

    
public:

    bool
    operator==(const _Self& __x) const
    { 
        return _M_root == __x._M_root
            && _M_done == __x._M_done
            && (_M_done || _M_it_stack.top() == __x._M_it_stack.top());
    }

    bool
    operator!=(const _Self& __x) const
    { return !operator==(__x);}

    
protected:
    value_type _M_root;
    pointer _M_root_ptr;

    std::stack<child_iterator> _M_it_stack;
    std::stack<child_iterator> _M_end_stack;
    bool _M_done;

    _Begin _begin;
    _End _end;
};

// default functor for _Begin
template <typename _Container>
struct begin {
    typedef typename _Container::value_type    value_type;
    typedef typename _Container::iterator      child_iterator;

    child_iterator operator()(value_type x)
    { return x->children.begin(); }
};

// default functor for _End
template <typename _Container>
struct end {
    typedef typename _Container::value_type    value_type;
    typedef typename _Container::iterator      child_iterator;

    child_iterator operator()(value_type x)
    { return x->children.end(); }
};


template <typename _Container, 
          typename _Begin = begin<_Container>, 
          typename _End = end<_Container> >
struct _Composite_pre_iterator
    : public _Composite_iterator_base<_Container, _Begin, _End>
{
    typedef _Composite_pre_iterator<_Container, _Begin, _End> _Self;
    typedef _Composite_iterator_base<_Container, _Begin, _End> _Parent;

    typedef ptrdiff_t                          difference_type;
    typedef std::forward_iterator_tag          iterator_category;
    typedef typename _Container::value_type    value_type;
    typedef typename _Container::iterator      child_iterator;
    typedef value_type*                        pointer;
    typedef value_type&                        reference;

    _Composite_pre_iterator(value_type __root, bool done, _Begin __b, _End __e)
        : _Parent(__root, done, __b, __e), _M_first(true) {}

    _Composite_pre_iterator(value_type __root, bool done)
        : _Parent(__root, done, _Begin(), _End()), _M_first(true) {}

    _Composite_pre_iterator(): _M_first(true) {}
    
    _Composite_pre_iterator(const _Self &__x)
        :_Parent(__x)
    {
        _M_first = __x._M_first;
    }

    reference
    operator*() const
    {
        if(_M_first)
        return *_Parent::_M_root_ptr;
        return _Parent::operator*();
    }

    pointer
    operator->() const
    { return &(operator*());}

    _Self&
    operator++()
    {
        if(_M_first) {
            _M_first = false;
            _Parent::_M_it_stack.push(_begin(_Parent::_M_root));
            _Parent::_M_end_stack.push(_end(_Parent::_M_root));
        } else {
            child_iterator __it;
            __it = _Parent::_M_it_stack.top();
            _Parent::_M_it_stack.push(_begin(*__it));
            _Parent::_M_end_stack.push(_end(*__it));
        }
        _M_next();
        return *this;
    }

    _Self
    operator++(int)
    {
        _Self __tmp = *this;
        ++(*this);
        return __tmp;
    }

protected:

    void _M_next(void) 
    {
        while(_Parent::_M_it_stack.size() > 0
              && _Parent::_M_it_stack.top() == _Parent::_M_end_stack.top()) {
                  _Parent::_M_it_stack.pop();
                  _Parent::_M_end_stack.pop();
                  if(_Parent::_M_it_stack.size() > 0)
                  ++ _Parent::_M_it_stack.top();
                  else
                  _Parent::_M_done = true;
              }
    }

    bool _M_first;
};


template <typename _Container, 
          typename _Begin = begin<_Container>, 
          typename _End = end<_Container> >
struct _Composite_post_iterator
    : public _Composite_iterator_base<_Container, _Begin, _End>
{
    typedef _Composite_post_iterator<_Container, _Begin, _End> _Self;
    typedef _Composite_iterator_base<_Container, _Begin, _End> _Parent;

    typedef ptrdiff_t                          difference_type;
    typedef std::forward_iterator_tag          iterator_category;
    typedef typename _Container::value_type    value_type;
    typedef typename _Container::iterator      child_iterator;
    typedef value_type*                        pointer;
    typedef value_type&                        reference;

    _Composite_post_iterator(value_type __root, bool done, 
                             _Begin __b, _End __e)
        : _Parent(__root, done, __b, __e)
    {
        _M_push_left_most(__root);
        _M_last = (_Parent::_M_it_stack.size() == 0);
        
    }

    _Composite_post_iterator(value_type __root, bool done)
        : _Parent(__root, done, _Begin(), _End())
    {
        _M_push_left_most(__root);
        _M_last = (_Parent::_M_it_stack.size() == 0);
        
    }

    _Composite_post_iterator(): _M_last(false) {}
    
    _Composite_post_iterator(const _Self &__x)
        :_Parent(__x)
    {
        _M_last = __x._M_last;
    }

    reference
    operator*() const
    {
        if(_M_last)
            return *_Parent::_M_root_ptr;
        return _Parent::operator*();
    }

    pointer
    operator->() const
    { return &(operator*());}

    _Self&
    operator++()
    {
        if(_M_last) {
            _M_last = false;
            _Parent::_M_done = true;
            return *this;
        }

        ++ _Parent::_M_it_stack.top();
        
        if(_Parent::_M_it_stack.top() == _Parent::_M_end_stack.top()) {
            _Parent::_M_it_stack.pop();
            _Parent::_M_end_stack.pop();
            
            _M_last = (_Parent::_M_it_stack.size() == 0);
            return *this;
        }

        _M_push_left_most(*_Parent::_M_it_stack.top());

        return *this;
    }

    _Self
    operator++(int)
    {
        _Self __tmp = *this;
        ++(*this);
        return __tmp;
    }

protected:
    
    void _M_push_left_most(const value_type& __tmp)
    {
        value_type __x = __tmp;
        while(_begin(__x) != _end(__x)) {
            _Parent::_M_it_stack.push(_begin(__x));
            _Parent::_M_end_stack.push(_end(__x));
            __x = *(_begin(__x));
        }
    }

    bool _M_last;
};

#ifdef TEST

// an exampble

#include <iostream>
#include <list>
using namespace std;

class Test {
public:
    int id;

    list<Test*> children;

    typedef list<Test*> container_type;
    typedef _Composite_pre_iterator<container_type> iterator;

    typedef _Composite_post_iterator<container_type> post_iterator;

    Test(int i): id(i) {}

    iterator begin()
    { return  iterator(this, false); }
    
    iterator end()
    { return iterator(this, true); }


    post_iterator pbegin()
    { return  post_iterator(this, false); }
    
    post_iterator pend()
    { return post_iterator(this, true); }
};

Test a(0);
Test::iterator it;
Test::post_iterator pit;

void test(void)
{
    cout << "======"<<endl;
    for(it=a.begin(); it!=a.end(); ++it)
        cout<< (*it)->id << endl;

    cout << "======"<<endl;
    for(pit=a.pbegin(); pit!=a.pend(); ++pit)
        cout<< (*pit)->id << endl;
}

int main(void)
{
    test();    
    a.children.push_back(new Test(1));
    test();
    a.children.front()->children.push_back(new Test(2));
    test();
    a.children.front()->children.push_back(new Test(3));
    test();
    a.children.push_back(new Test(4));
    test();
    a.children.push_back(new Test(5));
    test();
    a.children.back()->children.push_back(new Test(6));
    test();
    a.children.back()->children.push_back(new Test(7));
    test();
    a.children.back()->children.push_back(new Test(8));
    test();
     
}
#endif

#endif
