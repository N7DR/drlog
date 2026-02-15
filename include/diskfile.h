// $Id: diskfile.h 255 2024-11-10 20:30:33Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   diskfile.h

    Useful file-related functions.
*/

#ifndef DISKFILE_H
#define DISKFILE_H

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

enum class LINKS { INCLUDE,
  NO_INCLUDE
};

using FSID = unsigned long;         // type of filesystem ID

constexpr auto MAX_FSID { std::numeric_limits<FSID>::max() };    // maximum value of FSID, used to indicate abnormal filesystem or an error

class diskfile_exception : public std::exception
{
protected:

  std::string _what;    ///< the value to be passed upwards

public:

/*! \brief  constructor
    \param  the string to be passed upwards when the exception is thrown
*/
  //  diskfile_exception(const std::string& str) :
  diskfile_exception(const std::string_view str) :
  _what (str)
  { }

  /// return the string to be passed upwards
  inline const char* what(void) const throw ()
  { return _what.c_str(); }
};

/*! \brief              Append a string to a file
    \param  filename    name of file
    \param  str         string to be appended

    Creates <i>filename</i> if it does not exist
*/
inline void append_to_file(const std::string_view filename, const std::string& str)
{ std::ofstream(std::string { filename }, std::ios_base::app) << str; }

/*! \brief              Does a (regular) file exist?
 *    \param  filename    name of file
 *    \return             whether file <i>filename</i> exists and is a regular file
 */
//bool file_exists(const std::string& filename);
bool file_exists(const std::string_view filename);

/*! \brief              What is the size of a file?
 *    \param  filename    name of file
 *    \return             length of the file <i>filename</i> in bytes
 *
 *    Returns <i>false</i> if the file does not exist or it exists and is not readable.
 */
unsigned long file_size(const std::string_view filename);

/*! \brief              Is a file empty?
 *    \param  filename    name of file
 *    \return             whether the file <i>filename</i> is empty
 *
 *    Returns <i>true</i> if the file does not exist or it exists and is not readable.
 */
//inline bool file_empty(const std::string& filename)
inline bool file_empty(const std::string_view filename)
{ return (file_size(filename) == 0); }

/*! \brief              Delete a file
 *    \param  filename    name of file
 */
//void file_delete(const std::string& filename);
void file_delete(const std::string_view filename);

/*! \brief                          Copy a file
 *    \param  source_filename         name of the source file
 *    \param  destination_filename    name of the destination file
 *
 *    Does nothing if the source does not exist
 */
void file_copy(const std::string& source_filename, const std::string& destination_filename);

/*! \brief                          Rename a file
 *    \param  source_filename         original name of file
 *    \param  destination_filename    final name of file
 *
 *    Does nothing if the source file does not exist
 */
void file_rename(const std::string& source_filename, const std::string& destination_filename);

/*! \brief                          Move a file
 *    \param  source_filename         original filename
 *    \param  destination_filename    final filename
 *
 *    Does nothing if the source file does not exist
 */
inline void file_move(const std::string& source_filename, const std::string& destination_filename)
{ file_rename(source_filename, destination_filename); }

/*! \brief              directory in which a file is located
 *    \param  file_name   full or relative name of file
 *    \return             name of directory in which <i>file_name</i> is located
 *
 *    Returns <i>file_name</i> if <i>file_name</i> is a directory
 *    Returns "." if <i>file_name</i> does not contain a directory name
 */
//std::string directory_name(const std::string& file_name);
std::string directory_name(const std::string_view file_name);

/*! \brief              Create a directory
 *    \param  dirname     name of the directory to create
 *
 *    Throws exception if the directory already exists
 */
void directory_create(const std::string& dirname);

/*! \brief              Does a directory exist?
 *    \param  dirname     name of the directory to test for existence
 *    \param  link        whether to allow <i>dirname</i> to be a link
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             whether <i>dirname</i> exists
 */
//bool directory_exists(const std::string& dirname, const enum LINKS link, const FSID fsid = MAX_FSID);
bool directory_exists(const std::string_view dirname, const enum LINKS link, const FSID fsid = MAX_FSID);

/*! \brief              Does a directory exist?
 *    \param  dirname     name of the directory to test for existence
 *    \param  link        whether to allow <i>dirname</i> to be a link
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             whether <i>dirname</i> exists
 */
inline bool is_directory(const std::string& dirname, const enum LINKS link, const FSID fsid = MAX_FSID)
//  { return directory_exists(dirname, link); }
{ return directory_exists(dirname, link, fsid); }

/*! \brief            Create a directory if it does not already exist
    \param  dirname   name of the directory to create
    \return           Whether directory was created
*/
//void directory_create_if_necessary(const std::string& dirname);
bool directory_create_if_necessary(const std::string_view dirname);

/*! \brief              Create a directory for a particular filename, if necessary
 *    \param  filename    full filename for the file
 *
 *    Directory is created only if it does not already exist
 */
inline void directory_create_if_necessary_for_file(const std::string_view filename)
{ directory_create_if_necessary(directory_name(filename)); }

