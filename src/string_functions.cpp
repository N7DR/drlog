// $Id: string_functions.cpp 89 2015-01-03 13:59:15Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#include "macros.h"
#include "string_functions.h"

#include <iostream>

#include <cctype>
#include <cstdio>

#include <langinfo.h>

#include <arpa/inet.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>

using namespace std;

extern ofstream ost;                   ///< for debugging, info

/*! \brief convert from a CSV line to a vector of strings, each containing one field
    \param  line    CSV line
    \return         vector of fields from the CSV line

    This is actually quite difficult to do properly
*/
const vector<string> from_csv(const string& line)
{ static const char quote = '"';
  static const char comma = ',';

  vector<string> rv;
  size_t posn = 0;

  string this_value;
  bool inside_value = false;;

  while (posn < line.size())
  { const char& this_char = line[posn];

    if (this_char == quote)
    { if (inside_value)               // we're inside a field
      { if (posn < line.size() - 1)   // make sure there's at least one unread character
        { const char next_char = line[posn + 1];

          if (next_char == quote)     // it's a doubled quote
          { posn += 2;                // skip the next character
            this_value += quote;
          }
          else                        // it's the end of the value
          { rv.push_back(this_value);
            inside_value = false;
            posn++;
          }
        }
        else                          // there are no more unread characters; declare this as the end
        { rv.push_back(this_value);
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
          { const char next_char = line[posn + 1];

            if (next_char == comma)
            { rv.push_back(string());   // empty value
              posn++;
            }
            else
              posn++;
          }
          else                        // we've finished with a comma; this is really an error, we just assume an empty last field
          { rv.push_back(string());   // empty value
            posn++;
          }
        }
      }
    }
  }

  return rv;
}

/* \brief       duplicate a particular character within a string
    \param  s   string in which characters are to be duplicated
    \param  c   character to be duplicated
    \return     <i>s</i>, modified so that every instance of <i>c</i> is doubled
*/
const string duplicate_char(const string& s, const char& c)
{ string rv;

  for (size_t n = 0; n < s.length(); ++n)
  { if (s[n] == c)
      rv += c;

    rv += s[n];
  }

  return rv;
}

/*! \brief              safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \param  length      length of substring to be extracted
    \return             substring of length <i>length</i>, starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn, length)</i>, except does not throw a range exception
*/
const string substring(const string& str, const size_t start_posn, const size_t length)
{ if (str.size() > start_posn)
    return str.substr(start_posn, length);

   ost << "range problem in substring(); str = " << str << ", string length = " << str.length() << ", start_posn = " << start_posn << ", length = " << length << endl;  // log the problem

  return string();
}

/*! \brief              safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \return             substring starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn)</i>, except does not throw a range exception
*/
const string substring(const string& str, const size_t start_posn)
{ if (str.size() > start_posn)
    return str.substr(start_posn);

  ost << "range problem in substring(); str = " << str << ", string length = " << str.length() << ", start_posn = " << start_posn << endl;    // log the problem

  return string();
}

/*! \brief  provide a formatted date/time string
    \return current date and time in the format: YYYY-MM-DDTHH:MM
*/
const string date_time_string(void)
{ const time_t now = time(NULL);             // get the time from the kernel
  struct tm    structured_time;

  gmtime_r(&now, &structured_time);          // convert to UTC

  char buf[26];                                // buffer to hold the ASCII date/time info; see man page for gmtime()

  asctime_r(&structured_time, &buf[0]);                   // convert to ASCII

  const string ascii_time(&buf[0], 26);                   // this is a modern language

  const string _utc  = ascii_time.substr(11, 5);                                                     // hh:mm
  const string _date = to_string(structured_time.tm_year + 1900) + "-" +
                         pad_string(to_string(structured_time.tm_mon + 1), 2, PAD_LEFT, '0') + "-" +
                         pad_string(to_string(structured_time.tm_mday), 2, PAD_LEFT, '0');             // yyyy-mm-dd

  return _date + "T" + _utc;
}

/*! \brief  convert struct tm pointer to formatted string
    \param  format  format to be used
    \param  tmp     date/time to be formatted
    \return         formatted string

    Uses strftime() to perform the formatting
*/
const string format_time(const string& format, const tm* tmp)
{ const unsigned int BUFLEN = 60;
  char buf[BUFLEN];
  const size_t nchars = strftime(buf, BUFLEN, format.c_str(), tmp);

  if (!nchars)
    throw string_function_error(STRING_CONVERSION_FAILURE, "Unable to format time");

  return string(buf);
}

