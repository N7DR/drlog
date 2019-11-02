// $Id: pthread_support.h 152 2019-08-21 20:23:38Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    IPfonix, Inc.
//    N7DR

// I find pthreads to be far less confusing than the native C++ library support for threading

#ifndef PTHREADSUPPORT_H
#define PTHREADSUPPORT_H

/*! \file   pthread_support.h

    Support for pthreads
*/

#include "macros.h"
#include "x_error.h"

#include <iostream>
#include <string>
#include <vector>

#include <pthread.h>

using namespace std::literals::string_literals;

/// Lock a mutex
#define LOCK(z)  z##_mutex.lock()

/// Unlock a mutex
#define UNLOCK(z) z##_mutex.unlock()

/// Syntactic sugar to create a safe lock
#define SAFELOCK(z) safelock safelock_z(z##_mutex, (std::string)(#z))

// errors
constexpr int PTHREAD_LOCK_ERROR                      { -1 },       ///< Error locking mutex
              PTHREAD_UNLOCK_ERROR                    { -2 },       ///< Error unlocking mutex
              PTHREAD_INVALID_MUTEX                   { -3 },       ///< Attempt to operate on an invalid mutex
              PTHREAD_ATTR_ERROR                      { -4 },       ///< Error when managing a thread_attribute
              PTHREAD_CREATION_ERROR                  { -5 },       ///< Error attempting to create a pthread
              PTHREAD_CONDVAR_WAIT_ERROR              { -6 },       ///< Error while waiting on a condvar
              PTHREAD_UNRECOGNISED_POLICY             { -7 },       ///< Policy is unknown
              PTHREAD_POLICY_ERROR                    { -8 },       ///< Error setting policy
              PTHREAD_UNRECOGNISED_SCOPE              { -9 },       ///< Scope is unknown
              PTHREAD_SCOPE_ERROR                     { -10 },      ///< Error setting scope
              PTHREAD_UNRECOGNISED_INHERITANCE_POLICY { -11 },      ///< Inheritance policy is unknown
              PTHREAD_INHERITANCE_POLICY_ERROR        { -12 },      ///< Error setting inheritance policy
              PTHREAD_STACK_SIZE_ERROR                { -13 },      ///< Error setting stack size
              PTHREAD_PRIORITY_ERROR                  { -14 },      ///< Error related to priority
              PTHREAD_MUTEX_ATTR_GET_SET_ERROR        { -15 };      ///< Error getting or setting a mutex attribute

// attributes that can be set at the time that a thread_attribute object is created
constexpr unsigned int PTHREAD_DETACHED     { 1 };        ///< detached pthread

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

/*! \brief          Construct using data from a thread
    \param  tid     thread_id
*/
  explicit thread_attribute(const pthread_t tid);


/*! \brief              Construct using data from a C-style attribute "object"
    \param  ori_attr    C-style attributes
*/
  inline explicit thread_attribute(const pthread_attr_t& ori_attr) :
    _attr(ori_attr)
    { }

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

/*! \brief          Set the scheduling policy
    \param  policy  the requested policy

    <i>policy</i> MUST be one of:
        SCHED_FIFO
        SCHED_RR
    otherwise an exception is thrown

    See also: http://jackaudio.org/faq/linux_rt_config.html
*/
  void policy(const int policy);

/// get the scheduling policy
  const int policy(void) const;

/*! \brief          Set the scope
    \param  scope   the requested scope

    <i>scope</i> MUST be one of:
        PTHREAD_SCOPE_SYSTEM
        PTHREAD_SCOPE_PROCESS
    otherwise an exception is thrown
*/
  void scope(const int scope);

/// get the scope
  const int scope(void) const;

/*! \brief              Set the scheduling inheritance policy
    \param  ipolicy     the requested inheritance policy

    <i>ipolicy</i> MUST be one of:
        PTHREAD_EXPLICIT_SCHED
        PTHREAD_INHERIT_SCHED
    otherwise an exception is thrown

    See also: http://jackaudio.org/faq/linux_rt_config.html
*/
  void inheritance_policy(const int ipolicy);

