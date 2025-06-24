// $Id: bands-modes.cpp 271 2025-06-23 16:32:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#include "bands-modes.h"
#include "log_message.h"

using namespace std;

extern message_stream ost;          ///< for debugging and logging
extern string FREQUENCY_STRING_POINT;

/// default frequencies for bands and modes
const unordered_map<pair<BAND, MODE>, frequency > DEFAULT_FREQUENCIES { { { BAND_160, MODE_CW },  1'800_kHz },
                                                                        { { BAND_160, MODE_SSB }, 1'900_kHz },
                                                                        { { BAND_80,  MODE_CW },  3'500_kHz },
                                                                        { { BAND_80,  MODE_SSB }, 3'750_kHz },
                                                                        { { BAND_40,  MODE_CW },  7'000_kHz },
                                                                        { { BAND_40,  MODE_SSB }, 7'150_kHz },
                                                                        { { BAND_30,  MODE_CW },  10'100_kHz },
                                                                        { { BAND_30,  MODE_SSB }, 10'100_kHz },        // just to make things easier if we go to 30m while in SSB mode
                                                                        { { BAND_20,  MODE_CW },  14'000_kHz },
                                                                        { { BAND_20,  MODE_SSB }, 14'150_kHz },
                                                                        { { BAND_17,  MODE_CW },  18'068_kHz },
                                                                        { { BAND_17,  MODE_SSB }, 18'100_kHz },
                                                                        { { BAND_15,  MODE_CW },  21'000_kHz },
                                                                        { { BAND_15,  MODE_SSB }, 21'200_kHz },
                                                                        { { BAND_12,  MODE_CW },  24'890_kHz },
                                                                        { { BAND_12,  MODE_SSB }, 24'940_kHz },
                                                                        { { BAND_10,  MODE_CW },  28'000_kHz },
                                                                        { { BAND_10,  MODE_SSB }, 28'300_kHz },
                                                                        { { BAND_6,   MODE_CW },  50'090_kHz },   // go to a widely-used CW frequency on 6m
                                                                        { { BAND_6,   MODE_SSB }, 50'100_kHz },
                                                                      };

// ----------------------------------------------------  frequency  -----------------------------------------------

/*! \class  frequency
    \brief  A convenient class for handling frequencies
*/

/*! \brief      construct from a double
    \param f    frequency in Hz, kHz or MHz
*/
frequency::frequency(const double f)
{ if (f < 100)                                                // MHz
    _hz = static_cast<HZ_TYPE>(f * 1'000'000 + 0.5);
  else
    _hz = static_cast<HZ_TYPE>( (f < 100'000) ? (f * 1'000 + 0.5) : (f + 0.5) );  // kHz and Hz
}

/*! \brief      return string suitable for use in bandmap
    \return     string of the frequency in kHz, to one decimal place ([x]xxxx.y)
*/
string frequency::display_string(void) const
{ unsigned int khz { _hz / 1000 };
  unsigned int hhz { ( ( _hz - (khz * 1000) ) / 10 + 5 ) / 10 };  // first decimal place in kHz frequency

  if (hhz == 10)
  { khz++;
    hhz = 0;
  }

  return (to_string(khz) + "."s + to_string(hhz));
}

/*! \brief      Return frequency in MHz as string (with 3 dp)
    \return     string of the frequency in MHz, to three decimal placex ([xxxx].yyy)
*/
string frequency::display_string_MHz(void) const
{ const unsigned int mhz     { _hz / 1'000'000 };
  const unsigned int khz     { (_hz / 1000) - (mhz * 1000) };
  const string       mhz_str { to_string(mhz) };
  const string       khz_str { pad_leftz(khz, 3) };

  return mhz_str + "."s + khz_str;
}

/// return lower band edge that corresponds to frequency
frequency frequency::lower_band_edge(void) const
{ const static map<BAND, frequency> edge_map { { BAND_160, 1.8_MHz },
                                               { BAND_80,  3.5_MHz },
                                               { BAND_60,  frequency(0.0) },  // dunno where band edge is;
                                               { BAND_40,  7.0_MHz },
                                               { BAND_30,  10.1_MHz },
                                               { BAND_20,  14.0_MHz },
                                               { BAND_17,  18.068_MHz },
                                               { BAND_15,  21.0_MHz },
                                               { BAND_12,  24.89_MHz },
                                               { BAND_10,  28.0_MHz },
                                               { BAND_6,   50.0_MHz }
                                             };

  return MUM_VALUE(edge_map, BAND(*this), frequency(0.0));
}

/// difference in two frequencies, always +ve
frequency frequency::difference(const frequency f2) const
{ frequency rv;

  const int d { ( (hz() > f2.hz()) ? (hz() - f2.hz()) : (f2.hz() - hz()) ) };

  rv.hz(d);

  return rv;
}

/*! \brief          Find the next lower band
    \param  bands   set of bands that may be returned
    \return         band from <i>bands</i> that is the highest band below the frequency

    Returns BAND_160 if the frequency is outside all bands
*/
BAND frequency::next_band_down(const set<BAND>& bands) const
{ for (auto crit { bands.crbegin() }; crit != bands.crend(); ++crit)
  { if (upper_edge(*crit) < *this)
     return *crit;
  }

  return ( is_within_ham_band() ? BAND(*this) : BAND_160 );
}

/*! \brief          Find the next higher band
    \param  bands   set of bands that may be returned
    \return         band from <i>bands</i> that is the lowest band above the frequency

    Returns BAND_160 if the frequency is outside all bands
*/
BAND frequency::next_band_up(const set<BAND>& bands) const
{ for (const BAND b : bands)
  { if (lower_edge(b) > *this)
     return b;
  }

  return (is_within_ham_band() ? BAND(*this) : BAND_160 );
}

/// ostream << frequency
ostream& operator<<(ostream& ost, const frequency f)
{ ost << f.hz();

  return ost;
}

/*! \brief      Return the lower edge of a band
    \param  b   target band
    \return     the frequency at the low edge of band <i>b</i>
*/
frequency lower_edge(const BAND b)
{ switch (b)
  { case BAND_160 :
      return 1'800_kHz;

    case BAND_80 :
      return 3'500_kHz;

    case BAND_60 :
      return 5'000_kHz;

    case BAND_40 :
      return 7'000_kHz;

    case BAND_30 :
      return 10'100_kHz;

    case BAND_20 :
      return 14'000_kHz;

    case BAND_17 :
      return 18'068_kHz;

    case BAND_15 :
      return 21'000_kHz;

    case BAND_12 :
      return 24'890_kHz;

    case BAND_10 :
      return 28'000_kHz;

    case BAND_6 :
      return 50'000_kHz;

    default :
      return 1'800_kHz;
  }
}

/*! \brief      Return the upper edge of a band
    \param  b   target band
    \return     the frequency at the top edge of band <i>b</i>
*/
frequency upper_edge(const BAND b)
{ switch (b)
  { case BAND_160 :
      return 2'000_kHz;

    case BAND_80 :
      return 4'000_kHz;

    case BAND_60 :
      return 6'000_kHz;

    case BAND_40 :
      return 7'300_kHz;

    case BAND_30 :
      return 10'150_kHz;

    case BAND_20 :
      return 14'350_kHz;

    case BAND_17 :
      return 18'168_kHz;

    case BAND_15 :
      return 21'450_kHz;

    case BAND_12 :
      return 24'990_kHz;

    case BAND_10 :
      return 29'700_kHz;

    case BAND_6 :
      return 54'000_kHz;

    default :
      return 1'800_kHz;
  }
}

/*! \brief      guess the mode for a particular frequency
    \param  f   frequency
    \return     guessed mode for the frequency <i>f</i>
*/
MODE putative_mode(const frequency f)
{ try
  { const BAND      b           { to_BAND(f) };
    const frequency break_point { MODE_BREAK_POINT.at(b) };

    return ( (f < break_point) ? MODE_CW : MODE_SSB );
  }

  catch (...)
  { ost << "Unable to determine putative mode for frequency: " << f << endl;
    return MODE_CW;     // default
  }
}