/*! \brief              replace every instance of one character with another
    \param  s           string on which to operate
    \param  old_char    character to be replaced
    \param  new_char    replacement character
    \return             <i>s</i>, with every instance of <i>old_char</i> replaced by <i>new_char</i>
*/
const string replace_char(const string& s, char old_char, char new_char)
{ string rv;

//  for (size_t n = 0; n < s.length(); ++n)
//    rv += ( (s[n] == old_char) ? new_char : s[n] );

  FOR_ALL(s, [=, &rv] (const char c) { rv += ( (c == old_char) ? new_char : c ); } );

  return rv;
}

/*! \brief              replace every instance of one string with another
    \param  s           string on which to operate
    \param  old_str     string to be replaced
    \param  new_str     replacement string
    \return             <i>s</i>, with every instance of <i>old_str</i> replaced by <i>new_str</i>
*/
const string replace(const string& s, const string& old_str, const string& new_str)
{ string rv;
  size_t posn = 0, last_posn = 0;

  while ((posn = s.find(old_str, last_posn)) != string::npos)
  { rv += s.substr(last_posn, posn - last_posn) + new_str;
    last_posn = posn + old_str.length();
  }

  rv += s.substr(last_posn);

  return rv;
}

/*! \brief  pad a string to a particular size
    \param  s original string
    \param  len length of returned string
    \param  pad_direction side on which to pad
    \param  pad_char  character with which to pad
    \return padded version of <i>s</i>
  
  If <i>s</i> is already longer than <i>len</i>, then <i>s</i> is returned.
*/
const string pad_string(const string& s, const unsigned int len, const enum pad_direction pad_side, const char pad_char)
{ string rv = s;

  if (rv.length() >= len)
    return rv;
  
  const unsigned int n_pad_chars = len - rv.length();
  const string pad_string(n_pad_chars, pad_char);
  
  if (pad_side == PAD_LEFT)
    rv = pad_string + rv;
  else
    rv += pad_string;
  
  return rv;
}

/*! \brief  Read the contents of a file into a single string
  \param  filename  Name of file to be read
  \return Contents of file <i>filename</i>
  
  Throws exception if the file does not exist, or if any
  of several bad things happen. Assumes that the file is a reasonable length.
*/
const string read_file(const string& filename)
{ FILE* fp = fopen(filename.c_str(), "rb");

  if (!fp)
    throw string_function_error(STRING_INVALID_FILE, "Cannot open file: " + filename);

// check that the file is not a directory  
  struct stat stat_buffer;
  int status = ::stat(filename.c_str(), &stat_buffer);

  if (status)
    throw string_function_error(STRING_UNABLE_TO_STAT_FILE, "Unable to stat file: " + filename);

  const bool is_directory = ((stat_buffer.st_mode & S_IFDIR) != 0);

  if (is_directory)
    throw string_function_error(STRING_FILE_IS_DIRECTORY, filename + (string)" is a directory");

// get the length of the file
  fseek(fp, 0, SEEK_END);
  unsigned long file_length = ftell(fp);
  
// go to the start
  if (file_length)
  { fseek(fp, 0, SEEK_SET);

// create a buffer to hold the contents
    unsigned char* buf = new unsigned char [file_length];
  
// read and close the file
    const size_t nread = fread(buf, file_length, 1, fp);

    if (nread != 1)
    { fclose(fp);
      delete [] buf;
      throw exception();
    }

    fclose(fp);

    const string rv((const char*)(buf), file_length);    // convert to a real string
    delete [] buf;                                       // delete the buffer that held the file
    return rv;
  }
  else
    fclose(fp);

  return string();
}

/*! \brief  Read the contents of a file into a single string
    \param  path the different directories to try, in order
    \param  filename  Name of file to be read
    \return Contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
const string read_file(const vector<string>& path, const string& filename)
{ for (const auto& this_path : path)
  { try
    { return read_file(this_path + "/" + filename);
    }

    catch (...)
    {
    }
  }

  throw string_function_error(STRING_INVALID_FILE, "Cannot open file: " + filename + " with non-trivial path");
}

// write a string to a binary file; throws exception if cannot be written
void write_file(const string& s, const string& filename)
{ FILE* fp = fopen(filename.c_str(), "wb");
  if (fp == 0)
//    throw stringmanip_error(STRINGMANIP_INVALID_FILE, "Cannot open file: " + filename);
    throw exception();

// don't do this the obvious way because we don't know anything about threads here
//  try
  if (s.length())
  { char* cp = new char [s.length()];

    for (unsigned int n = 0; n < s.length(); n++)
      cp[n] = s[n];

    fwrite(cp, s.length(), 1, fp);

    delete [] cp;
  }
  fclose(fp);
}

/*! \brief  split a string into components
  \param  cs  Original string
  \param  separator Separator string (typically a single character)

  \return Vector containing the separate components
*/
const vector<string> split_string(const string& cs, const string& separator)
{ size_t start_posn = 0;
  vector<string> rv;

  while (start_posn < cs.length())
  { unsigned long posn = cs.find(separator, start_posn);

    if (posn == string::npos)                       // no more separators
    { rv.push_back(cs.substr(start_posn));
      start_posn = cs.length();
    }
    else                                            // at least one separator
    { rv.push_back(cs.substr(start_posn, posn - start_posn));
      start_posn = posn + separator.length();
    }
  }

  return rv;
}

