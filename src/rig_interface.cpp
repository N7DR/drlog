// $Id: rig_interface.cpp 68 2014-06-28 15:42:35Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file rig_interface.cpp

        Classes and functions related to transferring information
        between the computer and the rig
*/

#include "bands-modes.h"
#include "drlog_context.h"
#include "rig_interface.h"
#include "string_functions.h"

#include "hamlib/serial.h"

#include <chrono>
#include <iostream>
#include <thread>

#include <cstring>

#include <termios.h>
#include <cstdio>
#include <fcntl.h>

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread

const bool RESPONSE_EXPECTED = true;

/* The current version of Hamlib seems to be both slow and unreliable with the K3. Anent unreliability, for example, the is_locked() function
 * as written below causes the entire program to freeze (presumably some kind of blocking or threading issue in hamlib).
 *
 * Anyway, as a result of this, we use explicit K3 control commands where appropriate. This may be changed if/when hamlib proves
 * itself sufficiently robust
 */

// sleep for a designated number of milliseconds
/*void rig_interface::_msec_sleep(const unsigned int msec)
{ struct timespec nanosleep_time;
  struct timespec remaining_nanosleep_time;

  nanosleep_time.tv_sec = 0;
  nanosleep_time.tv_nsec = msec * 1000000;              // milliseconds -> nanoseconds

  while (nanosleep_time.tv_nsec >= 1000000000)
  { nanosleep_time.tv_sec++;
    nanosleep_time.tv_nsec -= 1000000000;
  }

  const int status = nanosleep(&nanosleep_time, &remaining_nanosleep_time);

  if (status != 0)
    _error_alert("problem with nanosleep() in msec_sleep()");
}
*/

void* rig_interface::_static_poll_thread_function(void* arg)    // arg is the this pointer, in order to allow static member access to a real object
{ rig_interface* bufp = static_cast<rig_interface*>(arg);

  bufp->_poll_thread_function(nullptr);

  return nullptr;
}

void* rig_interface::_poll_thread_function(void* vp)
{ while (true)
  { _status.freq(rig_frequency());
    _status.mode(rig_mode());

//    _msec_sleep(_rig_poll_interval);
    sleep_for(milliseconds(_rig_poll_interval));
  }

  return nullptr;
}

void rig_interface::_error_alert(const string& msg)
{ if (_error_alert_function)
    (*_error_alert_function)(msg);
}

// ---------------------------------------- rig_interface -------------------------

/*!     \class rig_interface
        \brief The interface to a rig
*/

