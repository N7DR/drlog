// $Id: command_line.h 13 2013-02-15 22:24:04Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef COMMANDLINEH
#define COMMANDLINEH

#include <string>

/*! \file command_line.h

    API for managing the command line
*/

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

/*!     command_line = command_line
*/
  void operator=(const command_line&);

/*!     \brief  Obtain a particular parameter from the command line
        \param  n       The number of the parameter to return (wrt 0)
        \return The parameter corresponding to <i>n</i>
        
        If the value of <i>n</i> does not correspond to a parameter that was actually present, this functions throws an x_command_line_invalid_parameter()
*/
  std::string parameter(const unsigned int n) const;

/*!     \brief  Obtain the name of the program
        \return The name of the program
*/
  inline std::string program_name(void) const
    { return _arg[0]; }

/*!     \brief  Obtain the number of parameters passed to the program
        \return The number of parameters
*/
  inline int n_parameters(void) const
    { return (_argc - 1); }

/*!     \brief  Convert the entire command line to lower case
*/
  void tolower(void);

/*!     \brief  Convert the entire command so that the case matches exactly what was originally passed to the program
*/
  void tooriginal(void);

/*!     \brief  Convert the entire command line to upper case
*/
  void toupper(void);

/*!     \brief  Convert a particular parameter to lower case
        \param  n  Parameter number to convert (wrt 0) 
*/
  void tolower(const unsigned int n);

/*!     \brief  Convert a particular parameter to its original case
        \param  n  Parameter number to convert (wrt 0) 
*/
  void tooriginal(const unsigned int n);

/*!     \brief  Convert a particular parameter to upper case
        \param  n  Parameter number to convert (wrt 0) 
*/
  void toupper(const unsigned int n);

/*!     \brief  Is a particular value present?
        \param  v       Value for which to look
        \return Whether the value corresponding to <i>v</i> is present
        
        A "value" is something like a parameter to a -xxx option. If, for example, value_present("-xxx") is TRUE, it means that -xxx is present, and a value follows it  
*/
  bool value_present(const std::string& v) const;

/*!     \brief  Return a particular value
        \param  v       Value to return
        \return The value
        
        A "value" is something like a parameter to a -xxx option. If, for example, the command line contains "-xxx burble", then value("-xxx") will return "burble"  
*/
  std::string value(const std::string& v) const;

/*!     \brief  Is a particular parameter present?
        \param  p       Parameter for which to look
        \return Whether the parameter <i>p</i> is present
        
        A "parameter" is an actual parameter that appears on the command line.
*/
  bool parameter_present(const std::string& p) const;

};

// ---------------------------  exceptions  ----------------------

/*! \class  x_command_line_invalid_parameter
    \brief  Trivial class for exceptions in command line processing
*/

class x_command_line_invalid_parameter
{
};

#endif    // !COMMANDLINEH
