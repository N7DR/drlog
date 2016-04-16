// $Id: drlog_context.h 128 2016-04-16 15:47:23Z  $

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

enum country_multiplier_type { COUNTRY_MULT_NONE,       ///< no country multipliers
                               COUNTRY_MULT_DXCC,       ///< use DXCC list
                               COUNTRY_MULT_WAEDC       ///< use DARC WAEDC list
                             };

extern pt_mutex _context_mutex;             ///< mutex for the drlog context

/// Syntactic sugar for read-only access
#define CONTEXTREAD(y)          \
  inline const decltype(_##y)& y(void) const { SAFELOCK(_context); return _##y; }  ///< Read-only access to _##y

// -----------  drlog_context  ----------------

class drlog_context
{
protected:

  int                                          _accept_colour;                              ///< colour for calls that have been worked, but are not dupes
  std::string                                  _alternative_exchange_cq;                    ///< alternative exchange in CQ mode
  std::string                                  _alternative_exchange_sap;                   ///< alternative exchange in SAP mode
  std::string                                  _alternative_qsl_message;                    ///< alternative confirmation at end of QSO
  std::string                                  _archive_name;                               ///< name of the archive for save/restore information
  std::string                                  _auto_backup;                                ///< directory for auto backup files
  bool                                         _auto_remaining_callsign_mults;              ///< do we auto-generate the remaining callsign mults?
  bool                                         _auto_remaining_country_mults;               ///< do we auto-generate the remaining country mults?
  std::set<std::string>                        _auto_remaining_exchange_mults;              ///< the exchange mults for which we auto-generate the values
  unsigned int                                 _auto_remaining_country_mults_threshold;     ///< number of times a canonical prefix must be seen before it becomes known
  bool                                         _auto_screenshot;                            ///< do we create a screenshot every hour?

  unsigned int                                 _bandmap_decay_time_local;           ///< time (in minutes) for an entry to age off the bandmap (local entries)
  unsigned int                                 _bandmap_decay_time_cluster;         ///< time (in minutes) for an entry to age off the bandmap (cluster entries)
  unsigned int                                 _bandmap_decay_time_rbn;             ///< time (in minutes) for an entry to age off the bandmap (RBN entries)
  std::vector<int>                             _bandmap_fade_colours;               ///< the colours calls adopt as they fade
  std::vector<std::string>                     _bandmap_filter;                     ///< the strings in the bandmap filter
  int                                          _bandmap_filter_disabled_colour;     ///< background colour when bandmap filter is disabled
  bool                                         _bandmap_filter_enabled;             ///< is the bandmap filter enabled?
  int                                          _bandmap_filter_foreground_colour;   ///< colour of foreground in the bandmap filter
  int                                          _bandmap_filter_hide_colour;         ///< background colour when bandmap filter is in hide mode
  bool                                         _bandmap_filter_show;                ///< is the bandmap filter set to show? (If not, then it's set to hide)
  int                                          _bandmap_filter_show_colour;         ///< background colour when bandmap filter is in show mode
  int                                          _bandmap_recent_colour;              ///< colour for bandmap entries that are less than two minutes old
  std::string                                  _bands;                              ///< comma-delimited bands
  std::string                                  _batch_messages_file;                ///< file that contains per-call batch messages

  std::string                                  _cabrillo_eol;                       ///< EOL used in the cabrillo file; one of: "LF", "CR" or "CRLF"
  std::string                                  _cabrillo_filename;                  ///< name of Cabrillo log
  bool                                         _cabrillo_include_score;             ///< is the CLAIMED-SCORE line included in the Cabrillo file?

// Cabrillo records
  std::string                                  _cabrillo_address_1;                 ///< first ADDRESS: line
  std::string                                  _cabrillo_address_2;                 ///< second ADDRESS: line
  std::string                                  _cabrillo_address_3;                 ///< third ADDRESS: line
  std::string                                  _cabrillo_address_4;                 ///< fourth ADDRESS: line
  std::string                                  _cabrillo_address_city;              ///< ADDRESS-CITY:
  std::string                                  _cabrillo_address_state_province;    ///< ADDRESS-STATE-PROVINCE:
  std::string                                  _cabrillo_address_postalcode;        ///< ADDRESS-POSTALCODE:
  std::string                                  _cabrillo_address_country;           ///< ADDRESS-COUNTRY:
  std::string                                  _cabrillo_callsign;                  ///< CALLSIGN:
  std::string                                  _cabrillo_category_assisted;         ///< CATEGORY-ASSISTED:
  std::string                                  _cabrillo_category_band;             ///< CATEGORY-BAND:
  std::string                                  _cabrillo_category_mode;             ///< CATEGORY-MODE:
  std::string                                  _cabrillo_category_operator;         ///< CATEGORY-OPERATOR:
  std::string                                  _cabrillo_category_overlay;          ///< CATEGORY-OVERLAY:
  std::string                                  _cabrillo_category_power;            ///< CATEGORY-POWER:
  std::string                                  _cabrillo_category_station;          ///< CATEGORY-STATION:
  std::string                                  _cabrillo_category_time;             ///< CATEGORY-TIME:
  std::string                                  _cabrillo_category_transmitter;      ///< CATEGORY-TRANSMITTER:
  std::string                                  _cabrillo_certificate;               ///< CERTIFICATE:
  std::string                                  _cabrillo_club;                      ///< CLUB:
  std::string                                  _cabrillo_contest;                   ///< CONTEST:
  std::string                                  _cabrillo_e_mail;                    ///< EMAIL: (sic)
  std::string                                  _cabrillo_location;                  ///< LOCATION:
  std::string                                  _cabrillo_name;                      ///< NAME:
  std::string                                  _cabrillo_operators;                 ///< OPERATORS:
  std::string                                  _cabrillo_qso_template;              ///< format for Cabrillo QSOs

  std::string                                  _call_ok_now_message;        ///< message if call was changed
  std::set<std::string>                        _callsign_mults;             ///< mults derived from callsign; e.g., WPXPX
  bool                                         _callsign_mults_per_band;    ///< are callsign mults per-band?
  bool                                         _callsign_mults_per_mode;    ///< are callsign mults per-mode?
  unsigned int                                 _cluster_port;               ///< port on the cluster server
  std::string                                  _cluster_server;             ///< hostname or IP of cluster server
  std::string                                  _cluster_username;           ///< username to use on the cluster
  std::string                                  _contest_name;               ///< name of the contest
  enum country_list_type                       _country_list;               ///< DXCC or WAE list?
  std::string                                  _country_mults_filter;       ///< the command from the configuration file
  bool                                         _country_mults_per_band;     ///< are country mults per-band?
  bool                                         _country_mults_per_mode;     ///< are country mults per-mode?
  bool                                         _cq_auto_lock;               ///< whether to lock the transmitter in CQ mode
  bool                                         _cq_auto_rit;                ///< whether to enable RIT in CQ mode
  std::string                                  _cty_filename;               ///< filename of country file (default = "cty.dat")
  unsigned int                                 _cw_speed;                   ///< speed in WPM

  std::string                                  _decimal_point;                  ///< character to use as decimal point
  bool                                         _display_communication_errors;   ///< whether to display errors communicating with rig
  std::vector<std::string>                     _do_not_show;                    ///< do not show these calls when spotted (MY CALL is automatically not shown)
  std::string                                  _do_not_show_filename;           ///< filename of calls (one per line) not to be shown
  std::string                                  _drmaster_filename;              ///< filename of drmaster file (default = "drmaster")

  std::string                                  _exchange;                   ///< comma-delimited received exchange
  std::string                                  _exchange_cq;                ///< exchange in CQ mode
  std::string                                  _exchange_fields_filename;   ///< file that holds regex templates of exchange fields
  std::string                                  _exchange_mults;             ///< comma-delimited exchange fields that are mults
  bool                                         _exchange_mults_per_band;    ///< are exchange mults per-band?
  bool                                         _exchange_mults_per_mode;    ///< are exchange mults per-mode?
  std::map<std::string, std::string>           _exchange_per_country;       ///< per-country exchanges; key = prefix-or-call; value = exchange
  std::string                                  _exchange_sap;               ///< exchange in SAP mode

  unsigned int                                 _fast_cq_bandwidth;          ///< fast CW bandwidth in CQ mode, in Hz
  unsigned int                                 _fast_sap_bandwidth;         ///< fast CW bandwidth in SAP mode, in Hz

  std::map<MODE, unsigned int>                 _guard_band;                 ///< guard band, in Hz

  std::string                                  _individual_messages_file;   ///< name of file that contains per-call individual messages

  std::string                                  _keyer_port;                 ///< the device that is to be used as a keyer

  std::string                                  _logfile;                    ///< name of the log file

  std::map<MODE, std::vector<std::pair<frequency, frequency>>> _mark_frequencies;   ///< frequency ranges to be marked on-screen
  unsigned int                                 _match_minimum;                      ///< number of characters before SCP or fuzzy match kicks in

// we use the KeySymbol as the integer, although other I/O implementations could use something else
  std::map<int, std::string >                  _messages;                           ///< CW messages

  std::string                                  _message_cq_1;                       ///< CQ message #1 (generally, a short CQ)
  std::string                                  _message_cq_2;                       ///< CQ message #2 (generally, a long CQ)
  std::string                                  _modes;                              ///< comma-delimited modes CW, SSB
  std::map<BAND, frequency>                    _mode_break_points;                  ///< override default mode break points
  std::string                                  _my_call;                            ///< my call
  std::string                                  _my_continent;                       ///< my continent
  unsigned int                                 _my_cq_zone;                         ///< my CQ zone
  std::string                                  _my_grid;                            ///< my grid square identifier
  std::string                                  _my_ip;                              ///< my IP address
  unsigned int                                 _my_itu_zone;                        ///< my ITU zone
  float                                        _my_latitude;                        ///< my latitude in degrees (north +ve)
  float                                        _my_longitude;                       ///< my longitude in degrees (east +ve)

  bool                                         _nearby_extract;      ///< whether to display NEARBY calls in EXTRACT window
  bool                                         _normalise_rate;      ///< whether to display rates as per-hour
  std::string                                  _not_country_mults;   ///< comma-separated list of countries that are explicitly NOT country mults

  std::string                                  _old_adif_log_name;  ///< name of ADIF file that contains old QSOs

  std::vector<std::string>                     _path;                             ///< directories to search, in order
  std::map<BAND, int>                          _per_band_country_mult_factor;     ///< country mult factor structure for each band
  std::array<std::map<BAND, std::string>, N_MODES> _per_band_points;              ///< points structure for each band and mode
//  std::map<std::string /* exchange field */, decltype(_per_band_points) > _per_band_points_with_exchange_field;              ///< points structure for each band and mode, if a particular exchange field is present

  unsigned int                                 _ptt_delay;                        ///< PTT delay in milliseconds ( 0 => PTT disabled)
  bool                                         _p3;                               ///< is a P3 available?
  bool                                         _p3_ignore_checksum_error;         ///< should checksum errors be ignored when acquiring P3 screendumps?
  std::string                                  _p3_snapshot_file;                 ///< base name of file for P3 snapshot
  unsigned int                                 _p3_span_cq;                       ///< P3 span in CQ mode, in kHz
  unsigned int                                 _p3_span_sap;                      ///< P3 span in SAP mode, in kHz

//  std::string                                  _quick_qsl_message;   ///< hurried confirm at end of QSO
  std::string                                  _qsl_message;         ///< confirm at end of QSO
  bool                                         _qso_multiple_bands;  ///< whether OK to work station on another band
  bool                                         _qso_multiple_modes;  ///< whether OK to work station on another mode
  bool                                         _qtcs;                ///< whether QTCs are enabled
  bool                                         _qtc_double_space;    ///< whether to leave a longer pause between elements of a QTC
  std::string                                  _qtc_filename;        ///< name of file where QTCs are stored
  unsigned int                                 _qtc_qrs;             ///< WPM decrease when sending QTC
  std::map<std::string, std::set<std::string>> _qthx;                ///< allowed exchanges values as a function of country

  std::vector<unsigned int>                    _rate_periods;                   ///< periods (in minutes) over which rates should be calculated
  bool                                         _rbn_beacons;                    ///< whether to place RBN posts from beacons on the bandmap
  unsigned int                                 _rbn_port;                       ///< port number on the RBN server
  std::string                                  _rbn_server;                     ///< hostname or IP address of RBN server
  unsigned int                                 _rbn_threshold;                  ///< number of different stations that have to post a station to the RBN before it shows on the bandmap
  std::string                                  _rbn_username;                   ///< username to use on the RBN server
  int                                          _reject_colour;                  ///< colour for calls that are dupes
  std::set<std::string>                        _remaining_callsign_mults_list;  ///< callsign mults to display
  std::set<std::string>                        _remaining_country_mults_list;   ///< country mults to display
  unsigned int                                 _rig1_baud;                      ///< baud rate for rig
  unsigned int                                 _rig1_data_bits;                 ///< number of data bits for rig
  std::string                                  _rig1_name;                      ///< name of rig
  std::string                                  _rig1_port;                      ///< port over which to communicate with rig
  unsigned int                                 _rig1_stop_bits;                 ///< number of stop bits for rig
  std::string                                  _rig1_type;                      ///< model name of rig
  std::string                                  _russian_filename;               ///< filename of russian location file (default = "russian-data")

  std::set<BAND>                               _score_bands;                    ///< which bands are going to be scored?
  std::set<MODE>                               _score_modes;                    ///< which modes are going to be scored?
  std::string                                  _screen_snapshot_file;           ///< base name of file for screenshot
  std::vector<std::pair<std::string, std::string> > _sent_exchange;             ///< names and values of sent exchange fields
  std::vector<std::pair<std::string, std::string> > _sent_exchange_cw;          ///< names and values of sent exchange fields, CW
  std::vector<std::pair<std::string, std::string> > _sent_exchange_ssb;         ///< names and values of sent exchange fields, SSB
  unsigned int                                 _shift_delta;                    ///< how many Hertz to QSY per poll of the shift key
  unsigned int                                 _shift_poll;                     ///< how frequently is the shift key polled during an RIT QSY, in milliseconds
  std::string                                  _society_list_filename;          ///< name of file containing IARU society exchanges
  enum BAND                                    _start_band;                     ///< on what band do we start?
  enum MODE                                    _start_mode;                     ///< on which mode do we start?
  std::map<std::string /* name */,
             std::pair<std::string /* contents */,
                         std::vector<window_information> > > _static_windows;   ///< size, position and content information for each static window
  bool                                         _sync_keyer;                     ///< whether to synchronise the rig keyer speed with the computer

  bool                                         _test;                           ///< whether to put rig in TEST mode
  std::string                                  _thousands_separator;            ///< character used as thousands separator in numbers

  bool                                         _uba_bonus;                      ///< whether to add UBA bonus QSO points

  std::map<std::string, window_information >   _windows;                        ///< size and position info for each window
  std::string                                  _worked_mults_colour;            ///< colour of worked mults in the mult windows

/*! \brief              Process a configuration file
    \param  filename    name of file to process

    This routine may be called recursively (by the RULES statement in the processed file)
*/
  void _process_configuration_file(const std::string& filename);

/*! \brief              Set the value of points, using the POINTS [CW|SSB] command
    \param  command     the complete line from the configuration file
    \param  m           mode
*/
  void _set_points(const std::string& command, const MODE m);

public:

/// default constructor
  drlog_context(void)
    { }

/// construct from file
  drlog_context( const std::string& filename );

  SAFEREAD(accept_colour, _context);                            ///< colour for calls that have been worked, but are not dupes
  SAFEREAD(alternative_exchange_cq, _context);                  ///< alternative exchange in SAP mode
  SAFEREAD(alternative_exchange_sap, _context);                 ///< alternative exchange in SAP mode
  SAFEREAD(alternative_qsl_message, _context);                  ///< alternative confirmation at end of QSO
  SAFEREAD(archive_name, _context);                             ///< name of the archive for save/restore information
  SAFEREAD(auto_backup, _context);                              ///< directory for auto backup files
  SAFEREAD(auto_remaining_callsign_mults, _context);            ///< do we auto-generate the remaining callsign mults?
  SAFEREAD(auto_remaining_country_mults, _context);             ///< do we auto-generate the remaining country mults?
  SAFEREAD(auto_remaining_country_mults_threshold, _context);   ///< number of times a canonical prefix must be seen before it becomes known

/*! \brief              Do we auto-generate remaining mults for a particular exchange mult?
    \param  mult_name   name of the exchange mult
    \return             whether the remaining values of the exchange mult <i>mult_name</i> are auto generated
*/
  inline const bool auto_remaining_exchange_mults(const std::string& mult_name) const
    { SAFELOCK(_context);

      return (_auto_remaining_exchange_mults.find(mult_name) != _auto_remaining_exchange_mults.end() );
    }

  SAFEREAD(auto_screenshot, _context);                  ///< do we create a screenshot every hour?

  SAFEREAD(bandmap_decay_time_local, _context);         ///< time (in minutes) for an entry to age off the bandmap (local entries)
  SAFEREAD(bandmap_decay_time_cluster, _context);       ///< time (in minutes) for an entry to age off the bandmap (cluster entries)
  SAFEREAD(bandmap_decay_time_rbn, _context);           ///< time (in minutes) for an entry to age off the bandmap (RBN entries)
  SAFEREAD(bandmap_fade_colours, _context);             ///< the colours calls adopt as they fade
  SAFEREAD(bandmap_filter, _context);                   ///< the strings in the bandmap filter
  SAFEREAD(bandmap_filter_disabled_colour, _context);   ///< background colour when bandmap filter is disabled
  SAFEREAD(bandmap_filter_enabled, _context);           ///< is the bandmap filter enabled?
  SAFEREAD(bandmap_filter_foreground_colour, _context); ///< colour of foreground in the bandmap filter

/// is the bandmap filter set to hide? (If not, then it's set to show)
  inline const bool bandmap_filter_hide(void) const
    { return !bandmap_filter_show(); }

  SAFEREAD(bandmap_filter_hide_colour, _context);       ///< background colour when bandmap filter is in hide mode
  SAFEREAD(bandmap_filter_show, _context);              ///< is the bandmap filter set to show? (If not, then it's set to hide)
  SAFEREAD(bandmap_filter_show_colour, _context);       ///< background colour when bandmap filter is in show mode
  SAFEREAD(bandmap_recent_colour, _context);            ///< colour for bandmap entries that are less than two minutes old
  SAFEREAD(bands, _context);                            ///< comma-delimited bands
  SAFEREAD(batch_messages_file, _context);              ///< file that contains per-call batch messages

  SAFEREAD(cabrillo_address_1, _context);               ///< first ADDRESS: line
  SAFEREAD(cabrillo_address_2, _context);               ///< second ADDRESS: line
  SAFEREAD(cabrillo_address_3, _context);               ///< third ADDRESS: line
  SAFEREAD(cabrillo_address_4, _context);               ///< fourth ADDRESS: line
  SAFEREAD(cabrillo_address_city, _context);            ///< ADDRESS-CITY:
  SAFEREAD(cabrillo_address_state_province, _context);  ///< ADDRESS-STATE-PROVINCE:
  SAFEREAD(cabrillo_address_postalcode, _context);      ///< ADDRESS-POSTALCODE:
  SAFEREAD(cabrillo_address_country, _context);         ///< ADDRESS-COUNTRY:
  SAFEREAD(cabrillo_callsign, _context);                ///< CALLSIGN:
  SAFEREAD(cabrillo_category_assisted, _context);       ///< CATEGORY-ASSISTED:
  SAFEREAD(cabrillo_category_band, _context);           ///< CATEGORY-BAND:
  SAFEREAD(cabrillo_category_mode, _context);           ///< CATEGORY-MODE:
  SAFEREAD(cabrillo_category_operator, _context);       ///< CATEGORY-OPERATOR:
  SAFEREAD(cabrillo_category_overlay, _context);        ///< CATEGORY-OVERLAY:
  SAFEREAD(cabrillo_category_power, _context);          ///< CATEGORY-POWER:
  SAFEREAD(cabrillo_category_station, _context);        ///< CATEGORY-STATION:
  SAFEREAD(cabrillo_category_time, _context);           ///< CATEGORY-TIME:
  SAFEREAD(cabrillo_category_transmitter, _context);    ///< CATEGORY-TRANSMITTER:
  SAFEREAD(cabrillo_certificate, _context);             ///< CERTIFICATE:
  SAFEREAD(cabrillo_club, _context);                    ///< CLUB:
  SAFEREAD(cabrillo_contest, _context);                 ///< CONTEST:
  SAFEREAD(cabrillo_eol, _context);                     ///< EOL used in the cabrillo file; one of: "LF", "CR" or "CRLF"
  SAFEREAD(cabrillo_e_mail, _context);                  ///< EMAIL: (sic)
  SAFEREAD(cabrillo_filename, _context);                ///< name of Cabrillo log
  SAFEREAD(cabrillo_include_score, _context);           ///< is the CLAIMED-SCORE line included in the Cabrillo file?
  SAFEREAD(cabrillo_location, _context);                ///< LOCATION:
  SAFEREAD(cabrillo_name, _context);                    ///< NAME:
  SAFEREAD(cabrillo_operators, _context);               ///< OPERATORS:
  SAFEREAD(cabrillo_qso_template, _context);            ///< format for Cabrillo QSOs

  SAFEREAD(call_ok_now_message, _context);              ///< message if call was changed
  SAFEREAD(callsign_mults, _context);                   ///< mults derived from callsign; e.g., WPXPX
  SAFEREAD(callsign_mults_per_band, _context);          ///< are callsign mults per-band?
  SAFEREAD(callsign_mults_per_mode, _context);          ///< are callsign mults per-mode?
  SAFEREAD(cluster_port, _context);                     ///< port on the cluster server
  SAFEREAD(cluster_server, _context);                   ///< hostname or IP of cluster server
  SAFEREAD(cluster_username, _context);                 ///< username to use on the cluster
  SAFEREAD(contest_name, _context);                     ///< name of the contest
  SAFEREAD(country_list, _context);                     ///< DXCC or WAE list?
  SAFEREAD(country_mults_filter, _context);             ///< the command from the configuration file
  SAFEREAD(country_mults_per_band, _context);           ///< are country mults per-band?
  SAFEREAD(country_mults_per_mode, _context);           ///< are country mults per-mode?
  SAFEREAD(cq_auto_lock, _context);                     ///< whether to lock the transmitter in CQ mode
  SAFEREAD(cq_auto_rit, _context);                      ///< whether to enable RIT in CQ mode
  SAFEREAD(cty_filename, _context);                     ///< filename of country file (default = "cty.dat")
  SAFEREAD(cw_speed, _context);                         ///< speed in WPM

  SAFEREAD(decimal_point, _context);                    ///< character to use as decimal point
  SAFEREAD(display_communication_errors, _context);     ///< whether to display errors communicating with rig
  SAFEREAD(do_not_show, _context);                      ///< do not show these calls when spotted (MY CALL is automatically not shown)
  SAFEREAD(do_not_show_filename, _context);             ///< filename of calls (one per line) not to be shown
  SAFEREAD(drmaster_filename, _context);                ///< filename of drmaster file (default = "drmaster")

  SAFEREAD(exchange, _context);                         ///< comma-delimited received exchange
  SAFEREAD(exchange_cq, _context);                      ///< exchange in CQ mode
  SAFEREAD(exchange_fields_filename, _context);         ///< file that holds regex templates of values of exchange fields
  SAFEREAD(exchange_mults, _context);                   ///< comma-delimited exchange fields that are mults
  SAFEREAD(exchange_mults_per_band, _context);          ///< are exchange mults per-band?
  SAFEREAD(exchange_mults_per_mode, _context);          ///< are exchange mults per-mode?
  SAFEREAD(exchange_per_country, _context);             ///< per-country exchanges; key = prefix-or-call; value = exchange
  SAFEREAD(exchange_sap, _context);                     ///< exchange in SAP mode

  SAFEREAD(fast_cq_bandwidth, _context);                ///< fast CW bandwidth in CQ mode, in Hz
  SAFEREAD(fast_sap_bandwidth, _context);               ///< fast CW bandwidth in SAP mode, in Hz

/*! \brief      Get the guard band for a particular mode
    \param  m   target mode
    \return     guard band for mode <i>m</i>, in Hz
*/
  const unsigned int guard_band(const MODE m)
  { SAFELOCK(_context);

    const auto cit = _guard_band.find(m);

    return  ( (cit == _guard_band.end()) ? 1000 : cit->second );
  }

  SAFEREAD(individual_messages_file, _context);     ///< name of file that contains per-call individual messages

  SAFEREAD(keyer_port, _context);                   ///< the device that is to be used as a keyer

  SAFEREAD(logfile, _context);                      ///< name of the log file

  SAFEREAD(mark_frequencies, _context);             ///< frequency ranges to be marked on-screen
  SAFEREAD(match_minimum, _context);                ///< number of characters before SCP or fuzzy match kicks in
  SAFEREAD(messages, _context);                     ///< CW messages
  SAFEREAD(message_cq_1, _context);                 ///< CQ message #1 (generally, a short CQ)
  SAFEREAD(message_cq_2, _context);                 ///< CQ message #2 (generally, a long CQ)
  SAFEREAD(modes, _context);                        ///< comma-delimited modes CW, SSB
  SAFEREAD(mode_break_points, _context);            ///< override default mode break points
  SAFEREAD(my_call, _context);                      ///< my call
  SAFEREAD(my_continent, _context);                 ///< my continent
  SAFEREAD(my_cq_zone, _context);                   ///< my CQ zone
  SAFEREAD(my_grid, _context);                      ///< my grid square identifier
  SAFEREAD(my_ip, _context);                        ///< my IP address
  SAFEREAD(my_itu_zone, _context);                  ///< my ITU zone
  SAFEREAD(my_latitude, _context);                  ///< my latitude in degrees (north +ve)
  SAFEREAD(my_longitude, _context);                 ///< my longitude in degrees (east +ve)

  SAFEREAD(nearby_extract, _context);               ///< whether to display NEARBY calls in EXTRACT window
  SAFEREAD(normalise_rate, _context);               ///< whether to display rates as per-hour
  SAFEREAD(not_country_mults, _context);            ///< comma-separated list of countries that are explicitly NOT country mults

  SAFEREAD(old_adif_log_name, _context);            ///< name of ADIF file that contains old QSOs

  SAFEREAD(path, _context);                         ///< directories to search, in order
  SAFEREAD(per_band_country_mult_factor, _context); ///< country mult factor structure for each band
  SAFEREAD(per_band_points, _context);              ///< points structure for each band and mode


//  std::map<std::string /* exchange field */, decltype(_per_band_points) > _per_band_points_with_exchange_field;              ///< points structure for each band and mode, if a particular exchange field is present


/*! \brief      Get the points string for a particular band and mode
    \param  b   band
    \param  m   mode
    \return     the points string corresponding to band <i>b</i> and mode <i>m</i>
*/
  const std::string points(const BAND b, const MODE m) const;

/*! \brief                  Get the points string for a particular band and mode, if a particular exchange field is present
    \param  exchange_field  exchange field
    \param  b               band
    \param  m               mode
    \return                 the points string corresponding to band <i>b</i> and mode <i>m</i> when exchange field <i>exchange_field</i> is present
*/
//  const std::string points(const std::string& exchange_field, const BAND b, const MODE m) const;

  SAFEREAD(ptt_delay, _context);                    ///< PTT delay in milliseconds ( 0 => PTT disabled)
  SAFEREAD(p3, _context);                           ///< is a P3 available?
  SAFEREAD(p3_ignore_checksum_error, _context);     ///< should checksum errors be ignored when acquiring P3 screendumps?
  SAFEREAD(p3_snapshot_file, _context);             ///< base name of file for P3 snapshot
  SAFEREAD(p3_span_cq, _context);                   ///< P3 span in CQ mode, in kHz
  SAFEREAD(p3_span_sap, _context);                  ///< P3 span in SAP mode, in kHz

  SAFEREAD(qsl_message, _context);                  ///< confirm at end of QSO
  SAFEREAD(qso_multiple_bands, _context);           ///< whether OK to work station on another band
  SAFEREAD(qso_multiple_modes, _context);           ///< whether OK to work station on another mode
  SAFEREAD(qtcs, _context);                         ///< whether QTCs are enabled
  SAFEREAD(qtc_double_space, _context);             ///< whether to leave a longer pause between elements of a QTC
  SAFEREAD(qtc_filename, _context);                 ///< name of file where QTCs are stored
  SAFEREAD(qtc_qrs, _context);                      ///< WPM decrease when sending QTC
  SAFEREAD(qthx, _context);                         ///< allowed exchanges values as a function of country

  SAFEREAD(rate_periods, _context);                     ///< periods (in minutes) over which rates should be calculated
  SAFEREAD(rbn_beacons, _context);                      ///< whether to place RBN posts from beacons on the bandmap
  SAFEREAD(rbn_port, _context);                         ///< port number on the RBN server
  SAFEREAD(rbn_server, _context);                       ///< hostname or IP address of RBN server
  SAFEREAD(rbn_threshold, _context);                    ///< number of different stations that have to post a station to the RBN before it shows on the bandmap
  SAFEREAD(rbn_username, _context);                     ///< username to use on the RBN server
  SAFEREAD(reject_colour, _context);                    ///< colour for calls that are dupes
  SAFEREAD(remaining_callsign_mults_list, _context);    ///< callsign mults to display
  SAFEREAD(remaining_country_mults_list, _context);     ///< country mults to display
  SAFEREAD(rig1_baud, _context);                        ///< baud rate for rig
  SAFEREAD(rig1_data_bits, _context);                   ///< number of data bits for rig
  SAFEREAD(rig1_name, _context);                        ///< name of rig
  SAFEREAD(rig1_port, _context);                        ///< port over which to communicate with rig
  SAFEREAD(rig1_stop_bits, _context);                   ///< number of stop bits for rig
  SAFEREAD(rig1_type, _context);                        ///< model name of rig
  SAFEREAD(russian_filename, _context);                 ///< filename of russian location file (default = "russian-data")

  SAFEREAD(score_bands, _context);                      ///< which bands are going to be scored?
  SAFEREAD(score_modes, _context);                      ///< which modes are going to be scored?
  SAFEREAD(screen_snapshot_file, _context);             ///< base name of file for screenshot

  const decltype(_sent_exchange) sent_exchange(const MODE m);   ///< names and values of sent exchange fields for mode <i>m</i>

  SAFEREAD(sent_exchange_cw, _context);                 ///< names and values of sent exchange fields, CW
  SAFEREAD(sent_exchange_ssb, _context);                ///< names and values of sent exchange fields, SSB
  SAFEREAD(shift_delta, _context);                      ///< how many Hertz to QSY per poll of the shift key
  SAFEREAD(shift_poll, _context);                       ///< how frequently is the shift key polled during an RIT QSY, in milliseconds
  SAFEREAD(society_list_filename, _context);            ///< name of file containing IARU society exchanges
  SAFEREAD(start_band, _context);                       ///< on what band do we start?
  SAFEREAD(start_mode, _context);                       ///< on which mode do we start?
  SAFEREAD(static_windows, _context);                   ///< size, position and content information for each static window
  SAFEREAD(sync_keyer, _context);                       ///< whether to synchronise the rig keyer speed with the computer

  SAFEREAD(test, _context);                             ///< whether to put rig in TEST mode
  SAFEREAD(thousands_separator, _context);              ///< character used as thousands separator in numbers

  SAFEREAD(uba_bonus, _context);                        ///< whether to add UBA bonus QSO points

  SAFEREAD(worked_mults_colour, _context);              ///< colour of worked mults in the mult windows

/*! \brief          Information pertaining to a particular window
    \param  name    name of window
    \return         location, size and colour information
*/
  const window_information window_info(const std::string& name) const;

/// vector of the names of bands (e.g., "160", "80", etc.)
  inline const std::vector<std::string> band_names(void) const
    { SAFELOCK(_context);
      return split_string(_bands, ",");
    }

/// vector of the names of modes (e.g., "CW", "SSB", etc.)
  inline const std::vector<std::string> mode_names(void) const
    { SAFELOCK(_context);
      return split_string(_modes, ",");
    }

/// how many bands are used in this contest?
  inline const unsigned int n_bands(void) const
    { SAFELOCK(_context);
      return band_names().size();
    }

/// how many modes are used in this contest?
  inline const unsigned int n_modes(void) const
    { SAFELOCK(_context);
      return mode_names().size();
    }

/*! \brief          All the windows whose name contains a particular substring
    \param  substr  substring for which to search
    \return         all the window names that include <i>substr</i>
*/
  const std::vector<std::string> window_name_contains(const std::string& substr) const;

/*! \brief      Is a particular frequency within any marked range?
    \param  m   mode
    \param  f   frequency to test
    \return     whether <i>f</i> is in any marked range for the mode <i>m</i>
*/
  const bool mark_frequency(const MODE, const frequency& f);

/*! \brief      Get all the field names in the sent exchange
    \return     the names of all the fields in the sent exchange
*/
  const std::vector<std::string> sent_exchange_names(void) const;

/*! \brief      Get all the field names in the sent CW exchange
    \return     the names of all the fields in the sent CW exchange
*/
  const std::vector<std::string> sent_exchange_cw_names(void) const;

/*! \brief      Get all the field names in the sent SSB exchange
    \return     the names of all the fields in the sent SSB exchange
*/
  const std::vector<std::string> sent_exchange_ssb_names(void) const;

/// swap QSL and ALTERNATIVE QSL messages
  inline void swap_qsl_messages(void)
    { SAFELOCK(_context);
      swap(_qsl_message, _alternative_qsl_message);
    }

/// are multiple modes permitted?
  inline const bool multiple_modes(void) const
  { SAFELOCK(_context);
    return (_modes.size() != 1);
  }
};

#endif    // DRLOG_CONTEXT_H


