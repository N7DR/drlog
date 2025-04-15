// $Id: string_functions.cpp 266 2025-04-07 22:34:06Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   string_functions.cpp

    Functions related to the manipulation of strings
*/

#include "diskfile.h"
#include "log_message.h"
#include "macros.h"
#include "string_functions.h"

#include <cctype>
#include <cstdio>
#include <iomanip>

#include <langinfo.h>

#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/stat.h>

using namespace std;

extern message_stream    ost;           ///< for debugging and logging

/*! \brief          Convert from a CSV line to a vector of strings, each containing one field
    \param  line    CSV line
    \return         vector of fields from the CSV line

    This is actually quite difficult to do properly
*/
vector<string> from_csv(const string_view line)
{ constexpr char quote { '"' };
  constexpr char comma { ',' };

  vector<string> rv;

  size_t posn { 0 };

  string this_value;

  bool inside_value { false };

  while (posn < line.size())
  { if (const char this_char { line[posn] }; this_char == quote)
    { if (inside_value)               // we're inside a field
      { if (posn < line.size() - 1)   // make sure there's at least one unread character
        { const char next_char { line[posn + 1] };

          if (next_char == quote)     // it's a doubled quote
          { posn += 2;                // skip the next character
            this_value += quote;
          }
          else                        // it's the end of the value
          { rv += this_value;
            inside_value = false;
            posn++;
          }
        }
        else                          // there are no more unread characters; declare this as the end
        { rv += this_value;
          inside_value = false;
          posn++;
        }
      }
      else                            // we have a quote and we aren't inside a value; start a new value
      { this_value.clear();
        inside_value = true;
        posn++;
      }
    }
    else                              // not a quote; anything inside a value gets added, anything outside gets ignored
    { if (inside_value)
      { this_value += this_char;
        posn++;
      }
      else
      { if (this_char == comma)
        { if (posn < line.size() - 1)   // make sure there's at least one unread character
          { if (const char next_char { line[posn + 1] }; next_char == comma)
            { rv += EMPTY_STR;   // empty value
              posn++;
            }
            else
              posn++;
          }
          else                        // we've finished with a comma; this is really an error, we just assume an empty last field
          { rv += EMPTY_STR;   // empty value
            posn++;
          }
        }
      }
    }
  }

  return rv;
}

/*! \brief      Append a string_view to a string
    \param  s   original string
    \param  sv  string_view to append
    \return     concatenation of <i>s</i> and <i>sv</i>
*/
string operator+(const string& s, const string_view sv)
{ string rv { };

  rv.reserve(s.size() + sv.size());

  rv = s + string { sv };

  return rv;
}

/*! \brief      Append a character to a string_view
    \param  sv  original string
    \param  c   character to append
    \return     concatenation of <i>sv</i> and <i>c</i>
*/
std::string operator+(const std::string_view sv, const char c)
{ string rv;

  rv.reserve(sv.size() + 1);

  rv = string { sv } + c;

  return rv;
}

/*! \brief      Duplicate a particular character within a string
    \param  s   string in which characters are to be duplicated
    \param  c   character to be duplicated
    \return     <i>s</i>, modified so that every instance of <i>c</i> is doubled
*/
string duplicate_char(const string_view s, const char c)
{ size_t start_posn { 0 };
  size_t next_posn  { 0 };
  string rv         { };

  do
  { next_posn = s.find(c, start_posn);

    if (next_posn != string::npos)
    { rv += (s.substr(start_posn, next_posn - start_posn + 1) + c);
      start_posn = next_posn + 1;
    }
  } while (next_posn != string::npos);

  rv += s.substr(start_posn);

  return rv;
}

