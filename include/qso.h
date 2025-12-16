// $Id: qso.h 282 2025-12-15 20:55:01Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef QSO_H
#define QSO_H

/*! \file   qso.h

    Classes and functions related to QSO information
*/

#include "drlog_context.h"
#include "macros.h"
#include "rules.h"

#include <iostream>
#include <string>
#include <utility>

extern bool         QSO_DISPLAY_COUNTRY_MULT;   ///< whether country mults are written on the log line
extern unsigned int QSO_MULT_WIDTH;             ///< width of mult fields on log line

// forward declarations
class running_statistics;

// fundamental types are initialized with zero Josuttis, 2nd ed. p. 69
// (We need the is_possible mult field so that things align correctly on the log line)
// WRAPPER_4_SERIALIZE(received_field, std::string, name, std::string, value, bool, is_possible_mult, bool, is_mult);  ///< class to encapsulate received fields

// -----------  received_field  ----------------

/*! \class  received_field
    \brief  A single field received from the other party

    Originally a WRAPPER_4_SERIALIZE; changed because of the interplay of string and string_view
    A kind of WRAPPER-4_SERIALIZE could be implemented as a template, but I don't think it's worth it
*/
class received_field : public std::tuple < std::string /* name */, std::string /* value */, bool /* is_possible_mult */, bool /* is_mult */ >
{
protected:

public:

  received_field(void) = default;

/*! \brief        Construct from provided values
    \param  nm    name
    \param  val   value
    \param  ipm   is a possible mult?
    \param  im    is a mult?
*/
  inline received_field(const std::string_view nm, const std::string_view val, const bool ipm = false, const bool im = false)
  { std::get<0>(*this) = nm;
    std::get<1>(*this) = val;
    std::get<2>(*this) = ipm;
    std::get<3>(*this) = im;
  }

/// return the name
  inline std::string name(void) const
    { return std::get<0>(*this); }

/// set the name
  inline void name(const std::string_view nm)
    { std::get<0>(*this) = nm; }

/// return the value
  inline std::string value(void) const
    { return std::get<1>(*this); }

/// set the value
  inline void value(const std::string_view val)
    { std::get<1>(*this) = val; }

/// return the value of is_possible_mult
  inline bool is_possible_mult(void) const
    { return std::get<2>(*this); }

/// set whether it's a possible mult
  inline void is_possible_mult(const bool ipm)
    { std::get<2>(*this) = ipm; }

/// get whether it's a mult
  inline bool is_mult(void) const
    { return std::get<3>(*this); }

/// set whether it's a mult
  inline void is_mult(const bool im)
    { std::get<3>(*this) = im; }


template<typename Archive>
  void serialize(Archive& ar, const unsigned int version)
    { unsigned int v { version }; /* dummy; for now, version isn't used */
      v = v + 0;

      ar & std::get<0>(*this)
         & std::get<1>(*this)
         & std::get<2>(*this)
         & std::get<3>(*this);
    }
};

/// ostream << received_field
inline std::ostream& operator<<(std::ostream& ost, const received_field& rf)
{ ost << "received_field: " << std::endl
      << "name: " << rf.name() << std::endl
      << "value: " << rf.value() << std::endl
      << "is_possible_mult: " << rf.is_possible_mult() << std::endl
      << "is_mult: " << rf.is_mult();

  return ost;
}

// -----------  QSO  ----------------

/*! \class  QSO
    \brief  The details of a single QSO
*/

class QSO
{
protected:
  
