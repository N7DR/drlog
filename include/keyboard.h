// $Id: keyboard.h 27 2013-06-30 22:19:55Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file keyboard.h

        Classes and functions related to obtaining and processing keyboard input
*/

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "pthread_support.h"
#include "macros.h"
#include "string_functions.h"

#include <deque>
#include <string>
#include <unordered_set>

//#include <X11/keysymdef.h>    // /usr/include/X11/keysymdef.h
#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern const std::map<std::string, int>         key_names;              ///< The names of the keys on the keyboard; maps names to X KeySyms
extern const std::map<std::string, std::string> equivalent_key_names;   ///< names that are equivalent
extern const std::unordered_set<KeySym>         keypad_numbers;         ///< the keypad numbers and their equivalents

enum key_event_type { KEY_PRESS,
                      KEY_RELEASE
                    };

typedef unsigned int key_code;

// helper functions for KeySyms

inline const bool is_upper_case_letter(const KeySym ks)
  { return ((ks >= XK_A) and (ks <= XK_Z)); }

inline const bool is_lower_case_letter(const KeySym ks)
  { return ((ks >= XK_a) and (ks <= XK_z)); }

inline const bool is_letter(const KeySym ks)
  { return (is_upper_case_letter(ks) or is_lower_case_letter(ks)); }

inline const bool symbol_is_digit(const KeySym ks)
  { return ((ks >= XK_0) and (ks <= XK_9)); }

class keyboard_event
{
protected:
  key_event_type _event;
  key_code       _code;
  KeySym         _symbol;
  std::string    _str;
  unsigned int   _xkey_state;    // the XKeyEvent.state value
  Time           _xkey_time;     // the XKeyEvent.time value

public:

  READ_AND_WRITE(event);
  READ_AND_WRITE(code);
  READ_AND_WRITE(symbol);
  READ_AND_WRITE(str);
  READ_AND_WRITE(xkey_state);
  READ_AND_WRITE(xkey_time);

// return values related to the state. These are the values immediately
// PRIOR to the event: http://www.tronche.com/gui/x/xlib/events/keyboard-pointer/keyboard-pointer.html#XKeyEvent

  inline const bool is_shifted(void) const
    { return (_xkey_state bitand ShiftMask); }

  inline const bool is_control(void) const
    { return (_xkey_state bitand ControlMask); }

  inline const bool is_ctrl(void) const
    { return (is_control()); }

  inline const bool is_alt(void) const
    { return (_xkey_state bitand Mod1Mask); }

// Numlock is Mod2Mask (see xmodmap command), so we can't merely test _xkey_state against zero
  inline const bool is_unmodified(void) const
    { return ( (_xkey_state bitand (ShiftMask bitor ControlMask bitor Mod1Mask) ) == 0  ); }

  inline const bool is_modified(void) const
    { return !is_unmodified(); }

  inline const bool is_upper_case_letter(void) const
    { return (is_unmodified() and ::is_upper_case_letter(_symbol)); }

  inline const bool is_lower_case_letter(void) const
    { return (is_unmodified() and ::is_lower_case_letter(_symbol)); }

  inline const bool is_letter(void) const
    { return (is_unmodified() and ::is_letter(_symbol)); }

  inline const bool is_digit(void) const
    { return (is_unmodified() and symbol_is_digit(_symbol)); }

  inline const bool is_char(const char c) const
     { return (is_unmodified() and (_str == create_string(c))); }

  inline const bool is_char(const int n) const
     { return (is_char(static_cast<char>(n))); }

  inline const bool is_control(const char c) const
    { return (is_control() and (_str == create_string(c - 'a' + 1))); }

  inline const bool is_alt(const char c) const
    { return (is_alt() and (_str == create_string(c))); }
};


// -------------------------------------  keyboard_queue  ---------------------------

/*!     \class keyboard_queue
        \brief The basic queue of keyboard events, which is just a wrapper
        around an STL deque
*/

class keyboard_queue
{
protected:
  std::deque<keyboard_event>  _events;      // the actual queue

  Display*                    _display_p;   // the X display pointer
  Window                      _window_id;   // the X window ID

  keyboard_event              _last_event;  // the event most recently removed from the queue

  bool                        _x_multithreaded;    // do we want to permit multiple threads in X?

  pt_mutex _keyboard_mutex;

// X error handler
  static int _x_error_handler(Display* display_p, XErrorEvent* error_event_p);

public:

// default constructor
  keyboard_queue(void);

// destructor
  virtual ~keyboard_queue(void)
  { }

  READ_AND_WRITE(x_multithreaded);

  const size_t size(void);

  const bool empty(void);

// place keyboard events in the queue
  void process_events(void);

// peek at the front of the queue
  const keyboard_event peek(void);

// pop the frontmost event
  const keyboard_event pop(void);

// get the most-recently removed event
  const keyboard_event last(void);

  inline Display* display_p(void) const
    { return _display_p; }

  inline const Window window_id(void) const
    { return _window_id; }

  void push_key_press(const char c);

  void push_key_press(const KeySym ks);

  void push_key_press(const std::string& str, const int ms_delay = 0);
};

# endif    // KEYBOARD_H
