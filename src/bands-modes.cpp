// $Id: bands-modes.cpp 153 2019-09-01 14:27:02Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#include "bands-modes.h"

using namespace std;

extern string FREQUENCY_STRING_POINT;

/// default frequencies for bands and modes
map<pair<BAND, MODE>, frequency > DEFAULT_FREQUENCIES = { { { BAND_160, MODE_CW },  frequency(1'800'000) },
                                                          { { BAND_160, MODE_SSB }, frequency(1'900'000) },
                                                          { { BAND_80,  MODE_CW },  frequency(3'500'000) },
                                                          { { BAND_80,  MODE_SSB }, frequency(3'750'000) },
                                                          { { BAND_40,  MODE_CW },  frequency(7'000'000) },
                                                          { { BAND_40,  MODE_SSB }, frequency(7'150'000) },
                                                          { { BAND_30,  MODE_CW },  frequency(10'100'000) },
                                                          { { BAND_30,  MODE_SSB }, frequency(10'100'000) },        // just to make things easier if we go to 30m while in SSB mode
                                                          { { BAND_20,  MODE_CW },  frequency(14'000'000) },
                                                          { { BAND_20,  MODE_SSB }, frequency(14'150'000) },
                                                          { { BAND_17,  MODE_CW },  frequency(18'068'000) },
                                                          { { BAND_17,  MODE_SSB }, frequency(18'100'000) },
                                                          { { BAND_15,  MODE_CW },  frequency(21'000'000) },
                                                          { { BAND_15,  MODE_SSB }, frequency(21'200'000) },
                                                          { { BAND_12,  MODE_CW },  frequency(24'890'000) },
                                                          { { BAND_12,  MODE_SSB }, frequency(24'940'000) },
                                                          { { BAND_10,  MODE_CW },  frequency(28'000'000) },
                                                          { { BAND_10,  MODE_SSB }, frequency(28'300'000) }
                                                        };

// ----------------------------------------------------  frequency  -----------------------------------------------

/*! \class  frequency
    \brief  A convenient class for handling frequencies
*/

/*! \brief      construct from a double
    \param f    frequency in Hz, kHz or MHz
*/
frequency::frequency(const double f)
{ if (f < 100)                  // MHz
    _hz = static_cast<unsigned int>(f * 1'000'000 + 0.5);
  else
  { if (f < 100'000)             // kHz
      _hz = static_cast<unsigned int>(f * 1'000 + 0.5);
    else
      _hz = static_cast<unsigned int>(f + 0.5);
  }
}

/*! \brief          Construct from a double and an explicit unit
    \param f        frequency in Hz, kHz or MHz
    \param unit     frequency unit
*/
frequency::frequency(const double f, const FREQUENCY_UNIT unit)

{ switch (unit)
  { case FREQUENCY_UNIT::HZ :
      _hz = static_cast<unsigned int>(f + 0.5);
      break;

    case FREQUENCY_UNIT::KHZ :
      _hz = static_cast<unsigned int>(f * 1'000 + 0.5);
      break;

    case FREQUENCY_UNIT::MHZ :
      _hz = static_cast<unsigned int>(f * 1'000'000 + 0.5);
      break;
  }
}

/*! \brief      construct from a band
    \param b    band

    Sets the frequency to the low edge of the band <i>b</i>
*/
frequency::frequency(const enum BAND b)
{ switch (b)
  { case BAND_160 :
      _hz = 1'800'000;
      break;

    case BAND_80 :
      _hz = 3'500'000;
      break;

    case BAND_60 :
      _hz = 5'000'000;
      break;

    case BAND_40 :
      _hz = 7'000'000;
      break;

    case BAND_30 :
      _hz = 10'100'000;
      break;

    case BAND_20 :
      _hz = 14'000'000;
      break;

    case BAND_17 :
      _hz = 18'068'000;
      break;

    case BAND_15 :
      _hz = 21'000'000;
      break;

    case BAND_12 :
      _hz = 24'890'000;
      break;

    case BAND_10 :
      _hz = 28'000'000;
      break;

    default :           // should never happen
      _hz = 1'800'000;
      break;
  }
}

/*! \brief      return string suitable for use in bandmap
    \return     string of the frequency in kHz, to one decimal place ([x]xxxx.y)

    Sets the frequency to the low edge of the band <i>b</i>
*/
const string frequency::display_string(void) const
{ //const string POINT(".");                                      // during termination, static can cause a core dump; https://stackoverflow.com/questions/246564/what-is-the-lifetime-of-a-static-variable-in-a-c-function

  unsigned int khz { _hz / 1000 };
  unsigned int hhz { ( ( _hz - (khz * 1000) ) / 10 + 5 ) / 10 };  // first decimal place in kHz frequency

  if (hhz == 10)
  { khz++;
    hhz = 0;
  }

  return (to_string(khz) + "."s + to_string(hhz));
}

/// return lower band edge that corresponds to frequency
const frequency frequency::lower_band_edge(void) const
{ const BAND b { BAND(*this) };

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
      return frequency(24.89);

    case BAND_10 :
      return frequency(28.0);

    default :
      return frequency(0.0);
  }
}

/// is the frequency within a band?
const bool frequency::is_within_ham_band(void) const
{ const BAND b { BAND(*this) };

  if (b != BAND_160)
    return true;

 return ( (_hz >= 1'800'000) and (_hz <= 2'000'000) );    // check if BAND_160, because that's the returned band if frequency is outside a band
}

