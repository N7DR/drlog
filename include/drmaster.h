// $Id: drmaster.h 290 2026-03-30 15:48:47Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   drmaster.h

    Classes associated with MASTER.DTA, TRMASTER.[DTA, ASC] and drmaster files.

    Parts of this file are based on the very old trm.h for the manipulation of TRLOG files
*/

#ifndef DRMASTER_H
#define DRMASTER_H

#include "macros.h"
#include "string_functions.h"

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------  drmaster_line  ---------------------------------

/*! \class  drmaster_line
    \brief  Manipulate a line from a drmaster file
*/

constexpr unsigned int TRMASTER_N_USER_PARAMETERS { 5 };      ///< the number of user parameters in a TRMASTER file

class drmaster_line
{
protected:

  std::string _call;                                            ///< callsign
  std::string _check;                                           ///< Sweepstakes check
  std::string _cq_zone;                                         ///< CQ zone
  std::string _foc;                                             ///< FOC number
  std::string _hit_count;                                       ///< hit count
  std::string _qth;                                             ///< QTH information (actual information varies as a function of country)
  std::string _ten_ten;                                         ///< 10-10 number

  std::array<std::string, TRMASTER_N_USER_PARAMETERS> _user;    ///< user-defined fields
  std::string _section;                                         ///< ARRL/RAC section
  std::string _name;                                            ///< name
  std::string _grid;                                            ///< Maidenhead grid square
  std::string _itu_zone;                                        ///< ITU zone
  std::string _old_call;                                        ///< old callsign
  std::string _speed;                                           ///< CW speed

// extensions for drlog
  std::string _age_aa_cw;                                       ///< age received in AA CW
  std::string _age_aa_ssb;                                      ///< age received in AA SSB
  std::string _cw_power;                                        ///< power received in ARRL DX CW
  std::string _date;                                            ///< most recent date at which the record was updated
  std::string _iota;                                            ///< IOTA designation
  std::string _precedence;                                      ///< Sweepstakes precedence
  std::string _qth2;                                            ///< alternative QTH information (actual information varies as a function of country)
  std::string _skcc;                                            ///< SKCC number
  std::string _society;                                         ///< HQ designation from IARU contest
  std::string _spc;                                             ///< SKCC state/province/country
  std::string _ssb_power;                                       ///< power received in ARRL DX SSB
  std::string _state_160;                                       ///< for CQ 160m contest: W and VE only
  std::string _state_10;                                        ///< for ARRL 10m contest; W, VE and XE only

  int _xscp { 0 };                                              ///< extended SCP value

/*! \brief                      Extract a single field from the record
    \param  fields              all the fields (e.g., "=Xabc")
    \param  field_indicator     indicator that prefixes the field (for example: "=H")
    \return                     Value of the field with the indicator <i>field_indicator</i>

    Returns empty string if no field has the indicator <i>field_indicator</i>
*/
std::string _extract_field(const std::vector<std::string>& fields, const std::string_view field_indicator);

/*! \brief      Process a single field (such as: "=Xabc")
    \param  sv  the field to processs (e.g., "=Xabc")

    Does nothing if the field does not exist
*/
  void _process_field(const std::string_view sv);

public:

/// default constructor
  inline drmaster_line(void) = default;

/*! \brief                  Construct from a call or from a line from a drmaster file
    \param  line_or_call    line from file, or a call

    Constructs an object that contains only the call if <i>line_or_call</i> contains a call
*/
  explicit drmaster_line(const std::string_view line_or_call);

/// convert to a string
  std::string to_string(void) const;

// the usual get/set functions
  READ_AND_WRITE(call);                                            ///< callsign
  READ_AND_WRITE(check);                                           ///< Sweepstakes check
  READ_AND_WRITE(cq_zone);                                         ///< CQ zone
  READ_AND_WRITE(foc);                                             ///< FOC number
  READ_AND_WRITE(grid);                                            ///< Maidenhead grid square
  READ_AND_WRITE(hit_count);                                       ///< hit count
  READ_AND_WRITE(itu_zone);                                        ///< ITU zone
  READ_AND_WRITE(name);                                            ///< name
  READ_AND_WRITE(qth);                                             ///< QTH information (actual information varies as a function of country)
  READ_AND_WRITE(old_call);                                        ///< old callsign
  READ_AND_WRITE(section);                                         ///< ARRL/RAC section
  READ_AND_WRITE(speed);                                           ///< CW speed
  READ_AND_WRITE(ten_ten);                                         ///< 10-10 number

/// set user parameters; wrt 1
  inline void user(const int n, const std::string_view sv)
    { _user.at(n - 1) = sv; }

/// get user parameters; wrt 1
  inline std::string user(const int n) const
    { return _user.at(n - 1); }

/// set hit count
  inline void hit_count(const int n)
    { hit_count(::to_string(n)); }

