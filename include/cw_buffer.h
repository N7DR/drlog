// $Id: cw_buffer.h 164 2020-08-16 19:57:42Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   cw_buffer.h

    Classes and functions related to sending CW

    If real-time scheduling is desired/needed, then it is best to set:
      ulimit -Sr unlimited

    and have:
      <user>            hard    rtprio          unlimited
    in /etc/security/limits.conf
*/

#ifndef CWBUFFER_H
#define CWBUFFER_H

#include "parallel_port.h"
#include "pthread_support.h"
#include "rig_interface.h"

#include <queue>
#include <string>

// ---------------------------------------- cw_buffer -------------------------

/*! \class  cw_buffer
    \brief  A buffer for sending CW
*/

class cw_buffer
{
protected:

  bool                  _aborted;           ///< have we received an "abort" command?
  pt_mutex              _abort_mutex { "CW ABORT"s };       ///< mutex to allow thread-safe execution of an "abort" command
  pt_condition_variable _condvar;           ///< condvar associated with the play thread
  pt_mutex              _condvar_mutex { "CW CONDVAR"s };     ///< mutex associated with the condvar
  bool                  _disabled_cw;       ///< whether actual sending is disabled

/*  positive numbers represent key down
    negative numbers represent key up
    zero represents the start of an embedded command

    the duration of key up/down is in units in which 100 == the standard length of a dot
*/
  std::queue<int>       _key_buffer;        ///< the queue of key up/down motions remaining to be executed
  pt_mutex              _key_buffer_mutex { "CW KEY BUFFER"s };  ///< mutex to allow thread-safe access to <i>_key_buffer</i>
  parallel_port         _port;              ///< the associated parallel port
  unsigned int          _ptt_delay;         ///< delay between asserting PTT and transmitting the start of a character, in milliseconds
  rig_interface*        _rigp;              ///< associated rig
  pt_mutex              _speed_mutex { "CW SPEED"s };       ///< mutex for reading/writing speed and ptt delay
  pthread_t             _thread_id;         ///< ID for the thread that plays the buffer
  unsigned int          _usec;              ///< dot length in microseconds
  unsigned int          _wpm;               ///< keyer speed in WPM

/*! \brief      Add an action to the key buffer
    \param  n   coded action

    positive values of <i>n</i> represent key down;
    negative values represent key up;
    zero represents the start of an embedded command
*/
  void _add_action(const int n);

/*! \brief      Play the buffer
    \return     nullptr

    The parameter is unused
*/
  void* _play(void*);

/*! \brief          Wrapper function to play the buffer
    \param  arg     "this" pointer
    \return         nullptr
*/
  static void* _static_play(void* arg);    ///< pointer to static function to play the buffer

public:

/*! \brief                  Construct on a parallel port
    \param  filename        device file
    \param  delay           PTT delay (in milliseconds)
    \param  wpm_speed       speed in WPM
    \param  cw_priority     priority of the thread that sends CW
*/
  cw_buffer(const std::string& filename, const unsigned int delay, const unsigned int wpm_speed, const int cw_priority);

/// no copy constructor
  cw_buffer(const cw_buffer& cwb) = delete;

/// destructor
  /* virtual */ ~cw_buffer(void);

/*! \brief          Set the speed
    \param  wpm     speed in WPM
*/
  void speed(const unsigned int wpm);

/// get the speed in wpm
  unsigned int speed(void);

/*! \brief          Set the PTT delay
    \param  msec    delay in milliseconds
*/
  void ptt_delay(const unsigned int msec);

/// get the PTT delay in milliseconds
  unsigned int ptt_delay(void);

/*! \brief          Add a key-down interval, along with a subsequent gap
    \param  n       key-down interval (100 = 1 dot)
    \param  space   terminating key-up interval (100 = 1 dot)
*/
  void key_down(const int n, const int space = 100);

/*! \brief      Add a key-up interval, with no subsequent gap
    \param  n   key-up interval (100 = 1 dot)
*/
  void key_up(const int n);

/*! \brief                      Send a single character, along with a subsequent gap
    \param  c                   character to send
    \param  character_space     additional terminating key-up interval (100 = 1 dot)

    There is always a 100-unit space appended to the character, in addition to
    the interval defined by <i>character_space</i>
*/
  void add(const char c, const int character_space = 200);

/*! \brief          Send a string
    \param  str     string to send

    Special characters and commands embedded in <i>str</i> are expanded and/or processed
    prior to transmission
*/
  void operator<<(const std::string& str);

/// clear the buffer
  void clear(void);

/*! \brief  Abort sending

    Sending halts (essentially) immediately (even mid-character)
*/
  inline void abort(void)
    { clear(); }

/*! \brief          Associate a rig with the buffer
    \param  rigp    pointer to rig interface to be associated with the buffer
*/
  void associate_rig(rig_interface* rigp);

/// is the buffer empty?
  bool empty(void);

/// disable sending
  inline void disable(void)
    { _disabled_cw = true; }

/// enable sending
  inline void enable(void)
    { _disabled_cw = false; }

/// toggle sending
  inline void toggle(void)
    { _disabled_cw = !_disabled_cw; }

/// is sending disabled?
  inline bool disabled(void) const
    { return _disabled_cw; }

/// is sending enabled?
  inline bool enabled(void) const
    { return !disabled(); }

/// assert PTT
  inline void assert_ptt(void)
    { _port.control(_ptt_delay ? C1284_NINIT : 0);  }            // key up; PTT asserted

/// clear (i.e., de-assert) PTT
  inline void clear_ptt(void)
    { _port.control(0);  }
};

// ---------------------------------------- cw_messages -------------------------

/*! \class  cw_messages
    \brief  A class for managing CW messages
*/

class cw_messages
{
protected:

  std::map<int, std::string > _messages;          ///< sparse vector to hold the messages
  pt_mutex                    _messages_mutex { "CW MESSAGES"s };    ///< mutex to allow for thread-safe access

public:

/*! \brief      Constructor
    \param  m   map of message numbers and message contents
*/
  inline explicit cw_messages(const std::map<int /* message number */, std::string >& m) :
    _messages(m)
    { }

/// default constructor
  inline cw_messages(void) = default;

/*! \brief      Get a particular CW message
    \param  n   number of message to return
    \return     CW message number <i>n</i>

    Returns empty string if message number <i>n</i> does not exist
*/
  std::string operator[](const int n);
};

#endif /* CWBUFFER_H */
