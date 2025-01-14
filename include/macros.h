// $Id: macros.h 258 2024-12-16 16:29:04Z  $

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
#include <ranges>
#include <set>
#include <tuple>
#include <unordered_set>

#include <cmath>

// convenient definitions for use with chrono functions
using centiseconds = std::chrono::duration<long, std::centi>;
using deciseconds = std::chrono::duration<long, std::deci>;

using TIME_POINT = std::chrono::time_point<std::chrono::system_clock>;

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

// R/W for strings that should be accessible via string_view
#define READ_AND_WRITE_STR(y)                                       \
/*! Read access to _##y */                                      \
  [[nodiscard]] inline const decltype(_##y)& y(void) const& { return _##y; }    \
  [[nodiscard]] inline decltype(_##y) y(void) && { return std::move(_##y); }    \
\
/*! Write access to _##y */                                     \
  inline void y(const decltype(_##y)& n) { _##y = n; } \
  inline void y(const std::string_view n_sv) { _##y = n_sv; }

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

#define WRITE_STR(y)                                       \
/*! Write access to _##y, where its stype is a std::string */                            \
  inline void y(const decltype(_##y)& n) { _##y = n; } \
  inline void y(const std::string_view n) { _##y = n; }

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

// see: https://stackoverflow.com/questions/51032671/idiomatic-way-to-write-concept-that-says-that-type-is-a-stdvector

template <class, template<class...> class>
inline constexpr bool is_specialization = false;

template <template<class...> class T, class... Args>
inline constexpr bool is_specialization<T<Args...>, T> = true;

// arrays are different; https://stackoverflow.com/questions/62940931/c20-concepts-and-templates-fixe-size-and-resizable-array
template<class A>
inline constexpr bool is_stdarray = false;

template<class T, std::size_t I>
inline constexpr bool is_stdarray<std::array<T, I>> = true;

// basic types + string
template <class T> concept is_int         = std::is_same_v<T, int>;
template <class T> concept is_uint        = std::is_same_v<T, unsigned int>;
template <class T> concept is_string      = std::is_same_v<base_type<T>, std::string>;     // is_same<> includes const and volatile qualifiers
template <class T> concept is_string_view = std::is_same_v<base_type<T>, std::string_view>;

// standard containers
template <class T> concept is_array              = is_stdarray<T>;
template <class T> concept is_deque              = is_specialization<T, std::deque>;
template <class T> concept is_list               = is_specialization<T, std::list>;
template <class T> concept is_map                = is_specialization<T, std::map>;
template <class T> concept is_multimap           = is_specialization<T, std::multimap>;
template <class T> concept is_multiset           = is_specialization<T, std::multiset>;
template <class T> concept is_queue              = is_specialization<T, std::queue>;
template <class T> concept is_set                = is_specialization<T, std::set>;
template <class T> concept is_unordered_map      = is_specialization<T, std::unordered_map>;
template <class T> concept is_unordered_multimap = is_specialization<T, std::unordered_multimap>;
template <class T> concept is_unordered_multiset = is_specialization<T, std::unordered_multiset>;
template <class T> concept is_unordered_set      = is_specialization<T, std::unordered_set>;
template <class T> concept is_vector             = is_specialization<T, std::vector>;

// combination concepts
template <class T> concept is_mum   = is_map<T>      or is_unordered_map<T>;
template <class T> concept is_mmumm = is_multimap<T> or is_unordered_multimap<T>;
template <class T> concept is_sus   = is_set<T>      or is_unordered_set<T>;
template <class T> concept is_ssuss = is_multiset<T> or is_unordered_multiset<T>;
template <class T> concept is_ssv   = is_string<T>   or is_string_view<T>;

// combinations of combinations
template <class T> concept ANYSET = is_sus<T> or is_ssuss<T>;

//template <class T> concept is_container_of_strings = (is_sus<T> or is_ssuss<T> or is_vector<T> or is_list<T>) and is_string<typename T::value_type>;
template <class T> concept is_container_of_strings = (is_sus<T> or is_ssuss<T> or is_vector<T> or is_list<T>) and is_ssv<typename T::value_type>;

// heterogeneous lookup for strings
// https://schneide.blog/2024/10/23/heterogeneous-lookup-in-unordered-c-containers/
struct stringly_hash
{ using is_transparent = void;

  [[nodiscard]] inline size_t operator()(const char* rhs) const
    { return std::hash<std::string_view>{}(rhs); }

  [[nodiscard]] inline size_t operator()(const std::string_view rhs) const
    { return std::hash<std::string_view>{}(rhs); }

  [[nodiscard]] inline size_t operator()(const std::string& rhs) const
    { return std::hash<std::string>{}(rhs); }
};

// make the versions with heterogeneous lookup easily available

// heterogenous lookup for unordered maps with string keys
template <typename ValueType>
using UNORDERED_STRING_MAP = std::unordered_map<std::string, ValueType, stringly_hash, std::equal_to<>>;

// heterogenous lookup for unordered multimaps with string keys
template <typename ValueType>
using UNORDERED_STRING_MULTIMAP = std::unordered_multimap<std::string, ValueType, stringly_hash, std::equal_to<>>;

// heterogenous lookup for unordered sets of strings
using UNORDERED_STRING_SET = std::unordered_set<std::string, stringly_hash, std::equal_to<>>;

// heterogenous lookup for ordered maps with string keys
template <typename ValueType>
using STRING_MAP = std::map<std::string, ValueType, std::less<>>;

// heterogenous lookup for multimaps with string keys
template <typename ValueType>
using STRING_MULTIMAP = std::multimap<std::string, ValueType, std::less<>>;

// heterogenous lookup for ordered sets of strings
using STRING_SET = std::set<std::string, std::less<>>;

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

/*! \brief      Does a vector contain a particular member?
    \param  v   vector to be tested
    \param  e   object to be tested for membership
    \return     Whether <i>e</i> is a member of <i>v</i>
*/
template <class T, class U>
  requires (is_vector<T>) and (std::is_same_v<typename T::value_type, U>)
inline bool operator>(const T& v, const U& e)
  { return (std::find(v.cbegin(), v.cend(), e) != v.cend() ); }

/*! \brief      Does a set or unordered_set contains a particular member?
    \param  s   set or unordered_set  to be tested
    \param  v   object to be tested for membership
    \return     Whether <i>v</i> is a member of <i>s</i>
*/
template <class T, class U>
  requires (is_sus<T>) and (std::is_same_v<typename T::value_type, U>)
inline bool operator>(const T& s, const U& v)
  { return s.contains(v); }

/*! \brief      Is an object a member of a set or unordered_set?
    \param  v   object to be tested for membership
    \param  s   set or unordered_set  to be tested
    \return     Whether <i>v</i> is a member of <i>s</i>
*/
template <class E, class S>
  requires (is_sus<S>) and (std::is_same_v<typename S::value_type, E>)
inline bool operator<(const E& v, const S& s)
{ return (s > v); }

/*! \brief      Union of two sets of the same type
    \param  s1  first set
    \param  s2  second set
    \return     union of <i>s1</i> and <i>s2</i>
*/
template<class T>
  requires (is_set<T>)
T operator+(const T& s1, const T& s2)
{ T rv;

  std::set_union(s1.cbegin(), s1.cend(), s2.cbegin(), s2.cend(), std::inserter(rv, rv.end()));

  return rv;
}

/*! \brief      Is an object a key of a map or unordered map; if so return the value, otherwise return a provided default
    \param  m   map or unordered map to be searched
    \param  k   target key
    \param  d   default
    \return     if <i>k</i> is a member of <i>m</i>, the corresponding value, otherwise the default

    The difference between this and ">" is that this does not tell you whether the key was found
*/
template <class C, class K>
  requires (is_mum<C>) and (std::is_constructible_v<typename C::key_type, K>)
typename C::mapped_type MUM_VALUE(const C& m, const K& k, const typename C::mapped_type& d = typename C::mapped_type())
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
  requires (is_mum<C>) and (std::is_constructible_v<typename C::key_type, K>)
auto MUMF_VALUE(const C& m, const K& k, PF pf, RT d = RT { } ) -> RT
{ const auto cit { m.find(k) };

  return ( (cit == m.cend()) ? d : (cit->second.*pf)() );
} 

/*! \brief      Is an object a key of a map or unordered map; if so return the value in an optional, otherwise return an empty optional
    \param  m   map or unordered map to be searched
    \param  k   target key
    \return     if <i>k</i> is a member of <i>m</i>, the corresponding value, otherwise an empty optional
*/
template <class C, class K>
  requires (is_mum<C>) and (std::is_constructible_v<typename C::key_type, K>)
std::optional<typename C::mapped_type> OPT_MUM_VALUE(const C& m, const K& k)
{ std::optional<typename C::mapped_type> rv { };

  if (const auto cit { m.find(k) }; cit != m.cend())
    rv = cit->second;

  return rv;
}

// convenient syntactic sugar for some STL functions

/*! \brief              Add an element to a MUM
    \param  m           destination MUM
    \param  element     element to insert
*/
template <typename C, typename K, typename V>
  requires ( (is_mum<C> and (std::is_same_v<typename C::key_type, K>) and (std::is_same_v<typename C::mapped_type, V>)) or
             (is_mmumm<C> and (std::is_same_v<typename C::key_type, K>) and (std::is_same_v<typename C::mapped_type, V>))
           )
inline void operator+=(C& mum, std::pair<K, V>&& element)
  { mum.emplace(std::move(element)); }

/*! \brief              Add an element to a MUM
    \param  m           destination MUM
    \param  element     element to insert
*/
/* The received "wisdom" is that requires clauses are clearer than SFINAE. In this case, that is clearly untrue.
    Compare this to the above function, where I purposefully used requires clauses to achieve the same result
*/
template <typename C>
  requires (is_mum<C> or is_mmumm<C>)
inline void operator+=(C& mum, const std::pair<typename C::key_type, typename C::mapped_type>& il)
  { mum.emplace(il); }

/*! \brief        Add a string_view element to a map with a string key
    \param  mum   destination map or unordered map
    \param  il    element to insert
*/
template <typename C>
  requires (is_mum<C> or is_mmumm<C>) and is_string<typename C::key_type>
inline void operator+=(C& mum, const std::pair<std::string_view, typename C::mapped_type>& il)
  { mum.emplace( std::pair<std::string, typename C::mapped_type> { std::string {il.first}, il.second } ); }

/*! \brief        Add a string_view element to a map with a string key
    \param  mum   destination map or unordered map
    \param  il    element to insert
*/
template <typename C, typename K, typename V>
  requires (is_mum<C> or is_mmumm<C>) and is_string<typename C::key_type> and is_string_view<K> and (std::is_same_v<typename C::mapped_type, V>)
inline void operator+=(C& mum, std::pair<K, V>&& element)
  { mum.emplace(std::pair<std::string, typename C::mapped_type> { std::string {element.first}, element.second }); }

/*! \brief          Write a map to an output stream
    \param  ost     output stream
    \param  mp      object to write
    \return         the output stream
*/
template <class T>
  requires is_map<T>
std::ostream& operator<<(std::ostream& ost, const T& mp)
{ for (auto cit = mp.cbegin(); cit != mp.cend(); ++cit)
    ost << "map[" << cit->first << "]: " << cit->second << std::endl;

  return ost;
}

/*! \brief          Write an unordered map to an output stream
    \param  ost     output stream
    \param  mp      object to write
    \return         the output stream
    
    Note that the output order is, unsurprisingly, effectively random
    This works with unordered maps that support heterogeneous lookup
*/
template <class T>
  requires is_unordered_map<T>
std::ostream& operator<<(std::ostream& ost, const T& mp)
{ for (auto cit = mp.cbegin(); cit != mp.cend(); ++cit)
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
constexpr bool NONE_OF( I first, S last, Pred pred, Proj proj = {} ) { return std::ranges::none_of(first, last, pred, proj); };

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

/*! \brief      Reverse the contents of a container
    \param  v   container
*/
template <class Input>
inline void REVERSE(Input& v)
  { std::ranges::reverse(v); }

/*! \brief          Append from one container to another if a predicate is met
    \param  s       source container
    \param  d       destination container
    \param  pred    predicate
*/
template <class Input, class Output, typename PredicateT>
inline void APPEND_IF(const Input& s, Output& d, const PredicateT& pred)
  { std::copy_if(s.cbegin(), s.cend(), std::back_inserter(d), pred); }

/*! \brief          Create a container and append from another if a predicate is met
    \param  s       source container
    \param  pred    predicate
    \return         the new container

    Called as, for example, CREATE_AND_FILL<vector<string>>(in_vec, [](const string& s) { return (s == "BURBLE"s); } );
*/
template <typename Output, typename Input, typename PredicateT>
auto CREATE_AND_FILL(Input&& s, const PredicateT& pred) -> const Output
{ Output rv;

  std::ranges::copy_if(s, std::back_inserter(rv), pred);

  return rv;
}

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
template <typename T, typename U, typename V>
inline T LIMIT(const T val, const U low_val, const V high_val)
  { return std::clamp(val, static_cast<T>(low_val), static_cast<T>(high_val)); }

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
template <typename C, typename E>
  requires (is_sus<C> or is_ssuss<C>) and (std::convertible_to<base_type<E>, typename C::value_type>)
inline void operator+=(C& sus, E&& element)
  { sus.insert(std::forward<E>(element)); }

/*! \brief              Add an element to a set or unordered set
    \param  sus         destination set or unordered set
    \param  element     element to insert
*/
// Seems to be OK; https://www.youtube.com/watch?v=SYLgG7Q5Zws at 16:28; I think that the single ampersand is right
// see also https://www.sandordargo.com/blog/2021/02/17/cpp-concepts-4-ways-to-use-them
inline void operator+=(ANYSET auto& sus, const typename decltype(sus)::value_type& element)
  { sus.insert(element); }

/*! \brief              Add a string_view element to a set or unordered set or strings
    \param  sus         destination set or unordered set
    \param  element     element to insert
*/
template <typename C>
  requires is_sus<C> and is_string<typename C::value_type>
inline void operator+=(C& sus, const std::string_view element)
  { sus += (std::string { element }); }

/*! \brief          Add all elements of a vector to a set or unordered set
    \param  sus     destination set or unordered set
    \param  vec     vector to insert
*/
template <typename C, typename V>
  requires is_sus<C> and is_vector<V> and (std::is_same_v<typename C::value_type, typename V::value_type>)
inline void operator+=(C& sus, const V& vec)
  { std::copy(vec.cbegin(), vec.cend(), std::inserter(sus, sus.end())); };

/*! \brief              Remove an element from a set, map, unordered set or unordered map
    \param  sus         destination set or unordered set
    \param  element     element to remove, or an iterator into <i>sus</i>
*/
template <typename C, typename T>
  requires (is_sus<C> or is_mum<C>) and (std::is_convertible_v<base_type<T>, typename C::key_type> or std::is_same_v<base_type<T>, typename C::iterator>)
//  requires (is_sus<C> or is_mum<C>) and (std::convertible_to<base_type<T>, typename C::key_type> or std::is_same_v<base_type<T>, typename C::iterator>)     // I don't know why this fails for string -= string_view
inline void operator-=(C& sus, const T& element)
  { sus.erase(element); }

/*! \brief              Remove an element from a set, map, unordered set or unordered map based on strings, referenced by a strng_view
    \param  sus         destination set or unordered set
    \param  element     element to remove, or an iterator into <i>sus</i>

    Needed because string_view is NOT convertible to string : https://en.cppreference.com/w/cpp/types/is_convertible
*/
template <typename C, typename T>
//  requires (is_sus<C> or is_mum<C>) and is_string<typename C::key_type> and is_ssv<T>
  requires (is_sus<C> or is_mum<C>) and is_string<typename C::key_type> and is_string_view<T>
inline void operator-=(C& sus, const T& element)
  { sus.erase(static_cast<std::string>(element)); }

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
  requires ( ( (is_mum<MUMD>) and (is_mum<MUMS>) and
             (std::is_same_v<typename MUMD::key_type, typename MUMS::key_type>) and (std::is_same_v<typename MUMD::mapped_type, typename MUMS::mapped_type>) ) or
             ( (is_sus<MUMD>) and (is_sus<MUMS>) and (std::is_same_v<typename MUMD::value_type, typename MUMS::value_type>) ) )
inline void operator+=(MUMD& dest, const MUMS& src)
  { dest.insert(src.cbegin(), src.cend()); }

/*! \brief        Append one vector to another
    \param  dest  destination vector
    \param  src   source vector
    \return       <i>dest</i> with <i>src</i> appended
*/
template <typename V>
  requires (is_vector<V>)
void operator+=(V& dest, V&& src)
  { dest.reserve(dest.size() + src.size());
    dest.insert(dest.end(), src.begin(), src.end());
  }

/*! \brief        Append one vector to another
    \param  dest  destination vector
    \param  src   source vector
    \return       <i>dest</i> with <i>src</i> appended
*/
template <typename V>
  requires (is_vector<V>)
void operator+=(V& dest, const V& src)
  { dest.reserve(dest.size() + src.size());
    dest.insert(dest.end(), src.begin(), src.end());
  }

/*! \brief        Concatenate two vectors
    \param  dest  destination vector
    \param  src   source vector
    \return       <i>dest</i> with <i>src</i> appended
*/
template <typename V>
  requires (is_vector<V>)
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
template <typename V> requires (is_vector<V>)
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
  requires (is_vector<V>) and (is_sus<SUS>) and (std::is_same_v<base_type<typename V::value_type>, base_type<typename SUS::value_type>>)
void operator+=(V& vec, const SUS& sus)
{ vec.reserve(vec.size() + sus.size());

  COPY_ALL(sus, back_inserter(vec));
}

/*! \brief              Add an element to a vector
    \param  dest        destination vector
    \param  element     element to append
    \return             <i>dest</i> with <i>element</i> appended
*/
template <typename V, typename E>
  requires is_vector<V> and (std::is_same_v<typename V::value_type, base_type<E>>)
auto operator+(const V& v1, E&& element) -> V
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
  requires is_sus<S> and (std::is_same_v<typename S::value_type, base_type<E>>)
auto operator+(const S& s1, E&& element) -> S
{ S rv { s1 };

  rv.insert(std::forward<E>(element));
  
  return rv; 
}

/*! \brief              Add a string_view element to a set or unordered set or strings
    \param  sus         destination set or unordered set
    \param  element     element to insert
    \return             a new container, comprising <i>sus</i> with <i>element</i> inserted
*/
template <typename C>
  requires is_sus<C> and is_string<typename C::value_type>
C operator+(const C& sus, const std::string_view element)
{ C rv { sus };

  rv += element;

  return rv;
}

/*! \brief              Append an element to a deque, list or vector
    \param  c1          destination deque, list or vector
    \param  element     element to append
*/
template <typename C>
  requires is_deque<C> or is_list<C> or is_vector<C>
inline void operator+=(C& c1, typename C::value_type&& element)
  { c1.push_back(std::forward<typename C::value_type>(element)); }

/*! \brief              Append an element to a deque, list or vector
    \param  c1          destination deque, list or vector
    \param  element     element to append
*/
template <typename C, typename E>
  requires (is_deque<C> or is_list<C> or is_vector<C>) and (std::convertible_to<E, typename C::value_type>)
inline void operator+=(C& c1, const E& element)
  { c1.push_back(element ); }

/*! \brief              Append string_view element to a deque, list or vector of strings
    \param  c1          destination deque, list or vector
    \param  element     element to append
*/
template <typename C>
  requires (is_deque<C> or is_list<C> or is_vector<C>) and (is_string<typename C::value_type>)
inline void operator+=(C& c1, std::string_view element)
  { c1.emplace_back(std::string { element }); }

/*! \brief      Insert an element into a list
    \param  l1  destination list
    \param  pr  location and value to insert
*/
template <typename L>
  requires is_list<L>
inline void operator+=(L& l1, std::pair<typename L::const_iterator, typename L::value_type>&& pr)
  { l1.insert(std::forward<typename L::const_iterator>(pr.first), std::forward<typename L::value_type>(pr.second)); }

/*! \brief      Insert an element into a list
    \param  l1  destination list
    \param  pr  location and value to insert
*/
template <typename L>
  requires is_list<L>
inline void operator+=(L& l1, const std::pair<typename L::const_iterator, typename L::value_type>& pr)
  { l1.insert(pr.first, pr.second); }

/*! \brief              Remove all elements with a particular value from a list
    \param  c1          list
    \param  element     value to remove
*/
template <typename C>
  requires is_list<C>
inline void operator-=(C& c1, typename C::value_type&& element)
  { c1.remove(std::forward<typename C::value_type>(element)); }

/*! \brief              Remove all elements with a particular value from a list
    \param  c1          list
    \param  element     value to remove
*/
template <typename C>
  requires is_list<C>
inline void operator-=(C& c1, const typename C::value_type& element)
  { c1.remove(element); }

/*! \brief              Append an element to a queue
    \param  q1          destination queue
    \param  element     element to append

    std::queue is NOT thread safe; so this must be called only when protected by a mutex
*/
template <typename Q>
  requires is_queue<Q>
inline void operator+=(Q& q1, typename Q::value_type&& element)
  { q1.push(std::forward<typename Q::value_type>(element)); }

/*! \brief              Append an element to a queue
    \param  q1          destination queue
    \param  element     element to append

    std::queue is NOT thread safe; so this must be called only when protected by a mutex
*/
template <typename Q, typename E>
  requires is_queue<Q> and (std::convertible_to<E, typename Q::value_type>)
inline void operator+=(Q& q1, const E& element)
  { q1.push(element); }

/*! \brief              Remove and call destructor on front element of deque 
    \param  D1          destination deque

    Does nothing if the deque is empty
*/
template <typename D>
  requires is_deque<D>
void operator--(D& d /*, int*/) // int for post-decrement
{ if (!d.empty())
    d.pop_front();
}

/*! \brief      Remove an element referenced by an iterator from a deque
    \param  d   destination deque
    \param  it  iterator
*/
template <typename D>
  requires is_deque<D>
inline void operator-=(D& d, typename D::iterator&& it)
  { d.erase(std::forward<typename D::iterator>(it)); }

/*! \brief      Remove an element referenced by an iterator from a deque
    \param  c1  destination deque
    \param  it  iterator
*/
template <typename D>
  requires is_deque<D>
inline void operator-=(D& d, const typename D::iterator& it)
{ d.erase(it); }

/*! \brief              Is an element in a deque, list or vector?
    \param  c           the deque, list or vector
    \param  element     the value to test
    \return             whether <i>element</i> is an element of <i>v</i>
*/
template <typename C, typename E>
  requires (is_deque<C> or is_list<C> or is_vector<C>) and (std::convertible_to<E, typename C::value_type>)
inline bool contains(const C& c, const E& element)
  { return std::ranges::find(c, element) != c.cend(); }

/*! \brief        Obtain a view of a range of numbers
    \param  u     the lowest value in the range
    \param  v     one more than the highest value in the range
    \return       a view of the values from <i>u</i> to <i>v-1</i>
*/
template <typename T = int, typename U, typename V>
inline auto RANGE(const U u, const V v)
  { return std::ranges::iota_view { static_cast<T>(u), static_cast<T>(v) }; }

/*! \brief          Find the first element in a container that matches a predicate, or return the default-constructed element if none match
    \param  c       the container
    \param  pred    predicate to apply
    \return         first element in <i>c<i> for which <i>pred</i> is true; if none, then a default-constructed element
*/
template <class C, class UnaryPredicate>
  requires is_list<C>
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
  { FOR_ALL(c2, [&c1] (const typename C::value_type& v) { c1 -= v; }); }

/*! \brief          Return all the keys in a map or unordered_map
    \param  m       map or unordered_map
    \return         all the keys in <i>m</i>
*/
template <typename RV, typename M>
  requires is_mum<M>
auto ALL_KEYS(const M& m) -> RV
{ RV rv;

  for (const auto& [ k, v ] : m)
    rv += k;

  return rv;
}

/*! \brief          Return all the keys in a map or unordered_map as a set
    \param  m       map or unordered_map
    \return         all the keys in <i>m</i>
*/
template <typename M>
  requires is_mum<M>
inline auto ALL_KEYS_SET(const M& m) -> std::set<typename M::key_type>
  { return ALL_KEYS <std::set<typename M::key_type>> (m); }

/*! \brief          Return all the keys in a map or unordered_map as an unordered_set
    \param  m       map or unordered_map
    \return         all the keys in <i>m</i>
*/
template <typename M>
  requires is_mum<M>
inline auto ALL_KEYS_USET(const M& m) -> std::unordered_set<typename M::key_type>
  { return ALL_KEYS <std::unordered_set<typename M::key_type>> (m); }

/*! \brief                      Invert a mapping from map<T, set<T> > to map<T, T>, where final keys are the elements of the original set
    \param  original_mapping    original mapping
    \return                     inverted mapping
*/
#if 0
template <typename M>  // M = map<T, set<T> >
  requires is_mum<M>
auto INVERT_MAPPING(const M& original_mapping) -> std::map<typename M::key_type, typename M::key_type>
{ std::map<typename M::key_type, typename M::key_type> rv;

  for (const auto& [ k1, v1 ] : original_mapping)
    FOR_ALL(v1, [ k1, &rv ] (const auto& v) { rv += { v, k1 }; });

  return rv;
}
#endif

#if 0
template <typename M>  // M = map<T, set<T> >
  requires is_mum<M>
auto INVERT_MAPPING(const M& original_mapping) -> std::map<typename M::key_type, typename M::key_type, typename M::key_compare>
{ std::map<typename M::key_type, typename M::key_type, typename M::key_compare> rv;

  for (const auto& [ k1, v1 ] : original_mapping)
    FOR_ALL(v1, [ k1, &rv ] (const auto& v) { rv += { v, k1 }; });

  return rv;
}
#endif

// *** https://en.cppreference.com/w/cpp/container/map

#if 1
// need to use this so as to get heterogeneous lookup
template <typename M>  // M = map<T, set<T> >
//  requires is_mum<M> and is_sus<typename M::mapped_type> and std::is_same_v<typename M::key_type, typename M::mapped_type::value_type> and is_string<typename M::key_type>  // is_sus fails; I don't know why
  requires is_mum<M> and is_string<typename M::key_type>
auto INVERT_MAPPING(const M& original_mapping) -> STRING_MAP<std::string>
{ //std::map<typename M::key_type, typename M::key_type> rv;
  STRING_MAP<std::string> rv;

  for (const auto& [ k1, v1 ] : original_mapping)
    FOR_ALL(v1, [ k1, &rv ] (const auto& v) { rv += { v, k1 }; });

  return rv;
}
#endif

#if 0
template <typename M>  // M = map<T, set<T> >
  requires is_mum<M> and is_sus<typename M::value_type> and std::is_same_v<typename M::key_type, typename M::value_type::value_type>
auto INVERT_MAPPING(const M& original_mapping) -> M
{ M rv;

  for (const auto& [ k1, v1 ] : original_mapping)
    FOR_ALL(v1, [ k1, &rv ] (const auto& v) { rv += { v, k1 }; });

  return rv;
}
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
  { if (!(_values.contains(val)))
      _values += { val, n };
    else
      _values[val] += n;

    return (_values[val] >= _threshold);
  }

/*! \brief          Number of times a value has been added
    \param  val     target value
    \return         total number of times <i>value</i> has been added
*/
  inline unsigned int value(const T& val) const
    { return ( (_values.contains(val)) ? _values.at(val) : 0 ); }
};

// current time (in seconds since the epoch)
inline time_t NOW(void)
  { return ::time(NULL); }           // get the time from the kernel

// current time, as a TIME_POINT
inline TIME_POINT NOW_TP(void)
  { return std::chrono::system_clock::now(); }

/*! \brief        Create a reverse iterator from a bidirectional iterator, pointing to the same element
    \param  it    bidirectional iterator
    \return       reverse iterator that points to <i>*fit</i>

    See: Josuttis "The C++ Standard Library", first ed. p. 266f
*/
template <typename IT>
  requires std::bidirectional_iterator<IT>
inline auto REVERSE_IT(IT it) -> std::reverse_iterator<decltype(it)>
  { return std::prev(std::reverse_iterator<decltype(it)>(it)); }

/*! \brief      Return all the values in a MUM (including duplicates) as a vector
    \param  m   source MUM
    \return     the values in <i>m</i> as a vector
*/
template <typename RTYPE, typename M>
  requires is_mum<M>
auto VALUES(const M& m) -> std::vector<typename M::mapped_type>
{ std::vector<typename M::mapped_type> rv;

  FOR_ALL(m, [&rv] (const typename M::value_type& v) { rv += v.second; });

  return rv;
}

/*! \brief      Return all the values in a SUS (including duplicates) as a vector
    \param  s   source SUS
    \return     the values in <i>s</i> as a vector
*/
template <typename RTYPE, typename S>
  requires is_sus<S>
auto VALUES(const S& s) -> std::vector<typename S::value_type>
{ std::vector<typename S::value_type> rv;

  FOR_ALL(s, [&rv] (const typename S::value_type& v) { rv += v; });

  return rv;
}

/*! \brief      Convert a range to a vector
    \param  r   range
    \return     <i>r</i> as a vector
*/
template <std::ranges::range R>
auto RANGE_VECTOR(R&& r)
{ auto r_common { std::forward<R>(r) | std::views::common };

  return std::vector(r_common.begin(), r_common.end());
}

/*! \brief      Convert a range to a list
    \param  r   range
    \return     <i>r</i> as a list
*/
template <std::ranges::range R>
auto RANGE_LIST(R&& r)
{ auto r_common { std::forward<R>(r) | std::views::common };

  return std::list(r_common.begin(), r_common.end());
}

/*! \brief      Convert a range to a set
    \param  r   range
    \return     <i>r</i> as a set
*/
template <std::ranges::range R>
auto RANGE_SET(R&& r)
{ auto r_common { std::forward<R>(r) | std::views::common };

  return std::set(r_common.begin(), r_common.end());
}

/*! \brief      Convert a range to an unordered set
    \param  r   range
    \return     <i>r</i> as an unordered set
*/
template <std::ranges::range R>
auto RANGE_USET(R&& r)
{ auto r_common { std::forward<R>(r) | std::views::common };

  return std::unordered_set(r_common.begin(), r_common.end());
}

/*! \brief      Convert a range to a particular container type
    \param  r   range
    \return     <i>r</i> as a particular container
*/
template <typename RT, std::ranges::range R>
auto RANGE_CONTAINER(R&& r) -> RT
{ auto r_common { std::forward<R>(r) | std::views::common };

  return RT(r_common.begin(), r_common.end());
}

/*! \brief      Given a container of values and a coresponding pseudo-index, return the calue corresponding to a particular pseudo-indes
    \param  r   range
    \return     <i>r</i> as a particular container
*/
template <typename T>
T MAP_VALUE(const std::span<T> values, const size_t idx_1, const size_t idx_2, const size_t idx)
{ if (values.empty())
    return T { };

  const size_t min_idx { (idx_1 < idx_2) ? idx_1 : idx_2 };
  const size_t max_idx { (min_idx == idx_1) ? idx_2 : idx_1 };

//  const size_t limited_idx { std::clamp(idx, min_idx, max_idx) };   // force the index into the valid range
  const size_t idx_range   { max_idx - min_idx };
  const size_t n_values    { values.size() };

  const float frac { static_cast<float>(idx - min_idx) / static_cast<float>(idx_range) };

  size_t element_nr { static_cast<size_t> (frac * n_values) };

  element_nr = std::clamp(element_nr, 0UL, n_values - 1);

  return values[element_nr];
}

template <typename C>
  requires is_vector<C> or is_array<C>
inline typename C::value_type MAP_VALUE(const C& values, const size_t idx_1, const size_t idx_2, const size_t idx)
  { return MAP_VALUE(std::span<const typename C::value_type> { values }, idx_1, idx_2, idx); }
//  { ///*const*/ std::span<typename C::value_type> tmps { values };
//    return MAP_VALUE(static_cast<std::span<const typename C::value_type>>(values), idx_1, idx_2, idx);
//  }

template <typename T>
std::pair<size_t, T> MAP_VALUE_PAIR(std::span<T> values, const size_t idx_1, const size_t idx_2, const size_t idx)
{ if (values.empty())
    return { 0, T { } };

  const size_t min_idx { (idx_1 < idx_2) ? idx_1 : idx_2 };
  const size_t max_idx { (min_idx == idx_1) ? idx_2 : idx_1 };

  const size_t limited_idx { std::clamp(idx, min_idx, max_idx) };   // force the index into the valid range
  const size_t idx_range   { max_idx - min_idx };
  const size_t n_values    { values.size() };

  const float frac { static_cast<float>(idx - min_idx) / static_cast<float>(idx_range) };

  size_t element_nr { static_cast<size_t> ((frac * n_values) /* + 0.5 */) };

  element_nr = std::clamp(element_nr, 0UL, n_values - 1);

  return { element_nr, values[element_nr] };
}

#endif    // MACROS_H
