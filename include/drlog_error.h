// $Id: drlog_error.h 268 2025-05-04 12:31:03Z  $

#ifndef DRLOG_ERROR_H
#define DRLOG_ERROR_H

/*! \file drlog_error.h

    A simple base error class
*/

#include <exception>
#include <string>

/*! \class  drlog_error
  \brief  Trivial base class for drlog errors 
*/
class drlog_error : public std::exception
{
protected:

  int         _code;      ///< Error code
  std::string _reason;      ///< Error reason

public:

/*! \brief  Constructor from code and reason
  \param  n Error code
  \param  s Reason
*/
  drlog_error(const int n, const std::string& s);

/*! \brief  Destructor
*/
  inline virtual ~drlog_error(void) = default; /* throw() */

/*! \brief  RO access to _code
*/
  inline /* const */int code(void) const
    { return _code; }

/*! \brief  RO access to _reason
*/
  inline const std::string reason(void) const
    { return _reason; }
};

#endif    // DRLOG_ERROR_H
