// $Id: string_functions.cpp 169 2020-10-18 17:16:44Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   string_functions.cpp

    Functions related to the manipulation of strings
*/

#include "macros.h"
#include "string_functions.h"

#include <cctype>
#include <cstdio>

#include <langinfo.h>

#include <arpa/inet.h>
#include <sys/stat.h>

using namespace std;

extern ofstream ost;                   ///< for debugging, info

const string     EOL      { "\n" };       ///< end-of-line marker as string
constexpr char   EOL_CHAR { '\n' };       ///< end-of-line marker as character

const string  LF       { "\n" };      ///< LF as string
const string& LF_STR   { LF };        ///< LF as string
constexpr char    LF_CHAR  { '\n' };      ///< LF as character

/*! \brief          Convert from a CSV line to a vector of strings, each containing one field
    \param  line    CSV line
    \return         vector of fields from the CSV line

    This is actually quite difficult to do properly
*/
vector<string> from_csv(experimental::string_view line)
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
          { const char next_char { line[posn + 1] };

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

/*! \brief      Duplicate a particular character within a string
    \param  s   string in which characters are to be duplicated
    \param  c   character to be duplicated
    \return     <i>s</i>, modified so that every instance of <i>c</i> is doubled
*/
string duplicate_char(const string& s, const char c)
{ size_t start_posn { 0 };
  size_t next_posn  { 0 };
  string rv         { };

  do
  { next_posn = s.find(c, start_posn);

    if (next_posn != string::npos)
    { rv += s.substr(start_posn, next_posn - start_posn + 1);
      rv += c;
      start_posn = next_posn + 1;
    }
  } while (next_posn != string::npos);

  rv += s.substr(start_posn);

  return rv;
}

/*! \brief              Safe version of the substr() member function
    \param  str         string on which to operate
    \param  start_posn  position at which to start operation
    \param  length      length of substring to be extracted
    \return             substring of length <i>length</i>, starting at position <i>start_posn</i>

    Operates like <i>str.substr(start_posn, length)</i>, except does not throw a range exception
*/
string substring(const string& str, const size_t start_posn, const size_t length)
{ if (str.size() > start_posn)
    return str.substr(start_posn, length);

   ost << "range problem in substring(); str = " << str << ", string length = " << str.length() << ", start_posn = " << start_posn << ", length = " << length << endl;  // log the problem

  return string();
}

/*! \brief                      Provide a formatted date/time string
    \param  include_seconds     whether to include the portion oft he string that designates seconds
    \return                     current date and time in the format: YYYY-MM-DDTHH:MM or YYYY-MM-DDTHH:MM:SS
*/
string date_time_string(const bool include_seconds)
{ constexpr size_t TIME_BUF_LEN { 26 };

  const time_t now { time(NULL) };            // get the time from the kernel

  struct tm    structured_time;

  gmtime_r(&now, &structured_time);         // convert to UTC

  array<char, TIME_BUF_LEN> buf;                           // buffer to hold the ASCII date/time info; see man page for gmtime()

  asctime_r(&structured_time, buf.data());                     // convert to ASCII

  const string ascii_time { buf.data(), TIME_BUF_LEN };
  const string _utc       { ascii_time.substr(11, (include_seconds ? 8 : 5)) };                            // hh:mm[:ss]
  const string _date      { to_string(structured_time.tm_year + 1900) + "-" +
                              pad_string(to_string(structured_time.tm_mon + 1), 2, PAD_LEFT, '0') + "-" +
                              pad_string(to_string(structured_time.tm_mday), 2, PAD_LEFT, '0') };          // yyyy-mm-dd

  return (_date + "T" + _utc);
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

  const size_t nchars { strftime(buf, BUFLEN, format.c_str(), tmp) };

  if (!nchars)
    throw string_function_error(STRING_CONVERSION_FAILURE, "Unable to format time");

  return string(buf);
}

/*! \brief              Replace every instance of one character with another
    \param  s           string on which to operate
    \param  old_char    character to be replaced
    \param  new_char    replacement character
    \return             <i>s</i>, with every instance of <i>old_char</i> replaced by <i>new_char</i>
*/
string replace_char(const string& s, char old_char, char new_char)
{ string rv;

  FOR_ALL(s, [=, &rv] (const char c) { rv += ( (c == old_char) ? new_char : c ); } );

  return rv;
}

/*! \brief              Replace every instance of one string with another
    \param  s           string on which to operate
    \param  old_str     string to be replaced
    \param  new_str     replacement string
    \return             <i>s</i>, with every instance of <i>old_str</i> replaced by <i>new_str</i>
*/
string replace(const string& s, const string& old_str, const string& new_str)
{ string rv        { };
  size_t posn      { 0 };
  size_t last_posn { 0 };

  while ((posn = s.find(old_str, last_posn)) != string::npos)
  { rv += s.substr(last_posn, posn - last_posn) + new_str;
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
string pad_string(const string& s, const size_t len, const enum pad_direction pad_side, const char pad_char)
{ string rv { s };

  if (rv.length() >= len)
    return rv;
  
  const size_t n_pad_chars { len - rv.length() };
  
  const string pstring(n_pad_chars, pad_char);  // cannot use initializer-list

  return ( (pad_side == PAD_LEFT) ? pstring + rv : rv + pstring );
}

/*! \brief              Read the contents of a file into a single string
    \param  filename    name of file to be read
    \return             contents of file <i>filename</i>
  
    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
string read_file(const string& filename)
{  //std::ifstream file("file.ignore");
   //std::string str{std::istreambuf_iterator<char>(file), {}};


//  std::ifstream file("file.ignore", std::ios::ate);
//  auto sz = file.tellg();
//  file.seekg(0);
//  std::string str(sz, '\0');
//  file.read(&str[0], sz);

// start by performing a bunch of checks

  FILE* fp { fopen(filename.c_str(), "rb") };

  if (!fp)
    throw string_function_error(STRING_INVALID_FILE, "Cannot open file: "s + filename);
  else
    fclose(fp);

// check that the file is not a directory  
  struct stat stat_buffer;

  const int status { ::stat(filename.c_str(), &stat_buffer) };

  if (status)
    throw string_function_error(STRING_UNABLE_TO_STAT_FILE, "Unable to stat file: "s + filename);

  const bool is_directory { ( (stat_buffer.st_mode bitand S_IFDIR) != 0 ) };

  if (is_directory)
    throw string_function_error(STRING_FILE_IS_DIRECTORY, filename + " is a directory"s);

// now perform the actual read
  std::ifstream file { filename };
  std::string   str  {std::istreambuf_iterator<char>(file), {} };

  return str;
}

/*! \brief              Read the contents of a file into a single string
    \param  path        the different directories to try, in order
    \param  filename    name of file to be read
    \return             contents of file <i>filename</i>

    Throws exception if the file does not exist, or if any
    of several bad things happen. Assumes that the file is a reasonable length.
*/
string read_file(const vector<string>& path, const string& filename)
{ for (const auto& this_path : path)
  { try
    { return read_file(this_path + "/"s + filename);
    }

    catch (...)
    {
    }
  }

  throw string_function_error(STRING_INVALID_FILE, "Cannot open file: "s + filename + " with non-trivial path"s);
}

/*! \brief              Write a string to a (binary) file
    \param  cs          string to write
    \param  filename    name of file to be written

    Throws exception if the file cannot be written
*/

//void write_file(const string& cs, const string& filename)
//{ //ofstream outfile(filename.c_str(), ofstream::binary);

  //outfile << cs;

//  ofstream(filename.c_str(), ofstream::binary) << cs;

#if 0
  FILE* fp = fopen(filename.c_str(), "wb");

  if (fp == 0)
    throw string_function_error(STRING_UNWRITEABLE_FILE, "Cannot write to file: " + filename);

  if (cs.length())
  { char* cp = new char [cs.length()];

    for (unsigned int n = 0; n < cs.length(); ++n)
      cp[n] = cs[n];

    fwrite(cp, cs.length(), 1, fp);
    delete [] cp;
  }

  fclose(fp);
#endif
//}

/*! \brief              Split a string into components
    \param  cs          original string
    \param  separator   separator string (typically a single character)
    \return             vector containing the separate components
*/
vector<string> split_string(const string& cs, const string& separator)
{ size_t start_posn { 0 };

  vector<string> rv;

  while (start_posn < cs.length())
  { if (unsigned long posn { cs.find(separator, start_posn) }; posn == string::npos)                       // no more separators
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

/*! \brief                  Split a string into equal-length records
    \param  cs              original string
    \param  record_length   length of each record
    \return                 vector containing the separate components

    Any non-full record at the end is silently discarded
*/
vector<string> split_string(const string& cs, const unsigned int record_length)
{ vector<string> rv;

  string cp { cs };

  while (cp.length() >= record_length)
  { rv.push_back(cp.substr(0, record_length));
    cp = cp.substr(record_length);
  }

  return rv;
}

/*! \brief      Squash repeated occurrences of a character
    \param  cs  original string
    \param  c   character to squash
    \return     <i>cs</i>, but with all consecutive instances of <i>c</i> converted to a single instance
*/
string squash(const string& cs, const char c)
{ auto both_match = [=](const char lhs, const char rhs) { return ( (lhs == rhs) and (lhs == c) ); }; ///< do both match the target character?

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
{ vector<string> rv;

  FOR_ALL(lines, [&rv] (const string& line) { if (!line.empty()) rv.push_back(line); } );

  return rv;
}

/*! \brief      Remove all instances of a specific leading character
    \param  cs  original string
    \param  c   leading character to remove (if present)
    \return     <i>cs</i> with any leading octets with the value <i>c</i> removed
*/
string remove_leading(const string& cs, const char c)
{ if (cs.empty())
    return cs;

  const size_t posn { cs.find_first_not_of(create_string(c)) };

  return ( (posn == string::npos) ? cs : substring(cs, posn) );
}

/*! \brief      Remove all instances of a specific trailing character
    \param  cs  original string
    \param  c   trailing character to remove (if present)
    \return     <i>cs</i> with any trailing octets with the value <i>c</i> removed
*/
string remove_trailing(const string& cs, const char c)
{ string rv { cs };

  while (rv.length() && (rv[rv.length() - 1] == c))
    rv = rv.substr(0, rv.length() - 1);
  
  return rv;
}

/*! \brief                  Remove all instances of a particular char from a string
    \param  cs              original string
    \param  char_to_remove  character to be removed from <i>cs</i>
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed
*/
string remove_char(const string& cs, const char char_to_remove)
{ string rv { cs };

  rv.erase( remove(rv.begin(), rv.end(), char_to_remove), rv.end() );

  return rv;
} 

/*! \brief                  Remove all instances of a particular char from all delimited substrings
    \param  cs              original string
    \param  char_to_remove  character to be removed from delimited substrings in <i>cs</i>
    \param  delim_1         opening delimiter
    \param  delim_2         closing delimiter
    \return                 <i>cs</i> with all instances of <i>char_to_remove</i> removed from inside substrings delimited by <i>delim_1</i> and <i>delim_2</i>

    delimiters are kept in the output

//    should replace this by a method that uses find()
//    or for each found delimited substring, execute remove_char
//    or build vector of delimited substrings, then remove char from them, then recombine
*/
string remove_char_from_delimited_substrings(const string& cs, const char char_to_remove, const char delim_1, const char delim_2)
{ 
#if 1
// this should be faster than the next one
  string rv;

  size_t start_posn { 0 };

  while (true)
  { size_t posn_1 { cs.find(delim_1, start_posn) };

    if (posn_1 == string::npos)
      return (rv + cs.substr(start_posn));

// we found the first delimiter
    size_t posn_2 { cs.find(delim_2, posn_1 + 1) };

// if there is no matching second delimiter, pretend second delimiter is at end
    if (posn_2 == string::npos)
      return (rv + remove_char(substring(cs, posn_1), char_to_remove));

// we have matching delimiters
    size_t substr_length   { posn_2 - posn_1 + 1 };
    string modified_substr { remove_char(substring(cs, posn_1, substr_length), char_to_remove) };  // keeps the delimiters in the string

    rv += ( cs.substr(start_posn, posn_1) + modified_substr );

    start_posn = posn_2 + 1;

    if (start_posn >= cs.length())
      return rv;
  }

// should never get here
  return rv;
#endif

#if 0
  string rv;

  bool inside_delimiters { false };

  for (unsigned int n = 0; n < cs.length(); n++)
  { const char& c { cs[n] };

    if (!inside_delimiters or (c != char_to_remove))
      rv += c;

    if (c == delim_1)
      inside_delimiters = true;

    if (c == delim_2)
      inside_delimiters = false;
  }

  return rv;
#endif
}

/*! \brief                      Remove all instances of particular characters from a string
    \param  s                   original string
    \param  chars_to_remove     string whose characters are to be removed from <i>s</i>
    \return                     <i>s</i> with all instances of the characters in <i>chars_to_remove</i> removed
*/
string remove_chars(const string& s, const string& chars_to_remove)
{ string rv { s };

  rv.erase( remove_if(rv.begin(), rv.end(), [=](const char& c) { return chars_to_remove.find(c) != string::npos; } ), rv.end() );

  return rv;
}

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
string delimited_substring(const string& cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters)
{ const size_t posn_1 { cs.find(delim_1) };
  
  if (posn_1 == string::npos)
    return string();  
  
  const size_t posn_2 { cs.find(delim_2, posn_1 + 1) };
  
  if (posn_2 == string::npos)
    return string();
  
  return ( (return_delimiters == DELIMITERS::DROP) ? cs.substr(posn_1 + 1, posn_2 - posn_1 - 1) : cs.substr(posn_1, posn_2 - posn_1) ) ;
}

/*! \brief                      Obtain all occurrences of a delimited substring
    \param  cs                  original string
    \param  delim_1             opening delimiter
    \param  delim_2             closing delimiter
    \param  return_delimiters   whether to keep delimiters in the returned value
    \return                     all substrings between <i>delim_1</i> and <i>delim_2</i>, possibly including the delimiters
*/
vector<string> delimited_substrings(const string& cs, const char delim_1, const char delim_2, const DELIMITERS return_delimiters)
{ vector<string> rv;

  size_t start_posn { 0 };      // start posn is, and remains global (i.e., wrt cs)

//  ost << "cs: " << cs << endl;
//  ost << "start delim = " << delim_1 << ", end delim = " << delim_2 << endl;

//  int counter { 0 };

  while ( (start_posn < cs.length() and !substring(cs, start_posn).empty()) )  // initial test so substring() doesn't write to output
  { //ost << "start_posn = " << start_posn << endl;

    const string& sstring { substring(cs, start_posn) };
    const size_t  posn_1  { sstring.find(delim_1) };

    if (posn_1 == string::npos)             // no more starting delimiters
      return rv;

    const size_t posn_2 { sstring.find(delim_2, posn_1 + 1) };

    if (posn_2 == string::npos)
      return rv;                            // no more ending delimiters

    if (return_delimiters == DELIMITERS::KEEP)
      rv.push_back( sstring.substr(posn_1, posn_2 - posn_1) );
    else
      rv.push_back( sstring.substr(posn_1 + 1, posn_2 - posn_1 - 1) );

    start_posn += (posn_2 + 1);   // remember, start_posn is global

 //   if (counter++ > 10)
 //     exit(-1);
  }

  return rv;
}

/*! \brief          Centre a string
    \param  str     string to be centred
    \param  width   final width of the centred string
    \return         <i>str</i> centred in a string of spaces, with total size <i>width</i>,
*/
string create_centred_string(const string& str, const unsigned int width)
{ const size_t len { str.length() };

  if (len > width)
    return substring(str, 0, width);

  if (len == width)
    return str;

  const string l { create_string(' ', (width - len) / 2) };
  const string r { create_string(' ', width - len - l.length()) };

  return (l + str + r);
}

/*! \brief      Get the last character in a string
    \param  cs  source string
    \return     last character in <i>cs</i>

    Throws exception if <i>cs</i> is empty
*/
char last_char(const string& cs)
{ if (cs.empty())
    throw string_function_error(STRING_BOUNDS_ERROR, "Attempt to access character in empty string"s);
    
  return cs[cs.length() - 1];
}

/*! \brief      Get the penultimate character in a string
    \param  cs  source string
    \return     penultimate character in <i>cs</i>

    Throws exception if <i>cs</i> is empty or contains only one character
*/
char penultimate_char(const string& cs)
{ if (cs.length() < 2)
    throw string_function_error(STRING_BOUNDS_ERROR, "Attempt to access character beyond end of string"s);
    
  return cs[cs.length() - 2];
}

/*! \brief      Get the antepenultimate character in a string
    \param  cs  source string
    \return     antepenultimate character in <i>cs</i>

    Throws exception if <i>cs</i> contains fewer than two characters
*/
char antepenultimate_char(const string& cs)
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

  return ( cp ? string(cp) : string() );
}

/*! \brief      Transform a string
    \param  cs  original string
    \param  pf  pointer to transformation function
    \return     <i>cs</i> with the transformation <i>*pf</i> applied
*/
string transform_string(const string& cs, int(*pf)(int))
{ string rv { cs };
  
  transform(rv.begin(), rv.end(), rv.begin(), pf);
  
  return rv;
}

/*! \brief      Get location of start all words
    \param  s   string to be analysed
    \return     positions of all the starts of words in <i>s</i>
*/
vector<size_t> starts_of_words(const string& s)
{ vector<size_t> rv;

  if (s.empty())
    return rv;

// start of first word
  size_t posn { s.find_first_not_of(SPACE_STR, 0) };

  if (posn == string::npos)
    return rv;

  rv.push_back(posn);

// next space
  while (1)
  { posn = s.find_first_of(SPACE_STR, posn);

    if (posn == string::npos)
      return rv;

    posn = s.find_first_not_of(SPACE_STR, posn);

    if (posn == string::npos)
      return rv;

    rv.push_back(posn);
  }
}

/*! \brief                  Get location of start of next word
    \param  str             string which the next word is to be found
    \param  current_posn    position from which to search
    \return                 position of start of next word, beginning at position <i>current_posn</i> in <i>str</i>

    Returns <i>string::npos</i> if no word can be found
*/
size_t next_word_posn(const string& str, const size_t current_posn)
{ if (str.length() <= current_posn)
    return string::npos;

  const bool is_space { (str[current_posn] == ' ') };

  if (is_space)
    return ( str.find_first_not_of(SPACE_STR, current_posn) );

// we are inside a word
  const size_t space_posn { str.find_first_of(SPACE_STR, current_posn) };
  const size_t word_posn  { str.find_first_not_of(SPACE_STR, space_posn) };

  return word_posn;
}

/*! \brief          Get nth word in a string
    \param  s       string to be analysed
    \param  n       word number to be returned
    \param  wrt     value with respoct to which <i>n</i> is counted
    \return         the <i>n</i>th word, counting with respect to <i>wrt</i>

    Returns <i>string::npos</i> if there is no <i>n</i>th word
*/
string nth_word(const string& s, const unsigned int n, const unsigned int wrt)
{ string rv;

  if (n < wrt)
    return rv;

  const unsigned int   actual_word_number { n - wrt };
  const vector<size_t> starts             { starts_of_words(s) };

  if (actual_word_number >= starts.size())
    return rv;

  const size_t posn_1 { starts[actual_word_number] };
  const size_t posn_2 { ( (actual_word_number + 1) >= starts.size() ? string::npos : starts[actual_word_number + 1] ) };

  rv = substring(s, posn_1, posn_2 - posn_1);
  rv = remove_peripheral_spaces(rv);

  return rv;
}

/*! \brief          Get the actual length, in bytes, of a UTF-8-encoded string
    \param  str     UTF-8 string to be analysed
    \return         number of bytes occupied by <i>str</i>

    See: https://stackoverflow.com/questions/4063146/getting-the-actual-length-of-a-utf-8-encoded-stdstring
    TODO: generalise using locales/facets, instead of assuming UTF-8
*/
size_t n_chars(const string& str)
{ if (string(nl_langinfo(CODESET)) != "UTF-8"s)
    throw string_function_error(STRING_UNKNOWN_ENCODING, "Unknown character encoding: "s + string(nl_langinfo(CODESET)));

  const size_t n_bytes { str.size() };

  char*  cp     { const_cast<char*>(str.data()) };
  char*  end_cp { cp + n_bytes };  // one past the end of the contents of str
  size_t rv     { 0 };

  while (cp < end_cp)
  { if ( (*cp++ & 0xc0) != 0x80 )
      rv++;
  }

  return rv;
}

/*! \brief      Does a string contain a legal dotted-decimal IPv4 address
    \param  cs  original string
    \return     whether <i>cs</i> contains a legal dotted decimal IPv4 address
*/
bool is_legal_ipv4_address(const string& cs)
{ static const string separator { "."s };

  const vector<string> fields { split_string(cs, separator) };

  if (fields.size() != 4)
    return false;

  for (const auto& field : fields)
  { try
    { const int value { from_string<int>(field) };

      if ((value < 0) or (value > 255))
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
{ static const string separator { "."s };

// put into network order (so that we can guarantee the order of the octets in the long)
  const uint32_t network_val { htonl(val) };

  unsigned char* cp { (unsigned char*)(&network_val) };

  string rv;

  for (int n = 0; n < 3; n++)
  { const unsigned char c { cp[n] };
  
    rv += to_string((int)c) + separator;
  }

  const unsigned char c { cp[3] };

  rv += to_string((int)c);

  return rv;
}

/*! \brief                  Is a string a legal value from a list?
    \param  value           target string
    \param  legal_values    all the legal values, separated by <i>separator</i>
    \param  separator       separator in the string <i>legal_values</i>
    \return                 whether <i>value</i> appears in <i>legal_values</i>
*/
bool is_legal_value(const string& value, const string& legal_values, const string& separator)
{ const vector<string> vec { split_string(legal_values, separator) };

  return (find(vec.begin(), vec.end(), value) != vec.end());
}

/*! \brief          Is one call earlier than another, according to callsign sort order?
    \param  call1   first call
    \param  call2   second call
    \return         whether <i>call1</i> appears before <i>call2</i> in callsign sort order
*/
bool compare_calls(const string& call1, const string& call2)
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

/*! \brief          Does a string contain any letters?
    \param  str     string to test
    \return         whether <i>str</i> contains any letters

    This should be faster than the find_next_of() or C++ is_letter or similar generic functions
*/
bool contains_letter(const string& str)
{ for (const char& c : str)
    if ( (c >= 'A' and c <= 'Z') or (c >= 'a' and c <= 'z') )
      return true;

  return false;
}

/*! \brief          Does a string contain any digits?
    \param  str     string to test
    \return         whether <i>str</i> contains any digits

    This should be faster than the find_next_of() or C++ is_digit or similar generic functions
*/
bool contains_digit(const string& str)
{ for (const char& c : str)
    if (c >= '0' and c <= '9')
      return true;

  return false;
}

/*! \brief          Return a number with a particular number of decimal places
    \param  str     initial value
    \param  n       number of decimal places
    \return         <i>str</i> with <i>n</i> decimal places

    Assumes that <i>str</i> is a number
*/
string decimal_places(const string& str, const int n)
{
// for now, assume that it's a number
  if ( (str.length() >= 2) and (str[str.length() - 2] != '.') )
  { const float fl { from_string<float>(str) };

    ostringstream stream;

    stream << fixed << setprecision(n) << fl;
    return stream.str();
  }

  return str;
}

/*! \brief          Return the longest line from a vector of lines
    \param  lines   the lines to search
    \return         the longest line in the vector <i>lines</i>
*/
string longest_line(const vector<string>& lines)
{ string rv;

  for (const string& line : lines)
    if (line.length() > rv.length())
      rv = line;

  return rv;
}

/*! \brief          Deal with wprintw's idiotic insertion of newlines when reaching the right hand of a window
    \param  str     string to be reformatted
    \param  width   width of line in destination window
    \return         <i>str</i> reformatted for the window

    See http://stackoverflow.com/questions/7540029/wprintw-in-ncurses-when-writing-a-newline-terminated-line-of-exactly-the-same
*/
string reformat_for_wprintw(const string& str, const int width)
{ string rv;

  int since_last_newline { 0 };

  for (size_t posn = 0; posn < str.length(); ++posn)
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

  for (const auto& s : vecstr)
    rv.push_back(reformat_for_wprintw(s, width));

  return rv;
}

/*! \brief          Write a <i>vector<string></i> object to an output stream
    \param  ost     output stream
    \param  vec     object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const vector<string>& vec)
{ unsigned int idx { 0 };

  for (const auto str : vec)
  { ost << "[" << idx++ << "]: " << str;

    if (idx != vec.size())
      ost << endl;
  }

  return ost;
}

/*! \brief                  Remove a trailing inline comment
    \param  str             string
    \param  comment_str     string that introduces the comment
    \return                 <i>str</i> with the trailing comment and any additional trailing spaces removed

    Generally it is expected that <i>str</i> is a single line (without the EOL marker)
*/
string remove_trailing_comment(const string& str, const string& comment_str)
{ const size_t posn { str.find(comment_str) };

  return ( (posn == string::npos) ? str : remove_trailing_spaces(substring(str, 0, posn)) );
}

/*! \brief              Perform a case-insensitive search for a substring
    \param  str         string to search
    \param  target      substring for which to search
    \param  start_posn  location in <i>str</i> at which to start the search
    \return             position of the first character in <i>target</i> in <i>str</i>
    
    Returns string::npos if <i>target</i> cannot be found
*/
size_t case_insensitive_find(const std::string& str, const std::string& target, const size_t start_posn)
{ auto it { str.cbegin() };

  if (start_posn != 0)
    advance(it, start_posn);
  
  const auto posn { search(it, str.cend (), target.cbegin(), target.cend(), [](const char ch1, const char ch2) { return toupper(ch1) == toupper(ch2); } ) };

  return ( (posn == str.cend()) ? string::npos : distance(it, posn) + start_posn);
}
