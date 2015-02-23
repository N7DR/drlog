// $Id: bands-modes.cpp 85 2014-12-01 23:26:41Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#include "bands-modes.h"

using namespace std;

extern string FREQUENCY_STRING_POINT;

// ----------------------------------------------------  frequency  -----------------------------------------------

/*!     \class frequency
        \brief A convenient class for handling frequencies
*/

/// default constructor
frequency::frequency(void) :
  _hz(0)
{ }

/*! \brief      construct from a double
    \param f    frequency in Hz, kHz or MHz
*/
frequency::frequency(const double f)
{ if (f < 100)                  // MHz
    _hz = static_cast<unsigned int>(f * 1000000 + 0.5);
  else
  { if (f < 100000)            // kHz
      _hz = static_cast<unsigned int>(f * 1000 + 0.5);
    else
      _hz = static_cast<unsigned int>(f + 0.5);
  }
}

/*! \brief      construct from a band
    \param b    band

    Sets the frequency to the low edge of the band <i>b</i>
*/
frequency::frequency(const enum BAND b)
{ switch (b)
  { case BAND_160 :
      _hz = 1800000;
      break;

    case BAND_80 :
      _hz = 3500000;
      break;

    case BAND_60 :
      _hz = 5000000;
      break;

    case BAND_40 :
      _hz = 7000000;
      break;

    case BAND_30 :
      _hz = 10100000;
      break;

    case BAND_20 :
      _hz = 14000000;
      break;

    case BAND_17 :
      _hz = 18000000;
      break;

    case BAND_15 :
      _hz = 21000000;
      break;

    case BAND_12 :
      _hz = 24900000;
      break;

    case BAND_10 :
      _hz = 28000000;
      break;
  }
}

/*! \brief      return string suitable for use in bandmap
    \return     string of the frequency in kHz, to one decimal place ([x]xxxx.y)

    Sets the frequency to the low edge of the band <i>b</i>
*/
const string frequency::display_string(void) const
{ const string POINT(".");                                      // during termination, static can cause a core dump; https://stackoverflow.com/questions/246564/what-is-the-lifetime-of-a-static-variable-in-a-c-function

  unsigned int khz = _hz / 1000;
  unsigned int hhz = ( ( _hz - (khz * 1000) ) / 10 + 5 ) / 10;  // first decimal place in kHz frequency

  if (hhz == 10)
  { khz++;
    hhz = 0;
  }

  return (to_string(khz) + POINT + to_string(hhz));
}

/*! \brief      convert to BAND
    \return     BAND in which the frequency is located

    Returns BAND_160 if the frequency is outside all bands
*/
frequency::operator BAND(void) const
{ return to_BAND(hz());
}

/// return lower band edge that corresponds to frequency
const frequency frequency::lower_band_edge(void) const
{ const BAND b = BAND(*this);

  switch (b)
  { case BAND_160 :
      return frequency(1.8);

    case BAND_80 :
      return frequency(3.5);

    case BAND_60 :
      return frequency(0.0);  // dunno where band edge is;

    case BAND_40 :
      return frequency(7.0);

    case BAND_30 :
      return frequency(10.1);

    case BAND_20 :
      return frequency(14.0);

    case BAND_17 :
      return frequency(18.068);

    case BAND_15 :
      return frequency(21.0);

    case BAND_12 :
      return frequency(0.0);  // dunno where band edge is;

    case BAND_10 :
      return frequency (28.0);

    default :
      return frequency(0.0);
  }
}

/// is the frequency within a band?
const bool frequency::is_within_ham_band(void) const
{ const BAND b = BAND(*this);

  if (b != BAND_160)
    return true;

 return ( (_hz >= 1800000) and (_hz <= 2000000) );    // check if BAND_160, because that's the returned band if frequency is outside a band
}

