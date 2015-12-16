// $Id: cw_buffer.cpp 113 2015-08-01 14:57:22Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file cw_buffer.cpp

        Classes and functions related to sending CW
*/

#include "cw_buffer.h"
#include "log_message.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;
using namespace   chrono;               // std::chrono
using namespace   this_thread;          // std::this_thread

const unsigned int MIN_SPEED = 12;      ///< minimum CW speed, in wpm
const unsigned int MAX_SPEED = 50;      ///< maximum CW speed, in wpm

// special commands that may be embedded in the CW stream
const int CMD_CLEAR_RIT = 0,            ///< clear the RIT
          CMD_SLOWER    = 1,            ///< decrease speed by 1 wpm
          CMD_FASTER    = 2;            ///< increase speed by 1 wpm

extern message_stream ost;              ///< for debugging, info

// just to be sure; we do not want these to be defined in this file
#undef KEY_DOWN
#undef KEY_UP

// ---------------------------------------- cw_buffer -------------------------

/*!     \class cw_buffer
        \brief A buffer for sending CW
*/

/*! \brief      Add an action to the key buffer
    \param  n   coded action

    positive values of <i>n</i> represent key down;
    negative values represent key up;
    zero represents the start of an embedded command
*/
void cw_buffer::_add_action(const int n)
{ SAFELOCK(_key_buffer);

  _key_buffer.push(n);
}

/*! \brief          Wrapper function to play the buffer
    \param  arg     "this" pointer
    \return         nullptr
*/
void* cw_buffer::_static_play(void* arg)              // arg is the "this" pointer, in order to allow static member access to a real object
{ cw_buffer* bufp = static_cast<cw_buffer*>(arg);

  bufp->_play(nullptr);

  return nullptr;
}


/*! \brief      Play the buffer
    \param      unused
    \return     nullptr
*/
void* cw_buffer::_play(void*)
{
// we have to use CW_ prefix in order to avoid clashes with ncurses -- yes, it's ridiculous that this file knows about ncurses

#if 1
  const char STROBE = C1284_NSTROBE;
  const char PTT      = (_ptt_delay ? C1284_NINIT : 0);
  const char CW_KEY_DOWN = C1284_NSELECTIN | PTT;
  const char CW_KEY_UP   = PTT;
#endif

#if 0
  const char STROBE = C1284_NSTROBE;
  const char PTT      = (_ptt_delay ? C1284_NINIT | C1284_NSTROBE: 0);
  const char CW_KEY_DOWN = C1284_NSELECTIN | PTT  | C1284_NSTROBE;
  const char CW_KEY_UP   = PTT | C1284_NSTROBE;
#endif


  bool ptt_asserted = false;

  while (true)
  { int next_action   = 0;                // next key up/down/command
    bool buffer_was_empty;


    { SAFELOCK(_key_buffer);

      if (!_key_buffer.empty())
      { next_action = _key_buffer.front();
        _key_buffer.pop();
        buffer_was_empty = false;
      }
      else
        buffer_was_empty = true;
    }

    if (!buffer_was_empty)                                         // do we have something to do?
    {
// execute key down
      if (next_action > 0)
      { if (!ptt_asserted)                      // we need to assert PTT alone for a while
        { ptt_asserted = true;
          _port.control(PTT);                   // assert PTT

          sleep_for(microseconds(_ptt_delay * 1000));
        }

        const unsigned int duration = _usec * next_action / 100;    // key-down duration in microseconds

        _port.control(CW_KEY_DOWN);
        sleep_for(microseconds(duration));

        bool buffer_is_empty;

        { SAFELOCK(_key_buffer);

          buffer_is_empty = _key_buffer.empty();
        }

        if (buffer_is_empty)      // did we empty the buffer?
        {
// possibly we should leave PTT asserted for a millisecond
          _port.control(PTT);         // key up but keep PT asserted
          sleep_for(microseconds(1000));

          _port.control(0);
          ptt_asserted = false;
        }
        else
          _port.control(PTT);              // key up; keep PTT asserted
      }

// execute key up
      if (next_action < 0)
      { next_action = -next_action;

        const unsigned int duration = _usec * next_action / 100;    // key-up duration in microseconds

        _port.control(PTT);                // key up; PTT asserted
        sleep_for(microseconds(duration));

        bool buffer_is_empty;

        { SAFELOCK(_key_buffer);

          buffer_is_empty = _key_buffer.empty();
        }

        if (buffer_is_empty)      // did we empty the buffer?
        {
// leave PTT asserted for a millisecond
          sleep_for(microseconds(1000));

          _port.control(0);
          ptt_asserted = false;
        }
      }

// execute special command
      if (next_action == 0)
      { bool got_command = false;
        int  command;
        int time_out_counter = 0;

        while (!got_command and (time_out_counter < 5))      // 5 millisecond time-out
        { bool buffer_is_empty = false;

          { SAFELOCK(_key_buffer);

            buffer_is_empty = _key_buffer.empty();
          }

          if (buffer_is_empty)
          { sleep_for(microseconds(1000));  // wait for one millisecond this should never happen: it means that we got an command indicator without a command

            time_out_counter++;
          }
          else
          { SAFELOCK(_key_buffer);

            command = _key_buffer.front();
            _key_buffer.pop();

            got_command = true;
          }
        }

        if (got_command)
        { switch (command)
          { case CMD_CLEAR_RIT :
              if (_rigp)
                _rigp->rit(0);
              break;

            case CMD_SLOWER :
              speed(speed() - 1);
              break;

            case CMD_FASTER :
              speed(speed() + 1);
              break;

            default:
              break;
          }
        }
        else
          ost << "WARNING: COMMAND INDICATOR WITHOUT SUBSEQUENT COMMAND IN CW_BUFFER()" << endl;

        bool buffer_is_empty;

        { SAFELOCK(_key_buffer);

          buffer_is_empty = _key_buffer.empty();
        }

        if (buffer_is_empty)      // did we empty the buffer?
        {
// possibly we should leave PTT asserted for a millisecond
          _port.control(PTT);         // key up but keep PTT asserted
          sleep_for(microseconds(1000));

          _port.control(0);
          ptt_asserted = false;
        }
        else
          _port.control(PTT);              // key up; keep PTT asserted
      }
    }                    // end of !(buffer_was_empty)

    bool buffer_is_empty = false;

    { SAFELOCK(_key_buffer);

      buffer_is_empty = _key_buffer.empty();
    }

// if there's nothing else in the buffer, ensure that PTT is not asserted
// although I think every path to here actually de-asserts it before now
    if (buffer_is_empty)
    { _port.control(0);         // de-assert (!) PTT
      ptt_asserted = false;

      try
      { SAFELOCK(_condvar);            // must have the lock before waiting
        _condvar.wait();
      }

      catch (const pthread_error& e)
      { cout << "condvar wait error code = " << e.code() << "; reason = " << e.reason() << endl;
        exit(-1);
      }

    }
  }

  return nullptr;
}