  enum BAND                                         _band;                          ///< band
  std::string                                       _callsign;                      ///< call
  std::string                                       _canonical_prefix;              ///< canonical prefix of country; NOT automatically set when callsign is set
  std::string                                       _comment;                       ///< comment to be carried with QSO (unused)
  std::string                                       _continent;                     ///< continent; NOT automatically set when callsign is set
  std::string                                       _date;                          ///< yyyy-mm-dd
  time_t                                            _epoch_time;                    ///< time in seconds since the UNIX epoch
  std::string                                       _frequency_rx;                  ///< RX frequency in form xxxxx.y (kHz)
  std::string                                       _frequency_tx;                  ///< TX frequency in form xxxxx.y (kHz)
  bool                                              _is_country_mult { false };     ///< is this QSO a country mult?
  bool                                              _is_dupe         { false };     ///< is this QSO a dupe?
  bool                                              _is_prefix_mult  { false };     ///< is this QSO a prefix mult?
  std::vector<std::string>                          _log_line_fields;               ///< separate fields from the log line
  enum MODE                                         _mode;                          ///< mode
  std::string                                       _my_call;                       ///< my call
  unsigned int                                      _number;                        ///< qso number
  unsigned int                                      _points          { 1 };         ///< points for this QSO (unused)
  std::string                                       _prefix;                        ///< prefix, according to the contest's definition
  std::vector<received_field>                       _received_exchange;             ///< names do not include the REXCH-
  bool                                              _is_sap        { true };        ///< whether QSO is in SAP mode
  std::vector<std::pair<std::string, std::string> > _sent_exchange;                 ///< vector<pair<name, value>>; names do not include the TEXCH-
  std::string                                       _utc;                           ///< hh:mm:ss
  
/*! \brief                      Is a particular field that might be received as part of the exchange optional?
    \param  field_name          the name of the field
    \param  fields_from_rules   the possible fields, taken from the rules
    \return                     whether field <i>field_name</i> is optional

    Works regardless of whether <i>field_name</i> includes an initial "received-" string
*/
  bool _is_received_field_optional(const std::string_view field_name, const std::vector<exchange_field>& fields_from_rules) const;

/*! \brief      Process a name/value pair from a drlog log line to insert values in the QSO object
    \param  nv  name and value to be processed
    \return     whether <i>nv</i> was processed

    Does not process fields whose name begins with "received-"
*/
  bool _process_name_value_pair(const std::pair<std::string, std::string>& nv);

/*! \brief      Process a name/value pair from a drlog log line to insert values in the QSO object
    \param  nv  name and value to be processed
    \return     whether <i>nv</i> was processed

    Does not process fields whose name begins with "received-"
*/
  inline bool _process_name_value_pair(const std::string_view nm, const std::string_view val)
    { return _process_name_value_pair( { std::string { nm }, std::string { val } } ); }

/*! \brief               Obtain the epoch time from a date and time in drlog format
    \param  date_str     date string in drlog format
    \param  utc_str      time string in drlog format
    \return              time in seconds since the UNIX epoch
*/
  time_t _to_epoch_time(const std::string_view date_str, const std::string_view utc_str) const;

public:
  
/// constructor; automatically fills in the current date and time
  QSO(void);

/*! \brief              Constructor from a line in the disk log
    \param  context     drlog context
    \param  str         string from log file
    \param  rules       rules for this contest
    \param  statistics  contest statistics

    line in disk log looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=

    <i>statistics</i> might be changed by this function
*/
  QSO(const drlog_context& context, const std::string_view str, const contest_rules& rules, running_statistics& statistics);

