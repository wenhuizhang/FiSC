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


#ifndef __UNMARSHAL_H
#define __UNMARSHAL_H

#if 0
#    include <iostream>
#    define DEBUG_UNMARSHAL(x) std::cout<<(x)<<std::endl
#else
#    define DEBUG_UNMARSHAL(x)
#endif

// unmarshal into scalar type T
template<class T>
inline void unmarshal(const char* &off, T& t)
{
    DEBUG_UNMARSHAL("unmarshal(const char* &off, T& t)");

    memcpy(&t, off, sizeof t);

    off += sizeof t;
}

// T is a scalar type.  allocated T*, then unmarshal into
// allocated buffer
template<class T>
inline void unmarshal(const char* &off, T*& t)
{

    DEBUG_UNMARSHAL("unmarshal(const char* &off, T*& t)");

    t = new T;

    memcpy(t, off, sizeof *t);
    off += sizeof *t;
}

// unmarshal a string of type char*.  Treat it like a scalar type.
// First unmarshal its length, then its content.
template<>
inline void unmarshal<char>(const char* &off, char*& str)
{
    DEBUG_UNMARSHAL("unmarshal<char>(const char* &off, char*& str)");

    // unmarshal length
    int len;
    unmarshal(off, len);

    if(len == 0) {
        str = NULL;
        return;
    }

    // unmarshal string content
    str = new char[len+1];
    memcpy(str, off, len);
    
    // null terminate the string
    str[len] = '\0';

    off += len;
}

// unmarshal an array of known size @size.  This function is
// useful when we unmarshal the size and the contents of the array
// separately
template<class T, class N>
inline void unmarshal(const char* &off, T*& data, N size)
{
    DEBUG_UNMARSHAL("unmarshal(const char* &off, T*& data, N size)");

    if(size == (N)0) {
        data = NULL;
        return;
    }

    data = new T[size];
    memcpy(data, off, size * sizeof *data);
    
    off += size * sizeof *data;
}

// unmarshal an array.  First unmarshal its size, then its content
template<class T>
inline void unmarshal_array(const char* &off, T*& data, int& size)
{
    DEBUG_UNMARSHAL("unmarshal_array(const char* &off, T*& data, N& size)");

    unmarshal(off, size);
    unmarshal(off, data, size);
}

// unmarshal an array.  Ignore the size
template<class T>
inline void unmarshal_array(const char* &off, T*& data)
{
    DEBUG_UNMARSHAL("unmarshal_array(const char* &off, T*& data)");

    int size;
    unmarshal_array(off, data, size);
}


#endif //#ifndef __UNMARSHAL_H
