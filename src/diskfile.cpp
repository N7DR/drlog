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

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <iostream>

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

using namespace std;

namespace fs = std::filesystem;

/*! \brief              What is the size of a file?
    \param  filename    name of file
    \return             length of the file <i>filename</i> in bytes

    Returns 0 if the file does not exist or is not readable.
*/
//unsigned long file_size(const string& filename)
unsigned long file_size(const string_view filename)
{ ifstream in(string { filename }, ifstream::in bitor ifstream::binary);

if (in)
{ in.seekg(0, ifstream::end);
  return in.tellg();
}

return 0;
}

/*! \brief              Delete a file
    \param  filename    name of file
*/
//void file_delete(const string& filename)
void file_delete(const string_view filename)
{ const string fn { filename };

if (file_exists(fn))
  unlink(fn.c_str());
}

/*! \brief                          Copy a file
 *    \param  source_filename         name of the source file
 *    \param  destination_filename    name of the destination file
 *
 *    Does nothing if the source file does not exist
 */
void file_copy(const string& source_filename, const string& destination_filename)
{ if (file_exists(source_filename))
  ofstream(destination_filename) << ifstream(source_filename).rdbuf();  // perform the copy
}

/*! \brief                          Rename a file
 *    \param  source_filename         original name of file
 *    \param  destination_filename    final name of file
 *
 *    Does nothing if the source file does not exist
 */
void file_rename(const string& source_filename, const string& destination_filename)
{ if (file_exists(source_filename))
  { if (const int status { rename(source_filename.c_str(), destination_filename.c_str()) }; status)
  throw exception();
  }
}

/*! \brief              Create a directory
 *    \param  dirname     name of the directory to create
 *
 *    Throws exception if the directory already exists
 */
void directory_create(const string& dirname)
{ if (const int status { mkdir(dirname.c_str(), 0x1ed) }; status)  // rwxrxrx
throw exception();
}

/*! \brief              Does a (regular) file exist?
 *    \param  filename    name of file
 *    \return             whether file <i>filename</i> exists and is a regular file
 */
//bool file_exists(const string& filename)
bool file_exists(const string_view filename)
{ struct stat stat_buffer;

  const string str { filename };

  int status { lstat(str.c_str(), &stat_buffer) };

  if (status)
    return false;

  status = stat(str.c_str(), &stat_buffer);

  return S_ISREG(stat_buffer.st_mode);
}

/*! \brief              Does a directory exist?
 *    \param  dirname     name of the directory to test for existence
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             whether <i>dirname</i> exists
 */
bool directory_exists(const string_view filename, const enum LINKS link, const FSID fsid)
{ const string fn { filename };

  struct stat stat_buffer;

  int status { lstat(fn.c_str(), &stat_buffer) };

  if (status)
    return false;

  // if LINKS::NO_INCLUDE, return false if it's a link
  if (link == LINKS::NO_INCLUDE)
  { if (is_link(fn))
    return false;
  }

  // if it's a link, follow it
  bool rv { false };

  if ( (fsid == MAX_FSID) or (filesystem_id(fn) == fsid) )
  { status = stat(fn.c_str(), &stat_buffer);

    rv = S_ISDIR(stat_buffer.st_mode);
  }

  return rv;
}


/*! \brief            Create a directory if it does not already exist
    \param  dirname   name of the directory to create
    \return           Whether directory was created
*/

/*! \brief            Create a directory if it does not already exist
 *  \param  dirname   name of the directory to create
 *  \return           Whether directory was created
 */

bool directory_create_if_necessary(const string_view dirname)
{ if (!directory_exists(dirname, LINKS::INCLUDE))
  { fs::create_directories(dirname);
    return true;;
  }

  return false;
}

/*! \brief              What files (including directories) does a directory contain?
 *    \param  dirname     name of the directory to examine
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of filenames, in alphabetical order
 *
 *    The returned vector does not include "." or "..".
 *    Returns empty vector if the directory <i>dirname</i> does not exist
 */