  READ_AND_WRITE(band);                   ///< band
  READ_AND_WRITE_STR(callsign);           ///< call
  READ_AND_WRITE_STR(canonical_prefix);   ///< canonical prefix for the country
  READ_AND_WRITE_STR(comment);            ///< comment to be carried with QSO
  READ_AND_WRITE_STR(continent);          ///< continent
  READ_AND_WRITE_STR(date);               ///< yyyy-mm-dd
  READ_AND_WRITE_STR(frequency_rx);       ///< RX frequency in form xxxxx.y (kHz)
  READ_AND_WRITE(mode);                   ///< mode
  READ_AND_WRITE_STR(my_call);            ///< my call
  READ_AND_WRITE(number);                 ///< qso number
  READ_AND_WRITE(points);                 ///< points for this QSO
  READ_AND_WRITE_STR(prefix);             ///< prefix, according to the contest's definition
  READ_AND_WRITE(is_sap);                 ///< whether QSO is in SAP mode
  READ_AND_WRITE(sent_exchange);          ///< vector<pair<name, value>>; names do not include the TEXCH-
  READ_AND_WRITE_STR(utc);                ///< hh:mm:ss

/// return whether the QSO is in CQ mode
  inline bool cq_mode(void) const
    { return !_is_sap; }

/// return whether the QSO is in SAP mode
  inline bool sap_mode(void) const
    { return _is_sap; }

/// get TX frequency as a string
  inline decltype(_frequency_tx) freq(void) const
    { return _frequency_tx; }

/// set TX frequency from a string of the form xxxxx.y
  inline void freq(const std::string_view str)
    { _frequency_tx = str; }
  
/// set TX frequency and band from a string of the form xxxxx.y
  void freq_and_band(const std::string_view str);

  READ_AND_WRITE(epoch_time);           ///< time in seconds since the UNIX epoch

  READ_AND_WRITE(received_exchange);    ///< names do not include the REXCH-
  READ_AND_WRITE(is_country_mult);      ///< is this QSO a country mult?
  READ_AND_WRITE(is_prefix_mult);       ///< is this QSO a prefix mult?

