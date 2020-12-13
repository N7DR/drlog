// $Id: rig_interface.cpp 176 2020-12-13 18:28:41Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   rig_interface.cpp

    Classes and functions related to transferring information between the computer and the rig.

    The user must be a member of the dialout group in order to use the serial port.
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

extern bool rig_is_split;

extern void alert(const string& msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);     ///< alert the user (not used for errors)

/* The current version of Hamlib seems to be both slow and unreliable with the K3. Anent unreliability, for example, the is_locked() function
 * as written below causes the entire program to freeze (presumably some kind of blocking or threading issue in the current version of hamlib).
 *
 * Anyway, as a result of this, we use explicit K3 control commands where appropriate. This may be changed if/when hamlib proves
 * itself sufficiently robust.
 *
 * Another issue is that there is simply no good detailed description of the expected behaviour corresponding to many hamlib
 * functions (for example, the split-related functions -- e.g., CURR_VFO is defined simply as "the current VFO", but it plainly is not,
 * as a few test function calls quickly demonstrate); nor, indeed, is there even a description of the theoretical transceiver that is modelled by
 * hamlib. There doesn't even appear to be a guarantee that if a function is "successful" (i.e., does not return an error), it
 * will behave identically on all rigs.
 *
 * There are other weirdnesses: polling the split status on the K3 actually SWAPS the frequencies of the two VFOs, even though
 * it is a *get* function. Life is too short to try to sort out all this stuff and to get it to work correctly, if that's even possible,
 * even just for the K3 -- especially as there is no guarantee that other rigs will respond the same way to the same (successful) calls.
 *
 * So, unfortunately, we end up using a lot of K3-specific calls. This was not my desire. But I got fed up of trying to make sense
 * of the current implementation of hamlib.
 */

/* But there are many problems with the K3 soi-disant protocol as well. Even after trying to get clear answers from Elecraft, the following
 * issues (at least) remain (after incorporating the Elecraft responses, which mostly did not address the questions I asked):
 *
 *   1. Which commands might cause a "?;" response if the K3 is "busy"?
 *   2. Do these include commands that don't normally return a response?
 *   3. Which commands do eventually get executed even if the K3 is "busy"?
 *   4. How long might one have to wait for some commands to execute if the K3 is "busy"?
 *   5. Which commands are subject to delay if the K3 is "busy"?
 *   6. What is the precise definition of "busy"?
 *
 *   All in all, the only thing to do seems to be to throw commands at the K3 and hope that they stick. It is impractical to
 *   check each time whether a command was executed, because the documentation says that certain commands (which it does not define)
 *   might take as long as half a second to execute if the K3 is "busy".
 *
 *   The fundamental problem in all this is that the protocol has no concept of a transaction. It is unclear why simple 1980s-era
 *   protocols are still being used to exchange information with rigs. Baud rates now are fast enough that, even for serial lines, real
 *   protocols could be run over, for example, SLIP.
*/

// ---------------------------------------- rig_interface -------------------------

/*! \brief       Alert the user with a message
    \param  msg  message for the user

    Calls <i>_error_alert_function</i> to perform the actual alerting
*/
void rig_interface::_error_alert(const string& msg)
{ if (_error_alert_function)
    (*_error_alert_function)(msg);
}

/*! \brief      Set frequency of a VFO
    \param  f   new frequency
    \param  v   VFO

    Does nothing if <i>f</i> is not within a ham band
*/
void rig_interface::_rig_frequency(const frequency& f, const VFO v)
{ constexpr std::chrono::milliseconds RETRY_TIME { milliseconds(100) };      // period between retries

  if (f.is_within_ham_band())
  { switch (v)
    { case VFO::A :
        _last_commanded_frequency = f;
        break;

      case VFO::B :
        _last_commanded_frequency_b = f;
        break;
    }

    if (_rig_connected)
    { SAFELOCK(_rig);           // hold the lock until we have received confirmation that the frequency is correct

      bool retry { true };

      while (retry)
      { //const int status { rig_set_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_A : RIG_VFO_B ), f.hz()) };

 //       status = rig_set_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_A : RIG_VFO_B ), f.hz());
 //       if (status == RIG_OK)
 //         _time_last_commanded_frequency = DRLOG_CLOCK::now();
 //       else
        if (const int status { rig_set_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_A : RIG_VFO_B ), f.hz()) }; status != RIG_OK)
          _error_alert("Error setting frequency of VFO "s + ((v == VFO::A) ? "A"s : "B"s));

        if (_rig_frequency(v) != f)     // explicitly check the frequency
          sleep_for(RETRY_TIME);
        else
          retry = false;
      }
    }
  }
}

/*! \brief      Get the frequency of a VFO
    \param  v   VFO
    \return     frequency of v
*/
frequency rig_interface::_rig_frequency(const VFO v)
{ if (!_rig_connected)
    return ( (v == VFO::A) ? _last_commanded_frequency : _last_commanded_frequency_b);
  else
  { freq_t hz;

    SAFELOCK(_rig);

    if (const int status { rig_get_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_CURR : RIG_VFO_B ), &hz) }; status != RIG_OK)
    { _error_alert("Error getting frequency of VFO "s + ((v == VFO::A) ? "A"s : "B"s));
      return ( (v == VFO::A) ? _last_commanded_frequency : _last_commanded_frequency_b) ;
    }

    return frequency(hz);
  }
}