/*! \brief                      Provide a formatted UTC date/time string
    \param  include_seconds     whether to include the portion of the string that designates seconds
    \return                     current date and time in the format: YYYY-MM-DDTHH:MM or YYYY-MM-DDTHH:MM:SS
*/
string date_time_string(const SECONDS include_seconds)
{ constexpr size_t TIME_BUF_LEN { 26 };

  const time_t now { NOW() };            // get the time from the kernel

  struct tm structured_time;

  gmtime_r(&now, &structured_time);         // convert to UTC

  array<char, TIME_BUF_LEN> buf;                           // buffer to hold the ASCII date/time info; see man page for gmtime()

  asctime_r(&structured_time, buf.data());                     // convert to ASCII

  const string ascii_time { buf.data(), TIME_BUF_LEN };
  const string _utc       { ascii_time.substr(11, ( (include_seconds == SECONDS::INCLUDE) ? 8 : 5)) };                            // hh:mm[:ss]
  const string _date      { to_string(structured_time.tm_year + 1900) + "-"s + pad_leftz((structured_time.tm_mon + 1), 2) + "-"s + pad_leftz(structured_time.tm_mday, 2) };   // yyyy-mm-dd

  return (_date + "T"s + _utc);
}

/*! \brief          Convert struct tm pointer to formatted string
    \param  format  format to be used
    \param  tmp     date/time to be formatted
    \return         formatted string

    Uses strftime() to perform the formatting
*/
string format_time(const string& format, const tm* tmp)
{ constexpr size_t BUFLEN { 60 };

  char buf[BUFLEN];

  if (const size_t nchars { strftime(buf, BUFLEN, format.c_str(), tmp) }; !nchars)
    throw string_function_error(STRING_CONVERSION_FAILURE, "Unable to format time");

  return string(buf);
}

/*! \brief          Return the first digit in a string
    \param  sv      string to test
    \param  c       character to return if no digit is present in <i>sv</i>
    \return         the first digit in <i>sv</i>, or <i>c</i>
*/
char first_digit(const std::string_view sv, const char c)
{ const auto posn { sv.find_first_of(DIGITS) };

  return ( (posn == string::npos) ? c : sv[posn] );
}

/*! \brief              Replace every instance of one character with another
    \param  s           string on which to operate
    \param  old_char    character to be replaced
    \param  new_char    replacement character
    \return             <i>s</i>, with every instance of <i>old_char</i> replaced by <i>new_char</i>
*/
string replace_char(const string_view s, const char old_char, const char new_char)
{ string rv { s };

  replace(rv.begin(), rv.end(), old_char, new_char);

  return rv;
}

/*! \brief              Replace every instance of one string with another
    \param  s           string on which to operate
    \param  old_str     string to be replaced
    \param  new_str     replacement string
    \return             <i>s</i>, with every instance of <i>old_str</i> replaced by <i>new_str</i>
*/
string replace(const string_view s, const string_view old_str, const string_view new_str)
{ string rv        { };
  size_t posn      { 0 };
  size_t last_posn { 0 };

  const string new_as_str { new_str };

  while ( (posn = s.find(old_str, last_posn)) != string_view::npos )
  { rv += ( string { s.substr(last_posn, posn - last_posn) } + new_as_str );
    last_posn = posn + old_str.length();
  }

  rv += s.substr(last_posn);

  return rv;
}

