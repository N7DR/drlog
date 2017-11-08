// $Id: drmaster.h 140 2017-11-05 15:16:46Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file drmaster.h

    Classes associated with MASTER.DTA, TRMASTER.[DTA, ASC] and drmaster files.

    Parts of this file are based on the very old trm.h for TRLOG manipulation
*/

#ifndef DRMASTER_H
#define DRMASTER_H

#include "macros.h"
#include "string_functions.h"

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// -----------------------------------------------------  master_dta  ---------------------------------

/*! \class  master_dta
    \brief  Manipulate a K1EA MASTER.DTA file
*/

class master_dta
{
protected:
  std::vector<std::string> _calls;       ///< the calls in the MASTER.DTA file

/*! \brief              Private function to return the call starting at <i>posn</i> in <i>contents</i>
    \param  contents    contents of a MASTER.DTA file
    \param  posn        location at which to start returning call
    \return             call that starts at <i>posn</i> in <i>contents</i>

    <i>posn</i> is updated to point at the start of the next call
*/
  const std::string _get_call(const std::string& contents, size_t& posn) const;

public:

/// constructor
  master_dta(const std::string& filename = "master.dta");

/// return all the calls
  inline const std::vector<std::string> calls(void) const
    { return _calls; }
};

// -----------------------------------------------------  trmaster_line  ---------------------------------

/*! \class  trmaster_line
    \brief  Manipulate a line from an N6TR TRMASTER.ASC file
*/

const unsigned int TRMASTER_N_USER_PARAMETERS = 5;  ///< The number of user parameters in a TRMASTER file

class trmaster_line
{
protected:

  std::string   _call;                                          ///< callsign
  int           _check;                                         ///< Sweepstakes check
  int           _cq_zone;                                       ///< CQ zone
  int           _foc;                                           ///< FOC membership number
  std::string   _grid;                                          ///< Maidenhead grid locator
  int           _hit_count;                                     ///< nominal number of QSOs with this station
  std::string   _itu_zone;                                      ///< ITU zone (string because of the way TR treats HQ stations)
  std::string   _name;                                          ///< operator's name
  std::string   _old_call;                                      ///< operator's old call
  std::string   _qth;                                           ///< precise meaning depends on location of this station
  std::string   _section;                                       ///< ARRL section
  std::string   _speed;                                         ///< CW speed
  int           _ten_ten;                                       ///< 10-X membership number
  std::array<std::string, TRMASTER_N_USER_PARAMETERS> _user;    ///< user parameters

public:

/// default constructor
  trmaster_line(void);

/*! \brief          Construct from a TRMASTER.ASC line
    \param  line    line from the TRMASTER.ASC file
*/
  explicit trmaster_line(const std::string& line);

/// destructor
  virtual ~trmaster_line(void);

/// convert to a string.
  const std::string to_string(void) const;

  READ_AND_WRITE(call);                       ///< callsign
  READ_AND_WRITE(check);                      ///< Sweepstakes check
  READ_AND_WRITE(cq_zone);                    ///< CQ zone
  READ_AND_WRITE(foc);                        ///< FOC membership number
  READ_AND_WRITE(grid);                       ///< Maidenhead grid locator
  READ_AND_WRITE(hit_count);                  ///< nominal number of QSOs with this station
  READ_AND_WRITE(itu_zone);                   ///< ITU zone (because TR treats HQ stations differently)
  READ_AND_WRITE(name);                       ///< operator's name
  READ_AND_WRITE(old_call);                   ///< operator's old call
  READ_AND_WRITE(qth);                        ///< precise meaning depends on location of this station
  READ_AND_WRITE(section);                    ///< ARRL section
  READ_AND_WRITE(speed);                      ///< CW speed
  READ_AND_WRITE(ten_ten);                    ///< 10-X membership number

/// use the emptiness of <i>_call</i> as a proxy for emptiness of the object
  inline const bool empty(void) const
    { return _call.empty(); }

/*! \brief      Set a user parameter
    \param  n   parameter number (with respect to 1)
    \param  v   value to which to set the user parameter
*/
  inline void user(const int n, const std::string& v)
    { _user[n - 1] = v; }

/*!     \brief  get a user parameter
        \param  n  parameter number (with respect to 1)
        \return  the string associated with user parameter <i>n</i> (with respect to 1)
*/
  inline const std::string user(const int n) const
    { return _user[n - 1]; }

/*! \brief          Merge with another trmaster_line
    \param  trml    line to be merged
    \return         line merged with <i>ln</i>

    New values (i.e., values in <i>ln</i>) take precedence if there's a conflict
*/
  const trmaster_line operator+(const trmaster_line& trml) const;

/*!     \brief  merge with another trmaster_line
        \param  ln  line to be merged

        New values (i.e., values in <i>ln</i>) take precedence if there's a conflict
*/
  inline void operator+=(const trmaster_line& ln)
    { *this = *this + ln; }
};

/// ostream << trmaster_line
std::ostream& operator<<(std::ostream& ost, const trmaster_line& trml);

