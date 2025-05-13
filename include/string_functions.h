// $Id: string_functions.h 266 2025-04-07 22:34:06Z  $

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
#include <charconv>
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
std::vector<std::string> from_csv(const std::string_view line);

/*! \brief      Duplicate a particular character within a string
    \param  s   string in which characters are to be duplicated
    \param  c   character to be duplicated
    \return     <i>s</i>, modified so that every instance of <i>c</i> is doubled
*/
std::string duplicate_char(const std::string_view s, const char c = '"');

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

/*! \brief      Generic conversion from string_view
    \param  sv  string_view
    \return     <i>sv</i> converted to type <i>T</i>

    https://stackoverflow.com/questions/68615212/how-to-constrain-a-template-six-different-usages-of-stdenable-if
*/
template <class T>
T from_string(const std::string_view sv)
{ T rv { };

  std::from_chars(sv.data(), sv.data() + sv.size(), rv);

  return rv;
}

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

/// an explicit no-op
inline std::string from_string(const std::string& s)
  { return s; }

/*! \brief      Generic conversion from C-style char*
    \param  cp  pointer to start of C string
    \return     <i>cp</i> converted to type <i>T</i>
*/
template <class T>
inline T from_string(const char* cp)
  { return from_string<T>(std::string_view { cp, strlen(cp) }); }

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
inline void auto_from_string(const std::string_view s, T& var)
  { var = from_string<T>(s); }

/*! \brief          Generic conversion to string
    \param  val     value to convert
    \return         <i>val</i>converted to a string
*/
template <class T>
std::string to_string(const T val)
  requires std::is_integral_v<T>
{ std::array<char, 128> arr;

  if (auto [ptr, err_code] = std::to_chars(arr.data(), arr.data() + arr.size(), val); err_code == std::errc())
    return std::string { arr.data(), ptr };

  return ""s;
}

/*! \brief      Generic conversion to string, for parameters that can be trivially converted
    \param  s   value to convert
    \return     <i>s</i>converted to a string
*/
template <class T>
inline T to_string(const std::convertible_to<std::string> auto& s)
  { return s; }

/*! \brief      Convert a single char to a string
    \param  c   char to convert
    \return     <i>c</i>converted to a string
*/
inline std::string to_string(const char c)
  { return std::string { c }; }

// catchall
template <class T>
  requires (!is_string<T> and !std::is_integral_v<T>)
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

/*! \brief      Append a string_view to a string
    \param  s   original string
    \param  sv  string_view to append
    \return     concatenation of <i>s</i> and <i>sv</i>
*/
  std::string operator+(const std::string& s, const std::string_view sv);

/*! \brief      Append a character to a string_view
    \param  sv  original string
    \param  c   character to append
    \return     concatenation of <i>sv</i> and <i>c</i>
*/
  std::string operator+(const std::string_view sv, const char c);

/*! \brief              Safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \param  length      length of substring to be extracted
    \return             substring of length <i>length</i>, starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn, length)</i>, except does not throw a range exception.
    Compiler error if one uses str.size() - start_posn as a default length; value may not be calculated from other parameters
*/
template <typename STYPE>
inline auto substring(const std::string_view str, const size_t start_posn, const size_t length) -> STYPE
{ if (length == 0)
    return STYPE { EMPTY_STR };

  return ( (str.size() > start_posn) ? STYPE {str.substr(start_posn, length)} : STYPE { EMPTY_STR } );
}

/*! \brief              Safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \return             substring starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn)</i>, except does not throw a range exception
*/
template <typename STYPE>
inline auto substring(const std::string_view str, const size_t start_posn) -> STYPE
  { return substring <STYPE> (str, start_posn, str.size() - start_posn); }

/*! \brief              Replace every instance of one character with another
    \param  s           string on which to operate
    \param  old_char    character to be replaced
    \param  new_char    replacement character
    \return             <i>s</i>, with every instance of <i>old_char</i> replaced by <i>new_char</i>
*/
std::string replace_char(const std::string_view s, const char old_char, const char new_char);

/*! \brief              Replace every instance of one string with another
    \param  s           string on which to operate
    \param  old_str     string to be replaced
    \param  new_str     replacement string
    \return             <i>s</i>, with every instance of <i>old_str</i> replaced by <i>new_str</i>
*/
std::string replace(const std::string_view s, const std::string_view old_str, const std::string_view new_str);

/*! \brief              Replace part of a string with a byte-for-byte copy of an object
    \param  s           string on which to operate
    \param  start_posn  position at substitution begins
    \param  value       bytes that are to be placed into <i>s</i>
    \return             <i>s</i>, overwriting characters starting at <i>start_posn</i> with the bytes of <i>value</i>

    Will not return a string of length greater than <i>s</i>; will truncate to that length if necessary
*/
template <typename T>
std::string replace_substring(const std::string_view s, const size_t start_posn, const T& value)
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
inline bool contains(const std::string_view s, const std::string_view ss)
  { return s.find(ss) != std::string_view::npos; }