/*! \brief              What files (including directories) does a directory contain?
 *    \param  dirname     name of the directory to examine
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of filenames, in alphabetical order
 *
 *    The returned vector does not include "." or "..".
 *    Returns empty vector if the directory <i>dirname</i> does not exist
 */
std::vector<std::string> directory_contents(const std::string& dirname, const FSID fsid = MAX_FSID);

/*! \brief              What directories does a directory contain?
 *    \param  dirname     name of the directory to examine
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of directories
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist
 */
std::vector<std::string> directories_in_directory(const std::string& dirname, const enum LINKS links, const FSID fsid = MAX_FSID);

/*! \brief              What directories does a directory contain, recursively?
 *    \param  dirname     name of the directory to examine
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of directories
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist.
 *    Returned vector is unsorted.
 *    Returned vector includes <i>dirname</i>
 *    Searches across filesystems
 */
std::vector<std::string> directories_in_hierarchy(const std::string& rootname, const enum LINKS links, const FSID fsid = MAX_FSID);

/*! \brief              What directories does a directory contain, recursively?
 *    \param  dirname     name of the directory to examine
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of directories
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist.
 *    Returned vector is unsorted.
 *    Returned vector does not include <i>dirname</i>
 */
std::vector<std::string> subdirectories_in_hierarchy(const std::string& dirname, const enum LINKS links, const FSID fsid = MAX_FSID);

/*! \brief              What files does a directory contain?
 *    \param  dirname     name of the directory to examine
 *    \param  links       whether to include links in the output
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of files
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist
 *    Returns only regular files, in alphabetical order
 */
std::vector<std::string> files_in_directory(const std::string& dirname, const enum LINKS links, const FSID fsid = MAX_FSID);

/*! \brief              What files does a vector of directories contain?
 *    \param  dirnames    names of the directories to examine
 *    \param  links       whether to include links in the output
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of files contained in all the directories in <i>dirnames</i>
 */
std::vector<std::string> files_in_directories(const std::vector<std::string>& dirnames, const enum LINKS links, const FSID fsid = MAX_FSID);

/*! \brief              What files does a directory contain, recursively?
 *    \param  rootname    name of the directory to examine
 *    \param  links       whether to include links in the output
 *    \param  fsid        limit filesystems to this fsid, if not MAX_FSID
 *    \return             vector of files
 *
 *    Returns empty vector if the directory <i>dirname</i> does not exist
 *    Returned vector is unsorted.
 */
std::vector<std::string> files_in_hierarchy(const std::string& rootname, const enum LINKS links, const FSID fsid = MAX_FSID);

/*! \brief              Truncate a file
 *    \param  filename    name of file to truncate
 *
 *    Creates <i>filename</i> if it does not exist
 */
inline void file_truncate(const std::string& filename)
{ std::ofstream(filename, std::ios_base::trunc); }

/*! \brief              atime associated with a file or directory
 *    \param  filename    name of file or directory
 */
time_t atime(const std::string& filename);

/*! \brief              ctime associated with a file or directory
 *    \param  filename    name of file or directory
 */
time_t ctime(const std::string& filename);

/*! \brief              mtime associated with a file or directory
 *    \param  filename    name of file or directory
 */
time_t mtime(const std::string& filename);

/*! \brief              base name of a file
 *    \param  filename    name of file or directory
 *    \return             the base name of the file
 *
 *    Returns empty string if <i>filename</i> ends in a "/"
 */
std::string base_name(const std::string& filename);

/*! \brief              base name of a file
 *    \param  filename    name of file or directory
 *    \return             the base name of the file
 *
 *    Returns empty string if <i>filename</i> ends in a "/"
 */
inline std::string basename(const std::string& filename)
{ return base_name(filename); }

/*! \brief      Base name of files in a container
 *    \param  t   container of filenames
 *    \return     base names of the files in <i>t</i>
 */
template <typename T>
T base_name(const T& t)
{ typename std::remove_const<T>::type rv;

  FOR_ALL(t, [&rv](const std::string& s) { rv += base_name(s); } );

  return rv;
}

/*! \brief          is a file a link?
 *    \param  name    name of file
 *    \return         whether <i>name</i> is a link
 */
bool is_link(const std::string& name);

/*! \brief          write the status of a file to certr
 *    \param  name    name of file
 */
void file_status(const std::string& name);

/*! \brief              Does a directory contain one or more subdirectories?
 *    \param  dirname     name of directory
 *    \param  links       whether to include links in the output
 *    \return             whether <i>dirname</i> has any subdirectories
 *
 *    Returns <i>false</i> if <i>dirname</i> is not a directory
 */
bool has_subdirectory(const std::string& dirname, const enum LINKS links);

/*! \brief              Is a directory a direct ancestor of a particular directory?
 *    \param  dir1        first directory
 *    \param  dir2        second directory
 *    \return             whether <i>dir2</i> is a direct ancestor of <i>dir1</i>
 */
bool is_ancestor_directory_of_directory(const std::string& dir1, const std::string& dir2);

