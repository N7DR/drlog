#include "drlog_error.h"

using namespace std;

/*! \brief  Constructor from code and reason
  \param  n Error code
  \param  s Reason
*/
drlog_error::drlog_error(const int n, const string& s) :
  _code(n),
  _reason(s)
{
}