// -----------------------------------------------------  trmaster  ---------------------------------

/*! \class  trmaster
    \brief  Manipulate an N6TR TRMASTER file
*/

class trmaster
{
protected:

  std::map<std::string /* callsign */, trmaster_line> _records;       ///< the information for each call

/// get a "line" from a TRMASTER binary file
  const trmaster_line _get_binary_record(const std::string& contents, uint32_t& posn);

public:

/// constructor -- can take either an ASCII or binary file
  trmaster(const std::string& filename = "trmaster.asc");

/// all the calls (in alphabetical order)
  const std::vector<std::string> calls(void) const;
};

// -----------------------------------------------------  drmaster_line  ---------------------------------

/*! \class  drmaster_line
    \brief  Manipulate a line from a drmaster file
*/

class drmaster_line
{
protected:

  std::string _hit_count;                                       ///< hit count
  std::string _cq_zone;                                         ///< CQ zone
  std::string _check;                                           ///< Sweepstakes check
  std::string _foc;                                             ///< FOC number
  std::string _ten_ten;                                         ///< 10-10 number

  std::string _call;
  std::string _qth;
  std::array<std::string, TRMASTER_N_USER_PARAMETERS> _user;
  std::string _section;
  std::string _name;
  std::string _grid;
  std::string _itu_zone;                // because TR treats HQ stations differently
  std::string _old_call;
  std::string _speed;

// extensions
  std::string _cw_power;
  std::string _date;
  std::string _iota;
  std::string _precedence;
  std::string _society;
  std::string _ssb_power;
  std::string _state_10;        // for ARRL 10m contest; W, VE and XE only

//  string _temporary;
  const std::string _extract_field(const std::vector<std::string>& fields, const std::string& field_indicator);

public:

/// default constructor
  drmaster_line(void)
  { }

/// constructor
  explicit drmaster_line(const std::string& line_or_call);

/// destructor
  virtual ~drmaster_line(void);

// convert to a string
  const std::string to_string(void) const;

// the usual get/set functions
  READ_AND_WRITE(call);
  READ_AND_WRITE(check);
  READ_AND_WRITE(cq_zone);
  READ_AND_WRITE(foc);
  READ_AND_WRITE(grid);
  READ_AND_WRITE(hit_count);
  READ_AND_WRITE(itu_zone);
  READ_AND_WRITE(name);
  READ_AND_WRITE(qth);         // state in US or XE; province in VE
  READ_AND_WRITE(old_call);
  READ_AND_WRITE(section);
  READ_AND_WRITE(speed);
  READ_AND_WRITE(ten_ten);

// user parameters; wrt 1
  inline void user(const int n, const std::string& v)
    { _user[n - 1] = v; }

  inline const std::string user(const int n) const
    { return _user[n - 1]; }

  inline void hit_count(const int n)
    { hit_count(::to_string(n)); }

  READ_AND_WRITE(cw_power);
  READ_AND_WRITE(date);
  READ_AND_WRITE(iota);
  READ_AND_WRITE(precedence);
  READ_AND_WRITE(society);
  READ_AND_WRITE(ssb_power);
  READ_AND_WRITE(state_10);

// merge with another drmaster_line; new values take precedence if there's a conflict
  const drmaster_line operator+(const drmaster_line&) const;

  inline void operator+=(const drmaster_line& ln)
    { *this = *this + ln; }

// increment hit count
  inline void operator++(int)
    { hit_count(::to_string(1 + from_string<int>(hit_count()))); }

  inline const bool empty(void) const
    { return _call.empty(); }
};

std::ostream& operator<<(std::ostream& ost, const drmaster_line& trml);

// -----------------------------------------------------  drmaster  ---------------------------------

/*! \class  drmaster
    \brief  Manipulate a drmaster file

    A drmaster file is a superset of a TRMASTER.ASC file
*/

class drmaster
{
protected:

  std::map<std::string, drmaster_line> _records;       ///< the information

public:

// constructor from a file
  drmaster(const std::string& filename = "drmaster");

// constructor from a file
  drmaster(const std::vector<std::string>& path, const std::string& filename = "drmaster");

  void prepare(const std::string& filename = "drmaster");

  void prepare(const std::vector<std::string>& path, const std::string& filename = "drmaster");

// all the calls (in alphabetical order)
  const std::vector<std::string> calls(void) const;

// format for output
  const std::string to_string(void);

// add a call -- does nothing if the call is already present
  void operator+=(const std::string& str);

// add a drmaster_line. If there's already an entry for this call, then performs a merge
  void operator+=(const drmaster_line& drml);

  inline const size_t size(void) const
    { return _records.size(); }

// return a particular record
  const drmaster_line operator[](const std::string& call);

// remove a call -- does nothing if the call is not present
  void operator-=(const std::string& str);

// remove a call -- does nothing if the call is not present
  inline void remove(const std::string& str)
    { *this -= str; }

// is a particular call present?
  const bool contains(const std::string& call);

};

#endif    // DRMASTER_H
