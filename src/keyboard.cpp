// $Id: keyboard.cpp 153 2019-09-01 14:27:02Z  $

/*! \file 	keyboard.cpp

    Classes and functions related to obtaining and processing keyboard input
*/

#include "keyboard.h"
#include "log_message.h"
#include "string_functions.h"

#include <chrono>
#include <iostream>
#include <map>
#include <thread>

#include <X11/Xutil.h>

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread

extern message_stream           ost;    ///< for debugging, info

// -------------------------------------------------  keyboard_event  -----------------------------------

/*! \class  keyboard_event
    \brief  encapsulate an event from the keyboard
*/

/// is a character a control character version of the character in <i>_str</i>?
// this is complicated because c might not be a letter
const bool keyboard_event::is_control(const char c) const
{ if (is_control())
  { //const int i = static_cast<int>(c);

    //ost << "i = " << i << std::endl;

    if ( (c >= 'a') and (c <= 'z') )  // if it's a letter
      return (_str == create_string(c - 'a' + 1));
    else
      return (_str == string(1, c));
  }
  else
    return false;
}

// -------------------------------------  keyboard_queue  ---------------------------

/*! \class  keyboard_queue
    \brief  The basic queue of keyboard events, which is just a wrapper
            around an STL deque
*/

/*! \brief                  X error handler
    \param  display_p       pointer to X display
    \param  error_event_p   pointer to X error event
    \return                 ignored value (see man XSetErrorHandler)

    Although ignored, the return type has to match the type documented for the parameter to XSetErrorHandler()
*/
int keyboard_queue::_x_error_handler(Display* display_p, XErrorEvent* error_event_p)
{ 
// http://www.rahul.net/kenton/xproto/xrequests.html
  if ( (static_cast<int>(error_event_p->error_code) == BadMatch) and (static_cast<int>(error_event_p->request_code) == 73) )
  { ost << "X BadMatch error in XGetImage() ignored" << endl;
      return 0;
  }
  
  ost << "X Error" << endl;

/* from man XErrorEvent:

       typedef struct {
               int type;
               Display *display;       // Display the event was read from
               XID resourceid;         // resource id
               unsigned long serial;           // serial number of failed request
               unsigned char error_code;       // error code of failed request
               unsigned char request_code;     // Major op-code of failed request
               unsigned char minor_code;       // Minor op-code of failed request
       } XErrorEvent;
       
      The serial member is the number of requests, starting from one, sent over the network connection since it was opened.  
      It is the number that was the value of NextRequest immediately before the failing call was made.  The
      request_code member is a protocol request of the procedure that failed, as defined in <X11/Xproto.h>.

*/

  ost << "XErrorEvent: " << endl
      << "  type         : " << error_event_p->type << endl
      << "  display ptr  : " << error_event_p->display << endl
      << "  resourceid   : " << error_event_p->resourceid << endl
      << "  serial       : " << error_event_p->serial << endl
      << "  error code   : " << static_cast<int>(error_event_p->error_code) << endl
      << "  request code : " << static_cast<int>(error_event_p->request_code) << endl
      << "  minor code   : " << static_cast<int>(error_event_p->minor_code) << endl;

    constexpr int BUF_SIZE { 4096 };

    char buf[BUF_SIZE];

    XGetErrorText(error_event_p->display, error_event_p->error_code, &buf[0], BUF_SIZE);
    
    ost << "Error text : " << buf << endl;

  sleep(2);
  exit(-1);
}

/*! \brief                  X I/O error handler
    \param  display_p       pointer to X display
    \return                 ignored value (see man XSetIOErrorHandler)

    Although ignored, the return type has to match the type documented for the parameter to XSetErrorHandler().
    The XSetIOErrorHandler man page says: If the I/O error handler does return, the client process exits.
*/
int keyboard_queue::_x_io_error_handler(Display* display_p)
{ ost << "X IO Error; terminating" << endl;

  sleep(2);
  exit(-1);
  
  return -1;
}

