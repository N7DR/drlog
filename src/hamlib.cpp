 
// $Id: hamlib.cpp 288 2026-03-14 19:49:46Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   hamlib.cpp

    Functions, classes, etc. related to hamlib
*/

/* My experience with hamilib has been rather unhappy -- when used with a K3, I found it both slow and buggy. Still,
   in order to support a range of rigs we have to start somewhere, and hamlib is probably the best place to begin.

   So the default behaviour for a rig is to use hamlib. One can then replace the less-than-useful aspects of hamlib
   on an ad-hoc basis for a rig by using a separate derived class for the rig (as the K3 does).

   I note that there is an official C++ wrapper for hamlib. But that seems to be both primitive, and thin, so we don't
   even try to use it.

   I also note that the documentation for hamlib is quite poor, and there is a lot about which one has to guess. It is
   likely that in what follows I have, in at least a few instances, guessed incorrectly.
*/

#include "hamlib.h"
#include "log_message.h"

extern message_stream ost;                  ///< for debugging, info

using namespace std;

HAMLIB_CAPABILITIES_TYPE hamlib_get_capabilities(RIG* rigp)
{ using enum HAMLIB_CAPABILITY;

  HAMLIB_CAPABILITIES_TYPE requested_capabilities { 0 };

  if (rigp)
  { requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(FAGC);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(NB);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(COMP);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(VOX);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(TONE);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(TSQL);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SBKIN);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(FBKIN);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(ANF);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(NR);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(AIP);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(APF);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(MON);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(MN);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(RF);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(ARO);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(LOCK);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(MUTE);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(VSC);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(REV);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SQL);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(ABM);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(BC);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(MBC);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(RIT);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(AFC);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SATMODE);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SCOPE);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(RESUME);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(TBURST);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(TUNER);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(XIT);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(NB2);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(CSQL);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(AFLT);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(ANL);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(BC2);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(DUAL_WATCH);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(DIVERSITY);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(DSQL);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SCEN);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SLICE);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(TRANSCEIVE);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SPECTRUM);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SPECTRUM_HOLD);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SEND_MORSE);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SEND_VOICE_MEM);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(OVF_STATUS);
    requested_capabilities += static_cast<HAMLIB_CAPABILITIES_TYPE>(SYNC);

    ost << "requesting capabilities = " << requested_capabilities << endl;
    ost << "binary: " << std::bitset<8*sizeof(requested_capabilities)>(requested_capabilities) << endl;

    HAMLIB_CAPABILITIES_TYPE rv { rig_has_get_func(rigp, requested_capabilities) };

    ost << "returned capabilties = " << rv << endl;
    ost << "binary: " << std::bitset<8*sizeof(rv)>(rv) << endl;

    return rv;
  }

  ost << "Error: hamlib_get_capabilities() called with nullptr" << endl;
  exit(-1);
}

void hamlib_capabilities::get_capabilities(RIG* rp)
{ _caps = hamlib_get_capabilities(rp);

  constexpr int BUFLEN { 1024 };

  char buf[BUFLEN];

  const int status { rig_get_vfo_list(rp, &buf[0], BUFLEN) };

  if (status == RIG_OK)
  { ost << "output from rig_get_vfo_list() in hamlib_capabilities::get_capabilities: " << buf << endl;
    _vfos_str = to_upper(string { buf });   // force upper case
    ost << "_vfos_str = " << _vfos_str << endl;
  }
  else
    ost << "Error return from rig_get_vfo_list() in hamlib_capabilities::get_capabilities" << endl;

  if (!_vfos_str.empty())
  { static const FLAT_STRING_SET VFOAs { "VFOA"s, "MAIN"s };
    static const FLAT_STRING_SET VFOBs { "VFOB"s, "SUB"s  };

    if (ANY_OF(VFOAs, [this] (const string& str) { return _vfos_str.contains(str); }))
    { _vfos += VFO::A;
      ost << "Added VFO A" << endl;
    }

    if (ANY_OF(VFOBs, [this] (const string& str) { return _vfos_str.contains(str); }))
    { _vfos += VFO::B;
      ost << "Added VFO B" << endl;
    }
  }
}

string hamlib_capabilities::to_string(void) const
{ string rv { };

#define WRITE_CAPABILITY(y) \
  { std::string msg { }; \
  \
    if ( y() ) msg = "  HAS hamlib capability "s + #y ; \
    else msg = "  DOES NOT HAVE hamlib capability "s + #y ;  \
  \
    rv += (msg + EOL); \
  }

  WRITE_CAPABILITY(FAGC);
  WRITE_CAPABILITY(NB);
  WRITE_CAPABILITY(COMP);
  WRITE_CAPABILITY(VOX);
  WRITE_CAPABILITY(TONE);
  WRITE_CAPABILITY(TSQL);
  WRITE_CAPABILITY(SBKIN);
  WRITE_CAPABILITY(FBKIN);
  WRITE_CAPABILITY(ANF);
  WRITE_CAPABILITY(NR);
  WRITE_CAPABILITY(AIP);
  WRITE_CAPABILITY(APF);
  WRITE_CAPABILITY(MON);
  WRITE_CAPABILITY(MN);
  WRITE_CAPABILITY(RF);
  WRITE_CAPABILITY(ARO);

#pragma push_macro("LOCK")    // there's a LOCK macro defined in pthread_support.h
#undef LOCK

  WRITE_CAPABILITY(LOCK);

#pragma pop_macro("LOCK")

//  WRITE_CAPABILITY(LOCK);
  WRITE_CAPABILITY(MUTE);
  WRITE_CAPABILITY(VSC);
  WRITE_CAPABILITY(REV);
  WRITE_CAPABILITY(SQL);
  WRITE_CAPABILITY(ABM);
  WRITE_CAPABILITY(BC);
  WRITE_CAPABILITY(MBC);
  WRITE_CAPABILITY(RIT);
  WRITE_CAPABILITY(AFC);
  WRITE_CAPABILITY(SATMODE);
  WRITE_CAPABILITY(SCOPE);
  WRITE_CAPABILITY(RESUME);
  WRITE_CAPABILITY(TBURST);
  WRITE_CAPABILITY(TUNER);
  WRITE_CAPABILITY(XIT);
  WRITE_CAPABILITY(NB2);
  WRITE_CAPABILITY(CSQL);
  WRITE_CAPABILITY(AFLT);
  WRITE_CAPABILITY(ANL);
  WRITE_CAPABILITY(BC2);
  WRITE_CAPABILITY(DUAL_WATCH);
  WRITE_CAPABILITY(DIVERSITY);
  WRITE_CAPABILITY(DSQL);
  WRITE_CAPABILITY(SCEN);
  WRITE_CAPABILITY(SLICE);
  WRITE_CAPABILITY(TRANSCEIVE);
  WRITE_CAPABILITY(SPECTRUM);
  WRITE_CAPABILITY(SPECTRUM_HOLD);
  WRITE_CAPABILITY(SEND_MORSE);
  WRITE_CAPABILITY(SEND_VOICE_MEM);
  WRITE_CAPABILITY(OVF_STATUS);
  WRITE_CAPABILITY(SYNC);

#undef WRITE_CAPABILITY

  return rv;
}
