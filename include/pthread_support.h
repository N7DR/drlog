// $Id: pthread_support.h 145 2018-03-19 17:28:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    IPfonix, Inc.
//    N7DR

#ifndef PTHREADSUPPORT_H
#define PTHREADSUPPORT_H

/*! \file   pthread_support.h

    Support for pthreads
*/

#include "x_error.h"

#include <string>
#include <vector>

#include <pthread.h>

/// Lock a mutex
#define LOCK(z)  z##_mutex.lock()

/// Unlock a mutex
#define UNLOCK(z) z##_mutex.unlock()

/// Syntactic sugar to create a safe lock
#define SAFELOCK(z) safelock safelock_z(z##_mutex, (std::string)(#z))

// errors
const int PTHREAD_LOCK_ERROR            = -1,       ///< Error locking mutex
          PTHREAD_UNLOCK_ERROR          = -2,       ///< Error unlocking mutex
          PTHREAD_INVALID_MUTEX         = -3,       ///< Attempt to operate on an invalid mutex
          PTHREAD_ATTR_ERROR            = -4,       ///< Error when managing a thread_attribute
          PTHREAD_CREATION_ERROR        = -5,       ///< Error attempting to create a pthread
          PTHREAD_CONDVAR_WAIT_ERROR    = -6;       ///< Error while waiting on a condvar

// attributes that can be set at the time that a thread_attribute object is created
const unsigned int PTHREAD_DETACHED     = 1;        ///< detached pthread

/*! \brief                  Wrapper for pthread_create()
    \param  thread          pointer to thread ID
    \param  attr            pointer to pthread attributes
    \param  start_routine   name of function to run in the new thread
    \param  arg             arguments to be passed to <i>start_function</i>
    \param  thread_name     name of the thread

    The first four parameters are passed without change to <i>pthread_create</i>
*/
void create_thread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg, const std::string& thread_name = "");

// -------------------------------------------  thread_attribute  -----------------------

/*! \class  thread_attribute
    \brief  Encapsulate pthread_attr information
*/

class thread_attribute
{
protected:

  pthread_attr_t    _attr;          ///< attributes

public:

/*! \brief                      Construct with some attributes already set
    \param  initial_attributes  mask of attributes to be set at time of thread creation

    Supports only the PTHREAD_DETACHED attribute
*/
  explicit thread_attribute(const unsigned int initial_attributes = 0);

/// destructor
  virtual ~thread_attribute(void);

/*! \brief      Set the detached state
    \param  b   whether to set to DETACHED (true) or JOINABLE (false)
*/
  void detached(const bool b);

/*! \brief      Get the detached state
    \return     whether the thread is DETACHED
*/
  const bool detached(void) const;

/// get attributes
  inline const pthread_attr_t& attr(void) const  // note the reference
    { return _attr; }
};

// -------------------------------------------  thread_specific_data  -----------------------

/*! \class  thread_specific_data
    \brief  Encapsulate thread-specific data

    Typically, objects of this type are declared globally
*/

template<class T> class thread_specific_data
{
protected:

  pthread_key_t _key;                       ///< key into thread-specific data

public:

/// constructor
  thread_specific_data(void)
    { pthread_key_create(&_key, NULL); }

/*! \brief      Get a pointer into the thread-specific data
    \return     pointer to thread-specific data

    Returns 0 if the data do not exist
*/
  T* get(void)
    { T* tp = (T*)pthread_getspecific(_key); 
      if (tp)
        return tp;

      return 0;
    }

/*! \brief      Set thread-specific data
    \param  tp  pointer to data that is to become thread-specific
*/
  void set(const T* tp)
    { pthread_setspecific(_key, tp); }
};

// --------------------------------------------  pt_mutex  ----------------------------------

/*! \class  pt_mutex
    \brief  Encapsulate a pthread_mutex_t
  
    pt_mutex objects should be declared in global scope.
    This class implements a recursive mutex
*/

class pt_mutex
{
protected:

  pthread_mutex_t           _mutex;             ///< Encapsulated mutex
  pthread_t                 _thread_id;         ///< ID of the thread that owns the locked mutex
  thread_specific_data<int> _tsd_refcount;      ///< reference counter for recursive locking

/*! \brief  Copy constructor

    Not public, so it cannot be called, since we should never copy a mutex
*/
  pt_mutex(const pt_mutex&);

public:

/// constructor
  pt_mutex(void);

/// destructor
  virtual ~pt_mutex(void);

/// lock the mutex
  void lock(void);

/// unlock the mutex
  void unlock(void);

/// get the thread ID
  inline const pthread_t thread_id(void) const
    { return _thread_id; }

