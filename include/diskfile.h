// $Id: diskfile.h 30 2013-07-28 21:50:59Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file diskfile.h

    Useful file-related functions. This file is derived from proprietary code
    owned by IPfonix, Inc.
*/

#ifndef DISKFILE_H
#define DISKFILE_H

#include <fstream>
#include <string>
#include <vector>

/*! \brief  Append a string to a file
    \param  filename    Filename
    \param  str         string to be appended

    reates <i.filename</i> if it does not exist
*/
inline void append_to_file(const std::string& filename, const std::string& str)
{ std::ofstream(filename, std::ios_base::app) << str;
}

/*! \brief  Does a file exist?
    \param  filename    Filename
    \return Whether the file exists

    Actually checks for existence AND readability, which is much simpler
    than checking for existence.
*/
inline const bool file_exists(const std::string& filename)
  { return std::ifstream(filename); }

/*! \brief  What is the size of a file?
    \param  filename    Filename
    \return Length of the file in bytes

    Returns 0 if the file does not exist or is not readable.
*/
const unsigned long file_size(const std::string& filename);

inline const bool file_empty(const std::string& filename)
  { return (file_size(filename) == 0); }

/*! \brief  Delete a file
    \param  filename    Filename
*/
void file_delete(const std::string& filename);

/*! \brief  Copy a file
    \param  source_filename Filename of the source
    \param  destination_filename    Filename of the destination
*/
void file_copy(const std::string& source_filename, const std::string& destination_filename);       // does nothing if the source does not exist

/*! \brief  Rename a file
    \param  source_filename Original filename
    \param  destination_filename    Final filename
*/
void file_rename(const std::string& source_filename, const std::string& destination_filename);

/*! \brief  Move a file
    \param  source_filename Original filename
    \param  destination_filename    Final filename
*/
inline void file_move(const std::string& source_filename, const std::string& destination_filename)
  { file_rename(source_filename, destination_filename); }

/*! \brief  Create a directory
    \param  dirname Name of the directory to create
*/
void directory_create(const std::string& dirname);

/*! \brief  Does a directory exist?
    \param  dirname Name of the directory
    \return Whether the directory exists
*/
const bool directory_exists(const std::string& dirname);

/*! \brief  What files does a directory contain?
    \param  dirname Name of the directory to examine
    \return Vector of filenames

    The returned vector does not include "." or ".."
*/
const std::vector<std::string> directory_contents(const std::string& dirname);

#endif    // DISKFILE_H


