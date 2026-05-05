 // $Id: hamlib.h 290 2026-03-30 15:48:47Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   hamlib.h

    Functions, classes, etc. related to hamlib
*/

/* My experience with hamilib has been rather unhappy -- when used with a K3, I found it both slow and buggy. Still,
   in order to support a range of rigs we have to start somewhere, and hamlib is probably the best place to begin.

   So the default behaviour for a rig is to use hamlib. One can then replace the less-than-useful aspects of hamlib
   on an ad-hoc basis for a rig by using a separate derived class for the rig (as the K3 does).

   I note that there is an official C++ wrapper for hamlib. But that seems to be both primitive and thin, so we don't
   even try to use it.

   I also note that the documentation for hamlib is quite poor, and there is a lot about which one has to guess. It is
   likely that in what follows I have, in at least a few instances, guessed incorrectly.
*/

#ifndef DRLOG_HAMLIB_H
#define DRLOG_HAMLIB_H

#include "string_functions.h"

#include <hamlib/rig.h>

using HAMLIB_CAPABILITIES_TYPE = uint64_t;

/// the two VFOs
enum class VFO { A,                       ///< VFO A
                 B                        ///< VFO B
               };

// these are the RIG_FUNC macros from the hamlib rig.h file, along with the descriptions therefrom.
// Most of them are unused in drlog.
enum class HAMLIB_CAPABILITY : HAMLIB_CAPABILITIES_TYPE { FAGC           = RIG_FUNC_FAGC,           // Fast AGC
                                                          NB             = RIG_FUNC_NB,             // Noise Blanker
                                                          COMP           = RIG_FUNC_COMP,           // Speech Compression
                                                          VOX            = RIG_FUNC_VOX,            // Voice Operated Relay
                                                          TONE           = RIG_FUNC_TONE,           // CTCSS Tone TX
                                                          TSQL           = RIG_FUNC_TSQL,           // CTCSS Activate/De-activate RX
                                                          SBKIN          = RIG_FUNC_SBKIN,          // Semi Break-in (CW mode)
                                                          FBKIN          = RIG_FUNC_FBKIN,          // Full Break-in (CW mode)
                                                          ANF            = RIG_FUNC_ANF,            // Automatic Notch Filter (DSP)
                                                          NR             = RIG_FUNC_NR,             // Noise Reduction (DSP)
                                                          AIP            = RIG_FUNC_AIP,            // RF pre-amp (AIP on Kenwood, IPO on Yaesu, etc.)
                                                          APF            = RIG_FUNC_APF,            // Audio Peak Filter
                                                          MON            = RIG_FUNC_MON,            // Monitor transmitted signal
                                                          MN             = RIG_FUNC_MN,             // Manual Notch
                                                          RF             = RIG_FUNC_RF,             // RTTY Filter
                                                          ARO            = RIG_FUNC_ARO,            // Auto Repeater Offset
                                                          LOCK           = RIG_FUNC_LOCK,           // Lock
                                                          MUTE           = RIG_FUNC_MUTE,           // Mute
                                                          VSC            = RIG_FUNC_VSC,            // Voice Scan Control
                                                          REV            = RIG_FUNC_REV,            // Reverse transmit and receive frequencies
                                                          SQL            = RIG_FUNC_SQL,            // Turn Squelch Monitor on/off
                                                          ABM            = RIG_FUNC_ABM,            // Auto Band Mode
                                                          BC             = RIG_FUNC_BC,             // Beat Canceller
                                                          MBC            = RIG_FUNC_MBC,            // Manual Beat Canceller
                                                          RIT            = RIG_FUNC_RIT,            // Receiver Incremental Tuning
                                                          AFC            = RIG_FUNC_AFC,            // Auto Frequency Control ON/OFF
                                                          SATMODE        = RIG_FUNC_SATMODE,        // Satellite mode ON/OFF
                                                          SCOPE          = RIG_FUNC_SCOPE,          // Simple bandscope ON/OFF
                                                          RESUME         = RIG_FUNC_RESUME,         // Scan auto-resume
                                                          TBURST         = RIG_FUNC_TBURST,         // 1750 Hz tone burst
                                                          TUNER          = RIG_FUNC_TUNER,          // Enable automatic tuner
                                                          XIT            = RIG_FUNC_XIT,            // Transmitter Incremental Tuning
                                                          NB2            = RIG_FUNC_NB2,            // 2nd Noise Blanker
                                                          CSQL           = RIG_FUNC_CSQL,           // DCS Squelch setting
                                                          AFLT           = RIG_FUNC_AFLT,           // AF Filter setting
                                                          ANL            = RIG_FUNC_ANL,            // Noise limiter setting
                                                          BC2            = RIG_FUNC_BC2,            // 2nd Beat Cancel
                                                          DUAL_WATCH     = RIG_FUNC_DUAL_WATCH,     // Dual Watch / Sub Receiver
                                                          DIVERSITY      = RIG_FUNC_DIVERSITY,      // Diversity receive
                                                          DSQL           = RIG_FUNC_DSQL,           // Digital modes squelch
                                                          SCEN           = RIG_FUNC_SCEN,           // scrambler/encryption
                                                          SLICE          = RIG_FUNC_SLICE,          // Rig slice selection -- Flex
                                                          TRANSCEIVE     = RIG_FUNC_TRANSCEIVE,     // Send radio state changes automatically ON/OFF
                                                          SPECTRUM       = RIG_FUNC_SPECTRUM,       // Spectrum scope data output ON/OFF
                                                          SPECTRUM_HOLD  = RIG_FUNC_SPECTRUM_HOLD,  // Pause spectrum scope updates ON/OFF
                                                          SEND_MORSE     = RIG_FUNC_SEND_MORSE,     // Send specified characters using CW
                                                          SEND_VOICE_MEM = RIG_FUNC_SEND_VOICE_MEM, // Transmit in SSB message stored in memory
                                                          OVF_STATUS     = RIG_FUNC_OVF_STATUS,     // Read overflow status 0=Off, 1=On
                                                          SYNC           = RIG_FUNC_SYNC            // Synchronize VFOS -- FTDX101D/MP for now SY command
                                                        };

