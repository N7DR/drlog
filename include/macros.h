// $Id: macros.h 68 2014-06-28 15:42:35Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef MACROS_H
#define MACROS_H

/*!     \file macros.h

        Macros and templates for drlog.
*/

#include "bands-modes.h"
#include "serialization.h"

#include <algorithm>
#include <tuple>

#if (!defined(READ_AND_WRITE))

/// Syntactic sugar for read/write access
#define READ_AND_WRITE(y) \
/*! Read access to _##y */ \
  inline const decltype(_##y)& y(void) const { return _##y; } \
/*! Write access to _##y */ \
  inline void y(const decltype(_##y)& n) { _##y = n; }

#endif    // !READ_AND_WRITE

#if (!defined(SAFE_READ_AND_WRITE))

/// Syntactic sugar for read/write access
#define SAFE_READ_AND_WRITE(y, z) \
/*! Read access to _##y */ \
  inline const decltype(_##y)& y(void) const { SAFELOCK(z); return _##y; } \
/*! Write access to _##y */ \
  inline void y(const decltype(_##y)& n) { SAFELOCK(z); _##y = n; }

#endif    // !SAFE_READ_AND_WRITE

#if (!defined(READ))

/// Syntactic sugar for read-only access
#define READ(y) \
/*! Read-only access to _##y */ \
  inline const decltype(_##y)& y(void) const { return _##y; }

#endif    // !READ

#if (!defined(SAFEREAD))

/// Syntactic sugar for read-only access
#define SAFEREAD(y, z) \
/*! Read-only access to _##y */ \
  inline const decltype(_##y)& y(void) const { SAFELOCK(z); return _##y; }

#endif    // !SAFEREAD

// classes for tuples... it seems like there should be a way to do this with TMP,
// but the level-breaking caused by the need to control textual names seems to make
// this impossible without resorting to #defines

/// tuple class (1) -- complete overkill
#define WRAPPER_1(nm, a0, a1)                          \
                                                                          \
class nm : public std::tuple < a0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X )                                                       \
    { std::get<0>(*this) = X;                                                    \
    }                                                                     \
                                                                          \
  inline const a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(const a0 & var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
                                                                           \
                                                                          \
}

/// tuple class (2)
#define WRAPPER_2(nm, a0, a1, b0, b1)                          \
                                                                          \
class nm : public std::tuple < a0, b0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
    }                                                                     \
                                                                          \
  inline const a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(const a0 & var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline const b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(const b0 & var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
                                                                          \
}

/// tuple class (2)
#define WRAPPER_2_NC(nm, a0, a1, b0, b1)                          \
                                                                          \
class nm : public std::tuple < a0, b0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
    }                                                                     \
                                                                          \
  inline a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
                                                                          \
}

/// tuple class (3)
#define WRAPPER_3(nm, a0, a1, b0, b1, c0, c1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
    }                                                                     \
                                                                          \
                                                                          \
  nm( void ) { }                                                          \
                                                                          \
  inline const a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline const b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline const c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
                                                                          \
}

/// tuple class (3)
#define WRAPPER_3_NC(nm, a0, a1, b0, b1, c0, c1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
    }                                                                     \
                                                                          \
  inline a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
                                                                          \
}

// tuple class (3)
#define WRAPPER_3_SERIALIZE(nm, a0, a1, b0, b1, c0, c1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
    }                                                                     \
                                                                          \
                                                                          \
  nm( void ) { }                                                          \
                                                                          \
  inline const a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline const b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline const c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
    template<typename Archive>                                   \
  void serialize(Archive& ar, const unsigned version)           \
    { ar & std::get<0>(*this)                                                  \
         & std::get<1>(*this)                                                   \
         & std::get<2>(*this);                                                  \
    }                                                           \
                                                                          \
}

/// tuple class (4)
#define WRAPPER_4_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0, d0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z, d0 A)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
      std::get<3>(*this) = A;                                                    \
}                                                                     \
                                                                          \
  inline a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
  inline d0 d1(void) const                                                    \
    { return std::get<3>(*this); }                              \
                                                                          \
  inline void d1(d0 var)                                          \
    { std::get<3>(*this) = var; }                               \
                                                                          \
                                                                          \
}

// tuple class (4)
#define WRAPPER_4_SERIALIZE(nm, a0, a1, b0, b1, c0, c1, d0, d1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0, d0  >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z, d0 A)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
      std::get<3>(*this) = A;                                                    \
}                                                                     \
                                                                          \
                                                                          \
  nm( void ) { }                                                          \
                                                                          \
  inline const a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline const b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline const c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
  inline const d0 d1(void) const                                                    \
    { return std::get<3>(*this); }                              \
                                                                          \
  inline void d1(d0 var)                                          \
    { std::get<3>(*this) = var; }                               \
                                                                          \