// constructor
rig_interface::rig_interface (void /* const string& serial_port */) :
  _port_name(nullptr),
  _rigp(nullptr),
  _rig_poll_interval(1000),             // poll once per second
  _status(frequency(), MODE_CW),            // frequency and mode are set to defaults prior to reading from rig
  _rig_connected(false),                // no rig is connected
  _error_alert_function(nullptr),       // no default error handler
  _model(RIG_MODEL_DUMMY),
  _last_commanded_frequency(),
  _last_commanded_mode(MODE_CW)
{
#if 0
  const string serial_port = context.rig1_port();

/*

 int fd = ::open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);

  cout << "fd = " << fd << endl;

  struct termios  config;

  //
  // Input flags - Turn off input processing
  // convert break to null byte, no CR to NL translation,
  // no NL to CR translation, don't mark parity errors or breaks
  // no input parity check, don't strip high bit off,
  // no XON/XOFF software flow control
  //
  config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                      INLCR | PARMRK | INPCK | ISTRIP | IXON);
  //
  // Output flags - Turn off output processing
  // no CR to NL translation, no NL to CR-NL translation,
  // no NL to CR translation, no column 0 CR suppression,
  // no Ctrl-D suppression, no fill characters, no case mapping,
  // no local output processing
  //
  // config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
  //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
  config.c_oflag = 0;
  //
  // No line processing:
  // echo off, echo newline off, canonical mode off,
  // extended input processing off, signal chars off
  //
  config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
  //
  // Turn off character processing
  // clear current char size mask, no parity checking,
  // no output processing, force 8 bit input
  //
  config.c_cflag &= ~(CSIZE | PARENB);
  config.c_cflag |= CS8;
  //
  // One input byte is enough to return from read()
  // Inter-character timer off
  //
  config.c_cc[VMIN]  = 1;
  config.c_cc[VTIME] = 0;
  //
  // Communication speed (simple version, using the predefined
  // constants)
  //
  if(cfsetispeed(&config, B4800) < 0 || cfsetospeed(&config, B4800) < 0)
  {
      cout << "speed error" << endl;
  }
  //
  // Finally, apply the configuration
  //
   cout << "set attributes = " << tcsetattr(fd, TCSAFLUSH, &config) << endl;

   char* c_out = "ID;";
   char* c_in = new char [1000];


   write(fd, c_out, 3);
   sleep(1);
   int n_read = read(fd, c_in, 500);

   cout << "Received: " << n_read << " characters: " << endl;

   if (n_read > 0)
  { for (size_t n = 0; n < n_read; ++n)
     cout << c_in[n];
    cout << endl;
  }

   close(fd);
*/

  rig_set_debug(RIG_DEBUG_NONE);

  rig_load_all_backends();

  const string rig_type = context.rig1_type();

// ugly map of name to hamlib model number
//  rig_model_t rig_model_nr = 0;

  if (rig_type == "K3")
  { //rig_model_nr = RIG_MODEL_K3;
    _model = RIG_MODEL_K3;
  }

  if (!_model)
    throw rig_interface_error(RIG_NO_SUCH_RIG, "Unknown rig: " + rig_type);

//  const rig_model_t myrig_model = rig_probe(&_port);
//  const rig_model_t myrig_model = RIG_MODEL_K3;            // http://hamlib.sourceforge.net/manuals/1.2.14/riglist_8h.html#a9d6b6a604733ada9ee1c9fc08e8c16da
//  cout << "rig model number: " << myrig_model << endl;

  _rigp = rig_init(_model);

  if (!_rigp)
    throw rig_interface_error(RIG_UNABLE_TO_INITIALISE, "Unable to initialise rig structure for rig type " + rig_type);

  baud_rate(context.rig1_baud());
  data_bits(context.rig1_data_bits());
  stop_bits(context.rig1_stop_bits());

//  cout << " parameters: baud rate:  " << baud_rate() << "; data bits: " << data_bits() << "; stop bits: " << stop_bits() << endl;


//  for (size_t n = 0; n <= serial_port.length(); ++n)
//    _rigp->state.rigport.pathname[n] = _port_name[n];

  strncpy(_rigp->state.rigport.pathname, serial_port.c_str(), FILPATHLEN);
//  _rigp->state.rigport.parm.serial.rate = 4800;

//  cout << "about to open rig" << endl;
  const int status = rig_open(_rigp);

  if (status != RIG_OK)
  { //cout << "unable to open rig" << endl;
    throw rig_interface_error(RIG_UNABLE_TO_OPEN, "Unable to open the rig on port " + serial_port + ": Error = " + to_string(status));
  }
//  else
//    cout << "rig opened OK" << endl;
#endif

#if 0
  int fd = ::open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);

  cout << "fd = " << fd << endl;

  struct termios  config;
  config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                      INLCR | PARMRK | INPCK | ISTRIP | IXON);
  config.c_oflag = 0;
  config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
  config.c_cflag &= ~(CSIZE | PARENB);
  config.c_cflag |= CS8;
  config.c_cc[VMIN]  = 1;
  config.c_cc[VTIME] = 0;
  if(cfsetispeed(&config, B4800) < 0 || cfsetospeed(&config, B4800) < 0)
  {
      cout << "speed error" << endl;
  }
   cout << "set attributes = " << tcsetattr(fd, TCSAFLUSH, &config) << endl;

   char* c_out = "IF;";
   char* c_in = new char [1000];


   write(fd, c_out, 3);
   sleep(1);
   int n_read = read(fd, c_in, 500);

   cout << "Received: " << n_read << " characters: " << endl;

   if (n_read > 0)
  { for (size_t n = 0; n < n_read; ++n)
     cout << c_in[n];
    cout << endl;
  }

   cout << "setting frequency via hamlib; fd still open" << endl;
   frequency(28000000);

   write(fd, c_out, 3);
   sleep(1);
   n_read = read(fd, c_in, 500);

   cout << "Received: " << n_read << " characters: " << endl;

   if (n_read > 0)
  { for (size_t n = 0; n < n_read; ++n)
     cout << c_in[n];
    cout << endl;
  }

   close(fd);
   cout << "setting frequency via hamlib; fd closed" << endl;
   frequency(21000000);