HAMLIB_CAPABILITIES_TYPE hamlib_get_capabilities(RIG* rigp);    // NB hamlib requires a non-const pointer

class hamlib_capabilities
{
protected:

  HAMLIB_CAPABILITIES_TYPE _caps { 0 };        // uint64_t mask of capabilities

  std::string              _vfos_str { };      // output from rig_get_vfo_list()
  std::set<VFO>            _vfos     { };

public:

/*! \brief      get a rig's capabilities
    \param  rp  pointer to a hamlib RIG object
*/
  void get_capabilities(RIG* rp);

/// are the capabilities uninitialised?
  inline bool empty(void) const
    { return ( _caps == static_cast<HAMLIB_CAPABILITIES_TYPE>(0) ); }

#define HAS_CAPABILITY(y) \
  inline bool y(void) const \
    { return ( _caps bitand static_cast<HAMLIB_CAPABILITIES_TYPE>(HAMLIB_CAPABILITY::y) ); }

  HAS_CAPABILITY(FAGC);
  HAS_CAPABILITY(NB);
  HAS_CAPABILITY(COMP);
  HAS_CAPABILITY(VOX);
  HAS_CAPABILITY(TONE);
  HAS_CAPABILITY(TSQL);
  HAS_CAPABILITY(SBKIN);
  HAS_CAPABILITY(FBKIN);
  HAS_CAPABILITY(ANF);
  HAS_CAPABILITY(NR);

  HAS_CAPABILITY(AIP);
  HAS_CAPABILITY(APF);
  HAS_CAPABILITY(MON);
  HAS_CAPABILITY(MN);
  HAS_CAPABILITY(RF);

  HAS_CAPABILITY(ARO);

#pragma push_macro("LOCK")    // there's a LOCK macro defined in pthread_support.h
#undef LOCK

  HAS_CAPABILITY(LOCK);

#pragma pop_macro("LOCK")

  HAS_CAPABILITY(MUTE);
  HAS_CAPABILITY(VSC);
  HAS_CAPABILITY(REV);
  HAS_CAPABILITY(SQL);
  HAS_CAPABILITY(ABM);
  HAS_CAPABILITY(BC);
  HAS_CAPABILITY(MBC);
  HAS_CAPABILITY(RIT);
  HAS_CAPABILITY(AFC);
  HAS_CAPABILITY(SATMODE);
  HAS_CAPABILITY(SCOPE);
  HAS_CAPABILITY(RESUME);
  HAS_CAPABILITY(TBURST);
  HAS_CAPABILITY(TUNER);
  HAS_CAPABILITY(XIT);
  HAS_CAPABILITY(NB2);
  HAS_CAPABILITY(CSQL);
  HAS_CAPABILITY(AFLT);
  HAS_CAPABILITY(ANL);
  HAS_CAPABILITY(BC2);
  HAS_CAPABILITY(DUAL_WATCH);
  HAS_CAPABILITY(DIVERSITY);
  HAS_CAPABILITY(DSQL);
  HAS_CAPABILITY(SCEN);
  HAS_CAPABILITY(SLICE);
  HAS_CAPABILITY(TRANSCEIVE);
  HAS_CAPABILITY(SPECTRUM);
  HAS_CAPABILITY(SPECTRUM_HOLD);
  HAS_CAPABILITY(SEND_MORSE);
  HAS_CAPABILITY(SEND_VOICE_MEM);
  HAS_CAPABILITY(OVF_STATUS);
  HAS_CAPABILITY(SYNC);

#undef HAS_CAPABILITY

  READ(vfos);
  READ(vfos_str);

  std::string to_string(void) const;
};

#endif    // DRLOG_HAMLIB_H
