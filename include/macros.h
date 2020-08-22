// $Id: macros.h 166 2020-08-22 20:59:30Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef MACROS_H
#define MACROS_H

#undef NEW_RAW_COMMAND  // experiment to try to improve the reliability of the awful rig interface

/*! \file   macros.h

    Macros and templates for drlog.
*/

#include "serialization.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

#include <cmath>

// convenient definitions for use with chrono functions
using centiseconds = std::chrono::duration<long, std::centi>;
using deciseconds = std::chrono::duration<long, std::deci>;

/// Syntactic sugar for read/write access
#if (!defined(READ_AND_WRITE))

#define READ_AND_WRITE(y)                                                    \
/*! Read access to _##y */                                                   \
  [[nodiscard]] inline decltype(_##y) y(void) const { return _##y; }  \
/*! Write access to _##y */                                                  \
  inline void y(const decltype(_##y)& n) { _##y = n; }

#endif    // !READ_AND_WRITE

/// Syntactic sugar for read/write access with thread locking
#if (!defined(SAFE_READ_AND_WRITE))

#define SAFE_READ_AND_WRITE(y, z)                                                  \
/*! Read access to _##y */                                                         \
  [[nodiscard]] inline decltype(_##y) y(void) const { SAFELOCK(z); return _##y; }  \
/*! Write access to _##y */                                                        \
  inline void y(const decltype(_##y)& n) { SAFELOCK(z); _##y = n; }

#endif    // !SAFE_READ_AND_WRITE

/// Read and write with mutex that is part of the object
#if (!defined(SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX))

#define SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX(y, z)                       \
/*! Read access to _##y */                                                  \
  [[nodiscard]] inline decltype(_##y) y(void) { SAFELOCK(z); return _##y; } \
/*! Write access to _##y */                                                 \
  inline void y(const decltype(_##y)& n) { SAFELOCK(z); _##y = n; }

#endif    // !SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX

/// Syntactic sugar for read-only access
#if (!defined(READ))

#define READ(y)                                                       \
/*! Read-only access to _##y */                                       \
  [[nodiscard]] inline decltype(_##y) y(void) const { return _##y; }

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

// https://stackoverflow.com/questions/12042824/how-to-write-a-type-trait-is-container-or-is-vector
// https://wandbox.org/permlink/D6Nf3Sb7PHjP6SrN
template<class T>
struct is_int 
  { constexpr static bool value { false }; };

template<>
struct is_int<int> 
  { constexpr static bool value { true }; };

template<class T>
struct is_map 
  { constexpr static bool value { false }; };

template<class K, class V>
struct is_map<std::map<K, V>> 
  { constexpr static bool value { true }; };

template<class T>
struct is_set 
  { constexpr static bool value { false }; };

template<class T>
struct is_set<std::set<T>> 
  { constexpr static bool value { true }; };

template<class T>
struct is_unordered_map 
  { constexpr static bool value { false }; };

template<class K, class V>
struct is_unordered_map<std::unordered_map<K, V>> 
  { constexpr static bool value { true }; };

template<class T>
struct is_unordered_set 
  { constexpr static bool value { false }; };

template<class T>
struct is_unordered_set<std::unordered_set<T>> 
  { constexpr static bool value { true }; };

template<class T>
struct is_unsigned_int 
  { constexpr static bool value { false }; };

template<>
struct is_unsigned_int<unsigned int> 
  { constexpr static bool value { true }; };

template<class T>
struct is_vector 
  { constexpr static bool value { false }; };

template<class T>
struct is_vector<std::vector<T>> 
  { constexpr static bool value { true }; };
  
// current g++ does not support definition of concepts, even with -fconcepts !!
//template<typename T>
//concept SET_TYPE = requires(T a) 
//{ is_set<T>::value == true;
//    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
//};


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
  nm( a0 X )                                           \
    { std::get<0>(*this) = X;                          \
    }                                                  \
                                                       \
  inline /* const */ a0 a1(void) const                       \
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
  inline /* const */ a0 a1(void) const                       \
    { return std::get<0>(*this); }                     \
                                                       \
  inline void a1(const a0 & var)                       \
    { std::get<0>(*this) = var; }                      \
                                                       \
  inline /* const */ b0 b1(void) const                       \
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
  nm( void ) { }                                       \
                                                       \
  inline /* const */ a0 a1(void) const                       \
    { return std::get<0>(*this); }                     \
                                                       \
  inline void a1(a0 var)                               \
    { std::get<0>(*this) = var; }                      \
                                                       \
  inline /* const */ b0 b1(void) const                       \
    { return std::get<1>(*this); }                     \
                                                       \
  inline void b1(b0 var)                               \
    { std::get<1>(*this) = var; }                      \
                                                       \
  inline /* const */ c0 c1(void) const                       \
    { return std::get<2>(*this); }                     \
                                                       \
  inline void c1(c0 var)                               \
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
  nm( void ) { }                                                            \
                                                                            \
  inline /* const */ a0 a1(void) const                                            \
    { return std::get<0>(*this); }                                          \
                                                                            \
  inline void a1(a0 var)                                                    \
    { std::get<0>(*this) = var; }                                           \
                                                                            \
  inline /* const */ b0 b1(void) const                                            \
    { return std::get<1>(*this); }                                          \
                                                                            \
  inline void b1(b0 var)                                                    \
    { std::get<1>(*this) = var; }                                           \
                                                                            \
  inline /* const */ c0 c1(void) const                                            \
    { return std::get<2>(*this); }                                          \
                                                                            \
  inline void c1(c0 var)                                                    \
    { std::get<2>(*this) = var; }                                           \
                                                                            \
    template<typename Archive>                                              \
  void serialize(Archive& ar, const unsigned version)                       \
    { ar & std::get<0>(*this)                                               \
         & std::get<1>(*this)                                               \
         & std::get<2>(*this);                                              \
    }                                                                       \
                                                                            \
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
}                                                                           \
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
  nm( void ) { }                                                            \
                                                                            \
  inline /* const */ a0 a1(void) const                                            \
    { return std::get<0>(*this); }                                          \
                                                                            \
  inline void a1(a0 var)                                                    \
    { std::get<0>(*this) = var; }                                           \
                                                                            \
  inline /* const */ b0 b1(void) const                                            \
    { return std::get<1>(*this); }                                          \
                                                                            \
  inline void b1(b0 var)                                                    \
    { std::get<1>(*this) = var; }                                           \
                                                                            \
  inline /* const */ c0 c1(void) const                                            \
    { return std::get<2>(*this); }                                          \
                                                                            \
  inline void c1(c0 var)                                                    \
    { std::get<2>(*this) = var; }                                           \
                                                                            \
  inline /* const */ d0 d1(void) const                                            \
    { return std::get<3>(*this); }                                          \
                                                                            \
  inline void d1(d0 var)                                                    \
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
  nm(void) { }                                                              \
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

/*! \brief      Is an object a member of a set or unordered_set?
    \param  s   set or unordered_set  to be tested
    \param  v   object to be tested for membership
    \return     Whether <i>t</i> is a member of <i>s</i>
*/
template <class T, class U>
bool operator>(const T& s, const U& v)
  requires (is_set<T>::value == true || is_unordered_set<T>::value == true) && (std::is_same<typename T::value_type, U>::value == true)
  { return s.find(v) != s.cend(); }

/*! \brief      Is an object a key of a map or unordered_map, and if so return the value (or the default-constructed value)
    \param  m   map or unordered_map to be searched
    \param  k   target key
    \return     Whether <i>k</i> is a member of <i>m</i> and, if so the corresponding value (or the default-constructed value)
    
    // possibly should return variant instead
*/
template <class M, class K>
std::pair<bool, typename M::mapped_type> operator>(const M& m, const K& k)
  requires (is_map<M>::value == true || is_unordered_map<M>::value == true) && std::is_same<typename M::key_type, K>::value == true && std::is_default_constructible<typename M::mapped_type>::value == true
{ using V = typename M::mapped_type;
  using RT = std::pair<bool, typename M::mapped_type>;

  const auto cit { m.find(k) };

  return ( (cit == m.cend()) ? RT { false, V() } : RT { true, cit->second } );
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
  requires (is_map<C>::value == true || is_unordered_map<C>::value == true) && std::is_same<typename C::key_type, K>::value == true /* && std::is_default_constructible<typename C::mapped_type>::value == true */
{ const auto cit { m.find(k) };

  return ( (cit == m.cend()) ? d : cit->second );
}

/*! \brief                      Invert a mapping from map<T, set<T> > to map<T, set<T> >, where final keys are the elements of the original set
    \param  original_mapping    original mapping
    \return                     inverted mapping
*/
template <typename M>  // M = map<T, set<T> >
auto INVERT_MAPPING(const M& original_mapping) -> std::map<typename M::key_type, typename M::key_type>
{ std::map<typename M::key_type, typename M::key_type> rv;

  for (auto cit = original_mapping.cbegin(); cit != original_mapping.cend(); ++cit)
  { for (const auto& p : cit->second)
      rv.insert( { p, cit->first } );
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

/*! \class  RANGE
    \brief  allow easy execution of a loop a predetermined number of times

    This does throw a warning, but I can't think of a better way to
    execute a loop a predetermined number of times. C++14 might be going
    to provide a better mechanism.

    See also the UNUSED template below
*/
template <typename T>
class RANGE : public std::vector<T>
{
public:

/*! \brief      Generate a range
    \param  v1  lowest value
    \param  v2  highest value
*/
  RANGE(const T& v1, const T& v2)
  { if (v1 > v2)
    { T value { v1 };

      while (value != v2)
      { this->push_back(value--);
      }

      this->push_back(v2);
    }
    else
    { for (T value = v1; value <= v2; ++value)
        this->push_back(value);
    }
  }
};

/// Syntactic sugar to avoid the "unused variable" warning when using the RANGE template, until C++ provides a proper way to have unused range-based loop variables
template<typename Unused>
inline void UNUSED( Unused&& )
  { }

/*! \class  accumulator
    \brief  accumulate values, and inform when a threshold is reached
*/
template <typename T>
class accumulator
{
protected:

  std::map<T, unsigned int> _values;                ///< all the known values, with the number of times it's been added
  unsigned int              _threshold;             ///< threshold value

public:

/// default constructor
  accumulator(const unsigned int thold = 1) :
    _threshold(thold)
  { }

  READ_AND_WRITE(threshold);                        ///< threshold value

/*! \brief          Add a value or increment it a known number of times
    \param  val     value to add or increment
    \param  n       number of times to add it
    \return         whether final number of times <i>value</i> has been added is at or greater than the threshold
*/
  bool add(const T& val, const int n = 1)
  { if (_values.find(val) == _values.end())
      _values.insert( { val, n } );
    else
      _values[val] += n;

    return (_values[val] >= _threshold);
  }

/*! \brief          Number of times a value has been added
    \param  val     target value
    \return         total number of times <i>value</i> has been added
*/
  unsigned int value(const T& val) const
    { return ( ( _values.find(val) == _values.cend() ) ? 0 : _values.at(val) ); }
};

// convenient syntactic sugar for some STL functions

/*! \brief          Write a <i>map<key, value></i> object to an output stream
    \param  ost     output stream
    \param  mp      object to write
    \return         the output stream
*/
template <class T1, class T2>
std::ostream& operator<<(std::ostream& ost, const std::map<T1, T2>& mp)
{ for (typename std::map<T1, T2>::const_iterator cit = mp.begin(); cit != mp.end(); ++cit)
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
{ for (typename std::unordered_map<T1, T2>::const_iterator cit = mp.begin(); cit != mp.end(); ++cit)
    ost << "unordered_map[" << cit->first << "]: " << cit->second << std::endl;

  return ost;
}

/*! \brief          Apply a function to all in a container
    \param  first   container
    \param  fn      function
    \return         <i>fn</i>
*/
template<class Input, class Function>
inline Function FOR_ALL(Input& first, Function fn)
  { return (std::for_each(first.begin(), first.end(), fn)); }

/*! \brief          Apply a function to all in a container
    \param  first   container
    \param  fn      function
    \return         <i>fn</i>
*/
template<class Input, class Function>
inline Function FOR_ALL(const Input& first, Function fn)
  { return (std::for_each(first.cbegin(), first.cend(), fn)); }

/*! \brief          Copy all in a container to another container
    \param  first   initial container
    \param  oi      iterator on final container
    \return         <i>oi</i>
*/
template<class Input, class OutputIterator>
inline OutputIterator COPY_ALL(Input& first, OutputIterator oi)
  { return (std::copy(first.begin(), first.end(), oi)); }

/*! \brief          Remove values in a container that match a predicate, and resize the container
    \param  first   container
    \param  pred    predicate to apply

    Does not work for maps
*/
template <class Input, class UnaryPredicate>
inline void REMOVE_IF_AND_RESIZE(Input& first, UnaryPredicate pred)
  { first.erase(std::remove_if(first.begin(), first.end(), pred), first.end()); }

// https://stackoverflow.com/questions/800955/remove-if-equivalent-for-stdmap
// there should be some way to choose this function instead of the prior one, based on
// traits, but I can't figure out a way to tell whether T is a map
//template< typename ContainerT, typename PredicateT >
//void MAP_REMOVE_IF( ContainerT& items, const PredicateT& predicate ) {
//  for( auto it = items.begin(); it != items.end(); ) {
//    if( predicate(*it) ) it = items.erase(it);
//    else ++it;
//  }
//};

/*! \brief          Remove map values that match a predicate, and resize the map
    \param  items   map
    \param  pred    predicate to apply

    Does not work for maps
*/
template< typename K, typename V, typename PredicateT >
void REMOVE_IF_AND_RESIZE( std::map<K, V>& items, const PredicateT& pred )
{ for( auto it = items.begin(); it != items.end(); )
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
  { std::reverse(v.begin(), v.end()); }

/*! \brief          Find first value in a container that matches a predicate
    \param  v       container
    \param  pred    (boolean) predicate to apply
    \return         first value in <i>v</i> for which <i>pred</i> is true
*/
template <typename Input, typename UnaryPredicate>
inline auto FIND_IF(Input& v, UnaryPredicate pred) -> typename Input::iterator
  { return std::find_if(v.begin(), v.end(), pred); }

/*! \brief          Find first value in a container that matches a predicate
    \param  v       container (const)
    \param  pred    (boolean) predicate to apply
    \return         first value in <i>v</i> for which <i>pred</i> is true
*/
template <typename Input, typename UnaryPredicate>
inline auto FIND_IF(const Input& v, UnaryPredicate pred) -> typename Input::const_iterator
  { return std::find_if(v.cbegin(), v.cend(), pred); }

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

#endif    // MACROS_H
