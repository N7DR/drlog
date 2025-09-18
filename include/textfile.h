// $Id: textfile.h 274 2025-08-11 20:42:36Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef TEXTFILE_H
#define TEXTFILE_H

/*!     \file textfile.h

        Allow one to iterate line-by-line through a text file
*/

#include <fstream>
#include <iostream>

#include <ranges>
#include <string>

#include <cstddef>

using namespace std::literals::string_literals;

class textfile;
class textfile_iterator;

class textstream;
class textstream_iterator;

// -----------  textfile  ----------------

/*! \class  textfile
    \brief  class that allows iterating through a file, line by line and in a ranges-compatible manner
*/

class textfile : public std::ifstream
{
protected:

public:

/*! \brief              Constructor
    \param  filename    name of the file to be read
*/
  inline explicit textfile(const std::string& filename) :
    std::ifstream(filename)
  { }

/*! \brief              Constructor
    \param  filename    name of the file to be read

    Needed because ifstreams are not constructible from string_views
*/
  inline explicit textfile(const std::string_view filename) :
    std::ifstream(std::string { filename })
  { }

/// a no-op
  inline void nop(void) const    // used in textfile_iterator end(textfile& tf)
    { };

/// begin() needs access to <i>textfile</i> internals
  friend textfile_iterator begin(textfile& tf);
};

// -----------  textfile_iterator  ----------------

/*! \class  textfile_iterator
    \brief  iterator for the <i>textfile</i> class
*/

class textfile_iterator
{
protected:

  std::string  _last_line    { };               ///< last line that was read
  std::string* _strp         { nullptr };       ///< pointer to the last line that was read
  int          _last_line_nr { -1 };            ///< last-read line number (wrt 0)

  textfile*    _streamp      { nullptr };       ///< pointer to the associated <i>textfile</i> object

public:

  using difference_type = std::ptrdiff_t;       // needed for ranges-compatible iterator (I think) // https://www.reedbeta.com/blog/ranges-compatible-containers/
  using value_type      = std::string;          // needed for ranges-compatible iterator (I think)

  textfile_iterator(void) = default;                   ///< default default constructor

/*! \brief      Dereference the iterator (i.e., what was the last line read?)
    \return     the last line read, or the string "end()" if the iterator is at end()
*/
  inline std::string operator*(void) const
    { return ( (_strp == nullptr) ? "end()"s : _last_line ); }  // dereferencing end() is bad, but at least we will be benign about it

/*! \brief      Pre-increment the iterator
    \return     the iterator, pre-incremented
*/
  textfile_iterator& operator++(void);

/// textfile_iterator == textfile_iterator
  inline bool operator==(const textfile_iterator& it) const   // equality with iterators
    { return (_last_line_nr == it._last_line_nr); }

/*! \brief      Post-increment the iterator
    \return     the iterator, post-incremented
*/
  textfile_iterator operator++(int);

/// a no-op
  void nop(void) const
   { };

/// begin() needs access to <i>textfile_iterator</i> internals
  friend textfile_iterator begin(textfile& tf);
};

/*! \brief      Obtain the first iterator associated with a <i>textfile</i>
    \param  tf  textfile
    \return     the begin() iterator associated with <i>tf</i>
*/
textfile_iterator begin(textfile& tf);

/*! \brief      Obtain the end() iterator associated with a <i>textfile</i>
    \param  tf  textfile
    \return     the end() iterator associated with <i>tf/i>
*/
inline textfile_iterator end(textfile& tf)
  { tf.nop();
    return textfile_iterator { };
  }

// -----------  textstream  ----------------

/*! \class  textstream
    \brief  class that allows iterating through an input stream, line by line and in a ranges-compatible manner
*/

class textstream : public std::istream
{
protected:

public:

/*! \brief          Constructor
    \param  ist     input stream to be read
*/
  inline explicit textstream(std::istream& ist)
    { rdbuf(ist.rdbuf()); }

/// a no-op
  void nop(void) const    // used in textstream_iterator end(textstream& tf)
    { };

/// begin() needs access to <i>textstream</i> internals
  friend textstream_iterator begin(textstream& ts);
};

// -----------  textstream_iterator  ----------------

/*! \class  textstream_iterator
    \brief  iterator for the <i>textstream</i> class
*/

class textstream_iterator
{
protected:

  std::string   _last_line    { };
  std::string*  _strp         { nullptr };
  int           _last_line_nr { -1 };     // last-read line number (wrt 0)

  textstream*   _streamp      { nullptr };

public:

  using difference_type = std::ptrdiff_t;     // 2. public difference_type alias
  using value_type      = std::string;

  textstream_iterator(void) = default;                   // 1. default constructor

/// dereference the iterator (i.e., what was the last line read?)
  inline std::string operator*(void) const
    { return ( (_strp == nullptr) ? "end()"s : _last_line ); }  // dereferencing end() is bad, but at least we will be benign about it

/*! \brief      Pre-increment the iterator
    \return     the iterator, pre-incremented
*/
  textstream_iterator& operator++(void);    // pre-incrementable

/// textstream_iterator == textstream_iterator
  inline bool operator==(const textstream_iterator& it) const   // equality with iterators
    { return (_last_line_nr == it._last_line_nr); }

/*! \brief      Post-increment the iterator
    \return     the iterator, post-incremented
*/
  inline textstream_iterator& operator++(int)
    { return ++(*this); }

/// begin() needs access to <i>textstream_iterator</i> internals
  friend textstream_iterator begin(textstream& is);
};

/*! \brief      Obtain the first iterator associated with a <i>textstream</i>
    \param  tf  textstream
    \return     the begin() iterator associated with <i>tf</i>
*/
textstream_iterator begin(textstream& tf);

/*! \brief      Obtain the end() iterator associated with a <i>textstream</i>
    \param  tf  textstream
    \return     the end() iterator associated with <i>tf/i>
*/
inline textstream_iterator end(textstream& tf)
  { tf.nop();
    return textstream_iterator { };
  }

#endif    // TEXTFILE_H
