// $Id: string_functions.h 86 2014-12-13 20:06:24Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#include "x_error.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

#include <time.h>

static const std::string EOL      = "\n";      ///< end-of-line marker as string
static const char        EOL_CHAR = '\n';      ///< end-of-line marker as character

static const std::string  LF       = "\n";     ///< LF as string
static const std::string& LF_STR  = LF;        ///< LF as string
static const char         LF_CHAR  = '\n';     ///< LF as character

static const std::string CR       = "\r";      ///< CR as string
static const char        CR_CHAR  = '\r';      ///< CR as character
static const std::string CRLF     = "\r\n";    ///< CR followed by LF
  
/// directions in which a string can be padded
enum pad_direction { PAD_LEFT,                 ///< pad to the left
                     PAD_RIGHT                 ///< pad to the right
                   };

// error numbers
const int STRING_UNDERFLOW            = -1,    ///< Underflow
          STRING_UNEXPECTED_CHARACTER = -2,    ///< Unexpected character found in string
          STRING_INVALID_FILE         = -3,    ///< Invalid file
          STRING_INVALID_LENGTH       = -4,    ///< Length is invalid
          STRING_FILE_IS_DIRECTORY    = -5,    ///< File is a directory
          STRING_UNABLE_TO_STAT_FILE  = -6,    ///< Unable to stat a file
          STRING_BOUNDS_ERROR         = -7,    ///< Attempt to access range outside string
          STRING_CONVERSION_FAILURE   = -8,    ///< Attempt to convert the format of a string failed
          STRING_UNKNOWN_ENCODING     = -9;    ///< Unknown character encoding

/*! \brief convert from a CSV line to a vector of strings, each containing one field
    \param  line    CSV line
    \return         vector of fields from the CSV line

    This is actually quite difficult to do properly
*/
const std::vector<std::string> from_csv(const std::string& line);

/* \brief       duplicate a particular character within a string
    \param  s   string in which characters are to be duplicated
    \param  c   character to be duplicated
    \return     <i>s</i>, modified so that every instance of <i>c</i> is doubled
*/
const std::string duplicate_char(const std::string& s, const char& c = '"');

/*! \brief  provide a formatted date/time string
    \return current date and time in the format: YYYY-MM-DDTHH:MM
*/
const std::string date_time_string(void);

/*! \brief  convert struct tm pointer to formatted string
    \param  format  format to be used
    \param  tmp     date/time to be formatted
    \return         formatted string

    Uses strftime() to perform the formatting
*/
const std::string format_time(const std::string& format, const tm* tmp);

/*! \brief  generic conversion from string
    \param  s string
    \return string converted to type <i>T</i>
*/
template <class T>
const T from_string(const std::string& s)
{ std::istringstream stream (s);
  T t;
     
  stream >> t;
  return t;
}

/*! \brief  generic conversion to string
  \param  val value to convert
  \return string  <i>val</i>converted to a string
*/
template <class T>
const std::string to_string(const T val)
{ std::ostringstream stream;
                
  stream << val;
  return stream.str();
}

/*! \brief              safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \param  length      length of substring to be extracted
    \return             substring of length <i>length</i>, starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn, length)</i>, except does not throw a range exception
*/
const std::string substring(const std::string& str, const size_t start_posn, const size_t length);

/*! \brief              safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \return             substring starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn)</i>, except does not throw a range exception
*/
const std::string substring(const std::string& str, const size_t start_posn);

/*! \brief              replace every instance of one character with another
    \param  s           string on which to operate
    \param  old_char    character to be replaced
    \param  new_char    replacement character
    \return             <i>s</i>, with every instance of <i>old_char</i> replaced by <i>new_char</i>
*/
const std::string replace_char(const std::string& s, char old_char, char new_char);

/*! \brief              replace every instance of one string with another
    \param  s           string on which to operate
    \param  old_str     string to be replaced
    \param  new_str     replacement string
    \return             <i>s</i>, with every instance of <i>old_str</i> replaced by <i>new_str</i>
*/
const std::string replace(const std::string& s, const std::string& old_str, const std::string& new_str);

/*! \brief              does a string contain a particular substring?
    \param  s           string to test
    \param  ss          substring for which to search
    \return             whether <i>s</i> contains the substring <i>ss</i>
*/
inline const bool contains(const std::string& s, const std::string& ss)
  { return s.find(ss) != std::string::npos; }

/*! \brief does a string contain a particular character?
*/
inline const bool contains(const std::string& s, const char c)
  { return s.find(c) != std::string::npos; }

/*! \brief does a string contain any letters?
*/
const bool contains_letter(const std::string& str);

/*! \brief does a string contain any digits?
*/
const bool contains_digit(const std::string& str);