/*! \brief      Thread function used to poll rig for status, forever
    \param  vp  unused (should be nullptr)
    \return     nullptr

    Sets the frequency and mode in the <i>_status</i> object

    *** Should be modified to provide guaranteed graceful exit during program shutdown
*/
void* rig_interface::_poll_thread_function(void* vp)
{ while (true)
  { _status.freq(rig_frequency());
    _status.mode(rig_mode());

    sleep_for(milliseconds(_rig_poll_interval));
  }

  return nullptr;
}

/*! \brief          static wrapper for function to poll rig for status
    \param  this_p  the <i>this</i> pointer, in order to allow static member access to a real object
    \return         nullptr
*/
void* rig_interface::_static_poll_thread_function(void* this_p)
{ rig_interface* bufp { static_cast<rig_interface*>(this_p) };

  bufp->_poll_thread_function(nullptr);

  return nullptr;
}

// ---------------------------------------- rig_interface -------------------------

/*! \class  rig_interface
    \brief  The interface to a rig
*/

/*! \brief              Prepare rig for use
    \param  context     context for the contest
*/
void rig_interface::prepare(const drlog_context& context)
{ _port_name = context.rig1_port();
  rig_set_debug(RIG_DEBUG_NONE);
  rig_load_all_backends();              // this function returns an int -- in true Linux fashion, there is no documentation as to possible values and their meaning
                                        // see: http://hamlib.sourceforge.net/manuals/1.2.15/group__rig.html

  const string rig_type { context.rig1_type() };

// ugly map of name to hamlib model number
  if (rig_type == "K3"s)
    _model = RIG_MODEL_K3;

  if (_model == RIG_MODEL_DUMMY and !rig_type.empty())
    _error_alert("Unknown rig type: "s + rig_type);

  _rigp = rig_init(_model);

  if (!_rigp)
    throw rig_interface_error(RIG_UNABLE_TO_INITIALISE, "Unable to initialise rig structure for rig type "s + (rig_type.empty() ? "DUMMY"s : rig_type));

  if (_model != RIG_MODEL_DUMMY)        // hamlib documentation is unclear as to whether this is necessary; probably not
  { baud_rate(context.rig1_baud());
    data_bits(context.rig1_data_bits());
    stop_bits(context.rig1_stop_bits());

    if (_port_name.size() >= (FILPATHLEN - 2))                        // ridiculous C-ism
    { const string msg { "Port name is too long: "s + _port_name };

      _error_alert(msg);
      ost << msg << endl;
      exit(-1);
    }

    strncpy(_rigp->state.rigport.pathname, _port_name.c_str(), FILPATHLEN - 1);     // !! -1 to remove compiler warning about length
  }

  const int status { rig_open(_rigp) };

  if (status != RIG_OK)
  { const string msg { "Unable to open the rig on port "s + _port_name + ": Error = "s + hamlib_error_code_to_string(status) };

    _error_alert(msg);
    ost << msg << endl;
    throw rig_interface_error(RIG_UNABLE_TO_OPEN, "Error opening rig: " + rig_type);
  }

  if ((status == RIG_OK) and (_model != RIG_MODEL_DUMMY))
    _rig_connected = true;
}

/*! \brief      Set mode
    \param  m   new mode

    Also sets the bandwidth (because it's easier to follow hamlib's model, even though it is obviously flawed)
*/
void rig_interface::rig_mode(const MODE m)
{ static pbwidth_t last_cw_bandwidth  { 200 };
  static pbwidth_t last_ssb_bandwidth { 1800 };

  constexpr std::chrono::milliseconds RETRY_TIME { milliseconds(10) };  // wait time if a retry is necessary

  _last_commanded_mode = m;

  if (_rig_connected)
  { 
// set correct hamlib mode 
    rmode_t hamlib_m { RIG_MODE_CW };

    if (m == MODE_SSB)
      hamlib_m = ( (rig_frequency().mhz() < 10) ? RIG_MODE_LSB : RIG_MODE_USB );

    int status;

// hamlib, for reasons I can't guess, sets both the mode and the bandwidth in a single command
    pbwidth_t tmp_bandwidth;
    rmode_t   tmp_mode;

    { SAFELOCK(_rig);
      status = rig_get_mode(_rigp, RIG_VFO_CURR, &tmp_mode, &tmp_bandwidth);
    }

    if (status != RIG_OK)
      _error_alert("Error getting mode prior to setting mode");
    else
    {  switch (tmp_mode)
      { case RIG_MODE_CW:
          last_cw_bandwidth = tmp_bandwidth;
          break;

        case RIG_MODE_LSB:
        case RIG_MODE_USB:
          last_ssb_bandwidth = tmp_bandwidth;
          break;

        default:
          break;
      }

      bool retry { true };

      SAFELOCK(_rig);       // hold the lock until we receive positive confirmation that the rig is properly set to correct mode and bandwidth

      while (retry)
      { const pbwidth_t new_bandwidth    { ( (m == MODE_SSB) ? last_ssb_bandwidth : last_cw_bandwidth ) };
        const pbwidth_t bandwidth_to_set { (tmp_mode == hamlib_m) ? tmp_bandwidth : new_bandwidth };

        status = rig_set_mode(_rigp, RIG_VFO_CURR, hamlib_m, bandwidth_to_set) ;

        if (status != RIG_OK)
          _error_alert("Error setting mode"s);

        status = rig_get_mode(_rigp, RIG_VFO_CURR, &tmp_mode, &tmp_bandwidth);

        if (status != RIG_OK)
          _error_alert("Error getting mode after setting mode"s);

        if ( (tmp_mode != hamlib_m) or (tmp_bandwidth != new_bandwidth) )   // explicitly check the mode and bandwidth
          sleep_for(RETRY_TIME);
        else
          retry = false;                                                    // we're done
      }
    }
  }
}

