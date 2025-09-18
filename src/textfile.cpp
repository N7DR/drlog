/*!     \file textfile.cpp

        Allow one to iterate line-by-line through a text file
*/

#include "log_message.h"
#include "textfile.h"

using namespace std;

extern message_stream    ost;       ///< debugging/logging output

// -----------  textfile_iterator  ----------------

/*! \class  textfile_iterator
    \brief  iterator for the <i>textfile</i> class
*/

/*! \brief      Pre-increment the iterator
    \return     the iterator, pre-incremented
*/
textfile_iterator& textfile_iterator::operator++(void)    // pre-incrementable
{ if (_streamp -> eof())
  { *this = textfile_iterator();  // default value if at the end of the file
    return *this;
  }

  static string the_line;       // construct it only once

  getline((*_streamp), the_line);    // get the next line

  _last_line_nr++;
  _last_line = move(the_line);

  return *this;
}

/*! \brief      Post-increment the iterator
    \return     the iterator, post-incremented
*/
textfile_iterator textfile_iterator::operator++(int)  // post-incrementable, returns prev value
{ textfile_iterator temp { *this };

  ++*this;

  return temp;
}

/*! \brief      Obtain the first iterator associated with a <i>textfile</i>
    \param  tf  textfile
    \return     the begin() iterator associated with <i>tf</i>
*/
textfile_iterator begin(textfile& tf)
{ textfile_iterator rv;
  string            the_line;

  if (tf.eof())
    return rv;

  rv._streamp = &tf;     // attach the stream to the iterator

  getline(tf, the_line);

  rv._last_line_nr = 0;
  rv._last_line = move(the_line);
  rv._strp = &rv._last_line;

  return rv;
}

// -----------  textstream_iterator  ----------------

/*! \class  textstream_iterator
    \brief  iterator for the <i>textstream</i> class
*/

/*! \brief      Pre-increment the iterator
    \return     the iterator, pre-incremented
*/
textstream_iterator& textstream_iterator::operator++(void)    // pre-incrementable
{ getline((*_streamp), _last_line);

  if (_streamp -> eof())
  { *this = textstream_iterator { };

    return *this;
  }

  _last_line_nr++;

  return *this;
}

/*! \brief      Obtain the first iterator associated with a <i>textstream</i>
    \param  tf  textstream
    \return     the begin() iterator associated with <i>tf</i>
*/
textstream_iterator begin(textstream& tf)
{ textstream_iterator rv;
  string              the_line;

  if (tf.eof())
    return rv;

  rv._streamp = &tf;     // attach the stream to the iterator

  std::getline(tf, the_line);

  if (rv._streamp -> eof())
    return textstream_iterator { };

  rv._last_line_nr = 0;
  rv._last_line    = move(the_line);
  rv._strp         = &rv._last_line;

  return rv;
}
