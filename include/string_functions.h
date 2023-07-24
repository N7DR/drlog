// $Id: string_functions.h 222 2023-07-09 12:58:56Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   string_functions.h

    Functions related to the manipulation of strings
*/

#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#include "macros.h"
#include "x_error.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <ranges>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <time.h>

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

// see https://stackoverflow.com/questions/44636549/why-is-there-no-support-for-concatenating-stdstring-and-stdstring-view
// brain-dead: cannot perform s + sv until at least C++26
constexpr std::string EOL { "\n"s };            ///< end-of-line marker as string

constexpr char EOL_CHAR { '\n' };                   ///< end-of-line marker as character

constexpr std::string LF     { "\n"s };         ///< LF as string
constexpr std::string LF_STR { "\n"s };            ///< LF as string

constexpr char LF_CHAR { '\n' };                    ///< LF as character

constexpr std::string CR     { "\r"s };         ///< CR as string
constexpr std::string CR_STR { "\r"s };            ///< CR as string

constexpr char CR_CHAR { '\r' };                    ///< CR as character

constexpr std::string CRLF { "\r\n"s };          ///< CR followed by LF

constexpr std::string EMPTY_STR { };             ///< an empty string
constexpr std::string FULL_STOP { "."s };        ///< full stop as string
constexpr std::string SPACE_STR { " "s };        ///< space as string

