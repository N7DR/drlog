// $Id: drlog_error.h 290 2026-03-30 15:48:47Z  $

#ifndef DRLOG_ERROR_H
#define DRLOG_ERROR_H

/*! \file drlog_error.h

    A simple base error class
*/

#include "macros.h"

#include <exception>
#include <string>

/*! \class  drlog_error
    \brief  Trivial base class for drlog errors
*/
class drlog_error : public std::exception
{
protected:

  int         _code;      ///< Error code
  std::string _reason;    ///< Error reason

public:

/*! \brief  Constructor from code and reason
    \param  n Error code
    \param  s Reason
*/
  inline drlog_error(const int n, const std::string_view s) :
    _code(n),
    _reason(s)
  { }

/*! \brief  Destructor
*/
  inline virtual ~drlog_error(void) = default;

  READ(code);      ///< Error code
  READ(reason);    ///< Error reason
};

#endif    // DRLOG_ERROR_H
