// $Id: internals.cpp 264 2025-03-13 20:01:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   internals.cpp

    Objects and functions related to GNU internals
*/

#include "internals.h"
#include "log_message.h"
#include "string_functions.h"

#include <cxxabi.h>

using namespace std;

extern message_stream ost;                                              ///< for debugging and logging

/*
   https://www.gnu.org/software/libc/manual/html_node/Backtraces.html:

Function: int backtrace (void **buffer, int size)

    Preliminary: | MT-Safe | AS-Unsafe init heap dlopen plugin lock | AC-Unsafe init mem lock fd | See POSIX Safety Concepts.

    The backtrace function obtains a backtrace for the current thread, as a list of pointers, and places the information into buffer. The argument size should be the number of void * elements that will fit into buffer. The return value is the actual number of entries of buffer that are obtained, and is at most size.

    The pointers placed in buffer are actually return addresses obtained by inspecting the stack, one return address per stack frame.

    Note that certain compiler optimizations may interfere with obtaining a valid backtrace. Function inlining causes the inlined function to not have a stack frame; tail call optimization replaces one stack frame with another; frame pointer elimination will stop backtrace from interpreting the stack contents correctly.

 -----

Function: char ** backtrace_symbols (void *const *buffer, int size)

    Preliminary: | MT-Safe | AS-Unsafe heap | AC-Unsafe mem lock | See POSIX Safety Concepts.

    The backtrace_symbols function translates the information obtained from the backtrace function into an array of strings. The argument buffer should be a pointer to an array of addresses obtained via the backtrace function, and size is the number of entries in that array (the return value of backtrace).

    The return value is a pointer to an array of strings, which has size entries just like the array buffer. Each string contains a printable representation of the corresponding element of buffer. It includes the function name (if this can be determined), an offset into the function, and the actual return address (in hexadecimal).

    Currently, the function name and offset can only be obtained on systems that use the ELF binary format for programs and libraries. On other systems, only the hexadecimal return address will be present. Also, you may need to pass additional flags to the linker to make the function names available to the program. (For example, on systems using GNU ld, you must pass -rdynamic.)

    The return value of backtrace_symbols is a pointer obtained via the malloc function, and it is the responsibility of the caller to free that pointer. Note that only the return value need be freed, not the individual strings.

    The return value is NULL if sufficient memory for the strings cannot be obtained.

-----

.we can extract the mangled function name _Z16displayBacktracev. To demangle this, we'd call the following code:

#include <cxxabi.h>
...
int status = -1;
char *demangledName = abi::__cxa_demangle( "_Z16displayBacktracev", NULL, NULL, &status );
if ( status == 0 )
{
	std::cout << demangledName << std::endl;
}
free( demangledName );

https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html

*/
gcc_backtrace::gcc_backtrace(const bool acquire, const int max_sz)
{ if (acquire)
  { _max_size = max_sz;

// get the initial backtrace as pointers
    void* vp [max_sz];
    void** vpp = vp;        // horrible C

    int status = backtrace(vpp, max_sz);

    ost << "backtrace size = " << status << endl;

    if (status > 0)
    { /* const */ char** cpp = backtrace_symbols(vpp, status);

      _backtrace.reserve(status);

      for (int n { 0 }; n < status; ++n)
      { const string demangled_name = _backtrace.emplace_back(cpp[n]);
        ost << n << ": " << _backtrace[n] << endl;
      }


      for (int n { 0 }; n < status; ++n)
      { //char *demangledName = abi::__cxa_demangle( "_Z16displayBacktracev", NULL, NULL, &status );

        // need to convert from something like: /home/n7dr/projects/drlog/build-debug/drlog(_ZN13gcc_backtraceC1Ebi+0xc9) [0x55a704be0875]

        const string str = delimited_substring <string> (_backtrace[n], '(', ')', DELIMITERS::DROP);
        const vector<string> fields = split_string <string> (str, "+"s);

        if (fields.size() > 1)
        { ost << n << ": fields[0] = " << fields[0] << endl;

          int val = -1;

          char* demangled = abi::__cxa_demangle( fields[0].c_str(), NULL, NULL, &val );

          if (val == 0)
          { //ost << "val = " << val << endl;
            ost << n << ": " << demangled << endl;
          }
          else
//            ost << n << ": elided; val = " << val << " demangled name = " << demangled << endl;
            ost << n << ": elided; val = " << val << " str = " << str << endl;
        }
        else
          ost << n << ": elided; str = " << str << endl;

//        _backtrace.emplace_back(cpp[n]);
//        ost << n << ": " << _backtrace[n] << endl;
      }
    }
  }
}