template<typename Archive>                                   \
  void serialize(Archive& ar, const unsigned version)           \
    { ar & std::get<0>(*this)                                                  \
         & std::get<1>(*this)                                                   \
         & std::get<2>(*this)                                                  \
         & std::get<3>(*this);                                                  \
    }                                                           \
                                                                          \
}

/// tuple class (5)
#define WRAPPER_5_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1, e0, e1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0, d0, e0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z, d0 A, e0 B)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
      std::get<3>(*this) = A;                                                    \
      std::get<4>(*this) = B;                                                    \
    }                                                                     \
                                                                          \
  nm(void) { }                                                            \
                                                                          \
  inline a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
  inline d0 d1(void) const                                                    \
    { return std::get<3>(*this); }                              \
                                                                          \
  inline void d1(d0 var)                                          \
    { std::get<3>(*this) = var; }                               \
                                                                           \
  inline e0 e1(void) const                                                    \
    { return std::get<4>(*this); }                              \
                                                                          \
  inline void e1(e0 var)                                          \
    { std::get<4>(*this) = var; }                               \
                                                                         \
                                                                          \
}

/// tuple class (6)
#define WRAPPER_6_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1, e0, e1, f0, f1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0, d0, e0, f0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z, d0 A, e0 B, f0 C)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
      std::get<3>(*this) = A;                                                    \
      std::get<4>(*this) = B;                                                    \
      std::get<5>(*this) = C;                                                    \
}                                                                     \
                                                                          \
  inline a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
  inline d0 d1(void) const                                                    \
    { return std::get<3>(*this); }                              \
                                                                          \
  inline void d1(d0 var)                                          \
    { std::get<3>(*this) = var; }                               \
                                                                           \
  inline e0 e1(void) const                                                    \
    { return std::get<4>(*this); }                              \
                                                                          \
  inline void e1(e0 var)                                          \
    { std::get<4>(*this) = var; }                               \
                                                                           \
  inline f0 f1(void) const                                                    \
    { return std::get<5>(*this); }                              \
                                                                          \
  inline void f1(f0 var)                                          \
    { std::get<5>(*this) = var; }                               \
                                                                         \
                                                                          \
}

/// tuple class (7)
#define WRAPPER_7_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1, e0, e1, f0, f1, g0, g1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0, d0, e0, f0, g0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z, d0 A, e0 B, f0 C, g0 D)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
      std::get<3>(*this) = A;                                                    \
      std::get<4>(*this) = B;                                                    \
      std::get<5>(*this) = C;                                                    \
      std::get<6>(*this) = D;                                                    \
}                                                                     \
                                                                          \
  inline a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
  inline d0 d1(void) const                                                    \
    { return std::get<3>(*this); }                              \
                                                                          \
  inline void d1(d0 var)                                          \
    { std::get<3>(*this) = var; }                               \
                                                                           \
  inline e0 e1(void) const                                                    \
    { return std::get<4>(*this); }                              \
                                                                          \
  inline void e1(e0 var)                                          \
    { std::get<4>(*this) = var; }                               \
                                                                           \
  inline f0 f1(void) const                                                    \
    { return std::get<5>(*this); }                              \
                                                                          \
  inline void f1(f0 var)                                          \
    { std::get<5>(*this) = var; }                               \
                                                                           \
  inline g0 g1(void) const                                                    \
    { return std::get<6>(*this); }                              \
                                                                          \
  inline void g1(g0 var)                                          \
    { std::get<6>(*this) = var; }                               \
                                                                         \
                                                                          \
}

/// tuple class (8)
#define WRAPPER_8_NC(nm, a0, a1, b0, b1, c0, c1, d0, d1, e0, e1, f0, f1, g0, g1, h0, h1)                   \
                                                                          \