constexpr std::string_view CALLSIGN_CHARS                { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/"sv };    ///< convenient place to hold all characters that are legal in callsigns
constexpr std::string_view DIGITS                        { "0123456789"sv };                               ///< convenient place to hold all digits
constexpr std::string_view DIGITS_AND_UPPER_CASE_LETTERS { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv };     ///< convenient place to hold all digits and upper case letters
constexpr std::string_view UPPER_CASE_LETTERS            { "ABCDEFGHIJKLMNOPQRSTUVWXYZ"sv };               ///< convenient place to hold all upper case letters

constexpr char BACKSLASH_CHAR { '\\' };
constexpr char SPACE_CHAR     { ' ' };
  
/// directions in which a string can be padded
enum class PAD { LEFT,                  ///< pad to the left
                 RIGHT                  ///< pad to the right
               };

/// treatment of delimiters when manipulating string
enum class DELIMITERS { KEEP,                   ///< include the delimiters in the output
                        DROP                    ///< do not include the delimiters in the output
                      };

/// whether to include seconds
enum class SECONDS { INCLUDE, 
                     NO_INCLUDE
                   };

// error numbers
constexpr int STRING_UNDERFLOW            { -1 },    ///< Underflow
              STRING_UNEXPECTED_CHARACTER { -2 },    ///< Unexpected character found in string
              STRING_INVALID_FILE         { -3 },    ///< Invalid file
              STRING_INVALID_LENGTH       { -4 },    ///< Length is invalid
              STRING_FILE_IS_DIRECTORY    { -5 },    ///< File is a directory
              STRING_UNABLE_TO_STAT_FILE  { -6 },    ///< Unable to stat a file
              STRING_BOUNDS_ERROR         { -7 },    ///< Attempt to access range outside string
              STRING_CONVERSION_FAILURE   { -8 },    ///< Attempt to convert the format of a string failed
              STRING_UNKNOWN_ENCODING     { -9 },    ///< Unknown character encoding
              STRING_UNWRITEABLE_FILE     { -10 };   ///< File cannot be written

/*! \brief          Convert from a CSV line to a vector of strings, each containing one field
    \param  line    CSV line
    \return         vector of fields from the CSV line

    This is actually quite difficult to do properly
*/
std::vector<std::string> from_csv(std::string_view line);

/*! \brief      Duplicate a particular character within a string
    \param  s   string in which characters are to be duplicated
    \param  c   character to be duplicated
    \return     <i>s</i>, modified so that every instance of <i>c</i> is doubled
*/
std::string duplicate_char(const std::string& s, const char c = '"');

/*! \brief                      Provide a formatted UTC date/time string
    \param  include_seconds     whether to include the portion of the string that designates seconds
    \return                     current date and time in the format: YYYY-MM-DDTHH:MM or YYYY-MM-DDTHH:MM:SS
*/
std::string date_time_string(const SECONDS include_seconds);

/*! \brief          Convert struct tm pointer to formatted string
    \param  format  format to be used
    \param  tmp     date/time to be formatted
    \return         formatted string

    Uses strftime() to perform the formatting
*/
std::string format_time(const std::string& format, const tm* tmp);

/*! \brief      Generic conversion from string
    \param  s   string
    \return     <i>s</i> converted to type <i>T</i>

    This is a complex no-op if type <i>T</i> is a string
*/
template <class T>
T from_string(const std::string& s)
{ std::istringstream stream { s };
  T t;
     
  stream >> t;
  return t;
}

/*! \brief      Generic conversion from string_view
    \param  sv  string_view
    \return     <i>sv</i> converted to type <i>T</i>
*/
template <class T>
inline T from_string(std::string_view sv)
{ return from_string<T>(std::string { sv }); }

/*! \brief      Generic conversion from C-style char*
    \param  cp  string_view
    \return     <i>cp</i> converted to type <i>T</i>
*/
template <class T>
inline T from_string(const char* cp)
  { return from_string<T>(std::string { cp }); }

/*! \brief          Generic conversion from string, without an explicit type
    \param  s       string
    \param  var     variable into which the converted value is to be placed
*/
template <class T>
inline void auto_from_string(const std::string& s, T& var)
  { var = from_string<T>(s); }

/*! \brief          Generic conversion from string, without an explicit type
    \param  s       string
    \param  var     variable into which the converted value is to be placed
*/
template <class T>
inline void auto_from_string(std::string_view s, T& var)
  { var = from_string<T>(s); }

/*! \brief          Generic conversion to string
    \param  val     value to convert
    \return         <i>val</i>converted to a string
*/
template <class T>
std::string to_string(const T val)
{ std::ostringstream stream;
                
  stream << val;
  return stream.str();
}

/*! \brief          No-op conversion to string
    \param  val     value to convert
    \return         <i>val</i>
*/
template <class T>
  requires is_string<T>
inline std::string to_string(T&& val)
  { return std::forward<T>(val); } 

/*! \brief              Safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \param  length      length of substring to be extracted
    \return             substring of length <i>length</i>, starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn, length)</i>, except does not throw a range exception.
    Compiler error if one uses str.size() - start_posn as a default length; value may not be calculated from other parameters
*/
//inline std::string substring(const std::string& str, const size_t start_posn, const size_t length)
//inline std::string substring(std::string_view str, const size_t start_posn, const size_t length)
//  { return ( (str.size() > start_posn) ? static_cast<std::string>(str.substr(start_posn, length)) : EMPTY_STR ); }
template <typename STYPE>
inline auto substring(std::string_view str, const size_t start_posn, const size_t length) -> STYPE
  { return ( (str.size() > start_posn) ? STYPE {str.substr(start_posn, length)} : STYPE { EMPTY_STR } ); }

/*! \brief              Safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \return             substring starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn)</i>, except does not throw a range exception
*/
//inline std::string substring(const std::string& str, const size_t start_posn)
//inline std::string substring(std::string_view str, const size_t start_posn)
template <typename STYPE>
inline auto substring(std::string_view str, const size_t start_posn) -> STYPE
  { return substring <STYPE> (str, start_posn, str.size() - start_posn); }

/*! \brief              Replace every instance of one character with another
    \param  s           string on which to operate
    \param  old_char    character to be replaced
    \param  new_char    replacement character
    \return             <i>s</i>, with every instance of <i>old_char</i> replaced by <i>new_char</i>
*/
//std::string replace_char(const std::string& s, const char old_char, const char new_char);
std::string replace_char(std::string_view s, const char old_char, const char new_char);

/*! \brief              Replace every instance of one string with another
    \param  s           string on which to operate
    \param  old_str     string to be replaced
    \param  new_str     replacement string
    \return             <i>s</i>, with every instance of <i>old_str</i> replaced by <i>new_str</i>
*/
//std::string replace(const std::string& s, const std::string& old_str, const std::string& new_str);
std::string replace(std::string_view s, std::string_view old_str, std::string_view new_str);

/*! \brief              Replace part of a string with a byte-for-byte copy of an object
    \param  s           string on which to operate
    \param  start_posn  position at substitution begins
    \param  value       bytes that are to be placed into <i>s</i>
    \return             <i>s</i>, overwriting characters starting at <i>start_posn</i> with the bytes of <i>value</i>

    Will not return a string of length greater than <i>s</i>; will truncate to that length if necessary
*/
template <typename T>
//std::string replace_substring(const std::string& s, const size_t start_posn, const T& value)
std::string replace_substring(std::string_view s, const size_t start_posn, const T& value)
{ std::string rv { s };

  constexpr size_t value_size { sizeof(value) };

  u_char* cp { (u_char*)(&value) };

  for (size_t n { 0 }; n < value_size; ++n)
  { if ( (start_posn + n) < rv.size() )
      rv[start_posn + n] = cp[n];
  }

  return rv;
}

/*! \brief      Does a string contain a particular substring?
    \param  s   string to test
    \param  ss  substring for which to search
    \return     whether <i>s</i> contains the substring <i>ss</i>
*/
//inline bool contains(const std::string& s, const std::string& ss)
inline bool contains(std::string_view s, std::string_view ss)
//  { return s.find(ss) != std::string::npos; }
  { return s.find(ss) != std::string_view::npos; }

/*! \brief          Does a string contain a particular substring at a particular location?
    \param  s       string to test
    \param  ss      substring for which to search
    \param  posn    putative start position of <i>ss</i>
    \return         whether <i>s</i> contains the substring <i>ss</i>, starting at position <i>posn</i>
*/
//inline bool contains_at(const std::string& s, const std::string& ss, const size_t posn)
//  { return (s.length() >= posn + ss.length()) and (substring <std::string> (s, posn, ss.length()) == ss); }
inline bool contains_at(std::string_view s, std::string_view ss, const size_t posn)
  { return (s.length() >= posn + ss.length()) and (substring <std::string_view> (s, posn, ss.length()) == ss); }

/*! \brief      Does a string contain a particular character?
    \param  s   string to test
    \param  c   character for which to search
    \return     whether <i>s</i> contains the character <i>c</i>
*/
//inline bool contains(const std::string& s, const char c)
inline bool contains(std::string_view s, const char c)
  { return s.find(c) != std::string_view::npos; }

/*! \brief          Does a string contain a particular character at a particular location?
    \param  s       string to test
    \param  c       character for which to search
    \param  posn    location to test
    \return         whether <i>s</i> contains the character <i>c</i> at position <i>posn</i>
*/
//inline bool contains_at(const std::string& s, const char c, const size_t posn)
inline bool contains_at(std::string_view s, const char c, const size_t posn)
  { return (s.length() > posn) and (s[posn] == c); }

/*! \brief          Does a string contain any letters?
    \param  str     string to test
    \return         whether <i>str</i> contains any letters

    This should give the correct result in any locale
*/
//inline bool contains_letter(const std::string& str)
inline bool contains_letter(std::string_view str)
  { return ANY_OF(str, [] (const char c) { return ((c >= 'A' and c <= 'Z') or (c >= 'a' and c <= 'z')); } ); }

/*! \brief          Does a string contain any upper case letters?
    \param  str     string to test
    \return         whether <i>str</i> contains any upper case letters
*/
//inline bool contains_upper_case_letter(const std::string& str)
inline bool contains_upper_case_letter(std::string_view str)
  { return ANY_OF(str, [] (const char c) { return (c >= 'A' and c <= 'Z'); } ); }

/*! \brief          Does a string contain any digits?
    \param  str     string to test
    \return         whether <i>str</i> contains any digits
*/
//inline bool contains_digit(const std::string& str)
inline bool contains_digit(std::string_view str)
  { return ANY_OF(str, [] (const char c) { return isdigit(c); } ); }

/*! \brief          Does a string contain only digits?
    \param  str     string to test
    \return         whether <i>str</i> comprises only digits
*/
//inline bool is_digits(const std::string& str)
inline bool is_digits(std::string_view str)
  { return ALL_OF(str, [] (const char c) { return isdigit(c); } ); }

/*! \brief          Is a character a digit?
    \param  c   character to test
    \return     whether <i>c</i> is a digit

    This is present because C-based isdigit(c) returns an int
*/
  inline bool is_digit(const char c)
    { return static_cast<bool>(isdigit(c)); }

/*! \brief              Pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_side    side on which to pad
    \param  pad_char    character with which to pad
    \return             padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
//std::string pad_string(const std::string& s, const size_t len, const enum PAD pad_side = PAD::LEFT, const char pad_char = ' ');
std::string pad_string(std::string_view s, const size_t len, const enum PAD pad_side = PAD::LEFT, const char pad_char = ' ');

/*! \brief              Left pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_char    character with which to pad
    \return             left padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
//template <typename T>
//inline std::string pad_left(const T& s, const size_t len, const char pad_char = ' ')
//  { return pad_string(to_string(s), len, PAD::LEFT, pad_char); }
inline std::string pad_left(std::string_view s, const size_t len, const char pad_char = ' ')
  { return pad_string(s, len, PAD::LEFT, pad_char); }

template <typename T>
  requires (std::is_integral_v<T>)
inline std::string pad_left(T s, const size_t len, const char pad_char = ' ')
  { return pad_left(std::string_view { to_string(s) }, len, pad_char); }

/*! \brief      Left pad an integer or string type with zeroes to a particular size
    \param  s   integer value, or a string
    \return     left padded version of <i>s</i> rendered as a string and left padded with zeroes
*/
template <typename T>
  requires (std::is_integral_v<T> or std::is_same_v<base_type<T>, std::string>)
inline std::string pad_leftz(const T& s, const size_t len)
  { return pad_left(std::string_view { to_string(s) }, len, '0'); }

/*! \brief              Right pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_char    character with which to pad
    \return             right padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
//inline std::string pad_right(const std::string& s, const size_t len, const char pad_char = SPACE_CHAR)
//  { return pad_string(s, len, PAD::RIGHT, pad_char); }
inline std::string pad_right(std::string_view s, const size_t len, const char pad_char = ' ')
  { return pad_string(s, len, PAD::RIGHT, pad_char); }

template <typename T>
  requires (std::is_integral_v<T>)
inline std::string pad_right(T s, const size_t len, const char pad_char = ' ')
  { return pad_right(std::string_view { to_string(s) }, len, pad_char); }

/*! \brief              Read the contents of a file into a single string
    \param  filename    name of file to be read
    \return             contents of file <i>filename</i>
  
    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
std::string read_file(const std::string& filename);

/*! \brief              Read the contents of a file into a single string
    \param  path        the different directories to try, in order
    \param  filename    name of file to be read
    \return             contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
std::string read_file(const std::vector<std::string>& path, const std::string& filename);

/*! \brief              Read the contents of a file into a single string
    \param  filename    name of file to be read
    \param  path        the different directories to try, in order
    \return             contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
inline std::string read_file(const std::string& filename, const std::vector<std::string>& path)
  { return read_file(path, filename); }

/*! \brief              Write a string to a file
    \param  cs          string to be written to file
    \param  filename    name of file to be written

    Throws exception if the file cannot be written
*/
inline void write_file(const std::string& cs, const std::string& filename)
  { std::ofstream(filename.c_str(), std::ofstream::binary) << cs; }

/*! \brief  Create a string of a certain length, with all characters the same
    \param  c   Character that the string will contain
    \param  n   Length of string to be created
    \return String containing <i>n</i> copies of <i>c</i>
*/
inline std::string create_string(const char c, const int n = 1)
  { return std::string(n, c); }

/*! \brief      Does a string begin with one of a number of particular substrings?
    \param  cs  string to test
    \param  ss  container of substrings (strings or string_views) to look for
    \return     whether <i>cs</i> begins with any of the entries in <i>ss</i>
*/
//template <typename T>
//inline bool starts_with(const std::string& cs, const T& ss)
//  requires (is_string<typename T::value_type>)
//  { return ANY_OF(ss, [cs] (const std::string& str) { return cs.starts_with(str); }); }

template <typename T>
//inline bool starts_with(const std::string& cs, const T& ss)
inline bool starts_with(std::string_view cs, const T& ss)
  requires ( (is_string<typename T::value_type>) or (is_string_view<typename T::value_type>) )
  { return ANY_OF(ss, [cs] (std::string_view str) { return cs.starts_with(str); }); }

/*! \brief      Remove specific string from the start of a string if it is present
    \param  s   original string
    \param  ss  substring to remove
    \return     <i>s</i> with <i>ss</i> removed if it is present at the start of the string
*/
//std::string remove_from_start(const std::string& s, std::string_view ss);

template <typename STYPE>
auto remove_from_start(std::string_view s, std::string_view ss) -> STYPE
{ if (ss.empty())
    return STYPE { s };

  if (s.starts_with(ss))
    return substring <STYPE> (s, ss.size());

  return STYPE { s };
}

/*! \brief      Remove characters from the end of a string
    \param  s   original string
    \param  n   number of chars to remove
    \return     <i>s</i> with the last <i>n</i> characters removed
  
    If <i>n</i> is equal to or greater than the length of <i>s</i>, then
    the empty string is returned.
*/
//inline std::string remove_from_end(const std::string& s, const unsigned int n)
//  { return ( (n >= s.length()) ? std::string() : s.substr(0, s.length() - n) ); }
template <typename STYPE>
inline auto remove_from_end(std::string_view s, const unsigned int n) -> STYPE
  { return ( (n >= s.length()) ? STYPE { EMPTY_STR } : STYPE { s.substr(0, s.length() - n) } ); }

/*! \brief      Remove characters if present at the end of a string
    \param  s   original string
    \param  e   string to remove
    \return     <i>s</i> with the <i>e</i> removed, if it was present

    If <i>e</i> is not present, just returns <i>s</i>
*/
//inline std::string remove_from_end(const std::string& s, const std::string& e)
template <typename STYPE>
inline auto remove_from_end(std::string_view s, std::string_view e) -> STYPE
  { return ( s.ends_with(e) ? remove_from_end <STYPE> (s, e.length()) : STYPE { s } ); }

/*! \brief      Remove character if present at the end of a string
    \param  s   original string
    \param  c   character to remove
    \return     <i>s</i> with the <i>c</i> removed, if it was present

    If <i>c</i> is not present, just returns <i>s</i>
*/
//inline std::string remove_from_end(const std::string& s, const char c)
//  { return ( s.ends_with(c) ? remove_from_end(s, 1u) : s ); }
template <typename STYPE>
inline auto remove_from_end(std::string_view s, const char c) -> STYPE
  { return ( s.ends_with(c) ? remove_from_end <STYPE> (s, 1u) : STYPE { s } ); }

/*! \brief      Remove all instances of a specific leading character
    \param  cs  original string
    \param  c   leading character to remove (if present)
    \return     <i>cs</i> with any leading octets with the value <i>c</i> removed
*/
//std::string remove_leading(const std::string& cs, const char c);
template <typename STYPE>
auto remove_leading(std::string_view cs, const char c) -> STYPE
{ if (cs.empty())
    return STYPE { cs };

  const size_t posn { cs.find_first_not_of(create_string(c)) };

  return ( (posn == std::string_view::npos) ? STYPE { cs } : substring <STYPE> (cs, posn) );
}

/*! \brief      Remove leading spaces
    \param  cs  original string
    \return     <i>cs</i> with any leading spaces removed
*/
//inline std::string remove_leading_spaces(const std::string& cs)
//  { return remove_leading <std::string> (cs, ' '); }
template <typename STYPE>
inline auto remove_leading_spaces(std::string_view cs) -> STYPE
  { return remove_leading <STYPE> (cs, ' '); }

/*! \brief      Remove all instances of a specific trailing character
    \param  cs  original string
    \param  c   trailing character to remove (if present)
    \return     <i>cs</i> with any trailing octets with the value <i>c</i> removed
*/
//std::string remove_trailing(const std::string& cs, const char c);
template <typename STYPE>
auto remove_trailing(std::string_view cs, const char c) -> STYPE
{ STYPE rv { cs };

  while (rv.length() and (rv[rv.length() - 1] == c))
    rv = rv.substr(0, rv.length() - 1);
  
  return rv;
}

/*! \brief      Remove trailing spaces
    \param  cs  original string
    \return     <i>cs</i> with any trailing spaces removed
*/
//inline std::string remove_trailing_spaces(const std::string& cs)
//  { return remove_trailing <std::string> (cs, ' '); }
template <typename STYPE>
inline auto remove_trailing_spaces(std::string_view cs) -> STYPE
  { return remove_trailing <STYPE> (cs, ' '); }

/*! \brief      Remove leading and trailing spaces
    \param  cs  original string
    \return     <i>cs</i> with any leading or trailing spaces removed
*/
//inline std::string remove_peripheral_spaces(const std::string& cs)
//  { return remove_trailing_spaces <std::string> (remove_leading_spaces <std::string> (cs)); }
template <typename STYPE>
inline auto remove_peripheral_spaces(std::string_view cs) -> STYPE
  { return remove_trailing_spaces <STYPE> (remove_leading_spaces <std::string_view> (cs)); }

/*! \brief      Remove leading and trailing spaces from each element in a vector of strings
    \param  t   container of strings
    \return     <i>t</i> with leading and trailing spaces removed from the individual elements

    NB There should be specialisation of this for vectors that uses reserve()
*/
template <typename STYPE, typename T>
  requires ( is_vector<T> and (is_string<typename T::value_type>) or (is_string_view<typename T::value_type>) )
auto remove_peripheral_spaces(const T& t) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  FOR_ALL(t, [&rv] (const auto& s) { rv += remove_peripheral_spaces <STYPE> (s); } );

  return rv;
}