/*!     \brief  split a string into equal-length records
        \param  cs              Original string
        \param  record_length   Length of each record

        \return Vector containing the separate components

        Any non-full record at the end is silently discarded
*/
const vector<string> split_string(const string& cs, const unsigned int record_length)
{ vector<string> rv;
  string cp = cs;

  while (cp.length() >= record_length)
  { rv.push_back(cp.substr(0, record_length));
    cp = cp.substr(record_length);
  }

  return rv;
}

/// squash repeated occurrences of a character
const string squash(const string& cs, const char c)
{ string rv = cs;
  const string separator(1, c);
  size_t posn;

  while ((posn = rv.find(separator + separator), posn) != string::npos)
    rv = rv.substr(0, posn) + rv.substr(posn + separator.length()); 
  
  return rv;
}

/*! \brief      Join the elements of a string vector, using a provided separator
    \param  vec container of strings
    \param  sep separator inserted between the elements of <i>vec</i>
    \return     all the elements of <i>vec</i>, concatenated, but with <i>sep</i> inserted between elements
*/
const string join(const vector<string>& vec, const string& sep)
{ string rv;

  for (unsigned int n = 0; n < vec.size(); ++n)
  { rv += vec[n];

    if (n != vec.size() - 1)
      rv += sep;
  }
  
  return rv;
}

// join all the elements of a string array together, with a known separator
const string join(const deque<string>& vec, const string& sep)
{ string rv;

  for (unsigned int n = 0; n < vec.size(); ++n)
  { rv += vec[n];

    if (n != vec.size() - 1)
      rv += sep;
  }
  
  return rv;
}

/*! \brief remove characters from the end of a string
  \param  s original string
  \param  n number of chars to remove
  \return <i>s</i> with the last <i>n</i> characters removed
  
  If <i>n</i> is equal to or greater than the length of <i>s</i>, then
  the empty string is returned.
*/
const string remove_from_end(const string& s, const unsigned int n)
{ if (n >= s.length())
    return s;
    
  return s.substr(0, s.length() - n);
}

/*! \brief  Remove all instances of a specific leading character
  \param  cs  Original string
  \param  c Leading character to remove (if present)
  \return <i>cs</i> with any leading octets with the value <i>c</i> removed
*/
const string remove_leading(const string& str, const char c)
{ string rv = str;

  while (rv.length() && (rv[0] == c))
    rv = rv.substr(1);
  
  return rv;
}

/*! \brief  Remove all instances of a specific trailing character
  \param  cs  Original string
  \param  c Trailing character to remove (if present)
  \return <i>cs</i> with any trailing octets with the value <i>c</i> removed
*/
const string remove_trailing(const string& s, const char c)
{ string rv = s;

  while (rv.length() && (rv[rv.length() - 1] == c))
    rv = rv.substr(0, rv.length() - 1);
  
  return rv;
}

/*! \brief  Remove all instances of a particular char from a string
  \param  cs  Original string
  \param  char_to_remove  Character to be removed from <i>cs</i>
  \return <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
const string remove_char(const string& s, const char c)
{ string rv;

  for (unsigned int n = 0; n < s.length(); n++)
    if (s[n] != c)
      rv += s[n];

  return rv; 
} 

/*! \brief  Obtain a delimited substring
  \param  cs  Original string
  \param  delim_1 opening delimiter
  \param  delim_2 closing delimiter
  \return substring between <i>delim_1</i> and <i>delim_2</i>
  
  Returns the empty string if the delimiters do not exist, or if
  <i>delim_2</i> does not appear after <i>delim_1</i>
*/
const string delimited_substring(const string& cs, const char delim_1, const char delim_2)
{ const size_t posn_1 = cs.find(delim_1);
  
  if (posn_1 == string::npos)
    return string();  
  
  const size_t posn_2 = cs.find(delim_2, posn_1 + 1);
  
  if (posn_2 == string::npos)
    return string();
  
  return cs.substr(posn_1 + 1, posn_2 - posn_1 - 1);
}

