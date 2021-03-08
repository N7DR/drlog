// $Id: bands-modes.h 179 2021-02-22 15:55:56Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef BANDSMODES_H
#define BANDSMODES_H

/*! \file   bands-modes.h

    Classes and functions related to bands, frequencies and modes
*/

#include "string_functions.h"

#include <array>
#include <map>
#include <string>

/// units for measuring frequency
enum class FREQUENCY_UNIT { HZ,
                            KHZ,
                            MHZ
                          };

/// bands that drlog knows about
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
          };

constexpr unsigned int NUMBER_OF_BANDS { MAX_BAND + 1 };                          ///< how many bands does drlog know about?
constexpr unsigned int N_BANDS         { NUMBER_OF_BANDS };                       ///< how many bands does drlog know about?

static std::array<std::string, NUMBER_OF_BANDS> BAND_NAME { { "160"s,
                                                              "80"s,
                                                              "60"s,
                                                              "40"s,
                                                              "30"s,
                                                              "20"s,
                                                              "17"s,
                                                              "15"s,
                                                              "12"s,
                                                              "10"s
                                                           } };         ///< names of bands

static std::map<std::string, BAND> BAND_FROM_NAME { { "160"s, BAND_160 },
                                                    { "80"s,  BAND_80 },
                                                    { "60"s,  BAND_60 },
                                                    { "40"s,  BAND_40 },
                                                    { "30"s,  BAND_30 },
                                                    { "20"s,  BAND_20 },
                                                    { "17"s,  BAND_17 },
                                                    { "15"s,  BAND_15 },
                                                    { "12"s,  BAND_12 },
                                                    { "10"s,  BAND_10 }
                                                  };                    ///< map a band name to a band

static std::map<std::string, BAND> BAND_FROM_ADIF3_NAME { { "160m"s, BAND_160 },
                                                          { "80m"s,  BAND_80 },
                                                          { "60m"s,  BAND_60 },
                                                          { "40m"s,  BAND_40 },
                                                          { "30m"s,  BAND_30 },
                                                          { "20m"s,  BAND_20 },
                                                          { "17m"s,  BAND_17 },
                                                          { "15m"s,  BAND_15 },
                                                          { "12m"s,  BAND_12 },
                                                          { "10m"s,  BAND_10 }
                                                        };                    ///< map an ADIF3 band to a band


/// modes that drlog knows about
enum MODE { MODE_CW = 0,
            MODE_SSB,
            MODE_RTTY,
            ANY_MODE,
            MIN_MODE = MODE_CW,
            MAX_MODE = ANY_MODE - 1
          };

constexpr unsigned int NUMBER_OF_MODES { MAX_MODE + 1 };        ///< how many modes does drlog know about?
constexpr unsigned int N_MODES         { NUMBER_OF_MODES };     ///< how many modes does drlog know about?
constexpr unsigned int ALL_MODES       { N_MODES };             ///< indicator used to mean "all modes"

/// mode names
static const std::array<std::string, NUMBER_OF_MODES> MODE_NAME = { "CW"s,
                                                                    "SSB"s,
                                                                    "RTTY"s
                                                                  };

/// generate the mode from a name
static const std::map<std::string, MODE> MODE_FROM_NAME { { "CW"s,   MODE_CW },
                                                          { "SSB"s,  MODE_SSB },
                                                          { "RTTY"s, MODE_RTTY }
                                                        };

static const std::map<BAND, std::string> BOTTOM_OF_BAND { { BAND_160, "1800"s },
                                                          { BAND_80,  "3500"s },
                                                          { BAND_60,  "5330.5"s },
                                                          { BAND_40,  "7100"s },
                                                          { BAND_30,  "10100"s },
                                                          { BAND_20,  "14000"s },
                                                          { BAND_17,  "18068"s },
                                                          { BAND_15,  "21000"s },
                                                          { BAND_12,  "24890"s },
                                                          { BAND_10,  "28000"s }
                                                        };

using bandmode = std::pair<BAND, MODE>;    ///< tuple for encapsulating a band and mode

// forward declaration
class frequency;

extern const std::unordered_map<bandmode, frequency > DEFAULT_FREQUENCIES;    ///< default frequencies, per-band and per-mode

/*!  \brief     Convert a frequency to a band
     \param  f  frequency
     \return    band corresponding to <i>f</i>

     Frequency may be in Hz, kHz or MHz.
*/
template<class T> const BAND to_BAND(T f)
{ if (f <= 0)
    return MIN_BAND;

  if (f < 1000)       // MHz
    return to_BAND(static_cast<long>(f) * 1'000'000);

  if (f < 1'000'000)    // kHz
    return to_BAND(static_cast<long>(f) * 1000);

// Hz
  if ( (f >= 1'800'000) and (f <= 2'000'000) )
    return BAND_160;

  if ( (f >= 3'500'000) and (f <= 4'000'000) )
    return BAND_80;

  if ( (f >= 5'000'000) and (f <= 6'000'000) )
    return BAND_60;

  if ( (f >= 7'000'000) and (f <= 7'300'000) )
    return BAND_40;

  if ( (f == 10'000'000) or ((f >= 10'100'000) and (f <= 10'150'000)) )
    return BAND_30;

  if ( (f >= 14'000'000) and (f <= 14'350'000) )
    return BAND_20;

  if ( (f == 18'000'000) or ((f >= 18'068'000) and (f <= 18'168'000)) )
    return BAND_17;

  if ( (f >= 21'000'000) and (f <= 21'450'000) )
    return BAND_15;

  if ( (f == 24'000'000) or ((f >= 24'890'000) and (f <= 24'990'000)) )
    return BAND_12;

  if ( (f >= 28'000'000) and (f <= 29'700'000) )
    return BAND_10;

  return MIN_BAND;
}