  friend class pt_condition_variable;       ///< needs access to details of the mutex
};

// --------------------------------------------  pt_condition variable  --------------------------

/*! \class  pt_condition_variable
    \brief  Encapsulate a condition variable
  
    Should also work on systems that allow false wake-ups
*/

class pt_condition_variable
{
protected:

  pthread_cond_t _cond;         ///< Encapsulated condition variable
  pt_mutex*      _mutex_p;      ///< Pointer to associated mutex
  bool           _predicate;    ///< predicate used to handle false wake-ups on brain-dead systems

/*! \brief  Copy constructor

  Not public, so it cannot be called, since we should never copy a condition variable
*/
  pt_condition_variable(const pt_condition_variable&);


public:

/*! \brief  Default Constructor
*/
  pt_condition_variable(void);

/*! \brief          Construct and associate a mutex with the condition variable
    \param  mtx     mutex to be associated with the condition variable
*/
  pt_condition_variable(pt_mutex& mtx);
   
/// destructor
  inline virtual ~pt_condition_variable(void)
    { pthread_cond_destroy(&_cond); }

/*! \brief          Set the value of the associated mutex
    \param  mtx     mutex to associate with this condition variable
  
    Typically used with the default constructor
*/
  inline void set_mutex(pt_mutex& mtx)
    { _mutex_p = &mtx; }

/*! \brief  Wait on the condition variable

    We MUST have the lock as we come into this routine
*/
  void wait(void);

/*! \brief          Wait on the condition variable for a predefined duration
    \param  n_secs  number of seconds to wait
    \return         whether the wait timed-out
*/
  const bool wait(const unsigned int n_secs);

/*! \brief  Signal the condition variable

    According to POSIX, only one waiting thread gets woken. We MUST have the lock as we come into this routine.
*/
  void signal(void);

/*! \brief  Broadcast the condition variable

    According to POSIX, all waiting threads get woken.
*/
  inline void broadcast(void)
    { pthread_cond_broadcast(&_cond); }
};

// ------------------------------------------------  safelock  ---------------------------

/*! \class  safelock
    \brief  Implements an exception-friendly mechanism for locking

    Locks a mutex that is automatically unlocked when the destructor is called
*/

class safelock
{
protected:

  std::string _name;        ///< name (if any)
  pt_mutex*   _ptm_p;       ///< encapsulated mutex

/*! \brief  Copy constructor

    Not public, so it cannot be called, since we should never copy a mutex
*/
  safelock(const safelock&);

/*! \brief  safelock = safelock

    Not public, so it cannot be called, since we should never copy a mutex
*/
  void operator=(const safelock&);

public:

/*! \brief          Construct from a mutex
    \param  mtx     mutex to be locked
*/
  safelock(pt_mutex& mtx);

/*! \brief          Construct from a named mutex
    \param  mtx     mutex to be locked
    \param  name    name of mutex
*/
  safelock(pt_mutex& mtx, const std::string& name);

/*! \brief  Destructor
*/
  ~safelock(void);
};

// -------------------------------------- Error messages  -----------------------------------

/*! \class  pthread_error_messages
    \brief  Error messages related to string processing
*/

class pthread_error_messages : public std::vector<std::string>
{
public:

/// Default constructor
  pthread_error_messages(void);

/// Destructor
  virtual ~pthread_error_messages(void) { }

/*!  \brief             Add a reason message to the list of possible error messages
     \param  code       reason code
     \param  reason     message to add
*/
  inline void add(const int code, const std::string& reason)
    { push_back(reason); }
};

/// How many threads belong to this process?
const unsigned int n_threads(void);

extern const pthread_error_messages pthread_error_message;    ///< pthread error messages

// -------------------------------------- Errors  -----------------------------------

/*! \class  pthread_error
    \brief  Errors related to string processing
*/

class pthread_error : public x_error
{ 
protected:

public:

/*! \brief  Construct from error code and reason
    \param  n Error code
    \param  s Reason
*/
  pthread_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};

#endif    // PTHREADSUPPORT_H
