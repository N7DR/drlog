// $Id: pthread_support.h 44 2013-12-13 17:45:49Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef PTHREADSUPPORTH
#define PTHREADSUPPORTH

/*! \file pthread_support.h

    Support for pthreads
*/

#include "x_error.h"

#include <string>
#include <vector>

#if (defined(LINUX))
#include <pthread.h>
#endif

/// Lock a mutex
#define LOCK(z)  /* cout << ((std::string)("LOCK ") + #z ) << endl; */ z##_mutex.lock()

/// Unlock a mutex
#define UNLOCK(z) /* cout << ((std::string)("UNLOCK ") + #z ) << endl; */ z##_mutex.unlock()

/// Syntactic sugar to create a safe lock
#define SAFELOCK(z) safelock safelock_z(z##_mutex, (std::string)(#z))

//template <class T>
//const T SAFELOCK_GET(const T& v, pt_mutex& m)
//{ safelock safelock_z(m, "SAFELOCK_GET");
//
//  return v;
//}

/// Ordinary definition to catch all exceptions
#if !defined(CATCHALL)

#if (defined(LINUX))
#define CATCHALL catch (...)
#endif    // LINUX

#endif    // !CATCHALL

// errors
const int PTHREAD_LOCK_ERROR            = -1,    ///< Error locking mutex
          PTHREAD_UNLOCK_ERROR          = -2,    ///< Error unlocking mutex
          PTHREAD_INVALID_MUTEX         = -3,    ///< Attempt to operate on an invalid mutex
          PTHREAD_ATTR_ERROR            = -4,    ///< Error when managing a thread_attribute
          PTHREAD_CREATION_ERROR        = -5;    ///< Error attempting to create a pthread

// attributes that can be set at cthe time that a thread_attribute object is created
const unsigned int PTHREAD_DETACHED     = 1;

// wrapper for pthread_create
void create_thread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg, const std::string& thread_name = "");

// -------------------------------------------  thread_attribute  -----------------------

/*! \class  pthread_attribute
    \brief  Encapsulate pthread_attr information
*/

class thread_attribute
{
protected:
  pthread_attr_t    _attr;

public:

/// default constructor
//  thread_attribute(void);

/// construct with some attributes already set
  explicit thread_attribute(const unsigned int initial_attributes = 0);

/// destructor
  virtual ~thread_attribute(void);

// get/set detached state
  void detached(const bool b);

  const bool detached(void);

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
  pthread_key_t _key;     ///< Key into thread-specific data

public:

/*! \brief  Default Constructor
*/
  thread_specific_data(void)
    { pthread_key_create(&_key, NULL); }

/*! \brief  Get a pointer into the thread-specific data
    \return Pointer to thread-specific data

    Returns 0 if the data do not exist
*/
  T* get(void)
    { T* tp = (T*)pthread_getspecific(_key); 
      if (tp)
        return tp;

      return 0;
    }

/*! \brief  Set thread-specific data
    \param  tp  Pointer to data that is to become thread-specific
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
  pthread_mutex_t _mutex;     ///< Encapsulated mutex  
  pthread_t       _thread_id;                   ///< ID of the thread that owns the locked mutex
  thread_specific_data<int> _tsd_refcount;  ///< reference counter for recursive locking

/*! \brief  Copy constructor

    Not public, so it cannot be called, since we should never copy a mutex
*/
  pt_mutex(const pt_mutex&);

public:

/*! \brief  Default Constructor
*/
  pt_mutex(void);

/*! \brief  Destructor
*/
  virtual ~pt_mutex(void);

/*! \brief  Lock the mutex
*/
  void lock(void);

/*! \brief  Unlock the mutex
*/
  void unlock(void);

/// Return the thread ID
  inline const pthread_t thread_id(void) const
    { return _thread_id; }

  friend class pt_condition_variable;
};

// --------------------------------------------  pt_condition variable  --------------------------

/*! \class  pt_condition_variable
    \brief  Encapsulate a condition variable
  
    Should also work on systems that allow false wake-ups
*/

class pt_condition_variable
{
protected:
  pthread_cond_t _cond;     ///< Encapsulated condition variable
  pt_mutex*      _mutex_p;    ///< Pointer to associated mutex
  bool           _predicate;    ///< predicate used to handle false wake-ups on brain-dead systems

/*! \brief  Copy constructor

  Not public, so it cannot be called, since we should never copy a condition variable
*/
  pt_condition_variable(const pt_condition_variable&);


public:

/*! \brief  Default Constructor
*/
  pt_condition_variable(void);

/*! \brief  Construct and associate a mutex with the condition variable
  \param  mtx Mutex to be associated with the condition variable
*/
  pt_condition_variable(pt_mutex& mtx);
   
/*! \brief  Destructor
*/
  virtual ~pt_condition_variable(void);

/*! \brief  Set the value of the associated mutex
    \param  mtx Mutex to associate with this condition variable
  
    Typically used with the default constructor
*/
  void set_mutex(pt_mutex& mtx);

/*! \brief  Wait on the condition variable
*/
  void wait(void);

/*! \brief  Wait on the condition variable for a predefined duration
    \param  n_secs  Number of seconds to wait
    \return Whether the wait timed-out
*/
  bool wait(unsigned int n_secs);

/*! \brief  Signal the condition variable

    According to POSIX, only one waiting thread gets woken
*/
  void signal(void);

/*! \brief  Broadcast the condition variable

    According to POSIX, all waiting threads get woken
*/
  void broadcast(void);
};

// ------------------------------------------------  safelock  ---------------------------

/*! \class  safelock
    \brief  Implements an exception-friendly mechanism for locking

    Locks a mutex that is automatically unlocked when the destructor is called
*/

class safelock
{
protected:

  pt_mutex* _ptm_p;     ///< Encapsulated mutex
  std::string _name;      ///< Name (if any)

/*! \brief  Copy constructor

  Not public, so it cannot be called, since we should never copy a mutex
*/
  safelock(const safelock&);

/*! \brief  safelock = safelock

  Not public, so it cannot be called, since we should never copy a mutex
*/
  void operator=(const safelock&);

public:

/*! \brief  Construct from a mutex
    \param  mtx Mutex to be locked
*/
  safelock(pt_mutex& mtx);

/*! \brief  Construct from a named mutex
    \param  mtx Mutex to be locked
    \param  name  Name of mutex
*/
  safelock(pt_mutex& mtx, const std::string& name);

/*! \brief  Destructor
*/
  ~safelock(void);
};

#if (defined(LINUX) || defined(SOLARIS))
void SCHED_YIELD(void);
#endif

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

/*!  \brief  Add a reason message to the list of possible error messages
     \param  code    Reason code
     \param  reason  Message to add
*/
  inline void add(const int code, const std::string& reason)
    { push_back(reason); }
};

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

#endif    // PTHREADSUPPORTH