/*! \brief          Does a string contain a particular substring at a particular location?
    \param  s       string to test
    \param  ss      substring for which to search
    \param  posn    putative start position of <i>ss</i>
    \return         whether <i>s</i> contains the substring <i>ss</i>, starting at position <i>posn</i>
*/
inline bool contains_at(std::string_view s, std::string_view ss, const size_t posn)
  { return (s.length() >= posn + ss.length()) and (substring <std::string_view> (s, posn, ss.length()) == ss); }

/*! \brief      Does a string contain a particular character?
    \param  s   string to test
    \param  c   character for which to search
    \return     whether <i>s</i> contains the character <i>c</i>
*/
inline bool contains(const std::string_view s, const char c)
  { return s.find(c) != std::string_view::npos; }

/*! \brief          Does a vector of strings contain an element that is equivalent to a particular string_view?
    \param  strvec  vector of strings to test
    \param  sv      string_view for which to search
    \return         whether <i>strvec</i> contains the string_view <i>sv</i>
*/
inline bool contains(const std::vector<std::string>& strvec, const std::string_view sv)
  { return ANY_OF(strvec, [sv] (const std::string& str) { return (str == sv); }); }

/*! \brief          Does a string contain a particular character at a particular location?
    \param  s       string to test
    \param  c       character for which to search
    \param  posn    location to test
    \return         whether <i>s</i> contains the character <i>c</i> at position <i>posn</i>
*/
inline bool contains_at(const std::string_view s, const char c, const size_t posn)
  { return (s.length() > posn) and (s[posn] == c); }

/*! \brief          Does a string contain any letters?
    \param  str     string to test
    \return         whether <i>str</i> contains any letters

    This should give the correct result in any locale
*/
inline bool contains_letter(const std::string_view str)
  { return ANY_OF(str, [] (const char c) { return ((c >= 'A' and c <= 'Z') or (c >= 'a' and c <= 'z')); } ); }

/*! \brief          Does a string contain any upper case letters?
    \param  str     string to test
    \return         whether <i>str</i> contains any upper case letters
*/
inline bool contains_upper_case_letter(const std::string_view str)
  { return ANY_OF(str, [] (const char c) { return (c >= 'A' and c <= 'Z'); } ); }

/*! \brief          Does a string contain any digits?
    \param  str     string to test
    \return         whether <i>str</i> contains any digits
*/
inline bool contains_digit(const std::string_view str)
  { return ANY_OF(str, [] (const char c) { return isdigit(c); } ); }

/*! \brief          Return the first digit in a string
    \param  sv      string to test
    \param  c       character to return if no digit is present in <i>sv</i>
    \return         the first digit in <i>sv</i>, or <i>c</i>
*/
char first_digit(const std::string_view sv, const char c = ' ');

/*! \brief          Does a string contain only digits?
    \param  str     string to test
    \return         whether <i>str</i> comprises only digits
*/
inline bool is_digits(const std::string_view str)
  { return ALL_OF(str, [] (const char c) { return isdigit(c); } ); }

/*! \brief      Is a character a digit?
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
std::string pad_string(const std::string_view s, const size_t len, const enum PAD pad_side = PAD::LEFT, const char pad_char = ' ');

/*! \brief              Left pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_char    character with which to pad
    \return             left padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
inline std::string pad_left(const std::string_view s, const size_t len, const char pad_char = ' ')
  { return pad_string(s, len, PAD::LEFT, pad_char); }

  /*! \brief            Left pad a number to a particular size
    \param  s           original number
    \param  len         length of returned string
    \param  pad_char    character with which to pad
    \return             left padded version of string of <i>s</i>

    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
template <typename T>
  requires (std::is_integral_v<T>)
inline std::string pad_left(T s, const size_t len, const char pad_char = ' ')
  { return pad_left(std::string_view { to_string(s) }, len, pad_char); }

/*! \brief      Left pad an integer or string type with zeroes to a particular size
    \param  s   integer value, or a string
    \return     left padded version of <i>s</i> rendered as a string and left padded with zeroes
*/
template <typename T>
  requires (std::is_integral_v<T> or is_string<base_type<T>>)
inline std::string pad_leftz(const T& s, const size_t len)
  { return pad_left(std::string_view { to_string(s) }, len, '0'); }

/*! \brief      Left pad a string with zeroes to a particular size
    \param  sv  string to be left-padded
    \return     left padded version of <i>sv</i> rendered as a string and left padded with zeroes
*/
inline std::string pad_leftz(const std::string_view sv, const size_t len)
  { return pad_left(sv, len, '0'); }

/*! \brief              Right pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_char    character with which to pad
    \return             right padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
inline std::string pad_right(const std::string_view s, const size_t len, const char pad_char = ' ')
  { return pad_string(s, len, PAD::RIGHT, pad_char); }

/*! \brief            Right pad an integer type to a particular size
    \param  s         integer value
    \param  len       length of returned string
    \param  pad_char  character with which to pad
    \return           right-padded version of <i>s</i> rendered as a string
*/
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
std::string read_file(const std::string_view filename);