#endif    // 0

}

// destructor
rig_interface::~rig_interface(void)
{ if (_port_name)
    delete [] _port_name;
}

/// prepare rig for use
void rig_interface::prepare(const drlog_context& context)
{ const string serial_port = context.rig1_port();

/*

 int fd = ::open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);

  cout << "fd = " << fd << endl;

  struct termios  config;

  //
  // Input flags - Turn off input processing
  // convert break to null byte, no CR to NL translation,
  // no NL to CR translation, don't mark parity errors or breaks
  // no input parity check, don't strip high bit off,
  // no XON/XOFF software flow control
  //
  config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                      INLCR | PARMRK | INPCK | ISTRIP | IXON);
  //
  // Output flags - Turn off output processing
  // no CR to NL translation, no NL to CR-NL translation,
  // no NL to CR translation, no column 0 CR suppression,
  // no Ctrl-D suppression, no fill characters, no case mapping,
  // no local output processing
  //
  // config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
  //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
  config.c_oflag = 0;
  //
  // No line processing:
  // echo off, echo newline off, canonical mode off,
  // extended input processing off, signal chars off
  //
  config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
  //
  // Turn off character processing
  // clear current char size mask, no parity checking,
  // no output processing, force 8 bit input
  //
  config.c_cflag &= ~(CSIZE | PARENB);
  config.c_cflag |= CS8;
  //
  // One input byte is enough to return from read()
  // Inter-character timer off
  //
  config.c_cc[VMIN]  = 1;
  config.c_cc[VTIME] = 0;
  //
  // Communication speed (simple version, using the predefined
  // constants)
  //
  if(cfsetispeed(&config, B4800) < 0 || cfsetospeed(&config, B4800) < 0)
  {
      cout << "speed error" << endl;
  }
  //
  // Finally, apply the configuration
  //
   cout << "set attributes = " << tcsetattr(fd, TCSAFLUSH, &config) << endl;

   char* c_out = "ID;";
   char* c_in = new char [1000];


   write(fd, c_out, 3);
   sleep(1);
   int n_read = read(fd, c_in, 500);

   cout << "Received: " << n_read << " characters: " << endl;

   if (n_read > 0)
  { for (size_t n = 0; n < n_read; ++n)
     cout << c_in[n];
    cout << endl;
  }

   close(fd);
*/

  rig_set_debug(RIG_DEBUG_NONE);

  rig_load_all_backends();

  const string rig_type = context.rig1_type();

// ugly map of name to hamlib model number
  if (rig_type == "K3")
    _model = RIG_MODEL_K3;

  if (_model == RIG_MODEL_DUMMY and !rig_type.empty())
    _error_alert("Unknown rig: " + rig_type);

  _rigp = rig_init(_model);

  if (!_rigp)
    throw rig_interface_error(RIG_UNABLE_TO_INITIALISE, "Unable to initialise rig structure for rig type " + (rig_type.empty() ? "DUMMY" : rig_type));

  if (_model != RIG_MODEL_DUMMY)        // hamlib documentation is unclear as to whether this is necessary; probably not
  { baud_rate(context.rig1_baud());
    data_bits(context.rig1_data_bits());
    stop_bits(context.rig1_stop_bits());

    strncpy(_rigp->state.rigport.pathname, serial_port.c_str(), FILPATHLEN);
  }

  const int status = rig_open(_rigp);

  if (status != RIG_OK)
  { const string msg = "Unable to open the rig on port " + serial_port + ": Error = " + to_string(status);

    _error_alert(msg);
    ost << msg << endl;
  }

  if ((status == RIG_OK) and (_model != RIG_MODEL_DUMMY))
    _rig_connected = true;
}

