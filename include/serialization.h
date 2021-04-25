// $Id: serialization.h 152 2019-08-21 20:23:38Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*
 * serialization.h
 *
 *  Created on: Jan 1, 2013
 *      Author: n7dr
 */

#ifndef SERIALIZATION_H
#define SERIALIZATION_H

// boost uses oodles of headers in its implementation of serialization
// we put them all here so we shouldn't have to worry about missing any

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/array.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <boost/serialization/vector.hpp>

#if 0
UNORDERED_MAP DOES NOT WORK EVEN WITH THE FOLLOWING

// http://stackoverflow.com/questions/4287299/boostserialization-of-boostunordered-map
// https://code.google.com/p/ntest/source/browse/unordered_map_serialization.h

#ifndef BOOST_SERIALIZATION_UNORDEREDMAP_HPP
#define BOOST_SERIALIZATION_UNORDEREDMAP_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// serialization/map.hpp:
// serialization for stl map templates

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#include <boost/unordered_map.hpp>

#include <boost/config.hpp>

#include <boost/serialization/utility.hpp>
#include <boost/serialization/collections_save_imp.hpp>
#include <boost/serialization/collections_load_imp.hpp>
#include <boost/serialization/split_free.hpp>

namespace boost {
namespace serialization {

template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
inline void save(
Archive & ar,
const boost::unordered_map<Key, Type, Hash, Compare, Allocator> &t,
const unsigned int /* file_version */
){
boost::serialization::stl::save_collection<
Archive,
boost::unordered_map<Key, Type, Hash, Compare, Allocator>
>(ar, t);
}

template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
inline void load(
Archive & ar,
boost::unordered_map<Key, Type, Hash, Compare, Allocator> &t,
const unsigned int /* file_version */
){
boost::serialization::stl::load_collection<
Archive,
boost::unordered_map<Key, Type, Hash, Compare, Allocator>,
boost::serialization::stl::archive_input_map<
Archive, boost::unordered_map<Key, Type, Hash, Compare, Allocator> >,
boost::serialization::stl::no_reserve_imp<boost::unordered_map<
Key, Type, Hash, Compare, Allocator
>
>
>(ar, t);
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
inline void serialize(
Archive & ar,
boost::unordered_map<Key, Type, Hash, Compare, Allocator> &t,
const unsigned int file_version
){
boost::serialization::split_free(ar, t, file_version);
}

// multimap
template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
inline void save(
Archive & ar,
const boost::unordered_multimap<Key, Type, Hash, Compare, Allocator> &t,
const unsigned int /* file_version */
){
boost::serialization::stl::save_collection<
Archive,
boost::unordered_multimap<Key, Type, Hash, Compare, Allocator>
>(ar, t);
}

template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
inline void load(
Archive & ar,
boost::unordered_multimap<Key, Type, Hash, Compare, Allocator> &t,
const unsigned int /* file_version */
){
boost::serialization::stl::load_collection<
Archive,
boost::unordered_multimap<Key, Type, Hash, Compare, Allocator>,
boost::serialization::stl::archive_input_multimap<
Archive, boost::unordered_multimap<Key, Type, Hash, Compare, Allocator>
>,
boost::serialization::stl::no_reserve_imp<
boost::unordered_multimap<Key, Type, Hash, Compare, Allocator>
>
>(ar, t);
}

// split non-intrusive serialization function member into separate
// non intrusive save/load member functions
template<class Archive, class Type, class Key, class Hash, class Compare, class Allocator >
inline void serialize(
Archive & ar,
boost::unordered_multimap<Key, Type, Hash, Compare, Allocator> &t,
const unsigned int file_version
){
boost::serialization::split_free(ar, t, file_version);
}

} // serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_UNORDEREDMAP_HPP

#endif    // 0





// despite documentation explicitly claiming that boost serialization works with all STL containers
// "The serialization library contains code for serialization of all STL classes", boost does not in fact support STL arrays.
// The following allows boost serialization to work transparently with STL arrays, see:
//   http://stackoverflow.com/questions/11585273/using-boost-serialization-with-stl-containers-as-template-parameters

#if 0

#include <array>

namespace boost
{ namespace serialization
  {

template<class Archive, class T, size_t N>
    void serialize(Archive & ar, std::array<T,N> & a, const unsigned int version)
    { ar & /* boost::serialization:: */make_array(a.data(), a.size());
    }

  } // namespace serialization
} // namespace boost

#endif

// neither does it work for unordered maps

//#include <unordered_map>

//namespace boost
//{ namespace serialization
//  {

//template<class Archive, class K, class V>
//    void serialize(Archive & ar, std::unordered_map<K,V> & m, const unsigned int version)
//    { ar & /* boost::serialization:: */make_array(m.data(), m.size());
//    }

//  } // namespace serialization
//} // namespace boost




// the following is claimed to work (from the boost-users reflector, in response to
// a request from me). Of course, it doesn't.

/* It is less intimidating than it looks. You can simply copy the
boost/serialization/set.hpp header and replace the type.

N7DR: NO YOU CAN'T ... IT DOESN'T WORK

In just five minutes I created the attached example that uses std:tr1::unordered_set.

N7DR: Great... except it DOESN'T WORK.

People keep saying how simple it is; but if it were so simple, it would surely already be part of boost.
*/

#if 0
#include <boost/tr1/unordered_set.hpp>
#include <boost/serialization/collections_save_imp.hpp>
#include <boost/serialization/collections_load_imp.hpp>
#include <boost/serialization/split_free.hpp>

namespace boost {
namespace serialization {

template<class Archive, class Key, typename Hasher, class Compare, class Allocator >
inline void save(Archive & ar,
                 const std::tr1::unordered_set<Key, Hasher, Compare, Allocator> &t,
                 const unsigned int /* file_version */)
{
    boost::serialization::stl::save_collection<
        Archive,
        std::tr1::unordered_set<Key, Hasher, Compare, Allocator>
        >(ar, t);
}

template<class Archive, class Key, typename Hasher, class Compare, class Allocator >
inline void load(Archive & ar,
                 std::tr1::unordered_set<Key, Hasher, Compare, Allocator> &t,
                 const unsigned int /* file_version */)
{
    boost::serialization::stl::load_collection<
        Archive,
        std::tr1::unordered_set<Key, Hasher, Compare, Allocator>,
        boost::serialization::stl::archive_input_set<
            Archive, std::tr1::unordered_set<Key, Hasher, Compare, Allocator>
        >,
        boost::serialization::stl::no_reserve_imp<
            std::tr1::unordered_set<Key, Hasher, Compare, Allocator>
        >
    >(ar, t);
}

template<typename Archive, typename Key, typename Hasher, typename Compare, typename Allocator >
inline void serialize(Archive & ar,
                      std::tr1::unordered_set<Key, Hasher, Compare, Allocator> & t,
                      const unsigned int file_version)
{
    boost::serialization::split_free(ar, t, file_version);
}

} // namespace serialization
} // namespace boost
#endif


/// THIS works -- temporary no compile

#if 0
// http://stackoverflow.com/questions/16314996/compile-error-on-serializing-boostunordered-set

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace boost
{ namespace serialization
  { template<class Archive, typename T, typename H, typename P, typename A>
    void save(Archive &ar, const std::unordered_set<T,H,P,A> &s, const unsigned int)
    { std::vector<T> vec(s.begin(),s.end());

      ar << vec;
    }

    template<class Archive, typename T, typename H, typename P, typename A>
    void load(Archive &ar, std::unordered_set<T,H,P,A> &s, const unsigned int)
    { std::vector<T> vec;

      ar >> vec;
      std::copy(vec.begin(),vec.end(), std::inserter(s,s.begin()));
    }

    template<class Archive, typename T, typename H, typename P, typename A>
    void serialize(Archive &ar, std::unordered_set<T,H,P,A> &s, const unsigned int version)
    { boost::serialization::split_free(ar,s,version);
    }

    template<class Archive, typename K, typename T, typename H, typename P, typename A>
    void save(Archive &ar, const std::unordered_map<K, T,H,P,A> &s, const unsigned int)
    { std::vector<T> vec(s.begin(),s.end());

      ar << vec;
    }

    template<class Archive, typename K, typename T, typename H, typename P, typename A>
    void load(Archive &ar, std::unordered_map<K, T,H,P,A> &s, const unsigned int)
    { std::vector<T> vec;

      ar >> vec;
      std::copy(vec.begin(),vec.end(), std::inserter(s,s.begin()));
    }

template<class Archive, typename K, typename T, typename H, typename P, typename A>
void serialize(Archive &ar,
               std::unordered_map<K, T,H,P,A> &s, const unsigned int version) {
    boost::serialization::split_free(ar,s,version);
}


}
}

#endif


#endif /* SERIALIZATION_H */