/// get the inheritance policy
  const int inheritance_policy(void) const;

/*! \brief          Set the stack size
    \param  size    the requested stack size, in bytes

    See also: https://users.cs.cf.ac.uk/Dave.Marshall/C/node30.html; in particular,
    "[w]hen a stack is specified, the thread should also be created PTHREAD_CREATE_JOINABLE"
*/
  void stack_size(const size_t size);

/// get the stack size (in bytes)
  const size_t stack_size(void) const;

/// maximum allowed priority for the scheduling policy
  const int max_priority(void) const;

/// minimum allowed priority for the scheduling policy
  const int min_priority(void) const;

/*! \brief              Set the scheduling priority for the scheduling policy
    \param  priority    the requested scheduling priority

    Adjusts <i>priority</i> if it is less than minimum permitted or
    more than maximum permitted.

    The scheduling policy has to be set before this is called.
*/
  void priority(const int priority);

/// get the priority
  const int priority(void) const;

/// get attributes
  inline const pthread_attr_t& attr(void) const  // note the reference
    { return _attr; }
};

/*! \brief          Write a <i>thread_attribute</i> object to an output stream
    \param  ost     output stream
    \param  ta      object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const thread_attribute& ta);

/*! \brief          Write a <i>pthread_attr_t</i> object to an output stream
    \param  ost     output stream
    \param  pa      object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const pthread_attr_t& pa);

/*! \brief      Get the detached state of C-style attributes
    \param  pa  C-style attributes
    \return     whether <i>pa</i> is DETACHED
*/
const bool attribute_detached(const pthread_attr_t& pa);

/*! \brief      Get the scheduling policy of C-style attributes
    \param  pa  C-style attributes
    \return     the scheduling policy associated with <i>pa</i>
*/
const int attribute_policy(const pthread_attr_t& pa);

/*! \brief      Get the scope of C-style attributes
    \param  pa  C-style attributes
    \return     the scope associated with <i>pa</i>
*/
const int attribute_scope(const pthread_attr_t& pa);

/*! \brief      Get the inheritance policy of C-style attributes
    \param  pa  C-style attributes
    \return     the inheritance policy associated with <i>pa</i>
*/
const int attribute_inheritance_policy(const pthread_attr_t& pa);

/*! \brief      Get the stack size of C-style attributes
    \param  pa  C-style attributes
    \return     the stack size associated with <i>pa</i>, in bytes
*/
const size_t attribute_stack_size(const pthread_attr_t& pa);

/*! \brief      Get the maximum allowed priority for the scheduling policy of C-style attributes
    \param  pa  C-style attributes
    \return     maximum allowed priority for the scheduling policy of <i>pa</i>
*/
const int attribute_max_priority(const pthread_attr_t& pa);

/*! \brief      Get the minimum allowed priority for the scheduling policy of C-style attributes
    \param  pa  C-style attributes
    \return     minimum allowed priority for the scheduling policy of <i>pa</i>
*/
const int attribute_min_priority(const pthread_attr_t& pa);

/*! \brief      Get the priority of C-style attributes
    \param  pa  C-style attributes
    \return     the priority associated with <i>pa</i>
*/
const int attribute_priority(const pthread_attr_t& pa);

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
    { T* tp { (T*)pthread_getspecific(_key) };

      if (tp)
        return tp;

      return nullptr;
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
  std::string               _name { "NONE"s };  ///< name of mutex
  pthread_t                 _thread_id;         ///< ID of the thread that owns the locked mutex
  thread_specific_data<int> _tsd_refcount;      ///< reference counter for recursive locking

public:

/// constructor
  inline pt_mutex(void)
    { pthread_mutex_init(&_mutex, NULL); }
    
  inline explicit pt_mutex(const std::string& nm) :
    _name(nm)
  { pthread_mutex_init(&_mutex, NULL); }
    
/// forbid copying
  pt_mutex(const pt_mutex&) = delete;

/// destructor
  virtual ~pt_mutex(void);

/// lock the mutex
  void lock(void);

/// unlock the mutex
  void unlock(void);

/// get the thread ID
  inline const pthread_t thread_id(void) const
    { return _thread_id; }
    
  inline const std::string name(void) const
    { return _name; }

  friend class pt_condition_variable;       ///< needs access to details of the mutex
};