/*! \brief  Obtain a delimited substring
  \param  s Original string
  \param  delim_1 opening delimiter
  \param  delim_2 closing delimiter
  \param  del     whether to remove substrings and delimiters from <i>s</i>
  \return substring between <i>delim_1</i> and <i>delim_2</i>
  
  Returns the empty string if the delimiters do not exist, or if
  <i>delim_2</i> does not appear after <i>delim_1</i>
*/
/*
const string delimited_substring(string& s, const char delim_1, const char delim_2, const bool del)
{ const size_t posn_1 = s.find(delim_1);
  
  if (posn_1 == string::npos)
    return string();  
  
  const size_t posn_2 = s.find(delim_2, posn_1 + 1);
  
  if (posn_2 == string::npos)
    return string();
  
  const string rv = s.substr(posn_1 + 1, posn_2 - posn_1 - 1);

  if (del)
    s = s.substr(0, posn_1) + s.substr(posn_2 + 1);  
    
  return rv;  
}
*/

const string create_centred_string(const string& str, const unsigned int width)
{ const size_t len = str.length();

  if (len > width)
    return substring(str, 0, width);

  if (len == width)
    return str;

  const string l = create_string(' ', (width - len) / 2);
  const string r = create_string(' ', width - len - l.length());

  return (l + str + r);
}

// simple functions for chars near the end of strings
/// the last character in a string
const char last_char(const string& cs)
{ if (cs.empty())
    throw string_function_error(STRING_BOUNDS_ERROR, "Attempt to access character in empty string");
    
  return cs[cs.length() - 1];
}

/// the penultimate character in a string
const char penultimate_char(const std::string& cs)
{ if (cs.length() < 1)
    throw string_function_error(STRING_BOUNDS_ERROR, "Attempt to access character beyond end of string");
    
  return cs[cs.length() - 2];
}

/// the antepenultimate character in a string
const char antepenultimate_char(const std::string& cs)
{ if (cs.length() < 2)
    throw string_function_error(STRING_BOUNDS_ERROR, "Attempt to access character beyond end of string");
    
  return cs[cs.length() - 3];
}

/*!   \brief  get an environment variable
  \return the environment variable

  returns the empty string if the variable does not exist
*/
const string get_environment_variable(const std::string& var_name)
{ const char* cp = getenv(var_name.c_str());

  if (!cp)
    return string();
  else
    return string(cp);
  
}

const string transform_string(const string& cs, int(*pf)(int))
{ string rv = cs;
  
  transform(rv.begin(), rv.end(), rv.begin(), pf);
  
  return rv;
}

/// convert an integer to a comma-separated string
const string separated_string(const int n, const string& sep)
{ // char separator = ',';
  const char separator = sep.at(0);

  string tmp = to_string(n);
  string rv;
  
  while (!tmp.empty())
  { for (unsigned int N = 0; N < 3 and !tmp.empty(); ++N)
    { rv = string(1, last_char(tmp)) + rv;
      tmp = tmp.substr(0, tmp.length() - 1);
    }
    
    if (!tmp.empty())
      rv = string(1, separator) + rv;
  }
  
  return rv;
}

/// return the starting position for each word
const vector<size_t> starts_of_words(const string& s)
{ vector<size_t> rv;

  if (s.empty())
    return rv;

// start of first word
  size_t posn = s.find_first_not_of(" ", 0);

  if (posn == string::npos)
    return rv;

  rv.push_back(posn);

// next space
  while (1)
  { posn = s.find_first_of(" ", posn);

    if (posn == string::npos)
      return rv;

    posn = s.find_first_not_of(" ", posn);

    if (posn == string::npos)
      return rv;

    rv.push_back(posn);
  }
}

/// get location of start of next word
const size_t next_word_posn(const string& str, const size_t current_posn)
{ if (str.length() <= current_posn)
    return string::npos;

  const bool is_space = (str[current_posn] == ' ');

  if (is_space)
    return ( str.find_first_not_of(" ", current_posn) );

// we are inside a word
  const size_t space_posn = str.find_first_of(" ", current_posn);
  const size_t word_posn = str.find_first_not_of(" ", space_posn);

  return word_posn;
}

