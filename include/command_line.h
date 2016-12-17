// $Id: command_line.h 137 2016-12-15 20:07:54Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   command_line.h

    API for managing the command line
*/

#ifndef COMMANDLINEH
#define COMMANDLINEH

#include <string>

// -----------  command_line  ----------------

/*! \class command_line
    \brief Class that implements management of the command line
*/

class command_line
{
protected:
  unsigned int _argc;                    ///< Number of arguments

// these can't be const because of the = operator
  char** _argv;                      ///< Pointers to arguments

  std::string* _arg;                 ///< Pointers to arguments

public:

/*!     \brief  Constructor
        \param  argc    Number of arguments
        \param  argv    Pointer to array of individual arguments
*/
  command_line(int argc, char** argv);

/*! \brief  Copy constructor
    \param  obj   Object to be copied
*/
  command_line(const command_line& obj);

/*! \brief  Destructor
*/
  virtual ~command_line(void);

/// command_line = command_line
  void operator=(const command_line&);

/*! \brief          Obtain a particular parameter from the command line
    \param  n       the number of the parameter to return (wrt 0)
    \return         the parameter corresponding to <i>n</i>

    If the value of <i>n</i> does not correspond to a parameter that was actually present, this functions throws an x_command_line_invalid_parameter()
*/
  const std::string parameter(const unsigned int n) const;

/*! \brief      Obtain the name of the program
    \return     the name of the program
*/
  inline const std::string program_name(void) const
    { return _arg[0]; }

/*! \brief      Obtain the number of parameters passed to the program
    \return     the number of parameters
*/
  inline const int n_parameters(void) const
    { return (_argc - 1); }

/*! \brief  Convert the entire command line to lower case
*/
  void tolower(void);

/*! \brief  Convert the entire command so that the case matches exactly what was originally passed to the program
*/
  void tooriginal(void);

/*! \brief  Convert the entire command line to upper case
*/
  void toupper(void);

/*! \brief      Convert a particular parameter to lower case
    \param  n   parameter number to convert (wrt 0)
*/
  void tolower(const unsigned int n);

/*! \brief      Convert a particular parameter to its original case
    \param  n   parameter number to convert (wrt 0)
*/
  void tooriginal(const unsigned int n);

/*! \brief      Convert a particular parameter to upper case
    \param  n   parameter number to convert (wrt 0)
*/
  void toupper(const unsigned int n);

/*! \brief      Is a particular value present?
    \param  v   value for which to look
    \return     whether the value corresponding to <i>v</i> is present

    A "value" is something like a parameter to a -xxx option. If, for example, value_present("-xxx") is TRUE, it means that -xxx is present, and a value follows it
*/
  const bool value_present(const std::string& v) const;

/*! \brief          Return a particular value
    \param  v       value to return
    \return         the value
        
    A "value" is something like a parameter to a -xxx option. If, for example, the command line contains "-xxx burble", then value("-xxx") will return "burble"
*/
  const std::string value(const std::string& v) const;

/*! \brief      Is a particular parameter present?
    \param  p   parameter for which to look
    \return     whether the parameter <i>p</i> is present
        
    A "parameter" is an actual parameter that appears on the command line.
*/
  const bool parameter_present(const std::string& p) const;
};

// ---------------------------  exceptions  ----------------------

/*! \class  x_command_line_invalid_parameter
    \brief  Trivial class for exceptions in command line processing
*/

class x_command_line_invalid_parameter
{
};

#endif    // !COMMANDLINEH