#if 0
template <typename T>
  requires (is_string<typename T::value_type>) 
T remove_peripheral_spaces(const T& t)
{ typename std::remove_const<T>::type rv;

  FOR_ALL(t, [&rv] (const std::string& s) { rv += remove_peripheral_spaces <std::string> (s); } );

  return rv;
}
#endif

/*! \brief      Remove leading and trailing spaces from each element in a container of strings
    \param  t   container of strings
    \return     <i>t</i> with leading and trailing spaces removed from the individual elements

    NB There should be specialisation of this for vectors that uses reserve()
*/
#if 1
template <typename STYPE, typename T>
  requires (is_string<typename T::value_type>) and (is_string<STYPE>)
auto remove_peripheral_spaces(T&& t) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  FOR_ALL(std::forward<T>(t), [&rv] (const std::string& s) { rv += remove_peripheral_spaces <std::string> (s); } );

  return rv;
}
#endif

#if 0
template <typename T>
  requires (is_string<typename T::value_type>)
T remove_peripheral_spaces(T&& t)
{ T rv;

  FOR_ALL(std::forward<T>(t), [&rv] (const std::string& s) { rv += remove_peripheral_spaces <std::string> (s); } );

  return rv;
}
#endif

/*! \brief              Split a string into components
    \param  cs          original string
    \param  separator   separator string (typically a single character)
    \return             vector containing the separate components
*/
//std::vector<std::string> split_string(const std::string& cs, const std::string& separator);
//std::vector<std::string> split_string(const std::string& cs, std::string_view separator);
template <typename STYPE>
//vector<string> split_string(const string& cs, string_view separator)
auto split_string(std::string_view cs, std::string_view separator) -> std::vector<STYPE>
{ size_t start_posn { 0 };

  std::vector<STYPE> rv;

  while (start_posn < cs.length())
  { if (size_t posn { cs.find(separator, start_posn) }; posn == std::string::npos)                       // no more separators
    { rv += STYPE { cs.substr(start_posn) };
      start_posn = cs.length();
    }
    else                                            // at least one separator
    { rv += STYPE { cs.substr(start_posn, posn - start_posn) };
      start_posn = posn + separator.length();
    }
  }

  return rv;
}

