// $Id: diskfile.cpp 254 2024-10-20 15:53:54Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   diskfile.cpp

    Useful file-related functions.
*/

#include "diskfile.h"
#include "string_functions.h"

#include <array>
#include <exception>

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

/*! \brief              Does a file exist?
    \param  filename    name of file
    \return             whether file <i>filename</i> exists

    Actually checks for existence AND readability, which is much simpler
    than checking for existence. See:
      https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
*/
bool file_exists(const string& filename)
{ struct stat buffer;

  return (stat (filename.c_str(), &buffer) == 0);
}

/*! \brief              Find the location of a file in a path
    \param  path        directories in which to look (with or without trailing "/"), in order
    \param  filename    name of file
    \return             full filename if a file is found, otherwise the empty string

    Actually checks for existence AND readability, which is much simpler
    than checking for existence. See:
      https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
*/
string find_file(const vector<string>& path, const string_view filename)
{ for (const auto& dir : path)
  { const string sep { dir.ends_with('/') ? ""s : "/"s };     // put a "/" at the end, if necessary

    if ( const auto fullname { dir + sep + filename }; file_exists(fullname) )
      return fullname;
  }

  return string { };
}

/*! \brief              What is the size of a file?
    \param  filename    name of file
    \return             length of the file <i>filename</i> in bytes

    Returns 0 if the file does not exist or is not readable.
*/
unsigned long file_size(const string& filename)
{ ifstream in(filename, ifstream::in bitor ifstream::binary);

  if (in)
  { in.seekg(0, ifstream::end);

    return in.tellg();
  }
  else
    return 0;
}

/*! \brief              Delete a file
    \param  filename    name of file
*/
void file_delete(const string& filename)
{ if (file_exists(filename))
    unlink(filename.c_str());
}

/*! \brief                          Copy a file
    \param  source_filename         name of the source file
    \param  destination_filename    name of the destination file

    Does nothing if the source file does not exist
*/
void file_copy(const string& source_filename, const string& destination_filename)
{ if (file_exists(source_filename))
    ofstream(destination_filename) << ifstream(source_filename).rdbuf();          // perform the copy
}

/*! \brief                          Rename a file
    \param  source_filename         original name of file
    \param  destination_filename    final name of file

    Does nothing if the source file does not exist.
    Throws exception if the renaming fails.
*/
void file_rename(const string& source_filename, const string& destination_filename)
{ if (file_exists(source_filename))
    if (const int status { rename(source_filename.c_str(), destination_filename.c_str()) }; status)
      throw exception();
}

/*! \brief              Create a directory
    \param  dirname     name of the directory to create
*/
//void directory_create(const string& dirname)
void directory_create(const string_view dirname)
{ const string name_to_test { dirname.ends_with('/') ? remove_trailing <std::string> (dirname, '/') : dirname };

  if (const int status { mkdir(name_to_test.c_str(), 0xff) }; status)
    throw exception();
}

/*! \brief              Does a directory exist?
    \param  dirname     name of the directory to test for existence
    \return             whether <i>dirname</i> exists
*/
bool directory_exists(const string_view dirname)
{ struct stat stat_buffer;

  const string name_to_test { dirname.ends_with('/') ? remove_trailing <std::string> (dirname, '/') : dirname };

  if (const int status { stat(name_to_test.c_str(), &stat_buffer) }; status)
    return false;

  const bool rv { ((stat_buffer.st_mode & S_IFDIR) != 0) };

  return rv;
}

/*! \brief              What files does a directory contain?
    \param  dirname     name of the directory to examine
    \return             vector of filenames

    The returned vector does not include "." or "..".
    Returns empty vector if the directory <i>dirname</i> does not exist
    <i>dirname</i> may or may not end in "/"
*/
vector<string> directory_contents(const string_view dirname)
{ vector<string> rv { };

  if (!directory_exists(dirname))
    return rv;

  const string dirname_slash { dirname.ends_with('/') ? dirname : dirname + "/"s };   // makse sure we have a trailing slash

  struct dirent** namelist;

  const int status { scandir((dirname_slash).c_str(), &namelist, 0, alphasort) };

  if (status == -1)
    return rv;                                  // shouldn't happen

  for (int n { 0 }; n < status; n++)
  { if (const string name { namelist[n] -> d_name }; ( (name != "."s) and (name != ".."s) ))
      rv += name;
  }

  return rv;
}