class nm : public std::tuple < a0, b0, c0, d0, e0, f0, g0, h0 >                                        \
{                                                                         \
protected:                                                                \
                                                                          \
public:                                                                   \
                                                                          \
  nm( a0 X, b0 Y, c0 Z, d0 A, e0 B, f0 C, g0 D, h0 E)                                                       \
    { std::get<0>(*this) = X;                                                    \
      std::get<1>(*this) = Y;                                                    \
      std::get<2>(*this) = Z;                                                    \
      std::get<3>(*this) = A;                                                    \
      std::get<4>(*this) = B;                                                    \
      std::get<5>(*this) = C;                                                    \
      std::get<6>(*this) = D;                                                    \
      std::get<7>(*this) = E;                                                    \
}                                                                     \
                                                                          \
  inline a0 a1(void) const                                                     \
    { return std::get<0>(*this); }                              \
                                                                          \
  inline void a1(a0 var)                                          \
    { std::get<0>(*this) = var; }                               \
                                                                          \
  inline b0 b1(void) const                                                    \
    { return std::get<1>(*this); }                              \
                                                                          \
  inline void b1(b0 var)                                          \
    { std::get<1>(*this) = var; }                               \
                                                                          \
  inline c0 c1(void) const                                                    \
    { return std::get<2>(*this); }                              \
                                                                          \
  inline void c1(c0 var)                                          \
    { std::get<2>(*this) = var; }                               \
                                                                          \
  inline d0 d1(void) const                                                    \
    { return std::get<3>(*this); }                              \
                                                                          \
  inline void d1(d0 var)                                          \
    { std::get<3>(*this) = var; }                               \
                                                                           \
  inline e0 e1(void) const                                                    \
    { return std::get<4>(*this); }                              \
                                                                          \
  inline void e1(e0 var)                                          \
    { std::get<4>(*this) = var; }                               \
                                                                           \
  inline f0 f1(void) const                                                    \
    { return std::get<5>(*this); }                              \
                                                                          \
  inline void f1(f0 var)                                          \
    { std::get<5>(*this) = var; }                               \
                                                                           \
  inline g0 g1(void) const                                                    \
    { return std::get<6>(*this); }                              \
                                                                          \
  inline void g1(g0 var)                                          \
    { std::get<6>(*this) = var; }                               \
                                                                           \
  inline h0 h1(void) const                                                    \
    { return std::get<7>(*this); }                              \
                                                                          \
  inline void h1(h0 var)                                          \
    { std::get<7>(*this) = var; }                               \
                                                                         \
                                                                          \
}

#include <set>

template <class T>
const bool operator<(const std::set<T>& s, const T& v)
  { return s.find(v) != s.cend(); }

#include <unordered_set>

template <class T>
const bool operator<(const std::unordered_set<T>& s, const T& v)
  { return s.find(v) != s.cend(); }

#include <map>

#include "pthread_support.h"

template <class T>
const T SAFELOCK_GET(pt_mutex& m, const T& v)
{ safelock safelock_z(m, "SAFELOCK_GET");

  return v;
}

template <class T>
void SAFELOCK_SET(pt_mutex& m, T& var, const T& val)
{ safelock safelock_z(m, "SAFELOCK_SET");

  var = val;
}

template <typename M>  // M = map<T, set<T> >
auto INVERT_MAPPING(const M& original_mapping) -> std::map<typename M::key_type, typename M::key_type>
{ std::map<typename M::key_type, typename M::key_type> rv;

  for (auto cit = original_mapping.cbegin(); cit != original_mapping.cend(); ++cit)
  { for (const auto& p : cit->second)
      rv.insert( { p, cit->first } );
  }

  return rv;
}

#include <chrono>

typedef std::chrono::duration<long, std::centi> centiseconds;
typedef std::chrono::duration<long, std::deci>  deciseconds;

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

template <typename T>
class RANGE : public std::vector<T>
{
public:

  RANGE(const T& v1, const T& v2)
  { if (v1 > v2)
    { T value = v1;

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

template<class Input, class Function>
  Function FOR_ALL(Input& first, Function fn)
{ return (std::for_each(first.begin(), first.end(), fn));
}

template<class Input, class OutputIterator>
  OutputIterator COPY_ALL(Input& first, OutputIterator oi)
{ return (copy(first.begin(), first.end(), oi));
}

template <class Input, class UnaryPredicate>
  void REMOVE_IF_AND_RESIZE(Input& first, UnaryPredicate pred)
{ first.erase(remove_if(first.begin(), first.end(), pred), first.end());
}

// ------------------------ container for per-band and per-mode information ------------

#include <array>

template <typename T>
class BAND_MODE_CONTAINER
{
protected:

  typedef std::array<T, N_BANDS> _ROW;

  std::array<_ROW, N_MODES>                        _data;

public:

  inline const std::array<T, N_BANDS> band_data(const MODE m)
    { return _data[static_cast<int>(m)]; }

  const std::array<T, N_MODES> mode_data(const BAND b)
    { std::array<T, N_MODES> rv;

      for (int n = 0; n < N_MODES; ++n)
      {  rv[n] = _data[n][static_cast<int>(b)];
      }

      return rv;
    }

  inline T& operator()(const BAND b, const MODE m)
  { return _data[static_cast<int>(m)][static_cast<int>(b)];
  }

/// allow archiving
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _data;
    }
};

#endif    // MACROS_H
