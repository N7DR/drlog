// $Id: qso.h 105 2015-06-01 19:33:27Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef QSO_H
#define QSO_H

/*!     \file qso.h

        Classes and functions related to QSO information
*/

#include "drlog_context.h"
#include "macros.h"
#include "rules.h"

#include <iostream>
#include <string>
#include <utility>

extern bool QSO_DISPLAY_COUNTRY_MULT;   ///< whether country mults are written on the log line
extern int  QSO_MULT_WIDTH;             ///< width of mult fields on log line

// forward declarations
class running_statistics;

// fundamental types are initialized with zero Josuttis, 2nd ed. p. 69
// (We need the is_possible mult field so that things align correctly on the log line)
WRAPPER_4_SERIALIZE(received_field, std::string, name, std::string, value, bool, is_possible_mult, bool, is_mult);  ///< class to encapsulate received fields

// -----------  QSO  ----------------

/*!     \class QSO
        \brief The details of a single QSO
*/

class QSO
{
protected:
  
  enum BAND                                         _band;              ///< band
  std::string                                       _callsign;          ///< call
  std::string                                       _canonical_prefix;  ///< NOT automatically set when callsign is set
  std::string                                       _comment;           ///< comment to be carried with QSO (unused)
  std::string                                       _continent;         ///< NOT automatically set when callsign is set
  std::string                                       _date;              ///< yyyy-mm-dd
  time_t                                            _epoch_time;        ///< time in seconds since the UNIX epoch
//  std::string                                       _frequency;         ///< frequency in form xxxxx.y (kHz)
  std::string                                       _frequency_tx;      ///< TX frequency in form xxxxx.y (kHz)
  std::string                                       _frequency_rx;      ///< RX frequency in form xxxxx.y (kHz)
  bool                                              _is_country_mult;   ///< is this QSO a country mult?
  bool                                              _is_prefix_mult;    ///< is this QSO a prefix mult?
  bool                                              _is_dupe;           ///< is this QSO a dupe?
  std::vector<std::string>                          _log_line_fields;   ///< separate fields from the log line
  enum MODE                                         _mode;              ///< mode
  std::string                                       _my_call;           ///< my call
  unsigned int                                      _number;            ///< qso number
  unsigned int                                      _points;            ///< points for this QSO (unused)
  std::string                                       _prefix;            ///< prefix, according to the contest's definition
  std::vector<received_field>                       _received_exchange; ///< names do not include the REXCH-
  std::vector<std::pair<std::string, std::string> > _sent_exchange;     ///< vector<pair<name, value>>; names do not include the TEXCH-
  std::string                                       _utc;               ///< hh:mm:ss
  
/*! \brief          Obtain the next name and value from a drlog-format line
    \param  str     a drlog-format line
    \param  posn    character position within line
    \return         The next (<i>i.e.</i>, after <i>posn</i>) name and value separated by an "="

    Correctly handles extraneous spaces in <i>str</i>.
    <i>str</i> looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=
*/
  const std::pair<std::string, std::string> _next_name_value_pair(const std::string& str, size_t& posn);

/*! \brief               Obtain the epoch time from a date and time in drlog format
    \param  date_str     date string in drlog format
    \param  utc_str      time string in drlog format
    \return              time in seconds since the UNIX epoch
*/
  const time_t _to_epoch_time(const std::string& date_str, const std::string& utc_str) const;

public:
  
/// constructor; automatically fills in the current date and time
  QSO(void);

/// destructor
  virtual ~QSO(void)
    { }

  READ_AND_WRITE(band);              ///< band
  READ_AND_WRITE(callsign);          ///< call
  READ_AND_WRITE(date);              ///< yyyy-mm-dd
  READ_AND_WRITE(mode);              ///< mode
  READ_AND_WRITE(number);            ///< qso number
  READ_AND_WRITE(utc);               ///< hh:mm:ss

/// get TX frequency as a string
  inline const decltype(_frequency_tx) freq(void) const
    { return _frequency_tx; }

/// set TX frequency from a string of the form xxxxx.y
  inline void freq(const decltype(_frequency_tx)& str)
    { _frequency_tx = str; }

  READ_AND_WRITE(frequency_rx);

  READ_AND_WRITE(comment);           ///< comment to be carried with QSO
  READ_AND_WRITE(canonical_prefix);  ///< canonical prefix for the country
  READ_AND_WRITE(continent);         ///< continent
  READ_AND_WRITE(points);            ///< points for this QSO
  