// set frequency -- do nothing if zero is passed
void rig_interface::rig_frequency(const frequency& f)
{ if (f.hz())
  { _last_commanded_frequency = f;

    if (_rig_connected)
    { int status;

      { SAFELOCK(_rig);

        status = rig_set_freq(_rigp, RIG_VFO_CURR, f.hz());
      }

      if (status != RIG_OK)
        _error_alert("Error setting frequency");
    }
  }
}

// set frequency -- do nothing if zero is passed
void rig_interface::rig_frequency_b(const frequency& f)
{ if (f.hz())
  { _last_commanded_frequency_b = f;

    if (_rig_connected)
    { int status;

      { SAFELOCK(_rig);

        status = rig_set_freq(_rigp, RIG_VFO_B, f.hz());
      }

      if (status != RIG_OK)
        _error_alert("Error setting frequency");
    }
  }
}

void rig_interface::rig_mode(const MODE m)
{_last_commanded_mode = m;

  if (_rig_connected)
  { rmode_t hamlib_m = RIG_MODE_CW;

    if (m == MODE_SSB)
      hamlib_m = ( (rig_frequency().mhz() < 10) ? RIG_MODE_LSB : RIG_MODE_USB );

    int status;

    {
// hamlib not-very-intelligently sets both the mode and the bandwidth in a single command
      pbwidth_t tmp_bandwidth;
      rmode_t tmp_mode;

      { SAFELOCK(_rig);
        status = rig_get_mode(_rigp, RIG_VFO_CURR, &tmp_mode, &tmp_bandwidth);
      }

      if (status != RIG_OK)
        _error_alert("Error getting mode prior to setting mode");
      else
      {
        { SAFELOCK(_rig);
          status = rig_set_mode(_rigp, RIG_VFO_CURR, hamlib_m, ( (tmp_mode == hamlib_m) ? tmp_bandwidth : rig_passband_normal(_rigp, hamlib_m) ));
        }

        if (status != RIG_OK)
          _error_alert("Error setting mode");
      }
    }
  }
}

// get frequency
const frequency rig_interface::rig_frequency(void)
{ if (!_rig_connected)
    return _last_commanded_frequency;
  else
  { freq_t hz;
    SAFELOCK(_rig);

    const int status = rig_get_freq(_rigp, RIG_VFO_CURR, &hz);

    if (status != RIG_OK)
    { _error_alert("Error getting frequency");
      return _last_commanded_frequency;
    }

    return frequency(hz);
  }
}

// get frequency
const frequency rig_interface::rig_frequency_b(void)
{ if (!_rig_connected)
    return _last_commanded_frequency_b;
  else
  { freq_t hz;
    SAFELOCK(_rig);

    const int status = rig_get_freq(_rigp, RIG_VFO_B, &hz);

    if (status != RIG_OK)
    { _error_alert("Error getting frequency of VFO B");
      return _last_commanded_frequency_b;
    }

    return frequency(hz);
  }
}

// get/set baud rate
void rig_interface::baud_rate(const unsigned int rate)
{ if (_rigp)
  { SAFELOCK(_rig);

    _rigp->state.rigport.parm.serial.rate = rate;
  }
}

const unsigned int rig_interface::baud_rate(void)
{ SAFELOCK(_rig);

  return (_rigp ? _rigp->state.rigport.parm.serial.rate : 0);
}

// get/set number of data bits
void rig_interface::data_bits(const unsigned int bits)
{ if (bits < 7 or bits > 8)
    throw rig_interface_error(RIG_INVALID_DATA_BITS, "Attempt to set invalid number of data bits: " + to_string(bits));

  SAFELOCK(_rig);

  if (_rigp)
    _rigp->state.rigport.parm.serial.data_bits = bits;
}

