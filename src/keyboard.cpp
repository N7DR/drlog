// $Id: keyboard.cpp 58 2014-04-12 17:23:28Z  $

/*!     \file keyboard.cpp

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

extern message_stream           ost;

//int (*XSetErrorHandler(handler))()
//      int (*handler)(Display *, XErrorEvent *)


// -------------------------------------  keyboard_queue  ---------------------------

/*!     \class keyboard_queue
        \brief The basic queue of keyboard events, which is just a wrapper
        around an STL deque
*/

// X error handler
int keyboard_queue::_x_error_handler(Display* display_p, XErrorEvent* error_event_p)
{ cerr << "X Error" << endl;
  exit(-1);
}

void* runit(void* vp)
{ keyboard_queue* kqp = static_cast<keyboard_queue*>(vp);

  kqp->process_events();

  return nullptr;
}

/*
/void* runflush(void* vp)
{ keyboard_queue* kqp = static_cast<keyboard_queue*>(vp);

  while (1)
  { XFlush(kqp->display_p());
    sleep(1);
  }

  return nullptr;
}
*/

// default constructor
keyboard_queue::keyboard_queue(void) :
  _display_p(nullptr),
  _window_id(0),
  _x_multithreaded(true)        // to be safe, until we're sure we aren't running the simulator
{ XInitThreads();

// set the error handler for X
  XSetErrorHandler(_x_error_handler);

  _display_p = XOpenDisplay(NULL);

  if (!_display_p)
  { cerr << "Fatal error: unable to open display" << endl;
    exit (-1);
  }

// get the window ID
  const char* cp = getenv("WINDOWID");
  if (!cp)
  { cerr << "Fatal error: unable to obtain window ID" << endl;
    exit (-1);
  }

  _window_id = from_string<Window>(cp);

// we want to be the only process to have access to key presses in our window
  XGrabKey(_display_p, AnyKey, AnyModifier, _window_id, false, GrabModeAsync, GrabModeAsync);

// we are interested only in key events
  XSelectInput(_display_p, _window_id, KeyPressMask | KeyReleaseMask);

  pthread_t   thread_id_1;
  int ret = pthread_create(&thread_id_1, NULL, runit, this);

//  pthread_t   thread_id_2;
//  ret = pthread_create(&thread_id_2, NULL, runflush, this);
}

// place keyboard events in the queue
void keyboard_queue::process_events(void)
{ XEvent event;                    // the X event

  const int BUF_SIZE = 256;
  char buf[BUF_SIZE + 1];
  KeySym ks;

// if I take this out, then the events sent by XSendEvent() aren't seen by
// XWindowEvent() until after a real key has been pressed. I have no real idea why
// this is so -- nothing seems to suggest that XKeysymToKeycode()
// does anything other than simply perform the expected mapping.

// My best guess is that it's something to do with threading... indeed, from what I can
// tell I'm supposed to explicitly lock the display before a call to an X function that
// uses the display pointer... but I can't find any example of how that really works in
// practice: one can't wrap the call XWindowEvent() in such a lock because it blocks.

// Perhaps one is supposed to call XPending() first, and wrap that, and only call XWindowEvent()
// when we know there's an event waiting. But if there's no such event what does one do? One can hardly
// call usleep() for some arbitrary period and then retry the XPending() call: that would be a horribly
// inefficient polling mechanism, checking the queue many times per second.

// One wants a function that blocks and is guaranteed thread-safe, but xlib() doesn't seem to provide one.

// The XCB library is supposed to be robustly thread-safe... which would be great if it provided a useful
// replacement for XWindowEvent(); but apparently, from reading comments on the web, it does not (yet).

// So we're back to including the next line and hoping that it works.

  /* const KeyCode kc3 = */ //XKeysymToKeycode(_display_p, XK_g);

//  ost << "After XKeysymToKeycode() kc3" << endl;

  while (1)
  {
// get the next event
//    sleep(10);
//    XFlush(_display_p);

    if (_x_multithreaded)
    { bool got_event = false;

      while (!got_event)
      { XLockDisplay(_display_p);
        int pending = XPending(_display_p);
        XUnlockDisplay(_display_p);

        if (pending)
        { XLockDisplay(_display_p);
          XWindowEvent(_display_p, _window_id, KeyPressMask | KeyReleaseMask, &event);
          XUnlockDisplay(_display_p);
          got_event = true;
        }
        else
          sleep_for(milliseconds(10));
      }
    }
    else
      XWindowEvent(_display_p, _window_id, KeyPressMask | KeyReleaseMask, &event);

    ost << "Event: keycode = " << event.xkey.keycode << endl;

    if ((event.type == KeyPress) or (event.type == KeyRelease))    // should always be true
    { ost << "KeyPress or KeyRelease" << endl;

      keyboard_event ke;

      ke.event(event.type == KeyPress ? KEY_PRESS : KEY_RELEASE);
      ke.code(event.xkey.keycode);
      ke.xkey_state(event.xkey.state);
      ke.xkey_time(event.xkey.time);

// get the related KeySym and string representation
      const int n_chars = XLookupString((XKeyEvent*)(&event), buf, BUF_SIZE, &ks, NULL);

      ost << "n_chars = " << n_chars << endl;

      buf[n_chars] = static_cast<char>(0);

      ke.symbol(ks);
      ke.str(string(buf));

      ost << "event string seems to be: " << ke.str() << endl;

// for drlog we are interested only in:
//   1. KeyPress
//   2. KeyRelease for the SHIFT keys
// KeySyms are defined in:      /usr/include/X11/keysymdef.h

      bool interesting_event = false;
      interesting_event |= (event.type == KeyPress);
      interesting_event |= (event.type == KeyRelease and ((ks == XK_Shift_L) or (ks == XK_Shift_R)));

      if (interesting_event)
      { ost << "event seems to be interesting" << endl;

        SAFELOCK(_keyboard);

        _events.push_back(ke);
      }
    }
  }
}

