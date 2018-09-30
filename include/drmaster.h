// $Id: drmaster.h 147 2018-04-20 21:32:50Z  $

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

/*! \brief              Default constructor
    \param  filename    name of file from which to donstruct the object
*/
  master_dta(const std::string& filename = "master.dta");

/*! \brief      Get all the calls
    \return     all the calls from the file
*/
  inline const std::vector<std::string> calls(void) const
    { return _calls; }
};

// -----------------------------------------------------  trmaster_line  ---------------------------------

/*! \class  trmaster_line
    \brief  Manipulate a line from an N6TR TRMASTER.ASC file
*/

const unsigned int TRMASTER_N_USER_PARAMETERS = 5;      ///< the number of user parameters in a TRMASTER file

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
  inline virtual ~trmaster_line(void)
    { }

/*! \brief      Convert to a string
    \return     the line as a string suitable for use in a TRMASTER file
*/
  const std::string to_string(void) const;

  READ_AND_WRITE(call);                       ///< callsign
  READ_AND_WRITE(check);                      ///< Sweepstakes check
  READ_AND_WRITE(cq_zone);                    ///< CQ zone
  READ_AND_WRITE(foc);                        ///< FOC membership number
  READ_AND_WRITE(grid);                       ///< Maidenhead grid locator
  READ_AND_WRITE(hit_count);                  ///< nominal number of QSOs with this station
  READ_AND_WRITE(itu_zone);                   ///< ITU zone
  READ_AND_WRITE(name);                       ///< operator's name
  READ_AND_WRITE(old_call);                   ///< operator's old call
  READ_AND_WRITE(qth);                        ///< precise meaning depends on location of this station
  READ_AND_WRITE(section);                    ///< ARRL section
  READ_AND_WRITE(speed);                      ///< CW speed
  READ_AND_WRITE(ten_ten);                    ///< 10-X membership number

/*! \brief      Test for emptiness
    \return     whether the object is empty
*/
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
    \return         line merged with <i>trml</i>

    New values (i.e., values in <i>trml</i>) take precedence if there's a conflict
*/
  const trmaster_line operator+(const trmaster_line& trml) const;

/*! \brief      Merge with another trmaster_line
    \param  ln  line to be merged

    New values (i.e., values in <i>ln</i>) take precedence if there's a conflict
*/
  inline void operator+=(const trmaster_line& ln)
    { *this = *this + ln; }
};

/*! \brief          Output a line from a TRMASTER file to a stream
    \param  ost     output stream
    \param  trml    the line to be written
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const trmaster_line& trml);

// -----------------------------------------------------  trmaster  ---------------------------------

/*! \class  trmaster
    \brief  Manipulate an N6TR TRMASTER file
*/

class trmaster
{
protected:

  std::map<std::string /* callsign */, trmaster_line> _records;       ///< the information for each call

/*! \brief              Get a "line" from a TRMASTER binary file
    \param  contents    string that contains a record (plus, perhaps, much more)
    \param  posn        position of the call in <i>contents</i> (i.e., the start of the record)
    \return             The TRMASTER record that begins at <i>posn</i>

    Updates <i>posn</i> to point to the start of the next call
*/
  const trmaster_line _get_binary_record(const std::string& contents, uint32_t& posn);

public:

/*! \brief              Constructor
    \param  filename    name of file from which data are to be read

    The file <i>filename</i> may be either an ASCII or a binary file
*/
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
  std::string _cw_power;                                        ///< power received in ARRL DX CW
  std::string _date;                                            ///< most recent date at which the record was updated
  std::string _iota;                                            ///< IOTA designation
  std::string _precedence;                                      ///< Sweepstakes precedence
  std::string _society;                                         ///< HQ designation from IARU contest
  std::string _spc;                                             ///< SKCC state/province/country
  std::string _ssb_power;                                       ///< power received in ARRL DX SSB
  std::string _state_160;                                       ///< for CQ 160m contest: W and VE only
  std::string _state_10;                                        ///< for ARRL 10m contest; W, VE and XE only

/*! \brief                      Extract a single field from the record
    \param  fields              all the fields
    \param  field_indicator     indicator that prefixes the field (for example: "=H")
    \return                     Value of the field with the indicator <i>field_indicator</i>

    Returns empty string if no field has the indicator <i>field_indicator</i>
*/
  const std::string _extract_field(const std::vector<std::string>& fields, const std::string& field_indicator);

public:

/// default constructor
  inline drmaster_line(void)
    { }

/*! \brief                  Construct fronm a call or from a line from a drmaster file
    \param  line_or_call    line from file, or a call

    Constructs an object that contains only the call if <i>line_or_call</i> contains a call
*/
  explicit drmaster_line(const std::string& line_or_call);

/// destructor
  inline virtual ~drmaster_line(void)
    { }

/// convert to a string
  const std::string to_string(void) const;

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
  inline void user(const int n, const std::string& v)
    { _user[n - 1] = v; }

/// get user parameters; wrt 1
  inline const std::string user(const int n) const
    { return _user[n - 1]; }

/// set hit count
  inline void hit_count(const int n)
    { hit_count(::to_string(n)); }