const unsigned int rig_interface::data_bits(void)
{ SAFELOCK(_rig);

  return (_rigp ? _rigp->state.rigport.parm.serial.data_bits : 0);
}

// get/set number of stop bits
void rig_interface::stop_bits(const unsigned int bits)
{ if (bits < 1 or bits > 2)
    throw rig_interface_error(RIG_INVALID_STOP_BITS, "Attempt to set invalid number of stop bits: " + to_string(bits));

  SAFELOCK(_rig);

  if (_rigp)
    _rigp->state.rigport.parm.serial.stop_bits = bits;
}

const unsigned int rig_interface::stop_bits(void)
{ SAFELOCK(_rig);

  return (_rigp ? _rigp->state.rigport.parm.serial.stop_bits : 0);
}

const MODE rig_interface::rig_mode(void)
{ if (!_rig_connected)
    return _last_commanded_mode;
  else
  { rmode_t   m;
    pbwidth_t w;

    SAFELOCK(_rig);

    const int status = rig_get_mode(_rigp, RIG_VFO_CURR, &m, &w);

    if (status != RIG_OK)
    { _error_alert("Error getting mode");
      return _last_commanded_mode;
    }

    switch (m)
    { case RIG_MODE_CW :
        return MODE_CW;

      case RIG_MODE_LSB :
      case RIG_MODE_USB :
        return MODE_SSB;

      default :
        return MODE_CW;
    }
  }
}

// set RIT/XIT
void rig_interface::rit(const int hz)
{ ost << "RIT should be set to " << hz << endl;

// do nothing if no rig is connected
  if (!_rig_connected)
    return;

// hamlib's behaviour anent the K3 is not what we want
  if (_model == RIG_MODEL_K3)
  { if (hz == 0)                                // just clear the RIT/XIT
      raw_command(string("RC;"));
    else
    { const int positive_hz = abs(hz);
      string hz_str = pad_string(to_string(positive_hz), 4, PAD_LEFT, '0');

      if (hz >= 0)
        hz_str = string("+") + hz_str;
      else
        hz_str = string("-") + hz_str;

      raw_command(string("RO") + hz_str +";", false);
    }
  }
  else
  { SAFELOCK(_rig);

    const int status = rig_set_rit(_rigp, RIG_VFO_CURR, hz);

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error while setting RIT offset");
  }
}

const int rig_interface::rit(void)
{ if (_model == RIG_MODEL_K3)
  { const string value = raw_command(string("RO;"), 8);
    const int rv = from_string<int>(substring(value, 2, 5));

    return rv;
  }
  else
  { SAFELOCK(_rig);

    shortfreq_t hz;
    const int status = rig_get_rit(_rigp, RIG_VFO_CURR, &hz);

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error while getting RIT offset");

    return static_cast<int>(hz);
  }
}

// this is a kludge, since hamlib equates an offset with rit turned off (!)
void rig_interface::rit_enable(void)
{ if (_model == RIG_MODEL_K3)
    raw_command(string("RT1;"), 0);
  else
    rit(1);                 // 1 Hz offset
}

// this is a kludge, since hamlib equates an offset with rit turned off (!)
void rig_interface::rit_disable(void)
{ if (_model == RIG_MODEL_K3)
    raw_command(string("RT0;"), 0);
  else
    rit(0);
}

const bool rig_interface::rit_enabled(void)
{ string response;

  if (_model == RIG_MODEL_K3)
    response = raw_command(string("RT;"), true);

  if (response.length() != 4)
    throw rig_interface_error(RIG_UNEXPECTED_RESPONSE, "Invalid length in rit_enabled()");  // handle this error upstairs

  return (response[2] == '1');
}

// this is a kludge, since hamlib equates an offset with rit turned off (!)
void rig_interface::xit_enable(void)
{ if (_model == RIG_MODEL_K3)
    raw_command(string("XT1;"), 0);
  else
    xit(1);                 // 1 Hz offset
}

