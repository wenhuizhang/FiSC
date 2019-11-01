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


#ifndef CMC_HASH_MAP_COMPAT_H
#define CMC_HASH_MAP_COMPAT_H

/*
 * Compatibility header to correctly access the STL hash_map
 * depending on what version of libstdc++ we are using.
 *
 * This code was taken from the libstdc++ FAQ at http://gcc.gnu.org
 */

#ifdef __GNUC__
  #if __GNUC__ < 3
    #include <hash_map.h>
    namespace Sgi { using ::hash_map; using ::hash; }; // inherit globals
  #else
    #include <ext/hash_map>
    #if __GNUC__ == 3 && __GNUC_MINOR__ == 0
      namespace Sgi = std;               // GCC 3.0
    #else
      namespace Sgi = ::__gnu_cxx;       // GCC 3.1 and later
    #endif
  #endif
#else      // ...  there are other compilers, right?
  namespace Sgi = std;
#endif


#endif // CMC_HASH_MAP_COMPAT_H
