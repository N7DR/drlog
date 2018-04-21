// $Id: pthread_support.cpp 147 2018-04-20 21:32:50Z  $

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
void create_thread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg, const string& thread_name)
{ const int status = pthread_create(thread, attr, start_routine, arg);

  if (status != 0)
  { string errname;

    switch (status)
    { case EAGAIN :
        errname = "EAGAIN";
        break;

      case EINVAL :
        errname = "EINVAL";
        break;

      case EPERM :
        errname = "EPERM";
        break;

      default :
        errname = "UNKNOWN [" + to_string(status) + "]";
    }

    const string message = errname + ": " + strerror(status);

    throw pthread_error(PTHREAD_CREATION_ERROR, thread_name + " error: " + message);
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
{ const int status = pthread_attr_init(&_attr);             // man page says that this always succeeds and returns 0

  if (status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure in pthread_attr_init(): error number = " + to_string(status));

  if (initial_attributes bitand PTHREAD_DETACHED)
    detached(true);
}

/// destructor
thread_attribute::~thread_attribute(void)
{ const int status = pthread_attr_destroy(&_attr);

  if (status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure in pthread_attr_destroy()");
}

/*! \brief      Set the detached state
    \param  b   whether to set to DETACHED (true) or JOINABLE (false)
*/
void thread_attribute::detached(const bool b)
{ const int status = pthread_attr_setdetachstate(&_attr, (b ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE));

  if (status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure setting detached state of attribute");
}

/*! \brief      Get the detached state
    \return     whether the thread is DETACHED
*/
const bool thread_attribute::detached(void) const
{ int state;

  const int status = pthread_attr_getdetachstate(&_attr, &state);

  if (status)
    throw pthread_error(PTHREAD_ATTR_ERROR, "Failure getting detached state of attribute");

  return (state == PTHREAD_CREATE_DETACHED);
}

// -------------------------------------- pt_mutex ------------------------

/*! \class  pt_mutex
    \brief  Encapsulate a pthread_mutex_t

    pt_mutex objects should be declared in global scope.
    This class implements a recursive mutex
*/

/// destructor
pt_mutex::~pt_mutex(void)
{ pthread_mutex_destroy(&_mutex);

  int* ip;

  ip = _tsd_refcount.get();
  delete ip; 
}

/// lock
void pt_mutex::lock(void)
{ int* ip;

  ip = _tsd_refcount.get();        // defined to return 0 if it has not been set in this thread

  if (ip == 0)
  { ip = new int(0);
    _tsd_refcount.set(ip);
  }
  
  if (*ip == 0)
  { const int status = pthread_mutex_lock(&_mutex);

    if (status != 0)
      throw pthread_error(PTHREAD_LOCK_ERROR, (string)"ERROR LOCKING MUTEX: " + to_string(status));

    _thread_id = pthread_self();
  }
  
  (*ip)++;
}

/// unlock
void pt_mutex::unlock(void)
{ int* ip;

  ip = _tsd_refcount.get();

  if (ip == 0)
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

    const int status = pthread_mutex_unlock(&_mutex);

    if (status != 0)
      throw pthread_error(PTHREAD_UNLOCK_ERROR, (string)"ERROR UNLOCKING MUTEX: " + to_string(status));
  }
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
{ pthread_cond_init(&_cond, NULL);
}

/*! \brief          Construct and associate a mutex with the condition variable
    \param  mtx     mutex to be associated with the condition variable
*/
pt_condition_variable::pt_condition_variable(pt_mutex& mtx) :
  _mutex_p(&mtx),
  _predicate(false)
{ pthread_cond_init(&_cond, NULL);
}

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
  { const int status =  pthread_cond_wait(&_cond, &(_mutex_p->_mutex));

    if (status != 0)
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
const bool pt_condition_variable::wait(const unsigned int n_secs)
{ if (_mutex_p == NULL)
    throw pthread_error(PTHREAD_INVALID_MUTEX, "pointer to mutex is NULL in timed wait() function");

  struct timespec timeout { static_cast<__time_t>(time(NULL) + n_secs), 0 };

  int status = pthread_cond_timedwait(&_cond, &(_mutex_p->_mutex), &timeout);

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

/*! \brief          Construct from a named mutex
    \param  ptm     mutex to be locked
    \param  name    name of mutex
*/
safelock::safelock(pt_mutex& ptm, const string& name) :
 _name(name)
{ try
  { _ptm_p = &ptm;
    _ptm_p->lock();
  }

  catch (...)
  { ost << "ERROR in safelock constructor: " + _name << endl;
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
    throw;
  }
}

/// How many threads belong to this process?
const unsigned int n_threads(void)
{ const pid_t pid = getpid();
  const string filename = string("/proc/") + to_string(pid) + "/status";
  const string contents = read_file(filename);
  const vector<string> lines = to_lines(contents);

  for (const auto& line : lines)
  { if (line.length() > 8 and line.substr(0, 8) == string("Threads:"))
   { const string n = remove_peripheral_spaces(line.substr(8));
     const unsigned int rv = from_string<unsigned int>(n);

     return rv;
   }
  }

  return 0;
}

// -----------------------------------------  Errors  -----------------------------------

pthread_error_messages::pthread_error_messages(void)
{ add(0,                         "No error");
  add(PTHREAD_LOCK_ERROR,        "Lock error");
  add(PTHREAD_UNLOCK_ERROR,      "Unlock error");
  add(PTHREAD_INVALID_MUTEX,     "Invalid mutex");
  add(PTHREAD_ATTR_ERROR,        "Error managing thread attribute");
  add(PTHREAD_CREATION_ERROR,    "Error creating thread");
}