/*! \brief              Pad a string to a particular size
    \param  s           original string
    \param  len         length of returned string
    \param  pad_side    side on which to pad
    \param  pad_char    character with which to pad
    \return             padded version of <i>s</i>
  
    If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
string pad_string(const string_view s, const size_t len, const PAD pad_side, const char pad_char)
{ const string s_str { s }; 

  if ( (static_cast<int>(len) <= 0) or (s.length() >= len) )
    return s_str;

  const size_t n_pad_chars { len - s.length() };
  
  const string pstring(n_pad_chars, pad_char);  // cannot use initializer-list

  return ( (pad_side == PAD::LEFT) ? (pstring + s_str) : (s_str + pstring) );
}

/*! \brief              Read the contents of a file into a single string
    \param  filename    name of file to be read
    \return             contents of file <i>filename</i>
  
    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
string read_file(const string_view filename)
{ const string filename_str { filename };   // because we need c_str()

  FILE* fp { fopen(filename_str.c_str(), "rb") };

  if (!fp)
    throw string_function_error(STRING_INVALID_FILE, "Cannot open file: "s + filename_str);
  else
    fclose(fp);

// check that the file is not a directory  
  struct stat stat_buffer;

  if (const int status { ::stat(filename_str.c_str(), &stat_buffer) }; status)
    throw string_function_error(STRING_UNABLE_TO_STAT_FILE, "Unable to stat file: "s + filename_str);

  const bool is_directory { ( (stat_buffer.st_mode bitand S_IFDIR) != 0 ) };

  if (is_directory)
    throw string_function_error(STRING_FILE_IS_DIRECTORY, filename_str + " is a directory"s);

// now perform the actual read
  ifstream file { filename_str };
  string   str  {std::istreambuf_iterator<char>(file), { } };

  return str;
}

/*! \brief              Read the contents of a file into a single string
    \param  path        the different directories to try, in order
    \param  filename    name of file to be read
    \return             contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
string read_file(const vector<string>& path, const string_view filename)
{ const string valid_filename { find_file(path, filename) };

  if (valid_filename.empty())
    throw string_function_error(STRING_INVALID_FILE, "Cannot open file: "s + filename + " with non-trivial path"s);

  return read_file(valid_filename);
}

/*! \brief      Squash repeated occurrences of a character
    \param  cs  original string
    \param  c   character to squash
    \return     <i>cs</i>, but with all consecutive instances of <i>c</i> converted to a single instance
*/
string squash(const string_view cs, const char c)
{ auto both_match = [c] (const char lhs, const char rhs) { return ( (lhs == rhs) and (lhs == c) ); }; ///< do both match the target character?

  string rv { cs };

  rv.erase(std::unique(rv.begin(), rv.end(), both_match), rv.end());

  return rv;
}

/*! \brief          Remove empty lines from a vector of lines
    \param  lines   the original vector of lines
    \return         <i>lines</i>, but with all empty lines removed

    If the line contains anything, even just whitespace, it is not removed
*/
vector<string> remove_empty_lines(const vector<string>& lines)
{ vector<string> rv { lines };

//  FOR_ALL(lines, [&rv] (const string& line) { if (!line.empty()) rv += line; } );
//string rv { s };

  erase_if(rv, [] (const string& line) { return line.empty(); } );

  return rv;
}

/*! \brief                  Remove all instances of a particular char from a string
    \param  cs              original string
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
string remove_char(const string_view cs, const char char_to_remove)
{ string rv { cs };

  erase(rv, char_to_remove);

  return rv;
} 

/*! \brief                  Remove all instances of a particular char from all delimited substrings
    \param  cs              original string
    \param  char_to_remove  character to be removed from delimited substrings in <i>cs</i>
    \param  delim_1         opening delimiter
    \param  delim_2         closing delimiter
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed from inside substrings delimited by <i>delim_1</i> and <i>delim_2</i>

    delimiters are kept in the output
*/
string remove_char_from_delimited_substrings(const string_view cs, const char char_to_remove, const char delim_1, const char delim_2)
{ string rv         { };
  size_t start_posn { 0 };

  while (true)
  { size_t delim_1_posn { cs.find(delim_1, start_posn) };

    if (delim_1_posn == string_view::npos)
      return (rv + string { cs.substr(start_posn) });

// we found the first delimiter
    size_t delim_2_posn { cs.find(delim_2, delim_1_posn + 1) };

// if there is no matching second delimiter, pretend second delimiter is at end
    if (delim_2_posn == string_view::npos)
      return (rv + remove_char(substring <std::string> (cs, delim_1_posn), char_to_remove));

// we have matching delimiters
    const size_t substr_length   { delim_2_posn - delim_1_posn + 1 };
    const string modified_substr { remove_char(substring <std::string> (cs, delim_1_posn, substr_length), char_to_remove) };  // keeps the delimiters in the string

    rv += ( string { cs.substr(start_posn, delim_1_posn) } + modified_substr );

    start_posn = delim_2_posn + 1;

    if (start_posn >= cs.length())
      return rv;
  }

// should never get here
  return rv;
}