/*! \brief              Construct on a parallel port
    \param  filename    device file
    \param  delay       PTT delay (in milliseconds)
    \param  speed       Speed (in WPM)
*/
cw_buffer::cw_buffer(const string& filename, const unsigned int delay, const unsigned int wpm_speed) :
  _port(filename),
  _ptt_delay(delay),
  _aborted(false),
  _rigp(nullptr),
  _disabled_cw(false)
{ speed(wpm_speed);
  _condvar.set_mutex(_condvar_mutex);
  _port.control(0);                            // explicitly turn off PTT

  try
  { create_thread(&_thread_id, NULL, &_static_play, this, "CW BUFFER");
  }

  catch (const pthread_error& e)
  { ost << "Error creating thread: CW BUFFER" << endl;
    throw;
  }
}

/// destructor
cw_buffer::~cw_buffer(void)
{ SAFELOCK(_key_buffer);

  while (!_key_buffer.empty())
    _key_buffer.pop();

  _port.control(0);
}

/*! \brief          Set the speed
    \param  wpm     speed in WPM
*/
void cw_buffer::speed(const unsigned int wpm)
{ unsigned int new_wpm = min(wpm, MAX_SPEED);

  new_wpm = max(new_wpm, MIN_SPEED);

  SAFELOCK(_speed);

  _wpm = new_wpm;
  _usec = 1200000 / _wpm;
}

/// get the speed in wpm
const unsigned int cw_buffer::speed(void)
{ SAFELOCK(_speed);

  return _wpm;
}

// set the PTT delay in msec
void cw_buffer::ptt_delay(const unsigned int msec)
{ SAFELOCK(_speed);

  _ptt_delay = msec;
}

/// get the PTT delay in msec
const unsigned int cw_buffer::ptt_delay(void)
{ SAFELOCK(_speed);

  return _ptt_delay;
}

