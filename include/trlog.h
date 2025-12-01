// $Id: trlog.h 279 2025-12-01 15:09:34Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#if 1

/*! \file   trlog.h

    Classes and functions related to TRlog log files
*/

#ifndef TRLOGH
#define TRLOGH

#include "bands-modes.h"

#include <string>

#include <cstdio>

// -----------  tr_record  ----------------

/*! \class  tr_record
    \brief  Encapsulate a single TRLOG QSO
*/

class tr_record
{
protected:

  std::string _record;                  ///< the record from a file

/*! \brief          Convert part of the record to an integer
    \param  posn    position at which to commence conversion
    \param  len     number of characters to convert

    \return characters converted to an integer
*/
  inline int _convert_to_int(const int posn, const int len) const
    { return from_string<int>( substring <std::string> (_record, posn, len) ); }

public:

/// default constructor
  inline tr_record(void) = default;

/*! \brief      Construct from a line of a TRLOG file
    \param  s   a single line from a TRLOG file
*/
  inline explicit tr_record(const std::string& s) :
    _record(s)
    { }

/// copy constructor
  inline tr_record(const tr_record& rec) = default;

/*! \brief      Construct from a line of a TRLOG file
    \param  cp  a single line from a TRLOG file
*/
  inline explicit tr_record(const char* cp)
    { _record = cp; }

/// tr_record = tr_record
  inline tr_record& operator=(const tr_record& rec) = default;

/// tr_record = char*
  inline void operator=(const char* cp)
    { _record = cp; }

// retrieve information

/// callsign
  inline std::string call(void) const
    { return remove_trailing_spaces <std::string> (to_upper(substring <std::string> (_record, 29, 14))); }

/// mode
  inline MODE mode(void) const
    { return ( (_record[3] == 'C') ? MODE_CW : MODE_SSB ); }

  BAND band(void) const;           ///< band

/// day of the month; 1 - 31
  inline int day(void) const
    { return _convert_to_int(7, 2); }

/// month of the year; 1 - 12
  int month(void) const;

/// four-digit year
  int year(void) const;

/// hour; 0 - 23
  inline int hour(void) const
    { return _convert_to_int(17, 2); }

/// minute; 0 - 59
  inline int minute(void) const
    { return _convert_to_int(20, 2); }

/// sent RST
  int rst(void) const;

/// received RST
  inline int rst_received(void) const
    { return from_string<int>(substring <std::string> (_record, 49, _record[51] == ' ' ? 2 : 3)); }

  std::string frequency(void) const; ///< frequency in MHz

/// the received exchange; maximum of four characters
  inline std::string exchange_received(void) const
    { return remove_peripheral_spaces <std::string> (substring <std::string> (_record, 53, 4)); }

  inline std::string record(void) const       ///< the entire record
    { return _record; }

  inline bool sap_mode(void) const            ///< was this a SAP-mode QSO?
//    { return contains(_record, "$"s); }
    { return _record.contains('$'); }

  inline bool cq_mode(void) const             ///< was this a CQ-mode QSO?
    { return !sap_mode(); }
};

// -----------  tr_log  ----------------

/*!     \class tr_log
        \brief Manipulate a TRLOG file
*/

class tr_log
{
protected:

  long   _number_of_qsos;      ///< number of QSOs in the log
  size_t _record_length;       ///< length of a single QSO -- use size_t to match parameter to fwrite
  FILE*  _fp;                  ///< file pointer

public:

/*! \brief  Construct from a TRLOG file
    \param  filename  name of a TRLOG file
*/
  explicit tr_log(const std::string& filename);

/// destructor
  inline ~tr_log(void)
    { fclose(_fp); }              // not really necessary

/// return record number <i>n</i>
  tr_record read(const int n);

/*! \brief          Write a record to the file
    \param  trr     the text of the record to be written
    \param  n       the record number at which <i>trr</i> should be written
*/
  void write(const tr_record& trr, const int n);

/// sort the log in order of callsign
  void sort_by_call(void);

/// the number of QSOs in the log
  inline int number_of_qsos(void) const
    { return _number_of_qsos; }
};

#endif

#endif  // 0