/*! \brief                      Remove all instances of particular characters from a string
    \param  s                   original string
    \param  chars_to_remove     string whose characters are to be removed from <i>s</i>
    \return                     <i>s</i> with all instances of the characters in <i>chars_to_remove</i> removed
*/
string remove_chars(const string_view s, const string_view chars_to_remove)
{ string rv { s };

  erase_if(rv, [&chars_to_remove] (const char& c) { return contains(chars_to_remove, c); } );

  return rv;
}

/*! \brief          Centre a string
    \param  str     string to be centred
    \param  width   final width of the centred string
    \return         <i>str</i> centred in a string of spaces, with total size <i>width</i>,
*/
string centred_string(const string_view str, const unsigned int width)
{ const size_t len { str.length() };

  if (len > width)
    return substring <std::string> (str, 0, width);

  if (len == width)
    return string { str };

  const string l { create_string(' ', (width - len) / 2) };
  const string r { create_string(' ', width - len - l.length()) };

  return (l + string { str } + r);
}

/*! \brief      Get the last character in a string
    \param  cs  source string
    \return     last character in <i>cs</i>

    Throws exception if <i>cs</i> is empty
*/
char last_char(const string_view cs)
{ if (cs.empty())
    throw string_function_error(STRING_BOUNDS_ERROR, "Attempt to access character in empty string"s);
    
  return cs[cs.length() - 1];
}

/*! \brief      Get the penultimate character in a string
    \param  cs  source string
    \return     penultimate character in <i>cs</i>

    Throws exception if <i>cs</i> is empty or contains only one character
*/
char penultimate_char(const string_view cs)
{ if (cs.length() < 2)
    throw string_function_error(STRING_BOUNDS_ERROR, "Attempt to access character beyond end of string"s);
    
  return cs[cs.length() - 2];
}

/*! \brief      Get the antepenultimate character in a string
    \param  cs  source string
    \return     antepenultimate character in <i>cs</i>

    Throws exception if <i>cs</i> contains fewer than two characters
*/
char antepenultimate_char(const string_view cs)
{ if (cs.length() < 3)
    throw string_function_error(STRING_BOUNDS_ERROR, "Attempt to access character beyond end of string"s);
    
  return cs[cs.length() - 3];
}

/*! \brief              Get an environment variable
    \param  var_name    name of the environment variable
    \return             the value of the environment variable <i>var_name</i>

    Returns the empty string if the variable does not exist
*/
string get_environment_variable(const string& var_name)
{ const char* cp { getenv(var_name.c_str()) };

  return ( cp ? string { cp } : string { } );
}

/*! \brief      Transform a string
    \param  cs  original string
    \param  pf  pointer to transformation function
    \return     <i>cs</i> with the transformation <i>*pf</i> applied
*/
string transform_string(const string_view cs, int (*pf) (int))
{ string rv { cs };
  
  std::ranges::transform(rv, rv.begin(), pf);
  
  return rv;
}

/*! \brief      Get location of start all words
    \param  s   string to be analysed
    \return     positions of all the starts of words in <i>s</i>
*/
vector<size_t> starts_of_words(const string_view s)
{ vector<size_t> rv { };

  if (s.empty())
    return rv;

// start of first word
  size_t posn { s.find_first_not_of(' ', 0) };

  if (posn == string::npos)
    return rv;

  rv += posn;

// next space
  while (true)
  { posn = s.find_first_of(' ', posn);

    if (posn == string::npos)
      return rv;

    posn = s.find_first_not_of(' ', posn);

    if (posn == string::npos)
      return rv;

    rv += posn;
  }
}

