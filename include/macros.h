// $Id: macros.h 204 2022-04-10 14:54:55Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef MACROS_H
#define MACROS_H

#undef NEW_RAW_COMMAND  // experiment to try to improve the reliability of the awful K3 interface

/*! \file   macros.h

    Macros and templates for drlog.
*/

#include "serialization.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <queue>
#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

#include <cmath>

// convenient definitions for use with chrono functions
using centiseconds = std::chrono::duration<long, std::centi>;
using deciseconds = std::chrono::duration<long, std::deci>;

// define enum classes used in more than one file

// used for whether to display time in alert()
enum class SHOW_TIME { SHOW,
                       NO_SHOW
                     };

/// Syntactic sugar for read/write access
// See: "C++ Move Semantics", p. 81; 5.1.3 Using Move Semantics to Solve the Dilemma

#if (!defined(READ_AND_WRITE))

#define READ_AND_WRITE_RET(y)                                       \
/*! Read access to _##y */                                      \
  [[nodiscard]] inline const decltype(_##y)& y(void) const& { return _##y; }    \
  [[nodiscard]] inline decltype(_##y) y(void) && { return std::move(_##y); }    \
\
/*! Write access to _##y */                                     \
  inline auto y(const decltype(_##y)& n) -> decltype(*this)& { _##y = n; return (*this); }

#define READ_AND_WRITE(y)                                       \
/*! Read access to _##y */                                      \
  [[nodiscard]] inline const decltype(_##y)& y(void) const& { return _##y; }    \
  [[nodiscard]] inline decltype(_##y) y(void) && { return std::move(_##y); }    \
\
/*! Write access to _##y */                                     \
  inline void y(const decltype(_##y)& n) { _##y = n; }

#endif    // !READ_AND_WRITE

/// Syntactic sugar for read/write access with thread locking
#if (!defined(SAFE_READ_AND_WRITE))

#define SAFE_READ_AND_WRITE(y, z)                                                  \
/*! Read access to _##y */                                                         \
  [[nodiscard]] inline const decltype(_##y)& y(void) const& { SAFELOCK(z); return _##y; }  \
  [[nodiscard]] inline decltype(_##y) y(void) && { SAFELOCK(z); return std::move(_##y); } \
/*! Write access to _##y */                                                        \
  inline void y(const decltype(_##y)& n) { SAFELOCK(z); _##y = n; }

#endif    // !SAFE_READ_AND_WRITE

/// Read and write with mutex that is part of the object
#if (!defined(SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX))

#define SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX(y, z)                       \
/*! Read access to _##y */                                                  \
  [[nodiscard]] inline const decltype(_##y)& y(void) const& { SAFELOCK(z); return _##y; }  \
  [[nodiscard]] inline decltype(_##y) y(void) && { SAFELOCK(z); return std::move(_##y); } \
/*! Write access to _##y */                                                 \
  inline void y(const decltype(_##y)& n) { SAFELOCK(z); _##y = n; }

#endif    // !SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX

/// Syntactic sugar for read-only access
#if (!defined(READ))

#define READ(y)                                                       \
/*! Read-only access to _##y */                                       \
  [[nodiscard]] inline const decltype(_##y)& y(void) const& { return _##y; }    \
  [[nodiscard]] inline decltype(_##y) y(void) && { return std::move(_##y); }

#endif    // !READ

/// Syntactic sugar for read-only access with thread locking
#if (!defined(SAFEREAD))

#define SAFEREAD(y, z)                                                            \
/*! Read-only access to _##y */                                                   \
  [[nodiscard]] inline decltype(_##y) y(void) const { SAFELOCK(z); return _##y; }

#endif    // !SAFEREAD

// alternative name for SAFEREAD
#if (!defined(SAFE_READ))

/// read with a mutex
#define SAFE_READ(y, z)                                                           \
/*! Read-only access to _##y */                                                   \
  [[nodiscard]] inline decltype(_##y) y(void) const { SAFELOCK(z); return _##y; }

#endif    // !SAFE_READ

/// read with mutex that is part of the object
#if (!defined(SAFEREAD_WITH_INTERNAL_MUTEX))

#define SAFEREAD_WITH_INTERNAL_MUTEX(y, z)                                                      \
/*! Read-only access to _##y */                                             \
  [[nodiscard]] inline decltype(_##y) y(void) { SAFELOCK(z); return _##y; }

#endif    // !SAFEREAD_WITH_INTERNAL_MUTEX

// alternative name for SAFEREAD_WITH_INTERNAL_MUTEX
#if (!defined(SAFE_READ_WITH_INTERNAL_MUTEX))

/// read with an internal mutex
#define SAFE_READ_WITH_INTERNAL_MUTEX(y, z)                                                      \
/*! Read-only access to _##y */                                             \
  [[nodiscard]] inline decltype(_##y) y(void) { SAFELOCK(z); return _##y; }

#endif    // !SAFE_READ_WITH_INTERNAL_MUTEX

/// Syntactic sugar for write access
#if (!defined(WRITE))

#define WRITE(y)                                       \
/*! Write access to _##y */                            \
  inline void y(const decltype(_##y)& n) { _##y = n; }

#define WRITE_RET(y)                                       \
/*! Write access to _##y */                            \
  inline auto y(const decltype(_##y)& n) -> decltype(*this)& { _##y = n; return (*this); }

#endif    // !READ_AND_WRITE

/// Error classes are all similar, so define a macro for them
#if (!defined(ERROR_CLASS))

#define ERROR_CLASS(z) \
\
class z : public x_error \
{ \
protected: \
\
public: \
\
/*! \brief      Construct from error code and reason \
    \param  n   error code \
    \param  s   reason \
*/ \
  inline z(const int n, const std::string& s = std::string()) : \
    x_error(n, s) \
  { } \
}

#endif      // !ERROR_CLASS

#if (!defined(FORTYPE))

#define FORTYPE(x) remove_const_t<decltype(x)>

#endif    // !FORTYPE

// ---------------------------------------------------------------------------

// a fairly large number of useful type-related functions

template<typename T>
using base_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

// https://stackoverflow.com/questions/12042824/how-to-write-a-type-trait-is-container-or-is-vector
// https://wandbox.org/permlink/D6Nf3Sb7PHjP6SrN

//(std::is_same_v<typename T::value_type, U>)
//template< typename T1, typename T2 >
//struct is_type
//  { constexpr static bool value { false }; };

#if 0
template< typename T1, typename T2 >
struct is_type
  { constexpr static bool value { std::is_same_v<base_type<T1>, base_type<T2>> }; };

template< typename T1, typename T2 >
  constexpr bool is_type_v = is_type<T1, T2>::value;

template< typename T1, typename T2 >
using IS_DEQUE = is_type_v<T1, std::deque<T2>>

#endif

//template< typename T1, typename T2 >
//struct is_same_type      { enum { result = false }; };

//template< typename T>
//struct is_same_type<T,T> { enum { result = true }; };
/*
https://stackoverflow.com/questions/51032671/idiomatic-way-to-write-concept-that-says-that-type-is-a-stdvector

template<class, template<class...> class>
inline constexpr bool is_specialization = false;
template<template<class...> class T, class... Args>
inline constexpr bool is_specialization<T<Args...>, T> = true;

template<class T>
concept bool Vec = is_specialization<T, std::vector>;

template<class T>
concept bool VecInt = Vec<T> && 
  std::is_same_v<int, typename T::value_type>;
*/

// see: https://stackoverflow.com/questions/51032671/idiomatic-way-to-write-concept-that-says-that-type-is-a-stdvector

template <class, template<class...> class>
inline constexpr bool is_specialization = false;

template <template<class...> class T, class... Args>
inline constexpr bool is_specialization<T<Args...>, T> = true;

// basic types
template <class T> concept is_int = std::is_same_v<T, int>;
template <class T> concept is_uint = std::is_same_v<T, unsigned int>;

// standard containers
template <class T> concept is_deque = is_specialization<T, std::deque>;

template <class T> concept is_list_v = is_specialization<T, std::list>;

template <class T> concept is_map_v = is_specialization<T, std::map>;

template <class T> concept is_multimap_v = is_specialization<T, std::multimap>;

template <class T> concept is_queue_v = is_specialization<T, std::queue>;

template <class T> concept is_set_v = is_specialization<T, std::set>;

template <class T> concept is_string_v = std::is_same_v<T, std::string>;

template <class T> concept is_multiset_v = is_specialization<T, std::multiset>;

template <class T> concept is_unordered_map_v = is_specialization<T, std::unordered_map>;

template <class T> concept is_unordered_multimap_v = is_specialization<T, std::unordered_multimap>;

template <class T> concept is_unordered_multiset_v = is_specialization<T, std::unordered_multiset>;

template <class T> concept is_unordered_set_v = is_specialization<T, std::unordered_set>;

template <class T> concept is_vector_v = is_specialization<T, std::vector>;

template <class T> concept is_mum_v = is_map_v<T> or is_unordered_map_v<T>;

template <class T> concept is_mmumm_v = is_multimap_v<T> or is_unordered_multimap_v<T>;

template <class T> concept is_sus_v = is_set_v<T> or is_unordered_set_v<T>;

template <class T> concept is_ssuss_v = is_multiset_v<T> or is_unordered_multiset_v<T>;

template <class T> concept ANYSET = is_sus_v<T> or is_ssuss_v<T>;

#if 0
// is a type a list?
template<class T>
struct is_list 
  { constexpr static bool value { false }; };

template<class T>
struct is_list<std::list<T>> 
  { constexpr static bool value { true }; };

template<class T>
constexpr bool is_list_v = is_list<T>::value;
#endif

#if 0
// is a type a map?
template<class T>
struct is_map 
  { constexpr static bool value { false }; };

template<class K, class V>
struct is_map<std::map<K, V>> 
  { constexpr static bool value { true }; };

template<class T>
constexpr bool is_map_v = is_map<T>::value;
#endif

#if 0
// is a type a queue?
template<class T>
struct is_queue 
  { constexpr static bool value { false }; };

template<class T>
struct is_queue<std::queue<T>> 
  { constexpr static bool value { true }; };

template<class T>
constexpr bool is_queue_v = is_queue<T>::value;
#endif

#if 0
// is a type a set?
template<class T>
struct is_set 
  { constexpr static bool value { false }; };

template<class T, class C, class A>
struct is_set<std::set<T, C, A>> 
  { constexpr static bool value { true }; };

template<class T>
constexpr bool is_set_v { is_set<T>::value };

template<typename T>
concept SET = requires(T a) 
{ is_set<T>::value == true;
};
#endif

#if 0
// is a type an unordered map?
template<class T>
struct is_unordered_map 
  { constexpr static bool value { false }; };

template<class K, class V>
struct is_unordered_map<std::unordered_map<K, V>> 
  { constexpr static bool value { true }; };

template< class T>
constexpr bool is_unordered_map_v = is_unordered_map<T>::value;
#endif

// is a type a map or unordered map?
//template<class T>
//constexpr bool is_mum_v { is_map_v<T> or is_unordered_map_v<T> };

#if 0
// is a type an unordered set?
template<class T>
struct is_unordered_set 
  { constexpr static bool value { false }; };

template<class T>
struct is_unordered_set<std::unordered_set<T>> 
  { constexpr static bool value { true }; };

template< class T>
constexpr bool is_unordered_set_v = is_unordered_set<T>::value;
#endif

#if 0
// is a type a set or unordered set?

template<class T>
constexpr bool is_sus_v { is_set_v<T> or is_unordered_set_v<T> };

template <class T> concept SUS = is_sus_v<T>;
#endif

#if 0
// is a type a multimap or unordered multimap?
template<class T>
struct is_multimap 
  { constexpr static bool value { false }; };

template<class K, class V>
struct is_multimap<std::multimap<K, V>> 
  { constexpr static bool value { true }; };
  
template<class T>
constexpr bool is_multimap_v { is_multimap<T>::value };

template<class T>
struct is_unordered_multimap 
  { constexpr static bool value { false }; };

template<class K, class V>
struct is_unordered_multimap<std::unordered_multimap<K, V>> 
  { constexpr static bool value { true }; };
  
template<class T>
constexpr bool is_unordered_multimap_v { is_unordered_multimap<T>::value };

template<class T>
constexpr bool is_mmumm_v { is_multimap_v<T> or is_unordered_multimap_v<T> };
#endif

#if 0
// is a type a multiset or unordered multiset?
template<class T>
struct is_multiset 
  { constexpr static bool value { false }; };

template<class E>
struct is_multiset<std::multiset<E>> 
  { constexpr static bool value { true }; };
  
template<class T>
constexpr bool is_multiset_v { is_multiset<T>::value };

template<class T>
struct is_unordered_multiset 
  { constexpr static bool value { false }; };

template<class E>
struct is_unordered_multiset<std::unordered_multiset<E>> 
  { constexpr static bool value { true }; };
  
template<class T>
constexpr bool is_unordered_multiset_v { is_unordered_multiset<T>::value };

template<class T>
constexpr bool is_ssuss_v { is_multiset_v<T> or is_unordered_multiset_v<T> };

template <class T> concept ANYSET = is_sus_v<T> or is_ssuss_v<T>;
#endif

#if 0
// is a type a string?
template<class T>
struct is_string 
  { constexpr static bool value { false }; };

template<>
struct is_string<std::string> 
  { constexpr static bool value { true }; };

template< class T>
constexpr bool is_string_v = is_string<T>::value;
#endif

#if 0
// is a type a vector?
template<class T>
struct is_vector 
  { constexpr static bool value { false }; };

template<class T>
struct is_vector<std::vector<T>> 
  { constexpr static bool value { true }; };

template<class T>
constexpr bool is_vector_v = is_vector<T>::value;

template<typename T> concept VECTOR = is_vector_v<T>;
#endif

// ---------------------------------------------------------------------------

// classes for tuples... it seems like there should be a way to do this with TMP,
// but the level-breaking caused by the need to control textual names seems to make
// this impossible without resorting to #defines. So since I don't immediately see
// a way to avoid #defines completely while keeping the usual access syntax
// (i.e., obj.name()), we might as well use a sledgehammer and do everything with #defines.

// we could access with something like obj.at<name>, but that would mean a different access
// style for this kind of object as compared to ordinary classes using the READ and READ_AND_WRITE
// macros

/// tuple class (1) -- complete overkill
#define WRAPPER_1(nm, a0, a1)                          \
                                                       \
class nm : public std::tuple < a0 >                    \
{                                                      \
protected:                                             \
                                                       \
public:                                                \
                                                       \
  explicit nm( const a0 & X )                          \
    { std::get<0>(*this) = X; }                        \
                                                       \
  inline a0 a1(void) const                             \
    { return std::get<0>(*this); }                     \
                                                       \
  inline void a1(const a0 & var)                       \
    { std::get<0>(*this) = var; }                      \
}

/// tuple class (2)
#define WRAPPER_2(nm, a0, a1, b0, b1)                  \
                                                       \
class nm : public std::tuple < a0, b0 >                \
{                                                      \
protected:                                             \
                                                       \
public:                                                \
                                                       \
  nm( a0 X, b0 Y)                                      \
    { std::get<0>(*this) = X;                          \
      std::get<1>(*this) = Y;                          \
    }                                                  \
                                                       \
  inline a0 a1(void) const                             \
    { return std::get<0>(*this); }                     \
                                                       \
  inline void a1(const a0 & var)                       \
    { std::get<0>(*this) = var; }                      \
                                                       \
  inline b0 b1(void) const                             \
    { return std::get<1>(*this); }                     \
                                                       \
  inline void b1(const b0 & var)                       \
    { std::get<1>(*this) = var; }                      \
}

/// tuple class (2)
#define WRAPPER_2_NC(nm, a0, a1, b0, b1)               \
                                                       \
class nm : public std::tuple < a0, b0 >                \
{                                                      \
protected:                                             \
                                                       \
public:                                                \
                                                       \
  nm( a0 X, b0 Y)                                      \
    { std::get<0>(*this) = X;                          \
      std::get<1>(*this) = Y;                          \
    }                                                  \
                                                       \
  inline a0 a1(void) const                             \
    { return std::get<0>(*this); }                     \
                                                       \
  inline void a1(a0 var)                               \
    { std::get<0>(*this) = var; }                      \
                                                       \
  inline b0 b1(void) const                             \
    { return std::get<1>(*this); }                     \
                                                       \
  inline void b1(b0 var)                               \
    { std::get<1>(*this) = var; }                      \
}

/// tuple class (3)
#define WRAPPER_3(nm, a0, a1, b0, b1, c0, c1)          \
                                                       \
class nm : public std::tuple < a0, b0, c0 >            \
{                                                      \
protected:                                             \
                                                       \
public:                                                \
                                                       \
  nm( a0 X, b0 Y, c0 Z)                                \
    { std::get<0>(*this) = X;                          \
      std::get<1>(*this) = Y;                          \
      std::get<2>(*this) = Z;                          \
    }                                                  \
                                                       \
  nm(void) = default;                                  \
                                                       \
  inline a0 a1(void) const                             \
    { return std::get<0>(*this); }                     \
                                                       \
  inline void a1(const a0 & var)                       \
    { std::get<0>(*this) = var; }                      \
                                                       \
  inline b0 b1(void) const                             \
    { return std::get<1>(*this); }                     \
                                                       \
  inline void b1(const b0 & var)                       \
    { std::get<1>(*this) = var; }                      \
                                                       \
  inline c0 c1(void) const                             \
    { return std::get<2>(*this); }                     \
                                                       \
  inline void c1(const c0 & var)                       \
    { std::get<2>(*this) = var; }                      \
};                                                     \
                                                       \
inline std::ostream& operator<<(std::ostream& ost, const nm& type)  \
{ ost << #nm << ": " << std::endl                                   \
      << #a1 << ": " << type.a1() << std::endl                      \
      << #b1 << ": " << type.b1() << std::endl                      \
      << #c1 << ": " << type.c1();                                  \
                                                                    \
  return ost;                                                       \
}

/// tuple class (3)
#define WRAPPER_3_NC(nm, a0, a1, b0, b1, c0, c1)        \
                                                        \
class nm : public std::tuple < a0, b0, c0 >             \
{                                                       \
protected:                                              \
                                                        \
public:                                                 \
                                                        \
  nm( a0 X, b0 Y, c0 Z)                                 \
    { std::get<0>(*this) = X;                           \
      std::get<1>(*this) = Y;                           \
      std::get<2>(*this) = Z;                           \
    }                                                   \
                                                        \
  inline a0 a1(void) const                              \
    { return std::get<0>(*this); }                      \
                                                        \
  inline void a1(a0 var)                                \
    { std::get<0>(*this) = var; }                       \
                                                        \
  inline b0 b1(void) const                              \
    { return std::get<1>(*this); }                      \
                                                        \
  inline void b1(b0 var)                                \
    { std::get<1>(*this) = var; }                       \
                                                        \
  inline c0 c1(void) const                              \
    { return std::get<2>(*this); }                      \
                                                        \
  inline void c1(c0 var)                                \
    { std::get<2>(*this) = var; }                       \
}

/// tuple class (3)
#define WRAPPER_3_SERIALIZE(nm, a0, a1, b0, b1, c0, c1)                     \
                                                                            \
class nm : public std::tuple < a0, b0, c0 >                                 \
{                                                                           \
protected:                                                                  \
                                                                            \
public:                                                                     \
                                                                            \
  nm( a0 X, b0 Y, c0 Z)                                                     \
    { std::get<0>(*this) = X;                                               \
      std::get<1>(*this) = Y;                                               \
      std::get<2>(*this) = Z;                                               \
    }                                                                       \
                                                                            \
  nm(void) = default;                                                       \
                                                                            \
  inline a0 a1(void) const                                                  \
    { return std::get<0>(*this); }                                          \
                                                                            \
  inline void a1(const a0 & var)                                            \
    { std::get<0>(*this) = var; }                                           \
                                                                            \
  inline b0 b1(void) const                                                  \
    { return std::get<1>(*this); }                                          \
                                                                            \
  inline void b1(const b0 & var)                                            \
    { std::get<1>(*this) = var; }                                           \
                                                                            \
  inline c0 c1(void) const                                                  \
    { return std::get<2>(*this); }                                          \
                                                                            \
  inline void c1(const c0 & var)                                            \
    { std::get<2>(*this) = var; }                                           \
                                                                            \
    template<typename Archive>                                              \
  void serialize(Archive& ar, const unsigned version)                       \
    { ar & std::get<0>(*this)                                               \
         & std::get<1>(*this)                                               \
         & std::get<2>(*this);                                              \
    }                                                                       \
}

/// tuple class (4)
#define WRAPPER_4_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1)                    \
                                                                            \
class nm : public std::tuple < a0, b0, c0, d0 >                             \
{                                                                           \
protected:                                                                  \
                                                                            \
public:                                                                     \
                                                                            \
  nm( a0 X, b0 Y, c0 Z, d0 A)                                               \
    { std::get<0>(*this) = X;                                               \
      std::get<1>(*this) = Y;                                               \
      std::get<2>(*this) = Z;                                               \
      std::get<3>(*this) = A;                                               \
    }                                                                       \
                                                                            \
  inline a0 a1(void) const                                                  \
    { return std::get<0>(*this); }                                          \
                                                                            \
  inline void a1(a0 var)                                                    \
    { std::get<0>(*this) = var; }                                           \
                                                                            \
  inline b0 b1(void) const                                                  \
    { return std::get<1>(*this); }                                          \
                                                                            \
  inline void b1(b0 var)                                                    \
    { std::get<1>(*this) = var; }                                           \
                                                                            \
  inline c0 c1(void) const                                                  \
    { return std::get<2>(*this); }                                          \
                                                                            \
  inline void c1(c0 var)                                                    \
    { std::get<2>(*this) = var; }                                           \
                                                                            \
  inline d0 d1(void) const                                                  \
    { return std::get<3>(*this); }                                          \
                                                                            \
  inline void d1(d0 var)                                                    \
    { std::get<3>(*this) = var; }                                           \
}

/// tuple class (4)
#define WRAPPER_4_SERIALIZE(nm, a0, a1, b0, b1, c0, c1, d0, d1)             \
                                                                            \
class nm : public std::tuple < a0, b0, c0, d0  >                            \
{                                                                           \
protected:                                                                  \
                                                                            \
public:                                                                     \
                                                                            \
  nm( a0 X, b0 Y, c0 Z, d0 A)                                               \
    { std::get<0>(*this) = X;                                               \
      std::get<1>(*this) = Y;                                               \
      std::get<2>(*this) = Z;                                               \
      std::get<3>(*this) = A;                                               \
    }                                                                       \
                                                                            \
  nm(void) = default;                                                       \
                                                                            \
  inline a0 a1(void) const                                                  \
    { return std::get<0>(*this); }                                          \
                                                                            \
  inline void a1(const a0 & var)                                            \
    { std::get<0>(*this) = var; }                                           \
                                                                            \
  inline b0 b1(void) const                                                  \
    { return std::get<1>(*this); }                                          \
                                                                            \
  inline void b1(const b0 & var)                                            \
    { std::get<1>(*this) = var; }                                           \
                                                                            \
  inline c0 c1(void) const                                                  \
    { return std::get<2>(*this); }                                          \
                                                                            \
  inline void c1(const c0 & var)                                            \
    { std::get<2>(*this) = var; }                                           \
                                                                            \
  inline d0 d1(void) const                                                  \
    { return std::get<3>(*this); }                                          \
                                                                            \
  inline void d1(const d0 & var)                                            \
    { std::get<3>(*this) = var; }                                           \
                                                                            \
template<typename Archive>                                                  \
  void serialize(Archive& ar, const unsigned version)                       \
    { ar & std::get<0>(*this)                                               \
         & std::get<1>(*this)                                               \
         & std::get<2>(*this)                                               \
         & std::get<3>(*this);                                              \
    }                                                                       \
};                                                                          \
                                                                            \
inline std::ostream& operator<<(std::ostream& ost, const nm& type)          \
{ ost << #nm << ": " << std::endl                                           \
      << #a1 << ": " << type.a1() << std::endl                              \
      << #b1 << ": " << type.b1() << std::endl                              \
      << #c1 << ": " << type.c1() << std::endl                              \
      << #d1 << ": " << type.d1();                                          \
                                                                            \
  return ost;                                                               \
}

/// tuple class (5)
#define WRAPPER_5_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1, e0, e1)            \
                                                                            \
class nm : public std::tuple < a0, b0, c0, d0, e0 >                         \
{                                                                           \
protected:                                                                  \
                                                                            \
public:                                                                     \
                                                                            \
  nm( a0 X, b0 Y, c0 Z, d0 A, e0 B)                                         \
    { std::get<0>(*this) = X;                                               \
      std::get<1>(*this) = Y;                                               \
      std::get<2>(*this) = Z;                                               \
      std::get<3>(*this) = A;                                               \
      std::get<4>(*this) = B;                                               \
    }                                                                       \
                                                                            \
  nm(void) = default;                                                       \
                                                                            \
  inline a0 a1(void) const                                                  \
    { return std::get<0>(*this); }                                          \
                                                                            \
  inline void a1(a0 var)                                                    \
    { std::get<0>(*this) = var; }                                           \
                                                                            \
  inline b0 b1(void) const                                                  \
    { return std::get<1>(*this); }                                          \
                                                                            \
  inline void b1(b0 var)                                                    \
    { std::get<1>(*this) = var; }                                           \
                                                                            \
  inline c0 c1(void) const                                                  \
    { return std::get<2>(*this); }                                          \
                                                                            \
  inline void c1(c0 var)                                                    \
    { std::get<2>(*this) = var; }                                           \
                                                                            \
  inline d0 d1(void) const                                                  \
    { return std::get<3>(*this); }                                          \
                                                                            \
  inline void d1(d0 var)                                                    \
    { std::get<3>(*this) = var; }                                           \
                                                                            \
  inline e0 e1(void) const                                                  \
    { return std::get<4>(*this); }                                          \
                                                                            \
  inline void e1(e0 var)                                                    \
    { std::get<4>(*this) = var; }                                           \
}

/// tuple class (6)
#define WRAPPER_6_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1, e0, e1, f0, f1)    \
                                                                            \
class nm : public std::tuple < a0, b0, c0, d0, e0, f0 >                     \
{                                                                           \
protected:                                                                  \
                                                                            \
public:                                                                     \
                                                                            \
  nm( a0 X, b0 Y, c0 Z, d0 A, e0 B, f0 C)                                   \
    { std::get<0>(*this) = X;                                               \
      std::get<1>(*this) = Y;                                               \
      std::get<2>(*this) = Z;                                               \
      std::get<3>(*this) = A;                                               \
      std::get<4>(*this) = B;                                               \
      std::get<5>(*this) = C;                                               \
    }                                                                       \
                                                                            \
  inline a0 a1(void) const                                                  \
    { return std::get<0>(*this); }                                          \
                                                                            \
  inline void a1(a0 var)                                                    \
    { std::get<0>(*this) = var; }                                           \
                                                                            \
  inline b0 b1(void) const                                                  \
    { return std::get<1>(*this); }                                          \
                                                                            \
  inline void b1(b0 var)                                                    \
    { std::get<1>(*this) = var; }                                           \
                                                                            \
  inline c0 c1(void) const                                                  \
    { return std::get<2>(*this); }                                          \
                                                                            \
  inline void c1(c0 var)                                                    \
    { std::get<2>(*this) = var; }                                           \
                                                                            \
  inline d0 d1(void) const                                                  \
    { return std::get<3>(*this); }                                          \
                                                                            \
  inline void d1(d0 var)                                                    \
    { std::get<3>(*this) = var; }                                           \
                                                                            \
  inline e0 e1(void) const                                                  \
    { return std::get<4>(*this); }                                          \
                                                                            \
  inline void e1(e0 var)                                                    \
    { std::get<4>(*this) = var; }                                           \
                                                                            \
  inline f0 f1(void) const                                                  \
    { return std::get<5>(*this); }                                          \
                                                                            \
  inline void f1(f0 var)                                                    \
    { std::get<5>(*this) = var; }                                           \
}

/// tuple class (7)
#define WRAPPER_7_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1, e0, e1, f0, f1, g0, g1)    \
                                                                                    \
class nm : public std::tuple < a0, b0, c0, d0, e0, f0, g0 >                         \
{                                                                                   \
protected:                                                                          \
                                                                                    \
public:                                                                             \
                                                                                    \
  nm( a0 X, b0 Y, c0 Z, d0 A, e0 B, f0 C, g0 D)                                     \
    { std::get<0>(*this) = X;                                                       \
      std::get<1>(*this) = Y;                                                       \
      std::get<2>(*this) = Z;                                                       \
      std::get<3>(*this) = A;                                                       \
      std::get<4>(*this) = B;                                                       \
      std::get<5>(*this) = C;                                                       \
      std::get<6>(*this) = D;                                                       \
    }                                                                               \
                                                                                    \
  inline a0 a1(void) const                                                          \
    { return std::get<0>(*this); }                                                  \
                                                                                    \
  inline void a1(a0 var)                                                            \
    { std::get<0>(*this) = var; }                                                   \
                                                                                    \
  inline b0 b1(void) const                                                          \
    { return std::get<1>(*this); }                                                  \
                                                                                    \
  inline void b1(b0 var)                                                            \
    { std::get<1>(*this) = var; }                                                   \
                                                                                    \
  inline c0 c1(void) const                                                          \
    { return std::get<2>(*this); }                                                  \
                                                                                    \
  inline void c1(c0 var)                                                            \
    { std::get<2>(*this) = var; }                                                   \
                                                                                    \
  inline d0 d1(void) const                                                          \
    { return std::get<3>(*this); }                                                  \
                                                                                    \
  inline void d1(d0 var)                                                            \
    { std::get<3>(*this) = var; }                                                   \
                                                                                    \
  inline e0 e1(void) const                                                          \
    { return std::get<4>(*this); }                                                  \
                                                                                    \
  inline void e1(e0 var)                                                            \
    { std::get<4>(*this) = var; }                                                   \
                                                                                    \
  inline f0 f1(void) const                                                          \
    { return std::get<5>(*this); }                                                  \
                                                                                    \
  inline void f1(f0 var)                                                            \
    { std::get<5>(*this) = var; }                                                   \
                                                                                    \
  inline g0 g1(void) const                                                          \
    { return std::get<6>(*this); }                                                  \
                                                                                    \
  inline void g1(g0 var)                                                            \
    { std::get<6>(*this) = var; }                                                   \
}

/// tuple class (8)
#define WRAPPER_8_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1, e0, e1, f0, f1, g0, g1, h0, h1)    \
                                                                                            \
class nm : public std::tuple < a0, b0, c0, d0, e0, f0, g0, h0 >                             \
{                                                                                           \
protected:                                                                                  \
                                                                                            \
public:                                                                                     \
                                                                                            \
  nm( a0 X, b0 Y, c0 Z, d0 A, e0 B, f0 C, g0 D, h0 E)                                       \
    { std::get<0>(*this) = X;                                                               \
      std::get<1>(*this) = Y;                                                               \
      std::get<2>(*this) = Z;                                                               \
      std::get<3>(*this) = A;                                                               \
      std::get<4>(*this) = B;                                                               \
      std::get<5>(*this) = C;                                                               \
      std::get<6>(*this) = D;                                                               \
      std::get<7>(*this) = E;                                                               \
    }                                                                                       \
                                                                                            \
  inline a0 a1(void) const                                                                  \
    { return std::get<0>(*this); }                                                          \
                                                                                            \
  inline void a1(a0 var)                                                                    \
    { std::get<0>(*this) = var; }                                                           \
                                                                                            \
  inline b0 b1(void) const                                                                  \
    { return std::get<1>(*this); }                                                          \
                                                                                            \
  inline void b1(b0 var)                                                                    \
    { std::get<1>(*this) = var; }                                                           \
                                                                                            \
  inline c0 c1(void) const                                                                  \
    { return std::get<2>(*this); }                                                          \
                                                                                            \
  inline void c1(c0 var)                                                                    \
    { std::get<2>(*this) = var; }                                                           \
                                                                                            \
  inline d0 d1(void) const                                                                  \
    { return std::get<3>(*this); }                                                          \
                                                                                            \
  inline void d1(d0 var)                                                                    \
    { std::get<3>(*this) = var; }                                                           \
                                                                                            \
  inline e0 e1(void) const                                                                  \
    { return std::get<4>(*this); }                                                          \
                                                                                            \
  inline void e1(e0 var)                                                                    \
    { std::get<4>(*this) = var; }                                                           \
                                                                                            \
  inline f0 f1(void) const                                                                  \
    { return std::get<5>(*this); }                                                          \
                                                                                            \
  inline void f1(f0 var)                                                                    \
    { std::get<5>(*this) = var; }                                                           \
                                                                                            \
  inline g0 g1(void) const                                                                  \
    { return std::get<6>(*this); }                                                          \
                                                                                            \
  inline void g1(g0 var)                                                                    \
    { std::get<6>(*this) = var; }                                                           \
                                                                                            \
  inline h0 h1(void) const                                                                  \
    { return std::get<7>(*this); }                                                          \
                                                                                            \
  inline void h1(h0 var)                                                                    \
    { std::get<7>(*this) = var; }                                                           \
}

/*! \brief      Does a set or unordered_set contains a particular member?
    \param  s   set or unordered_set  to be tested
    \param  v   object to be tested for membership
    \return     Whether <i>v</i> is a member of <i>s</i>
*/
template <class T, class U>
bool operator>(const T& s, const U& v)
  requires (is_sus_v<T>) and (std::is_same_v<typename T::value_type, U>)
  { return s.contains(v); }

/*! \brief      Is an object a member of a set or unordered_set?
    \param  v   object to be tested for membership
    \param  s   set or unordered_set  to be tested
    \return     Whether <i>v</i> is a member of <i>s</i>
*/
template <class E, class S>
bool operator<(const E& v, const S& s)
  requires (is_sus_v<S>) and (std::is_same_v<typename S::value_type, E>)
//  { return s.contains(v); }
  { return (s > v); }

/*! \brief      Is an object a member of a set, an unordered_set, or a key in a map or unordered map?
    \param  s   set or unordered_set  to be tested
    \param  v   object to be tested for membership
    \return     Whether <i>t</i> is a member/key of <i>s</i>
*/
//template <class T, class U>
//bool contains(const T& s, const U& v)
//  requires (is_sus_v<T> or is_mum_v<T>) and (std::is_same_v<typename T::key_type, U>)
//  { return s.find(v) != s.cend(); }

/// union of two sets of the same type
template<class T>
T operator+(const T& s1, const T& s2)
  requires (is_set_v<T>)
  { T rv;

    std::set_union(s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(), std::inserter(rv, rv.end()));

    return rv;
  }

/*! \brief      Is an object a key of a map or unordered_map and, if so, return the value (or the default-constructed value)
    \param  m   map or unordered_map to be searched
    \param  k   target key
    \return     Whether <i>k</i> is a member of <i>m</i> and, if so, the corresponding value (or the default-constructed value)
    
    // possibly should return variant instead
*/
#if 0
template <class M, class K>
std::pair<bool, typename M::mapped_type> operator>(const M& m, const K& k)
  requires (is_mum_v<M>) and (std::is_same_v<typename M::key_type, K>) and (std::is_default_constructible_v<typename M::mapped_type>)
{ using V  = typename M::mapped_type;
  using RT = std::invoke_result_t< decltype(operator><M, K>), const M&, const K&>;

  const auto cit { m.find(k) };

  return ( (cit == m.cend()) ? RT { false, V() } : RT { true, cit->second } );
}
#endif

template <class M, class K>
inline bool operator>(const M& m, const K& k)
  requires (is_mum_v<M>) and (std::is_same_v<typename M::key_type, K>) /* and (std::is_default_constructible_v<typename M::mapped_type> ) */
{ //using V  = typename M::mapped_type;
  //using RT = std::invoke_result_t< decltype(operator><M, K>), const M&, const K&>;

  //const auto cit { m.find(k) };

  //return ( (cit == m.cend()) ? RT { false, V() } : RT { true, cit->second } );
  return m.contains(k);
}

template <class M, class K>
inline bool operator<(const K& k, const M& m)
  requires (is_mum_v<M>) and (std::is_same_v<typename M::key_type, K>) /* and (std::is_default_constructible_v<typename M::mapped_type>) */
{ //using V  = typename M::mapped_type;
  //using RT = std::invoke_result_t< decltype(operator><M, K>), const M&, const K&>;

  //const auto cit { m.find(k) };

  //return ( (cit == m.cend()) ? RT { false, V() } : RT { true, cit->second } );
  return (m > k);
}

/*! \brief      Is an object a key of a map or unordered map; if so return the value, otherwise return a provided default
    \param  m   map or unordered map to be searched
    \param  k   target key
    \param  d   default
    \return     if <i>k</i> is a member of <i>m</i>, the corresponding value, otherwise the default

    The difference between this and ">" is that this does not tell you whether the key was found
*/
template <class C, class K>
typename C::mapped_type MUM_VALUE(const C& m, const K& k, const typename C::mapped_type& d = typename C::mapped_type())
  requires (is_mum_v<C>) && (std::is_same_v<typename C::key_type, K>)
{ const auto cit { m.find(k) };

  return ( (cit == m.cend()) ? d : cit->second );
}

/*! \brief      Is an object a key of a map or unordered map; if so return the result of executing a member function on the value, otherwise return a provided default
    \param  m   map or unordered map to be searched
    \param  k   target key
    \param  pf  pointer to member function to be executed
    \param  d   default value
    \return     if <i>k</i> is a member of <i>m</i>, the result of executing <i>pf</i> in the corresponding value, otherwise the default
*/
template <class C, class K, class PF, class MT = typename C::mapped_type, class RT = std::invoke_result_t<PF, MT>>
auto MUMF_VALUE(const C& m, const K& k, PF pf, RT d = RT { } ) -> RT
  requires (is_mum_v<C>) and (std::is_same_v<typename C::key_type, K>)
{ const auto cit { m.find(k) };

  return ( (cit == m.cend()) ? d : (cit->second.*pf)() );
} 

/*! \brief                      Invert a mapping from map<T, set<T> > to map<T, set<T> >, where final keys are the elements of the original set
    \param  original_mapping    original mapping
    \return                     inverted mapping
*/
template <typename M>  // M = map<T, set<T> >
auto INVERT_MAPPING(const M& original_mapping) -> std::map<typename M::key_type, typename M::key_type>
{ std::map<typename M::key_type, typename M::key_type> rv;

  for (auto cit { original_mapping.cbegin() }; cit != original_mapping.cend(); ++cit)
  { for (const auto& p : cit->second)
      rv += { p, cit->first };
  }

  return rv;
}

#if 0
template <class D, class P> // P = parameter; D = data in the database
class cached_data
{
protected:
  std::map<P, D> _db;

  const D (*_lookup_fn)(const P&);

public:
  explicit cached_data(const D (*fn)(const P&)) :
    _lookup_fn(fn)
  { }

  D data(P& param)
    { auto it = _db.find(param);

      if (it != _db.end())
        return it->second;

      D value = _lookup_fn(param);

      _db.insert( { param, value } );
      return value;
    }

  D overwrite_data(P& param)
    { auto it = _db.find(param);

      if (it != _db.end())
        return it->second;

      D value = _lookup_fn(param);

      _db[param] = value;
      return value;
    }

  void erase(P& param)
  { auto it = _db.find(param);

    if (it == _db.end())
      return;

    _db.erase(param);
  }
};
#endif

/*! \class  accumulator
    \brief  accumulate values, and inform when a threshold is reached
*/
template <typename T>
class accumulator
{ using COUNTER = unsigned int;

protected:

  std::map<T, COUNTER> _values;                ///< all the known values, with the number of times each has been added
  COUNTER              _threshold;             ///< threshold value

public:

/// default constructor
  explicit accumulator(const COUNTER thold = 1) :
    _threshold(thold)
  { }

  READ_AND_WRITE(threshold);                        ///< threshold value

/*! \brief          Add a value or increment it a known number of times
    \param  val     value to add or increment
    \param  n       number of times to add it
    \return         whether final number of times <i>value</i> has been added is at or greater than the threshold
*/
  bool add(const T& val, const COUNTER n = 1)
  { //if (!contains(_values, val))
    if (!_values.contains(val))
      _values += { val, n };
    else
      _values[val] += n;

    return (_values[val] >= _threshold);
  }

/*! \brief          Number of times a value has been added
    \param  val     target value
    \return         total number of times <i>value</i> has been added
*/
  unsigned int value(const T& val) const
//    { return ( ( _values.find(val) == _values.cend() ) ? 0 : _values.at(val) ); }
    { return ( _values.contains(val) ? _values.at(val) : 0 ); }
};

// convenient syntactic sugar for some STL functions

/*! \brief              Add an element to a MUM
    \param  m           destination MUM
    \param  element     element to insert
*/
template <typename C, typename K, typename V>
void operator+=(C& mum, std::pair<K, V>&& element)
  requires ( (is_mum_v<C> and (std::is_same_v<typename C::key_type, K>) and (std::is_same_v<typename C::mapped_type, V>)) or
             (is_mmumm_v<C> and (std::is_same_v<typename C::key_type, K>) and (std::is_same_v<typename C::mapped_type, V>))
           )
  { mum.emplace(std::move(element)); }

/*! \brief              Add an element to a MUM
    \param  m           destination MUM
    \param  element     element to insert
*/
/* The perceived "wisdom" is that requires clauses are clearer than SFINAE,. In this case, that is clearly untrue.
    Compare this to the above function, where I purposefully used requires clauses to achieve the same result
*/
template <typename C>
void operator+=(C& mum, const std::pair<typename C::key_type, typename C::mapped_type>& il)
  requires (is_mum_v<C> or is_mmumm_v<C>)
  { mum.emplace(il); }

/*! \brief          Write a <i>map<key, value></i> object to an output stream
    \param  ost     output stream
    \param  mp      object to write
    \return         the output stream
*/
template <class T1, class T2>
std::ostream& operator<<(std::ostream& ost, const std::map<T1, T2>& mp)
{ for (typename std::map<T1, T2>::const_iterator cit = mp.cbegin(); cit != mp.cend(); ++cit)
    ost << "map[" << cit->first << "]: " << cit->second << std::endl;

  return ost;
}

/*! \brief          Write an <i>unordered_map<key, value></i> object to an output stream
    \param  ost     output stream
    \param  mp      object to write
    \return         the output stream
    
    Note that the output order is, unsurprisingly, effectively random
*/
template <class T1, class T2>
std::ostream& operator<<(std::ostream& ost, const std::unordered_map<T1, T2>& mp)
{ for (typename std::unordered_map<T1, T2>::const_iterator cit = mp.cbegin(); cit != mp.cend(); ++cit)
    ost << "unordered_map[" << cit->first << "]: " << cit->second << std::endl;

  return ost;
}

/*! \brief          Apply a function to all in a container
    \param  first   container
    \param  fn      function
    \return         <i>fn</i>
*/

// see https://en.cppreference.com/w/cpp/algorithm/ranges/for_each
template< std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity, std::indirectly_unary_invocable<std::projected<I, Proj>> Fun >
constexpr std::ranges::for_each_result<I, Fun> FOR_ALL( I first, S last, Fun f, Proj proj = {} ) { return std::ranges::for_each(first, last, f, proj); }; 

template< std::ranges::input_range R, class Proj = std::identity, std::indirectly_unary_invocable<std::projected<std::ranges::iterator_t<R>, Proj>> Fun >
constexpr std::ranges::for_each_result<std::ranges::borrowed_iterator_t<R>, Fun> FOR_ALL( R&& r, Fun f, Proj proj = {} ) { return std::ranges::for_each(std::forward<R>(r), f, proj); }; 

// https://en.cppreference.com/w/cpp/algorithm/ranges/all_any_none_of
template< std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity, std::indirect_unary_predicate<std::projected<I, Proj>> Pred >
constexpr bool ANY_OF( I first, S last, Pred pred, Proj proj = {} ) { return std::ranges::any_of(first, last, pred, proj); };

template< std::ranges::input_range R, class Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>,Proj>> Pred >
constexpr bool ANY_OF( R&& r, Pred pred, Proj proj = {} ) { return std::ranges::any_of(std::forward<R>(r), pred, proj); };

template< std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity, std::indirect_unary_predicate<std::projected<I, Proj>> Pred >
constexpr bool ALL_OF( I first, S last, Pred pred, Proj proj = {} ) { return std::ranges::all_of(first, last, pred, proj); };

template< std::ranges::input_range R, class Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>,Proj>> Pred >
constexpr bool ALL_OF( R&& r, Pred pred, Proj proj = {} ) { return std::ranges::all_of(std::forward<R>(r), pred, proj); };

template< std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity, std::indirect_unary_predicate<std::projected<I, Proj>> Pred >
constexpr bool NONE_OF( I first, S last, Pred pred, Proj proj = {} )  { return std::ranges::none_of(first, last, pred, proj); };

template< std::ranges::input_range R, class Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>,Proj>> Pred >
constexpr bool NONE_OF( R&& r, Pred pred, Proj proj = {} ) { return std::ranges::none_of(std::forward<R>(r), pred, proj); };

/*! \brief          Copy all in a container to another container
    \param  first   initial container
    \param  oi      iterator on final container
    \return         <i>oi</i>
*/

//https://en.cppreference.com/w/cpp/algorithm/ranges/copy
template< std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O > requires  std::indirectly_copyable<I, O>
constexpr std::ranges::in_out_result<I, O> COPY_ALL( I first, S last, O result ) { return std::ranges::copy(first, last, result); };

template< std::ranges::input_range R, std::weakly_incrementable O > requires  std::indirectly_copyable<std::ranges::iterator_t<R>, O>
constexpr std::ranges::in_out_result<std::ranges::borrowed_iterator_t<R>, O> COPY_ALL( R&& r, O result )  { return std::ranges::copy(std::forward<R>(r), result); };

/*! \brief          Remove values in a container that match a predicate, and resize the container
    \param  first   container
    \param  pred    predicate to apply

    Does not work for maps
*/
template <class Input, class UnaryPredicate>
void REMOVE_IF_AND_RESIZE(Input& first, UnaryPredicate pred)
  { first.erase(std::remove_if(first.begin(), first.end(), pred), first.end()); }

/*! \brief          Remove map/unordered map values that match a predicate, and resize the map
    \param  items   map
    \param  pred    predicate to apply
*/
template<typename M, typename PredicateT>
  requires (is_mum_v<M>)
void REMOVE_IF_AND_RESIZE(M& items, const PredicateT& pred)
{ for( auto it { items.begin() }; it != items.end(); )
  { if( pred(*it) )
      it = items.erase(it);
    else
      ++it;
  }
};

/*! \brief      Reverse the contents of a container
    \param  v   container
*/
template <class Input>
inline void REVERSE(Input& v)
  { std::ranges::reverse(v); }

/*! \brief          Find first value in a container that matches a predicate
    \param  v       container
    \param  pred    (boolean) predicate to apply
    \return         first value in <i>v</i> for which <i>pred</i> is true
*/
//template <typename Input, typename UnaryPredicate>
//inline auto FIND_IF(Input& v, UnaryPredicate pred) -> typename Input::iterator
//  { return std::find_if(v.begin(), v.end(), pred); }

/*! \brief          Find first value in a container that matches a predicate
    \param  v       container (const)
    \param  pred    (boolean) predicate to apply
    \return         first value in <i>v</i> for which <i>pred</i> is true
*/

// https://en.cppreference.com/w/cpp/algorithm/ranges/find
template< std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity, std::indirect_unary_predicate<std::projected<I, Proj>> Pred >
constexpr I FIND_IF( I first, S last, Pred pred, Proj proj = {} ) { return std::ranges::find_if(first, last, pred, proj); };

template< std::ranges::input_range R, class Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<R>, Proj>> Pred >
constexpr std::ranges::borrowed_iterator_t<R> FIND_IF( R&& r, Pred pred, Proj proj = {} ) { return std::ranges::find_if(r, pred, proj); };

/*! \brief              Bound a value within limits
    \param  val         value to bound
    \param  low_val     lower bound
    \param  high_val    upper bound
    \return             max(min(<i>val</i>, <i>max_val</i>), <i>min_val</i>)
*/
template <typename T>
inline T LIMIT(const T val, const T low_val, const T high_val)
  { return (val < low_val ? low_val : (val > high_val ? high_val : val)); }

/*! \brief              Bound a value within limits
    \param  val         value to bound
    \param  low_val     lower bound
    \param  high_val    upper bound
    \return             max(min(<i>val</i>, <i>max_val</i>), <i>min_val</i>)
*/
template <typename T, typename U, typename V>
inline T LIMIT(const T val, const U low_val, const V high_val)
  { return (val < static_cast<T>(low_val) ? static_cast<T>(low_val) : (val > static_cast<T>(high_val) ? static_cast<T>(high_val) : val)); }

/// a version of floor() that returns a float instead of a double (not quite the same as floorf)
template <typename T>
inline float ffloor(T val)
  { return static_cast<float>(floor(val)); }

/// a version of floor() that returns an int instead of a double (not quite the same as floorl)
template <typename T>
inline int ifloor(T val)
  { return static_cast<int>(floor(val)); }

// define a hash function for pairs
// http://stackoverflow.com/questions/13485979/hash-function-of-unordered-set/13486174#13486174
// http://www.cplusplus.com/reference/functional/hash/
// https://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
namespace std
{ template <typename T, typename U>
  struct hash< std::pair<T, U> >

  { using result_type = size_t;

    result_type operator()( const std::pair<T, U>& k ) const
    { result_type res { 17 };
            
      res = res * 31 + hash<T>()( k.first );
      res = res * 31 + hash<U>()( k.second );
      
      return res;
    }
  };
}

// generate a set from a vector, done here because the vector MUST NOT be generated twice 
// (e.g., by returning a member of a class twice), because then the iterators might not match
template <typename T>
inline std::set<T> SET_FROM_VECTOR(const std::vector<T>& v)
  { return std::set<T> { v.cbegin(), v.cend() }; }

/*! \brief      Sort the contents of a container
    \param  v   container
    \param  f   sort function
*/
template <typename C, typename F = std::less<>>
inline void SORT(C& v, F f = F())
  { std::ranges::sort(v, f); }

/*! \brief              Add an element to a set or unordered set
    \param  sus         destination set or unordered set
    \param  element     element to insert
*/
#if 1
template <typename C, typename T>
inline void operator+=(C& sus, T&& element)
//  requires is_sus_v<C> and (std::is_convertible_v<typename C::value_type, base_type<T>>) or
//           is_ssuss_v<C> and (std::is_convertible_v<typename C::value_type, base_type<T>>)
//  requires (is_sus_v<C> and (std::is_convertible_v<base_type<T>, typename C::value_type>)) or
//           (is_ssuss_v<C> and (std::is_convertible_v<base_type<T>, typename C::value_type>))
  requires (is_sus_v<C> or is_ssuss_v<C>) and (std::is_convertible_v<base_type<T>, typename C::value_type>)
  { sus.insert(std::forward<T>(element)); }
#endif
//inline void operator+=(ANYSET auto& sus, typename decltype(sus)::value_type&& element)
//  { sus.insert(std::forward<typename decltype(sus)::value_type>(element)); }

/*! \brief              Add an element to a set or unordered set
    \param  sus         destination set or unordered set
    \param  element     element to insert
*/
// OK
//template <typename C>
//inline void operator+=(C& sus, const typename C::value_type& element)
//  requires is_sus_v<C>
//  { sus.insert(element); }

// OK
//template <SUS C>
//inline void operator+=(C& sus, const typename C::value_type& element)
//  { sus.insert(element); }

// Seems to be OK; https://www.youtube.com/watch?v=SYLgG7Q5Zws at 16:28; I think that the single ampersand is right
// see also https://www.sandordargo.com/blog/2021/02/17/cpp-concepts-4-ways-to-use-them
inline void operator+=(ANYSET auto& sus, const typename decltype(sus)::value_type& element)
  { sus.insert(element); }
//template <typename C, typename T>
//inline void operator+=(C& sus, const typename C::value_type& element)
//  requires is_sus_v<C> or is_ssuss_v<C>
//  { sus.insert(element); }

/*! \brief          Add all elements of a vector to a set or unordered set
    \param  sus     destination set or unordered set
    \param  vec     vector to insert
*/
template <typename C, typename V>
inline void operator+=(C& sus, const V& vec)
  requires is_sus_v<C> and is_vector_v<V> and (std::is_same_v<typename C::value_type, typename V::value_type>)
  { std::copy(vec.cbegin(), vec.cend(), std::inserter(sus, sus.end())); };

/*! \brief              Remove an element from a set, map, unordered set or unordered map
    \param  sus         destination set or unordered set
    \param  element     element to remove, or an iterator into <i>sus</i>
*/
template <typename C, typename T>
inline void operator-=(C& sus, const T& element)
  requires (is_sus_v<C> or is_mum_v<C>) and (std::is_same_v<typename C::key_type, base_type<T>> or std::is_same_v<base_type<T>, typename C::iterator>)
  { sus.erase(element); }

/*! \brief              Add an element to a set
    \param  s           destination set
    \param  element     element to insert
*/
template <typename F, typename S>
inline void operator+=(std::set<std::pair<F, S>>& s, const std::pair<F, S>& element)
  { s.insert(element); }

/*! \brief              Add an element to a multiset
    \param  s           destination multiset
    \param  element     element to insert
*/
template <typename F, typename S>
inline void operator+=(std::multiset<std::pair<F, S>>& s, const std::pair<F, S>& element)
  { s.insert(element); }

/*! \brief        insert one MUM/SUS into another
    \param  dest  destination MUM/SUS
    \param  src   source MUM/SUS
    \return       <i>dest</i> with <i>src</i> inserted
*/
template <typename MUMD, typename MUMS>
  requires ( ( (is_mum_v<MUMD>) and (is_mum_v<MUMS>) and
             (std::is_same_v<typename MUMD::key_type, typename MUMS::key_type>) and (std::is_same_v<typename MUMD::mapped_type, typename MUMS::mapped_type>) ) or
             ( (is_sus_v<MUMD>) and (is_sus_v<MUMS>) and (std::is_same_v<typename MUMD::value_type, typename MUMS::value_type>) ) )
inline void operator+=(MUMD& dest, const MUMS& src)
  { dest.insert(src.cbegin(), src.cend()); }

/*! \brief        Append one vector to another
    \param  dest  destination vector
    \param  src   source vector
    \return       <i>dest</i> with <i>src</i> appended
*/
template <typename V>
  requires (is_vector_v<V>)
inline void operator+=(V& dest, V&& src)
  { dest.reserve(dest.size() + src.size());
    dest.insert(dest.end(), src.begin(), src.end());
  }

// too confusing
//template <typename V>
//  requires (is_vector_v<V>)
//void operator+=(VECTOR auto& dest, const VECTOR auto&& src)
//  { dest.reserve(dest.size() + src.size());
//    dest.insert(dest.end(), src.begin(), src.end());
//  }

/*! \brief        Append one vector to another
    \param  dest  destination vector
    \param  src   source vector
    \return       <i>dest</i> with <i>src</i> appended
*/
template <typename V>
  requires (is_vector_v<V>)
inline void operator+=(V& dest, const V& src)
  { dest.reserve(dest.size() + src.size());
    dest.insert(dest.end(), src.begin(), src.end());
  }
//void operator+=(VECTOR auto& dest, const VECTOR auto& src)
//  { dest.reserve(dest.size() + src.size());
//    dest.insert(dest.end(), src.begin(), src.end());
//  }

/*! \brief        Concatenate two vectors
    \param  dest  destination vector
    \param  src   source vector
    \return       <i>dest</i> with <i>src</i> appended
*/
template <typename V> requires (is_vector_v<V>)
V operator+(const V& v1, V&& v2)
{ V rv(v1.size() + v2.size());

  rv = v1;
  rv += std::forward<V>(v2);
  
  return rv;
}

/*! \brief        Concatenate two vectors
    \param  dest  destination vector
    \param  src   source vector
    \return       <i>dest</i> with <i>src</i> appended
*/
template <typename V> requires (is_vector_v<V>)
V operator+(const V& v1, const V& v2)
{ V rv(v1.size() + v2.size());

  rv = v1;
  rv += v2;
  
  return rv;
}

/*! \brief        Add all the elements of a SUS to a vector
    \param  dest  destination vector
    \param  sus   source SUS
    \return       <i>dest</i> with all the elements of <i>sus</i> appended

    Note that in general this means that the vector is subsequently going to be ordered
*/
template <typename V, typename SUS>
  requires (is_vector_v<V>) and (is_sus_v<SUS>) and (std::is_same_v<base_type<typename V::value_type>, base_type<typename SUS::value_type>>)
inline void operator+=(V& vec, const SUS& sus)
  { vec.reserve(vec.size() + sus.size());
    //dest.insert(dest.end(), src.begin(), src.end()); &&&
    COPY_ALL(sus, back_inserter(vec));
  }

/*! \brief              Add an element to a vector
    \param  dest        destination vector
    \param  element     element to append
    \return             <i>dest</i> with <i>element</i> appended
*/
template <typename V, typename E>
auto operator+(const V& v1, E&& element) -> V
  requires is_vector_v<V> and (std::is_same_v<typename V::value_type, base_type<E>>)
{ V rv(v1.size() + 1);

  rv = v1;
  rv.push_back(std::forward<E>(element));
  
  return rv; 
}

/*! \brief             Add an element to a set or unordered set
    \param  sus        initial SUS
    \param  element    element to add
    \return            <i>sus</i> with <i>element</i> added
*/
template <typename S, typename E>
auto operator+(const S& s1, E&& element) -> S
  requires is_sus_v<S> and (std::is_same_v<typename S::value_type, base_type<E>>)
{ S rv { s1 };

  rv.insert(std::forward<E>(element));
  
  return rv; 
}

/*! \brief              Append an element to a deque, list or vector
    \param  c1          destination deque, list or vector
    \param  element     element to append
*/
template <typename C>
inline void operator+=(C& c1, typename C::value_type&& element)
  requires is_deque<C> or is_list_v<C> or is_vector_v<C>
{ c1.push_back(std::forward<typename C::value_type>(element)); }

/*! \brief              Append an element to a deque, list or vector
    \param  c1          destination deque, list or vector
    \param  element     element to append
*/
template <typename C, typename E>
inline void operator+=(C& c1, const E& element)
  requires (is_deque<C> or is_list_v<C> or is_vector_v<C>) and (std::is_convertible_v<E, typename C::value_type>)
{ c1.push_back(element); }

/*! \brief      Insert an element into a list
    \param  l1  destination list
    \param  pr  location and value to insert
*/
template <typename L>
inline void operator+=(L& l1, std::pair<typename L::const_iterator, typename L::value_type>&& pr)
  requires is_list_v<L>
{ l1.insert(std::forward<typename L::const_iterator>(pr.first), std::forward<typename L::value_type>(pr.second)); }

/*! \brief      Insert an element into a list
    \param  l1  destination list
    \param  pr  location and value to insert
*/
template <typename L>
inline void operator+=(L& l1, const std::pair<typename L::const_iterator, typename L::value_type>& pr)
  requires is_list_v<L>
{ l1.insert(pr.first, pr.second); }

/*! \brief              Remove all elements with a particular value from a list
    \param  c1          list
    \param  element     value to remove
*/
template <typename C>
inline void operator-=(C& c1, typename C::value_type&& element)
  requires is_list_v<C>
{ c1.remove(std::forward<typename C::value_type>(element)); }

/*! \brief              Remove all elements with a particular value from a list
    \param  c1          list
    \param  element     value to remove
*/
template <typename C>
inline void operator-=(C& c1, const typename C::value_type& element)
  requires is_list_v<C>
{ c1.remove(element); }

/*! \brief              Append an element to a queue
    \param  q1          destination queue
    \param  element     element to append
*/
template <typename Q>
inline void operator+=(Q& q1, typename Q::value_type&& element)
  requires is_queue_v<Q>
{ q1.push(std::forward<typename Q::value_type>(element)); }

/*! \brief              Append an element to a queue
    \param  q1          destination queue
    \param  element     element to append
*/
template <typename Q, typename E>
inline void operator+=(Q& q1, const E& element)
  requires is_queue_v<Q> and (std::is_convertible_v<E, typename Q::value_type>)
{ q1.push(element); }

/*! \brief              Remove and call destructor on front element of deque 
    \param  D1          destination deque

    Does nothing if the deque is empty
*/
template <typename D>
void operator--(D& d /*, int*/) // int for post-decrement
  requires is_deque<D>
{ if (!d.empty())
    d.pop_front();
}

/*! \brief      Remove an element referenced by an iterator from a deque
    \param  d   destination deque
    \param  it  iterator
*/
template <typename D>
inline void operator-=(D& d, typename D::iterator&& it)
  requires is_deque<D>
{ d.erase(std::forward<typename D::iterator>(it)); }

/*! \brief      Remove an element referenced by an iterator from a deque
    \param  c1  destination deque
    \param  it  iterator
*/
template <typename D>
inline void operator-=(D& d, const typename D::iterator& it)
  requires is_deque<D>
{ d.erase(it); }

/*! \brief              Is an element in a deque, list or vector?
    \param  c           the deque, list or vector
    \param  element     the value to test
    \return             whether <i>element</i> is an element of <i>v</i>
*/
template <typename C, typename E>
  requires (is_deque<C> or is_list_v<C> or is_vector_v<C>) and (std::is_convertible_v<E, typename C::value_type>)
inline bool contains(const C& c, const E& element)
  { return std::ranges::find(c, element) != c.cend(); }

/*! \brief        Obtain a view of a range of numbers
    \param  u     the lowest value in the range
    \param  v     one more than the highest value in the range
    \return       a view of the values from <i>u</i> to <i>v-1</i>
*/
template <typename T = int, typename U, typename V>
auto RANGE(const U u, const V v)
  { return std::ranges::iota_view { static_cast<T>(u), static_cast<T>(v) }; }

#if 0
template <typename T = int>
//ranges::iota_view { 0u, call.length() - 2 } )
auto RANGE(std::initializer_list<T> l)
  { return std::ranges::iota_view { static_cast<T>(l.data[0]), static_cast<T>(l.data[1]) }; }
#endif

/*! \brief          Find the first element in a container that matches a predicate, or return the default-constructed element if none match
    \param  c       the container
    \param  pred    predicate to apply
    \return         first element in <i>c<i> for which <i>pred</i> is true; if none, then a default-constructed element
*/
template <class C, class UnaryPredicate>
  requires is_list_v<C>
typename C::value_type VALUE_IF(const C& c, UnaryPredicate pred)
  { const auto cit { FIND_IF(c, pred) };

    return ( (cit == c.end()) ? typename C::value_type {} : *cit );
  }

/*! \brief          Remove all the elements in a container from another container
    \param  c1      the container that may be modified
    \param  c2      the container of elements to be removed from <i>c1</i>

    It is permissible for <i>c2</i> to contain elements that are not present in <i>c1</i>
*/
template <typename C>
inline void operator-=(C& c1, C c2)
  { FOR_ALL(c2, [&c1] (const C::value_type& v) { c1 -= v; }); }

#endif    // MACROS_H
