// $Id: cw_buffer.h 68 2014-06-28 15:42:35Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file cwbuffer.h

        Classes and functions related to sending CW
*/

#ifndef CWBUFFER_H
#define CWBUFFER_H

#include "parallel_port.h"
#include "pthread_support.h"
#include "rig_interface.h"

#include <queue>
#include <string>

// ---------------------------------------- cw_buffer -------------------------

/*!     \class cw_buffer
        \brief A buffer for sending CW
*/

class cw_buffer
{
protected:

  parallel_port    _port;               ///< the associated parallel port

/*  positive numbers represent key down
 *  negative numbers represent key up
 *  zero represents the start of an embedded command
 *
 *  the duration of key up/down is in units in which 100 == the standard length of a dot
*/
  std::queue<int>   _key_buffer;       ///< the queue of key up/down motions remaining to be executed
  pt_mutex          _key_buffer_mutex; ///< mutex to allow thread-safe access to <i>_key_buffer</i>

  bool              _aborted;          ///< have we received an "abort" command?
  pt_mutex          _abort_mutex;      ///< mutex to allow thread-safe execution of an "abort" command

  bool              _disabled_cw;      ///< whether actual sending is disabled

  unsigned int      _wpm;              ///< keyer speed in WPM
  unsigned int      _usec;             ///< dot length in microseconds
  pt_mutex          _speed_mutex;      ///< mutex for reading/writing speed and ptt delay

  unsigned int      _ptt_delay;        ///< delay between asserting PTT and transmitting the start of a character, in milliseconds

  pthread_t             _thread_id;                 ///< ID for the thread that plays the buffer
  static void*          _static_play(void* arg);    ///< pointer to static function to play the buffer
  void*                 _play(void*);               ///< pointer to object function to play the buffer
  pt_condition_variable _condvar;                   ///< condvar associated with the play thread
  pt_mutex              _condvar_mutex;             ///< mutex associated with the condvar

  rig_interface*        _rigp;                      ///< associated rig

/*!     \brief      Add an action to the key buffer
        \param  n   coded action

        positive values of <i>n</i> represent key down;
        negative values represent key up;
        zero represents the start of an embedded command
*/
  void _add_action(const int n);

public:

/*!     \brief          Construct on a parallel port
        \param  filename    device file
        \param  delay       PTT delay (in milliseconds)
        \param  speed       Speed (in WPM)
*/
  cw_buffer(const std::string& filename, const unsigned int delay, const unsigned int speed);

/// destructor
  ~cw_buffer(void);

// set the speed in wpm
  void speed(const unsigned int wpm);

// get the speed in wpm
  const unsigned int speed(void);

// set the PTT delay in msec
  void ptt_delay(const unsigned int msec);

// get the PTT delay in msec
  const unsigned int ptt_delay(void);

// add a key-down interval (100 = 1 dot), along with a subsequent gap
  void key_down(const int n, const int space = 100);

// add a key-up interval (no subsequent gap) 100 = 1 dot
  void key_up(const int n);

// send a single character
  void add(const char c, const int character_space = 200);

// send a string
  void operator<<(const std::string& str);

// abort sending
  void abort(void);

// clear the buffer
  void clear(void);

// associate a rig with the buffer
  void associate_rig(rig_interface* rigp);

// is the buffer empty?
  const bool empty(void);

// disable sending
  inline void disable(void)
    { _disabled_cw = true; }

// enable sending
  inline void enable(void)
    { _disabled_cw = false; }

// toggle sending
  inline void toggle(void)
    { _disabled_cw = !_disabled_cw; }

// is sending disabled?
  inline const bool disabled(void) const
    { return _disabled_cw; }

// is sending enabled?
  inline const bool enabled(void) const
    { return !disabled(); }
};

// ---------------------------------------- cw_messages -------------------------

/*!     \class cw_messages
        \brief A class for managing CW messages
*/

class cw_messages
{
protected:
  std::map<int, std::string > _messages;          ///< sparse vector to hold the messages
  pt_mutex                    _messages_mutex;    ///< mutex to allow for thread-safe access

public:

  explicit cw_messages(const std::map<int, std::string >& m) :
    _messages(m)
  { }

  cw_messages(void)
  { }

  const std::string operator[](const int n);
};


#endif /* CWBUFFER_H */