/*! \brief              Read the contents of a file into a single string
    \param  path        the different directories to try, in order
    \param  filename    name of file to be read
    \return             contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
std::string read_file(const std::vector<std::string>& path, const std::string_view filename);

/*! \brief              Read the contents of a file into a single string
    \param  filename    name of file to be read
    \param  path        the different directories to try, in order
    \return             contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
inline std::string read_file(const std::string_view filename, const std::vector<std::string>& path)
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
template <typename T>
inline bool starts_with(const std::string_view cs, const T& ss)
  requires ( (is_string<typename T::value_type>) or (is_string_view<typename T::value_type>) )
  { return ANY_OF(ss, [cs] (const std::string_view str) { return cs.starts_with(str); }); }

/*! \brief      Remove specific string from the start of a string if it is present
    \param  s   original string
    \param  ss  substring to remove
    \return     <i>s</i> with <i>ss</i> removed if it is present at the start of the string
*/
template <typename STYPE>
auto remove_from_start(const std::string_view s, const std::string_view ss) -> STYPE
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
template <typename STYPE>
//inline auto remove_from_end(std::string_view s, const unsigned int n) -> STYPE
inline auto remove_chars_from_end(const std::string_view s, const unsigned int n) -> STYPE
  { return ( (n >= s.length()) ? STYPE { EMPTY_STR } : STYPE { s.substr(0, s.length() - n) } ); }

/*! \brief      Remove characters if present at the end of a string
    \param  s   original string
    \param  e   string to remove
    \return     <i>s</i> with the <i>e</i> removed, if it was present

    If <i>e</i> is not present, just returns <i>s</i>
*/
template <typename STYPE>
//inline auto remove_from_end(std::string_view s, std::string_view e) -> STYPE
inline auto remove_string_from_end(const std::string_view s, const std::string_view e) -> STYPE
  { return ( s.ends_with(e) ? remove_chars_from_end <STYPE> (s, e.length()) : STYPE { s } ); }

/*! \brief      Remove character if present at the end of a string
    \param  s   original string
    \param  c   character to remove
    \return     <i>s</i> with the <i>c</i> removed, if it was present

    If <i>c</i> is not present, just returns <i>s</i>
*/
template <typename STYPE>
inline auto remove_char_from_end(const std::string_view s, const char c) -> STYPE
  { return ( s.ends_with(c) ? remove_chars_from_end <STYPE> (s, 1u) : STYPE { s } ); }

/*! \brief      Remove all instances of a specific leading character
    \param  cs  original string
    \param  c   leading character to remove (if present)
    \return     <i>cs</i> with any leading octets with the value <i>c</i> removed
*/
template <typename STYPE>
auto remove_leading(const std::string_view cs, const char c) -> STYPE
{ if (cs.empty())
    return STYPE { };

  const size_t posn { cs.find_first_not_of(c) };

  return ( (posn == std::string_view::npos) ? STYPE { } : substring <STYPE> (cs, posn) );
}

/*! \brief      Remove leading spaces
    \param  cs  original string
    \return     <i>cs</i> with any leading spaces removed
*/
template <typename STYPE>
inline auto remove_leading_spaces(const std::string_view cs) -> STYPE
  { return remove_leading <STYPE> (cs, ' '); }

/*! \brief      Remove all instances of a specific trailing character
    \param  cs  original string
    \param  c   trailing character to remove (if present)
    \return     <i>cs</i> with any trailing octets with the value <i>c</i> removed
*/
template <typename STYPE>
auto remove_trailing(std::string_view cs, const char c) -> STYPE
{ STYPE rv { cs };

  while ( rv.length() and (rv[rv.length() - 1] == c) )
    rv = rv.substr(0, rv.length() - 1);
  
  return rv;
}

/*! \brief      Remove trailing spaces
    \param  cs  original string
    \return     <i>cs</i> with any trailing spaces removed
*/
template <typename STYPE>
inline auto remove_trailing_spaces(std::string_view cs) -> STYPE
  { return remove_trailing <STYPE> (cs, ' '); }

/*! \brief      Remove trailing instances of a particular character, for all strings in a container
    \param  t   original container of strings
    \param  c   char to remove
    \return     a vector of strings or string_views based on <i>t</i>with all trailing instances of <i>c</i> removed
*/
template <typename STYPE, typename T>
  requires is_vector<T> and is_ssv<typename T::value_type>
auto remove_trailing_chars(const T& t, const char c) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  FOR_ALL(t, [c, &rv] (const auto& s) { rv += remove_trailing <STYPE> (s, c); } );

  return rv;
}

/*! \brief      Remove trailing instances of a particular character, for all strings in a container
    \param  t   original container of strings
    \param  c   char to remove
    \return     a vector of strings or string_views based on <i>t</i>with all trailing instances of <i>c</i> removed
*/
template <typename STYPE, typename T>
//  requires ( is_vector<T> and ((is_string<typename T::value_type>) or (is_string_view<typename T::value_type>)) )
  requires is_vector<T> and is_ssv<typename T::value_type>
inline auto remove_trailing(const T& t, const char c) -> std::vector<STYPE>
  { return remove_trailing_chars <STYPE> (t, c); }

/*! \brief      Remove trailing spaces, for all strings in a container
    \param  t   original container of strings
    \return     a vector of strings or string_views based on <i>t</i>with all trailing spaces removed
*/
template <typename STYPE, typename T>
  requires is_vector<T> and is_ssv<typename T::value_type>