  READ(is_dupe);                        ///< is this QSO a dupe?
  
/// is any exchange field a mult?
  inline bool is_exchange_mult(void) const
    { return ANY_OF(_received_exchange, [] (const auto& field) { return field.is_mult(); }); }

/*! \brief              Set a field to be an exchange mult
    \param  field_name  name of field

    Does nothing if <i>field_name</i> is not a possible mult
*/
  void set_exchange_mult(const std::string_view field_name);

/// synonym for callsign()
  inline std::string call(void) const
    { return callsign(); }

/// simple proxy for emptiness
  inline bool empty(void) const
    { return callsign().empty(); }
    
/// mark as dupe
  inline void dupe(void)
    { _is_dupe = true; }
    
/// unmark as dupe
  inline void undupe(void)
    { _is_dupe = false; }
    
/// return a single date-and-time string
  inline std::string date_and_time(void) const
    { return (_date + "T"s + _utc); }
    
/// is this QSO earlier than another one? 
  inline bool earlier_than(const QSO& qso) const
    { return (_epoch_time < qso.epoch_time()); }
    
/*! \brief                          Re-format according to a Cabrillo template
    \param  cabrillo_qso_template   template for the QSO: line in a Cabrillo file, from configuration file

    Example template:
      CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1
*/
  std::string cabrillo_format(const std::string_view cabrillo_qso_template) const;
  
/// format for writing to disk (in the actual drlog log)
  std::string verbose_format(void) const;  

/*! \brief              Read fields from a line in the disk log
    \param  context     drlog context
    \param  str         string from log file
    \param  rules       rules for this contest
    \param  statistics  contest statistics

    line in disk log looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=
*/
  void populate_from_verbose_format(const drlog_context& context, const std::string_view str, const contest_rules& rules, running_statistics& statistics);

/*! \brief              Read fields from a line in the disk log
    \param  str         line from log file

    line in disk log looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=

    Performs a skeletal setting of values, without using the rules for the contest; used by simulator
*/
  void populate_from_verbose_format(const std::string_view str);

/*! \brief                  Does the QSO match an expression for a received exchange field?
    \param  rule_to_match   boolean rule to attempt to match
    \return                 whether the exchange in the QSO matches <i>rule_to_match</i>

    <i>rule_to_match is</i> from the configuration file, and looks like:
      [IOTA != -----]

    I don't think that this is currently used.
*/
  bool exchange_match(const std::string_view rule_to_match) const;

/*! \brief          Do the values of any of the exchange fields in the QSO match a target string?
    \param  target  target string
    \return         whether any of the exchange fields contain the value <i>target</i>
*/
  inline bool exchange_match_string(const std::string_view target) const
    { return ANY_OF(_received_exchange, [target] (const auto& field) { return (field.value() == target); } ); }

/*! \brief              Return a single field from the received exchange
    \param  field_name  name of field to return
    \return             value of <i>field_name</i>

    Returns the empty string if <i>field_name</i> is not found in the exchange
*/
  std::string received_exchange(const std::string_view field_name) const;

/*! \brief              Is a particular field present in the received exchange?
    \param  field_name  name of field for whose presence to test
    \return             whether <i>field_name</i> is present
*/
  inline bool is_exchange_field_present(const std::string_view field_name) const
    { return !(received_exchange(field_name).empty()); }

/*! \brief              Return a single field from the sent exchange
    \param  field_name  name of field to return
    \return             value of <i>field_name</i>

    Returns the empty string if <i>field_name</i> is not found in the exchange
*/
  std::string sent_exchange(const std::string_view field_name) const;

/*! \brief              Return a single field from the sent exchange
    \param  field_name  name of field to return
    \return             value of <i>field_name</i>

    Synonym for sent_exchange(<i>field_name</i>)
*/
  inline std::string sent_exchange_field(const std::string_view field_name) const
    { return sent_exchange(field_name); }

/*! \brief              Does the sent exchange include a particular field?
    \param  field_name  name of field to test
    \return             whether the sent exchange includes the field <i>field_name</i>
*/
  inline bool sent_exchange_includes(const std::string_view field_name) const
    { return ANY_OF(_sent_exchange, [field_name] (const auto& field) { return (field.first == field_name); }); }

/*! \brief      Obtain string in format suitable for display in the LOG window
    \return     QSO formatted for writing into the LOG window

    Also populates <i>_log_line_fields</i> to match the returned string
*/
  std::string log_line(void);

/*! \brief          Populate from a string (as visible in the log window)
    \param  str     string from visible log window
*/
  void populate_from_log_line(const std::string_view str);

/*! \brief      QSO == QSO
    \param  q   target QSO
    \return     whether the two QSOs are equal, for the purpose of comparison within a deque

    Only "important" members are compared.
*/
  bool operator==(const QSO& q) const;

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned int version)
    { unsigned int v { version };   // dummy; for now, version isn't used
      v = v + 0;

      ar & _band
         & _callsign
         & _canonical_prefix
         & _comment
         & _continent
         & _date
         & _epoch_time
         & _frequency_rx
         & _frequency_tx
         & _is_country_mult
         & _is_dupe
         & _is_prefix_mult
         & _log_line_fields
         & _mode
         & _my_call
         & _number
         & _points
         & _prefix
         & _received_exchange
         & _sent_exchange
         & _utc;
    }
};

/*! \brief          Write a <i>QSO</i> object to an output stream
    \param  ost     output stream
    \param  q       object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const QSO& q);

/*! \brief          Is one QSO earlier than another?
    \param  qso_1   first QSO
    \param  qso_2   second QSO  name of field to test
    \return         whether <i>qso_1</i> is earlier than <i>qso_2</i>
*/
inline bool earlier(const QSO& qso_1, const QSO& qso_2)
  { return (qso_1.earlier_than(qso_2)); }

/*! \brief          Obtain the next name and value from a drlog-format line
    \param  str     a drlog-format line
    \param  posn    character position within line
    \return         the next (<i>i.e.</i>, after <i>posn</i>) name and value separated by an "="

    Correctly handles extraneous spaces in <i>str</i>.
    <i>str</i> looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=

    The value of <i>posn</i> might be changed by this function.
*/
std::pair<std::string, std::string> next_name_value_pair(const std::string_view str, size_t& posn);

#endif    // QSO_H
