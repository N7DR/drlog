// $Id: pthread_support.cpp 271 2025-06-23 16:32:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    IPfonix, Inc.
//    N7DR

/*! \file   pthread_support.cpp

    Support for pthreads
*/

#include "log_message.h"
#include "pthread_support.h"
#include "string_functions.h"

#include <exception>

#include <cstring>
#include <sys/types.h>
#include <unistd.h>

extern message_stream ost;                        ///< for debugging, info

using namespace std;

/*

http://www.lambdacs.com/cpt/FAQ.html

(comp.programming.threads FAQ)

says:

Page 32 of the 1996 Posix.1 standard says "All functions defined by
Posix.1 and the C standard shall be thread-safe, except that the following
functions need not be thread-safe:

  asctime()
  ctime()
  getc_unlocked()*
  getchar_unlocked()*
  getgrid()
  getgrnam()
  getlogin()
  getpwnam()
  getpwuid()
  gmtime()
  localtime()
  putc_unlocked()*
  putchar_unlocked()*
  rand()
  readdir()
  strtok()
  ttyname()"

Note that a thread-safe XXX_r() version of the above are available,
other than those with an asterisk.  Also note that ctermid() and
tmpnam() are only thread-safe if a nonnull pointer is used as an
argument.

  Rich Stevens


    ================

POSIX and ANSI C specify only a small part of the "traditional UNIX
programming environment", though it's a start. The real danger in reading the
POSIX list quoted by Rich is that most people don't really know what's
included. While an inclusive list would be better than an exclusive list,
that'd be awfully long and awkward.

The Open Group (OSF and X/Open) has extended the Single UNIX Specification
(also known as "SPEC1170" for it's 1,170 UNIX interfaces, or UNIX95) to
include POSIX.1b-1993 realtime, POSIX.1c-1995 threads, and various
extensions. It's called the Single UNIX Specification, Version 2; or UNIX98.
Within this calendar year, it's safe to assume that most UNIX versions
currently branded by The Open Group (as XPG3, UNIX93, UNIX95) will extend
their brand validation to UNIX98.

The interfaces specification part of the Single UNIX Specification, Version 2
(known as XSH5), in section 2.8.2, "Thread-safety", specifies that all
interfaces defined by THIS specification will be thread-safe, except for "the
following". There are two explicit lists, and one implicit. One is the POSIX
list already quoted by Rich Stevens. The second is an additional list of
X/Open interfaces:

basename      dbm_open    fcvt        getutxline    pututxline
catgets       dbm_store   gamma       getw          setgrent
dbm_clearerr  dirname     gcvt        l64a          setkey
dbm_close     drand48     getdate     lgamma        setpwent
dbm_delete    ecvt        getenv      lrand48       setutxent
dbm_error     encrypt     getgrent    mrand48       strerror
dbm_fetch     endgrent    getpwent    nl_langinfo
dbm_firstkey  endpwent    getutxent   ptsname
dbm_nextkey   endutxent   getutxid    putenv

The implicit list is a statement that all interfaces in the "Legacy" feature
group need not be thread-safe. From another section, that list is:

advance       gamma          putw        sbrk          wait3
brk           getdtablesize  re_comp     sigstack
chroot        getpagesize    re_exec     step
compile       getpass        regcmp      ttyslot
cuserid       getw           regex       valloc
        
loc1          __loc1         loc2        locs

Obviously, this is still an exclusive list rather than inclusive. But then,
if UNIX95 had 1,170 interfaces, and UNIX98 is bigger, an inclusive list would
be rather awkward. (And don't expect ME to type it into the newsgroup!)

*/

/* 

The Linux documentation says that scheduling is available, but only if the policy is
a real-time one (i.e., not SCHED_OTHER, which is the default). The real-time policies
are available only to the super-user. So for now we shall not try to elevate the
priority of the [MAIN] thread. Windows doesn't support scheduling at all.

one consequence of this is that sched_yield() is not guaranteed to yield
  (see http://lkml.org/lkml/2205/8/20/34) 

*/