/*! \brief              Split a string into components
    \param  cs          original string
    \param  separator   separator character
    \return             vector containing the separate components
*/
//std::vector<std::string> split_string(const std::string& cs, const char separator = ',');

//template <typename T = std::string, typename U>
template <typename STYPE>
auto split_string(std::string_view cs, const char separator = ',') -> std::vector<STYPE>
{ size_t start_posn { 0 };

  std::vector<STYPE> rv;

  while (start_posn < cs.length())
  { if (size_t posn { cs.find(separator, start_posn) }; posn == std::string_view::npos)                       // no more separators
    { rv += STYPE { cs.substr(start_posn) };
      start_posn = cs.length();
    }
    else                                            // at least one separator
    { rv += STYPE { cs.substr(start_posn, posn - start_posn) };
      start_posn = posn + 1;
    }
  }

  return rv;
}

/*! \brief              Split a string into components, and remove peripheral spaces from each component
    \param  cs          original string
    \param  separator   separator string (typically a single character)
    \return             vector containing the separate components, with peripheral spaces removed
*/
template <typename STYPE>
//inline auto clean_split_string(const std::string& cs, const std::string& separator) -> std::vector<STYPE>
inline auto clean_split_string(std::string_view cs, std::string_view separator) -> std::vector<STYPE>
  { return remove_peripheral_spaces(split_string <STYPE> (cs, separator)); }

/*! \brief              Split a string into components, and remove peripheral spaces from each component
    \param  cs          original string
    \param  separator   separator string (typically a single character)
    \return             vector containing the separate components, with peripheral spaces removed
*/
template <typename STYPE>
//inline std::vector<std::string> clean_split_string(std::string&& cs, const std::string& separator)
inline auto clean_split_string(std::string&& cs, std::string_view separator) -> std::vector<STYPE>
  { return remove_peripheral_spaces(split_string <STYPE> (std::forward<std::string>(cs), separator)); }

/*! \brief              Split a string into components, and remove peripheral spaces from each component
    \param  cs          original string
    \param  separator   separator character
    \return             vector containing the separate components, with peripheral spaces removed
*/
template <typename STYPE>
inline auto clean_split_string(std::string_view cs, const char separator = ',') -> std::vector<STYPE>
  { return remove_peripheral_spaces <STYPE> (split_string <STYPE> (cs, separator)); }

/*! \brief              Split a string into components, and remove peripheral spaces from each component
    \param  cs          original string
    \param  separator   separator character
    \return             vector containing the separate components, with peripheral spaces removed
*/
//inline std::vector<std::string> clean_split_string(std::string&& cs, const char separator = ',')
//  { return remove_peripheral_spaces(split_string <std::string> (std::forward<std::string>(cs), separator)); }
template <typename STYPE>
inline auto clean_split_string(std::string&& cs, const char separator = ',') -> std::vector<STYPE>
  { return remove_peripheral_spaces <std::string> (split_string <STYPE> (std::forward<std::string>(cs), separator)); }

//template <typename STYPE, typename T>
//  requires (is_string<T> or is_string_view<T>)
//inline auto clean_split_string(T&& cs, const char separator = ',') -> std::vector<STYPE>
//  { return remove_peripheral_spaces(split_string <STYPE> (std::forward<T>(cs), separator)); }

