// $Id: command_line.cpp 163 2020-08-06 19:46:33Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   command_line.cpp

    API for managing the command line
*/

#include "command_line.h"
#include "string_functions.h"

#include <ctype.h>

using namespace std;

// -----------  command_line  ----------------

/*! \class  command_line
    \brief  Class that implements management of the command line
*/

/// internal initialisation function
void command_line::_init(void)
{ _arg = new string [_argc];

  for (unsigned int n = 0; n < _argc; n++)
    _arg[n] = (string)(_argv[n]);
}

/// command_line = command_line
void command_line::operator=(const command_line& cl)
{ _argc = cl._argc;
  _argv = cl._argv;

  if (_arg)
    delete [] _arg;

  _init();
}

/*! \brief  Obtain the base name of the program
    \return The base name of the program (i.e., with no "/" characters)
*/
string command_line::base_program_name(void) const
{ string rv { program_name() };

  const size_t posn { rv.find_last_of("/"s) };

  if (posn != string::npos)
    rv = rv.substr(posn + 1);

  return rv;
}

/*! \brief          Obtain a particular parameter from the command line
    \param  n       the number of the parameter to return (wrt 0)
    \return         the parameter corresponding to <i>n</i>

    If the value of <i>n</i> does not correspond to a parameter that was actually present, this functions throws an x_command_line_invalid_parameter()
*/
string command_line::parameter(const unsigned int n) const
{ if (n >= _argc)
    throw x_command_line_invalid_parameter();

  return _arg[n];
}

/*! \brief  Convert the entire command line to lower case
*/
void command_line::tolower(void)
{ for (int n = 0; n <= n_parameters(); n++)
    tolower(n);
}

/*! \brief      Convert a particular parameter to lower case
    \param  n   parameter number to convert (wrt 0)
*/
void command_line::tolower(const unsigned int n)
{ if (n >= _argc)
    throw x_command_line_invalid_parameter();

  string& s { _arg[n] };
    
  s = ::to_lower(s);
}

/*! \brief  Convert the entire command so that the case matches exactly what was originally passed to the program
*/
void command_line::tooriginal(void)
{ for (unsigned int n = 0; n < _argc; n++)
    tooriginal(n);
}

/*! \brief      Convert a particular parameter to its original case
    \param  n   parameter number to convert (wrt 0)
*/
void command_line::tooriginal(const unsigned int n)
{ if (n >= _argc)
    throw x_command_line_invalid_parameter();
    
  _arg[n] = (string)(_argv[n]);
}

/*! \brief  Convert the entire command line to upper case
*/
void command_line::toupper(void)
{ for (int n = 0; n <= n_parameters(); n++)
    toupper(n);
}

/*! \brief      Convert a particular parameter to upper case
    \param  n   parameter number to convert (wrt 0)
*/
void command_line::toupper(const unsigned int n)
{ if (n >= _argc)
    throw x_command_line_invalid_parameter();
    
  string& s { _arg[n] };

  s = ::to_upper(s);
}

/*! \brief      Is a particular value present?
    \param  v   value for which to look
    \return     whether the value corresponding to <i>v</i> is present

    A "value" is something like a parameter to a -xxx option. If, for example, value_present("-xxx") is TRUE, it means that -xxx is present, and a value follows it
*/
bool command_line::value_present(const string& s) const
{ bool rv { false };
  
  for (int n = 1; n < n_parameters(); n++)    // < because last might be the actual value
    rv = (rv || (parameter(n) == s));

  return rv;
}

/*! \brief          Return a particular value
    \param  v       value to return
    \return         the value

    A "value" is something like a parameter to a -xxx option. If, for example, the command line contains "-xxx burble", then value("-xxx") will return "burble"
*/
string command_line::value(const string& s) const
{ if (!value_present(s))
    return string();

  string rv;

  for (int n = 1; n < n_parameters(); n++)    // < because last might be the actual value
  { if (parameter(n) == s)
      rv = parameter(n + 1);
  }

  return rv;
}

/*! \brief      Is a particular parameter present?
    \param  p   parameter for which to look
    \return     whether the parameter <i>p</i> is present

    A "parameter" is an actual parameter that appears on the command line.
*/
bool command_line::parameter_present(const string& s) const
{ bool rv { false };
  
  for (int n = 1; n <= n_parameters(); n++)
    rv = (rv or (parameter(n) == s));

  return rv;
}