/*! \brief      Thread function to run the X event loop
    \param  vp  pointer to keyboard queue
    \return     nullptr
*/
void* runit(void* vp)
{ keyboard_queue* kqp { static_cast<keyboard_queue*>(vp) };

  kqp->process_events();

  return nullptr;
}

/// default constructor
keyboard_queue::keyboard_queue(void) :
  _display_p(nullptr),
  _window_id(0),
  _x_multithreaded(true)        // to be safe, until we're sure we aren't running the simulator
{
// write a message, pause, then exit
  auto delayed_exit = [] (const string& msg, const unsigned int delay_in_seconds)
  { cerr << msg << endl;
    sleep_for(seconds(delay_in_seconds));
    exit(-1);
  };

  Status status = XInitThreads();
  
  if (status == 0)
    delayed_exit("ERROR returned from XInitThreads; aborting"s, 2);

// set the error handlers for X
  XSetErrorHandler(_x_error_handler);
  XSetIOErrorHandler(_x_io_error_handler);

  _display_p = XOpenDisplay(NULL);

  if (!_display_p)
    delayed_exit("Fatal error: unable to open display"s, 2);

// get the window ID
  const char* cp { getenv("WINDOWID") };
  
  if (!cp)
    delayed_exit("Fatal error: unable to obtain window ID"s, 2);

  _window_id = from_string<Window>(cp);

// we want to be the only process to have access to key presses in our window

  XLockDisplay(_display_p);     // overkill, but do it anyway

  int istatus = XGrabKey(_display_p, AnyKey, AnyModifier, _window_id, false, GrabModeAsync, GrabModeAsync);
  
  ost << "Returned from XGrabKey: " << istatus << endl;

// we are interested only in key events
  istatus = XSelectInput(_display_p, _window_id, KeyPressMask | KeyReleaseMask);

  XUnlockDisplay(_display_p);

  ost << "Returned from XSelectInput: " << istatus << endl;

  static pthread_t thread_id_1;

  try
  { create_thread(&thread_id_1, NULL, runit, this, "KEYBOARD"s);
  }

  catch (const pthread_error& e)
  { delayed_exit("Error creating thread: KEYBOARD"s, 2);
  }
}

/// move any pending X keyboard events to the queue
void keyboard_queue::process_events(void)
{ XEvent event;                    // the X event

  constexpr int BUF_SIZE { 256 };

  char buf[BUF_SIZE + 1];
  KeySym ks;

// I leave these comments here, in case I ever need to change this code.
// The problem outlined below appears to have been solved by forcing X to be multithreaded.

// if I take this out, then the events sent by XSendEvent() aren't seen by
// XWindowEvent() until after a real key has been pressed. I have no real idea why
// this is so -- nothing seems to suggest that XKeysymToKeycode()
// does anything other than simply perform the expected mapping.

// My best guess is that it's something to do with threading... indeed, from what I can
// tell, I'm supposed to lock the display /explicitly/ before a call to an X function that
// uses the display pointer... but I can't find any example of how that really works in
// practice: one can't wrap the call XWindowEvent() in such a lock because then the call blocks.

// Perhaps one is supposed to call XPending() first, and wrap that, and only call XWindowEvent()
// when we know there's an event waiting. But if there's no such event what does one do? One can hardly
// call usleep() for some arbitrary period and then retry the XPending() call: that would be a horribly
// inefficient polling mechanism, checking the queue many times per second.

// One wants a function that blocks and is guaranteed thread-safe, but xlib() doesn't seem to provide one.

// The XCB library is supposed to be robustly thread-safe... which would be great if it provided a useful
// replacement for XWindowEvent(); but apparently, from reading comments on the web, it does not (yet).

// So we're back to including the next line and hoping that it works.

  /* const KeyCode kc3 = */ //XKeysymToKeycode(_display_p, XK_g);

  while (1)
  {
// get the next event
    if (_x_multithreaded)           // this should always be true in drlog
    { bool got_event { false };

      while (!got_event)
      { XLockDisplay(_display_p);
      
        int pending { XPending(_display_p) };
//        XUnlockDisplay(_display_p);
        
        if (pending)
        { //XLockDisplay(_display_p);
          XWindowEvent(_display_p, _window_id, KeyPressMask | KeyReleaseMask, &event);
          XUnlockDisplay(_display_p);
          got_event = true;
        }
        else
        { XUnlockDisplay(_display_p);
          sleep_for(milliseconds(10));
        }
      }
    }
    else                            // this should never be true in drlog
      XWindowEvent(_display_p, _window_id, KeyPressMask | KeyReleaseMask, &event);

    if ((event.type == KeyPress) or (event.type == KeyRelease))    // should always be true
    { keyboard_event ke;

      ke.event(event.type == KeyPress ? KEY_PRESS : KEY_RELEASE);
      ke.code(event.xkey.keycode);
      ke.xkey_state(event.xkey.state);
      ke.xkey_time(event.xkey.time);

// get the related KeySym and string representation
      const int n_chars { XLookupString((XKeyEvent*)(&event), buf, BUF_SIZE, &ks, NULL) };

      buf[n_chars] = static_cast<char>(0);

      ke.symbol(ks);
      ke.str(string(buf));

// for drlog we are interested only in:
//   1. KeyPress
//   2. KeyRelease for the SHIFT keys
// KeySyms are defined in:      /usr/include/X11/keysymdef.h

      bool interesting_event { false };
      
      interesting_event |= (event.type == KeyPress);
      interesting_event |= (event.type == KeyRelease and ((ks == XK_Shift_L) or (ks == XK_Shift_R)));

      if (interesting_event)
      { SAFELOCK(_keyboard);

        _events.push_back(ke);
      }
    }
  }
}