/*! \brief  Enable split operation

    hamlib has no good definition of exactly what split operation really means, and, hence,
    has no clear description of precisely what the hamlib rig_set_split_vfo() function is supposed
    to do for various values of the permitted parameters. There is general agreement on the reflector
    that the call contained herein *should* do the "right" thing -- but since there's no precise definition
    of any of this, not all backends are guaranteed to behave the same.

    Hence we use the explicit K3 command, since at least we know what that will do on that rig.
*/
void rig_interface::split_enable(void)
{ if (!_rig_connected)
    return;

  SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { raw_command("FT1;"s);
    rig_is_split = true;
    return;
  }

// not a K3
  if (const int status { rig_set_split_vfo(_rigp, RIG_VFO_CURR, RIG_SPLIT_ON, RIG_VFO_B) }; status != RIG_OK)  // magic parameters
    _error_alert("Error executing SPLIT command"s);
  else
    rig_is_split = true;
}

/// disable split operation; see caveats under split_enable()
void rig_interface::split_disable(void)
{ if (!_rig_connected)
    return;

  SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { raw_command("FR0;"s);
    rig_is_split = false;
    return;
  }

// not a K3
//  const int status = rig_set_split_vfo(_rigp, RIG_VFO_CURR, RIG_SPLIT_OFF, RIG_VFO_A);    // do not delete this line, in case we ever need to use this version instead of the following line
//  const int status { rig_set_split_vfo(_rigp, RIG_VFO_A, RIG_SPLIT_OFF, RIG_VFO_A) };     // the line above also works

  if (const int status { rig_set_split_vfo(_rigp, RIG_VFO_A, RIG_SPLIT_OFF, RIG_VFO_A) }; status != RIG_OK)
    _error_alert("Error executing SPLIT command"s);
  else
    rig_is_split = false;
}

/*! \brief      Is split enabled?
    \return     whether split is enabled on the rig

    This interrogates the rig; it neither reads not writes the variable rig_is_split
*/
bool rig_interface::split_enabled(void)
{ if (!_rig_connected)
    return false;

  if (_model == RIG_MODEL_K3)
  { SAFELOCK(_rig);

    if (const string transmit_vfo { raw_command("FT;"s, RESPONSE::EXPECTED) }; transmit_vfo.length() >= 4)
      return (transmit_vfo[2] == '1');

    _error_alert("Unable to determine whether rig is SPLIT"s);
    return false;
  }

// not a K3
  split_t split_mode;
  vfo_t   tx_vfo;

  SAFELOCK(_rig);

  if (const int status { rig_get_split_vfo(_rigp, RIG_VFO_B, &split_mode, &tx_vfo) }; status != RIG_OK)
  { _error_alert("Error getting SPLIT"s);
    return false;
  }

  return (split_mode == RIG_SPLIT_ON);
}

/*! \brief          Set baud rate
    \param  rate    baud rate to which the rig should be set
*/
void rig_interface::baud_rate(const unsigned int rate)
{ if (_rigp)
  { SAFELOCK(_rig);
    _rigp->state.rigport.parm.serial.rate = rate;
  }
}

/*! \brief      Get baud rate
    \return     rig baud rate
*/
unsigned int rig_interface::baud_rate(void)
{ SAFELOCK(_rig);
  return (_rigp ? _rigp->state.rigport.parm.serial.rate : 0);
}

/*! \brief          Set the number of data bits (7 or 8)
    \param  bits    the number of data bits to which the rig should be set

    Throws exception if <i>bits</i> is not 7 or 8
*/
void rig_interface::data_bits(const unsigned int bits)
{ if ( (bits < 7) or (bits > 8) )
    throw rig_interface_error(RIG_INVALID_DATA_BITS, "Attempt to set invalid number of data bits: "s + to_string(bits));

  SAFELOCK(_rig);

  if (_rigp)
    _rigp->state.rigport.parm.serial.data_bits = bits;
}

/*! \brief      Get the number of data bits
    \return     number of data bits
*/
unsigned int rig_interface::data_bits(void)
{ SAFELOCK(_rig);

  return (_rigp ? _rigp->state.rigport.parm.serial.data_bits : 0);
}

/*! \brief          Set the number of stop bits (1 or 2)
    \param  bits    the number of stop bits to which the rig should be set

    Throws exception if <i>bits</i> is not 1 or 2
*/
void rig_interface::stop_bits(const unsigned int bits)
{ if ( (bits < 1) or (bits > 2) )
    throw rig_interface_error(RIG_INVALID_STOP_BITS, "Attempt to set invalid number of stop bits: "s + to_string(bits));

  SAFELOCK(_rig);

  if (_rigp)
    _rigp->state.rigport.parm.serial.stop_bits = bits;
}