// this is a kludge, since hamlib equates an offset with rit turned off (!)
void rig_interface::xit_disable(void)
{ if (_model == RIG_MODEL_K3)
    raw_command(string("XT0;"), 0);
  else
    xit(1);                 // 1 Hz offset
}

const bool rig_interface::xit_enabled(void)
{ string response;

  if (_model == RIG_MODEL_K3)
    response = raw_command(string("XT;"), true);

  if (response.length() != 4)
    throw rig_interface_error(RIG_UNEXPECTED_RESPONSE, "Invalid length in xit_enabled()");  // handle this error upstairs

  return (response[2] == '1');
}

// set XIT; on the K3 this also sets the RIT
void rig_interface::xit(const int hz)
{ // hamlib's behaviour anent the K3 is not what we want
  if (_model == RIG_MODEL_K3)
  { if (hz == 0)                                // just clear the RIT/XIT
      raw_command(string("RC;"), 0);
    else
    { const int positive_hz = abs(hz);
      string hz_str = pad_string(to_string(positive_hz), 4, PAD_LEFT, '0');

      if (hz >= 0)
        hz_str = string("+") + hz_str;
      else
        hz_str = string("-") + hz_str;

      raw_command(string("RO") + hz_str +";", 0);
    }
  }
  else
  { SAFELOCK(_rig);

    const int status = rig_set_xit(_rigp, RIG_VFO_CURR, hz);

    if (status != RIG_OK)
    { throw exception();
    }
  }
}

const int rig_interface::xit(void)
{ shortfreq_t hz;
  SAFELOCK(_rig);

  const int status = rig_get_xit(_rigp, RIG_VFO_CURR, &hz);

  if (status != RIG_OK)
    throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error obtaining XIT offset");

  return static_cast<int>(hz);
}

// lock the VFO
void rig_interface::lock(void)
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
    raw_command("LK1;", 0);
  else
  { const int v = 1;

    const int status = rig_set_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, v);

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error locking VFO");
  }
}

// unlock the VFO
void rig_interface::unlock(void)
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
    raw_command("LK0;", 0);
  else
  { const int v = 0;

    const int status = rig_set_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, v);

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error unlocking VFO");
  }
}

// sub-receiver on/off
void rig_interface::sub_receiver(const bool b)
{ if (_model == RIG_MODEL_K3)
    raw_command( b ? "SB1;" : "SB0;", 0);
}

// is sub-receiver on?
const bool rig_interface::sub_receiver(void)
{ if (_model == RIG_MODEL_K3)
  { try
    { const string str = raw_command("SB;", true);

      if (str.length() < 3)
        throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error getting sub-receiver status");

      return (str[2] == '1');
    }

    catch (...)
    { throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error getting sub-receiver status");
    }
  }

  return false;    // keep compiler happy
}

// most recent rig status
const rig_status rig_interface::status(void)
{ SAFELOCK(_rig);

  return _status;
}

// set the keyer speed in WPM
void rig_interface::keyer_speed(const int wpm)
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { string cmd = "KS" + pad_string(to_string(wpm), 3, PAD_LEFT, '0') + ";";
    raw_command(cmd, 0);
  }
  else
  { value_t v;
    v.i = wpm;

    const int status = rig_set_level(_rigp, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, v);

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error setting keyer speed");
  }
}

// get the keyer speed in WPM
const int rig_interface::keyer_speed(void)
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { const string status_str = raw_command("KS;", 6);

    return from_string<int>(substring(status_str, 2, 3));
  }
  else
  { value_t v;
    const int status = rig_get_level(_rigp, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, &v);

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error getting keyer speed");

    return v.i;
  }
}


#if 0
// explicit K3 commands
const string rig_interface::raw_command(const string& cmd, const unsigned int msec)
{ struct rig_state* rs_p = &(_rigp->state);
  struct rig_state& rs   = *rs_p;
  const int fd           = _file_descriptor();
  char* c_in             = new char [1000];
  int n_read             = 0;

  { SAFELOCK(_rig_access);

    serial_flush(&rs_p->rigport);
    write(fd, cmd.c_str(), cmd.length());
    _msec_sleep(msec);
    n_read = read(fd, c_in, 500);
  }

  if (n_read >= 0)
  { c_in[n_read] = static_cast<char>(0);

    const string rv(c_in);
    delete [] c_in;

    return rv;
  }

  delete [] c_in;
  return string();
}

