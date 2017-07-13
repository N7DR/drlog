// $Id: keyboard.h 129 2016-09-29 21:13:34Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file keyboard.h

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

//#include <X11/keysymdef.h>    // /usr/include/X11/keysymdef.h; leave this comment so it is easy to find the keysymdef file
#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern const std::map<std::string, int>         key_names;              ///< The names of the keys on the keyboard; maps names to X KeySyms
extern const std::map<std::string, std::string> equivalent_key_names;   ///< names that are equivalent
extern const std::unordered_set<KeySym>         keypad_numbers;         ///< the keypad numbers and their equivalents

enum key_event_type { KEY_PRESS,
                      KEY_RELEASE
                    };                                                  ///< key events

typedef unsigned int key_code;                                          ///< syntactic sugar

// helper functions for KeySyms

/// is a KeySym an upper-case letter?
inline const bool is_upper_case_letter(const KeySym ks)
  { return ((ks >= XK_A) and (ks <= XK_Z)); }

/// is a KeySym a lower-case letter?
inline const bool is_lower_case_letter(const KeySym ks)
  { return ((ks >= XK_a) and (ks <= XK_z)); }

/// is a KeySym a letter?
inline const bool is_letter(const KeySym ks)
  { return (is_upper_case_letter(ks) or is_lower_case_letter(ks)); }

/// is a KeySym a digit? (Cannot name this function is_digit, becasue that is already in use.)
inline const bool symbol_is_digit(const KeySym ks)
  { return ((ks >= XK_0) and (ks <= XK_9)); }

// -------------------------------------------------  keyboard_event  -----------------------------------

/*! \class  keyboard_event
    \brief  encapsulate an event from the keyboard
*/

class keyboard_event
{
protected:
  key_code       _code;         ///< code for the relevant key
  key_event_type _event;        ///< the event
  std::string    _str;          ///< string version of the character
  KeySym         _symbol;       ///< symbol that corresponds to the key
  unsigned int   _xkey_state;   ///< the XKeyEvent.state value
  Time           _xkey_time;    ///< the XKeyEvent.time value

public:

  READ_AND_WRITE(code);         ///< code for the relevant key
  READ_AND_WRITE(event);        ///< the event
  READ_AND_WRITE(str);          ///< string version of the character
  READ_AND_WRITE(symbol);       ///< symbol that corresponds to the key
  READ_AND_WRITE(xkey_state);   ///< the XKeyEvent.state value
  READ_AND_WRITE(xkey_time);    ///< the XKeyEvent.time value

// return values related to the state. These are the values immediately
// PRIOR to the event: http://www.tronche.com/gui/x/xlib/events/keyboard-pointer/keyboard-pointer.html#XKeyEvent

/// is one of the shift keys pressed?
  inline const bool is_shifted(void) const
    { return (_xkey_state bitand ShiftMask); }

/// is one of the control keys pressed?
  inline const bool is_control(void) const
    { return (_xkey_state bitand ControlMask); }

/// is one of the control keys pressed?
  inline const bool is_ctrl(void) const
    { return (is_control()); }

/// is one of the alt keys pressed?
  inline const bool is_alt(void) const
    { return (_xkey_state bitand Mod1Mask); }

/// is one of the control keys, but not one of the alt keys, pressed?
  inline const bool is_control_and_not_alt(void) const
    { return ( is_control() and !is_alt() ); }

/// is one of the control keys, but not one of the alt keys, pressed?
  inline const bool is_ctrl_and_not_alt(void) const
    { return is_control_and_not_alt(); }

/// is one of the alt keys, but not one of the control keys, pressed?
  inline const bool is_alt_and_not_control(void) const
    { return ( is_alt() and !is_control() ); }

/// is one of the alt keys, but not one of the control keys, pressed?
  inline const bool is_alt_and_not_ctrl(void) const
    { return is_alt_and_not_control(); }

/// are control and alt keys both pressed?
  inline const bool is_alt_and_control(void) const
    { return is_alt() and is_control(); }

/// are control and alt keys both pressed?
  inline const bool is_alt_and_ctrl(void) const
    { return is_alt_and_control(); }

/// is the key unmodified?
// NB: Numlock is Mod2Mask (see xmodmap command), so we can't merely test _xkey_state against zero
  inline const bool is_unmodified(void) const
    { return ( (_xkey_state bitand (ShiftMask bitor ControlMask bitor Mod1Mask) ) == 0  ); }

/// is the key modified?
  inline const bool is_modified(void) const
    { return !is_unmodified(); }

/// is the key an upper case letter?
  inline const bool is_upper_case_letter(void) const
    { return (is_unmodified() and ::is_upper_case_letter(_symbol)); }

/// is the key a lower case letter?
  inline const bool is_lower_case_letter(void) const
    { return (is_unmodified() and ::is_lower_case_letter(_symbol)); }

/// is the key a letter?
  inline const bool is_letter(void) const
    { return (is_unmodified() and ::is_letter(_symbol)); }

/// is the key a digit?
  inline const bool is_digit(void) const
    { return (is_unmodified() and symbol_is_digit(_symbol)); }

/// does a character match the value of <i>_str</i>?
  inline const bool is_char(const char c) const
     { return (is_unmodified() and (_str == create_string(c))); }

/// does a character number match the value of <i>_str</i>?
  inline const bool is_char(const int n) const
     { return (is_char(static_cast<char>(n))); }

/// is a character a control character version of the character in <i>_str</i>?
  inline const bool is_control(const char c) const
    { return (is_control() and (_str == create_string(c - 'a' + 1))); }

/// is a character an alt version of the character in <i>_str</i>?
  inline const bool is_alt(const char c) const
    { return (is_alt() and (_str == create_string(c))); }
};