/*! \brief  pad a string to a particular size
  \param  s original string
  \param  len length of returned string
  \param  pad_sude  side on which to pad
  \param  pad_char  character with which to pad
  \return padded version of <i>s</i>
  
  If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
const std::string pad_string(const std::string& s, const unsigned int len, const enum pad_direction pad_side = PAD_LEFT, const char pad_char = ' ');

/*! \brief  Read the contents of a file into a single string
    \param  filename  Name of file to be read
    \return Contents of file <i>filename</i>
  
    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
const std::string read_file(const std::string& filename);

/*! \brief  Read the contents of a file into a single string
    \param  path the different directories to try, in order
    \param  filename  Name of file to be read
    \return Contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
const std::string read_file(const std::vector<std::string>& path, const std::string& filename);

/*! \brief  Read the contents of a file into a single string
    \param  filename  Name of file to be read
    \param  path the different directories to try, in order
    \return Contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
inline const std::string read_file(const std::string& filename, const std::vector<std::string>& path)
  { return read_file(path, filename); }

/*! \brief  Write a string to a file
    \param  cs  String to be written to file
    \param  filename  Name of file to be written

    Throws exception if the file cannot be written
*/
void write_file(const std::string& cs, const std::string& filename);


/*! \brief remove characters from the end of a string
    \param  s original string
    \param  n number of chars to remove
    \return <i>s</i> with the last <i>n</i> characters removed
  
    If <i>n</i> is equal to or greater than the length of <i>s</i>, then
    the empty string is returned.
*/
const std::string remove_from_end(const std::string& s, const unsigned int n);

/*! \brief  split a string into components
    \param  cs    Original string
    \param  separator Separator string (typically a single character)

    \return Vector containing the separate components
*/
const std::vector<std::string> split_string(const std::string& cs, const std::string& separator);

/*! \brief  split a string into components
    \param  cs              Original string
    \param  separator       Separator character

    \return Vector containing the separate components
*/
inline const std::vector<std::string> split_string(const std::string& cs, const char c)
  { return split_string(cs, std::string(1, c)); }

/*! \brief  split a string into equal-length records
    \param  cs              Original string
    \param  record_length   Length of each record

    \return Vector containing the separate components

    Any non-full record at the end is silently discarded
*/
const std::vector<std::string> split_string(const std::string& cs, const unsigned int record_length);

/*! \brief  split a string into equal-length records
    \param  cs              Original string
    \param  record_length   Length of each record

    \return Vector containing the separate components

    Any non-full record at the end is silently discarded
*/
inline const std::vector<std::string> split_string(const std::string& cs, const int record_length)
  { return split_string(cs, static_cast<unsigned int>(record_length)); }

/// squash repeated occurrences of a character
const std::string squash(const std::string& cs, const char c = ' ');

/*! \brief  split a string into lines
    \param  cs          Original string
    \param  eol_marker  EOL marker

    \return Vector containing the separate lines
*/
inline const std::vector<std::string> to_lines(const std::string& cs, const std::string& eol_marker = EOL)
  { return split_string(cs, eol_marker); }

/*! \brief  Remove all instances of a specific leading character
    \param  cs  Original string
    \param  c Leading character to remove (if present)

    \return <i>cs</i> with any leading octets with the value <i>c</i> removed
*/
const std::string remove_leading(const std::string& cs, const char c);

/*! \brief  Remove leading spaces
  \param  cs  Original string
  \return <i>cs</i> with any leading spaces removed
*/
inline const std::string remove_leading_spaces(const std::string& cs)
  { return remove_leading(cs, ' '); }

/*! \brief  Remove all instances of a specific trailing character
  \param  cs  Original string
  \param  c Trailing character to remove (if present)
  \return <i>cs</i> with any trailing octets with the value <i>c</i> removed
*/
const std::string remove_trailing(const std::string& cs, const char c);

/*! \brief  Remove trailing spaces
  \param  cs  Original string
  \return <i>cs</i> with any trailing spaces removed
*/
inline const std::string remove_trailing_spaces(const std::string& cs)
  { return remove_trailing(cs, ' '); }

/*!     \brief          Remove leading and trailing spaces
        \param  cs      Original string
        \return         <i>cs</i> with any leading or trailing spaces removed
*/
inline const std::string remove_peripheral_spaces(const std::string& cs)
  { return remove_trailing_spaces(remove_leading_spaces(cs)); }

/*!     \brief          Remove leading and trailing spaces
        \param  s       Original string
        \return         <i>s</i> with any leading or trailing spaces removed
*/
inline const std::string remove_peripheral_spaces(std::string& s)
  { return remove_trailing_spaces(remove_leading_spaces(s)); }

/*!     \brief          Remove leading and trailing spaces
        \param  t       Container of strings
        \return         <i>t</i> with leading and trailing spaces removed from the individual elements
*/
template <typename T>
T remove_peripheral_spaces(T& t)
{ typename std::remove_const<T>::type rv;

  for_each(t.begin(), t.end(), [&rv](const std::string& s) { rv.push_back(remove_peripheral_spaces(s)); } );

  return rv;
}

