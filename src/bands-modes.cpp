// $Id: bands-modes.cpp 187 2021-06-26 16:16:42Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#include "bands-modes.h"

using namespace std;

extern string FREQUENCY_STRING_POINT;

/// default frequencies for bands and modes
const unordered_map<pair<BAND, MODE>, frequency > DEFAULT_FREQUENCIES = { { { BAND_160, MODE_CW },  frequency(1'800'000) },
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
    _hz = static_cast<HZ_TYPE>(f * 1'000'000 + 0.5);
  else
  { //if (f < 100'000)             // kHz
   //   _hz = static_cast<unsigned int>(f * 1'000 + 0.5);
    //else
    //  _hz = static_cast<unsigned int>(f + 0.5);
    _hz = static_cast<HZ_TYPE>( (f < 100'000) ? (f * 1'000 + 0.5) : (f + 0.5) );
  }
}

/*! \brief          Construct from a double and an explicit unit
    \param f        frequency in Hz, kHz or MHz
    \param unit     frequency unit
*/
frequency::frequency(const double f, const FREQUENCY_UNIT unit)
{ switch (unit)
  { case FREQUENCY_UNIT::HZ :
      _hz = static_cast<HZ_TYPE>(f + 0.5);
      break;

    case FREQUENCY_UNIT::KHZ :
      _hz = static_cast<HZ_TYPE>(f * 1'000 + 0.5);
      break;

    case FREQUENCY_UNIT::MHZ :
      _hz = static_cast<HZ_TYPE>(f * 1'000'000 + 0.5);
      break;
  }
}

/*! \brief      construct from a band
    \param b    band

    Sets the frequency to the low edge of the band <i>b</i>
*/
#if 0
frequency::frequency(const enum BAND b)
{ _hz = lower_edge(b);
#if 0
  const static map<BAND, decltype(_hz)> hz_map { { BAND_160, 1'800'000 },
                                                 { BAND_80,  3'500'000 },
                                                 { BAND_60,  5'000'000 },  // dunno where band edge is;
                                                 { BAND_40,  7'000'000 },
                                                 { BAND_30,  10'100'000 },
                                                 { BAND_20,  14'000'000 },
                                                 { BAND_17,  18'068'000 },
                                                 { BAND_15,  21'000'000 },
                                                 { BAND_12,  24'890'000 },
                                                 { BAND_10,  28'000'000 }
                                               };

  _hz = MUM_VALUE(hz_map, b, 1'800'000);           // default to 160m
#endif
}
#endif

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
{ const static map<BAND, frequency> edge_map { { BAND_160, frequency(1.8) },
                                               { BAND_80,  frequency(3.5) },
                                               { BAND_60,  frequency(0.0) },  // dunno where band edge is;
                                               { BAND_40,  frequency(7.0) },
                                               { BAND_30,  frequency(10.1) },
                                               { BAND_20,  frequency(14.0) },
                                               { BAND_17,  frequency(18.068) },
                                               { BAND_15,  frequency(21.0) },
                                               { BAND_12,  frequency(24.89) },
                                               { BAND_10,  frequency(28.0) }
                                             };

  return MUM_VALUE(edge_map, BAND(*this), frequency(0.0));
}

/// difference in two frequencies, always +ve
frequency frequency::difference(const frequency& f2) const
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
{ for (auto cit { bands.crbegin() }; cit != bands.crend(); ++cit)
  { if (upper_edge(*cit) < *this)
     return *cit;
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
ostream& operator<<(ostream& ost, const frequency& f)
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
      return frequency { 1'800'000 };

    case BAND_80 :
      return frequency { 3'500'000 };

    case BAND_60 :
      return frequency { 5'000'000 };

    case BAND_40 :
      return frequency { 7'000'000 };

    case BAND_30 :
      return frequency { 10'100'000 };

    case BAND_20 :
      return frequency { 14'000'000 };

    case BAND_17 :
      return frequency { 18'068'000 };

    case BAND_15 :
      return frequency { 21'000'000 };

    case BAND_12 :
      return frequency { 24'890'000 };

    case BAND_10 :
      return frequency { 28'000'000 };

    default :
      return frequency { 1'800'000 };
  }
}

/*! \brief      Return the upper edge of a band
    \param  b   target band
    \return     the frequency at the top edge of band <i>b</i>
*/
frequency upper_edge(const BAND b)
{ switch (b)
  { case BAND_160 :
      return frequency { 2'000'000 };

    case BAND_80 :
      return frequency { 4'000'000 };

    case BAND_60 :
      return frequency { 6'000'000 };

    case BAND_40 :
      return frequency { 7'300'000 };

    case BAND_30 :
      return frequency { 10'150'000 };

    case BAND_20 :
      return frequency { 14'350'000 };

    case BAND_17 :
      return frequency { 18'168'000 };

    case BAND_15 :
      return frequency { 21'450'000 };

    case BAND_12 :
      return frequency { 24'990'000 };

    case BAND_10 :
      return frequency { 29'700'000 };

    default :
      return frequency { 1'800'000 };
  }
}