const size_t keyboard_queue::size(void)
{ SAFELOCK(_keyboard);

  return _events.size();
}

const bool keyboard_queue::empty(void)
{ SAFELOCK(_keyboard);

  return _events.empty();
}

// peek at the front of the queue
const keyboard_event keyboard_queue::peek(void)
{ SAFELOCK (_keyboard);

  return (!_events.empty() ? _events.front() : keyboard_event());
}

// pop the frontmost event
const keyboard_event keyboard_queue::pop(void)
{ SAFELOCK(_keyboard);

  keyboard_event rv = peek();

  if (!_events.empty())
  { _events.pop_front();
    _last_event = rv;

//    const KeyCode kc4 = XKeysymToKeycode(_display_p, XK_g);

//    ost << "After XKeysymToKeycode() kc4" << endl;
  }

  return rv;
}

// get the most-recently removed event
 const keyboard_event keyboard_queue::last(void)
 { SAFELOCK(_keyboard);

   return _last_event;
 }

 void keyboard_queue::push_key_press(const char c)
 {   //ost << "inside push_key_press" << endl;

 //const KeyCode kc5 = XKeysymToKeycode(_display_p, XK_g);

 //ost << "After XKeysymToKeycode() kc5" << endl;



// XEvent event;    // the X event

//   event.type = KeyPress;

   switch (c)
   { case '/' :
       push_key_press(static_cast<KeySym>(XK_slash));    // the default algorithm doesn't work for the solidus
       break;

     default:
     { const string c_str = create_string(c);


 //  ost << "calling XStringToKeysym" << endl;

       const KeySym ks = XStringToKeysym(c_str.c_str());

//   ost << "returned KeySym = " << ks << endl;

//   const char* cp = XKeysymToString(ks);

//   ost << "string equivalent = " << string(cp) << endl;

//       ost << "calling XKeysymToKeycode" << endl;

       push_key_press(ks);
     }
   }
 }

 void keyboard_queue::push_key_press(const KeySym ks)
 {XEvent event;    // the X event

 event.type = KeyPress;

//   const KeyCode kc = XKeysymToKeycode(_display_p, ks);
   if (_x_multithreaded)
     XLockDisplay(_display_p);

   const KeyCode kc = XKeysymToKeycode(_display_p, ks);

//   if (x_multithreaded)
//     XUnlockDisplay(_display_p);

   ost << "after XKeysymToKeycode kc =" << static_cast<int>(kc) << endl;

   event.xkey.keycode = kc;
   event.xkey.state = 0;      // no modifiers

   const Window window_id = _window_id;
   Display* display_p = _display_p;

   ost << "about to call XSendEvent" << endl;

// TRY http://stackoverflow.com/questions/6560553/linux-x11-global-keyboard-hook

   XKeyEvent e;

 //  event.xkey.type = /* (const) */ KeyPress;
   event.xkey.display = _display_p;
   event.xkey.window = window_id;
   event.xkey.root = DefaultRootWindow(/* display = */ _display_p);
   event.xkey.subwindow = /* (const) */ None;
//   event.xkey.time = 1000 * time(/* tloc = */ NULL);
//   event.xkey.time = CurrentTime;
   event.xkey.time = CurrentTime;
   event.xkey.x = 0;
   event.xkey.y = 0;
   event.xkey.x_root = 0;
   event.xkey.y_root = 0;
   event.xkey.state = /* (const) */ NoEventMask;
   event.xkey.keycode = XKeysymToKeycode(/* display = */ _display_p, /* keysym = */ ks);
   event.xkey.same_screen = /* (const) */ True;

 // TRY   http://www.doctort.org/adam/nerd-notes/x11-fake-keypress-event.html
   const int status = XSendEvent(display_p, window_id, False, KeyPressMask, &event);
   XFlush(display_p);

   if (_x_multithreaded)
     XUnlockDisplay(_display_p);
//   int pending = XPending(display_p);
//   XSync(display_p, false);

//   sleep(1);

//   event.xkey.keycode = XKeysymToKeycode(/* display = */ _display_p, /* keysym = */ XK_h);
//   usleep(1000);   // 1 millisecond

//   XSendEvent(display_p, window_id, True, KeyReleaseMask, &event);

//   ost << "XSendEvent status = " << status << endl;
 }

void keyboard_queue::push_key_press(const string& str, const int ms_delay)
{ for (size_t n = 0; n < str.length(); ++n)
  { push_key_press(str[n]);
//    XFlush(_display_p);

    if (n != str.length() - 1)
      sleep_for(milliseconds(ms_delay));
  }
}

 // maps key names in the config file to KeySym numbers (which are integers)
 // This is used to decode access to the correct CW messages when a key is pressed
 // See the file drlog_context.cpp to see this in use
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

const unordered_set<KeySym> keypad_numbers({ { XK_KP_0 }, { XK_KP_1 }, { XK_KP_2 }, { XK_KP_3 }, { XK_KP_4 },
                                             { XK_KP_5 }, { XK_KP_6 }, { XK_KP_7 }, { XK_KP_8 }, { XK_KP_9 },
                                             { XK_KP_Insert }, { XK_KP_End }, { XK_KP_Down }, { XK_KP_Next }, { XK_KP_Left },
                                             { XK_KP_Begin }, { XK_KP_Right }, { XK_KP_Home }, { XK_KP_Up }, { XK_KP_Prior }
                                           });