// -------------------------------------  keyboard_queue  ---------------------------

/*! \class  keyboard_queue
    \brief  The basic queue of keyboard events, which is just a wrapper around an STL deque
*/

class keyboard_queue
{
protected:
  Display*                    _display_p;           ///< the X display pointer
  std::deque<keyboard_event>  _events;              ///< the actual queue
  keyboard_event              _last_event;          ///< the event most recently removed from the queue
  Window                      _window_id;           ///< the X window ID
  bool                        _x_multithreaded;     ///< do we permit multiple threads in X?

  pt_mutex _keyboard_mutex;                         ///< mutex to keep the object thread-safe

/*! \brief                  X error handler
    \param  display_p       pointer to X display
    \param  error_event_p   pointer to X error event
    \return                 ignored value (see man XSetErrorHandler)
*/
  static int _x_error_handler(Display* display_p, XErrorEvent* error_event_p);

public:

/// default constructor
  keyboard_queue(void);

/// destructor
  virtual ~keyboard_queue(void)
  { }

  READ(display_p);                      ///< the X display pointer
  READ(window_id);                      ///< the X window ID
  READ_AND_WRITE(x_multithreaded);      ///< do we permit multiple threads in X?

/// how many events are in the queue?
  const size_t size(void);

/// is the queue empty?
  const bool empty(void);

/// move any pending X keyboard events to the queue
  void process_events(void);

/*! \brief  What event is at the front of the queue?

    Returns the default keyboad_event() if the queue is empty
*/
  const keyboard_event peek(void);

/// pop the frontmost event
  const keyboard_event pop(void);

/// get the event most recently popped
  const keyboard_event last(void);

/*! \brief      Emulate the pressing of a character key
    \param  c   pressed character
*/
  void push_key_press(const char c);

/*! \brief      Emulate the addition of a KeySym
    \param  ks  KeySym to add
*/
  void push_key_press(const KeySym ks);

/*! \brief              Emulate the pressing of a sequence of characters
    \param  str         pressed string
    \param  ms_delay    delay in milliseconds between each character in <i>str</i>
*/
  void push_key_press(const std::string& str, const int ms_delay = 0);
};

# endif    // KEYBOARD_H