/// how many events are in the queue?
const size_t keyboard_queue::size(void) const
{ SAFELOCK(_keyboard);

  return _events.size();
}

/// is the queue empty?
const bool keyboard_queue::empty(void) const
{ try
  { SAFELOCK(_keyboard);

    return _events.empty();
  }
  
  catch (const pthread_error& e)
  { ost << "Caught pthread exception in keyboard_queue::empty(); code = " << e.code() << "; reason = " << e.reason() << endl;
    throw;
  }
}

/*! \brief      What event is at the front of the queue?
    \return     the event at the front of the queue

    Does not remove the event from the queue.
    Returns the default keyboad_event() if the queue is empty.
*/
const keyboard_event keyboard_queue::peek(void)
{ SAFELOCK (_keyboard);

  return (!_events.empty() ? _events.front() : keyboard_event());
}

/// pop the frontmost event
const keyboard_event keyboard_queue::pop(void)
{ SAFELOCK(_keyboard);

  keyboard_event rv = peek();

  if (!_events.empty())
  { _events.pop_front();
    _last_event = rv;
  }

  return rv;
}

/// get the event most recently popped
const keyboard_event keyboard_queue::last(void)
{ SAFELOCK(_keyboard);

  return _last_event;
}

/*! \brief      Emulate the pressing of a character key
    \param  c   pressed character

    The corresponding Release event is not performed, since it is not processed by the drlog code
*/
void keyboard_queue::push_key_press(const char c)
{ switch (c)
  { case '/' :
      push_key_press(static_cast<KeySym>(XK_slash));    // the default algorithm doesn't work for the solidus
      break;

    default:
    { const string c_str = create_string(c);
      const KeySym ks = XStringToKeysym(c_str.c_str());

      push_key_press(ks);
    }
  }
}