inline auto remove_trailing_spaces(const T& t) -> std::vector<STYPE>
  { return remove_trailing <STYPE> (t, ' '); }

/*! \brief      Remove leading and trailing instances of a particular character
    \param  cs  original string
    \param  c   character to remove
    \return     <i>cs</i> with any leading or trailing instances of <i>c</i> removed
*/
template <typename STYPE>
inline auto remove_peripheral_chars(const std::string_view cs, const char c) -> STYPE
  { return remove_trailing <STYPE> (remove_leading <std::string_view> (cs, c), c); }

/*! \brief      Remove leading and trailing spaces
    \param  cs  original string
    \return     <i>cs</i> with any leading or trailing spaces removed
*/
template <typename STYPE>
inline auto remove_peripheral_spaces(const std::string_view cs) -> STYPE
  { return remove_peripheral_chars <STYPE> (cs, ' '); }

/*! \brief      Remove leading and trailing instances of a particular character from each element in a vector of strings
    \param  t   container of strings
    \param  c   character to remove
    \return     <i>t</i> with leading and trailing instances of <i>c</i> removed from the individual elements
*/
template <typename STYPE, typename T>
  requires is_vector<T> and is_ssv<typename T::value_type>
auto remove_peripheral_chars(const T& t, const char c) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  FOR_ALL(t, [c, &rv] (const auto& s) { rv += remove_peripheral_chars <STYPE> (s, c); } );

  return rv;
}

/*! \brief      Remove leading and trailing spaces from each element in a vector of strings
    \param  t   container of strings
    \return     <i>t</i> with leading and trailing spaces removed from the individual elements

    NB There should be specialisation of this for vectors that uses reserve()
*/
template <typename STYPE, typename T>
  requires is_vector<T> and is_ssv<typename T::value_type>
inline auto remove_peripheral_spaces(const T& t) -> std::vector<STYPE>
  { return remove_peripheral_chars <STYPE> (t, ' '); }

/*! \brief      Remove leading and trailing spaces from each element in a container of strings
    \param  t   container of strings
    \return     <i>t</i> with leading and trailing spaces removed from the individual elements

    NB There should be specialisation of this for vectors that uses reserve()
*/
template <typename STYPE, typename T>
  requires (is_string<typename T::value_type>) and (is_string<STYPE>)
auto remove_peripheral_spaces(T&& t) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  FOR_ALL(std::forward<T>(t), [&rv] (const std::string& s) { rv += remove_peripheral_spaces <std::string> (s); } );

  return rv;
}

/*! \brief              Split a string into components
    \param  cs          original string
    \param  separator   separator string (typically a single character)
    \return             vector containing the separate components
*/
template <typename STYPE>
auto split_string(const std::string_view cs, const std::string_view separator) -> std::vector<STYPE>
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

    Some of the returned elements may be the null string
*/
template <typename STYPE>
auto split_string(const std::string_view cs, const char separator = ',') -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  if (cs.empty())
    return rv;

  std::vector<size_t> posns;

  for (size_t n = 0; n < cs.size(); ++n)
    if (cs[n] == separator)
      posns += n;

  if (posns.empty())      // no separators; return the string as a single-element vector
  { rv += cs;

    return rv;
  }

  size_t start_posn { 0 };

  for (size_t posn_nr { 0 }; posn_nr < posns.size(); ++posn_nr)
  { rv += substring <STYPE> (cs, start_posn, posns[posn_nr] - start_posn);

    start_posn = posns[posn_nr] + 1;
  }


  rv += substring <STYPE> (cs, posns[posns.size() - 1] + 1);   // all the text after the last separator

  return rv;
}

/*! \brief              Split a string into records
    \param  cs          original string
    \param  eor_marker  character that marks the end of a record
    \param  delim_rule  whether to keep or drop the <i>eor_marker</i> in the returned vector
    \return             vector containing the separate records
*/
template <typename STYPE>
auto split_string_into_records(const std::string_view cs, const char eor_marker = '|', const enum DELIMITERS delim_rule = DELIMITERS::DROP) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  if (cs.empty())
    return rv;

  std::vector<size_t> posns;

// build an array of positions of the eor marker
  for (size_t n { 0 }; n < cs.size(); ++n)
    if (cs[n] == eor_marker)
      posns += n;

  if (posns.empty())      // no records; return an empty vector
    return rv;

  size_t start_posn { 0 };

  for (size_t posn_nr { 0 }; posn_nr < posns.size(); ++posn_nr)
  { rv += substring <STYPE> (cs, start_posn, posns[posn_nr] - start_posn + ( (delim_rule == DELIMITERS::KEEP) ? 1 : 0) );

    start_posn = posns[posn_nr] + 1;
  }

  return rv;
}

/*! \brief              Split a string into components, and remove peripheral spaces from each component
    \param  cs          original string
    \param  separator   separator string (typically a single character)
    \return             vector containing the separate components, with peripheral spaces removed
*/
template <typename STYPE>
inline auto clean_split_string(const std::string_view cs,const std::string_view separator) -> std::vector<STYPE>
  { return remove_peripheral_spaces(split_string <STYPE> (cs, separator)); }

