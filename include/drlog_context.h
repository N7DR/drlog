// $Id: drlog_context.h 58 2014-04-12 17:23:28Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef DRLOG_CONTEXT_H
#define DRLOG_CONTEXT_H

/*!     \file drlog_context.h

        The basic context for operation of drlog
*/

#include "bands-modes.h"
#include "cty_data.h"
#include "screen.h"

#include <array>
#include <set>
#include <string>

// -----------  drlog_context  ----------------

/*! \class drlog_context
    \brief The variables and constants that comprise the context for operation
*/

const unsigned int MAX_MEMORY_MESSAGES = 12;
const unsigned int CQ_MEMORY_MESSAGES  = 9;        // must not exceed 9 without changes to drlog_context.cpp
const unsigned int EX_MEMORY_MESSAGES  = 9;        // must not exceed 9 without changes to drlog_context.cpp
const unsigned int EX_ALT_MEMORY_MESSAGES  = 9;    // must not exceed 9 without changes to drlog_context.cpp
const unsigned int CQ_CTRL_MEMORY_MESSAGES  = 10;  // change already made in drlog_context.cpp; copy the logic for other memories if necessary

enum country_multiplier_type { COUNTRY_MULT_NONE,
                               COUNTRY_MULT_DXCC,     // DXCC list
                               COUNTRY_MULT_WAEDC     // DARC WAEDC list
                             };

extern pt_mutex _context_mutex;

