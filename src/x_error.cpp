#include "x_error.h"

using namespace std;

/*! \brief  Constructor from code and reason
    \param  n   Error code
    \param  s   Reason
*/
x_error::x_error(const int n, const string& s) :
  _code(n),
  _reason(s)
{ }