/*! \brief                  Get location of start of next word
    \param  str             string which the next word is to be found
    \param  current_posn    position from which to search
    \return                 position of start of next word, beginning at position <i>current_posn</i> in <i>str</i>

    Returns <i>string::npos</i> if no word can be found
*/
size_t next_word_posn(const string_view str, const size_t current_posn)
{ if (str.length() <= current_posn)
    return string::npos;

  const bool is_space { (str[current_posn] == ' ') };

  if (is_space)
    return ( str.find_first_not_of(' ', current_posn) );

// we are inside a word
  const size_t space_posn { str.find_first_of(' ', current_posn) };
  const size_t word_posn  { str.find_first_not_of(' ', space_posn) };

  return word_posn;
}

/*! \brief          Get nth word in a string
    \param  s       string to be analysed
    \param  n       word number to be returned
    \param  wrt     value with respect to which <i>n</i> is counted
    \return         the <i>n</i>th word, counting with respect to <i>wrt</i>

    Returns <i>string::npos</i> if there is no <i>n</i>th word
    Could modify this to be a template and return an STYPE, if that is useful
*/
string nth_word(const string_view s, const unsigned int n, const unsigned int wrt)
{ string rv { };

  if (n < wrt)
    return rv;

  const unsigned int   actual_word_number { n - wrt };
  const vector<size_t> starts             { starts_of_words(s) };

  if (actual_word_number >= starts.size())
    return rv;

  const size_t posn_1 { starts[actual_word_number] };
  const size_t posn_2 { ( (actual_word_number + 1) >= starts.size() ? string::npos : starts[actual_word_number + 1] ) };

  rv = remove_peripheral_spaces <std::string> (substring <std::string> (s, posn_1, posn_2 - posn_1));

  return rv;
}

/*! \brief          Get the actual length, in bytes, of a UTF-8-encoded string
    \param  str     UTF-8 string to be analysed
    \return         number of bytes occupied by <i>str</i>

    See: https://stackoverflow.com/questions/4063146/getting-the-actual-length-of-a-utf-8-encoded-stdstring
    TODO: generalise using locales/facets, instead of assuming UTF-8
*/
size_t n_chars(const string& str)
{ if (const string encoding { nl_langinfo(CODESET) }; encoding != "UTF-8"sv)
    throw string_function_error(STRING_UNKNOWN_ENCODING, "Unknown character encoding: "s + encoding);

  const size_t n_bytes { str.size() };

  char*  cp     { const_cast<char*>(str.data()) };
  char*  end_cp { cp + n_bytes };                   // one past the end of the contents of str
  size_t rv     { 0 };

  while (cp < end_cp)
  { if ( (*cp++ & 0xc0) != 0x80 )
      rv++;
  }

  return rv;
}

/*! \brief      Does a string contain a legal dotted-decimal IPv4 address
    \param  cs  string to test
    \return     whether <i>cs</i> contains a legal dotted decimal IPv4 address
*/
bool is_legal_ipv4_address(const string_view cs)
{ const vector<string_view> fields { split_string <std::string_view> (cs, '.') };

  if (fields.size() != 4)
    return false;

  for (const auto field : fields)
  { try
    { if (const int value { from_string<int>(field) }; (value < 0) or (value > 255))
        return false;
    }

    catch (...)
    { return false;
    }
  }

  return true;
}

/*! \brief          Convert a four-byte value to a dotted decimal string
    \param  val     original value
    \return         dotted decimal string corresponding to <i>val</i>
*/
string convert_to_dotted_decimal(const uint32_t val)
{ constexpr int N_BYTES { sizeof(uint32_t) };

// put into network order (so that we can guarantee the order of the octets in the long)
  const uint32_t network_val { htonl(val) };

  unsigned char* cp { (unsigned char*)(&network_val) };

  string rv;

  for (int n { 0 }; n < (N_BYTES - 1); n++)
  { const unsigned char c { cp[n] };
  
    rv += to_string(static_cast<int>(c)) + '.';
  }

  const unsigned char c { cp[N_BYTES - 1] };

  rv += to_string(static_cast<int>(c));

  return rv;
}