// get nth word
const string nth_word(const string& s, const unsigned int n, const unsigned int wrt)
{ string rv;

  if (n < wrt)
    return rv;

  const unsigned int actual_word_number = n - wrt;
  const vector<size_t> starts = starts_of_words(s);

  if (actual_word_number >= starts.size())
    return rv;

  const size_t posn_1 = starts[actual_word_number];
  const size_t posn_2 = ( (actual_word_number + 1) >= starts.size() ? string::npos : starts[actual_word_number + 1] );

  rv = substring(s, posn_1, posn_2 - posn_1);
  rv = remove_peripheral_spaces(rv);

  return rv;
}

// https://stackoverflow.com/questions/4063146/getting-the-actual-length-of-a-utf-8-encoded-stdstring
const size_t n_chars(const string& str)
{ if (string(nl_langinfo(CODESET)) != "UTF-8")
    throw string_function_error(STRING_UNKNOWN_ENCODING, "Unknown character encoding: " + string(nl_langinfo(CODESET)));

  const size_t n_bytes = str.size();
  char* cp = const_cast<char*>(str.data());
  char* end_cp = cp + n_bytes;  // one past the end of the contents of str
  size_t rv = 0;

  while (cp < end_cp)
  { if ( (*cp++ & 0xc0) != 0x80 )
      rv++;
  }

  return rv;
}

/*!     \brief  Does a string contain a legal dotted-decimal IPv4 address
        \param  cs  Original string
        \return  Whether <i>cs</i> contains a legal dotted decimal IPv4 address
*/
const bool is_legal_ipv4_address(const string& cs)
{ static const string separator(".");
  string tmp_str = cs;

  for (int field = 0; field < 3; field++)
  { const unsigned long posn = tmp_str.find(separator);
    if (posn == string::npos)
      return false;

    const string this_field = tmp_str.substr(0, posn);
    try
    { long value = from_string<long>(this_field);
      if ((value < 0) || (value > 255))
        return false;
    }

    catch (...)                 // caught exception(); string could not be converted
    { return false;
    }

    tmp_str = tmp_str.substr(posn + 1);
  }

// we still have to do the last field
  try
  { long value = from_string<long>(tmp_str);
    if ((value < 0) || (value > 255))
      return false;
  }

  catch (...)                 // caught exception(); string could not be converted
  { return false;
  }  

  return true;
}

/*!     \brief  Convert a long to dotted decimal string
        \param  val     Original value
        \return Dotted decimal string
        
        Assumes that a long is four octets
*/
string convert_to_dotted_decimal(const uint32_t val)
{ static const string separator(".");

// put into network order (so that we can guarantee the order of the octets in the long)
  const uint32_t network_val = htonl(val);
  unsigned char* cp = (unsigned char*)(&network_val);
  string rv;

  for (int n = 0; n < 3; n++)
  { unsigned char c = cp[n];
  
    rv += to_string((int)c) + separator;
  }

  unsigned char c = cp[3];

  rv += to_string((int)c);

  return rv;
}

/// is a string a legal value from a list?
const bool is_legal_value(const string& value, const string& legal_values, const string& separator)
{ const vector<string> vec = split_string(legal_values, separator);

  return (find(vec.begin(), vec.end(), value) != vec.end());
}

// is c1 < c2?
// changes to ordinary sort order:
//    '0' is the highest digit
//    numbers sort after letters
//    '/' comes after all digits and letters
const bool compchar(const char c1, const char c2)
{ if (c1 == c2)
    return false;

  if (c1 == '/')
    return false;

  if (c2 == '/')
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
}

// return true if call1 < call2
const bool compare_calls(const string& s1, const string& s2)
{ const size_t l1 = s1.size();
  const size_t l2 = s2.size();
  const size_t n_to_compare = min(l1, l2);

  size_t index = 0;

  while (index < n_to_compare)
  { if (s1[index] != s2[index])
      return compchar(s1[index], s2[index]);

    index++;
  }

  if (l1 < l2)
    return true;

  return false;
}

/*! \brief does a string contain any letters?
 *
 *  This should be faster than the find_next_of() or C++ is_letter or similar generic functions
*/
const bool contains_letter(const std::string& str)
{ for (unsigned int n = 0; n < str.size(); ++n)
  { const char& c = str[n];

    if ( (c >= 'A' and c <='Z') or (c >= 'a' and c <='z') )
      return true;
  }

  return false;
}

/*! \brief does a string contain any digits?
 *
 *  This should be faster than the find_next_of() or C++ is_digit or similar generic functions
*/
const bool contains_digit(const std::string& str)
{ for (unsigned int n = 0; n < str.size(); ++n)
  { const char& c = str[n];

    if (c >= '0' and c <= '9' )
      return true;
  }

  return false;
}

