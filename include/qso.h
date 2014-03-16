// $Id: qso.h 44 2013-12-13 17:45:49Z  $

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

extern bool QSO_DISPLAY_COUNTRY_MULT;
extern int  QSO_MULT_WIDTH;

// forward declarations
class running_statistics;

/// tuple for encapsulating received-fields
WRAPPER_4_SERIALIZE(received_field, std::string, name, std::string, value, bool, is_possible_mult, bool, is_mult);  // fundamental types are initialized with zero Josuttis, 2nd ed. p. 69
// (We need the is_possible mult field so that things align correctly on the log line)

// -----------  QSO  ----------------

/*!     \class QSO
        \brief The details of a single QSO
*/

class QSO
{
protected:
  
  unsigned int _number;          ///< qso number
  std::string  _date;            ///< yyyy-mm-dd
  std::string  _utc;             ///< hh:mm:ss
  std::string  _callsign;
  enum MODE    _mode;
  enum BAND    _band;
  std::string  _frequency;
  std::string  _comment;
  std::string  _canonical_prefix;  ///< NOT automatically set when callsign is set
  
  time_t       _epoch_time;

  std::string  _my_call;

// contest-specific
  unsigned int _points;        // points for this QSO
  bool         _is_dupe;

  std::string  _prefix;        // may depend on contest

  std::vector<std::pair<std::string, std::string> >           _sent_exchange;                  // vector<name, value>; names do not include the TEXCH-
//  std::map<std::string /* name */, std::string /* value */>   _received_exchange;              // names do not include the REXCH-
//  std::vector<std::pair<std::string /* name */, std::string /* value */> >   _received_exchange;              // names do not include the REXCH-
  std::vector<received_field>   _received_exchange;              // names do not include the REXCH-

  std::vector<std::string>  _log_line_fields;

// mults
  bool _is_country_mult;
  bool _is_prefix_mult;

  typedef enum MODE enum_MODE;
  typedef enum BAND enum_BAND;
  typedef std::vector<std::pair<std::string, std::string> > vector_pair_string_string;
  typedef std::map<std::string, std::string> map_string_string;
  
  const std::pair<std::string, std::string> _next_name_value_pair(const std::string& str, size_t& posn);

  const time_t _to_epoch_time(const std::string& date_str, const std::string& utc_str) const;

public:
  
/// constructor; automatically fills in the current date and time
  QSO(void);

/// destructor
  virtual ~QSO(void)
    { }

  READ_AND_WRITE(unsigned int, number);
  READ_AND_WRITE(std::string, callsign);
  READ_AND_WRITE(MODE, mode);
  READ_AND_WRITE(BAND, band);
  READ_AND_WRITE(std::string, date);
  READ_AND_WRITE(std::string, utc);  

  inline const std::string freq(void) const
    { return _frequency; }

  inline void freq(const std::string& str)
    { _frequency = str; }

  READ_AND_WRITE(std::string, comment);
  READ_AND_WRITE(std::string, canonical_prefix);
  READ_AND_WRITE(unsigned int, points);
  
  READ_AND_WRITE(std::string, prefix);

  READ_AND_WRITE(std::string, my_call);
  
  READ(time_t, epoch_time);

  READ_AND_WRITE(vector_pair_string_string, sent_exchange);

  inline std::vector<received_field> received_exchange(void)
    { return _received_exchange; }

  inline void received_exchange(const std::vector<received_field>& field)
    { _received_exchange = field; }

  READ_AND_WRITE(bool, is_country_mult);
  READ_AND_WRITE(bool, is_prefix_mult);

  READ(bool, is_dupe);
  
/// are any of the exchange fields a mult?
  const bool is_exchange_mult(void) const;

/// synonym for callsign()
  inline const std::string call(void) const
    { return callsign(); }
    
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
  inline const bool earlier_by_number(const QSO& qso) const
//    { return (number() < qso.number()); }
    { return (_epoch_time < qso.epoch_time()); }

  inline const bool earlier_by_clock(const QSO& qso) const
    { return (date_and_time() < qso.date_and_time()); }

  inline const bool earlier(const QSO& qso) const
    { return earlier_by_clock(qso); }
    
/// re-format according to a Cabrillo template
  const std::string cabrillo_format(const std::string& cabrillo_qso_template) const;
  
/// format for writing to disk
  const std::string verbose_format(void) const;  

/// read fields from a line in the disk log
  void populate_from_verbose_format(const std::string& str, const contest_rules& rules, running_statistics& statistics);

/// does the QSO match an expression for a received exchange field?
  const bool exchange_match(const std::string& rule_to_match) const;

/// return a single field from the received exchange
  const std::string received_exchange(const std::string& field_name) const;

/// return a single field from the received exchange
  const std::string sent_exchange(const std::string& field_name) const;

/// return a single field from the received exchange
  inline const std::string sent_exchange_field(const std::string& field_name) const
    { return sent_exchange(field_name); }

// does the sent exchange include a particular field?
  const bool sent_exchange_includes(const std::string& field_name);

  std::string log_line(void);

/// read fields from a log_line
  void populate_from_log_line(const std::string& str);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _number
         & _date
         & _utc
         & _callsign
         & _mode
         & _band
         & _frequency
         & _comment
         & _my_call
         & _points
         & _is_dupe
         & _sent_exchange
         & _received_exchange
         & _canonical_prefix
         & _epoch_time;
    }

};

/// ostream << QSO
std::ostream& operator<<(std::ostream& ost, const QSO& q);

/// is one QSO earlier than another?
inline const bool earlier(const QSO& qso_1, const QSO& qso_2)
  { return (qso_1.earlier(qso_2)); }

#endif    // QSO_H