/*! \brief          Is one call earlier than another, according to callsign sort order?
    \param  call1   first call
    \param  call2   second call
    \return         whether <i>call1</i> appears before <i>call2</i> in callsign sort order
*/
bool compare_calls(const string_view call1, const string_view call2)
{
/* callsign sort order

   changes to ordinary sort order:
    '0' is the highest digit
    numbers sort after letters
    '/' comes after all digits and letters
    '-' comes after all digits and letters; here because names of log files, at least as used by CQ, use "-" instead of "/"
 */
  const auto compchar = [] (const char c1, const char c2)
    { if (c1 == c2)
        return false;

      if ( (c1 == '/') or (c1 == '-') )
        return false;

      if ( (c2 == '/') or (c2 == '-') )
        return true;

      if (isalpha(c1) and isdigit(c2))
        return true;

      if (isdigit(c1) and isalpha(c2))
        return false;

      if (isdigit(c1) and isdigit(c2))
      { if (c1 == '0')
          return false;

        if (c2 == '0')
          return true;
      }

      return (c1 < c2);
    };

  const size_t l1           { call1.size() };
  const size_t l2           { call2.size() };
  const size_t n_to_compare { min(l1, l2) };

  size_t index { 0 };

  while (index < n_to_compare)
  { if (call1[index] != call2[index])
      return compchar(call1[index], call2[index]);

    index++;
  }

  return (l1 < l2);
}

/*! \brief          Is the value of one mult earlier than another?
    \param  mult1   first mult value
    \param  mult2   second mult value
    \return         whether <i>mult1</i> appears before <i>mult2</i> in displayed mult value sort order (used for exchange mults)
*/
bool compare_mults(const string_view mult1, const string_view mult2)
{ if ( (mult1.size() == 2) and isdigit(mult1[0]) and isdigit(mult1[1]) and      // if two 2-digit numeric values (such as zones)
       (mult2.size() == 2) and isdigit(mult2[0]) and isdigit(mult2[1]) )
    return (mult1 < mult2);                                                     // simple string comparison
  else
    return compare_calls(mult1, mult2);
}

/*! \brief          Return a number with a particular number of decimal places
    \param  str     initial value
    \param  n       number of decimal places
    \return         <i>str</i> with <i>n</i> decimal places

    Assumes that <i>str</i> is a number
*/
//string decimal_places(const string& str, const int n)
string decimal_places(const string_view str, const int n)
{
// for now, assume that it's a number
//  if ( (str.length() >= 2) and (str[str.length() - 2] != '.') )
  if ( (str.length() >= 2) and (penultimate_char(str) != '.') )
  { const float fl { from_string<float>(str) };

    ostringstream stream;

    stream << fixed << setprecision(n) << fl;
    return stream.str();
  }

  return string { str };
}

/*! \brief          Deal with wprintw's idiotic insertion of newlines when reaching the right hand of a window
    \param  str     string to be reformatted
    \param  width   width of line in destination window
    \return         <i>str</i> reformatted for the window

    See http://stackoverflow.com/questions/7540029/wprintw-in-ncurses-when-writing-a-newline-terminated-line-of-exactly-the-same
*/
//string reformat_for_wprintw(const string& str, const int width)
string reformat_for_wprintw(const string_view str, const int width)
{ string rv;

  int since_last_newline { 0 };

  for (size_t posn { 0 }; posn < str.length(); ++posn)
  { const char c { str[posn] };

    if (c != EOL_CHAR)
    { rv += c;
      since_last_newline++;
    }
    else    // character is EOL
    { if (since_last_newline != width)
        rv += EOL;        // add the explicit EOL

      since_last_newline = 0;
    }
  }

  return rv;
}