// is the VFO locked?
const bool rig_interface::is_locked(void)
{ const string status_str = raw_command("LK;");

  const char status_char = (status_str.length() >= 3 ? status_str[2] : '0');  // default is unlocked

  return (status_char == '1');
}
#endif    // 0

// explicit K3 commands
const string rig_interface::raw_command(const string& cmd, const bool response_expected)
{ struct rig_state* rs_p = &(_rigp->state);
  struct rig_state& rs   = *rs_p;
  const int fd           = _file_descriptor();
  static array<char, 1000> c_in;
  int n_read             = 0;
  unsigned int total_read         = 0;
  string rcvd;
  const bool is_p3_screenshot = (cmd == "#BMP;");   // this has to be treated differently: it is long and has no concluding semicolon

  if (!_rig_connected)
    return string();

  if (cmd.empty())
    return string();

  static const int max_attempts = 10;
  static const int timeout_microseconds = 100000;    // 100 milliseconds

// sanity check ... on K3 all commands end in a ";"
  if (cmd[cmd.length() - 1] != ';')
  {  _error_alert("Invalid rig command: " + cmd);
    return string();
  }

  bool completed = false;

  { SAFELOCK(_rig);

    serial_flush(&rs_p->rigport);
    write(fd, cmd.c_str(), cmd.length());
    serial_flush(&rs_p->rigport);
    sleep_for(milliseconds(100));

    fd_set set;
    struct timeval timeout;

    int counter = 0;

    if (response_expected)
    { if (is_p3_screenshot)
      { array<char, 131640> c_in;    // hide the static array

        const int n_bits = 131640 * 10;
        const int n_secs = n_bits / baud_rate();

        while (!completed and (counter < (n_secs + 5)) )    // add 5 extra seconds
        { FD_ZERO(&set);    // clear the set
          FD_SET(fd, &set); // add the file descriptor to the set

          timeout.tv_sec = 1;
          timeout.tv_usec = 0;

          int status = select(fd + 1, &set, NULL, NULL, &timeout);
          int nread = 0;

          if (status == -1)
            ost << "Error in select() in raw_command()" << endl;
          else
          { if (status == 0)
             ost << "timeout in select() in raw_command()" << endl;
            else
            { n_read = read(fd, c_in.data(), 131640 - total_read);

              ost << "n_read = " << n_read << endl;

              if (n_read > 0)                      // should always be true
              { total_read += n_read;
                rcvd.append(c_in.data(), n_read);

                if (rcvd.length() == 131640)
                  completed = true;
              }
            }
          }
          counter++;
          if (!completed)
          { static const string percent_str("%%");
            const int percent = rcvd.length() * 100 / 131640;

            _error_alert(string("P3 screendump progress: ") + to_string(percent) + percent_str);
//            ost << "not yet complete; counter now = " << counter << " and received length = " << rcvd.length() << endl;
            sleep_for(milliseconds(1000));  // we have the lock for all this time
          }
          else
            _error_alert("P3 screendump complete");
        }
      }
      else
      { while (!completed and (counter < max_attempts) )
        { FD_ZERO(&set);    // clear the set
          FD_SET(fd, &set); // add the file descriptor to the set

          timeout.tv_sec = 0;
          timeout.tv_usec = timeout_microseconds;

          int status = select(fd + 1, &set, NULL, NULL, &timeout);
          int nread = 0;

          if (status == -1)
            ost << "Error in select() in raw_command()" << endl;
          else
          { if (status == 0)
             ost << "timeout in select() in raw_command()" << endl;
            else
            { n_read = read(fd, c_in.data(), 500);        // read a maximum of 500 characters

              if (n_read > 0)                      // should always be true
              { total_read += n_read;
                c_in[n_read] = static_cast<char>(0);    // append a null byte
                rcvd += string(c_in.data());

                if (contains(rcvd, ";"))
                  completed = true;
              }
            }
          }
          counter++;
        }
      }
    }
  }

  if (response_expected and !completed)
    _error_alert("Incomplete response from rig to cmd: " + cmd + " length = " + to_string(rcvd.length()) + " " + rcvd);

  if (response_expected and completed)
    return rcvd;

  return string();
}