const pthread_error_messages pthread_error_message;     ///< object to hold error messages

/*! \brief                  Wrapper for pthread_create
    \param  thread          pointer to thread ID
    \param  attr            pointer to pthread attributes
    \param  start_routine   name of function to run in the new thread
    \param  arg             arguments to be passed to <i>start_function</i>
    \param  thread_name     name of the thread

    The first four parameters are passed without change to <i>pthread_create</i>
*/
//void create_thread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg, const string& thread_name)
void create_thread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg, const string_view thread_name)
{ if (const int status { pthread_create(thread, attr, start_routine, arg) }; status != 0)
  { string errname;

    switch (status)
    { case EAGAIN :
        errname = "EAGAIN"s;
        break;

      case EINVAL :
        errname = "EINVAL"s;
        break;

      case EPERM :
        errname = "EPERM"s;
        break;

      default :
        errname = "UNKNOWN ["s + to_string(status) + "]"s;
    }

    const string message { errname + ": "s + strerror(status) };

    ost << "Thread creation error for thread " << thread_name << ": " << message << endl;

    throw pthread_error(PTHREAD_CREATION_ERROR, thread_name + " error: "s + message);
  }
}

// -------------------------------------------  thread_attribute  -----------------------

/*! \class  thread_attribute
    \brief  Encapsulate pthread_attr information
*/

/*! \brief                      Construct with some attributes already set
    \param  initial_attributes  mask of attributes to be set at time of thread creation

    Supports only the PTHREAD_DETACHED attribute
*/
thread_attribute::thread_attribute(const unsigned int initial_attributes)
{ if (const int status { pthread_attr_init(&_attr) }; status)             // man page says that this always succeeds and returns 0
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure in pthread_attr_init(): error number = "s + to_string(status));

  if (initial_attributes bitand PTHREAD_DETACHED)
    detached(true);
}