/*! \brief      Emulate the addition of a KeySym
    \param  ks  KeySym to add

    The corresponding Release event is not performed, since it is not processed by the drlog code
*/
void keyboard_queue::push_key_press(const KeySym ks)
 { XEvent event;    // the X event

   event.type = KeyPress;

   if (_x_multithreaded)
     XLockDisplay(_display_p);

   const KeyCode kc = XKeysymToKeycode(_display_p, ks);

   event.xkey.keycode = kc;
   event.xkey.state = 0;      // no modifiers

   const Window window_id = _window_id;

//   Display* display_p = _display_p;

// TRY http://stackoverflow.com/questions/6560553/linux-x11-global-keyboard-hook

   event.xkey.display = _display_p;
   event.xkey.window = window_id;
   event.xkey.root = DefaultRootWindow(_display_p);
   event.xkey.subwindow = None;
   event.xkey.time = CurrentTime;
   event.xkey.x = 0;
   event.xkey.y = 0;
   event.xkey.x_root = 0;
   event.xkey.y_root = 0;
   event.xkey.state = NoEventMask;
   event.xkey.keycode = XKeysymToKeycode(_display_p, ks);
   event.xkey.same_screen = True;

 // TRY   http://www.doctort.org/adam/nerd-notes/x11-fake-keypress-event.html
   /* const int status = */ XSendEvent(_display_p, window_id, False, KeyPressMask, &event);
   XFlush(_display_p);

   if (_x_multithreaded)
     XUnlockDisplay(_display_p);
}

/*! \brief              Emulate the pressing of a sequence of characters
    \param  str         pressed string
    \param  ms_delay    delay in milliseconds between each character in <i>str</i>
*/
void keyboard_queue::push_key_press(const string& str, const int ms_delay)
{ for (size_t n = 0; n < str.length(); ++n)
  { push_key_press(str[n]);

    if (n != str.length() - 1)
      sleep_for(milliseconds(ms_delay));
  }
}

/*! \brief  Map key names in the config file to KeySym numbers (which are integers)

    This is used to decode access to the correct CW messages when a key is pressed
    See the file drlog_context.cpp to see this in use
*/
const map<string, int> key_names = { { "kp_0",      XK_KP_0 },
                                     { "kp_1",      XK_KP_1 },
                                     { "kp_2",      XK_KP_2 },
                                     { "kp_3",      XK_KP_3 },
                                     { "kp_4",      XK_KP_4 },
                                     { "kp_5",      XK_KP_5 },
                                     { "kp_6",      XK_KP_6 },
                                     { "kp_7",      XK_KP_7 },
                                     { "kp_8",      XK_KP_8 },
                                     { "kp_9",      XK_KP_9 },
                                     { "kp_insert", XK_KP_Insert },
                                     { "kp_end",    XK_KP_End },
                                     { "kp_down",   XK_KP_Down },
                                     { "kp_next",   XK_KP_Next },
                                     { "kp_left",   XK_KP_Left },
                                     { "kp_begin",  XK_KP_Begin },
                                     { "kp_right",  XK_KP_Right },
                                     { "kp_home",   XK_KP_Home },
                                     { "kp_up",     XK_KP_Up },
                                     { "kp_prior",  XK_KP_Prior }
                                   };

/// key names that are equivalent to one another
const map<string, string> equivalent_key_names = { { "kp_0", "kp_insert" },
                                                   { "kp_1", "kp_end" },
                                                   { "kp_2", "kp_down" },
                                                   { "kp_3", "kp_next" },
                                                   { "kp_4", "kp_left" },
                                                   { "kp_5", "kp_begin" },
                                                   { "kp_6", "kp_right" },
                                                   { "kp_7", "kp_home" },
                                                   { "kp_8", "kp_up" },
                                                   { "kp_9", "kp_prior" }
                                                 };

/// names of keys on the keypad
const unordered_set<KeySym> keypad_numbers({ { XK_KP_0 }, { XK_KP_1 }, { XK_KP_2 }, { XK_KP_3 }, { XK_KP_4 },
                                             { XK_KP_5 }, { XK_KP_6 }, { XK_KP_7 }, { XK_KP_8 }, { XK_KP_9 },
                                             { XK_KP_Insert }, { XK_KP_End }, { XK_KP_Down }, { XK_KP_Next }, { XK_KP_Left },
                                             { XK_KP_Begin }, { XK_KP_Right }, { XK_KP_Home }, { XK_KP_Up }, { XK_KP_Prior }
                                          });