vector<string> directory_contents(const string& dirname, const FSID fsid)
{ vector<string> rv { };

  if (!directory_exists(dirname, LINKS::INCLUDE, fsid))
    return rv;

  const string dirname_slash { dirname + "/"s };

  struct dirent** namelist;

  const int status { scandir((dirname_slash).c_str(), &namelist, 0, alphasort) };

  if (status == -1)
    return rv;                                  // shouldn't happen

  for (int n { 0 }; n < status; n++)
  { const string name { namelist[n]->d_name };

    if ((name != "."s) and (name != ".."s))
    { if ( (fsid == MAX_FSID) or (filesystem_id(dirname_slash + name) == fsid) )
      rv += move(name);
    }
  }

  SORT(rv);         // place in alphabetical order

  return rv;
}

/*! \brief              What files does a directory contain?
 *    \param  dirname     name of the directory to examine
 *    \param  links       whether to include links in the output
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of files, in alphabetical order
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist
 *    Returns only regular files
 */
vector<string> files_in_directory(const string& dirname, const enum LINKS links, const FSID fsid)
{ vector<string> rv { };

  if (!directory_exists(dirname, LINKS::INCLUDE, fsid))   // linked directory is allowed for base
    return rv;

  const string dirname_slash { dirname + "/"s };

  struct dirent** namelist;

  const int status { scandir((dirname_slash).c_str(), &namelist, 0, alphasort) };

  if (status == -1)
    return rv;                                  // shouldn't happen

  for (int n { 0 }; n < status; n++)
  { const string name      { namelist[n]->d_name };
    const string full_name { dirname_slash + name };

    struct stat stat_buffer;

    if (const int status { lstat(full_name.c_str(), &stat_buffer) }; status == 0)
    { switch (links)
      { case LINKS::INCLUDE :
          if (is_link(full_name))
          stat(full_name.c_str(), &stat_buffer);          // repopulate the stat buffer

          if ((stat_buffer.st_mode & S_IFMT) == S_IFREG)
            if ( (fsid == MAX_FSID) or (filesystem_id(full_name) == fsid) )
              rv += move(full_name);
          break;

        case LINKS::NO_INCLUDE :
          if (!is_link(full_name))
            if ((stat_buffer.st_mode & S_IFMT) == S_IFREG)
              if ( (fsid == MAX_FSID) or (filesystem_id(full_name) == fsid) )
                rv += move(full_name);
          break;
      }
    }
  }

  SORT(rv);         // place in alphabetical order

  return rv;
}

/*! \brief              What files does a vector of directories contain?
 *    \param  dirnames    names of the directories to examine
 *    \param  links       whether to include links in the output
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of files contained in all the directories in <i>dirnames</i>
 */
vector<string> files_in_directories(const vector<string>& dirnames, const enum LINKS links, const FSID fsid)
{ vector<string> rv;

  //  for (const string& dirname : dirnames)
  //    rv += files_in_directory(dirname, links, fsid);
  FOR_ALL(dirnames, [fsid, links, &rv] (const string& dirname) { rv += files_in_directory(dirname, links, fsid); });

  return rv;
}

/*! \brief              What directories does a directory contain?
 *    \param  dirname     name of the directory to examine
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of directories
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist
 */
vector<string> directories_in_directory(const string& dirname, const enum LINKS links, const FSID fsid)
{ vector<string> rv;

  if (!directory_exists(dirname, LINKS::INCLUDE, fsid))   // linked directory is allowed for base
    return rv;

  const string dirname_slash { dirname + "/"s };

  struct dirent** namelist;

  const int status { scandir((dirname_slash).c_str(), &namelist, 0, alphasort) };

  if (status == -1)
    return rv;                                  // shouldn't happen

  for (int n { 0 }; n < status; n++)
  { const string tail_name { namelist[n]->d_name };

    if (tail_name != "."s and tail_name != ".."s)
    { const string name { dirname_slash + namelist[n]->d_name };

      if ( (fsid == MAX_FSID) or (filesystem_id(name) == fsid) )
        if (directory_exists(name, links, fsid))
          rv += move(name);
    }
  }

  return rv;
}

/*! \brief              What directories does a directory contain, recursively?
 *    \param  dirname     name of the directory to examine
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of directories
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist.
 *    Returned vector is unsorted.
 *    Returned vector includes <i>dirname</i>
 */
vector<string> directories_in_hierarchy(const string& dirname, const enum LINKS links, const FSID fsid)
{ vector<string> rv;

  if (directory_exists(dirname, links, fsid))
  { rv += dirname;
    rv += subdirectories_in_hierarchy(dirname, links, fsid);
  }

  return rv;
}