//template <typename STYPE>
//inline auto clean_split_string(std::string_view&& cs, const char separator = ',') -> std::vector<STYPE>
//  { return remove_peripheral_spaces(split_string <STYPE> (std::forward<std::string_view>(cs), separator)); }

/*! \brief                  Split a string into equal-length records
    \param  cs              original string
    \param  record_length   length of each record
    \return                 vector containing the separate components

    Any non-full record at the end is silently discarded
*/
std::vector<std::string> split_string(const std::string& cs, const unsigned int record_length);

/*! \brief                  Split a string into equal-length records
    \param  cs              original string
    \param  record_length   length of each record
    \return                 vector containing the separate components

    Any non-full record at the end is silently discarded
*/
//inline std::vector<std::string> split_string(const std::string& cs, const int record_length)
//  { return split_string(cs, static_cast<unsigned int>(record_length)); }

/*! \brief      Squash repeated occurrences of a character
    \param  cs  original string
    \param  c   character to squash
    \return     <i>cs</i>, but with all consecutive instances of <i>c</i> converted to a single instance
*/
//std::string squash(const std::string& cs, const char c = ' ');
std::string squash(std::string_view cs, const char c = ' ');

/*! \brief          Remove empty lines from a vector of lines
    \param  lines   the original vector of lines
    \return         <i>lines</i>, but with all empty lines removed

    If the line contains anything, even just whitespace, it is not removed
*/
std::vector<std::string> remove_empty_lines(const std::vector<std::string>& lines);

/*! \brief              Split a string into lines
    \param  cs          original string
    \param  eol_marker  EOL marker
    \return             vector containing the separate lines
*/
//inline std::vector<std::string> to_lines(const std::string& cs, const std::string& eol_marker = EOL)
//  { return split_string <std::string> (cs, eol_marker); }
//inline std::vector<std::string> to_lines(std::string_view cs, std::string_view eol_marker = EOL)
//  { return split_string <std::string> (cs, eol_marker); }
template <typename STYPE>
inline auto to_lines(std::string_view cs, std::string_view eol_marker = EOL) -> std::vector<STYPE>
  { return split_string <STYPE> (cs, eol_marker); }

/*! \brief      Remove peripheral instances of a specific character
    \param  cs  original string
    \param  c   character to remove
    \return     <i>cs</i> with any leading or trailing instances of <i>c</i> removed
*/
//inline std::string remove_peripheral_character(const std::string& cs, const char c)
//  { return remove_trailing <std::string> (remove_leading <std::string> (cs, c), c); }
template <typename STYPE>
inline auto remove_peripheral_character(std::string_view cs, const char c) -> STYPE
  { return remove_trailing <STYPE> (remove_leading <std::string_view> (cs, c), c); }

/*! \brief                  Remove all instances of a particular char from a string
    \param  cs              original string
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
std::string remove_char(std::string_view cs, const char char_to_remove);

/*! \brief                  Remove all instances of a particular char from a string
    \param  s               original string
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
//inline std::string remove_char(std::string& s, const char char_to_remove)
//  { return remove_char(static_cast<const std::string>(s), char_to_remove); }
  
/*! \brief                  Remove all instances of a particular char from a container of strings
    \param  t               container of strings
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>t</i> with all instances of <i>char_to_remove</i> removed
*/
template <typename C>
  requires (is_string<typename C::value_type>)
C remove_char(C& t, const char char_to_remove)
{ typename std::remove_const<C>::type rv;

  FOR_ALL(t, [char_to_remove, &rv] (const std::string& cs) { rv += remove_char(cs, char_to_remove); } );

  return rv;
}

/*! \brief                  Remove all instances of a particular char from a container of strings
    \param  t               container of strings
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>t</i> with all instances of <i>char_to_remove</i> removed
*/
template <typename C>
  requires (is_string<typename C::value_type>)
C remove_char(C&& t, const char char_to_remove)
{ C rv;

  FOR_ALL(std::forward<C>(t), [char_to_remove, &rv] (const std::string& cs) { rv += remove_char(cs, char_to_remove); } );

  return rv;
}

/*! \brief                      Remove all instances of particular characters from a string
    \param  s                   original string
    \param  chars_to_remove     string whose characters are to be removed from <i>s</i>
    \return                     <i>s</i> with all instances of the characters in <i>chars_to_remove</i> removed
*/
//std::string remove_chars(const std::string& s, const std::string& chars_to_remove);
//std::string remove_chars(std::string_view s, const std::string& chars_to_remove);
std::string remove_chars(std::string_view s, std::string_view chars_to_remove);

/*! \brief                  Remove all instances of a particular char from all delimited substrings
    \param  cs              original string
    \param  char_to_remove  character to be removed from delimited substrings in <i>cs</i>
    \param  delim_1         opening delimiter
    \param  delim_2         closing delimiter
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed from inside substrings delimited by <i>delim_1</i> and <i>delim_2</i>
*/
//std::string remove_char_from_delimited_substrings(const std::string& cs, const char char_to_remove, const char delim_1, const char delim_2);
std::string remove_char_from_delimited_substrings(std::string_view cs, const char char_to_remove, const char delim_1, const char delim_2);

/*! \brief                      Obtain a delimited substring
    \param  cs                  original string
    \param  delim_1             opening delimiter
    \param  delim_2             closing delimiter
    \param  return_delimiters   whether to keep delimiters in the returned value
    \return                     substring between <i>delim_1</i> and <i>delim_2</i>, possibly including the delimiters
  
    Returns the empty string if the delimiters do not exist, or if
    <i>delim_2</i> does not appear after <i>delim_1</i>. Returns only the
    first delimited substring if more than one exists.
*/
//std::string delimited_substring(const std::string& cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters);
template <typename STYPE>
auto delimited_substring(std::string_view cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters) -> STYPE
{ const size_t posn_1 { cs.find(delim_1) };
  
  if (posn_1 == std::string_view::npos)
    return STYPE { EMPTY_STR };
  
  const size_t posn_2 { cs.find(delim_2, posn_1 + 1) };
  
  if (posn_2 == std::string_view::npos)
    return STYPE { EMPTY_STR };
  
  return ( (return_delimiters == DELIMITERS::DROP) ? STYPE { cs.substr(posn_1 + 1, posn_2 - posn_1 - 1) } : STYPE { cs.substr(posn_1, posn_2 - posn_1) } ) ;
}

