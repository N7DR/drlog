// $Id: trlog.h 145 2018-03-19 17:28:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   trlog.h

    Classes and functions related to TRlog log files
*/

#ifndef TRLOGH
#define TRLOGH

#include "bands-modes.h"

#include <string>

#include <cstdio>

/*!     \class QSO_CALL
        \brief Encapsulates a QSO number and a callsign
*/

struct QSO_CALL
{ int _qso;                 ///< QSO number
  std::string _call;        ///< callsign
};

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
  inline const int _convert_to_int(const int posn, const int len) const
    { return from_string<int>( substring(_record, posn, len) ); }

public:

/// default constructor
  tr_record(void) { }

/*! \brief  Construct from a line of a TRLOG file
    \param  s  a single line from a TRLOG file
*/
  explicit tr_record(const std::string& s) :
    _record(s)
    { }

/// copy constructor
  tr_record(const tr_record& rec)
    { _record = rec._record; }

/*! \brief  Construct from a line of a TRLOG file
    \param  cp  a single line from a TRLOG file
*/
  explicit tr_record(const char* cp)
    { _record = cp; }

/// tr_record = tr_record
  inline void operator=(const tr_record& rec)
    { _record = rec._record; }

/// tr_record = char*
  inline void operator=(const char* cp)
    { _record = cp; }

// retrieve information

/// callsign
  inline const std::string call(void) const
    { return remove_trailing_spaces(to_upper(substring(_record, 29, 14))); }

/// mode
  inline const MODE mode(void) const
    { return ( (_record[3] == 'C') ? MODE_CW : MODE_SSB ); }

  const BAND band(void) const;           ///< band

/// day of the month; 1 - 31
  inline const int day(void) const
    { return _convert_to_int(7, 2); }

  const int month(void) const;           ///< month of the year; 1 - 12

  const int year(void) const;            ///< four-digit year

/// hour; 0 - 23
  inline const int hour(void) const
    { return _convert_to_int(17, 2); }

/// minute; 0 - 59
  inline const int minute(void) const
    { return _convert_to_int(20, 2); }

  const int rst(void) const;             ///< sent RST

  const int rst_received(void) const;      ///< received RST

  const std::string frequency(void) const; ///< frequency in MHz

  const std::string exchange_received(void) const;  ///< the received exchange; maximum of four characters

  inline const std::string record(void) const       ///< the entire record
    { return _record; }

  inline const bool sap_mode(void) const            ///< was this a SAP-mode QSO?
    { return contains(_record, "$"); }

  inline const bool cq_mode(void) const             ///< was this a CQ-mode QSO?
    { return !sap_mode(); }
};

// -----------  tr_log  ----------------

/*!     \class tr_log
        \brief Manipulate a TRLOG file
*/

class tr_log
{
protected:

  long _number_of_qsos;      ///< number of QSOs in the log
  long _record_length;       ///< length of a single QSO
  FILE* _fp;                 ///< file pointer

public:

/*! \brief  Construct from a TRLOG file
    \param  filename  name of a TRLOG file
*/
  explicit tr_log(const std::string& filename);

/// destructor
  virtual ~tr_log(void);

/// return record number <i>n</i>
  const tr_record read(const int n);

/*! \brief  Write a record to the file
    \param  trr  the text of the record to be written
    \param  n    the record number at which <i>trr</i> should be written
*/
  void write(const tr_record& trr, const int n);

/// sort the log in order of callsign
  void sort_by_call(void);

/// the number of QSOs in the log
  inline const int number_of_qsos(void) const
    { return _number_of_qsos; }
};

#endif