/*! \brief              What directories does a directory contain, recursively?
 *    \param  dirname     name of the directory to examine
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of directories
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist.
 *    Returned vector is unsorted.
 *    Returned vector does not include <i>dirname</i>
 */
vector<string> subdirectories_in_hierarchy(const string& dirname, const enum LINKS links, const FSID fsid)
{ vector<string> rv;

  if (directory_exists(dirname, links, fsid))
  { rv += directories_in_directory(dirname, links, fsid);

    size_t index { 0 };

    while (index != rv.size())
      rv += directories_in_directory(rv[index++], links, fsid);
  }

  return rv;
}

/*! \brief              What files does a directory contain, recursively?
 *    \param  rootname    name of the directory to examine
 *    \param  links       whether to include links in the output
 *    \return             vector of files
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist
 *    Returned vector is unsorted.
 */
vector<string> files_in_hierarchy(const string& rootname, const enum LINKS links, const FSID fsid)
{
  // build a container of all the directory names (except . and ..) in the tree
  const vector<string> vec { directories_in_hierarchy(rootname, links, fsid) };       // includes root

  // now create a container of all the file names
  vector<string> rv;

  for (const auto& dirname : vec)
    rv += files_in_directory(dirname, links, fsid);

  return rv;
}

/*! \brief              mtime associated with a file or directory
 *    \param  filename    name of file or directory
 */
time_t atime(const string& filename)
{ struct stat stat_buffer;

  if (const int status { stat((filename).c_str(), &stat_buffer) }; status)
    throw diskfile_exception("Error finding atime for: "s + filename);

  return static_cast<time_t>(stat_buffer.st_atime);
}

/*! \brief              ctime associated with a file or directory
 *    \param  filename    name of file or directory
 */
time_t ctime(const string& filename)
{ struct stat stat_buffer;

  if (const int status { stat((filename).c_str(), &stat_buffer) }; status)
    throw diskfile_exception("Error finding ctime for: "s + filename);

  return static_cast<time_t>(stat_buffer.st_ctime);
}

/*! \brief              mtime associated with a file or directory
 *    \param  filename    name of file or directory
 */
time_t mtime(const string& filename)
{ struct stat stat_buffer;

  if (const int status { stat((filename).c_str(), &stat_buffer) }; status)
    throw diskfile_exception("Error finding mtime for: "s + filename);

  return static_cast<time_t>(stat_buffer.st_mtime);
}

/*! \brief              base name of a file
 *    \param  filename    name of file or directory
 *    \return             the base name of the file
 *
 *    Returns empty string if <i>filename</i> ends in a "/"
 */
string base_name(const string& filename)
{ string rv;

  if (filename.empty())
    return rv;

  // if last character is a "/" then it's a directory, so return empty string
  if (filename[filename.length() - 1] == '/')
    return rv;

  const size_t posn { filename.find_last_of("/"s) };

  if (posn == string::npos)     // no "/"
    return filename;

  return filename.substr(posn + 1);
}

/*! \brief          is a file a link?
 *    \param  name    name of file
 *    \return         whether <i>name</i> is a link
 */
bool is_link(const string& name)
{ struct stat stat_buffer;

  if (const int status { lstat(name.c_str(), &stat_buffer) }; status)
  { cerr << "stat status is non-zero for: " << name << endl;
    return false;
  }

  const mode_t& entry_mode { stat_buffer.st_mode };
  const bool    rv         { S_ISLNK(entry_mode) };

  return rv;
}

/*! \brief          write the status of a file to certr
 *    \param  name    name of file
 */
void file_status(const string& name)
{ struct stat stat_buffer;

  if (const int status { lstat(name.c_str(), &stat_buffer) }; status)
    cerr << "ERROR running lstat on " << name << endl;
    else
    { const mode_t& entry_mode { stat_buffer.st_mode };

  cerr << "ENTRY: " << name << endl
  << "  " << (S_ISREG(entry_mode) ? "regular file" : "NOT regular file") << endl
  << "  " << (S_ISDIR(entry_mode) ? "directory file" : "NOT directory file") << endl
  << "  " << (S_ISCHR(entry_mode) ? "character device" : "NOT character device") << endl
  << "  " << (S_ISBLK(entry_mode) ? "block device" : "NOT block device") << endl
  << "  " << (S_ISFIFO(entry_mode) ? "FIFO" : "NOT FIFO") << endl
  << "  " << (S_ISLNK(entry_mode) ? "symbolic link" : "NOT symbolic link") << endl
  << "  " << (S_ISSOCK(entry_mode) ? "socket" : "NOT socket") << endl;
    }
}