/*! \brief                      Obtain a delimited substring
    \param  cs                  original string
    \param  delim_1             opening delimiter
    \param  delim_2             closing delimiter
    \param  return_delimiters   whether to keep delimiters in the returned value
    \return                     substring between <i>delim_1</i> and <i>delim_2</i>
  
    Returns the empty string if the delimiters do not exist, or if
    <i>delim_2</i> does not appear after <i>delim_1</i>. Returns only the
    first delimited substring if more than one exists.
*/
//std::string delimited_substring(const std::string& cs, const std::string& delim_1, const std::string& delim_2, const DELIMITERS return_delimiters);
template <typename STYPE>
auto delimited_substring(std::string_view cs, std::string_view delim_1, std::string_view delim_2, const DELIMITERS return_delimiters) -> STYPE
{ const size_t posn_1 { cs.find(delim_1) };
  
  if (posn_1 == std::string_view::npos)
    return STYPE { EMPTY_STR };  
  
  const size_t length_to_skip { ( (return_delimiters == DELIMITERS::DROP) ? delim_1.length() : 0 ) };
  const size_t posn_2         { cs.find(delim_2, posn_1 + length_to_skip) };
  
  if (posn_2 == std::string_view::npos)
    return STYPE { EMPTY_STR };
  
  const size_t length_to_return { (return_delimiters == DELIMITERS::DROP) ? posn_2 - posn_1 - delim_1.length()
                                                                          : posn_2 + posn_1 + delim_2.length()
                                };
  
  return STYPE { cs.substr(posn_1 + length_to_skip, length_to_return) };
}

/*! \brief                      Obtain all occurrences of a delimited substring
    \param  cs                  original string
    \param  delim_1             opening delimiter
    \param  delim_2             closing delimiter
    \param  return_delimiters   whether to keep delimiters in the returned value
    \return                     all substrings between <i>delim_1</i> and <i>delim_2</i>
*/
//std::vector<std::string> delimited_substrings(const std::string& cs, const std::string& delim_1, const std::string& delim_2, const DELIMITERS return_delimiters);
//std::vector<std::string> delimited_substrings(std::string_view cs, std::string_view delim_1, std::string_view delim_2, const DELIMITERS return_delimiters);
template <typename STYPE>
auto delimited_substrings(std::string_view cs, std::string_view delim_1, std::string_view delim_2, const DELIMITERS return_delimiters) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  size_t cs_start_posn { 0 };

//  while (!substring <std::string> (cs, cs_start_posn).empty())
  while (!substring <std::string_view> (cs, cs_start_posn).empty())
  { std::string_view sstring { substring <std::string_view> (cs, cs_start_posn) };
    
    const size_t posn_1 { sstring.find(delim_1) };

    if (posn_1 == std::string_view::npos)             // no more starting delimiters
      return rv;

    const size_t posn_2 { sstring.find(delim_2, posn_1 + delim_1.length()) };

    if (posn_2 == std::string_view::npos)
      return rv;                            // no more ending delimiters

    if (return_delimiters == DELIMITERS::DROP)
      rv += sstring.substr(posn_1 + delim_1.length(), posn_2 - posn_1 - delim_1.length());
    else
      rv += sstring.substr(posn_1, posn_2 + delim_2.length() - posn_1);
      
    cs_start_posn += (posn_2 + delim_2.length());
  }

  return rv;
}

/*! \brief                      Obtain all occurrences of a delimited substring
    \param  cs                  original string
    \param  delim_1             opening delimiter
    \param  delim_2             closing delimiter
    \param  return_delimiters   whether to keep delimiters in the returned value
    \return                     all substrings between <i>delim_1</i> and <i>delim_2</i>, possibly including the delimiters
*/
//std::vector<std::string> delimited_substrings(const std::string& cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters);
template <typename STYPE>
auto delimited_substrings(std::string_view cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  size_t start_posn { 0 };      // start posn is, and remains global (i.e., wrt cs)

  while ( (start_posn < cs.length() and !substring <std::string> (cs, start_posn).empty()) )  // initial test so substring() doesn't write to output
  { std::string_view sstring { substring <std::string_view> (cs, start_posn) };

    const size_t  posn_1  { sstring.find(delim_1) };

    if (posn_1 == std::string_view::npos)             // no more starting delimiters
      return rv;

    const size_t posn_2 { sstring.find(delim_2, posn_1 + 1) };

    if (posn_2 == std::string_view::npos)
      return rv;                            // no more ending delimiters

    if (return_delimiters == DELIMITERS::KEEP)
      rv += sstring.substr(posn_1, posn_2 - posn_1);
    else
      rv += sstring.substr(posn_1 + 1, posn_2 - posn_1 - 1);

    start_posn += (posn_2 + 1);   // remember, start_posn is global
  }

  return rv;
}

/*! \brief          Join the elements of a container of strings, using a provided separator
    \param  ct      container of strings
    \param  sep     separator inserted between the elements of <i>vec</i>
    \return         all the elements of <i>ct</i>, concatenated, but with <i>sep</i> inserted between elements
*/
template <typename T, typename U>
std::string join(const T& ct, const U sep)
  requires (is_string<typename T::value_type>)
{ std::string rv { };

  for (auto cit { ct.cbegin() }; cit != ct.cend(); ++cit)
  { if (cit != ct.cbegin())
      rv += sep;

    rv += (*cit);
  }

  return rv;
}

/*! \brief          Centre a string
    \param  str     string to be centred
    \param  width   final width of the centred string
    \return         <i>str</i> centred in a string of spaces, with total size <i>width</i>,
*/
//std::string create_centred_string(const std::string& str, const unsigned int width);
std::string create_centred_string(std::string_view str, const unsigned int width);

/*! \brief      Get the last character in a string
    \param  cs  source string
    \return     last character in <i>cs</i>

    Throws exception if <i>cs</i> is empty
*/
//char last_char(const std::string& cs);
char last_char(std::string_view cs);

/*! \brief      Get the penultimate character in a string
    \param  cs  source string
    \return     penultimate character in <i>cs</i>

    Throws exception if <i>cs</i> is empty or contains only one character
*/
//char penultimate_char(const std::string& cs);
char penultimate_char(std::string_view cs);

/*! \brief      Get the antepenultimate character in a string
    \param  cs  source string
    \return     antepenultimate character in <i>cs</i>

    Throws exception if <i>cs</i> contains fewer than two characters
*/
//char antepenultimate_char(const std::string& cs);
char antepenultimate_char(std::string_view cs);

/*! \brief          Get the terminating part of a string
    \param  cs      original string
    \param  n       number of characters to return
    \return         the last <i>n</i> characters of <i>cs</i>
*/
//inline std::string last(const std::string& cs, unsigned int n)
//  { return (cs.length() < n ? cs : cs.substr(cs.length() - n)); }
template <typename STYPE>
inline auto last(std::string_view cs, unsigned int n) -> STYPE
  { return (cs.length() < n ? STYPE { cs } : STYPE { cs.substr(cs.length() - n) }); }

/*! \brief              Get an environment variable
    \param  var_name    name of the environment variable
    \return             the contents of the environment variable <i>var_name</i>

    Returns the empty string if the variable <i>var_name</i> does not exist
*/
std::string get_environment_variable(const std::string& var_name);