/*! \brief              Split a string into components, and remove peripheral spaces from each component
    \param  cs          original string
    \param  separator   separator character
    \return             vector containing the separate components, with peripheral spaces removed
*/
template <typename STYPE>
inline auto clean_split_string(const std::string_view cs, const char separator = ',') -> std::vector<STYPE>
  { return remove_peripheral_spaces <STYPE> (split_string <STYPE> (cs, separator)); }

/*! \brief      Squash repeated occurrences of a character
    \param  cs  original string
    \param  c   character to squash
    \return     <i>cs</i>, but with all consecutive instances of <i>c</i> converted to a single instance
*/
std::string squash(const std::string_view cs, const char c = ' ');

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
template <typename STYPE>
inline auto to_lines(const std::string_view cs, const std::string_view eol_marker = EOL) -> std::vector<STYPE>
  { return split_string <STYPE> (cs, eol_marker); }

/*! \brief      Remove peripheral instances of a specific character
    \param  cs  original string
    \param  c   character to remove
    \return     <i>cs</i> with any leading or trailing instances of <i>c</i> removed
*/
template <typename STYPE>
inline auto remove_peripheral_character(const std::string_view cs, const char c) -> STYPE
  { return remove_trailing <STYPE> (remove_leading <std::string_view> (cs, c), c); }