/*! \brief              directory in which a file is located
 *    \param  file_name   full or relative name of file
 *    \return             name of directory in which <i>file_name</i> is located
 *
 *    Returns <i>file_name</i> if <i>file_name</i> is a directory
 *    Returns "." if <i>file_name</i> does not contain a directory name
 */
//string directory_name(const string& file_name)
string directory_name(const string_view file_name)
{ if (directory_exists(file_name, LINKS::INCLUDE))
  return string { file_name };

  const size_t posn { file_name.find_last_of("/"s) };

  if (posn == string::npos)
    return "."s;

  return substring <std::string> (file_name, 0, posn);
}

/*! \brief              Does a directory contain one or more subdirectories?
 *    \param  dirname     name of directory
 *    \param  links       whether to include links in the output
 *    \return             whether <i>dirname</i> has any subdirectories
 *
 *    Returns <i>false</i> if <i>dirname</i> is not a directory
 */
bool has_subdirectory(const std::string& dirname, const enum LINKS links)
{ if (!is_directory(dirname, LINKS::INCLUDE))   // linked directory is allowed for base
  return false;

  const string dirname_slash { dirname + "/"s };

  struct dirent** namelist;

  const int status { scandir((dirname_slash).c_str(), &namelist, 0, alphasort) };

  if (status == -1)
    return false;                                  // shouldn't happen

  for (int n { 0 }; n < status; n++)
  { const string tail_name { namelist[n]->d_name };

    if (tail_name != "."s and tail_name != ".."s)
    { const string name { dirname_slash + namelist[n]->d_name };

      if (directory_exists(name, links))
        return true;
    }
  }

  return false;
}

/*! \brief              Is a directory a direct ancestor of a particular directory?
 *    \param  dir1        first directory
 *    \param  dir2        second directory
 *    \return             whether <i>dir2</i> is a direct ancestor of <i>dir1</i>
 */
bool is_ancestor_directory_of_directory(const string& dir1, const string& dir2)
{ if (!is_directory(dir1, LINKS::NO_INCLUDE) or !is_directory(dir2, LINKS::NO_INCLUDE))
  return false;

//  return ( (dir2.size() < dir1.size()) and starts_with(dir1, dir2) );
  return ( (dir2.size() < dir1.size()) and dir1.starts_with(dir2) );
}

/*! \brief              Get the filesystem ID of a file or directory
 *    \param  filename    name of file or directory
 *    \return             the file system ID of <i>filename</i>
 *
 *    https://stackoverflow.com/questions/59687286/how-to-check-if-a-directory-is-on-a-local-disk-or-a-remote-disk-in-c-or-fortran
 */
FSID filesystem_id(const string& filename)
{ if (!file_exists(filename) and !directory_exists(filename, LINKS::NO_INCLUDE))
  return MAX_FSID;

  struct statvfs fs_buf;

  if (const int status { statvfs(filename.c_str(), &fs_buf) }; status == -1)
    return MAX_FSID;

  return fs_buf.f_fsid;
}

/*! \brief              Find the location of a file in a path
 *   \param  path        directories in which to look (with or without trailing "/"), in order
 *   \param  filename    name of file
 *   \return             full filename if a file is found, otherwise the empty string
 *
 *   Actually checks for existence AND readability, which is much simpler
 *   than checking for existence. See:
 *     https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
 */
string find_file(const vector<string>& path, const string_view filename)
{ for (const auto& dir : path)
  { const string sep { dir.ends_with('/') ? ""s : "/"s };     // put a "/" at the end, if necessary

    if ( const auto fullname { dir + sep + filename }; file_exists(fullname) )
      return fullname;
  }

  return string { };
}

#if 0

OLD FILE

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

#endif    // 0  OLD FILE