// is the VFO locked?
const bool rig_interface::is_locked(void)
{ if (_model == RIG_MODEL_K3)
  { const string status_str = raw_command("LK;", 4);
    const char status_char = (status_str.length() >= 3 ? status_str[2] : '0');  // default is unlocked

    return (status_char == '1');
  }
  else
  { SAFELOCK(_rig);

    int v;

    const int status = rig_get_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, &v);

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error getting lock status");

    return (v == 1);
  }
}

// get the bandwidth in Hz
const int rig_interface::bandwidth(void)
{ if (!_rig_connected)
    return 0;

  const string status_str = raw_command("BW;", 7);

  if (status_str.empty())
    return 0;

  return from_string<int>(substring(status_str, 2, 4)) * 10;
}

const frequency rig_interface::get_last_frequency(const BAND b, const MODE m)
{ SAFELOCK(_rig);

  const auto cit = _last_frequency.find( { b, m } );

  return ( ( cit == _last_frequency.cend() ) ? frequency() : cit->second );    // return 0 if there's no last frequency
}

void rig_interface::set_last_frequency(const BAND b, const MODE m, const frequency& f) noexcept
{ SAFELOCK(_rig);

  _last_frequency[ { b, m } ] = f;
}

const bool rig_interface::is_transmitting(void)
{ if (_rig_connected)
  { bool rv = true;                                        // be paranoid

    if (_model == RIG_MODEL_K3)
    { const string response = raw_command("TQ;", 4);

      if (response.length() < 4)
      /* { } */   _error_alert("Unable to determine whether rig is transmitting");
      else
        rv = (response[2] == '1');
    }

    return rv;
  }
  else
    return false;
}

// is rig in TEST mode?
const bool rig_interface::test(void)
{ if (_model == RIG_MODEL_DUMMY)
    return true;

  bool rv = false;    // default

  if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
    { const string response = raw_command("IC;", 8);

      if (response.length() < 8)
        _error_alert("Unable to retrieve rig icons and status");
      else
      { const char c = response[2];
        rv = (c bitand (1 << 5));
      }
    }
  }

  return rv;
}

void rig_interface::test(const bool b)
{ if (test() != b)
  { if (_rig_connected)
    { if (_model == RIG_MODEL_K3)
        raw_command("SWH18;");    // toggles state
    }
  }
}

// register a function for alerting the user
void rig_interface::register_error_alert_function(void (*error_alert_function)(const string&) )
{ SAFELOCK(_rig);
  _error_alert_function = error_alert_function;
}

map<pair<BAND, MODE>, frequency > DEFAULT_FREQUENCIES = { { { BAND_160, MODE_CW },  frequency(1800000) },
                                                          { { BAND_160, MODE_SSB }, frequency(1900000) },
                                                          { { BAND_80,  MODE_CW },  frequency(3500000) },
                                                          { { BAND_80,  MODE_SSB }, frequency(3750000) },
                                                          { { BAND_40,  MODE_CW },  frequency(7000000) },
                                                          { { BAND_40,  MODE_SSB }, frequency(7150000) },
                                                          { { BAND_20,  MODE_CW },  frequency(14000000) },
                                                          { { BAND_20,  MODE_SSB }, frequency(14150000) },
                                                          { { BAND_15,  MODE_CW },  frequency(21000000) },
                                                          { { BAND_15,  MODE_SSB }, frequency(21200000) },
                                                          { { BAND_10,  MODE_CW },  frequency(28000000) },
                                                          { { BAND_10,  MODE_SSB }, frequency(28300000) }
                                                        };

