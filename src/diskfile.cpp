// $Id: diskfile.cpp 94 2015-02-07 15:06:10Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file diskfile.cpp

    Useful file-related functions. This file is derived from proprietary code
    owned by IPfonix, Inc.
*/

#include "diskfile.h"

#include <array>
#include <exception>

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

/*! \brief              What is the size of a file?
    \param  filename    name of file
    \return             length of the file <i>filename</i> in bytes

    Returns 0 if the file does not exist or is not readable.
*/
const unsigned long file_size(const string& filename)
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
  { ofstream(destination_filename) << ifstream(source_filename).rdbuf();          // perform the copy
  }
}

/*! \brief                          Rename a file
    \param  source_filename         original name of file
    \param  destination_filename    final name of file

    Does nothing if the source file does not exist
*/
void file_rename(const string& source_filename, const string& destination_filename)
{ if (file_exists(source_filename))
  { const int status = rename(source_filename.c_str(), destination_filename.c_str());

    if (status)
      throw exception();
  }
}

/*! \brief              Create a directory
    \param  dirname     name of the directory to create
*/
void directory_create(const string& dirname)
{ const int status = mkdir(dirname.c_str(), 0xff);

  if (status)
    throw exception();
}

// does a directory exist?
const bool directory_exists(const string& filename)
{ struct stat stat_buffer;
  int status = stat(filename.c_str(), &stat_buffer);

  if (status)
    return false;

  const bool rv = ((stat_buffer.st_mode & S_IFDIR) != 0);
    return rv;
}

// list all the files in a directory
const vector<string> directory_contents(const string& dirname)
{ vector<string> rv;

  if (!directory_exists(dirname))
    return rv;

  const string dirname_slash = dirname + "/";
  struct dirent** namelist;
  const int status = scandir((dirname_slash).c_str(), &namelist, 0, alphasort);

  if (status == -1)
    return rv;                                  // shouldn't happen

  for (int n = 0; n < status; n++)
  { const string name = namelist[n]->d_name;

    if ((name != ".") and (name != ".."))
      rv.push_back(name);
  }

  return rv;
}

/*! \brief              Truncate a file
    \param  filename    name of file to truncate

    Creates <i>filename</i> if it does not exist
*/
//void file_truncate(const string& filename)
//{ //FILE* fp = fopen(filename.c_str(), "w");
//
//  //fclose(fp);
//  ofstream(filename, ios_base::trunc);
//}