/*! \brief          Deal with wprintw's idiotic insertion of newlines when reaching the right hand of a window
    \param  vecstr  vector of strings to be reformatted
    \param  width   width of line in destination window
    \return         <i>str</i> reformatted for the window

    See http://stackoverflow.com/questions/7540029/wprintw-in-ncurses-when-writing-a-newline-terminated-line-of-exactly-the-same
*/
vector<string> reformat_for_wprintw(const vector<string>& vecstr, const int width)
{ vector<string> rv;
  rv.reserve(vecstr.size());

  FOR_ALL(vecstr, [width, &rv] (const string& s) { rv += reformat_for_wprintw(s, width); });

  return rv;
}

/*! \brief          Write a <i>vector<string></i> object to an output stream
    \param  ost     output stream
    \param  vec     object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const vector<string>& vec)
{ unsigned int idx { 0 };

  for (const auto& str : vec)
  { ost << "[" << idx++ << "]: " << str;

    if (idx != vec.size())
      ost << endl;
  }

  return ost;
}

/*! \brief              Perform a case-insensitive search for a substring
    \param  str         string to search
    \param  target      substring for which to search
    \param  start_posn  location in <i>str</i> at which to start the search
    \return             position of the first character in <i>target</i> in <i>str</i>
    
    Returns string::npos if <i>target</i> cannot be found
*/
size_t case_insensitive_find(const std::string_view str, const std::string_view target, const size_t start_posn)
{ auto it { str.cbegin() };

  if (start_posn != 0)
    advance(it, start_posn);
  
  const auto posn { search(it, str.cend (), target.cbegin(), target.cend(), [] (const char ch1, const char ch2) { return toupper(ch1) == toupper(ch2); } ) };

  return ( (posn == str.cend()) ? string::npos : distance(it, posn) + start_posn);
}

/*! \brief              Get the base portion of a call
    \param  callsign    original callsign
    \return             the base portion of <i>callsign</i>

    For example, a call such as VP9/G4AMJ/P returns G4AMJ.
*/
string base_call(const string_view callsign)
{ if (!contains(callsign, '/'))
    return string { callsign };

  return longest( split_string <std::string_view> (callsign, '/') );
}

/*! \brief      Provide a formatted date string: YYYYMMDD
    \return     current UTC date in the format: YYYYMMDD
*/
string YYYYMMDD_utc(void)
{ const string dts { date_time_string(SECONDS::NO_INCLUDE) };

  return (dts.substr(0, 4) + dts.substr(5, 2) + dts.substr(8, 2));
}

/*! \brief      Remove all instances of several substrings (sequentially) from a string
    \param  cs  original string
    \param  vs  vector of substrings to be removed, in the order in which they are to be removed
    \return     <i>cs</i>, with all instances of the elements of <i>vs</i> removed, applied in order
*/
string remove_substrings(const string_view cs, const vector<string>& vs)
{ string rv { cs };
  
  FOR_ALL(vs, [&rv] (const string& str) { rv = remove_substring(rv, str); });
      
  return rv; 
}

/*! \brief          Return position in a string at the end of a target string, if present
    \param  str     string to search
    \param  target  string to find
    \return         position in <>str</i> after the end of <i>target</i>

    Returns string::npos if <i>target</i> is not a substring of <i>str</i> OR if <i>target</i>
    is the conclusion of <i>str</i>
*/
size_t find_and_go_to_end_of(const string_view str, const string_view target)
{ size_t posn { str.find(target) };

  if (posn == string::npos)
    return string::npos;

  const size_t ts { target.size() };

  if ( (posn + ts) == str.size())
    return string::npos;

  return (posn + ts);
}