/*! \brief          Add a key-down interval, along with a subsequent gap
    \param  n       key-down interval (100 = 1 dot)
    \param  space   terminating key-up interval (100 = 1 dot)

*/
void cw_buffer::key_down(const int n, const int space)
{
  try
  {
    { SAFELOCK(_key_buffer);

      _key_buffer.push(n);

// add the space if it's non-zero
      if (space)
        _key_buffer.push(-space);
    }

    _condvar.signal();
  }

  catch (const pthread_error& e)
  { cout << "condvar signal error code = " << e.code() << "; reason = " << e.reason() << endl;
    exit(-1);
  }
}

/*! \brief      Add a key-up interval, with no subsequent gap
    \param  n   key-up interval (100 = 1 dot)
*/
void cw_buffer::key_up(const int n)
{
  { SAFELOCK(_key_buffer);

    _key_buffer.push(-n);
  }

  _condvar.signal();
}

// there *is* a use for #define in C++
// each of these is automatically followed by a 100-unit key up
#define DIT key_down(100)
#define DAH key_down(300)

/*! \brief                      Send a single character (followed by a character_space)
    \param  c                   character to send
    \param  character_space     duration of the character space (100 = 1 dot)
*/
void cw_buffer::add(const char c, const int character_space)
{ if (disabled())
    return;

  int space = character_space;                                 // a non-constant version

  switch (c)
  { case 'A' :
    case 'a' :
      DIT; DAH;
      break;

    case 'B' :
    case 'b' :
      DAH; DIT; DIT; DIT;
      break;

    case 'C' :
    case 'c' :
      DAH; DIT; DAH; DIT;
      break;

    case 'D' :
    case 'd' :
      DAH; DIT; DIT;
      break;

    case 'E' :
    case 'e' :
      DIT;
      break;

    case 'F' :
    case 'f' :
      DIT; DIT; DAH; DIT;;
      break;

    case 'G' :
    case 'g' :
      DAH; DAH; DIT;
      break;

    case 'H' :
    case 'h' :
      DIT; DIT; DIT; DIT;
      break;

    case 'I' :
    case 'i' :
      DIT; DIT;
      break;

    case 'J' :
    case 'j' :
      DIT; DAH; DAH; DAH;
      break;

    case 'K' :
    case 'k' :
      DAH; DIT; DAH;
      break;

    case 'L' :
    case 'l' :
      DIT; DAH; DIT; DIT;
      break;

    case 'M' :
    case 'm' :
      DAH; DAH;
      break;

    case 'N' :
    case 'n' :
      DAH; DIT;
      break;

    case 'O' :
    case 'o' :
      DAH; DAH; DAH;
      break;

    case 'P' :
    case 'p' :
      DIT; DAH; DAH; DIT;
      break;

    case 'Q' :
    case 'q' :
      DAH; DAH; DIT; DAH;
      break;

    case 'R' :
    case 'r' :
      DIT; DAH; DIT;
      break;

    case 'S' :
    case 's' :
      DIT; DIT; DIT;
      break;

    case 'T' :
    case 't' :
      DAH;
      break;

    case 'U' :
    case 'u' :
      DIT; DIT; DAH;
      break;

    case 'V' :
    case 'v' :
      DIT; DIT; DIT; DAH;
      break;

    case 'W' :
    case 'w' :
      DIT; DAH; DAH;
      break;

    case 'X' :
    case 'x' :
      DAH; DIT; DIT; DAH;
      break;

    case 'Y' :
    case 'y' :
      DAH; DIT; DAH; DAH;
      break;

    case 'Z' :
    case 'z' :
      DAH; DAH; DIT; DIT;
      break;

    case '1' :
      DIT; DAH; DAH; DAH; DAH;
      break;

    case '2' :
      DIT; DIT; DAH; DAH; DAH;
      break;

    case '3' :
      DIT; DIT; DIT; DAH; DAH;
      break;

    case '4' :
      DIT; DIT; DIT; DIT; DAH;
      break;

    case '5' :
      DIT; DIT; DIT; DIT; DIT;
      break;

    case '6' :
      DAH; DIT; DIT; DIT; DIT;
      break;

    case '7' :
      DAH; DAH; DIT; DIT; DIT;
      break;

    case '8' :
      DAH; DAH; DAH; DIT; DIT;
      break;

    case '9' :
      DAH; DAH; DAH; DAH; DIT;
      break;

    case '0' :
      DAH; DAH; DAH; DAH; DAH;
      break;

    case ' ' :
    case '_' :
      space = 0;       // ??? yes?
      key_up(400);                            // 100 from last character + 200 = 300
      break;

    case '/' :
      DAH; DIT; DIT; DAH; DIT;
      break;

    case '?' :
      DIT; DIT; DAH; DAH; DIT; DIT;
      break;

    case ',' :
      DAH; DAH; DIT; DIT; DAH; DAH;
      break;

    case '.' :
      DIT; DAH; DIT; DAH; DIT; DAH;
      break;

    case '@' :
      DIT; DAH; DAH; DIT; DAH; DIT;
      break;

    case '\'' :
      DIT; DAH; DAH; DAH; DAH; DIT;
      break;

    case '^' :                               // half-length space
      space = 0;
      key_up(150);                         // 100 from last character + 50 = 150 = (300 / 2)
      break;

    case '!' :                               // quarter-length space (not available in TR)
      space = 0;
      key_up(25);
      break;

// prosigns; compatible with TRLOG
    case '(' :                               // AR  '+' in TRLOG
      DIT; DAH; DIT; DAH; DIT;
      break;

    case '<' :                               // SK
      DIT; DIT; DIT; DAH; DIT; DAH;
      break;

    case '=' :                               // Pause (BT)
      DAH; DIT; DIT; DIT; DAH;
      break;

// short/long dits and dahs, compatible with TRLOG's very peculiar conventions,
// which were presumably driven by some limitation in DOS
    case static_cast<char>(16) :                // ctrl-P 60% dit
      space = 0;
      key_down(60);
      break;

    case static_cast<char>(17) :                // ctrl-Q 80% dit
      space = 0;
      key_down(80);
      break;

    case static_cast<char>(28) :                // ctrl-\ 100% dit
      space = 0;
      key_down(100);
      break;

    case static_cast<char>(22) :                // ctrl-V 120% dit
      space = 0;
      key_down(120);
      break;

    case static_cast<char>(12) :                // ctrl-L 140% dit
      space = 0;
      key_down(140);
      break;

    case static_cast<char>(5) :                 // ctrl-E 73% dah
      space = 0;
      key_down(219);
      break;

    case static_cast<char>(4) :                 // ctrl-D 87% dah
      space = 0;
      key_down(261);
      break;

    case static_cast<char>(11) :                // ctrl-K 100% dah
      space = 0;
      key_down(300);
      break;

    case static_cast<char>(14) :                // ctrl-N 113% dah
      space = 0;
      key_down(339);
      break;

    case static_cast<char>(15) :                // ctrl-O 127% dah
      space = 0;
      key_down(381);
      break;

    case ')' :                                  // 150% dah; '-' in TRLOG
      space = 0;
      key_down(450);
      break;

// special commands
    case '>' :                               // clear RIT
      space = 0;
      _key_buffer.push(0);                   // command
      _key_buffer.push(CMD_CLEAR_RIT);
      break;

    case '-' :                               // slower by 1 WPM (not compatible with TRLOG)
      space = 0;
      _key_buffer.push(0);                   // command
      _key_buffer.push(CMD_SLOWER);
      break;

    case '+' :                               // faster by 1 WPM (not compatible with TRLOG)
      space = 0;
      _key_buffer.push(0);                   // command
      _key_buffer.push(CMD_FASTER);
      break;

    default:
      key_down(1000);
      break;
  }

// typically add the extra two dit-length spaces so that the total inter-character space is 300
  if (space)
    key_up(space);
}