  READ_AND_WRITE(age_aa_cw);                                       ///< age received in AA CW
  READ_AND_WRITE(age_aa_ssb);                                      ///< age received in AA SSB
  READ_AND_WRITE(cw_power);                                        ///< power received in ARRL DX CW
  READ_AND_WRITE(date);                                            ///< most recent date at which the record was updated
  READ_AND_WRITE(iota);                                            ///< IOTA designation
  READ_AND_WRITE(precedence);                                      ///< Sweepstakes precedence
  READ_AND_WRITE(qth2);                                            ///< alternative QTH information (actual information varies as a function of country)
  READ_AND_WRITE(skcc);                                            ///< SKCC number
  READ_AND_WRITE(society);                                         ///< HQ designation from IARU contest
  READ_AND_WRITE(spc);                                             ///< SKCC state/province/country
  READ_AND_WRITE(ssb_power);                                       ///< power received in ARRL DX SSB
  READ_AND_WRITE(state_160);                                       ///< for CQ 160m contest: W and VE only
  READ_AND_WRITE(state_10);                                        ///< for ARRL 10m contest; W, VE and XE only
  READ_AND_WRITE(xscp);                                            ///< extended SCP value

/// merge with another drmaster_line; new values take precedence if there's a conflict
  drmaster_line operator+(const drmaster_line&) const;

/// merge with another drmaster_line; new values take precedence if there's a conflict
  inline void operator+=(const drmaster_line& ln)
    { *this = *this + ln; }

/// increment hit count
  inline void operator++(int)
    { hit_count(::to_string(1 + from_string<int>(hit_count()))); }

/// is the line empty?
  inline bool empty(void) const
    { return _call.empty(); }
};

/*! \brief          Write a <i>drmaster_line</i> object to an output stream
    \param  ost     output stream
    \param  trml    object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const drmaster_line& trml);

// -----------------------------------------------------  drmaster  ---------------------------------

/*! \class  drmaster
    \brief  Manipulate a drmaster file

    A drmaster file is a superset of a TRMASTER.ASC file
*/

class drmaster
{
protected:

  UNORDERED_STRING_MAP<drmaster_line> _records;       ///< the information; key = call

public:

  drmaster(void) = default;

/*! \brief              Construct from a file
    \param  filename    name of file to read
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Lines without XSCP data are always included

    Throws exception if the file does not exist or is incorrectly formatted;
*/
  explicit drmaster(const std::string_view filename, const int xscp_limit = 1);

/*! \brief              Construct from a file
    \param  path        directories to check
    \param  filename    name of file to read
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Lines without XSCP data are always included

    Constructs from the first instance of <i>filename</i> when traversing the <i>path</i> directories.
    Throws exception if the file does not exist or is incorrectly formatted
*/
  drmaster(const std::vector<std::string>& path, const std::string_view filename, const int xscp_limit = 1);

/// all the calls (in callsign order)
  std::vector<std::string> calls(void) const;

/// all the calls (in random order)
  std::vector<std::string> unordered_calls(void) const;

/// format for output
  std::string to_string(void) const;

/*! \brief          Add a callsign.
    \param  call    call to add

    If there's already an entry for <i>call</i>, then does nothing
*/
  void operator+=(const std::string_view call);

/*! \brief          Add a drmaster_line
    \param  drml    drmaster line to add

    If there's already an entry for the call in <i>drml</i>, then performs a merge
*/
  void operator+=(const drmaster_line& drml);

/// the number of records
  inline size_t size(void) const
    { return _records.size(); }

/*! \brief          Return the record for a particular call
    \param  call    target callsign
    \return         the record corresponding to <i>call</i>

    Returns empty <i>drmaster_line</i> object if no record corresponds to callsign <i>call</i>
*/
  inline drmaster_line operator[](const std::string_view call) const
    { return MUM_VALUE(_records, call); }
  
/*! \brief          Return the record for a particular call
    \param  call    target callsign
    \return         the record corresponding to <i>call</i>

    Returns empty <i>drmaster_line</i> object if no record corresponds to callsign <i>call</i>
*/
  inline drmaster_line data(const std::string_view call) const
    { return ((*this)[call]); }

/*! \brief          Remove a call
    \param  call    target callsign

    Does nothing if <i>call</i> is not present
*/
  inline void operator-=(const std::string_view call)
    { _records -= call; }

/*! \brief          Remove a call
    \param  call    target callsign

    Does nothing if <i>call</i> is not present
*/
  inline void remove(const std::string_view call)
    { *this -= call; }

/*! \brief          Is a particular call present in the file?
    \param  call    target callsign
    \return         whether <i>call</i> is present
*/
  inline bool contains(const std::string_view call) const
    {  return _records.contains(call); }

/*! \brief      Return object with only records with xscp below a given percentage value
    \param  pc  percentage limit
    \return     <i>drmaster</i> object containing only records with no xscp, and those for which the xscp value is >= the value of <i>pc</i>
*/
  drmaster prune(const int pc) const;
};

#endif    // DRMASTER_H
