// $Id: bands-modes.h 138 2017-06-20 21:41:26Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef BANDSMODES_H
#define BANDSMODES_H

/*! \file bands-modes.h

    Classes and functions related to bands, frequencies and modes
*/

#include "string_functions.h"

#include <array>
#include <map>
#include <string>

enum FREQ_UNIT { FREQ_HZ = 0,
                 FREQ_KHZ,
                 FREQ_MHZ
               };                                   ///< units for measuring frequency

enum BAND { BAND_160 = 0,
            BAND_80,
            BAND_60,
            BAND_40,
            BAND_30,
            BAND_20,
            BAND_17,
            BAND_15,
            BAND_12,
            BAND_10,
            ANY_BAND,
            ALL_BANDS = ANY_BAND,
            MIN_BAND = BAND_160,
            MAX_BAND = BAND_10
          };                                        ///< bands that drlog knows about

const unsigned int NUMBER_OF_BANDS = MAX_BAND + 1;                          ///< how many bands does drlog know about?
const unsigned int N_BANDS = NUMBER_OF_BANDS;                               ///< how many bands does drlog know about?

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
                                                           } };         ///< names of bands

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
                                                  };                    ///< map a band name to a band

enum MODE { MODE_CW = 0,
            MODE_SSB,
            MODE_RTTY,
            ANY_MODE,
            MIN_MODE = MODE_CW,
            MAX_MODE = ANY_MODE - 1
          };                                        ///< modes that drlog knows about

const unsigned int NUMBER_OF_MODES = MAX_MODE + 1;  ///< how many modes does drlog know about?
const unsigned int N_MODES = NUMBER_OF_MODES;       ///< how many modes does drlog know about?
const unsigned int ALL_MODES = N_MODES;             ///< indicator used to mean "all modes"

/// mode names
static std::array<std::string, NUMBER_OF_MODES> MODE_NAME = { { "CW",
                                                                "SSB",
                                                                "RTTY"
                                                            } };

/// generate the mode from a name
static std::map<std::string, MODE> MODE_FROM_NAME { { "CW", MODE_CW },
                                                    { "SSB",  MODE_SSB },
                                                    { "RTTY",  MODE_RTTY }
                                                  };

typedef std::pair<BAND, MODE> bandmode;    ///< tuple for encapsulating a band and mode

/*!  \brief     Convert a frequency to a band
     \param  f  frequency
     \return    band corresponding to <i>f</i>

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

  if ( (f >= 18068000) and (f <= 18168000) )
    return BAND_17;

  if ( (f >= 21000000) and (f <= 21450000) )
    return BAND_15;

  if ( (f >= 24890000) and (f <= 24990000) )
    return BAND_12;

  if ( (f >= 28000000) and (f <= 29700000) )
    return BAND_10;

  return MIN_BAND;
}

// ----------------------------------------------------  frequency  -----------------------------------------------

/*! \class  frequency
    \brief  A convenient class for handling frequencies
*/

class frequency
{
protected:

  unsigned int _hz;      ///< the actual frequency, in Hz

public:

/// default constructor
  frequency(void);

/*! \brief      Construct from a double
    \param f    frequency in Hz, kHz or MHz
*/
  explicit frequency(const double f);

/*! \brief          Construct from a double and an explicit unit
    \param f        frequency in Hz, kHz or MHz
    \param unit     frequency unit
*/
    frequency(const double f, const FREQ_UNIT unit);

/*! \brief      Construct from a string
    \param str  frequency in Hz, kHz or MHz
*/
  explicit frequency(const std::string& str)
    { *this = frequency(from_string<double>(str)); }

/*! \brief      Construct from a band
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

/*! \brief      Return string suitable for use in bandmap
    \return     string of the frequency in kHz, to one decimal place ([x]xxxx.y)
*/
  const std::string display_string(void) const;

/*! \brief      Convert to BAND
    \return     BAND in which the frequency is located

    Returns BAND_160 if the frequency is outside all bands
*/
  inline operator BAND(void) const
    { return to_BAND(hz()); }

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

/// serialize using boost
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _hz;
    }
};

/*!  \brief         Convert the string representation of a frequency to a band
     \param  str    any string representation of a frequency, such that the string can be converted to a frequency object
     \return        band corresponding to <i>f</i>

     Frequency may be in Hz, kHz or MHz.
*/
inline const BAND to_BAND(const std::string& str)
  { return to_BAND(frequency(str).hz()); }

/*!  \brief     Convert a frequency to a band
     \param  f  frequency to convert
     \return    band corresponding to <i>f</i>
*/
inline const BAND to_BAND(const frequency& f)
  { return to_BAND(f.hz()); }

/*!  \brief     Convert a frequency to a printable string
     \param  f  frequency to convert
     \return    comma-separated string of the frequency in Hz

     Appends " Hz" to the numerical frequency.
*/
inline const std::string to_string(const frequency& f)
  { return (comma_separated_string(f.hz()) + " Hz"); }

/// mode break points; CW below the break point, SSB above it
static std::map<BAND, frequency> MODE_BREAK_POINT { { BAND_160, frequency(1900) },
                                                    { BAND_80,  frequency(3600) },
                                                    { BAND_60,  frequency(5500) },
                                                    { BAND_40,  frequency(7100) },
                                                    { BAND_30,  frequency(10150) },
                                                    { BAND_20,  frequency(14150) },
                                                    { BAND_17,  frequency(18900) },
                                                    { BAND_15,  frequency(21200) },
                                                    { BAND_12,  frequency(24910) },
                                                    { BAND_10,  frequency(28300) }
                                                  };

#endif /* BANDSMODES_H */
