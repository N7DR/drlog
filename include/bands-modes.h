// $Id: bands-modes.h 84 2014-11-15 19:20:13Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*
 * bands-modes.h
 *
 *  Created on: Nov 24, 2010
 *      Author: n7dr
 */

#ifndef BANDSMODES_H
#define BANDSMODES_H

#include "string_functions.h"

#include <array>
#include <map>
#include <string>

/// bands that drlog knows about
enum  BAND  { BAND_160 = 0,
              BAND_80,
              BAND_60,
              BAND_40,
              BAND_30,
              BAND_20,
              BAND_17,
              BAND_15,
              BAND_12,
              BAND_10,
              MIN_BAND = BAND_160,
              MAX_BAND = BAND_10
            };

const unsigned int NUMBER_OF_BANDS = MAX_BAND + 1;  ///< how many bands does drlog know about?
const unsigned int N_BANDS = NUMBER_OF_BANDS;       ///< how many bands does drlog know about?

const unsigned int ALL_BANDS = static_cast<unsigned int>(MAX_BAND) + 1;  ///< indicator used to mean "all bands"

/// band names
static std::array<std::string, NUMBER_OF_BANDS> BAND_NAME { { "160",
                                                              "80",
                                                              "60",
                                                              "40",
                                                              "30",
                                                              "20",
                                                              "17",
                                                              "15",
                                                              "12",
                                                              "10"
                                                          } };

/// generate the band from a name
static std::map<std::string, BAND> BAND_FROM_NAME { { "160", BAND_160 },
                                                    { "80",  BAND_80 },
                                                    { "60",  BAND_60 },
                                                    { "40",  BAND_40 },
                                                    { "30",  BAND_30 },
                                                    { "20",  BAND_20 },
                                                    { "17",  BAND_17 },
                                                    { "15",  BAND_15 },
                                                    { "12",  BAND_12 },
                                                    { "10",  BAND_10 }
                                                  };

/// modes drlog knows about
enum  MODE  { MODE_CW = 0,
              MODE_SSB,
              MIN_MODE = MODE_CW,
              MAX_MODE = MODE_SSB
            };

const unsigned int NUMBER_OF_MODES = MAX_MODE + 1;  ///< how many modes does drlog know about?
const unsigned int N_MODES = NUMBER_OF_MODES;       ///< how many modes does drlog know about?

/// mode names
static std::array<std::string, NUMBER_OF_MODES> MODE_NAME = { { "CW",
                                                                "SSB"
                                                            } };

typedef std::pair<BAND, MODE> bandmode;    ///< tuple for encapsulating a band and mode

// ----------------------------------------------------  frequency  -----------------------------------------------

/*!     \class frequency
        \brief A convenient class for handling frequencies
*/

class frequency
{
protected:

  unsigned int _hz;      ///< the actual frequency, in Hz

public:

/// default constructor
  frequency(void);

/*! \brief      construct from a double
    \param f    frequency in Hz, kHz or MHz
*/
  explicit frequency(const double f);

/*! \brief      construct from a string
    \param str  frequency in Hz, kHz or MHz
*/
  explicit frequency(const std::string& str)
    { *this = frequency(from_string<double>(str)); }

/*! \brief      construct from a band
    \param b    band

    Sets the frequency to the low edge of the band <i>b</i>
*/
  explicit frequency(const enum BAND b);

/// set frequency in Hz
  inline void hz(const unsigned int n)
    { _hz = n; }

/// get frequency in Hz
  inline const int Hz(void) const
    { return static_cast<int>(_hz); }

/// get frequency in Hz; provided only because g++ gives a compile-time error if one actually uses Hz()
  inline const int hz(void) const
    { return static_cast<int>(_hz); }

/// get frequency in kHz
  inline const float kHz(void) const
    { return static_cast<float>(_hz) / 1000; }

/// get frequency in kHz
  inline const float khz(void) const
    { return static_cast<float>(_hz) / 1000; }

/// get frequency in MHz
  inline const float MHz(void) const
    { return static_cast<float>(_hz) / 1000000; }

/// get frequency in MHz (even though I shudder at the use of "m" to mean "mega")
  inline const float mhz(void) const
    { return static_cast<float>(_hz) / 1000000; }

/// get frequency in kHz, rounded to the nearest kHz
  inline const int rounded_kHz(void) const
    { return static_cast<int>(kHz() + 0.5); }

/*! \brief      return string suitable for use in bandmap
    \return     string of the frequency in kHz, to one decimal place ([x]xxxx.y)

    Sets the frequency to the low edge of the band <i>b</i>
*/
  const std::string display_string(void) const;

/*! \brief      convert to BAND
    \return     BAND in which the frequency is located

    Returns BAND_160 if the frequency is outside all bands
*/
  operator BAND(void) const;

/// is the frequency within a band?
  const bool is_within_ham_band(void) const;

/// return lower band edge that corresponds to frequency
  const frequency lower_band_edge(void) const;

/// frequency == frequency
  inline const bool operator==(const frequency& f) const
    { return (_hz == f._hz); }

/// frequency != frequency
  inline const bool operator!=(const frequency& f) const
    { return !(*this == f); }

/// frequency < frequency
  inline const bool operator<(const frequency& f) const
    { return (_hz < f._hz); }

/// frequency <= frequency
  inline const bool operator<=(const frequency& f) const
    { return (_hz <= f._hz); }

/// frequency > frequency
  inline const bool operator>(const frequency& f) const
    { return (_hz > f._hz); }

/// frequency >= frequency
  inline const bool operator>=(const frequency& f) const
    { return (_hz >= f._hz); }

/// allow archiving
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _hz;
    }
};

/*!  \brief  convert a frequency to a band
     \param  f frequency
     \return band corresponding to <i>f</i>

     Frequency may be in Hz, kHz or MHz.
*/
template<class T> const BAND to_BAND(T f)
{ if (f <= 0)
    return MIN_BAND;

  if (f < 1000)       // MHz
    return to_BAND(static_cast<long>(f) * 1000000);

  if (f < 1000000)    // kHz
    return to_BAND(static_cast<long>(f) * 1000);

// Hz
  if ( (f >= 1800000) and (f <= 2000000) )
    return BAND_160;

  if ( (f >= 3500000) and (f <= 4000000) )
    return BAND_80;

  if ( (f >= 5000000) and (f <= 6000000) )
    return BAND_60;

  if ( (f >= 7000000) and (f <= 7300000) )
    return BAND_40;

  if ( (f >= 1010000) and (f <= 10150000) )
    return BAND_30;

  if ( (f >= 14000000) and (f <= 14350000) )
    return BAND_20;

  if ( (f >= 18000000) and (f <= 19000000) )
    return BAND_17;

  if ( (f >= 21000000) and (f <= 21450000) )
    return BAND_15;

  if ( (f >= 24890000) and (f <= 25000000) )
    return BAND_12;

  if ( (f >= 28000000) and (f <= 29700000) )
    return BAND_10;

  return MIN_BAND;
}

/*!  \brief         convert the string representation of a frequency to a band
     \param  str    any string representation of a frequency, such that the string can be converted to a frequency object
     \return        band corresponding to <i>f</i>

     Frequency may be in Hz, kHz or MHz.
*/
inline const BAND to_BAND(const std::string& str)
{ return to_BAND(frequency(str).hz());
}

/*!  \brief         convert a frequency to a band
     \param  f      frequency to convert
     \return        band corresponding to <i>f</i>
*/
inline const BAND to_BAND(const frequency& f)
{ return to_BAND(f.hz());
}

/*!  \brief         convert a frequency to a string
     \param  f      frequency to convert
     \return        comma-separated string of the frequency in Hz

     Appends " Hz" to the numerical frequency.
*/
inline const std::string to_string(const frequency& f)
{ return (comma_separated_string(f.hz()) + " Hz");
}

#endif /* BANDSMODES_H */
