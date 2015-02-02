// $Id: cw_buffer.cpp 43 2013-12-07 20:29:56Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file cwbuffer.cpp

        Classes and functions related to sending CW
*/

#include "cw_buffer.h"
#include "log_message.h"

#include <chrono>
#include <iostream>
#include <thread>

//#include <unistd.h>        // for sleep()

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread

const unsigned int MIN_SPEED = 12;
const unsigned int MAX_SPEED = 50;

const int CMD_CLEAR_RIT = 0,
          CMD_SLOWER    = 1,
          CMD_FASTER    = 2;

extern message_stream ost;

// just to be sure
#undef KEY_DOWN
#undef KEY_UP

// ---------------------------------------- cw_buffer -------------------------

/*!     \class cw_buffer
        \brief A buffer for sending CW
*/

// add an action to the key buffer
void cw_buffer::_add_action(const int n)
{ SAFELOCK(_key_buffer);

  _key_buffer.push(n);
}

void* cw_buffer::_static_play(void* arg)    // arg is the this pointer, in order to allow static member access to a real object
{ cw_buffer* bufp = static_cast<cw_buffer*>(arg);

  bufp->_play(nullptr);

  return nullptr;
}

const int BUFFER_ESCAPE = 0;

//const char PTT      = C1284_NINIT;
//const char KEY_DOWN = C1284_NSELECTIN | PTT;
//const char KEY_UP   = PTT;