/*! \brief          Are two calls busts of each other?
    \param  call1   first call
    \param  call2   second call
    \return         whether <i>call1</i> and <i>call2</i> are possible busts of each other

    Testing for busts is a symmetrical function.
*/
bool is_bust_call(const string_view call1, const string_view call2) noexcept
{ if (call1 == call2)
    return false;                // not a bust if both calls are the same

  if ( abs(static_cast<int>(call1.length()) - static_cast<int>(call2.length())) >= 2 )
    return false;                // not a bust if the lengths differ by 2 or more

  if ( abs(static_cast<int>(call1.length()) - static_cast<int>(call2.length())) == 1 )  // lengths differ by unity
  { const string_view longer  { (call1.length() > call2.length() ? call1 : call2) };
    const string_view shorter { (call1.length() > call2.length() ? call2 : call1) };

    if (contains(longer, shorter))
      return true;

// is the bust in the form of an additional character somewhere in the call?
    for (size_t posn { 1 }; posn < longer.length() - 1; ++posn)
    { const string tmp { longer.substr(0, posn) + longer.substr(posn + 1) };    // have to use a string here

      if (tmp == shorter)
        return true;
    }

    return false;
  }

// the calls are the same length; do they differ by exactly one character?
  int differences { 0 };

  for (size_t posn { 0 }; ( (posn < call1.length()) and (differences < 2) ); ++posn)  // can stop looking once we have two differences
  { if (call1[posn] != call2[posn])
      differences++;
  }

  if (differences == 1)
    return true;

// is there a character inversion?
  for (unsigned int posn { 0 }; (posn < call1.length() - 1); ++posn)
  { string call_tmp { call1 };

    swap(call_tmp[posn], call_tmp[posn + 1]);

    if (call_tmp == call2)
      return true;
  }

  return false;
}

/*! \brief        Return a string that contains only normal, printable ASCII characters
    \param  str   string to convert
    \return       <i>str</i> but with all non-printable ASCII characters converted to human-readable binary values
*/
string to_printable_string(const string_view str)
{ string rv      { };
  int    rv_size { 0 };

  auto is_printable = [] (const char c) { const uint8_t ui { static_cast<uint8_t>(c) };

                                          return ( (ui > 31) and (ui < 127) );
                                        };

  for (const char c : str)
    rv_size += (is_printable(c) ? 1 : 10);

  rv.reserve(rv_size);

  for (const char c : str)
  { if (is_printable(c))
      rv += c;
    else
    { rv += '|';

      rv += (c bitand 0b10000000) ? '1' : '0';
      rv += (c bitand 0b01000000) ? '1' : '0';
      rv += (c bitand 0b00100000) ? '1' : '0';
      rv += (c bitand 0b00010000) ? '1' : '0';
      rv += (c bitand 0b00001000) ? '1' : '0';
      rv += (c bitand 0b00000100) ? '1' : '0';
      rv += (c bitand 0b00000010) ? '1' : '0';
      rv += (c bitand 0b00000001) ? '1' : '0';

      rv += '|';
    }
  }

  return rv;
}

/*! \brief                Read an istream until a string delimiter is reached
    \param  in            istream from which to read
    \param  delimiter     the delimiter that mars the end of a record
    \param  keep_or_drop  whether to keep or drop the delimiter in the returned string
    \return               the contents if <i>in</i> from the current point to the delimiter

    https://stackoverflow.com/questions/41851454/reading-a-iostream-until-a-string-delimiter-is-found
*/
string read_until(istream& in, const string_view delimiter, const DELIMITERS keep_or_drop)
{ string cr;

  const char   delim { *(delimiter.rbegin()) };
  const size_t sz    { delimiter.size() };

  size_t tot;

  do
  { string temp;

    getline(in, temp, delim);
    cr += temp + delim;
    tot = cr.size();
  } while ((tot < sz) or (cr.substr(tot - sz, sz) != delimiter));

  return ((keep_or_drop == DELIMITERS::DROP) ? cr.substr(0, tot - sz) : cr);
}

/*! \brief          Convert string to hex characters
    \param  str     string to convert
    \return         <i>str</i> as a series ofspace-separated  hex characters
*/
string hex_string(const string_view str)
{ ostringstream stream;

  stream << hex << setfill('0');

  for (const auto c : str)
    stream << setw(2) << static_cast<int>(static_cast<unsigned char>(c)) << ' ';  // setw needs to be sent for every character!

  return stream.str();
}