/// Syntactic sugar for read-only access
#define CONTEXTREAD(y) \
/*! Read-only access to _##y */ \
  inline const decltype(_##y)& y(void) const { SAFELOCK(_context); return _##y; }


// -----------  drlog_context  ----------------

class drlog_context
{
protected:

  std::string                                  _archive_name;                    ///< name of the archive for save/restore information
  std::string                                  _auto_backup;                     ///< directory for auto backup files
  bool                                         _auto_remaining_callsign_mults;   ///< do we auto-generate the remaining callsign mults?
  bool                                         _auto_remaining_country_mults;    ///< do we auto-generate the remaining country mults?

  unsigned int                                 _bandmap_decay_time_local;          ///< time (in minutes) for an entry to age off the bandmap (local entries)
  unsigned int                                 _bandmap_decay_time_cluster;        ///< time (in minutes) for an entry to age off the bandmap (cluster entries)
  unsigned int                                 _bandmap_decay_time_rbn;            ///< time (in minutes) for an entry to age off the bandmap (RBN entries)
  std::vector<int>                             _bandmap_fade_colours;              ///< the colours calls adopt as they fade
  std::vector<std::string>                     _bandmap_filter;                    ///< the strings in the bandmap filter
  int                                          _bandmap_filter_disabled_colour;    ///< background colour when bandmap filter is disabled
  bool                                         _bandmap_filter_enabled;            ///< is the bandmap filter enabled?
  int                                          _bandmap_filter_foreground_colour;  ///< colour of foreground in the bandmap filter
  int                                          _bandmap_filter_hide_colour;        ///< background colour when bandmap filter is in hide mode
  bool                                         _bandmap_filter_show;               ///< is the bandmap filter set to show? (If not, then it's set to hide)
  int                                          _bandmap_filter_show_colour;        ///< background colour when bandmap filter is in show mode
  std::string                                  _bands;                       ///< comma-delimited bands
  std::string                                  _batch_messages_file;         ///< file that contains per-call batch messages

  std::string                                  _cabrillo_filename;   ///< name of Cabrillo log

// Cabrillo records
  std::string                                  _cabrillo_callsign;                 ///< CALLSIGN:
  std::string                                  _cabrillo_address_1;                ///< first ADDRESS: line
  std::string                                  _cabrillo_address_2;                ///< second ADDRESS: line
  std::string                                  _cabrillo_address_3;                ///< third ADDRESS: line
  std::string                                  _cabrillo_address_4;                ///< fourth ADDRESS: line
  std::string                                  _cabrillo_address_city;             ///< ADDRESS-CITY:
  std::string                                  _cabrillo_address_state_province;   ///< ADDRESS-STATE-PROVINCE:
  std::string                                  _cabrillo_address_postalcode;       ///< ADDRESS-POSTALCODE:
  std::string                                  _cabrillo_address_country;          ///< ADDRESS-COUNTRY:
  std::string                                  _cabrillo_category_assisted;        ///< CATEGORY-ASSISTED:
  std::string                                  _cabrillo_category_band;            ///< CATEGORY-BAND:
  std::string                                  _cabrillo_category_mode;            ///< CATEGORY-MODE:
  std::string                                  _cabrillo_category_operator;        ///< CATEGORY-OPERATOR:
  std::string                                  _cabrillo_category_overlay;         ///< CATEGORY-OVERLAY:
  std::string                                  _cabrillo_category_power;           ///< CATEGORY-POWER:
  std::string                                  _cabrillo_category_station;         ///< CATEGORY-STATION:
  std::string                                  _cabrillo_category_time;            ///< CATEGORY-TIME:
  std::string                                  _cabrillo_category_transmitter;     ///< CATEGORY-TRANSMITTER:
  std::string                                  _cabrillo_club;                     ///< CLUB:
  std::string                                  _cabrillo_contest;                  ///< CONTEST:
  std::string                                  _cabrillo_e_mail;                   ///< EMAIL: (sic)
  std::string                                  _cabrillo_location;                 ///< LOCATION:
  std::string                                  _cabrillo_name;                     ///< NAME:
  std::string                                  _cabrillo_operators;                ///< OPERATORS:
  std::string                                  _cabrillo_qso_template;             ///< format for Cabrillo QSOs

  std::string                                  _call_ok_now_message;      ///< message if call was changed
  std::set<std::string>                        _callsign_mults;           ///< mults derived from callsign; e.g., WPXPX
  bool                                         _callsign_mults_per_band;  ///< are callsign mults per-band?
  unsigned int                                 _cluster_port;             ///< port on the cluster server
  std::string                                  _cluster_server;           ///< hostname or IP of cluster server
  std::string                                  _cluster_username;         ///< username to use on the cluster
  std::string                                  _contest_name;             ///< name of the contest
  std::vector<std::pair<std::string, std::string>> _country_exceptions;   ///< calls to be placed in non-default country
//  std::map<std::string, std::set<std::string>> _country_exchange;         ///< allowed exchanges values as a function of country
  enum country_list_type                       _country_list;             ///< DXCC or WAE list?
  std::string                                  _country_mults_filter;     ///< the command from the configuration file
  bool                                         _country_mults_per_band;   ///< are country mults per-band?
  bool                                         _cq_auto_lock;             ///< whether to lock the transmitter in CQ mode
  bool                                         _cq_auto_rit;              ///< whether to enable RIT in CQ mode
  std::array<std::string, CQ_MEMORY_MESSAGES + 1>
                                               _cq_memory;                ///< CQ memories, counted wrt 1
  std::string                                  _cq_menu;                  ///< menu displayed in CQ mode
  std::string                                  _cty_filename;             ///< filename of country file (default = "cty.dat")
  unsigned int                                 _cw_speed;                 ///< Speed in WPM

  std::vector<std::string>                     _do_not_show;           ///< do not show these calls when spotted (MY CALL is automatically not shown)
  std::string                                  _do_not_show_filename;  ///< filename of calls (one per line) not to be shown
  std::string                                  _drmaster_filename;     ///< filename of drmaster file (default = "drmaster")

  std::string                                  _exchange;                  ///< comma-delimited received exchange
  std::string                                  _exchange_cq;               ///< exchange in CQ mode
  std::string                                  _exchange_fields_filename;  ///< file that holds regex templates of exchange fields
  std::string                                  _exchange_mults;            ///< comma-delimited exchange fields that are mults
  bool                                         _exchange_mults_per_band;   ///< are exchange mults per-band?
  std::map<std::string, std::string>           _exchange_per_country;      ///< per-country exchanges; key = prefix-or-call; value = exchange
  std::string                                  _exchange_sap;              ///< exchange in SAP mode
  std::array<std::string, EX_MEMORY_MESSAGES>  _ex_memory;                 ///< exchange memories
  std::string                                  _ex_menu;                   ///< menu displayed during an exchange

  std::map<MODE, unsigned int>                 _guard_band;

  std::string                                  _individual_messages_file;  ///< file that contains per-call individual messages

  std::string                                  _keyer_port;          ///< the device that is to be used as a keyer

  std::string                                  _logfile;             ///< name of the log filename

  unsigned int                                 _match_minimum;       ///< number of characters before SCP or fuzzy match kicks in
  std::array<std::string, MAX_MEMORY_MESSAGES> _memory_messages;     ///< canned messages
  std::string                                  _message_cq_1;        ///< CQ message #1 (generally, a short CQ)
  std::string                                  _message_cq_2;        ///< CQ message #2 (generally, a long CQ)
  std::string                                  _modes;               ///< comma-delimited modes CW, DIGI, SSB
  std::string                                  _my_call;             ///< my call
  std::string                                  _my_continent;        ///< my continent
  unsigned int                                 _my_cq_zone;          ///< my CQ zone
  std::string                                  _my_grid;             ///< grid square identifier
  std::string                                  _my_ip;               ///< my IP address
  unsigned int                                 _my_itu_zone;         ///< my ITU zone
  float                                        _my_latitude;         ///< latitude in degrees (north +ve)
  float                                        _my_longitude;        ///< longitude in degrees (east +ve)

  bool                                         _normalise_rate;      ///< whether to display rates as per-hour
  std::string                                  _not_country_mults;   ///< comma-separated list of countries that are explicitly NOT country mults

  std::vector<std::string>                     _path;                ///< directories to search, in order
  std::map<BAND, std::string>                  _per_band_points;     ///< points structure for each band
  unsigned int                                 _ptt_delay;           ///< PTT delay in milliseconds ( 0 => PTT disabled)
  std::string                                  _p3_snapshot_file;    ///< base name of file for P3 snapshot

  std::string                                  _qsl_message;         ///< confirm at end of QSO
  std::string                                  _quick_qsl_message;   ///< hurried confirm at end of QSO
  bool                                         _qso_multiple_bands;  ///< whether OK to work station on another band
  bool                                         _qso_multiple_modes;  ///< whether OK to work station on another mode
  std::map<std::string, std::set<std::string>> _qthx;                ///< allowed exchanges values as a function of country

  std::vector<unsigned int>                    _rate_periods;                    ///< periods (in minutes) over which rates should be calculated
  unsigned int                                 _rbn_port;                        ///< port number on the RBN server
  std::string                                  _rbn_server;                      ///< hostname or IP of RBN server
  unsigned int                                 _rbn_threshold;                   ///< number of different stations that have to post to RBN before it shows on the bandmap
  std::string                                  _rbn_username;                    ///< username to use on the RBN
  std::set<std::string>                        _remaining_callsign_mults_list;   ///< callsign mults to display
  std::set<std::string>                        _remaining_country_mults_list;    ///< country mults to display
  unsigned int                                 _rig1_baud;                       ///< baud rate for rig
  unsigned int                                 _rig1_data_bits;                  ///< number of data bits for rig
  std::string                                  _rig1_name;                       ///< name of rig
  std::string                                  _rig1_port;                       ///< port to communicate with rig
  unsigned int                                 _rig1_stop_bits;                  ///< number of stop bits for rig
  std::string                                  _rig1_type;                       ///< model name of rig
  std::string                                  _russian_filename;                ///< filename of russian location file (default = "russian-data")

  std::set<BAND>                               _score_bands;         ///< which bands are going to be scored?
  std::string                                  _screen_snapshot_file;    ///< base name of file for screenshot
  std::vector<std::pair<std::string, std::string> > _sent_exchange;  ///< names and values of sent exchange fields
  unsigned int                                 _shift_delta;         ///< how many Hertz to QSY per poll of the shift key
  unsigned int                                 _shift_poll;          ///< how frequently is the shift key polled during an RIT QSY, in milliseconds
  enum BAND                                    _start_band;             ///< on what band do we start?
  enum MODE                                    _start_mode;          ///< on which mode do we start?
  std::map<std::string /* name */,
             std::pair<std::string /* contents */,
                         std::vector<window_information> > > _static_windows;
  bool                                         _sync_keyer;          ///< whether to synchronise the rig keyer speed with the computer

  bool                                         _test;                ///< put rig in TEST mode?

  std::map<std::string, window_information >   _windows;             ///< size and position info for each window
  std::string                                  _worked_mults_colour; ///< colour of worked mults in the mult windows



// cw messages
// we use the KeySymbol as the integer, although other I/O implementations could use something else
  std::map<int, std::string >                  _messages;

  void _process_configuration_file(const std::string& cs);

  public:

  drlog_context(void)
    { }

/// construct from file
  drlog_context( const std::string& filename );

typedef std::array<std::string, CQ_MEMORY_MESSAGES + 1> cq_memory_type;

  SAFEREAD(archive_name, _context);
  SAFEREAD(auto_backup, _context);
  SAFEREAD(auto_remaining_callsign_mults, _context);
  SAFEREAD(auto_remaining_country_mults, _context);

  SAFEREAD(bandmap_decay_time_local, _context);
  SAFEREAD(bandmap_decay_time_cluster, _context);
  SAFEREAD(bandmap_decay_time_rbn, _context);
  SAFEREAD(bands, _context);
  SAFEREAD(bandmap_fade_colours, _context);
  SAFEREAD(bandmap_filter_foreground_colour, _context);
  SAFEREAD(bandmap_filter_hide_colour, _context);
  SAFEREAD(bandmap_filter_show_colour, _context);
  SAFEREAD(bandmap_filter_disabled_colour, _context);
  SAFEREAD(bandmap_filter_enabled, _context);
  SAFEREAD(bandmap_filter_show, _context);
  SAFEREAD(bandmap_filter, _context);

  inline const bool bandmap_filter_hide(void) const
    { return !bandmap_filter_show(); }

  SAFEREAD(batch_messages_file, _context);

  SAFEREAD(cabrillo_address_1, _context);
  SAFEREAD(cabrillo_address_2, _context);
  SAFEREAD(cabrillo_address_3, _context);
  SAFEREAD(cabrillo_address_4, _context);
  SAFEREAD(cabrillo_address_city, _context);
  SAFEREAD(cabrillo_address_state_province, _context);
  SAFEREAD(cabrillo_address_postalcode, _context);
  SAFEREAD(cabrillo_address_country, _context);
  SAFEREAD(cabrillo_callsign, _context);
  SAFEREAD(cabrillo_category_assisted, _context);
  SAFEREAD(cabrillo_category_band, _context);
  SAFEREAD(cabrillo_category_mode, _context);
  SAFEREAD(cabrillo_category_operator, _context);
  SAFEREAD(cabrillo_category_overlay, _context);
  SAFEREAD(cabrillo_category_power, _context);
  SAFEREAD(cabrillo_category_station, _context);
  SAFEREAD(cabrillo_category_time, _context);
  SAFEREAD(cabrillo_category_transmitter, _context);
  SAFEREAD(cabrillo_club, _context);
  SAFEREAD(cabrillo_contest, _context);
  SAFEREAD(cabrillo_e_mail, _context);
  SAFEREAD(cabrillo_filename, _context);
  SAFEREAD(cabrillo_location, _context);
  SAFEREAD(cabrillo_name, _context);
  SAFEREAD(cabrillo_operators, _context);
  SAFEREAD(cabrillo_qso_template, _context);

  SAFEREAD(call_ok_now_message, _context);
  SAFEREAD(callsign_mults, _context);
  SAFEREAD(callsign_mults_per_band, _context);
  SAFEREAD(country_list, _context);
  SAFEREAD(country_mults_per_band, _context);
  SAFEREAD(cq_auto_lock, _context);
  SAFEREAD(cq_auto_rit, _context);
  SAFEREAD(cluster_port, _context);
  SAFEREAD(cluster_server, _context);
  SAFEREAD(cluster_username, _context);
  SAFEREAD(contest_name, _context);
  SAFEREAD(country_mults_filter, _context);
  SAFEREAD(cq_memory, _context);
  SAFEREAD(cq_menu, _context);
  SAFEREAD(cty_filename, _context);
  SAFEREAD(cw_speed, _context);

  SAFEREAD(do_not_show, _context);
  SAFEREAD(do_not_show_filename, _context);
  SAFEREAD(drmaster_filename, _context);

  SAFEREAD(exchange, _context);
  SAFEREAD(exchange_mults, _context);
  SAFEREAD(exchange_mults_per_band, _context);
  SAFEREAD(exchange_cq, _context);
  SAFEREAD(exchange_sap, _context);

  const std::map<std::string, std::string> exchange_per_country(void) const
    { SAFELOCK(_context);

      return _exchange_per_country;
    }

//  CONTEXTREAD(unsigned int, guard_band);
  const unsigned int guard_band(const MODE m)
  { SAFELOCK(_context);

    const auto cit = _guard_band.find(m);

    if (cit == _guard_band.end())
      return 1000;

    return cit->second;
  }

  SAFEREAD(individual_messages_file, _context);

  SAFEREAD(keyer_port, _context);

  SAFEREAD(logfile, _context);

  SAFEREAD(match_minimum, _context);
  SAFEREAD(modes, _context);
  SAFEREAD(my_call, _context);
  SAFEREAD(my_continent, _context);
  SAFEREAD(my_cq_zone, _context);
  SAFEREAD(my_ip, _context);
  SAFEREAD(my_itu_zone, _context);
  SAFEREAD(my_latitude, _context);
  SAFEREAD(my_longitude, _context);

  SAFEREAD(normalise_rate, _context);
  SAFEREAD(not_country_mults, _context);

  SAFEREAD(path, _context);
  SAFEREAD(ptt_delay, _context);
  SAFEREAD(p3_snapshot_file, _context);

  SAFEREAD(qthx, _context);         ///< allowed exchanges values as a function of country

  SAFEREAD(rbn_port, _context);
  SAFEREAD(rbn_server, _context);
  SAFEREAD(rbn_threshold, _context);
  SAFEREAD(rbn_username, _context);
  SAFEREAD(russian_filename, _context);

  SAFEREAD(per_band_points, _context);

  const std::string points(const BAND b) const
    { SAFELOCK(_context);

      if (_per_band_points.find(b) != _per_band_points.end())
        return _per_band_points.at(b);
      else
        return std::string();
    }

  SAFEREAD(quick_qsl_message, _context);
  SAFEREAD(qsl_message, _context);
  SAFEREAD(qso_multiple_bands, _context);
  SAFEREAD(qso_multiple_modes, _context);

  SAFEREAD(rate_periods, _context);
  SAFEREAD(remaining_callsign_mults_list, _context);
  SAFEREAD(remaining_country_mults_list, _context);
  SAFEREAD(rig1_baud, _context);
  SAFEREAD(rig1_data_bits, _context);
  SAFEREAD(rig1_port, _context);
  SAFEREAD(rig1_stop_bits, _context);
  SAFEREAD(rig1_type, _context);

//  typedef std::map<std::string /* name */, std::pair<std::string /* contents */, std::vector<window_information> > > STATIC_WINDOWS;

  SAFEREAD(score_bands, _context);
  SAFEREAD(screen_snapshot_file, _context);
  SAFEREAD(sent_exchange, _context);
  SAFEREAD(shift_delta, _context);
  SAFEREAD(shift_poll, _context);
  SAFEREAD(start_band, _context);
  SAFEREAD(start_mode, _context);
  SAFEREAD(static_windows, _context);
  SAFEREAD(sync_keyer, _context);

  SAFEREAD(test, _context);

  SAFEREAD(worked_mults_colour, _context);

//  inline const std::vector<std::pair<std::string, std::string> >  sent_exchange(void) const
//    { return _sent_exchange; }


/// location and size of a particular window
  const window_information window_info(const std::string& name) const;

// cw messages
  const std::string message(const int symbol);    // we use the KeySymbol as the integer, although other I/O implementations could use something else

// CQ messages
  SAFEREAD(message_cq_1, _context);
  SAFEREAD(message_cq_2, _context);

/// vector of the names of bands (e.g., "160", "80", etc.)
  inline const std::vector<std::string> band_names(void) const
    { SAFELOCK(_context);
      return split_string(_bands, ","); }

/// vector of the names of modes (e.g., "CW", "SSB", etc.)
  inline const std::vector<std::string> mode_names(void) const
    { SAFELOCK(_context);
      return split_string(_modes, ","); }

/// how many bands are used in this contest?
  inline const unsigned int n_bands(void) const
    { SAFELOCK(_context);
      return band_names().size(); }

/// how many modes are used in this contest?
  inline const unsigned int n_modes(void) const
    { SAFELOCK(_context);
      return mode_names().size(); }

/// a CQ memory
  inline const std::string cq_memory(const unsigned int n) const
    { SAFELOCK(_context);
      return (n < _cq_memory.size() ? _cq_memory[n] : std::string()); }

 // inline const std::map<int, std::string >  messages(void)
 //   { return _messages; }
  SAFEREAD(messages, _context);

  const std::vector<std::string> window_name_contains(const std::string& substr) const;

};

#endif    // DRLOG_CONTEXT_H


