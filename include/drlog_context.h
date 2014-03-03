// $Id: drlog_context.h 52 2014-03-01 16:17:18Z  $

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
#define CONTEXTREAD(x, y) \
/*! Read-only access to _##y */ \
  inline const x& y(void) const { SAFELOCK(_context); return _##y; }


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
  bool                                         _exchange_mults_per_band;    ///< are exchange mults per-band?
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

  std::string                                  _qsl_message;         ///< confirm at end of QSO
  std::string                                  _quick_qsl_message;   ///< hurried confirm at end of QSO
  bool                                         _qso_multiple_bands;  ///< whether OK to work station on another band
  bool                                         _qso_multiple_modes;  ///< whether OK to work station on another mode
  std::map<std::string, std::set<std::string>> _qthx;         ///< allowed exchanges values as a function of country

  std::vector<unsigned int>                    _rate_periods;                    ///< periods (in minutes) over which rates should be calculated
  std::string                                  _rbn_server;                      ///< hostname or IP of RBN server
  std::string                                  _rbn_username;                    ///< username to use on the RBN
  unsigned int                                 _rbn_port;                        ///< port number on the RBN server
  std::set<std::string>                        _remaining_callsign_mults_list;   ///< callsign mults to display
  std::set<std::string>                        _remaining_country_mults_list;    ///< country mults to display
  unsigned int                                 _rig1_baud;                       ///< baud rate for rig
  unsigned int                                 _rig1_data_bits;                  ///< number of data bits for rig
  std::string                                  _rig1_name;                       ///< name of rig
  std::string                                  _rig1_port;                       ///< port to communicate with rig
  unsigned int                                 _rig1_stop_bits;                  ///< number of stop bits for rig
  std::string                                  _rig1_type;                       ///< model name of rig

  std::set<BAND>                               _score_bands;         ///< which bands are going to be scored?
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

  CONTEXTREAD(std::string, archive_name);
  CONTEXTREAD(std::string, auto_backup);
  CONTEXTREAD(bool, auto_remaining_callsign_mults);
  CONTEXTREAD(bool, auto_remaining_country_mults);

  CONTEXTREAD(unsigned int, bandmap_decay_time_local);
  CONTEXTREAD(unsigned int, bandmap_decay_time_cluster);
  CONTEXTREAD(unsigned int, bandmap_decay_time_rbn);
  CONTEXTREAD(std::string, bands);
  CONTEXTREAD(std::vector<int>, bandmap_fade_colours);
  CONTEXTREAD(int, bandmap_filter_foreground_colour);
  CONTEXTREAD(int, bandmap_filter_hide_colour);
  CONTEXTREAD(int, bandmap_filter_show_colour);
  CONTEXTREAD(int, bandmap_filter_disabled_colour);
  CONTEXTREAD(bool, bandmap_filter_enabled);
  CONTEXTREAD(bool, bandmap_filter_show);
  CONTEXTREAD(std::vector<std::string>, bandmap_filter);

  inline const bool bandmap_filter_hide(void) const
    { return !bandmap_filter_show(); }

  CONTEXTREAD(std::string, batch_messages_file);

  CONTEXTREAD(std::string, cabrillo_address_1);
  CONTEXTREAD(std::string, cabrillo_address_2);
  CONTEXTREAD(std::string, cabrillo_address_3);
  CONTEXTREAD(std::string, cabrillo_address_4);
  CONTEXTREAD(std::string, cabrillo_address_city);
  CONTEXTREAD(std::string, cabrillo_address_state_province);
  CONTEXTREAD(std::string, cabrillo_address_postalcode);
  CONTEXTREAD(std::string, cabrillo_address_country);
  CONTEXTREAD(std::string, cabrillo_callsign);
  CONTEXTREAD(std::string, cabrillo_category_assisted);
  CONTEXTREAD(std::string, cabrillo_category_band);
  CONTEXTREAD(std::string, cabrillo_category_mode);
  CONTEXTREAD(std::string, cabrillo_category_operator);
  CONTEXTREAD(std::string, cabrillo_category_overlay);
  CONTEXTREAD(std::string, cabrillo_category_power);
  CONTEXTREAD(std::string, cabrillo_category_station);
  CONTEXTREAD(std::string, cabrillo_category_time);
  CONTEXTREAD(std::string, cabrillo_category_transmitter);
  CONTEXTREAD(std::string, cabrillo_club);
  CONTEXTREAD(std::string, cabrillo_contest);
  CONTEXTREAD(std::string, cabrillo_e_mail);
  CONTEXTREAD(std::string, cabrillo_filename);
  CONTEXTREAD(std::string, cabrillo_location);
  CONTEXTREAD(std::string, cabrillo_name);
  CONTEXTREAD(std::string, cabrillo_operators);
  CONTEXTREAD(std::string, cabrillo_qso_template);

  CONTEXTREAD(std::string, call_ok_now_message);
  CONTEXTREAD(std::set<std::string>, callsign_mults);
  CONTEXTREAD(bool, callsign_mults_per_band);
  CONTEXTREAD(enum country_list_type, country_list);
  CONTEXTREAD(bool, country_mults_per_band);
  CONTEXTREAD(bool, cq_auto_lock);
  CONTEXTREAD(bool, cq_auto_rit);
  CONTEXTREAD(unsigned int, cluster_port);
  CONTEXTREAD(std::string, cluster_server);
  CONTEXTREAD(std::string, cluster_username);
  CONTEXTREAD(std::string, contest_name);
  CONTEXTREAD(std::string, country_mults_filter);
  CONTEXTREAD(cq_memory_type, cq_memory);
  CONTEXTREAD(std::string, cq_menu);
  CONTEXTREAD(std::string, cty_filename);
  CONTEXTREAD(unsigned int, cw_speed);

  CONTEXTREAD(std::vector<std::string>, do_not_show);
  CONTEXTREAD(std::string, do_not_show_filename);
  CONTEXTREAD(std::string, drmaster_filename);

  CONTEXTREAD(std::string, exchange);
  CONTEXTREAD(std::string, exchange_mults);
  CONTEXTREAD(bool, exchange_mults_per_band);

  CONTEXTREAD(std::string, exchange_cq);
  CONTEXTREAD(std::string, exchange_sap);

//  CONTEXTREAD(unsigned int, guard_band);
  const unsigned int guard_band(const MODE m)
  { SAFELOCK(_context);

    const auto cit = _guard_band.find(m);

    if (cit == _guard_band.end())
      return 1000;

    return cit->second;
  }

  CONTEXTREAD(std::string, individual_messages_file);

  CONTEXTREAD(std::string, keyer_port);

  CONTEXTREAD(std::string, logfile);

  CONTEXTREAD(unsigned int, match_minimum);
  CONTEXTREAD(std::string,  modes);
  CONTEXTREAD(std::string,  my_call);
  CONTEXTREAD(std::string,  my_continent);
  CONTEXTREAD(unsigned int, my_cq_zone);
  CONTEXTREAD(std::string,  my_ip);
  CONTEXTREAD(unsigned int, my_itu_zone);
  CONTEXTREAD(float,        my_latitude);
  CONTEXTREAD(float,        my_longitude);

  CONTEXTREAD(bool,        normalise_rate);
  CONTEXTREAD(std::string, not_country_mults);

  CONTEXTREAD(std::vector<std::string>, path);
  CONTEXTREAD(unsigned int,             ptt_delay);

  typedef std::map<std::string, std::set<std::string>> qthx_type;
  CONTEXTREAD(qthx_type, qthx);         ///< allowed exchanges values as a function of country

  CONTEXTREAD(unsigned int, rbn_port);
  CONTEXTREAD(std::string, rbn_server);
  CONTEXTREAD(std::string, rbn_username);

  typedef std::map<BAND, std::string> MS;
  CONTEXTREAD(MS, per_band_points);

  const std::string points(const BAND b) const
    { if (_per_band_points.find(b) != _per_band_points.end())
        return _per_band_points.at(b);
      else
        return std::string();
    }

  CONTEXTREAD(std::string, quick_qsl_message);   ///< confirm at end of QSO
  CONTEXTREAD(std::string, qsl_message);         ///< confirm at end of QSO
  CONTEXTREAD(bool, qso_multiple_bands);
  CONTEXTREAD(bool, qso_multiple_modes);

  CONTEXTREAD(std::vector<unsigned int>, rate_periods);
  CONTEXTREAD(std::set<std::string>,     remaining_callsign_mults_list);
  CONTEXTREAD(std::set<std::string>,     remaining_country_mults_list);
  CONTEXTREAD(unsigned int,              rig1_baud);
  CONTEXTREAD(unsigned int,              rig1_data_bits);
  CONTEXTREAD(std::string,               rig1_port);
  CONTEXTREAD(unsigned int,              rig1_stop_bits);
  CONTEXTREAD(std::string,               rig1_type);

  typedef std::map<std::string /* name */, std::pair<std::string /* contents */, std::vector<window_information> > > STATIC_WINDOWS;

  CONTEXTREAD(std::set<BAND>, score_bands);
  CONTEXTREAD(unsigned int,   shift_delta);         ///< how many Hertz to QSY per poll of the shift key
  CONTEXTREAD(unsigned int,   shift_poll);          ///< how frequently is the shift key polled during an RIT QSY, in milliseconds
  CONTEXTREAD(BAND,           start_band);
  CONTEXTREAD(MODE,           start_mode);
  CONTEXTREAD(STATIC_WINDOWS, static_windows);
  CONTEXTREAD(bool,           sync_keyer);

//  std::map<std::string /* name */, std::pair<std::string /* contents */, std::vector<window_information> > > _static_windows;

  CONTEXTREAD(bool,                  test);

  CONTEXTREAD(std::string, worked_mults_colour);

  inline const std::vector<std::pair<std::string, std::string> >  sent_exchange(void) const
    { return _sent_exchange; }


/// location and size of a particular window
  const window_information window_info(const std::string& name) const;

// cw messages
  const std::string message(const int symbol);    // we use the KeySymbol as the integer, although other I/O implementations could use something else

// CQ messages
  CONTEXTREAD(std::string, message_cq_1);
  CONTEXTREAD(std::string, message_cq_2);

/// vector of the names of bands (e.g., "160", "80", etc.)
  inline const std::vector<std::string> band_names(void) const
    { return split_string(_bands, ","); }

/// vector of the names of modes (e.g., "CW", "SSB", etc.)
  inline const std::vector<std::string> mode_names(void) const
    { return split_string(_modes, ","); }

/// how many bands are used in this contest?
  inline const unsigned int n_bands(void) const
    { return band_names().size(); }

/// how many modes are used in this contest?
  inline const unsigned int n_modes(void) const
    { return mode_names().size(); }

/// a CQ memory
  inline const std::string cq_memory(const unsigned int n) const
    { return (n < _cq_memory.size() ? _cq_memory[n] : std::string()); }

  inline const std::map<int, std::string >  messages(void)
    { return _messages; }

  const std::vector<std::string> window_name_contains(const std::string& substr) const;

};

#endif    // DRLOG_CONTEXT_H