/*! \brief          Construct using data from a thread
    \param  tid     thread_id
*/
thread_attribute::thread_attribute(const pthread_t tid)
{ int status { pthread_attr_init(&_attr) };             // man page says that this always succeeds and returns 0

  if (status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure in pthread_attr_init() for thread id "s + to_string(tid) + ": error number = "s + to_string(status));

  status = pthread_getattr_np(tid, &_attr);

  if (status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure in pthread_getattr_np() for thread id "s + to_string(tid) + ": error number = "s + to_string(status));
}

/// destructor
thread_attribute::~thread_attribute(void)
{ if (const int status { pthread_attr_destroy(&_attr) }; status)
    ost << "Failure in pthread_attr_destroy(); Error "  << PTHREAD_ATTR_ERROR << "; status = " << status << endl;
}

/*! \brief      Set the detached state
    \param  b   whether to set to DETACHED (true) or JOINABLE (false)
*/
void thread_attribute::detached(const bool b)
{ if (const int status { pthread_attr_setdetachstate(&_attr, (b ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE)) }; status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure setting detached state of attribute"s);
}

/*! \brief      Get the detached state
    \return     whether the thread is DETACHED
*/
bool thread_attribute::detached(void) const
{ int state;

  if (const int status { pthread_attr_getdetachstate(&_attr, &state) }; status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure getting detached state of attribute"s);

  return (state == PTHREAD_CREATE_DETACHED);
}

/*! \brief          Set the scheduling policy
    \param  policy  the requested policy

    <i>policy</i> MUST be one of:
        SCHED_FIFO
        SCHED_RR
    otherwise an exception is thrown

    See also: http://jackaudio.org/faq/linux_rt_config.html
*/
void thread_attribute::policy(const int policy)
{ if ( (policy != SCHED_FIFO) and (policy != SCHED_RR) )
    throw pthread_error(PTHREAD_UNRECOGNISED_POLICY, "Unrecognised thread policy: "s + to_string(policy));

  if (const int status { pthread_attr_setschedpolicy(&_attr, policy) }; status != 0)
    throw pthread_error(PTHREAD_POLICY_ERROR, "Error setting policy: "s + to_string(policy) + "status = "s + to_string(status));
}

/// get the scheduling policy
int thread_attribute::policy(void) const
{ int policy;

  if (const int status { pthread_attr_getschedpolicy(&_attr, &policy) }; status != 0)
    throw pthread_error(PTHREAD_POLICY_ERROR, "Error getting policy: "s + to_string(policy) + "status = "s + to_string(status));

  return policy;
}

/*! \brief          Set the scope
    \param  scope   the requested scope

    <i>scope</i> MUST be one of:
        PTHREAD_SCOPE_SYSTEM
        PTHREAD_SCOPE_PROCESS
    otherwise an exception is thrown
*/
void thread_attribute::scope(const int scope)
{ if ( (scope != SCHED_FIFO) and (scope != SCHED_RR) )
    throw pthread_error(PTHREAD_UNRECOGNISED_SCOPE, "Unrecognised thread scope: "s + to_string(scope));

  if (const int status { pthread_attr_setscope(&_attr, scope) }; status != 0)
    throw pthread_error(PTHREAD_SCOPE_ERROR, "Error setting scope: "s + to_string(scope) + "status = "s + to_string(status));
}

/// get the scope
int thread_attribute::scope(void) const
{ int scope;

  if (const int status { pthread_attr_getscope(&_attr, &scope) }; status != 0)
    throw pthread_error(PTHREAD_SCOPE_ERROR, "Error getting scope: "s + to_string(scope) + "status = "s + to_string(status));

  return scope;
}

/*! \brief              Set the scheduling inheritance policy
    \param  ipolicy     the requested inheritance policy

    <i>ipolicy</i> MUST be one of:
        PTHREAD_EXPLICIT_SCHED
        PTHREAD_INHERIT_SCHED
    otherwise an exception is thrown

    See also: http://jackaudio.org/faq/linux_rt_config.html
*/
void thread_attribute::inheritance_policy(const int ipolicy)
{ if ( (ipolicy != PTHREAD_EXPLICIT_SCHED) and (ipolicy != PTHREAD_INHERIT_SCHED) )
    throw pthread_error(PTHREAD_UNRECOGNISED_INHERITANCE_POLICY, "Unrecognised thread inheritance policy: "s + to_string(ipolicy));

  if (const int status { pthread_attr_setinheritsched(&_attr, ipolicy) }; status != 0)
    throw pthread_error(PTHREAD_INHERITANCE_POLICY_ERROR, "Error setting inheritance policy: "s + to_string(ipolicy) + "status = "s + to_string(status));
}

/// get the inheritance policy
int thread_attribute::inheritance_policy(void) const
{ int ipolicy;

  if (const int status { pthread_attr_getinheritsched(&_attr, &ipolicy) }; status != 0)
    throw pthread_error(PTHREAD_INHERITANCE_POLICY_ERROR, "Error getting inheritance policy: "s + to_string(ipolicy) + "status = "s + to_string(status));

  return ipolicy;
}

/*! \brief          Set the stack size
    \param  size    the requested stack size, in bytes

    See also: https://users.cs.cf.ac.uk/Dave.Marshall/C/node30.html; in particular,
    "[w]hen a stack is specified, the thread should also be created PTHREAD_CREATE_JOINABLE"
*/
void thread_attribute::stack_size(const size_t size)
{ if (const int status { pthread_attr_setstacksize(&_attr, size) }; status != 0)
    throw pthread_error(PTHREAD_STACK_SIZE_ERROR, "Error setting stack size: "s + to_string(size) + "status = "s + to_string(status));
}

/// get the stack size (in bytes)
size_t thread_attribute::stack_size(void) const
{ size_t size;

  if (const int status { pthread_attr_getstacksize(&_attr, &size) }; status != 0)
    throw pthread_error(PTHREAD_STACK_SIZE_ERROR, "Error getting stack size: "s + to_string(size) + "status = "s + to_string(status));

  return size;
}

/// maximum allowed priority for the scheduling policy
int thread_attribute::max_priority(void) const
{ const int sched_policy { policy() };
  const int status       { sched_get_priority_max(sched_policy) };

  if (status == -1)
    throw pthread_error(PTHREAD_PRIORITY_ERROR, "Error getting maximum priority for policy: "s + to_string(sched_policy));

  return status;
}

/// minimum allowed priority for the scheduling policy
int thread_attribute::min_priority(void) const
{ const int sched_policy { policy() };
  const int status       { sched_get_priority_min(sched_policy) };

  if (status == -1)
    throw pthread_error(PTHREAD_PRIORITY_ERROR, "Error getting minimum priority for policy: "s + to_string(sched_policy));

  return status;
}

/*! \brief              Set the scheduling priority for the scheduling policy
    \param  priority    the requested scheduling priority

    Adjusts <i>priority</i> if it is less than minimum permitted or
    more than maximum permitted.

    The scheduling policy has to be set before this is called.
*/
void thread_attribute::priority(const int priority)
{ const int p { min(max_priority(), max(min_priority(), priority)) };

  struct sched_param param;

  param.sched_priority = p;

  if (const int status { pthread_attr_setschedparam(&_attr, &param) }; status != 0)
    throw pthread_error(PTHREAD_PRIORITY_ERROR, "Error setting priority: "s + to_string(priority) + "status = "s + to_string(status));
}

/// get the priority
int thread_attribute::priority(void) const
{ struct sched_param param;

  if (const int status { pthread_attr_getschedparam(&_attr, &param) }; status != 0)
    throw pthread_error(PTHREAD_PRIORITY_ERROR, "Error getting priority; status = "s + to_string(status));

  return param.sched_priority;
}

/*! \brief          Write a <i>pthread_attr_t</i> object to an output stream
    \param  ost     output stream
    \param  pa      object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const pthread_attr_t& pa)
{ ost << "Thread attributes:" << endl
      << "  detached/joinable: " << (attribute_detached(pa) ? "PTHREAD_CREATE_DETACHED" : "PTHREAD_CREATE_JOINABLE") << endl
      << "  policy: ";

  const int tpolicy { attribute_policy(pa) };

  string policy_str;

  switch (tpolicy)
  { case SCHED_FIFO :
      policy_str = "SCHED_FIFO"s;
      break;

    case SCHED_OTHER :
      policy_str = "SCHED_OTHER"s;
      break;

    case SCHED_RR :
      policy_str = "SCHED_RR"s;
      break;

    default :
      policy_str = "UNKNOWN"s;
  }

  ost << policy_str << endl
      << "  scope: ";

  const int tscope { attribute_scope(pa) };

  string scope_str { };

  switch (tscope)
  { case PTHREAD_SCOPE_PROCESS :
      scope_str = "PTHREAD_SCOPE_PROCESS"s;
      break;

    case PTHREAD_SCOPE_SYSTEM :
      scope_str = "PTHREAD_SCOPE_SYSTEM"s;
      break;

    default :
      scope_str = "UNKNOWN"s;
  }

  ost << scope_str << endl
      << "  inheritance policy: ";

  const int ipolicy { attribute_inheritance_policy(pa) };

  string ipolicy_str;

  switch (ipolicy)
  { case PTHREAD_EXPLICIT_SCHED :
      ipolicy_str = "PTHREAD_EXPLICIT_SCHED"s;
      break;

    case PTHREAD_INHERIT_SCHED :
      ipolicy_str = "PTHREAD_INHERIT_SCHED"s;
      break;

    default :
      ipolicy_str = "UNKNOWN"s;
  }

  ost << ipolicy_str << endl
      << "  stack size: " << attribute_stack_size(pa) << endl
      << "  min allowed priority: " << attribute_min_priority(pa) << endl
      << "  max allowed priority: " << attribute_max_priority(pa) << endl
      << "  actual priority: " << attribute_priority(pa);

  return ost;
}

/*! \brief      Get the detached state of C-style attributes
    \param  pa  C-style attributes
    \return     whether <i>pa</i> is DETACHED
*/
bool attribute_detached(const pthread_attr_t& pa)
{ int state;

  if (const int status { pthread_attr_getdetachstate(&pa, &state) }; status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure getting detached state of attribute"s);

  return (state == PTHREAD_CREATE_DETACHED);
}

/*! \brief      Get the scheduling policy of C-style attributes
    \param  pa  C-style attributes
    \return     the scheduling policy associated with <i>pa</i>
*/
int attribute_policy(const pthread_attr_t& pa)
{ int policy;

  if (const int status { pthread_attr_getschedpolicy(&pa, &policy) }; status != 0)
    throw pthread_error(PTHREAD_POLICY_ERROR, "Error getting policy: "s + to_string(policy) + "status = "s + to_string(status));

  return policy;
}

/*! \brief      Get the scope of C-style attributes
    \param  pa  C-style attributes
    \return     the scope associated with <i>pa</i>
*/
int attribute_scope(const pthread_attr_t& pa)
{ int scope;

  if (const int status { pthread_attr_getscope(&pa, &scope) }; status != 0)
    throw pthread_error(PTHREAD_SCOPE_ERROR, "Error getting scope: "s + to_string(scope) + "status = "s + to_string(status));

  return scope;
}

/*! \brief      Get the inheritance policy of C-style attributes
    \param  pa  C-style attributes
    \return     the inheritance policy associated with <i>pa</i>
*/
int attribute_inheritance_policy(const pthread_attr_t& pa)
{ int ipolicy;

  if (const int status { pthread_attr_getinheritsched(&pa, &ipolicy) }; status != 0)
    throw pthread_error(PTHREAD_INHERITANCE_POLICY_ERROR, "Error getting inheritance policy: "s + to_string(ipolicy) + "status = "s + to_string(status));

  return ipolicy;
}

/*! \brief      Get the stack size of C-style attributes
    \param  pa  C-style attributes
    \return     the stack size associated with <i>pa</i>, in bytes
*/
size_t attribute_stack_size(const pthread_attr_t& pa)
{ size_t size;

  if (const int status { pthread_attr_getstacksize(&pa, &size) }; status != 0)
    throw pthread_error(PTHREAD_STACK_SIZE_ERROR, "Error getting stack size: "s + to_string(size) + "status = "s + to_string(status));

  return size;
}

/*! \brief      Get the maximum allowed priority for the scheduling policy of C-style attributes
    \param  pa  C-style attributes
    \return     maximum allowed priority for the scheduling policy of <i>pa</i>
*/
int attribute_max_priority(const pthread_attr_t& pa)
{ const int sched_policy { attribute_policy(pa) };
  const int status       { sched_get_priority_max(sched_policy) };

  if (status == -1)
    throw pthread_error(PTHREAD_PRIORITY_ERROR, "Error getting maximum priority for policy: "s + to_string(sched_policy));

  return status;
}

/*! \brief      Get the minimum allowed priority for the scheduling policy of C-style attributes
    \param  pa  C-style attributes
    \return     minimum allowed priority for the scheduling policy of <i>pa</i>
*/
int attribute_min_priority(const pthread_attr_t& pa)
{ const int sched_policy { attribute_policy(pa) };
  const int status       { sched_get_priority_min(sched_policy) };

  if (status == -1)
    throw pthread_error(PTHREAD_PRIORITY_ERROR, "Error getting minimum priority for policy: "s + to_string(sched_policy));

  return status;
}

/*! \brief      Get the priority of C-style attributes
    \param  pa  C-style attributes
    \return     the priority associated with <i>pa</i>
*/
int attribute_priority(const pthread_attr_t& pa)
{ struct sched_param param;

  if (const int status { pthread_attr_getschedparam(&pa, &param) }; status != 0)
    throw pthread_error(PTHREAD_PRIORITY_ERROR, "Error getting priority; status = "s + to_string(status));

  return param.sched_priority;
}

// -------------------------------------- pt_mutex ------------------------

/*! \class  pt_mutex
    \brief  Encapsulate a pthread_mutex_t

    pt_mutex objects should be declared in global scope.
    This class implements a recursive mutex
*/

/// destructor; do not write to message_stream, because the mutex in that class might no longer
// exist if we are in the process of cleaning up
pt_mutex::~pt_mutex(void)
{ pthread_mutex_destroy(&_mutex);

  int* ip { nullptr };

  ip = _tsd_refcount.get();

  if (ip)
    delete ip;
}

/// lock
void pt_mutex::lock(void)
{ int* ip { nullptr };

  ip = _tsd_refcount.get();        // defined to return 0 if it has not been set in this thread

  if (ip == nullptr)
  { ip = new int(0);
    _tsd_refcount.set(ip);
  }
  
  if (*ip == 0)
  { const int status { pthread_mutex_lock(&_mutex) };

    if (status != 0)
      throw pthread_error(PTHREAD_LOCK_ERROR, (string)"ERROR LOCKING MUTEX: " + to_string(status) + " : " + strerror(status));

    _thread_id = pthread_self();
  }
  
  (*ip)++;
}

/// unlock
void pt_mutex::unlock(void)
{ int* ip { nullptr };

  ip = _tsd_refcount.get();

  if (ip == nullptr)
  { ip = new int(0);
    _tsd_refcount.set(ip);
  }

  if (*ip == 0)
  { ost << "ATTEMPT TO UNLOCK A MUTEX THAT IS NOT BEING HELD" << endl;
    throw pthread_error(PTHREAD_UNLOCK_ERROR, (string)"ATTEMPT TO UNLOCK A MUTEX THAT IS NOT BEING HELD");
  }

  (*ip)--;
  if (*ip == 0)
  {
// we change this value even though we currently have the lock, so there is a non-zero probability that 
// we will report that no thread has this lock, even though in fact someone still does.  
    _thread_id = 0;

    const int status { pthread_mutex_unlock(&_mutex) };

    if (status != 0)
      throw pthread_error(PTHREAD_UNLOCK_ERROR, (string)"ERROR UNLOCKING MUTEX: " + to_string(status));
  }
}

/// rename
//void pt_mutex::rename(const string& new_name)
void pt_mutex::rename(const string_view new_name)
{ //lock();
  _name = new_name;
  //unlock();
}

// --------------------------------------------  pt_mutex_attributes  ----------------------------------

/*! \class  pt_mutex_attributes
    \brief  Encapsulate a pthread_mutexattr
*/

/// get the priority ceiling
int pt_mutex_attributes::priority_ceiling(void) const
{ int rv;
  
//  const int status { pthread_mutexattr_getprioceiling(&_mutexattr, &rv) };
  
  if (const int status { pthread_mutexattr_getprioceiling(&_mutexattr, &rv) }; status)
    throw pthread_error(PTHREAD_MUTEX_ATTR_GET_SET_ERROR, "ERROR GETTING MUTEX PRIORITY CEILING: "s  + to_string(status));
    
  return rv;
}

/*! \brief      Set the priority ceiling
    \param  pc  new priority ceiling
*/
void pt_mutex_attributes::priority_ceiling(const int pc)
{ //const int status { pthread_mutexattr_setprioceiling(&_mutexattr, pc) }; 

  if (const int status { pthread_mutexattr_setprioceiling(&_mutexattr, pc) }; status)
    throw pthread_error(PTHREAD_MUTEX_ATTR_GET_SET_ERROR, "ERROR SETTING MUTEX PRIORITY CEILING: "s  + to_string(status));
}

/// get the protocol
int pt_mutex_attributes::protocol(void) const
{ int rv;
  
  //const int status { pthread_mutexattr_getprotocol(&_mutexattr, &rv) };
  
  if (const int status { pthread_mutexattr_getprotocol(&_mutexattr, &rv) }; status)
    throw pthread_error(PTHREAD_MUTEX_ATTR_GET_SET_ERROR, "ERROR GETTING MUTEX PROTOCOL: "s  + to_string(status));

  return rv;
}

/// get the protocol name
string pt_mutex_attributes::protocol_name(void) const
{ switch (protocol())
  { case PTHREAD_PRIO_NONE :
      return "PTHREAD_PRIO_NONE"s;
      
    case PTHREAD_PRIO_INHERIT :
      return "PTHREAD_PRIO_INHERIT"s;
      
    case PTHREAD_PRIO_PROTECT :
      return "PTHREAD_PRIO_PROTECT"s;
      
    default :
      return "UNKNOWN"s;
  }
}

/*! \brief      Set the protocol
    \param  pr  new protocol
*/
void pt_mutex_attributes::protocol(const int pr)
{ //const int status { pthread_mutexattr_setprotocol(&_mutexattr, pr) }; 

  if (const int status { pthread_mutexattr_setprotocol(&_mutexattr, pr) }; status)
    throw pthread_error(PTHREAD_MUTEX_ATTR_GET_SET_ERROR, "ERROR SETTING MUTEX PROTOCOL: "s  + to_string(status));
}

/// get the type
int pt_mutex_attributes::type(void) const
{ int rv;
  
//  const int status { pthread_mutexattr_gettype(&_mutexattr, &rv) };

  if (const int status { pthread_mutexattr_gettype(&_mutexattr, &rv) }; status)
    throw pthread_error(PTHREAD_MUTEX_ATTR_GET_SET_ERROR, "ERROR GETTING MUTEX TYPE: "s  + to_string(status));
    
  return rv;
}

/// get the name of the type
string pt_mutex_attributes::type_name(void) const
{ const int t { type() };

  string rv { ( (t == PTHREAD_MUTEX_DEFAULT) ? "PTHREAD_MUTEX_DEFAULT = "s : string() ) };
  
  switch (t)
  { case PTHREAD_MUTEX_NORMAL :
      return rv + "PTHREAD_MUTEX_NORMAL"s;
      
    case PTHREAD_MUTEX_ERRORCHECK :
      return rv + "PTHREAD_MUTEX_ERRORCHECK"s;
      
    case PTHREAD_MUTEX_RECURSIVE :
      return rv + "PTHREAD_MUTEX_RECURSIVE"s;
      
//    case PTHREAD_MUTEX_DEFAULT :          same as PTHREAD_MUTEX_NORMAL
//      return "PTHREAD_MUTEX_DEFAULT";

    default :
      return rv + "UNKNOWN"s;
  }
}

/*! \brief      Set the type
    \param  ty  new type
*/
void pt_mutex_attributes::type(const int ty)
{ //const int status {  pthread_mutexattr_settype(&_mutexattr, ty) };

  if (const int status {  pthread_mutexattr_settype(&_mutexattr, ty) }; status)
    throw pthread_error(PTHREAD_MUTEX_ATTR_GET_SET_ERROR, "ERROR SETTING MUTEX TYPE: "s  + to_string(status));
}

// ------------------------------ pt_condition variable ------------------------------

/*! \class  pt_condition_variable
    \brief  Encapsulate a condition variable

    Should also work on systems that allow false wake-ups
*/

/// default constructor
pt_condition_variable::pt_condition_variable(void) :
  _mutex_p(NULL),
  _predicate(false)
{ pthread_cond_init(&_cond, NULL); }

/*! \brief          Construct and associate a mutex with the condition variable
    \param  mtx     mutex to be associated with the condition variable
*/
pt_condition_variable::pt_condition_variable(pt_mutex& mtx) :
  _mutex_p(&mtx),
  _predicate(false)
{ pthread_cond_init(&_cond, NULL); }

/*! \brief  Wait on the condition variable

    We MUST have the lock as we come into this routine
*/
void pt_condition_variable::wait(void)
{ if (_mutex_p == NULL)
    throw pthread_error(PTHREAD_INVALID_MUTEX, "pointer to mutex is NULL in untimed wait() function");

// check that the predicate isn't somehow already true
  if (_predicate)
    _predicate = false;

execute_wait:
  try
  { //const int status {  pthread_cond_wait(&_cond, &(_mutex_p->_mutex)) };

    if (const int status {  pthread_cond_wait(&_cond, &(_mutex_p->_mutex)) }; status != 0)
      throw pthread_error(PTHREAD_CONDVAR_WAIT_ERROR, "Error waiting on condition variable");
  }

// for now, ignore any error
  catch (...)
  {
  }

// we have to check the predicate, to guard against a false wake-up (Solaris on MP boxes is hosed)
  if (!_predicate)
    goto execute_wait;

  _predicate = false;
}

/*! \brief          Wait on the condition variable for a predefined duration
    \param  n_secs  number of seconds to wait
    \return         whether the wait timed-out
*/
bool pt_condition_variable::wait(const unsigned int n_secs)
{ if (_mutex_p == NULL)
    throw pthread_error(PTHREAD_INVALID_MUTEX, "pointer to mutex is NULL in timed wait() function");

  struct timespec timeout { static_cast<__time_t>(time(NULL) + n_secs), 0 };

  const int status { pthread_cond_timedwait(&_cond, &(_mutex_p->_mutex), &timeout) };

// ETIMEDOUT is not defined in Linux
  return (status != 0);
}

/*! \brief  Signal the condition variable

    According to POSIX, only one waiting thread gets woken. We MUST have the lock as we come into this routine.
*/
void pt_condition_variable::signal(void)
{ _predicate = true;                          // yes, we really mean it
  pthread_cond_signal(&_cond);
}

// -------------------------------------- safelock ------------------------

/*! \brief          Construct from a mutex
    \param  ptm     mutex to be locked
    \param  name    name of safelock (defaults to name of mutex)
*/
//safelock::safelock(pt_mutex& ptm, const string& name) :
safelock::safelock(pt_mutex& ptm, const string_view name) :
 _name(name.empty() ? ptm.name() : name)
{ try
  { _ptm_p = &ptm;
    _ptm_p -> lock();
  }

  catch (...)
  { ost << "ERROR in safelock constructor: " + _name + "; " << endl;
    throw;
  }
}

/*! \brief  Destructor
*/
safelock::~safelock(void)
{ try
  { _ptm_p->unlock();
  }

  catch (...)
  { ost << "ERROR in safelock destructor: " + _name << endl;
//    throw;            // don't throw in destructor
  }
}

/// How many threads belong to this process?
unsigned int n_threads(void)
{ const pid_t          pid      { getpid() };
  const string         filename { "/proc/"s + to_string(pid) + "/status"s };
  const string         contents { read_file(filename) };
  const vector<string> lines    { to_lines <std::string> (contents) };

  for (const auto& line : lines)
  { if ( (line.length() > 8) and (line.substr(0, 8) == string("Threads:")) )
   { const string       n  { remove_peripheral_spaces <std::string> (line.substr(8)) };
     const unsigned int rv { from_string<unsigned int>(n) };

     return rv;
   }
  }

  return 0;
}

// -----------------------------------------  Errors  -----------------------------------

pthread_error_messages::pthread_error_messages(void)
{ add(/* 0,                         */"No error");         // 0
  add(/* PTHREAD_LOCK_ERROR,        */"Lock error");       // PTHREAD_LOCK_ERROR
  add(/* PTHREAD_UNLOCK_ERROR,      */"Unlock error");     // PTHREAD_UNLOCK_ERROR
  add(/* PTHREAD_INVALID_MUTEX,     */"Invalid mutex");    // PTHREAD_INVALID_MUTEX
  add(/* PTHREAD_ATTR_ERROR,        */"Error managing thread attribute");  // PTHREAD_ATTR_ERROR
  add(/* PTHREAD_CREATION_ERROR,    */"Error creating thread");            // PTHREAD_CREATION_ERROR
}