/*! \brief      Transform a string
    \param  cs  original string
    \param  pf  pointer to transformation function
    \return     <i>cs</i> with the transformation <i>*pf</i> applied
*/
//std::string transform_string(const std::string& cs, int(*pf)(int));
std::string transform_string(std::string_view cs, int(*pf)(int));

/*! \brief      Convert string to upper case
    \param  cs  original string
    \return     <i>cs</i> converted to upper case
*/
//inline std::string to_upper(const std::string& cs)
inline std::string to_upper(std::string_view cs)
  { return transform_string(cs, std::toupper); }

/*! \brief      Convert string to lower case
    \param  cs  original string
    \return     <i>cs</i> converted to lower case
*/
//inline std::string to_lower(const std::string& cs)
inline std::string to_lower(std::string_view cs)
  { return transform_string(cs, std::tolower); }

/*! \brief              Is a call a maritime mobile?
    \param  callsign    call to test
    \return             whether <i>callsign</i> appears to be a maritime mobile
*/
inline bool is_maritime_mobile(const std::string& callsign)
  { return to_upper(callsign).ends_with("/MM"sv); }

/*! \brief          Convert an integer to a character-separated string
    \param  n       number to convert
    \param  sep     string to act as the triplet separator
    \return         <i>n</i> with the separator <i>sep</i> separating each triplet

    Uses comma as separator if <i>sep</i> is empty.
*/
template <typename T>
std::string separated_string(const T n, const char sep = ',')
{ std::string tmp { to_string(n) };
  std::string rv  { };

  while (!tmp.empty())
  { for (unsigned int N { 0 }; N < 3 and !tmp.empty(); ++N)
    { rv = std::string(1, last_char(tmp)) + rv;
      tmp = tmp.substr(0, tmp.length() - 1);
    }

    if (!tmp.empty())
      rv.insert(rv.begin(), sep);
  }

  return rv;
}

/*! \brief      Convert an integer to a comma-separated string
    \param  n   number to convert
    \return     <i>n</i> with the separator <i>,</i> separating each triplet
*/
template <typename T>
inline std::string comma_separated_string(const T n)
  { return separated_string(n); }

/*! \brief      Convert an integer to a comma-separated string
    \param  n   number to convert
    \return     <i>n</i> with the separator <i>,</i> separating each triplet
*/
template <typename T>
inline std::string css(const T n)
  { return comma_separated_string(n); }

/*! \brief                  Get location of start of next word
    \param  str             string which the next word is to be found
    \param  current_posn    position from which to search
    \return                 position of start of next word, beginning at position <i>current_posn</i> in <i>str</i>

    Returns <i>string::npos</i> if no word can be found
*/
size_t next_word_posn(const std::string& str, const size_t current_posn);

/*! \brief      Get location of start all words
    \param  s   string to be analysed
    \return     positions of all the starts of words in <i>s</i>
*/
std::vector<size_t> starts_of_words(const std::string& s);

/*! \brief          Get nth word in a string
    \param  s       string to be analysed
    \param  n       word number to be returned
    \param  wrt     value with respect to which <i>n</i> is counted
    \return         the <i>n</i>th word, counting with respect to <i>wrt</i>

    Returns <i>string::npos</i> if there is no <i>n</i>th word
*/
std::string nth_word(const std::string& s, const unsigned int n, const unsigned int wrt = 0);

/*! \brief          Get the actual length, in bytes, of a UTF-8-encoded string
    \param  str     UTF-8 string to be analysed
    \return         number of bytes occupied by <i>str</i>

    See: https://stackoverflow.com/questions/4063146/getting-the-actual-length-of-a-utf-8-encoded-stdstring
    TODO: generalise using locales/facets, instead of assuming UTF-8
*/
size_t n_chars(const std::string& str);

/*! \brief      Does a string contain a legal dotted-decimal IPv4 address
    \param  cs  original string
    \return     whether <i>cs</i> contains a legal dotted decimal IPv4 address
*/
bool is_legal_ipv4_address(const std::string& cs);  

/*! \brief          Convert a four-byte value to a dotted decimal string
    \param  val     original value
    \return         dotted decimal string corresponding to <i>val</i>
*/
std::string convert_to_dotted_decimal(const uint32_t val);

/*! \brief                  Is a string a legal value from a list?
    \param  value           target string
    \param  legal_values    all the legal values, separated by <i>separator</i>
    \param  separator       separator in the string <i>legal_values</i>
    \return                 whether <i>value</i> appears in <i>legal_values</i>
*/
inline bool is_legal_value(const std::string& value, const std::string& legal_values, const std::string& separator)
  { return (contains( split_string <std::string> (legal_values, separator), value )); }

/*! \brief                  Is a string a legal value from a list?
    \param  value           target string
    \param  legal_values    all the legal values, separated by <i>separator</i>
    \param  separator       separator in the string <i>legal_values</i>
    \return                 whether <i>value</i> appears in <i>legal_values</i>
*/
inline bool is_legal_value(const std::string& value, const std::string& legal_values, const char separator = ',')
  { return (contains(split_string <std::string> (legal_values, separator), value)); }

/*! \brief          Is one call earlier than another, according to callsign sort order?
    \param  call1   first call
    \param  call2   second call
    \return         whether <i>call1</i> appears before <i>call2</i> in callsign sort order
*/
bool compare_calls(const std::string& call1, const std::string& call2);

/*! \brief          Is the value of one mult earlier than another?
    \param  mult1   first mult value
    \param  mult2   second mult value
    \return         whether <i>mult1</i> appears before <i>mult2</i> in displayed mult value sort order (used for exchange mults)
*/
bool compare_mults(const std::string& mult1, const std::string& mult2);

// https://stackoverflow.com/questions/2620862/using-custom-stdset-comparator
//using CALL_COMPARISON = std::integral_constant<decltype(&compare_calls), &compare_calls>;   // type that knows how to compare calls
using MULT_COMPARISON = std::integral_constant<decltype(&compare_mults), &compare_mults>;   // type that knows how to compare mult strings (for exchange mults)

/*! \brief          Return a number with a particular number of decimal places
    \param  str     initial value
    \param  n       number of decimal places
    \return         <i>str</i> with <i>n</i> decimal places

    Assumes that <i>str</i> is a number
*/
std::string decimal_places(const std::string& str, const int n);

/*! \brief          Return the longest line from a vector of lines
    \param  lines   the lines to search
    \return         the longest line in the vector <i>lines</i>
*/
std::string longest_line(const std::vector<std::string>& lines);

/*! \brief          Deal with wprintw's idiotic insertion of newlines when reaching the right hand of a window
    \param  str     string to be reformatted
    \param  width   width of line in destination window
    \return         <i>str</i> reformatted for the window

    See http://stackoverflow.com/questions/7540029/wprintw-in-ncurses-when-writing-a-newline-terminated-line-of-exactly-the-same
*/
std::string reformat_for_wprintw(const std::string& str, const int width);