/*!     \brief          Remove peripheral instances of a specific character
        \param  cs      Original string
        \param  c       Character to remove
        \return         <i>cs</i> with any leading or trailing instances of <i>c</i> removed
*/
inline const std::string remove_peripheral_character(const std::string& cs, const char c)
  { return remove_trailing(remove_leading(cs, c), c); }

/*! \brief                  Remove all instances of a particular char from a string
  \param  cs                Original string
  \param  char_to_remove    Character to be removed from <i>cs</i>
  \return                   <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
const std::string remove_char(const std::string& cs, const char char_to_remove);

/*! \brief          Obtain a delimited substring
  \param  cs        Original string
  \param  delim_1   opening delimiter
  \param  delim_2   closing delimiter
  \return           substring between <i>delim_1</i> and <i>delim_2</i>
  
  Returns the empty string if the delimiters do not exist, or if
  <i>delim_2</i> does not appear after <i>delim_1</i>
*/
const std::string delimited_substring(const std::string& cs, const char delim_1, const char delim_2);

/*! \brief      Join the elements of a string vector, using a provided separator
    \param  vec container of strings
    \param  sep separator inserted between the elements of <i>vec</i>
    \return     all the elements of <i>vec</i>, concatenated, but with <i>sep</i> inserted between elements
*/
const std::string join(const std::vector<std::string>& vec, const std::string& sep /* = " " */);

// join all the elements of a string deque together, with a known separator
const std::string join(const std::deque<std::string>& vec, const std::string& sep /* = " " */);

/*! \brief  Create a string of a certain length, with all characters the same
    \param  c   Character that the string will contain
    \param  n   Length of string to be created
    \return String containing <i>n</i> copies of <i>c</i>
*/
inline const std::string create_string(const char c, const int n = 1)
  { return std::string(n, c); }

// simple functions for chars near the end of strings
/// the last character in a string
const char last_char(const std::string& cs);

/// the penultimate character in a string
const char penultimate_char(const std::string& cs);

/// the antepenultimate character in a string
const char antepenultimate_char(const std::string& cs);

inline const std::string last(const std::string& cs, unsigned int n)
  { return (cs.length() < n ? cs : cs.substr(cs.length() - n)); }

/*!   \brief  get an environment variable
  \return the environment variable

  returns the empty string if the variable does not exist
*/
const std::string get_environment_variable(const std::string& var_name);

// begins with a string
inline const bool begins_with(const std::string& str, const std::string& test)
  { return (str.substr(0, test.length()) == test); }

/*! \brief  Transform a string
  \param  cs  original string
  \return <i>cs</i> with the transformation applied
*/
const std::string transform_string(const std::string& cs, int(*pf)(int));

/*! \brief  Convert to upper case
  \param  cs  original string
  \return <i>cs</i> converted to upper case
*/
inline std::string to_upper(const std::string& cs)
  { return transform_string(cs, std::toupper); }

/*! \brief  Convert to upper case
    \param  cs  original string
    \return <i>cs</i> converted to upper case
*/
inline std::string to_lower(const std::string& cs)
  { return transform_string(cs, std::tolower); }

/// does a string start with a particular substring?
inline bool starts_with(const std::string& cs, const std::string& ss)
  { return cs.find(ss) == 0; }

/// convert an integer to a character-separated string
const std::string separated_string(const int n, const std::string& sep = ",");

/// convert an integer to a comma-separated string
inline const std::string comma_separated_string(const int n)
  { return separated_string(n); }

/// get location of start of next word
const size_t next_word_posn(const std::string& str, const size_t current_posn);

/// return the starting position for each word
const std::vector<size_t> starts_of_words(const std::string& s);

// get nth word
const std::string nth_word(const std::string& s, const unsigned int n, const unsigned int wrt = 0);

// assumes UTF-8; TODO: generalise using locales/facets
const size_t n_chars(const std::string& str);
  
/*!     \brief  Does a string contain a legal dotted-decimal IPv4 address
        \param  cs  Original string
        \return  Whether <i>cs</i> contains a legal dotted decimal IPv4 address
*/
const bool is_legal_ipv4_address(const std::string& cs);  

/*!     \brief  Convert a long to dotted decimal string
        \param  val     Original value
        \return Dotted decimal string
        
        Assumes that a long is four octets
*/
std::string convert_to_dotted_decimal(const uint32_t val);

/// is a string a legal value from a list?
const bool is_legal_value(const std::string& value, const std::string& legal_values, const std::string& separator);

// return true if call1 < call2
const bool compare_calls(const std::string& s1, const std::string& s2);

// -------------------------------------- Errors  -----------------------------------

/*! \class  string_function_error
  \brief  Errors related to string processing
*/

class string_function_error : public x_error
{ 
protected:

public:

/*! \brief  Construct from error code and reason
  \param  n Error code
  \param  s Reason
*/
  inline string_function_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};


#endif    // STRING_FUNCTIONS_H