/*! \brief                  Remove all instances of a particular char from a string
    \param  cs              original string
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
std::string remove_char(const std::string_view cs, const char char_to_remove);
  
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
std::string remove_chars(const std::string_view s, const std::string_view chars_to_remove);

/*! \brief                  Remove all instances of a particular char from all delimited substrings
    \param  cs              original string
    \param  char_to_remove  character to be removed from delimited substrings in <i>cs</i>
    \param  delim_1         opening delimiter
    \param  delim_2         closing delimiter
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed from inside substrings delimited by <i>delim_1</i> and <i>delim_2</i>
*/
std::string remove_char_from_delimited_substrings(const std::string_view cs, const char char_to_remove, const char delim_1, const char delim_2);

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
template <typename STYPE>
auto delimited_substring(const std::string_view cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters) -> STYPE
{ const size_t delim_1_posn { cs.find(delim_1) };
  
  if (delim_1_posn == std::string_view::npos)
    return STYPE { EMPTY_STR };
  
  const size_t delim_2_posn { cs.find(delim_2, delim_1_posn + 1) };
  
  if (delim_2_posn == std::string_view::npos)
    return STYPE { EMPTY_STR };
  
  return ( (return_delimiters == DELIMITERS::DROP) ? STYPE { cs.substr(delim_1_posn + 1, delim_2_posn - delim_1_posn - 1) } : STYPE { cs.substr(delim_1_posn, delim_2_posn - delim_1_posn) } ) ;
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
template <typename STYPE>
auto delimited_substring(const std::string_view cs, const std::string_view delim_1, const std::string_view delim_2, const DELIMITERS return_delimiters) -> STYPE
{ const size_t delim_1_posn { cs.find(delim_1) };
  
  if (delim_1_posn == std::string_view::npos)
    return STYPE { EMPTY_STR };  
  
  const size_t length_to_skip { ( (return_delimiters == DELIMITERS::DROP) ? delim_1.length() : 0 ) };
  const size_t delim_2_posn   { cs.find(delim_2, delim_1_posn + length_to_skip) };
  
  if (delim_2_posn == std::string_view::npos)
    return STYPE { EMPTY_STR };
  
  const size_t length_to_return { (return_delimiters == DELIMITERS::DROP) ? delim_2_posn - delim_1_posn - delim_1.length()
                                                                          : delim_2_posn + delim_1_posn + delim_2.length()
                                };
  
  return STYPE { cs.substr(delim_1_posn + length_to_skip, length_to_return) };
}

/*! \brief                      Obtain all occurrences of a delimited substring
    \param  cs                  original string
    \param  delim_1             opening delimiter
    \param  delim_2             closing delimiter
    \param  return_delimiters   whether to keep delimiters in the returned value
    \return                     all substrings between <i>delim_1</i> and <i>delim_2</i>
*/
template <typename STYPE>
auto delimited_substrings(const std::string_view cs, const std::string_view delim_1, const std::string_view delim_2, const DELIMITERS return_delimiters) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  size_t cs_start_posn { 0 };

  while (!substring <std::string_view> (cs, cs_start_posn).empty())
  { std::string_view sstring { substring <std::string_view> (cs, cs_start_posn) };
    
    const size_t delim_1_posn { sstring.find(delim_1) };

    if (delim_1_posn == std::string_view::npos)             // no more starting delimiters
      return rv;

    const size_t delim_2_posn { sstring.find(delim_2, delim_1_posn + delim_1.length()) };

    if (delim_2_posn == std::string_view::npos)
      return rv;                            // no more ending delimiters

    if (return_delimiters == DELIMITERS::DROP)
      rv += sstring.substr(delim_1_posn + delim_1.length(), delim_2_posn - delim_1_posn - delim_1.length());
    else
      rv += sstring.substr(delim_1_posn, delim_2_posn + delim_2.length() - delim_1_posn);
      
    cs_start_posn += (delim_2_posn + delim_2.length());
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
template <typename STYPE>
auto delimited_substrings(const std::string_view cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters) -> std::vector<STYPE>
{ std::vector<STYPE> rv;

  size_t start_posn { 0 };      // start posn is, and remains global (i.e., wrt cs)

  while ( (start_posn < cs.length() and !substring <std::string> (cs, start_posn).empty()) )  // initial test so substring() doesn't write to output
  { std::string_view sstring { substring <std::string_view> (cs, start_posn) };

    const size_t delim_1_posn  { sstring.find(delim_1) };

    if (delim_1_posn == std::string_view::npos)             // no more starting delimiters
      return rv;

    const size_t delim_2_posn { sstring.find(delim_2, delim_1_posn + 1) };

    if (delim_2_posn == std::string_view::npos)
      return rv;                            // no more ending delimiters

    if (return_delimiters == DELIMITERS::KEEP)
      rv += sstring.substr(delim_1_posn, delim_2_posn - delim_1_posn);
    else
      rv += sstring.substr(delim_1_posn + 1, delim_2_posn - delim_1_posn - 1);

    start_posn += (delim_2_posn + 1);   // remember, start_posn is global
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
std::string centred_string(const std::string_view str, const unsigned int width);

/*! \brief      Get the last character in a string
    \param  cs  source string
    \return     last character in <i>cs</i>

    Throws exception if <i>cs</i> is empty
*/
char last_char(const std::string_view cs);

/*! \brief      Get the penultimate character in a string
    \param  cs  source string
    \return     penultimate character in <i>cs</i>

    Throws exception if <i>cs</i> is empty or contains only one character
*/
char penultimate_char(const std::string_view cs);

/*! \brief      Get the antepenultimate character in a string
    \param  cs  source string
    \return     antepenultimate character in <i>cs</i>

    Throws exception if <i>cs</i> contains fewer than two characters
*/
char antepenultimate_char(const std::string_view cs);

/*! \brief          Get the terminating part of a string
    \param  cs      original string
    \param  n       number of characters to return
    \return         the last <i>n</i> characters of <i>cs</i>
*/
template <typename STYPE>
inline auto last(const std::string_view cs, unsigned int n) -> STYPE
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
std::string transform_string(const std::string_view cs, int (*pf) (int));

/*! \brief      Convert string to upper case
    \param  cs  original string
    \return     <i>cs</i> converted to upper case
*/
inline std::string to_upper(const std::string_view cs)
  { return transform_string(cs, std::toupper); }

/*! \brief      Convert string to lower case
    \param  cs  original string
    \return     <i>cs</i> converted to lower case
*/
inline std::string to_lower(const std::string_view cs)
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
  requires std::is_integral_v<T>
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
  requires std::is_integral_v<T>
inline std::string comma_separated_string(const T n)
  { return separated_string(n); }

/*! \brief      Convert an integer to a comma-separated string
    \param  n   number to convert
    \return     <i>n</i> with the separator <i>,</i> separating each triplet
*/
template <typename T>
  requires std::is_integral_v<T>
inline std::string css(const T n)
  { return comma_separated_string(n); }

/*! \brief                  Get location of start of next word
    \param  str             string which the next word is to be found
    \param  current_posn    position from which to search
    \return                 position of start of next word, beginning at position <i>current_posn</i> in <i>str</i>

    Returns <i>string::npos</i> if no word can be found
*/
size_t next_word_posn(const std::string_view str, const size_t current_posn);

/*! \brief      Get location of start all words
    \param  s   string to be analysed
    \return     positions of all the starts of words in <i>s</i>
*/
std::vector<size_t> starts_of_words(const std::string_view s);

/*! \brief          Get nth word in a string
    \param  s       string to be analysed
    \param  n       word number to be returned
    \param  wrt     value with respect to which <i>n</i> is counted
    \return         the <i>n</i>th word, counting with respect to <i>wrt</i>

    Returns <i>string::npos</i> if there is no <i>n</i>th word
*/
std::string nth_word(const std::string_view s, const unsigned int n, const unsigned int wrt = 0);

/*! \brief          Get the actual length, in bytes, of a UTF-8-encoded string
    \param  str     UTF-8 string to be analysed
    \return         number of bytes occupied by <i>str</i>

    See: https://stackoverflow.com/questions/4063146/getting-the-actual-length-of-a-utf-8-encoded-stdstring
    TODO: generalise using locales/facets, instead of assuming UTF-8
*/
size_t n_chars(const std::string& str);

/*! \brief      Does a string contain a legal dotted-decimal IPv4 address
    \param  cs  string to test
    \return     whether <i>cs</i> contains a legal dotted decimal IPv4 address
*/
bool is_legal_ipv4_address(const std::string_view cs);

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
inline bool is_legal_value(const std::string_view value, const std::string_view legal_values, const std::string_view separator)
  { return contains( split_string <std::string_view> (legal_values, separator), value ); }

/*! \brief                  Is a string a legal value from a list?
    \param  value           target string
    \param  legal_values    all the legal values, separated by <i>separator</i>
    \param  separator       separator in the string <i>legal_values</i>
    \return                 whether <i>value</i> appears in <i>legal_values</i>
*/
inline bool is_legal_value(const std::string_view value, const std::string_view legal_values, const char separator = ',')
  { return (contains(split_string <std::string_view> (legal_values, separator), value)); }

/*! \brief          Is one call earlier than another, according to callsign sort order?
    \param  call1   first call
    \param  call2   second call
    \return         whether <i>call1</i> appears before <i>call2</i> in callsign sort order
*/
bool compare_calls(const std::string_view call1, const std::string_view call2);

/*! \brief          Is the value of one mult earlier than another?
    \param  mult1   first mult value
    \param  mult2   second mult value
    \return         whether <i>mult1</i> appears before <i>mult2</i> in displayed mult value sort order (used for exchange mults)
*/
bool compare_mults(const std::string_view mult1, const std::string_view mult2);

// ***** https://stackoverflow.com/questions/2620862/using-custom-stdset-comparator    *****

// *** https://www.fluentcpp.com/2017/06/09/search-set-another-type-key/ see discussion of is_transparent

// the old way:
// using CALL_SET = set<string, decltype(&compare_calls)>;     // set in callsign order
// then:
// CALL SET burble(compare_calls)

/*! \brief  structure to sort strings
    \param  PF  pointer to the function to perform the sorting
*/
template<bool (*PF)(const std::string_view, const std::string_view)>    // (sv, sv) => heterogeneous lookup automatically supported
struct CMP
{ using is_transparent = void;      // the magic incantation

  inline bool operator() (const std::string_view m1, const std::string_view m2) const
    { return PF(m1, m2); }
};

//using CALL_SET = std::set<std::string, CMP<compare_calls>>;
using CALL_SET = CUSTOM_STRING_SET<CMP<compare_mults>>;
//using MULT_SET = std::set<std::string, CMP<compare_mults>>;
using MULT_SET = CUSTOM_STRING_SET<CMP<compare_mults>>;

/*! \brief          Return a number with a particular number of decimal places
    \param  str     initial value
    \param  n       number of decimal places
    \return         <i>str</i> with <i>n</i> decimal places

    Assumes that <i>str</i> is a number
*/
std::string decimal_places(const std::string_view str, const int n);

/*! \brief        Return the longest string from a container of strings
    \param  strs  the strings to search
    \return       the longest string in the container <i>strs</i>
*/
template <typename T>
  requires is_container_of_strings<T>
std::string longest(const T& strs)
{ std::string rv { };

//  for (const auto& str : strs)
//    if (str.length() > rv.length())
//      rv = str;
  FOR_ALL(strs, [&rv] (const auto& str) { if (str.length() > rv.length())
                                            rv = str;
                                        });

  return rv;
}

/*! \brief        Return the longest string from a container of strings
    \param  strs  the strings to search
    \return       the longest string in the container <i>strs</i>
*/
template <typename T>
 requires is_container_of_strings<T>
std::string longest(T&& strs)
{ std::string rv;

  for (auto&& str : strs)
    if (str.length() > rv.length())
      rv = str;

  return rv;
}

/*! \brief        Erase a string from a container of strings
    \param  c     target container
    \param  str   string to erase

    This should be removed once .erase() has been implemented for string_view types in the GNU C++ library
*/
template <typename T>
 requires is_container_of_strings<T>
inline void STRC_ERASE(T& c, const std::string& str)
  { c.erase(str); }

/*! \brief        Erase a string from a container of strings
    \param  c     target container
    \param  str   atring to erase

    This should be removed once .erase() has been implemented for string_view types in the GNU C++ library
*/
template <typename T>
 requires is_container_of_strings<T>
inline void STRC_ERASE(T& c, const std::string_view sv)
{ if ( const auto posn { c.find(sv) }; posn != c.end() )
    c.erase(posn);
}

/*! \brief          Deal with wprintw's idiotic insertion of newlines when reaching the right hand of a window
    \param  str     string to be reformatted
    \param  width   width of line in destination window
    \return         <i>str</i> reformatted for the window

    See http://stackoverflow.com/questions/7540029/wprintw-in-ncurses-when-writing-a-newline-terminated-line-of-exactly-the-same
*/
std::string reformat_for_wprintw(const std::string_view str, const int width);

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
inline std::string remove_substring(const std::string_view cs, const std::string_view ss)
  { return ( contains(cs, ss) ? replace(cs, ss, std::string { }) : std::string { cs } ); }

/*! \brief      Create a string of spaces
    \param  n   length of string to be created
    \return     string of <i>n</i> space characters
*/
inline std::string space_string(const int n)
  { return (n > 0 ? create_string((char)32, n) : std::string { }); }

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
template <typename STYPE>
auto remove_trailing_comment(const std::string_view str, const std::string_view comment_str = "//"sv) -> STYPE
{ const size_t posn { str.find(comment_str) };

  return ( (posn == std::string_view::npos) ? STYPE { str } : remove_trailing_spaces <STYPE> (substring <std::string_view> (str, 0, posn)) );
}

/*! \brief              Add delimiters to a string
    \param  str         string
    \param  delim_1     opening delimiter
    \param  delim_2     closing delimiter
    \return             <i>str</i> preceded by <i>delim_1</i> and followed by <i>delim_2</i>
*/
//inline std::string delimit(const std::string_view str, const std::string& delim_1, const std::string& delim_2)
inline std::string delimit(const std::string_view str, const std::string_view delim_1, const std::string& delim_2)
  { return (std::string { delim_1 } + str + delim_2); }

/*! \brief              Perform a case-insensitive search for a substring
    \param  str         string to search
    \param  target      substring for which to search
    \param  start_posn  location in <i>str</i> at which to start the search
    \return             position of the first character in <i>target</i> in <i>str</i>
    
    Returns string::npos if <i>target</i> cannot be found
*/
size_t case_insensitive_find(const std::string_view str, const std::string_view target, const size_t start_posn = 0);

/*! \brief              Truncate a string immediately prior to the first occurrence of a particular character
    \param  str         string to truncate
    \param  c           target character
    \return             <i>str</i> truncated immediately prior to <i>c</i> (if <i>c</i> is present; otherwise <i>str</i>)
*/
template <typename STYPE>
inline auto truncate_before_first(const std::string_view str, const char c) -> STYPE
  { return (substring <STYPE> (str, 0, str.find(c))); }

/*! \brief              Remove up to after the first occurrence of a particular character
    \param  str         string to truncate
    \param  c           target character
    \return             <i>str</i> truncated immediately after <i>c</i> (if <i>c</i> is present; otherwise <i>str</i>)
*/
template <typename STYPE>
inline auto after_first(const std::string_view str, const char c) -> STYPE
  { return (substring <STYPE> (str, str.find(c) + 1)); }

/*! \brief          Return position in a string at the end of a target string, if present
    \param  str     string to search
    \param  target  string to find
    \return         position in <>str</i> after the end of <i>target</i>

    Returns string::npos if <i>target</i> is not a substring of <i>str</i> OR if <i>target</i>
    is the conclusion of <i>str</i>
*/
size_t find_and_go_to_end_of(const std::string_view str, const std::string_view target);

/*! \brief              Get the base portion of a call
    \param  callsign    original callsign
    \return             the base portion of <i>callsign</i>

    For example, a call such as VP9/G4AMJ/P returns G4AMJ.
*/
std::string base_call(const std::string_view callsign);

/*! \brief      Provide a formatted date string: YYYYMMDD
    \return     current UTC date in the format: YYYYMMDD
*/
std::string YYYYMMDD_utc(void);

/*! \brief      Remove all instances of several substrings (sequentially) from a string
    \param  cs  original string
    \param  vs  vector of substrings to be removed, in the order in which they are to be removed
    \return     <i>cs</i>, with all instances of the elements of <i>vs</i> removed, applied in order
*/
std::string remove_substrings(const std::string_view cs, const std::vector<std::string>& vs);

/*! \brief              Return all strings from a container that match a particular regular expression string
    \param  container   container of strings
    \param  s           regular expression string
    \return             All the elements of <i>container</i> that match <i>s</i>

    Can't use string_view as regex does not support heterogeneous lookup
*/
template <class T, class C>
T regex_matches(C&& container, const std::string& s)        // it seems that std::regex(param) does not support a string_view param(!)
//T regex_matches(C&& container, const std::string_view s)
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
inline size_t number_of_occurrences(const std::string_view str, const char c)
  { return static_cast<size_t>(std::count(str.begin(), str.end(), c)); }

/*! \brief          Are two calls busts of each other?
    \param  call1   first call
    \param  call2   second call
    \return         whether <i>call1</i> and <i>call2</i> are possible busts of each other

    Testing for busts is a symmetrical function.
*/
bool is_bust_call(const std::string_view call1, const std::string_view call2) noexcept;

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

/*! \brief        Return a string that contains only normal, printable ASCII characters
    \param  str   string to convert
    \return       <i>str</i> but with all non-printable ASCII characters converted to human-readable binary values
*/
std::string to_printable_string(const std::string_view str);

/// concatenate two string_views
inline std::string operator+(const std::string_view sv1, const std::string_view sv2)
  { return std::string(sv1) + sv2; }

// concatenate a string with a string_view
inline std::string operator+(const std::string_view sv1, const std::string& s2)
  { return std::string(sv1) + s2; }

/*! \brief                Read an istream until a string delimiter is reached
    \param  in            istream from which to read
    \param  delimiter     the delimiter that mars the end of a record
    \param  keep_or_drop  whether to keep or drop the delimiter in the returned string
    \return               the contents if <i>in</i> from the current point to the delimiter

    https://stackoverflow.com/questions/41851454/reading-a-iostream-until-a-string-delimiter-is-found
*/
std::string read_until(std::istream& in, const std::string_view delimiter, const DELIMITERS keep_or_drop = DELIMITERS::DROP);

/*! \brief          Convert string to hex characters
    \param  str     string to convert
    \return         <i>str</i> as a series of hex characters
*/
std::string hex_string(const std::string_view str);

/// a standard hash function for strings (the DJB function)
//constexpr long unsigned int STR_HASH(const char* str, int off = 0)
//  { return !str[off] ? 5381 : (STR_HASH(str, off + 1) * 33) ^ str[off]; }

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(string_function_error);     ///< string_function error

#endif    // STRING_FUNCTIONS_H