/*! \brief          Deal with wprintw's idiotic insertion of newlines when reaching the right hand of a window
    \param  vecstr  vector of strings to be reformatted
    \param  width   width of line in destination window
    \return         <i>str</i> reformatted for the window

    See http://stackoverflow.com/questions/7540029/wprintw-in-ncurses-when-writing-a-newline-terminated-line-of-exactly-the-same
*/
std::vector<std::string> reformat_for_wprintw(const std::vector<std::string>& vecstr, const int width);

/*! \brief      Remove all instances of a particular substring from a string
    \param  cs  original string
    \param  ss  substring to be removed
    \return     <i>cs</i>, with all instances of <i>ss</i> removed
*/
//inline std::string remove_substring(const std::string& cs, const std::string& ss)
//  { return ( contains(cs, ss) ? replace(cs, ss, std::string()) : cs ); }
inline std::string remove_substring(std::string_view cs, std::string_view ss)
  { return ( contains(cs, ss) ? replace(cs, ss, std::string { }) : std::string { cs } ); }

/*! \brief      Create a string of spaces
    \param  n   length of string to be created
    \return     string of <i>n</i> space characters
*/
inline std::string space_string(const int n)
  { return (n > 0 ? create_string((char)32, n) : std::string()); }

/*! \brief          Write a <i>vector<string></i> object to an output stream
    \param  ost     output stream
    \param  vec     object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const std::vector<std::string>& vec);

/*! \brief                  Remove a trailing inline comment
    \param  str             string
    \param  comment_str     string that introduces the comment
    \return                 <i>str</i> with the trailing comment and any additional trailing spaces removed

    Generally it is expected that <i>str</i> is a single line (without the EOL marker)
*/
std::string remove_trailing_comment(const std::string& str, const std::string& comment_str = "//"s);

/*! \brief              Add delimiters to a string
    \param  str         string
    \param  delim_1     opening delimiter
    \param  delim_2     closing delimiter
    \return             <i>str</i> preceded by <i>delim_1</i> and followed by <i>delim_2</i>
*/
inline std::string delimit(const std::string& str, const std::string& delim_1, const std::string& delim_2)
  { return (delim_1 + str + delim_2); }

/*! \brief              Perform a case-insensitive search for a substring
    \param  str         string to search
    \param  target      substring for which to search
    \param  start_posn  location in <i>str</i> at which to start the search
    \return             position of the first character in <i>target</i> in <i>str</i>
    
    Returns string::npos if <i>target</i> cannot be found
*/
size_t case_insensitive_find(const std::string& str, const std::string& target, const size_t start_posn = 0);

/*! \brief              Truncate a string immediately prior to the first occurrence of a particular character
    \param  str         string to truncate
    \param  c           target character
    \return             <i>str</i> truncated immediately prior to <i>c</i> (if <i>c</i> is present; otherwise <i>str</i>)
*/
//inline std::string truncate_before_first(const std::string& str, const char c)
//  { return (substring <std::string> (str, 0, str.find(c))); }
template <typename STYPE>
inline auto truncate_before_first(std::string_view str, const char c) -> STYPE
  { return (substring <STYPE> (str, 0, str.find(c))); }

/*! \brief              Remove up to after the first occurrence of a particular character
    \param  str         string to truncate
    \param  c           target character
    \return             <i>str</i> truncated immediately after <i>c</i> (if <i>c</i> is present; otherwise <i>str</i>)
*/
inline std::string after_first(const std::string& str, const char c)
  { return (substring <std::string> (str, str.find(c) + 1)); }

/*! \brief          Return position in a string at the end of a target string, if present
    \param  str     string to search
    \param  target  string to find
    \return         position in <>str</i> after the end of <i>target</i>

    Returns string::npos if <i>target</i> is not a substring of <i>str</i> OR if <i>target</i>
    is the conclusion of <i>str</i>
*/
size_t find_and_go_to_end_of(std::string_view str, std::string_view target);

/*! \brief              Get the base portion of a call
    \param  callsign    original callsign
    \return             the base portion of <i>callsign</i>

    For example, a call such as VP9/G4AMJ/P returns G4AMJ.
*/
std::string base_call(const std::string& callsign);

/*! \brief      Provide a formatted date string: YYYYMMDD
    \return     current UTC date in the format: YYYYMMDD
*/
std::string YYYYMMDD_utc(void);

/*! \brief      Remove all instances of several substrings (sequentially) from a string
    \param  cs  original string
    \param  vs  vector of substrings to be removed, in that order
    \return     <i>cs</i>, with all instances of the elements of <i>vs</i> removed, applied in order
*/
std::string remove_substrings(const std::string& cs, const std::vector<std::string>& vs);

/*! \brief              Return all strings from a container that match a particular regular expression string
    \param  container   container of strings
    \param  s           regular expression string
    \return             All the elements of <i>container</i> that match <i>s</i>
*/
template <class T, class C>
T regex_matches(C&& container, const std::string& s)
{ T rv { };
     
  const std::regex rgx { s };

  FOR_ALL(std::forward<C>(container) | std::ranges::views::filter([rgx] (const std::string& target) { return regex_match(target, rgx); }),
            [&rv] (const std::string& target) { rv += target; }); 

  return rv;
}

/*! \brief          Return the number of times that a particular character occurs in a particular string
    \param  str     the string to test
    \param  c       the target character
    \return         the number of times that <i>c</i> appears in <i>str</i>
*/
inline size_t number_of_occurrences(const std::string& str, const char c)
  { return static_cast<size_t>(std::count(str.begin(), str.end(), c)); }

/*! \brief          Are two calls busts of each other?
    \param  call1   first call
    \param  call2   second call
    \return         whether <i>call1</i> and <i>call2</i> are possible busts of each other

    Testing for busts is a symmetrical function.
*/
bool is_bust_call(const std::string& call1, const std::string& call2) noexcept;

/*! \brief              Create a container of bust mappings from a container of calls
    \param  container   a container of calls
    \return             A container (a map of some kind) mapping calls to all the busts of the call
*/
template <class RV, class C>
  requires (is_string<typename C::value_type>) and is_mum<RV> and is_string<typename RV::key_type>
    and is_sus<typename RV::value_type> and is_string<typename RV::value_type::value_type>
auto bust_map(const C& container) -> RV
{ RV rv;

  for (auto it { container.cbegin() }; it != container.cend(); ++it)
  { for (auto it2 { next(it) }; it2 != container.cend(); ++it2)
    { if (is_bust(*it, *it2))
      { rv[*it] += *it2;
        rv[*it2] += *it;
      }
    }
  }

  return rv;
}

constexpr long unsigned int STR_HASH(const char* str, int off = 0) 
  { return !str[off] ? 5381 : (STR_HASH(str, off + 1) * 33) ^ str[off]; }                                                                                

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(string_function_error);     ///< string_function error

#endif    // STRING_FUNCTIONS_H
