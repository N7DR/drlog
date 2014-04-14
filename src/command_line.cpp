#include "command_line.h"

#include <ctype.h>

// constructor
command_line::command_line(int argc, char** argv) : 
  _argc(argc), 
  _argv((char**)argv)
{ _arg = new std::string [_argc];

  for (unsigned int n = 0; n < _argc; n++)
    _arg[n] = (std::string)(_argv[n]);
}

// copy constructor
command_line::command_line(const command_line& cl) : 
  _argc(cl._argc), 
  _argv(cl._argv)
{ _arg = new std::string [_argc];

  for (unsigned int n = 0; n < _argc; n++)
    _arg[n] = (std::string)(_argv[n]);
}

// destructor
command_line::~command_line(void)
{ delete [] _arg;
}

// command_line = command_line
void command_line::operator=(const command_line& cl)
{ _argc = cl._argc;
  _argv = cl._argv;

  _arg = new std::string [_argc];

  for (unsigned int n = 0; n < _argc; n++)
    _arg[n] = (std::string)(_argv[n]);
}

// return parameter number (wrt 1)
std::string command_line::parameter(const unsigned int n) const
{ if (n >= _argc)
    throw x_command_line_invalid_parameter();

  return _arg[n];
}

// convert entire line to lower case
void command_line::tolower(void)
{ for (int n = 0; n <= n_parameters(); n++)
    tolower(n);
}

// convert item to lower case (note this is NOT the same as parameter number)
void command_line::tolower(const unsigned int n)
{ if (n >= _argc)
    throw x_command_line_invalid_parameter();

  std::string& s = _arg[n];
    
  for (unsigned int i = 0; i < s.length(); i++)
    s[i] = _tolower(s[i]);
}

// convert entire line to original case
void command_line::tooriginal(void)
{ for (unsigned int n = 0; n < _argc; n++)
    tooriginal(n);
}

// convert item to original case (note this is NOT the same as parameter number)
void command_line::tooriginal(const unsigned int n)
{ if (n >= _argc)
    throw x_command_line_invalid_parameter();
    
  _arg[n] = (std::string)(_argv[n]);
}

// convert entire line to upper case
void command_line::toupper(void)
{ for (int n = 0; n <= n_parameters(); n++)
    toupper(n);
}

// convert item to upper case (note this is NOT the same as parameter number)
void command_line::toupper(const unsigned int n)
{ if (n >= _argc)
    throw x_command_line_invalid_parameter();
    
  std::string& s = _arg[n];
    
  for (unsigned int i = 0; i < s.length(); i++)
    s[i] = _toupper(s[i]);
}

// is a particular value present?
bool command_line::value_present(const std::string& s) const
{ bool rv = false;
  
  for (int n = 1; n < n_parameters(); n++)    // < because last might be the actual value
    rv = (rv || (parameter(n) == s));

  return rv;
}

// return a value
std::string command_line::value(const std::string& s) const
{ if (!value_present(s))
    return "";

  std::string rv;

  for (int n = 1; n < n_parameters(); n++)    // < because last might be the actual value
  { if (parameter(n) == s)
      rv = parameter(n + 1);
  }

  return rv;
}

// is a particular parameter present?
bool command_line::parameter_present(const std::string& s) const
{ bool rv = false;
  
  for (int n = 1; n <= n_parameters(); n++)
    rv = (rv || (parameter(n) == s));

  return rv;
}
