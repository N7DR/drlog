// $Id: x_error.h 223 2023-07-30 13:37:25Z  $

#ifndef X_ERROR_H
#define X_ERROR_H

/*! \file x_error.h

    A simple base error class
*/

#include <exception>
#include <string>

/*! \class  x_error
    \brief  Trivial base class for errors
*/
class x_error : public std::exception
{
protected:

  int         _code;            ///< Error code
  std::string _reason;          ///< Error reason

public:

/*! \brief  Constructor from code and reason
    \param  n   Error code
    \param  s   Reason
*/
  x_error(const int n, const std::string& s);

/*! \brief  Destructor
*/
  inline ~x_error(void) throw()
    { }

/*! \brief  RO access to _code
*/
  inline int code(void) const
    { return _code; }

/*! \brief  RO access to _reason
*/
  inline std::string reason(void) const
    { return _reason; }
};

#endif    // X_ERROR_H