/*! \brief              Is a directory a direct descendent of a particular directory?
 *    \param  dir1        first directory
 *    \param  dir2        second directory
 *    \return             whether <i>dir2</i> is a direct descendent of <i>dir1</i>
 */
inline bool is_descendent_directory_of_directory(const std::string& dir1, const std::string& dir2)
{ return is_ancestor_directory_of_directory(dir2, dir1); }

/*! \brief              Get the filesystem ID of a file or directory
 *    \param  filename    name of file or directory
 *    \return             the file system ID of <i>filename</i>
 *
 *    https://stackoverflow.com/questions/59687286/how-to-check-if-a-directory-is-on-a-local-disk-or-a-remote-disk-in-c-or-fortran
 */
FSID filesystem_id(const std::string& filename);

/*! \brief              Find the location of a file in a path
 \ param  path        directories in which to look (with or without trailing "/"), in order*
 \param  filename    name of file
 \return             full filename if a file is found, otherwise the empty string

 Actually checks for existence AND readability, which is much simpler
 than checking for existence. See:
 https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
 */
std::string find_file(const std::vector<std::string>& path, const std::string_view filename);

#endif    // DISKFILE_H





#if 0

OLD FILE

#ifndef DISKFILE_H
#define DISKFILE_H

#include <fstream>
#include <string>
#include <vector>

/*! \brief              Append a string to a file
    \param  filename    name of file
    \param  str         string to be appended

    Creates <i>filename</i> if it does not exist
*/
inline void append_to_file(const std::string& filename, const std::string& str)       // can't use string_view here
  { std::ofstream(filename, std::ofstream::binary | std::ios_base::app) << str; }

/*! \brief              Does a file exist?
    \param  filename    name of file
    \return             whether file <i>filename</i> exists

    Actually checks for existence AND readability, which is much simpler
    than checking for existence. See:
      https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
*/
bool file_exists(const std::string& filename);       // can't use string_view here

/*! \brief              Does a file exist?
    \param  filename    name of file
    \return             whether file <i>filename</i> exists

    Actually checks for existence AND readability, which is much simpler
    than checking for existence. See:
      https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
*/
inline bool file_exists(const std::string_view filename)
  { return file_exists(std::string { filename }); }

/*! \brief              Find the location of a file in a path
    \param  path        directories in which to look (with or without trailing "/"), in order
    \param  filename    name of file
    \return             full filename if a file is found, otherwise the empty string

    Actually checks for existence AND readability, which is much simpler
    than checking for existence. See:
      https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
*/
std::string find_file(const std::vector<std::string>& path, const std::string_view filename);

/*! \brief              What is the size of a file?
    \param  filename    name of file
    \return             length of the file <i>filename</i> in bytes

    Returns <i>false</i> if the file does not exist or it exists and is not readable.
*/
unsigned long file_size(const std::string& filename);

/*! \brief              Is a file empty?
    \param  filename    name of file
    \return             whether the file <i>filename</i> is empty

    Returns <i>true</i> if the file does not exist or it exists and is not readable.
*/
inline bool file_empty(const std::string& filename)
  { return (file_size(filename) == 0); }

/*! \brief              Delete a file
    \param  filename    name of file
*/
void file_delete(const std::string& filename);

/*! \brief                          Copy a file
    \param  source_filename         name of the source file
    \param  destination_filename    name of the destination file

    Does nothing if the source does not exist
*/
void file_copy(const std::string& source_filename, const std::string& destination_filename);

/*! \brief                          Rename a file
    \param  source_filename         original name of file
    \param  destination_filename    final name of file

    Does nothing if the source file does not exist.
    Throws exception if the renaming fails.
*/
void file_rename(const std::string& source_filename, const std::string& destination_filename);

/*! \brief                          Move a file
    \param  source_filename         original filename
    \param  destination_filename    final filename

    Does nothing if the source file does not exist
*/
inline void file_move(const std::string& source_filename, const std::string& destination_filename)
  { file_rename(source_filename, destination_filename); }

/*! \brief              Create a directory
    \param  dirname     name of the directory to create
*/
//void directory_create(const std::string& dirname);
void directory_create(const std::string_view dirname);

/*! \brief              Does a directory exist?
    \param  dirname     name of the directory to test for existence
    \return             whether <i>dirname</i> exists
*/
bool directory_exists(const std::string_view dirname);

/*! \brief              What files does a directory contain?
    \param  dirname     name of the directory to examine
    \return             vector of filenames

    The returned vector does not include "." or "..".
    Returns empty vector if the directory <i>dirname</i> does not exist
    <i>dirname</i> may or may not end in "/"
*/
std::vector<std::string> directory_contents(const std::string_view dirname);

/*! \brief              Truncate a file
    \param  filename    name of file to truncate

    Creates <i>filename</i> if it does not exist
*/
inline void file_truncate(const std::string_view filename)
  { std::ofstream(std::string { filename }, std::ios_base::trunc); }

#endif    // DISKFILE_H

#endif    // 0  OLD FILE
