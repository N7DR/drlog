// $Id: string_functions.h 175 2020-12-06 17:44:13Z  $

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
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <experimental/string_view>
#include <vector>

#include <time.h>

using namespace std::literals::string_literals;
using namespace std::experimental::literals::string_view_literals;

// the next five are defined in string_functions.cpp
extern const std::string EOL;           ///< end-of-line marker as string
extern const char        EOL_CHAR;      ///< end-of-line marker as character

extern const std::string  LF;           ///< LF as string
extern const std::string& LF_STR;       ///< LF as string
extern const char         LF_CHAR;      ///< LF as character

static const std::string  CR       { "\r"s };       ///< CR as string
static const std::string& CR_STR   { CR };         ///< CR as string
static const char         CR_CHAR  { '\r' };       ///< CR as character

static const std::string CRLF      { "\r\n"s };     ///< CR followed by LF

static const std::string EMPTY_STR { };          ///< an empty string
static const std::string FULL_STOP { "."s };       ///< full stop as string
static const std::string SPACE_STR { " "s };       ///< space as string

static const std::string CALLSIGN_CHARS                { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/"s };    ///< convenient place to hold all characters that are legal in callsigns
static const std::string DIGITS                        { "0123456789"s };                               ///< convenient place to hold all digits
static const std::string DIGITS_AND_UPPER_CASE_LETTERS { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"s };     ///< convenient place to hold all digits and upper case letters
static const std::string UPPER_CASE_LETTERS            { "ABCDEFGHIJKLMNOPQRSTUVWXYZ"s };               ///< convenient place to hold all upper case letters

constexpr char SPACE_CHAR { ' ' };

//constexpr bool INCLUDE_SECONDS { true };             ///< whether to include seconds in date_time_string()
  
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
std::vector<std::string> from_csv(std::experimental::string_view line);

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
//std::string date_time_string(const bool include_seconds = !INCLUDE_SECONDS);
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
  requires is_string_v<T>
inline std::string to_string(const T& val)
  { return val; }

/*! \brief              Safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \param  length      length of substring to be extracted
    \return             substring of length <i>length</i>, starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn, length)</i>, except does not throw a range exception.
    Compiler error if one uses str.size() - start_posn as a default length; value may not be calculated from other parameters
*/
std::string substring(const std::string& str, const size_t start_posn, const size_t length);

/*! \brief              Safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \return             substring starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn)</i>, except does not throw a range exception
*/
inline std::string substring(const std::string& str, const size_t start_posn)
  { return substring(str, start_posn, str.size() - start_posn); }

/*! \brief              Replace every instance of one character with another
    \param  s           string on which to operate
    \param  old_char    character to be replaced
    \param  new_char    replacement character
    \return             <i>s</i>, with every instance of <i>old_char</i> replaced by <i>new_char</i>
*/
std::string replace_char(const std::string& s, char old_char, char new_char);

/*! \brief              Replace every instance of one string with another
    \param  s           string on which to operate
    \param  old_str     string to be replaced
    \param  new_str     replacement string
    \return             <i>s</i>, with every instance of <i>old_str</i> replaced by <i>new_str</i>
*/
std::string replace(const std::string& s, const std::string& old_str, const std::string& new_str);

/*! \brief              Replace part of a string with a byte-for-byte copy of an object
    \param  s           string on which to operate
    \param  start_posn  position at substitution begins
    \param  value       bytes that are to be placed into <i>s</i>
    \return             <i>s</i>, overwriting characters starting at <i>start_posn</i> with the bytes of <i>value</i>

    Will not return a string of length greater than <i>s</i>; will truncate to that length if necessary
*/
template <typename T>
[[nodiscard]] std::string replace_substring(const std::string& s, const size_t start_posn, const T& value)
{ std::string rv { s };

  constexpr size_t value_size { sizeof(value) };

  u_char* cp { (u_char*)(&value) };

  for (size_t n = 0; n < value_size; ++n)
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
inline bool contains(const std::string& s, const std::string& ss)
  { return s.find(ss) != std::string::npos; }

/*! \brief      Does a string contain a particular character?
    \param  s   string to test
    \param  c   character for which to search
    \return     whether <i>s</i> contains the character <i>c</i>
*/
inline bool contains(const std::string& s, const char c)
  { return s.find(c) != std::string::npos; }

/*! \brief          Does a string contain any letters?
    \param  str     string to test
    \return         whether <i>str</i> contains any letters
*/
bool contains_letter(const std::string& str);

/*! \brief          Does a string contain any upper case letters?
    \param  str     string to test
    \return         whether <i>str</i> contains any upper case letters
*/
inline bool contains_upper_case_letter(const std::string& str)
  { return (str.find_first_of(UPPER_CASE_LETTERS) != std::string::npos); }

/*! \brief          Does a string contain any digits?
    \param  str     string to test
    \return         whether <i>str</i> contains any digits
*/
bool contains_digit(const std::string& str);

/*! \brief              Pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_side    side on which to pad
    \param  pad_char    character with which to pad
    \return             padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
std::string pad_string(const std::string& s, const size_t len, const enum PAD pad_side = PAD::LEFT, const char pad_char = SPACE_CHAR);

/*! \brief              Left pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_char    character with which to pad
    \return             left padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
template <typename T>
inline std::string pad_left(const T& s, const size_t len, const char pad_char = SPACE_CHAR)
  { return pad_string(to_string(s), len, PAD::LEFT, pad_char); }

/*! \brief              Left pad an integer type with zeroes to a particular size
    \param  s           integer value
    \return             left padded version of <i>s</i> rendered as a string and left padded with zeroes
*/
template <typename T>
  requires (std::is_integral_v<T>)
inline std::string pad_leftz(const T& s, const size_t len)
  { return pad_left(s, len, '0'); }

/*! \brief              Right pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_char    character with which to pad
    \return             right padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
inline std::string pad_right(const std::string& s, const size_t len, const char pad_char = SPACE_CHAR)
  { return pad_string(s, len, PAD::RIGHT, pad_char); }

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

/*! \brief      Remove characters from the end of a string
    \param  s   original string
    \param  n   number of chars to remove
    \return     <i>s</i> with the last <i>n</i> characters removed
  
    If <i>n</i> is equal to or greater than the length of <i>s</i>, then
    the empty string is returned.
*/
inline std::string remove_from_end(const std::string& s, const unsigned int n)
  { return ( (n >= s.length()) ? std::string() : s.substr(0, s.length() - n) ); }

/*! \brief              Split a string into components
    \param  cs          original string
    \param  separator   separator string (typically a single character)
    \return             vector containing the separate components
*/
std::vector<std::string> split_string(const std::string& cs, const std::string& separator);

/*! \brief              Split a string into components
    \param  cs          original string
    \param  separator   separator character
    \return             vector containing the separate components
*/
inline std::vector<std::string> split_string(const std::string& cs, const char separator)
  { return split_string(cs, std::string(1, separator)); }

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
inline std::vector<std::string> split_string(const std::string& cs, const int record_length)
  { return split_string(cs, static_cast<unsigned int>(record_length)); }

/*! \brief      Squash repeated occurrences of a character
    \param  cs  original string
    \param  c   character to squash
    \return     <i>cs</i>, but with all consecutive instances of <i>c</i> converted to a single instance
*/
std::string squash(const std::string& cs, const char c = SPACE_CHAR);

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
inline std::vector<std::string> to_lines(const std::string& cs, const std::string& eol_marker = EOL)
  { return split_string(cs, eol_marker); }

/*! \brief      Remove all instances of a specific leading character
    \param  cs  original string
    \param  c   leading character to remove (if present)
    \return     <i>cs</i> with any leading octets with the value <i>c</i> removed
*/
std::string remove_leading(const std::string& cs, const char c);

/*! \brief      Remove leading spaces
    \param  cs  original string
    \return     <i>cs</i> with any leading spaces removed
*/
inline std::string remove_leading_spaces(const std::string& cs)
  { return remove_leading(cs, SPACE_CHAR); }

/*! \brief      Remove all instances of a specific trailing character
    \param  cs  original string
    \param  c   trailing character to remove (if present)
    \return     <i>cs</i> with any trailing octets with the value <i>c</i> removed
*/
std::string remove_trailing(const std::string& cs, const char c);

/*! \brief      Remove trailing spaces
    \param  cs  original string
    \return     <i>cs</i> with any trailing spaces removed
*/
inline std::string remove_trailing_spaces(const std::string& cs)
  { return remove_trailing(cs, ' '); }

/*! \brief      Remove leading and trailing spaces
    \param  cs  original string
    \return     <i>cs</i> with any leading or trailing spaces removed
*/
inline std::string remove_peripheral_spaces(const std::string& cs)
  { return remove_trailing_spaces(remove_leading_spaces(cs)); }

/*! \brief      Remove leading and trailing spaces
    \param  t   container of strings
    \return     <i>t</i> with leading and trailing spaces removed from the individual elements
*/
template <typename T>
T remove_peripheral_spaces(const T& t)
{ typename std::remove_const<T>::type rv;

  for_each(t.cbegin(), t.cend(), [&rv](const std::string& s) { rv.push_back(remove_peripheral_spaces(s)); } );

  return rv;
}

/*! \brief      Remove peripheral instances of a specific character
    \param  cs  original string
    \param  c   character to remove
    \return     <i>cs</i> with any leading or trailing instances of <i>c</i> removed
*/
inline std::string remove_peripheral_character(const std::string& cs, const char c)
  { return remove_trailing(remove_leading(cs, c), c); }

/*! \brief                  Remove all instances of a particular char from a string
    \param  cs              original string
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
std::string remove_char(const std::string& cs, const char char_to_remove);

/*! \brief                  Remove all instances of a particular char from a string
    \param  s               original string
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
inline std::string remove_char(std::string& s, const char char_to_remove)
  { return remove_char(static_cast<const std::string>(s), char_to_remove); }
  
/*! \brief                  Remove all instances of a particular char from a container of strings
    \param  t               container of strings
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>t</i> with all instances of <i>char_to_remove</i> removed
*/
template <typename T>
T remove_char(T& t, const char char_to_remove)
{ typename std::remove_const<T>::type rv;

  for_each(t.cbegin(), t.cend(), [=, &rv](const std::string& cs) { rv.push_back(remove_char(cs, char_to_remove)); } );

  return rv;
}

/*! \brief                      Remove all instances of particular characters from a string
    \param  s                   original string
    \param  chars_to_remove     string whose characters are to be removed from <i>s</i>
    \return                     <i>s</i> with all instances of the characters in <i>chars_to_remove</i> removed
*/
std::string remove_chars(const std::string& s, const std::string& chars_to_remove);

/*! \brief                      Remove all instances of particular characters from a string
    \param  cs                  original string
    \param  chars_to_remove     vector whose elements are to be removed from <i>s</i>
    \return                     <i>s</i> with all instances of the characters in <i>chars_to_remove</i> removed
*/
//std::string remove_chars(const std::string& cs, const std::vector<char>& chars_to_remove);

/*! \brief                  Remove all instances of a particular char from all delimited substrings
    \param  cs              original string
    \param  char_to_remove  character to be removed from delimited substrings in <i>cs</i>
    \param  delim_1         opening delimiter
    \param  delim_2         closing delimiter
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed from inside substrings delimited by <i>delim_1</i> and <i>delim_2</i>
*/
std::string remove_char_from_delimited_substrings(const std::string& cs, const char char_to_remove, const char delim_1, const char delim_2);

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
std::string delimited_substring(const std::string& cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters);

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
std::string delimited_substring(const std::string& cs, const std::string& delim_1, const std::string& delim_2, const DELIMITERS return_delimiters);

/*! \brief                      Obtain all occurrences of a delimited substring
    \param  cs                  original string
    \param  delim_1             opening delimiter
    \param  delim_2             closing delimiter
    \param  return_delimiters   whether to keep delimiters in the returned value
    \return                     all substrings between <i>delim_1</i> and <i>delim_2</i>
*/
std::vector<std::string> delimited_substrings(const std::string& cs, const std::string& delim_1, const std::string& delim_2, const DELIMITERS return_delimiters);

/*! \brief                      Obtain all occurrences of a delimited substring
    \param  cs                  original string
    \param  delim_1             opening delimiter
    \param  delim_2             closing delimiter
    \param  return_delimiters   whether to keep delimiters in the returned value
    \return                     all substrings between <i>delim_1</i> and <i>delim_2</i>, possibly including the delimiters
*/
std::vector<std::string> delimited_substrings(const std::string& cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters);

/*! \brief          Join the elements of a container of strings, using a provided separator
    \param  ct      container of strings
    \param  sep     separator inserted between the elements of <i>vec</i>
    \return         all the elements of <i>ct</i>, concatenated, but with <i>sep</i> inserted between elements
*/
template <typename T, typename U>
std::string join(const T& ct, const U sep)
  requires (is_string_v<typename T::value_type>)
{ std::string rv;

  for (auto cit { ct.cbegin() }; cit != ct.cend(); ++cit)
  { if (cit != ct.cbegin())
      rv += sep;

    rv += (*cit);
  }

  return rv;
}

/*! \brief  Create a string of a certain length, with all characters the same
    \param  c   Character that the string will contain
    \param  n   Length of string to be created
    \return String containing <i>n</i> copies of <i>c</i>
*/
inline std::string create_string(const char c, const int n = 1)
  { return std::string(n, c); }

/*! \brief          Centre a string
    \param  str     string to be centred
    \param  width   final width of the centred string
    \return         <i>str</i> centred in a string of spaces, with total size <i>width</i>,
*/
std::string create_centred_string(const std::string& str, const unsigned int width);

/*! \brief      Get the last character in a string
    \param  cs  source string
    \return     last character in <i>cs</i>

    Throws exception if <i>cs</i> is empty
*/
char last_char(const std::string& cs);

/*! \brief      Get the penultimate character in a string
    \param  cs  source string
    \return     penultimate character in <i>cs</i>

    Throws exception if <i>cs</i> is empty or contains only one character
*/
char penultimate_char(const std::string& cs);

/*! \brief      Get the antepenultimate character in a string
    \param  cs  source string
    \return     antepenultimate character in <i>cs</i>

    Throws exception if <i>cs</i> contains fewer than two characters
*/
char antepenultimate_char(const std::string& cs);

/*! \brief          Get the terminating part of a string
    \param  cs      original string
    \param  n       number of characters to return
    \return         the last <i>n</i> characters of <i>cs</i>
*/
inline std::string last(const std::string& cs, unsigned int n)
  { return (cs.length() < n ? cs : cs.substr(cs.length() - n)); }

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
std::string transform_string(const std::string& cs, int(*pf)(int));

/*! \brief      Convert string to upper case
    \param  cs  original string
    \return     <i>cs</i> converted to upper case
*/
inline std::string to_upper(const std::string& cs)
  { return transform_string(cs, std::toupper); }

/*! \brief      Convert string to lower case
    \param  cs  original string
    \return     <i>cs</i> converted to lower case
*/
inline std::string to_lower(const std::string& cs)
  { return transform_string(cs, std::tolower); }

/*! \brief      Does a string begin with a particular substring?
    \param  cs  string to test
    \param  ss  substring to look for
    \return     whether <i>cs</i> begins with <i>ss</i>

    See https://stackoverflow.com/questions/1878001/how-do-i-check-if-a-c-stdstring-starts-with-a-certain-string-and-convert-a
*/
inline bool starts_with(const std::string& cs, const std::string& ss)
  { return (cs.rfind(ss, 0) == 0); }

/*! \brief      Does a string begin with one of a number of particular substrings?
    \param  cs  string to test
    \param  ss  substrings to look for
    \return     whether <i>cs</i> begins with any of the entries in <i>ss</i>
*/
template <typename T>
bool starts_with(const std::string& cs, const T& ss)
  requires (is_string_v<typename T::value_type>)
{ for (const auto& str : ss)
    if (starts_with(cs, str))
      return true;

  return false;
}

/*! \brief      Does a string begin with a particular substring?
    \param  cs  string to test
    \param  ss  substring to look for
    \return     whether <i>cs</i> begins with <i>ss</i>
*/
inline bool begins_with(const std::string& cs, const std::string& ss)
  { return (starts_with(cs, ss) ); }

/*! \brief      Does a string end with a particular substring?
    \param  cs  string to test
    \param  ss  substring to look for
    \return     whether <i>cs</i> ends with <i>ss</i>
*/
inline bool ends_with(const std::string& cs, const std::string& ss)
  { return ( (cs.length() < ss.length()) ? false : ( cs.rfind(ss) == (cs.length() - ss.length()) ) ); }

/*! \brief              Is a call a maritime mobile?
    \param  callsign    call to test
    \return             whether <i>callsign</i> appears to be a maritime mobile
*/
inline bool is_maritime_mobile(const std::string& callsign)
  { return ( ends_with(to_upper(callsign), "/MM"s ) ); }

/*! \brief          Convert an integer to a character-separated string
    \param  n       number to convert
    \param  sep     string to act as the triplet separator
    \return         <i>n</i> with the separator <i>sep</i> separating each triplet

    Uses comma as separator if <i>sep</i> is empty.
*/
template <typename T>
std::string separated_string(const T n, const std::string& sep = ","s)
{ const char separator { (sep.empty() ? ',' : sep[0]) };

  std::string tmp { to_string(n) };
  std::string rv;

  while (!tmp.empty())
  { for (unsigned int N = 0; N < 3 and !tmp.empty(); ++N)
    { rv = std::string(1, last_char(tmp)) + rv;
      tmp = tmp.substr(0, tmp.length() - 1);
    }

    if (!tmp.empty())
      rv = std::string(1, separator) + rv;
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
bool is_legal_value(const std::string& value, const std::string& legal_values, const std::string& separator);

/*! \brief          Is one call earlier than another, according to callsign sort order?
    \param  call1   first call
    \param  call2   second call
    \return         whether <i>call1</i> appears before <i>call2</i> in callsign sort order
*/
bool compare_calls(const std::string& call1, const std::string& call2);

/*! \brief          Is the value of one mult earlier than another?
    \param  mult1   first mult value
    \param  mult2   second mult value
    \return         whether <i>mult1</i> appears before <i>mult2</i> in displayed mult value sort order  (used for exchange mults)
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
inline std::string remove_substring(const std::string& cs, const std::string& ss)
  { return ( contains(cs, ss) ? replace(cs, ss, std::string()) : cs ); }

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
std::string remove_trailing_comment(const std::string& str, const std::string& comment_str = "//");

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
inline std::string truncate_before_first(const std::string& str, const char c)
  { return (substring(str, 0, str.find(c))); }

/*! \brief          Return position in a string at the end of a target string, if present
    \param  str     string to search
    \param  target  string to find
    \return         position in <>str</i> after the end of <i>target</i>

    Returns string::npos if <i>target</i> is not a substring of <i>str</i> OR if <i>target</i>
    is the conclusion of <i>str</i>
*/
size_t find_and_go_to_end_of(const std::experimental::string_view str, const std::experimental::string_view target);

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
T regex_matches(const C& container, const std::string& s)
{ T rv;
     
  const std::regex rgx { s };

  FOR_ALL(container, [=, &rv](const std::string& callsign) { if (regex_match(callsign, rgx))
                                                               rv += callsign;
                                                           } );
  return rv;
}

constexpr long unsigned int STR_HASH(const char* str, int off = 0) 
  { return !str[off] ? 5381 : (STR_HASH(str, off + 1) * 33) ^ str[off]; }                                                                                

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(string_function_error);     ///< string_function error

#endif    // STRING_FUNCTIONS_H