// --------------------------------------------  pt_mutex_attributes  ----------------------------------

/*! \class  pt_mutex_attributes
    \brief  Encapsulate a pthread_mutexattr
*/

class pt_mutex_attributes
{
protected:

  pthread_mutexattr_t         _mutexattr;         ///< Encapsulated mutexsttr object

public:

/// constructor
  inline pt_mutex_attributes(void)
    { pthread_mutexattr_init(&_mutexattr); }

/// destructor
  inline virtual ~pt_mutex_attributes(void)
    { pthread_mutexattr_destroy(&_mutexattr); }

  const int priority_ceiling(void) const;

  void priority_ceiling(const int pc);

  const int protocol(void) const;
  
  const std::string protocol_name(void) const;

  void protocol(const int pc);

  const int type(void) const;
  
  const std::string type_name(void) const;

  void type(const int ty);
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

public:

/*! \brief  Default Constructor
*/
  pt_condition_variable(void);

/// forbid copying
  pt_condition_variable(const pt_condition_variable&) = delete;

/*! \brief          Construct and associate a mutex with the condition variable
    \param  mtx     mutex to be associated with the condition variable
*/
  explicit pt_condition_variable(pt_mutex& mtx);
   
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

public:

/*! \brief          Construct from a named mutex
    \param  ptm     mutex to be locked
    \param  name    name of mutex
*/
  safelock(pt_mutex& ptm, const std::string& name);

/// forbid copying
  safelock(const safelock&) = delete;

/// forbid setting equal
  void operator=(const safelock&) = delete;

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
  inline virtual ~pthread_error_messages(void) = default;

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

/*! \brief      Make an explicit safelock from a mutex
    \param  m   the mutex to be locked
    \param  v   object to be returned after the call to the lock (and implied unlock, in the destructor)
    \return     <i>v</i>, following a lock and unlock
*/
template <class T>
const T SAFELOCK_GET(pt_mutex& m, const T& v)
{ safelock safelock_z(m, "SAFELOCK_GET"s);

  return v;
}

/*! \brief          Make an explicit safelock from a mutex, then set a value
    \param  m       the mutex to be locked
    \param  var     variable to be set
    \param  val     value to which <i>var</i> is to be set following the lock
*/
template <class T>
void SAFELOCK_SET(pt_mutex& m, T& var, const T& val)
{ safelock safelock_z(m, "SAFELOCK_SET"s);

  var = val;
}

/*! \brief                  Wrapper for pthread_create()
    \param  thread          pointer to thread ID
    \param  attr            pointer to pthread attributes
    \param  start_routine   name of function to run in the new thread
    \param  arg             arguments to be passed to <i>start_function</i>
    \param  thread_name     name of the thread

    The first four parameters are passed without change to <i>pthread_create</i>
*/
void create_thread(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg, const std::string& thread_name = std::string());

/*! \brief                  Wrapper for pthread_create()
    \param  thread          pointer to thread ID
    \param  t_attr          thread attributes
    \param  start_routine   name of function to run in the new thread
    \param  arg             arguments to be passed to <i>start_function</i>
    \param  thread_name     name of the thread

    The first four parameters are passed without change to <i>pthread_create</i>
*/
inline void create_thread(pthread_t *thread, const thread_attribute& t_attr, void *(*start_routine) (void *), void *arg, const std::string& thread_name = std::string())
  { create_thread(thread, &(t_attr.attr()), start_routine, arg, thread_name); }

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(pthread_error);     ///< errors related to pthread processing

#endif    // PTHREADSUPPORT_H