/*! \brief      Get the number of stop bits
    \return     number of stop bits
*/
unsigned int rig_interface::stop_bits(void)
{ SAFELOCK(_rig);

  return (_rigp ? _rigp->state.rigport.parm.serial.stop_bits : 0);
}

/// get the rig's mode
MODE rig_interface::rig_mode(void)
{ if (!_rig_connected)
    return _last_commanded_mode;
  else
  { rmode_t   m;
    pbwidth_t w;

    SAFELOCK(_rig);

    if (const int status { rig_get_mode(_rigp, RIG_VFO_CURR, &m, &w) }; status != RIG_OK)
    { _error_alert("Error getting mode"s);
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

/*! \brief      Set rit offset (in Hz)
    \param  hz  offset in Hz
*/
void rig_interface::rit(const int hz)
{ if (!_rig_connected)                          // do nothing if no rig is connected
    return;

// hamlib's behaviour anent the K3 is brain dead if hz == 0
  if (_model == RIG_MODEL_K3)
  { if (hz == 0)                                // just clear the RIT/XIT
      raw_command("RC;"s);
    else
    { const int    positive_hz { abs(hz) };
//      const string hz_str      { ( (hz >= 0) ? "+"s : "-"s) + pad_left(to_string(positive_hz), 4, '0') };
      const string hz_str      { ( (hz >= 0) ? "+"s : "-"s) + pad_leftz(positive_hz, 4) };

      raw_command("RO"s + hz_str + ";"s);
    }
  }
  else
  { SAFELOCK(_rig);

    const int status { rig_set_rit(_rigp, RIG_VFO_CURR, hz) };

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error while setting RIT offset"s);
  }
}

/// get rit offset (in Hz)
int rig_interface::rit(void)
{ if (_model == RIG_MODEL_K3)
  { //const string value { raw_command("RO;"s, 8) };
    const string value { raw_command("RO;"s, RESPONSE::EXPECTED) };

    if (value.length() != 8)
    { _error_alert("Invalid rig response in rit(): "s + value);
      return 0;
    }

    return from_string<int>(substring(value, 2, 5));
  }
  else
  { SAFELOCK(_rig);

    shortfreq_t hz;

    if (const int status { rig_get_rit(_rigp, RIG_VFO_CURR, &hz) }; status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error while getting RIT offset"s);

    return static_cast<int>(hz);
  }
}

/*! \brief  Turn rit on

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
void rig_interface::rit_enable(void)
{ if (_model == RIG_MODEL_K3)
    raw_command("RT1;"s);           // proper enable for the K3
  else
    rit(1);                         // 1 Hz offset, since a zero offset would disable RIT
}

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
void rig_interface::rit_disable(void)
{ if (_model == RIG_MODEL_K3)
    raw_command("RT0;"s); // proper disable for the K3
  else
    rit(0);                         // 0 Hz offset, which hamlib regards as disabling RIT
}

/// is rit enabled?
bool rig_interface::rit_enabled(void)
{ switch (_model)
  { case RIG_MODEL_K3 :
    { const string response { raw_command("RT;"s, RESPONSE::EXPECTED) };

      if (response.length() != 4)
        throw rig_interface_error(RIG_UNEXPECTED_RESPONSE, "Invalid length in rit_enabled(): "s + response);  // handle this error upstairs
        
      return (response[2] == '1');
    }
    
    case RIG_MODEL_DUMMY :
      return false;
  }
  
  return false;         // default response
}

/*! \brief  Turn xit on

    This is a kludge, as hamlib equates an offset of zero with xit turned off (!)
*/
void rig_interface::xit_enable(void)
{ if (_model == RIG_MODEL_K3)
    raw_command("XT1;"s);
  else
    xit(1);                 // 1 Hz offset
}

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates an offset of zero with xit turned off (!)
*/
void rig_interface::xit_disable(void)
{ if (_model == RIG_MODEL_K3)
    raw_command("XT0;"s);
  else
    xit(1);                 // 1 Hz offset
}

/// is xit enabled?
bool rig_interface::xit_enabled(void)
{ switch (_model)
  { case RIG_MODEL_K3 :
    { const string response { raw_command("XT;"s, RESPONSE::EXPECTED) };

      if (response.length() != 4)
        throw rig_interface_error(RIG_UNEXPECTED_RESPONSE, "Invalid length in xit_enabled(): "s + response);  // handle this error upstairs

      return (response[2] == '1');
    }
    
    case RIG_MODEL_DUMMY :
      return false;
  }
  
  return false;         // default response
}

/*! \brief      Set xit offset (in Hz)
    \param  hz  offset in Hz

    On the K3 this also sets the RIT offset
*/
void rig_interface::xit(const int hz)
{ if (_model == RIG_MODEL_K3)                   // hamlib's behaviour anent the K3 is not what we want, have K3-specific code
  { if (hz == 0)                                // just clear the RIT/XIT
      raw_command("RC;"s);
    else
    { const int    positive_hz { abs(hz) };
      const string hz_str      { ( (hz >= 0) ? "+"s : "-"s ) + pad_left(to_string(positive_hz), 4, '0') };

      raw_command("RO"s + hz_str + ";"s);
    }
  }
  else
  { SAFELOCK(_rig);

    const int status { rig_set_xit(_rigp, RIG_VFO_CURR, hz) };

    if (status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error in xit(int)");  // handle this error upstairs
  }
}

/// get xit offset (in Hz)
int rig_interface::xit(void)
{ shortfreq_t hz;

  SAFELOCK(_rig);

  if (const int status { rig_get_xit(_rigp, RIG_VFO_CURR, &hz ) }; status != RIG_OK)
    throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error obtaining XIT offset"s);

  return static_cast<int>(hz);
}

/// lock the VFO
void rig_interface::lock(void)
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
    raw_command("LK1;"s, RESPONSE::NOT_EXPECTED);
  else
  { constexpr int v { 1 };

    if (const int status { rig_set_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, v) }; status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error locking VFO"s);
  }
}

/// unlock the VFO
void rig_interface::unlock(void)
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
    raw_command("LK0;"s, RESPONSE::NOT_EXPECTED);
  else
  { constexpr int v { 0 };

    if (const int status { rig_set_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, v) }; status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error unlocking VFO"s);
  }
}

/*! \brief      Turn sub-receiver on/off
    \param  b   turn sub-receiver on if TRUE, otherwise turn off
*/
void rig_interface::sub_receiver(const bool b)
{ if (_model == RIG_MODEL_K3)
    raw_command( (b ? "SB1;"s : "SB0;"s), RESPONSE::NOT_EXPECTED);
}

/// is sub-receiver on?
bool rig_interface::sub_receiver(void)
{ if (_model == RIG_MODEL_K3)
  { try
    { const string str { raw_command("SB;"s, RESPONSE::EXPECTED) };

      if (str.length() < 3)
        throw rig_interface_error(RIG_UNEXPECTED_RESPONSE, "SUBRX Short response"s);

      return (str[2] == '1');
    }

    catch (const rig_interface_error& e)
    { throw e;
    }

    catch (...)
    { throw rig_interface_error(RIG_MISC_ERROR, "Error getting SUBRX status"s);
    }
  }

  return false;    // keep compiler happy
}

/// toggle sub-receiver between on and off
void rig_interface::sub_receiver_toggle(void)
{ if (sub_receiver_enabled())
    sub_receiver_disable();
  else
    sub_receiver_enable();
}

/// return most recent rig status
rig_status rig_interface::status(void)
{ SAFELOCK(_rig);

  return _status;
}

/*! \brief          Set the keyer speed
    \param  wpm     keyer speed in WPM
*/
void rig_interface::keyer_speed(const int wpm)
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { string cmd { "KS"s + pad_left(to_string(wpm), 3, '0') + ";"s };

    raw_command(cmd, RESPONSE::NOT_EXPECTED);
  }
  else
  { value_t v;

    v.i = wpm;

    if (const int status { rig_set_level(_rigp, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, v) }; status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error setting keyer speed"s);
  }
}

/// get the keyer speed in WPM
int rig_interface::keyer_speed(void)
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { const string status_str { raw_command("KS;"s, RESPONSE::EXPECTED, 6) };

    return from_string<int>(substring(status_str, 2, 3));
  }
  else
  { value_t v;

    if (const int status { rig_get_level(_rigp, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, &v) }; status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error getting keyer speed"s);

    return v.i;
  }
}

// explicit K3 commands
#if !defined(NEW_RAW_COMMAND)

/*! \brief                  Send a raw command to the rig
    \param  cmd             the command to send
    \param  expectation     whether a response is expected
    \param  expected_len    expected length of response
    \return                 the response from the rig, or the empty string

    Currently any expected length is ignored; the routine looks for the concluding ";" instead
*/
string rig_interface::raw_command(const string& cmd, const RESPONSE expectation, const int expected_len)
{ const bool response_expected { expectation == RESPONSE::EXPECTED };

  struct rig_state* rs_p { &(_rigp->state) };

  const int fd { _file_descriptor() };

  constexpr int INBUF_SIZE { 1000 };        // size of input buffer

  static array<char, INBUF_SIZE> c_in;

  unsigned int total_read         { 0 };

  string rcvd;

  const bool is_p3_screenshot { (cmd == "#BMP;"s) };   // this has to be treated differently: the response is long and has no concluding semicolon

  if (!_rig_connected)
    return string();

  if (cmd.empty())
    return string();

  constexpr int MAX_ATTEMPTS         { 10 };
  constexpr int TIMEOUT_MICROSECONDS { 100'000 };    // 100 milliseconds

// sanity check ... on K3 all commands end in a ";"
  if (cmd[cmd.length() - 1] != ';')
  {  _error_alert("Invalid rig command: "s + cmd);
    return string();
  }

  bool completed { false };

  { SAFELOCK(_rig);

    serial_flush(&rs_p->rigport);
    write(fd, cmd.c_str(), cmd.length());
    serial_flush(&rs_p->rigport);
    sleep_for(milliseconds(100));

    fd_set set;
    struct timeval timeout;  // time_t (seconds), long (microseconds)

    auto set_timeout = [=, &set, &timeout](const time_t sec, const long usec)
      { FD_ZERO(&set);    // clear the set
        FD_SET(fd, &set); // add the file descriptor to the set

        timeout.tv_sec = sec;
        timeout.tv_usec = usec;
      };

    int counter { 0 };

    constexpr int SCREEN_BITS { 131'640 };

    if (response_expected)
    { if (is_p3_screenshot)
      { array<char, SCREEN_BITS> c_in;    // hide the static array

        const int n_bits { SCREEN_BITS * 10 };
        const int n_secs { n_bits / static_cast<int>(baud_rate() )};

        while (!completed and (counter < (n_secs + 5)) )    // add 5 extra seconds
        { set_timeout(1, 0);

//          FD_ZERO(&set);    // clear the set
//          FD_SET(fd, &set); // add the file descriptor to the set

//          timeout.tv_sec = 1;
//          timeout.tv_usec = 0;

 //         int status { select(fd + 1, &set, NULL, NULL, &timeout) };

          if (int status { select(fd + 1, &set, NULL, NULL, &timeout) }; status == -1)
            ost << "Error in select() in raw_command()" << endl;
          else
          { if (status == 0)
             ost << "timeout in select() in raw_command: " << cmd << endl;
            else
            { const ssize_t n_read { read(fd, c_in.data(), SCREEN_BITS - total_read) };

              if (n_read > 0)                      // should always be true
              { total_read += n_read;
                rcvd.append(c_in.data(), n_read);

                if (rcvd.length() == SCREEN_BITS)
                  completed = true;
              }
            }
          }
          counter++;
          if (!completed)
          { static const string percent_str { "%%"s };

            const int percent { static_cast<int>(rcvd.length() * 100) / SCREEN_BITS };

            alert("P3 screendump progress: "s + to_string(percent) + percent_str);
            sleep_for(milliseconds(1000));  // we have the lock for all this time
          }
//          else
//            alert("P3 screendump complete");
        }
      }
      else              // not a P3 screenshot
      { static int rig_communication_failures { 0 };

        while (!completed and (counter < MAX_ATTEMPTS) )
        { set_timeout(0, TIMEOUT_MICROSECONDS);

 //         FD_ZERO(&set);    // clear the set
//          FD_SET(fd, &set); // add the file descriptor to the set

//          timeout.tv_sec = 0;
//          timeout.tv_usec = TIMEOUT_MICROSECONDS;

          if (counter)                          // we've already slept the first time through
            sleep_for(milliseconds(50));

 //         int status { select(fd + 1, &set, NULL, NULL, &timeout) };

          if (const int status { select(fd + 1, &set, NULL, NULL, &timeout) }; status == -1)
            ost << "Error in select() in raw_command()" << endl;
          else
          { if (status == 0)                // possibly a timeout error
            { if (counter == MAX_ATTEMPTS - 1)
              { if (cmd == "TQ;"s)
                { rig_communication_failures++;

                  if (rig_communication_failures == 1)
                    ost << "status communication with rig failed" << endl;
                }
                else
                  ost << "last-attempt timeout (" << (TIMEOUT_MICROSECONDS * MAX_ATTEMPTS) << "µs) in select() in raw_command: " << cmd << endl;
              }
            }
            else
            { if (cmd == "TQ;"s)
              { if (rig_communication_failures != 0)
                { ost << "status communication with rig restored after " << rig_communication_failures << " failure" << (rig_communication_failures == 1 ? "" : "s") << endl;
                  rig_communication_failures = 0;
                }
              }

              const ssize_t n_read { read(fd, c_in.data(), INBUF_SIZE / 2) };   // read a maximum of 500 characters inot the static array; halve to provide margin of error

              if (n_read > 0)                      // should always be true
              { total_read += n_read;
                c_in[n_read] = static_cast<char>(0);    // append a null byte

                rcvd += string(c_in.data());

                if (contains(rcvd, ";"s))
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
  { // _error_alert("Incomplete response from rig to cmd: " + cmd + " length = " + to_string(rcvd.length()) + " " + rcvd);
  }

  if (response_expected and completed)
    return rcvd;

  return string();
}

#endif

#if defined(NEW_RAW_COMMAND)
const string rig_interface::raw_command(const string& cmd, const unsigned int expected_length)
{ struct rig_state* rs_p { &(_rigp->state) };
  struct rig_state& rs   { *rs_p };

  const int fd           { _file_descriptor() };

  static array<char, 1000> c_in;

  int n_read              { 0 };
  unsigned int total_read { 0 };

  string rcvd;

  const bool is_p3_screenshot { (cmd == "#BMP;"s) };   // this has to be treated differently: the response is long and has no concluding semicolon

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
             ost << "timeout in select() in raw_command: " << cmd << endl;
            else
            { n_read = read(fd, c_in.data(), 131640 - total_read);

//              ost << "n_read = " << n_read << endl;

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

          if (counter)                          // we've already slept the first time through
            sleep_for(milliseconds(50));

          int status = select(fd + 1, &set, NULL, NULL, &timeout);
          int nread = 0;

          if (status == -1)
            ost << "Error in select() in raw_command()" << endl;
          else
          { if (status == 0)
            { if (counter == max_attempts - 1)
                ost << "last-attempt timeout (" << timeout_microseconds << "µs) in select() in raw_command: " << cmd << endl;
            }
            else
            { n_read = read(fd, c_in.data(), 500);        // read a maximum of 500 characters

//              ost << "n_read = " << n_read << " bytes; counter = " << counter << endl;

              if (n_read > 0)                      // should always be true
              { total_read += n_read;
                c_in[n_read] = static_cast<char>(0);    // append a null byte
//                ost << "  received: *" << c_in.data() << "*" << endl;

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
#endif

/// is the VFO locked?
bool rig_interface::is_locked(void)
{ if (_model == RIG_MODEL_K3)
  { const string status_str  { raw_command("LK;"s, RESPONSE::EXPECTED, 4) };
    const char   status_char { (status_str.length() >= 3 ? status_str[2] : '0') };  // default is unlocked

    return (status_char == '1');
  }
  else
  { SAFELOCK(_rig);

    int v;

    if (const int status { rig_get_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, &v) }; status != RIG_OK)
      throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error getting lock status"s);

    return (v == 1);
  }
}

/*! \brief      Get the bandwidth in Hz
    \return     the current audio bandwidth, in hertz
*/
int rig_interface::bandwidth(void)
{ if (!_rig_connected)
    return 0;

  const string status_str { raw_command("BW;"s, RESPONSE::EXPECTED, 7) };

  return ( (status_str.size() < 7) ? 0 : from_string<int>(substring(status_str, 2, 4)) * 10);
}

/*! \brief      Get the most recent frequency for a particular band and mode
    \param  bm  band and mode
    \return     the rig's most recent frequency for bandmode <i>bm</i>
*/
frequency rig_interface::get_last_frequency(const bandmode bm)
{ SAFELOCK(_rig);

  return MUM_VALUE(_last_frequency, bm);    // returns empty frequency if not prsent in the map

//  const auto cit { _last_frequency.find( { b, m } ) };

//  return ( ( cit == _last_frequency.cend() ) ? frequency() : cit->second );    // return 0 if there's no last frequency
}

/*! \brief      Get the most recent frequency for a particular band and mode
    \param  b   band
    \param  m   mode
    \return     the rig's most recent frequency for band <i>b</i> and mode <i>m</i>.
*/
#if 0
frequency rig_interface::get_last_frequency(const BAND b, const MODE m)
{ SAFELOCK(_rig);

  const auto cit { _last_frequency.find( { b, m } ) };

  return ( ( cit == _last_frequency.cend() ) ? frequency() : cit->second );    // return 0 if there's no last frequency
}
#endif

/*! \brief      Set a new value for the most recent frequency for a particular band and mode
    \param  b   band
    \param  m   mode
    \param  f   frequency
*/
#if 0
void rig_interface::set_last_frequency(const BAND b, const MODE m, const frequency& f)
{ SAFELOCK(_rig);

  _last_frequency[ { b, m } ] = f;
}
#endif

/*! \brief      Set a new value for the most recent frequency for a particular band and mode
    \param  bm  band and mode
    \param  f   frequency
*/
void rig_interface::set_last_frequency(const bandmode bm, const frequency& f)
{ SAFELOCK(_rig);

  _last_frequency[bm] = f;
}

/*! \brief      Is the rig transmitting?
    \return     whether the rig is currently transmitting

    With the K3, this is unreliable: the routine frequently takes the _error_alert() path, even if the rig is not transmitting.
    (This is, unfortunately, just one example of the unreliability of the K3 in responding to commands. I could write a book;
    or at least a paper.)
*/
bool rig_interface::is_transmitting(void)
{ if (_rig_connected)
  { bool rv { true };                                        // default: be paranoid

    if (_model == RIG_MODEL_K3)
    { const string response { raw_command("TQ;"s, RESPONSE::EXPECTED, 4) };

      if (response.length() < 4)
      { // because this happens so often, don't report it
        // _error_alert("Unable to determine whether rig is transmitting");
      }
      else
        rv = (response[2] == '1');
    }

    return rv;
  }
  else              // no rig connected
    return false;
}

/*! \brief      Is the rig in TEST mode?
    \return     whether the rig is currently in TEST mode
*/
bool rig_interface::test(void)
{ if (_model == RIG_MODEL_DUMMY)
    return true;

  bool rv { false };    // default

  if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
    { const string response { raw_command("IC;"s, RESPONSE::EXPECTED, 8) };

      if (response.length() < 8)
        _error_alert("Unable to retrieve rig icons and status"s);
      else
      { const char c { response[2] };

        rv = (c bitand (1 << 5));
      }
    }
  }

  return rv;
}

/*! \brief      Explicitly put the rig into or out of TEST mode
    \param  b   whether to enter TEST mode

    This works only with the K3.
*/
void rig_interface::test(const bool b)
{ if (test() != b)
  { if (_rig_connected)
    { if (_model == RIG_MODEL_K3)
#if defined(NEW_RAW_COMMAND)
        raw_command("SWH18;"s, 0);    // toggles state
#else
        raw_command("SWH18;"s);    // toggles state
#endif
    }
  }
}

/*! \brief      Which VFO is currently used for transmitting?
    \return     the VFO that is currently set to be used when transmitting
*/
VFO rig_interface::tx_vfo(void)
{ if (!_rig_connected)
    return VFO::A;

  return (split_enabled() ? VFO::B : VFO::A);  // this is the recommended procedure from the hamlib reflector
// I think this is ridiculous, because it does not allow for the case where the rig is in reverse split,
// with the TX on A and RX on B.

// Even worse, something like the next few lines, which logically should provide the desired result, is,
// according to comments on the hamlib reflector, NOT guaranteed to give the correct answer

/*
  vfo_t v;
  const int status = rig_get_vfo(_rigp, &v);

  if (status != RIG_OK)
    throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error getting active VFO");

  return (v == RIG_VFO_A ? VFO_A : VFO_B);
*/
}

/*! \brief      Set the bandwidth of VFO A
    \param  hz  desired bandwidth, in Hz
*/
void rig_interface::bandwidth_a(const unsigned int hz)
{ constexpr std::chrono::milliseconds RETRY_TIME { milliseconds(100) };      // period between retries for the brain-dead K3
  constexpr int                       PRECISION  { 50 };

  if (_rig_connected)
  { if (_model == RIG_MODEL_K3)                             // astonishingly, there is no hamlib function to do this
    { const string k3_bw_units { pad_left(to_string( (hz + 5) / 10 ), 4, '0') };

      raw_command("BW"s + k3_bw_units + ";"s);

 //     while (bandwidth() != static_cast<int>(hz))       // the K3 is brain-dead
      while (abs(bandwidth() - static_cast<int>(hz)) > PRECISION)       // the K3 is brain-dead
      { sleep_for(RETRY_TIME);
        raw_command("BW"s + k3_bw_units + ";"s);
      }
    }
  }
}

/*! \brief      Set the bandwidth of VFO B
    \param  hz  desired bandwidth, in Hz
*/
void rig_interface::bandwidth_b(const unsigned int hz)
{ if (_rig_connected)
  { if (_model == RIG_MODEL_K3)                             // astonishingly, there is no hamlib function to do this
    { const string k3_bw_units { pad_left(to_string( (hz + 5) / 10 ), 4, '0') };

      raw_command("BW$"s + k3_bw_units + ";"s);
    }
  }
}

/*! \brief      Is an RX antenna in use?
    \return     whether an RX antenna is in use

    Works only with K3
*/
bool rig_interface::rx_ant(void)
{ if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
    { const string result { raw_command("AR;", RESPONSE::EXPECTED) };

      if ( (result != "AR0;"s) and (result != "AR1;"s) )
        ost << "ERROR in rx_ant(): result = " << result << endl;

      return (result == "AR1;"s);
    }
  }

  return false;
}

/*! \brief          Control use of the RX antenna
    \param  torf    whether to use the RX antenna

    Works only with K3
*/
void rig_interface::rx_ant(const bool torf)
{ if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
      raw_command( torf ? "AR1;"s : "AR0;"s );
  }
}

/// register a function for alerting the user
void rig_interface::register_error_alert_function(void (*error_alert_function)(const string&) )
{ SAFELOCK(_rig);
  _error_alert_function = error_alert_function;
}

/// set RIT, split, sub-rx off
void rig_interface::base_state(void)
{ if (rit_enabled())
  { rit_disable();
    sleep_for(seconds(1));          // the K3 is awfully slow; this should allow plenty of time before the next command
  }

  if (split_enabled())
  { split_disable();
    sleep_for(seconds(1));          // the K3 is awfully slow; this should allow plenty of time before the next command
  }

  if (sub_receiver_enabled())
  { sub_receiver_disable();
    sleep_for(seconds(1));          // the K3 is awfully slow; this should allow plenty of time before the next command
  }
}

/*! \brief      Convert a hamlib error code to a printable string
    \param  e   hamlib error code
    \return     Printable string corresponding to error code <i>e</i>
*/
string hamlib_error_code_to_string(const int e)
{ switch (e)
  { case RIG_OK :
      return "No error, operation completed sucessfully"s;

    case RIG_EINVAL :
      return "invalid parameter"s;

    case RIG_ECONF :
      return "invalid configuration (serial,..)"s;

    case RIG_ENOMEM :
      return "memory shortage"s;

    case RIG_ENIMPL :
      return "function not implemented, but will be"s;

    case RIG_ETIMEOUT :
      return "communication timed out"s;

    case RIG_EIO :
      return "IO error, including open failed"s;

    case RIG_EINTERNAL :
      return "Internal Hamlib error, huh!"s;

    case RIG_EPROTO :
      return "Protocol error"s;

    case RIG_ERJCTED :
      return "Command rejected by the rig"s;

    case RIG_ETRUNC :
      return "Command performed, but arg truncated"s;

    case RIG_ENAVAIL :
      return "function not available"s;

    case RIG_ENTARGET :
      return "VFO not targetable"s;

    case RIG_BUSERROR :
      return "Error talking on the bus"s;

    case RIG_BUSBUSY :
      return "Collision on the bus"s;

    case RIG_EARG :
      return "NULL RIG handle or any invalid pointer parameter in get arg"s;

    case RIG_EVFO :
      return "Invalid VFO"s;

    case RIG_EDOM :
      return "Argument out of domain of function"s;

    default :
      return "Unknown hamlib error number: "s + to_string(e);
  }
}
