// $Id: diskfile.cpp 56 2014-03-29 19:12:12Z  $

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
#include <fstream>

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

/*! \brief  What is the size of a file?
    \param  filename    Filename
    \return Length of the file in bytes

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

// delete a file
void file_delete(const string& filename)
{ if (file_exists(filename))
    unlink(filename.c_str());
}

// copy a file -- does nothing if the source does not exist; assumes that the source does not change size
// while the copy is being made
void file_copy(const string& source_filename, const string& destination_filename)
{ const unsigned int BUFFER_SIZE = 8192;

  if (file_exists(source_filename))
  {
    ofstream(destination_filename) << ifstream(source_filename).rdbuf();          // perform the copy



#if 0

    FILE* fps = fopen(source_filename.c_str(), "rb");
    FILE* fpd = fopen(destination_filename.c_str(), "wb");
    array<char, BUFFER_SIZE> pc;

// get the length of the source
    fseek(fps, 0, SEEK_END);
    unsigned long filesize = ftell(fps);
    fseek(fps, 0, SEEK_SET);

    while (filesize)
    { unsigned long bytes_to_transfer = ((filesize >= BUFFER_SIZE) ? BUFFER_SIZE : filesize);

      const size_t bytes_read = fread(pc.data(), bytes_to_transfer, 1, fps);

//      if (bytes_read != bytes_to_transfer)
//        throw exception();

      fwrite(pc.data(), bytes_read, 1, fpd);
      filesize -= bytes_read;
    }
    fclose(fps);
    fclose(fpd);
#endif

  }
}

/*! \brief  Rename a file
    \param  source_filename Original filename
    \param  destination_filename    Final filename
*/
void file_rename(const string& source_filename, const string& destination_filename)
{ const int status = rename(source_filename.c_str(), destination_filename.c_str());

  if (status)
    throw exception();
}

// create a directory
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