  READ_AND_WRITE(prefix);            ///< prefix, according to the contest's definition
  READ_AND_WRITE(my_call);           ///< my call
  
  READ(epoch_time);                  ///< time in seconds since the UNIX epoch

  READ_AND_WRITE(sent_exchange);     ///< vector<pair<name, value>>; names do not include the TEXCH-
  READ_AND_WRITE(received_exchange); ///< names do not include the REXCH-
  READ_AND_WRITE(is_country_mult);   ///< is this QSO a country mult?
  READ_AND_WRITE(is_prefix_mult);    ///< is this QSO a prefix mult?

  READ(is_dupe);                     ///< is this QSO a dupe?
  
/// is any exchange field a mult?
  const bool is_exchange_mult(void) const;

/// synonym for callsign()
  inline const std::string call(void) const
    { return callsign(); }

/// simple proxy for emptiness
  inline const bool empty(void) const
    { return callsign().empty(); }
    
/// mark as dupe
  inline void dupe(void)
    { _is_dupe = true; }
    
/// unmark as dupe
  inline void undupe(void)
    { _is_dupe = false; }
    
/// return a single date-and-time string
  inline const std::string date_and_time(void) const
    { return _date + "T" + _utc; }
    
/// is this QSO earlier than another one? 
  inline const bool earlier_than(const QSO& qso) const
    { return (_epoch_time < qso.epoch_time()); }
    
/*! \brief                          Re-format according to a Cabrillo template
    \param  cabrillo_qso_template   template for the QSO: line in a Cabrillo file, from configuration file

    Example template:
      CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1
*/
  const std::string cabrillo_format(const std::string& cabrillo_qso_template) const;
  
/// format for writing to disk (in the actual drlog log)
  const std::string verbose_format(void) const;  

/*! \brief              Read fields from a line in the disk log
    \param  context     drlog context
    \param  str         string from log file
    \param  rules       rules for this contest
    \param  statistics  contest statistics

    line in disk log looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=
*/
  void populate_from_verbose_format(const drlog_context& context, const std::string& str, const contest_rules& rules, running_statistics& statistics);

/*! \brief                  Does the QSO match an expression for a received exchange field?
    \param  rule_to_match   boolean rule to attempt to match
    \return                 whether the exchange in the QSO matches <i>rule_to_match</i>

    <i>rule_to_match is from the configuration file, and looks like:
      [IOTA != -----]
*/
  const bool exchange_match(const std::string& rule_to_match) const;

/*! \brief          Do any of the exchange fields the QSO match a target string?
    \param  target  target string
    \return         whether any of the exchange fields contain the value <i>target</i>
*/
  const bool exchange_match_string(const std::string& target) const;

/*! \brief              Return a single field from the received exchange
    \param  field_name  name of field to return
    \return             value of <i>field_name</i>

    Returns the empty string if <i>field_name</i> is not found in the exchange
*/
  const std::string received_exchange(const std::string& field_name) const;

/*! \brief              Return a single field from the sent exchange
    \param  field_name  name of field to return
    \return             value of <i>field_name</i>

    Returns the empty string if <i>field_name</i> is not found in the exchange
*/
  const std::string sent_exchange(const std::string& field_name) const;

/*! \brief              Return a single field from the sent exchange
    \param  field_name  name of field to return
    \return             value of <i>field_name</i>

    Synonym for sent_exchange(<i>field_name</i>)
*/
  inline const std::string sent_exchange_field(const std::string& field_name) const
    { return sent_exchange(field_name); }

/*! \brief              Does the sent exchange include a particular field?
    \param  field_name  name of field to test
    \return             whether the sent exchange includes the field <i>field_name</i>
*/
  const bool sent_exchange_includes(const std::string& field_name);

/// convert to a string suitable for display in the log window
  const std::string log_line(void);

/*! \brief          Populate from a string (as visible in the log window)
    \param  str     string from visible log window
*/
  void populate_from_log_line(const std::string& str);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _band
         & _callsign
         & _canonical_prefix
         & _comment
         & _continent
         & _date
         & _epoch_time
//         & _frequency
         & _frequency_tx
         & _frequency_rx
         & _is_dupe
         & _mode
         & _my_call
         & _number
         & _points
         & _received_exchange
         & _sent_exchange
         & _utc;
    }
};

/// ostream << QSO
std::ostream& operator<<(std::ostream& ost, const QSO& q);

/// is one QSO earlier than another?
inline const bool earlier(const QSO& qso_1, const QSO& qso_2)
  { return (qso_1.earlier_than(qso_2)); }

#endif    // QSO_H