// ----------------------------------------------------  frequency  -----------------------------------------------

/*! \class  frequency
    \brief  A convenient class for handling frequencies
*/

class frequency
{ using HZ_TYPE = uint32_t;     // type used to hold the value in hertz

protected:

  HZ_TYPE _hz { 0 };      ///< the actual frequency, in Hz

public:

/// default constructor
  frequency(void) = default;

/*! \brief      Construct from a double
    \param f    frequency in Hz, kHz or MHz
*/
  explicit frequency(const double f);

/*! \brief          Construct from a double and an explicit unit
    \param f        frequency in Hz, kHz or MHz
    \param unit     frequency unit
*/
  frequency(const double f, const FREQUENCY_UNIT unit);

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
  inline void hz(const decltype(_hz) n)
    { _hz = n; }

/// get frequency in Hz
  inline int Hz(void) const
    { return static_cast<int>(_hz); }

/// get frequency in Hz; provided only because g++ gives a compile-time error if one actually uses Hz()
  inline int hz(void) const
    { return static_cast<int>(_hz); }

/// get frequency in kHz
  inline float kHz(void) const
    { return static_cast<float>(_hz) / 1000; }

/// get frequency in kHz
  inline float khz(void) const
    { return static_cast<float>(_hz) / 1000; }

/// get frequency in MHz
  inline float MHz(void) const
    { return static_cast<float>(_hz) / 1'000'000; }

/// get frequency in MHz (even though I shudder at the use of "m" to mean "mega")
  inline float mhz(void) const
    { return static_cast<float>(_hz) / 1'000'000; }

/// get frequency in kHz, rounded to the nearest kHz
  inline int rounded_kHz(void) const
    { return static_cast<int>(kHz() + 0.5); }

/*! \brief      Return string suitable for use in bandmap
    \return     string of the frequency in kHz, to one decimal place ([x]xxxx.y)
*/
  std::string display_string(void) const;

/*! \brief      Return frequency in MHz as string (with 3 dp)
    \return     string of the frequency in MHz, to three decimal placex ([xxxx].yyy)
*/
  std::string display_string_MHz(void) const;

/*! \brief      Convert to BAND
    \return     BAND in which the frequency is located

    Returns BAND_160 if the frequency is outside all bands
*/
  inline operator BAND(void) const
    { return to_BAND(hz()); }

/// is the frequency within a band?
  inline bool is_within_ham_band(void) const
    { return ( (BAND(*this) != BAND_160) or ( (_hz >= 1'800'000) and (_hz <= 2'000'000) ) ); }    // check if BAND_160, because that's the returned band if frequency is outside a band

/// return lower band edge that corresponds to frequency
  frequency lower_band_edge(void) const;

/// frequency == frequency
  inline bool operator==(const frequency& f) const
    { return (_hz == f._hz); }

/// frequency != frequency
  inline bool operator!=(const frequency& f) const
    { return !(*this == f); }

/// frequency < frequency
  inline bool operator<(const frequency& f) const
    { return (_hz < f._hz); }

/// frequency <= frequency
  inline bool operator<=(const frequency& f) const
    { return (_hz <= f._hz); }

/// frequency > frequency
  inline bool operator>(const frequency& f) const
    { return (_hz > f._hz); }

/// frequency >= frequency
  inline bool operator>=(const frequency& f) const
    { return (_hz >= f._hz); }

/// difference in two frequencies, always +ve
  frequency difference(const frequency& f2) const;

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _hz;
    }
};

/// ostream << frequency
std::ostream& operator<<(std::ostream& ost, const frequency& f);

/*!  \brief         Convert the string representation of a frequency to a band
     \param  str    any string representation of a frequency, such that the string can be converted to a frequency object
     \return        band corresponding to <i>f</i>

     Frequency may be in Hz, kHz or MHz.
*/
inline BAND to_BAND(const std::string& str)
  { return to_BAND(frequency(str).hz()); }

/*!  \brief     Convert a frequency to a band
     \param  f  frequency to convert
     \return    band corresponding to <i>f</i>
*/
inline BAND to_BAND(const frequency& f)
  { return to_BAND(f.hz()); }

/*!  \brief     Convert a frequency to a printable string
     \param  f  frequency to convert
     \return    comma-separated string of the frequency in Hz

     Appends " Hz" to the numerical frequency.
*/
inline std::string to_string(const frequency& f)
  { return (comma_separated_string(f.hz()) + " Hz"s); }

/// mode break points; CW below the break point, SSB above it
static std::map<BAND, frequency> MODE_BREAK_POINT { { BAND_160, frequency(1'900) },
                                                    { BAND_80,  frequency(3'600) },
                                                    { BAND_60,  frequency(5'500) },
                                                    { BAND_40,  frequency(7'100) },
                                                    { BAND_30,  frequency(10'150) },
                                                    { BAND_20,  frequency(14'150) },
                                                    { BAND_17,  frequency(18'900) },
                                                    { BAND_15,  frequency(21'200) },
                                                    { BAND_12,  frequency(24'910) },
                                                    { BAND_10,  frequency(28'300) }
                                                  };

#endif /* BANDSMODES_H */
