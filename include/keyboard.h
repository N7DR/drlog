// $Id: keyboard.h 203 2022-03-28 22:08:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   keyboard.h

    Classes and functions related to obtaining and processing keyboard input
*/

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "log_message.h"
#include "macros.h"
#include "pthread_support.h"
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

extern message_stream ost;                  ///< for debugging, info

/// key events
enum class KEY_EVENT { PRESS,
                       RELEASE
                     };

using key_code = unsigned int;                                          ///< syntactic sugar

// helper functions for KeySyms

/*! \brief      Test whether a KeySym is an upper-case letter
    \param  ks  the KeySym to test
    \return     whether <i>ks</i> is an upper-case letter
*/
inline bool is_upper_case_letter(const KeySym ks)
  { return ((ks >= XK_A) and (ks <= XK_Z)); }

/*! \brief      Test whether a KeySym is a lower-case letter
    \param  ks  the KeySym to test
    \return     whether <i>ks</i> is a lower-case letter
*/
inline bool is_lower_case_letter(const KeySym ks)
  { return ((ks >= XK_a) and (ks <= XK_z)); }

/*! \brief      Test whether a KeySym is a letter
    \param  ks  the KeySym to test
    \return     whether <i>ks</i> is a letter
*/
inline bool is_letter(const KeySym ks)
  { return (is_upper_case_letter(ks) or is_lower_case_letter(ks)); }

/// is a KeySym a digit? (Cannot name this function is_digit, becasue that is already in use.)
inline bool symbol_is_digit(const KeySym ks)
  { return ((ks >= XK_0) and (ks <= XK_9)); }

// -------------------------------------------------  keyboard_event  -----------------------------------

/*! \class  keyboard_event
    \brief  encapsulate an event from the keyboard
*/

class keyboard_event
{
protected:

  key_code       _code;         ///< code for the relevant key
  KEY_EVENT      _event;        ///< the event
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
  inline bool is_shifted(void) const
    { return (_xkey_state bitand ShiftMask); }

/// is one of the control keys pressed?
  inline bool is_control(void) const
    { return (_xkey_state bitand ControlMask); }

/// is one of the control keys pressed?
  inline bool is_ctrl(void) const
    { return (is_control()); }

/// is one of the alt keys pressed?
  inline bool is_alt(void) const
    { return (_xkey_state bitand Mod1Mask); }

/// is one of the control keys, but not one of the alt keys, pressed?
  inline bool is_control_and_not_alt(void) const
    { return ( is_control() and !is_alt() ); }

/// is one of the control keys, but not one of the alt keys, pressed?
  inline bool is_ctrl_and_not_alt(void) const
    { return is_control_and_not_alt(); }

/// is one of the alt keys, but not one of the control keys, pressed?
  inline bool is_alt_and_not_control(void) const
    { return ( is_alt() and !is_control() ); }

/// is one of the alt keys, but not one of the control keys, pressed?
  inline bool is_alt_and_not_ctrl(void) const
    { return is_alt_and_not_control(); }

/// are control and alt keys both pressed?
  inline bool is_alt_and_control(void) const
    { return is_alt() and is_control(); }

/// are control and alt keys both pressed?
  inline bool is_alt_and_ctrl(void) const
    { return is_alt_and_control(); }

/// is the key unmodified? NB: Numlock is Mod2Mask (see xmodmap command), so we can't merely test _xkey_state against zero
  inline bool is_unmodified(void) const
    { return ( (_xkey_state bitand (ShiftMask bitor ControlMask bitor Mod1Mask) ) == 0  ); }

/// is the key modified?
  inline bool is_modified(void) const
    { return !is_unmodified(); }

/// is the key an upper case letter?
  inline bool is_upper_case_letter(void) const
    { return (is_unmodified() and ::is_upper_case_letter(_symbol)); }

/// is the key a lower case letter?
  inline bool is_lower_case_letter(void) const
    { return (is_unmodified() and ::is_lower_case_letter(_symbol)); }

/// is the key a letter?
  inline bool is_letter(void) const
    { return (is_unmodified() and ::is_letter(_symbol)); }

/// is the key a digit?
  inline bool is_digit(void) const
    { return (is_unmodified() and symbol_is_digit(_symbol)); }

/// does a character match the value of <i>_str</i>?
  inline bool is_char(const char c) const
     { return (is_unmodified() and (_str == create_string(c))); }

/// does a character number match the value of <i>_str</i>?
  inline bool is_char(const int n) const
     { return (is_char(static_cast<char>(n))); }

/// is a character a control character version of the character in <i>_str</i>?
  bool is_control(const char c) const;

/// is a character an alt version of the character in <i>_str</i>?
  inline bool is_alt(const char c) const
    { return (is_alt() and (_str == create_string(c))); }
};

// -------------------------------------  keyboard_queue  ---------------------------

/*! \class  keyboard_queue
    \brief  The basic queue of keyboard events, which is just a wrapper around an STL deque
*/

class keyboard_queue
{
protected:

  Display*                    _display_p       { nullptr };             ///< the X display pointer
  std::deque<keyboard_event>  _events          { };                     ///< the actual queue
  keyboard_event              _last_event      { };                     ///< the event most recently removed from the queue
  Window                      _window_id       { 0 };                   ///< the X window ID
  bool                        _x_multithreaded { true };                ///< do we permit multiple threads in X?

  mutable pt_mutex _keyboard_mutex             { "keyboard queue" };    ///< mutex to keep the object thread-safe

/*! \brief                  X error handler
    \param  display_p       pointer to X display
    \param  error_event_p   pointer to X error event
    \return                 ignored value (see man XSetErrorHandler)

    Although ignored, the return type has to match the type documented for the parameter to XSetErrorHandler()
*/
  static int _x_error_handler(Display* display_p, XErrorEvent* error_event_p);

/*! \brief                  X I/O error handler
    \param  display_p       pointer to X display
    \return                 ignored value (see man XSetIOErrorHandler)

    Although ignored, the return type has to match the type documented for the parameter to XSetErrorHandler()
*/
  static int _x_io_error_handler(Display* display_p);

public:

/// default constructor
  keyboard_queue(void);

/// destructor
  ~keyboard_queue(void) = default;

  SAFEREAD_WITH_INTERNAL_MUTEX(display_p, _keyboard);
  SAFEREAD_WITH_INTERNAL_MUTEX(window_id, _keyboard);
  SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX(x_multithreaded, _keyboard);

/// how many events are in the queue?
  inline size_t size(void) const
    { SAFELOCK(_keyboard);
      return _events.size();
    }

/// is the queue empty?
  bool empty(void) const;

/// move any pending X keyboard events to the queue
  void process_events(void);

/*! \brief      What event is at the front of the queue?
    \return     the event at the front of the queue

    Does not remove the event from the queue.
    Returns the default keyboad_event() if the queue is empty.
*/
  keyboard_event peek(void);

/// pop the frontmost event
  keyboard_event pop(void);

/// get the event most recently popped
  inline keyboard_event last(void)
    { SAFELOCK(_keyboard);
      return _last_event;
    }

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