/*! \brief          Send a string
    \param  str     string to send

    Special characters and commands embedded in <i>str</i> are expanded and/or processed
    prior to transmission
*/
void cw_buffer::operator<<(const string& str)
{ for (size_t n = 0; n < str.length(); ++n)
    add(str[n]);
}

// abort sending -- not sure whether just clearing the buffer is sufficient
//void cw_buffer::abort(void)
//{ clear();
//}

/// clear the buffer
void cw_buffer::clear(void)
{ SAFELOCK(_key_buffer);

  while (!_key_buffer.empty())              // deques have no clear() member
    _key_buffer.pop();
}

/*! \brief          Associate a rig with the buffer
    \param  rigp    pointer to rig interface to be associated with the buffer
*/
void cw_buffer::associate_rig(rig_interface* rigp)
{ if (rigp)
    _rigp = rigp;
}

/// is the buffer empty?
const bool cw_buffer::empty(void)
{ SAFELOCK(_key_buffer);

  return _key_buffer.empty();
}

// ---------------------------------------- cw_messages -------------------------

/*! \brief      Get a particular CW message
    \param  n   number of message to return
    \return     CW message number <i>n</i>

     Returns empty string if message number <i>n</i> does not exist
*/
const string cw_messages::operator[](const int n)
{ SAFELOCK(_messages);

  map<int, string>::const_iterator cit = _messages.find(n);

  return (cit == _messages.cend() ? string() : cit->second);
}