// we have to use CW_ in order to avoid clashes with ncurses -- yes, it's silly that this file knows about ncurses
void* cw_buffer::_play(void*)
{ const char PTT      = (_ptt_delay ? C1284_NINIT : 0);
  const char CW_KEY_DOWN = C1284_NSELECTIN | PTT;
  const char CW_KEY_UP   = PTT;

//  bool starting = true;                   // true if PTT is not asserted
  bool ptt_asserted = false;

//  ost << "starting CW play" << endl;
//  ost << "length of buffer is: " << _key_buffer.size() << endl;

  while (true)
  { int next_action   = 0;                // next key up/down/command
//    int command       = 0;
//    int subcommand    = 0;
    bool buffer_was_empty;


    { SAFELOCK(_key_buffer);

      if (!_key_buffer.empty())
      { //ost << "key buffer is not empty; next value = " << _key_buffer.front() << endl;

        next_action = _key_buffer.front();
        _key_buffer.pop();
        buffer_was_empty = false;

//        if (next_action == BUFFER_ESCAPE)    // was it a zero?
//        { if (!_key_buffer.empty())
//          { command = _key_buffer.front();
//            _key_buffer.pop();
//          }                                  // what about the case where there is no commande actually present???
//        }
      }
      else
        buffer_was_empty = true;
    }

    if (!buffer_was_empty)                                         // do we have something to do?
    { //ost << "executing action: " << next_action << endl;

// execute key down
      if (next_action > 0)
      { if (!ptt_asserted)                      // we need to assert PTT alone for a while
        { ptt_asserted = true;
          _port.control(PTT);              // assert PTT

          sleep_for(microseconds(_ptt_delay * 1000));
        }

        const unsigned int duration = _usec * next_action / 100;    // key-down duration in microseconds

      //cout << "key down for " << duration << " microseconds" << endl;

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

 //ost << "PTT sending control(0)" << endl;
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
// possibly we should leave PTT asserted for a millisecond
//          _port.control(PTT);         // key remains up but keep PT asserted
          sleep_for(microseconds(1000));

 //ost << "PTT sending control(0)" << endl;
          _port.control(0);
          ptt_asserted = false;
        }
      }

// execute special command
      if (next_action == 0)
      { //ost << "Next action is zero" << endl;

        bool got_command = false;
        int  command;
        int time_out_counter = 0;

        while (!got_command and (time_out_counter < 5))      // 5 millisecond time-out
        { bool buffer_is_empty = false;

          { SAFELOCK(_key_buffer);

            buffer_is_empty = _key_buffer.empty();
          }

          if (buffer_is_empty)
          { //ost << "Sleeping for one millisecond inside cw buffer" << endl;
            sleep_for(microseconds(1000));  // wait for one millisecond this should never happen: it means that we got an command indicator without a command

            time_out_counter++;
          }
          else
          { SAFELOCK(_key_buffer);

            command = _key_buffer.front();
            _key_buffer.pop();

//            ost << "Got the command" << endl;

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

// ost << "PTT sending control(0)" << endl;
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
      { //cout << "waiting" << endl;
        SAFELOCK(_condvar);            // must have the lock before waiting
        _condvar.wait();
        //cout << "signal caught; end of waiting" << endl;
      }

      catch (const pthread_error& e)
      { cout << "condvar wait error code = " << e.code() << "; reason = " << e.reason() << endl;
        exit(-1);
      }

    }
  }

  return nullptr;
}

// construct on a parallel port
cw_buffer::cw_buffer(const string& filename, const unsigned int delay, const unsigned int wpm_speed) :
  _port(filename),
  _ptt_delay(delay),
  _aborted(false),
  _rigp(nullptr),
  _disabled_cw(false)
{
//cout << "Setting up CW on " << filename << endl;
//cout << "delay = " << delay << endl;
//cout << "speed = " << wpm_speed << endl;

//  exit(0);


  speed(wpm_speed);
  _condvar.set_mutex(_condvar_mutex);
  _port.control(0);                            // explicitly turn off PTT

//  int ret =  pthread_create( &_thread_id, NULL, &_static_play, this);

//  if (ret)
//  { cout << "Error creating thread" << endl;
//    throw exception();
//  }

  try
  { create_thread(&_thread_id, NULL, &_static_play, this, "CW BUFFER");
  }

  catch (const pthread_error& e)
  { ost << "Error creating thread: CW BUFFER" << endl;
    throw;
  }
}

// destructor
cw_buffer::~cw_buffer(void)
{ SAFELOCK(_key_buffer);

  while (!_key_buffer.empty())
    _key_buffer.pop();

  _port.control(0);
//  pa_simple_free(_sp);
}

// set the speed in wpm
void cw_buffer::speed(const unsigned int wpm)
{ unsigned int new_wpm = min(wpm, MAX_SPEED);

  new_wpm = max(new_wpm, MIN_SPEED);

  SAFELOCK(_speed);

  _wpm = new_wpm;
  _usec = 1200000 / _wpm;
}

// get the speed in wpm
const unsigned int cw_buffer::speed(void)
{ SAFELOCK(_speed);

  return _wpm;
}

// set the PTT delay in msec
void cw_buffer::ptt_delay(const unsigned int msec)
{ SAFELOCK(_speed);

  _ptt_delay = msec;
}

// get the PTT delay in msec
const unsigned int cw_buffer::ptt_delay(void)
{ SAFELOCK(_speed);

  return _ptt_delay;
}

// add a key_down interval (100 = 1 dot)
void cw_buffer::key_down(const int n, const int space)
{
  try
  { //cout << "inside key-down" << endl;
    { SAFELOCK(_key_buffer);

      _key_buffer.push(n);

// add the space if it's non-zero
      if (space)
        _key_buffer.push(-space);
    }

    //cout << "sending signal on key-down" << endl;
    _condvar.signal();
    //cout << "sent signal on key-down" << endl;

  }

  catch (const pthread_error& e)
  { cout << "condvar signal error code = " << e.code() << "; reason = " << e.reason() << endl;
    exit(-1);
  }
}

// add a key-up interval (no subsequent gap) 100 = 1 dot
void cw_buffer::key_up(const int n)
{
  { SAFELOCK(_key_buffer);

    _key_buffer.push(-n);
  }

  //cout << "sending signal on key-up" << endl;
  _condvar.signal();
  //cout << "sent signal on key-up" << endl;

}

// there *is* a use for #define in C++
// each of these is automatically followed by a 100-unit key up
#define DIT key_down(100)
#define DAH key_down(300)

// send a single character (followed by a character_space)
void cw_buffer::add(const char c, const int character_space)
{ // ost << "adding char: " << c << endl;

  if (disabled())
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
//      key_up(50);
      break;

    case '!' :                               // quarter-length space (not available in TR)
      space = 0;
      key_up(25);                         // 100 from last character + 50 = 150 = (300 / 2)
//      key_up(50);
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
    case static_cast<char>(16) :             // ctrl-P 60% dit
      space = 0;
      key_down(60);
      break;

    case static_cast<char>(17) :             // ctrl-Q 80% dit
      space = 0;
      key_down(80);
      break;

    case static_cast<char>(28) :             // ctrl-\ 100% dit
      space = 0;
      key_down(100);
      break;

    case static_cast<char>(22) :             // ctrl-V 120% dit
      space = 0;
      key_down(120);
      break;

    case static_cast<char>(12) :             // ctrl-L 140% dit
      space = 0;
      key_down(140);
      break;

    case static_cast<char>(5) :             // ctrl-E 73% dah
      space = 0;
      key_down(219);
      break;

    case static_cast<char>(4) :             // ctrl-D 87% dah
      space = 0;
      key_down(261);
      break;

    case static_cast<char>(11) :             // ctrl-K 100% dah
      space = 0;
      key_down(300);
      break;

    case static_cast<char>(14) :             // ctrl-N 113% dah
      space = 0;
      key_down(339);
      break;

    case static_cast<char>(15) :             // ctrl-O 127% dah
      space = 0;
      key_down(381);
      break;

    case ')' :                               // 150% dah; '-' in TRLOG
      space = 0;
      key_down(450);
      break;

// inline speed controls; not compatible with TRLOG,
// although in practice should give very similar results to TRLOG's ±6% increment
//    case static_cast<char>(19) :             // ctrl-S decrease speed by 1 wpm
//      space = 0;
//      speed(speed() - 1);
//      break;

//    case static_cast<char>(6) :              // ctrl-F increase speed by 1 wpm
//      space = 0;
//      speed(speed() + 1);
//      break;

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

//    case '#' :                               // send a number; optionally followed by -N or +N
//      space = 0;
//      _key_buffer.push(0);                   // command
//      _key_buffer.push(CMD_CLEAR_RIT);
//      break;

    default:
      key_down(1000);
      break;
  }

// typically add the extra two dit-length spaces so that the total inter-character space is 300
  if (space)
    key_up(space);
}

// send a string
void cw_buffer::operator<<(const std::string& str)
{ for (size_t n = 0; n < str.length(); ++n)
    add(str[n]);

//  cerr << "added to buffer: " << str << endl;
}

// abort sending -- not sure whether just clearing the buffer is sufficient
void cw_buffer::abort(void)
{ clear();
}

// clear the buffer
void cw_buffer::clear(void)
{ SAFELOCK(_key_buffer);

  while (!_key_buffer.empty())
    _key_buffer.pop();
}

// associate a rig with the buffer
void cw_buffer::associate_rig(rig_interface* rigp)
{ if (rigp)
    _rigp = rigp;
}

// is the buffer empty?
const bool cw_buffer::empty(void)
{ SAFELOCK(_key_buffer);

  return _key_buffer.empty();
}

// ---------------------------------------- cw_messages -------------------------

const string cw_messages::operator[](const int n)
{ SAFELOCK(_messages);

  map<int, string>::const_iterator cit = _messages.find(n);

  return (cit == _messages.cend() ? "" : cit->second);
}