  READ_AND_WRITE(cw_power);                                        ///< power received in ARRL DX CW
  READ_AND_WRITE(date);                                            ///< most recent date at which the record was updated
  READ_AND_WRITE(iota);                                            ///< IOTA designation
  READ_AND_WRITE(precedence);                                      ///< Sweepstakes precedence
  READ_AND_WRITE(society);                                         ///< HQ designation from IARU contest
  READ_AND_WRITE(spc);                                             ///< SKCC state/province/country
  READ_AND_WRITE(ssb_power);                                       ///< power received in ARRL DX SSB
  READ_AND_WRITE(state_160);                                       ///< for CQ 160m contest: W and VE only
  READ_AND_WRITE(state_10);                                        ///< for ARRL 10m contest; W, VE and XE only

/// merge with another drmaster_line; new values take precedence if there's a conflict
  const drmaster_line operator+(const drmaster_line&) const;

/// merge with another drmaster_line; new values take precedence if there's a conflict
  inline void operator+=(const drmaster_line& ln)
    { *this = *this + ln; }

/// increment hit count
  inline void operator++(int)
    { hit_count(::to_string(1 + from_string<int>(hit_count()))); }

/// is the line empty?
  inline const bool empty(void) const
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

  std::unordered_map<std::string, drmaster_line> _records;       ///< the information

public:

/*! \brief              Construct from a file
    \param  filename    name of file to read

    Throws exception if the file does not exist or is incorrectly formatted
*/
  drmaster(const std::string& filename = "drmaster");

/*! \brief              Construct from a file
    \param  path        directories to check
    \param  filename    name of file to read

    Constructs from the first instance of <i>filename</i> when traversing the <i>path</i> directories.
    Throws exception if the file does not exist or is incorrectly formatted
*/
  drmaster(const std::vector<std::string>& path, const std::string& filename = "drmaster");

/*! \brief              Prepare the object by reading a file
    \param  filename    name of file to read

    Throws exception if the file does not exist or is incorrectly formatted
*/
  void prepare(const std::string& filename = "drmaster");

/*! \brief              Prepare the object by reading a file
    \param  path        directories to check
    \param  filename    name of file to read

    Processes the first instance of <i>filename</i> when traversing the <i>path</i> directories.
    Throws exception if the file does not exist or is incorrectly formatted
*/
  void prepare(const std::vector<std::string>& path, const std::string& filename = "drmaster");

/// all the calls (in alphabetical order)
  const std::vector<std::string> calls(void) const;

/// format for output
  const std::string to_string(void) const;

/*! \brief          Add a callsign.
    \param  call    call to add

    If there's already an entry for <i>call</i>, then does nothing
*/
  void operator+=(const std::string& call);

/*! \brief          Add a drmaster_line
    \param  drml    drmaster line to add

    If there's already an entry for the call in <i>drml</i>, then performs a merge
*/
  void operator+=(const drmaster_line& drml);

/// the number of records
  inline const size_t size(void) const
    { return _records.size(); }

/*! \brief          Return the record for a particular call
    \param  call    target callsign
    \return         the record corresponding to <i>call</i>

    Returns empty <i>drmaster_line</i> object if no record corresponds to callsign <i>call</i>
*/
  const drmaster_line operator[](const std::string& call) const;

/*! \brief          Remove a call
    \param  call    target callsign

    Does nothing if <i>call</i> is not present
*/
  inline void operator-=(const std::string& call)
    { _records.erase(call); }

/*! \brief          Remove a call
    \param  call    target callsign

    Does nothing if <i>call</i> is not present
*/
  inline void remove(const std::string& call)
    { *this -= call; }

/*! \brief          Is a particular call present in the file?
    \param  call    target callsign
    \return         whether <i>call</i> is present
*/
  inline const bool contains(const std::string& call) const
    { return ( _records.find(call) != _records.cend() ); }
};

#endif    // DRMASTER_H
