// $Id: drlog_context.h 197 2021-11-21 14:52:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef DRLOG_CONTEXT_H
#define DRLOG_CONTEXT_H

/*! \file   drlog_context.h

    The basic context for operation of drlog
*/

#include "bands-modes.h"
#include "cty_data.h"
#include "screen.h"

#include <array>
#include <set>
#include <string>

/// start-up control of audio recording
enum class AUDIO_RECORDING { AUTO,              ///< auto control
                             DO_NOT_START,      ///< do not start at start-up
                             START              ///< start at start-up
                           };

extern pt_mutex _context_mutex;             ///< mutex for the drlog context

/// Syntactic sugar for read-only access
#define CONTEXTREAD(y)          \
  inline decltype(_##y) y(void) const { SAFELOCK(_context); return _##y; }

// -----------  drlog_context  ----------------

/*! \class  drlog_context
    \brief  The variables and constants that comprise the context for operation
*/

class drlog_context
{
protected:

  int                                          _accept_colour                           { COLOUR_GREEN };               ///< colour for calls that have been worked, but are not dupes
  bool                                         _allow_audio_recording                   { false };                      ///< whether to allow recording of audio
//  bool                                         _allow_variable_sent_rst;                    ///< whether to permit the sent RST to vary
  std::string                                  _alternative_exchange_cq                 { };                            ///< alternative exchange in CQ mode
  std::string                                  _alternative_exchange_sap                { };                            ///< alternative exchange in SAP mode
  std::string                                  _alternative_qsl_message                 { };                            ///< alternative confirmation at end of QSO (default is changed once configuration file has been read)
  std::string                                  _archive_name                            { "drlog-restart"s };           ///< name of the archive for save/restore information
  unsigned int                                 _audio_channels                          { 1 };                          ///< number of audio channels
  std::string                                  _audio_device_name                       { "default"s };                 ///< name of audio device
  unsigned int                                 _audio_duration                          { 60 };                         ///< maximum duration in minutes, per file
  std::string                                  _audio_file                              { "audio"s };                   ///< base name of file for audio recordings
  unsigned int                                 _audio_rate                              { 8000 };                       ///< number of samples per second
  bool                                         _autocorrect_rbn                         { false };                      ///< whether to try to autocorrect posts from the RBN
  std::string                                  _auto_backup_directory                   { };                            ///< directory for auto backup files (default = no directory)
  bool                                         _auto_cq_mode_ssb                        { false };                      ///< do we automatically go to CQ mode on SSB?
  bool                                         _auto_remaining_callsign_mults           { false };                      ///< do we auto-generate the remaining callsign mults?
  unsigned int                                 _auto_remaining_callsign_mults_threshold { 1 };                          ///< number of times a callsign mult must be seen before it becomes known
  bool                                         _auto_remaining_country_mults            { false };                      ///< do we auto-generate the remaining country mults?
  unsigned int                                 _auto_remaining_country_mults_threshold  { 1 };                          ///< number of times a canonical prefix must be seen before it becomes known
  std::set<std::string>                        _auto_remaining_exchange_mults           { };                            ///< the exchange mults for which we auto-generate the values
  bool                                         _auto_screenshot                         { false };                      ///< do we create a screenshot every hour?

  int                                          _bandmap_cull_function                   { 0 };                          ///< number of the bandmap cull function
  unsigned int                                 _bandmap_decay_time_local                { 60 };                         ///< time (in minutes) for an entry to age off the bandmap (local entries)
  unsigned int                                 _bandmap_decay_time_cluster              { 60 };                         ///< time (in minutes) for an entry to age off the bandmap (cluster entries)
  unsigned int                                 _bandmap_decay_time_rbn                  { 60 };                         ///< time (in minutes) for an entry to age off the bandmap (RBN entries)
  std::vector<COLOUR_TYPE>                     _bandmap_fade_colours                    { 255, 200, 150, 100 };         ///< the colours calls adopt as they fade
  std::vector<std::string>                     _bandmap_filter                          { };                            ///< the strings in the bandmap filter
  COLOUR_TYPE                                  _bandmap_filter_disabled_colour          { COLOUR_BLACK };               ///< background colour when bandmap filter is disabled
  bool                                         _bandmap_filter_enabled                  { false };                      ///< is the bandmap filter enabled?
  COLOUR_TYPE                                  _bandmap_filter_foreground_colour        { COLOUR_WHITE };               ///< colour of foreground in the bandmap filter
  COLOUR_TYPE                                  _bandmap_filter_hide_colour              { COLOUR_RED };                 ///< background colour when bandmap filter is in hide mode
  bool                                         _bandmap_filter_show                     { false };                      ///< is the bandmap filter set to show? (If not, then it's set to hide)
  COLOUR_TYPE                                  _bandmap_filter_show_colour              { COLOUR_GREEN };               ///< background colour when bandmap filter is in show mode
  bool                                         _bandmap_frequency_up                    { false };                      ///< should increasing frequency go upwards in the bandmap?
  COLOUR_TYPE                                  _bandmap_recent_colour                   { COLOUR_BLACK };               ///< colour for bandmap entries that are less than two minutes old
  bool                                         _bandmap_show_marked_frequencies         { true };                       ///< whether to display entries that would be marked
  std::string                                  _bands                                   { "160, 80, 40, 20, 15, 10"s }; ///< comma-delimited list of bands that are legal for the contest
  std::string                                  _batch_messages_file                     { };                            ///< file that contains per-call batch messages
  std::string                                  _best_dx_unit                            { "MILES"s };                   ///< name of unit for the BEST DX window ("MILES" or "KM")

  std::string                                  _cabrillo_eol                            { "LF"s };                      ///< EOL used in the cabrillo file; one of: "LF", "CR" or "CRLF"
  std::string                                  _cabrillo_filename                       { "cabrillo"s };                ///< name of Cabrillo log
  bool                                         _cabrillo_include_score                  { true };                       ///< is the CLAIMED-SCORE line included in the Cabrillo file?

// Cabrillo records
  std::string                                  _cabrillo_address_1                      { };                            ///< first ADDRESS: line
  std::string                                  _cabrillo_address_2                      { };                            ///< second ADDRESS: line
  std::string                                  _cabrillo_address_3                      { };                            ///< third ADDRESS: line
  std::string                                  _cabrillo_address_4                      { };                            ///< fourth ADDRESS: line
  std::string                                  _cabrillo_address_city                   { };                            ///< ADDRESS-CITY:
  std::string                                  _cabrillo_address_country                { };                            ///< ADDRESS-COUNTRY:
  std::string                                  _cabrillo_address_postalcode             { };                            ///< ADDRESS-POSTALCODE:
  std::string                                  _cabrillo_address_state_province         { };                            ///< ADDRESS-STATE-PROVINCE:
  std::string                                  _cabrillo_callsign                       { };                            ///< CALLSIGN:
  std::string                                  _cabrillo_category_assisted              { "ASSISTED"s };                ///< CATEGORY-ASSISTED:
  std::string                                  _cabrillo_category_band                  { "ALL"s };                     ///< CATEGORY-BAND:
  std::string                                  _cabrillo_category_mode                  { };                            ///< CATEGORY-MODE: no default, must be filled in by the config file
  std::string                                  _cabrillo_category_operator              { "SINGLE-OP"s };               ///< CATEGORY-OPERATOR:
  std::string                                  _cabrillo_category_overlay               { };                            ///< CATEGORY-OVERLAY:
  std::string                                  _cabrillo_category_power                 { "HIGH"s };                    ///< CATEGORY-POWER:
  std::string                                  _cabrillo_category_station               { };                            ///< CATEGORY-STATION:
  std::string                                  _cabrillo_category_time                  { };                            ///< CATEGORY-TIME:
  std::string                                  _cabrillo_category_transmitter           { "ONE"s };                     ///< CATEGORY-TRANSMITTER:
  std::string                                  _cabrillo_certificate                    { "YES"s };                     ///< CERTIFICATE: explicitly request a certificate, because of inanity in the specification
  std::string                                  _cabrillo_club                           { };                            ///< CLUB:
  std::string                                  _cabrillo_contest                        { };                            ///< CONTEST:
  std::string                                  _cabrillo_e_mail                         { };                            ///< EMAIL: (sic)
  std::string                                  _cabrillo_location                       { };                            ///< LOCATION:
  std::string                                  _cabrillo_name                           { };                            ///< NAME:
  std::string                                  _cabrillo_operators                      { };                            ///< OPERATORS:
  std::string                                  _cabrillo_qso_template                   { };                            ///< format for Cabrillo QSOs; empty QSO template; this must be properly defined if Cabrillo is used

  std::set<BAND>                               _call_history_bands                      { };                            ///< bands to show in CALL HISTORY window
  std::string                                  _call_ok_now_message                     { };                            ///< message if call was changed
  std::set<std::string>                        _callsign_mults                          { };                            ///< mults derived from callsign; e.g., WPXPX
  bool                                         _callsign_mults_per_band                 { false };                      ///< are callsign mults per-band?
  bool                                         _callsign_mults_per_mode                 { false };                      ///< are callsign mults per-mode?
  unsigned int                                 _cluster_port                            { 23 };                         ///< port on the cluster server; standard telnet server port
  std::string                                  _cluster_server                          { };                            ///< hostname or IP of cluster server
  std::string                                  _cluster_username                        { };                            ///< username to use on the cluster
  std::string                                  _contest_name                            { };                            ///< name of the contest
  COUNTRY_LIST                                 _country_list                            { COUNTRY_LIST::WAEDC };         ///< DXCC or WAE list?
  std::string                                  _country_mults_filter                    { "ALL"s };                     ///< the command from the configuration file; default all countries are mults
  bool                                         _country_mults_per_band                  { true };                       ///< are country mults per-band?
  bool                                         _country_mults_per_mode                  { false };                      ///< are country mults per-mode?
  bool                                         _cq_auto_lock                            { false };                      ///< whether to lock the transmitter in CQ mode
  bool                                         _cq_auto_rit                             { false };                      ///< whether to enable RIT in CQ mode
  std::string                                  _cty_filename                            { "cty.dat"s };                 ///< filename for country data
  int                                          _cw_bandwidth_narrow                     { 100 };                        ///< narrow CW bandwidth (Hz)
  int                                          _cw_bandwidth_wide                       { 400 };                        ///< wide CW bandwidth (Hz)
  int                                          _cw_priority                             { -1 };                         ///< priority of CW thread (-1 = non-RT; 0 = middle RT; otherwise priority number)
  unsigned int                                 _cw_speed                                { 29 };                         ///< speed in WPM
  unsigned int                                 _cw_speed_change                         { 3 };                          ///< change in CW speed in WPM when pressing PAGE UP or PAGE DOWN

  std::string                                  _decimal_point                           { "·"s };                       ///< character to use as decimal point
  bool                                         _display_communication_errors            { true };                       ///< whether to display errors communicating with rig
  bool                                         _display_grid                            { false };                      ///< whether grid will be shown in GRID and INFO windows
  std::vector<std::string>                     _do_not_show                             { };                            ///< do not show these calls when spotted (MY CALL is automatically not shown)
  std::string                                  _do_not_show_filename                    { };                            ///< filename containing calls (one per line) not to be shown
  std::string                                  _drmaster_filename                       { "drmaster"s };                ///< filename of drmaster file

  std::string                                  _exchange                                { "RST"s };                      ///< comma-delimited received exchange
  std::string                                  _exchange_cq                             { };                            ///< exchange in CQ mode
  std::string                                  _exchange_fields_filename                { };                            ///< file that holds regex templates of exchange fields
  std::string                                  _exchange_mults                          { };                            ///< comma-delimited exchange fields that are mults
  bool                                         _exchange_mults_per_band                 { false };                      ///< are exchange mults per-band?
  bool                                         _exchange_mults_per_mode                 { false };                      ///< are exchange mults per-mode?
  std::map<std::string, std::string>           _exchange_per_country                    { };                            ///< per-country exchanges; key = prefix-or-call; value = exchange
  std::map<std::string, std::string>           _exchange_prefill_files                  { };                            ///< external prefill files for exchange fields
  std::string                                  _exchange_sap                            { };                            ///< exchange in SAP mode

  unsigned int                                 _fast_cq_bandwidth                       { 400 };                        ///< fast CW bandwidth in CQ mode, in Hz
  unsigned int                                 _fast_sap_bandwidth                      { 400 };                        ///< fast CW bandwidth in SAP mode, in Hz

  std::string                                  _geomagnetic_indices_command             { };                                        ///< command to get geomagnetic indices
  std::map<MODE, unsigned int>                 _guard_band                              { { MODE_CW, 500 }, { MODE_SSB, 2000 } };   ///< guard band, in Hz

  bool                                         _home_exchange_window                    { false };                      ///< whether to move cursor to left of exchange window (and insert space if necessary)

  int                                          _inactivity_timer                        { 0 };                          ///< duration in seconds before audio recording ceases in auto mode; 0 = no inactivity timer
  std::string                                  _individual_messages_file                { };                            ///< name of file that contains per-call individual messages

  std::string                                  _keyer_port                              { };                            ///< the device that is to be used as a keyer

  std::string                                  _logfile                                 { "drlog.dat"s };               ///< name of the log file
  unsigned int                                 _long_t                                  { 0 };                          ///< whether and amount to extend length of initial Ts in serial number

  std::map<MODE, std::vector<std::pair<frequency, frequency>>> _mark_frequencies        { };                            ///< frequency ranges to be marked on-screen
  bool                                         _mark_mode_break_points                  { false };                      ///< whether to mark the mode break points on the bandmap
  unsigned int                                 _match_minimum                           { 4 };                          ///< number of characters before SCP or fuzzy match kicks in
  unsigned int                                 _max_qsos_without_qsl                    { 4 };                          ///< cutoff for the N7DR matches_criteria() algorithm

// we use the KeySymbol as the integer, although other I/O implementations could use something else
  std::map<int, std::string >                  _messages                                { };                            ///< CW messages

  std::string                                  _message_cq_1                            { };                            ///< CQ message #1 (generally, a short CQ)
  std::string                                  _message_cq_2                            { };                            ///< CQ message #2 (generally, a long CQ)
  bool                                         _mm_country_mults                        { false };                      ///< can /MM QSOs be mults?
  std::string                                  _modes                                   { "CW"s };                      ///< comma-delimited list of modes that are legal for the contest
  std::map<BAND, frequency>                    _mode_break_points                       { };                            ///< override default mode break points
  std::string                                  _my_call                                 { "NONE"s };                    ///< my call
  std::string                                  _my_continent                            { "XX"s };                      ///< my continent
  unsigned int                                 _my_cq_zone                              { };                            ///< my CQ zone
  std::string                                  _my_grid                                 { };                            ///< my grid square identifier
  std::string                                  _my_ip                                   { };                            ///< my IP address
  unsigned int                                 _my_itu_zone                             { };                            ///< my ITU zone
  float                                        _my_latitude                             { 0 };                          ///< my latitude in degrees (north +ve)
  float                                        _my_longitude                            { 0 };                          ///< my longitude in degrees (east +ve)

  bool                                         _nearby_extract                          { false };                      ///< whether to display NEARBY calls in EXTRACT window
  bool                                         _normalise_rate                          { false };                      ///< whether to display rates as per-hour
  std::string                                  _not_country_mults                       { };                            ///< comma-separated list of countries that are explicitly NOT country mults
  bool                                         _no_default_rst                          { false };                      ///< is there no default assignment for received RST ?
  unsigned int                                 _n_memories                              { 0 };                          ///< number of rig memories

  std::string                                  _old_adif_log_name                       { };                            ///< name of ADIF file that contains old QSOs
  unsigned int                                 _old_qso_age_limit                       { 0 };                          ///< include old QSOs newer than this value (in years) [0 => all no limit]  

  std::vector<std::string>                         _path                                { "."s };                       ///< comma-separated list of directories to search, in order
  std::map<BAND, int>                              _per_band_country_mult_factor        { };                            ///< country mult factor structure for each band
  std::array<std::map<BAND, std::string>, N_MODES> _per_band_points                     { };                            ///< points structure for each band and mode
//  std::map<std::string /* exchange field */, decltype(_per_band_points) > _per_band_points_with_exchange_field;              ///< points structure for each band and mode, if a particular exchange field is present

  std::set<std::string>                        _post_monitor_calls                      { };                            ///< calls to be monitored
  std::set<std::string>                        _posted_by_continents                    { };                            ///< continents for POSTED BY window (empty => all DX continents)
  unsigned int                                 _ptt_delay                               { 25 };                         ///< PTT delay in milliseconds ( 0 => PTT disabled)
  bool                                         _p3                                      { false };                      ///< is a P3 available?
  bool                                         _p3_ignore_checksum_error                { false };                      ///< should checksum errors be ignored when acquiring P3 screendumps?
  std::string                                  _p3_snapshot_file                        { "P3"s };                      ///< base name of file for P3 snapshot
  unsigned int                                 _p3_span_cq                              { 0 };                          ///< P3 span in CQ mode, in kHz (0 = no default span)
  unsigned int                                 _p3_span_sap                             { 0 };                          ///< P3 span in SAP mode, in kHz (0 = no default span)

  std::string                                  _qsl_message                             { };                            ///< confirmation message at end of QSO
  bool                                         _qso_multiple_bands                      { false };                      ///< whether OK to work station on another band
  bool                                         _qso_multiple_modes                      { false };                      ///< whether OK to work station on another mode
  bool                                         _qsy_on_startup                          { true };                       ///< whether to go to START BAND on startup
  bool                                         _qtcs                                    { false };                      ///< whether QTCs are enabled
  bool                                         _qtc_double_space                        { false };                      ///< whether to leave a longer pause between elements of a QTC
  std::string                                  _qtc_filename                            { "QTCs"s };                    ///< name of file where QTCs are stored
  unsigned int                                 _qtc_long_t                              { 0 };                          ///< whether and amount to extend length of initial Ts in serial number in QTCs
  unsigned int                                 _qtc_qrs                                 { 0 };                          ///< WPM decrease when sending QTC
  std::map<std::string, std::set<std::string>> _qthx                                    { };                            ///< allowed exchange values as a function of country

  std::vector<unsigned int>                    _rate_periods                            { 15, 30, 60 } ;                ///< periods (in minutes) over which rates should be calculated
  bool                                         _rbn_beacons                             { false };                      ///< whether to place RBN posts identified as from beacons on the bandmap
  unsigned int                                 _rbn_port                                { 7000 };                       ///< port number on the RBN server
  std::string                                  _rbn_server                              { "telnet.reversebeacon.net"s };///< hostname or IP address of RBN server
  unsigned int                                 _rbn_threshold                           { 1 };                          ///< number of different stations that have to post a station to the RBN before it appears on the bandmap
  std::string                                  _rbn_username                            { };                            ///< username to use on the RBN server
  int                                          _reject_colour                           { COLOUR_RED };                 ///< colour for calls that are dupes
  std::set<std::string>                        _remaining_callsign_mults_list           { };                            ///< callsign mults to display
  std::set<std::string>                        _remaining_country_mults_list            { };                            ///< country mults to display
  bool                                         _require_dot_in_replacement_call         { false };                      ///< whether to require a dot in a replacement call in the EXCHANGE window
  unsigned int                                 _rig1_baud                               { 4800 };                       ///< baud rate for rig
  unsigned int                                 _rig1_data_bits                          { 8 };                          ///< number of data bits for rig
  std::string                                  _rig1_name                               { };                            ///< name of rig
  std::string                                  _rig1_port                               { "/dev/ttyS0"s };              ///< port over which to communicate with rig
  unsigned int                                 _rig1_stop_bits                          { 1 };                          ///< number of stop bits for rig
  std::string                                  _rig1_type                               { };                            ///< model name of rig
  std::string                                  _russian_filename                        { "russian-data"s };            ///< name of russian location file

  std::set<BAND>                               _score_bands                             { };                                ///< which bands are going to be scored?
  std::set<MODE>                               _score_modes                             { };                                ///< which modes are going to be scored?
  bool                                         _scoring_enabled                         { true };                           ///< is scoring enabled?
  std::string                                  _screen_snapshot_file                    { "screen"s };                      ///< base name of file for screenshot
  bool                                         _screen_snapshot_on_exit                 { false };                          ///< whether to take a screenshot on exit
  std::vector<std::pair<std::string, std::string> > _sent_exchange                      { };                                ///< names and values of sent exchange fields
  std::vector<std::pair<std::string, std::string> > _sent_exchange_cw                   { };                                ///< names and values of sent exchange fields, CW
  std::vector<std::pair<std::string, std::string> > _sent_exchange_ssb                  { };                                ///< names and values of sent exchange fields, SSB
  unsigned int                                 _serno_spaces                            { 0 };                              ///< number of half-length spaces to insert when sending serno
  unsigned int                                 _shift_delta_cw                          { 10 };                             ///< how many hertz to QSY per poll of the shift key on CW
  unsigned int                                 _shift_delta_ssb                         { 100 };                            ///< how many hertz to QSY per poll of the shift key on SSB
  unsigned int                                 _shift_poll                              { 50 };                             ///< how frequently to poll the shift key during an RIT QSY, in milliseconds
  bool                                         _short_serno                             { false };                          ///< whether to omit leading Ts
  std::string                                  _society_list_filename                   { };                                ///< name of file containing IARU society exchanges
  int                                          _ssb_bandwidth_narrow                    { 1600 };                           ///< narrow SSB bandwidth (Hz)
  int                                          _ssb_bandwidth_wide                      { 1800 };                           ///< wide SSB bandwidth (Hz)
  int                                          _ssb_centre_bandwidth_narrow             { 1300 };                           ///< centre frequency for narrow SSB bandwidth (Hz)
  int                                          _ssb_centre_bandwidth_wide               { 1500 };                           ///< centre frequency for wide SSB bandwidth (Hz)
  enum AUDIO_RECORDING                         _start_audio_recording                   { AUDIO_RECORDING::DO_NOT_START };  ///< whether and how to start recording of audio (if _allow_audio_recording is true)
  enum BAND                                    _start_band                              { BAND_20 };                        ///< on what band do we start?
  enum MODE                                    _start_mode                              { MODE_CW };                        ///< on which mode do we start?
  std::map<std::string /* name */,
             std::pair<std::string /* contents */,
                         std::vector<window_information> > > _static_windows            { };                                ///< size, position and content information for each static window
  bool                                         _sync_keyer                              { false };                          ///< whether to synchronise the rig keyer speed with the computer

  bool                                         _test                                    { false };                          ///< whether to put rig in TEST mode
  char                                         _thousands_separator                     { ',' };                            ///< character used as thousands separator in numbers

  bool                                         _uba_bonus                               { false };                          ///< whether to add UBA bonus QSO points

  std::map<std::string, window_information >   _windows                                 { };                                ///< size and position info for each window
  COLOUR_TYPE                                  _worked_mults_colour                     { COLOUR_RED };                     ///< colour of worked mults in the mult windows

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
  inline drlog_context(void) = default;

/// construct from file
  explicit drlog_context(const std::string& filename);

  drlog_context(const drlog_context&) = delete;         ///< disallow copying

  CONTEXTREAD(accept_colour);                            ///< colour for calls that have been worked, but are not dupes
  CONTEXTREAD(allow_audio_recording);                    ///< whether to allow recording of audio
//  CONTEXTREAD(allow_variable_sent_rst);                  ///< whether to permit the sent RST to vary
  CONTEXTREAD(alternative_exchange_cq);                  ///< alternative exchange in SAP mode
  CONTEXTREAD(alternative_exchange_sap);                 ///< alternative exchange in SAP mode
  CONTEXTREAD(alternative_qsl_message);                  ///< alternative confirmation at end of QSO
  CONTEXTREAD(archive_name);                             ///< name of the archive for save/restore information
  CONTEXTREAD(audio_channels);                           ///< number of audio channels
  CONTEXTREAD(audio_device_name);                        ///< name of audio device
  CONTEXTREAD(audio_duration);                           ///< maximum duration in minutes, per file
  CONTEXTREAD(audio_file);                               ///< base name of file for audio recordings
  CONTEXTREAD(audio_rate);                               ///< number of samples per second
  CONTEXTREAD(autocorrect_rbn);                          ///< whether to try to autocorrect posts from the RBN
  CONTEXTREAD(auto_backup_directory);                    ///< directory for auto backup files
  CONTEXTREAD(auto_cq_mode_ssb);                         ///< do we automatically go to CQ mode on SSB?
  CONTEXTREAD(auto_remaining_callsign_mults);            ///< do we auto-generate the remaining callsign mults?
  CONTEXTREAD(auto_remaining_country_mults);             ///< do we auto-generate the remaining country mults?
  CONTEXTREAD(auto_remaining_callsign_mults_threshold);  ///< number of times a callsign mult must be seen before it becomes known
  CONTEXTREAD(auto_remaining_country_mults_threshold);   ///< number of times a canonical prefix must be seen before it becomes known

/*! \brief              Do we auto-generate remaining mults for a particular exchange mult?
    \param  mult_name   name of the exchange mult
    \return             whether the remaining values of the exchange mult <i>mult_name</i> are auto generated
*/
  inline bool auto_remaining_exchange_mults(const std::string& mult_name) const
    { SAFELOCK(_context);

      return (_auto_remaining_exchange_mults.find(mult_name) != _auto_remaining_exchange_mults.end() );
    }

  CONTEXTREAD(auto_screenshot);                  ///< do we create a screenshot every hour?

  CONTEXTREAD(bandmap_cull_function);            ///< number of the bandmap cull function
  CONTEXTREAD(bandmap_decay_time_local);         ///< time (in minutes) for an entry to age off the bandmap (local entries)
  CONTEXTREAD(bandmap_decay_time_cluster);       ///< time (in minutes) for an entry to age off the bandmap (cluster entries)
  CONTEXTREAD(bandmap_decay_time_rbn);           ///< time (in minutes) for an entry to age off the bandmap (RBN entries)
  CONTEXTREAD(bandmap_fade_colours);             ///< the colours calls adopt as they fade
  CONTEXTREAD(bandmap_filter);                   ///< the strings in the bandmap filter
  CONTEXTREAD(bandmap_filter_disabled_colour);   ///< background colour when bandmap filter is disabled
  CONTEXTREAD(bandmap_filter_enabled);           ///< is the bandmap filter enabled?
  CONTEXTREAD(bandmap_filter_foreground_colour); ///< colour of foreground in the bandmap filter

/// is the bandmap filter set to hide? (If not, then it's set to show)
  inline bool bandmap_filter_hide(void) const
    { return !bandmap_filter_show(); }

  CONTEXTREAD(bandmap_filter_hide_colour);       ///< background colour when bandmap filter is in hide mode
  CONTEXTREAD(bandmap_filter_show);              ///< is the bandmap filter set to show? (If not, then it's set to hide)
  CONTEXTREAD(bandmap_filter_show_colour);       ///< background colour when bandmap filter is in show mode
  CONTEXTREAD(bandmap_frequency_up);             ///< should increasing frequency go upwards in the bandmap?
  CONTEXTREAD(bandmap_recent_colour);            ///< colour for bandmap entries that are less than two minutes old
  CONTEXTREAD(bandmap_show_marked_frequencies);  ///< whether to display entries that would be marked
  CONTEXTREAD(bands);                            ///< comma-delimited list of bands that are legal for the contest
  CONTEXTREAD(batch_messages_file);              ///< file that contains per-call batch messages
  CONTEXTREAD(best_dx_unit);                     ///< name of unit for the BEST DX window ("MILES" or "KM")

  CONTEXTREAD(cabrillo_address_1);               ///< first ADDRESS: line
  CONTEXTREAD(cabrillo_address_2);               ///< second ADDRESS: line
  CONTEXTREAD(cabrillo_address_3);               ///< third ADDRESS: line
  CONTEXTREAD(cabrillo_address_4);               ///< fourth ADDRESS: line
  CONTEXTREAD(cabrillo_address_city);            ///< ADDRESS-CITY:
  CONTEXTREAD(cabrillo_address_state_province);  ///< ADDRESS-STATE-PROVINCE:
  CONTEXTREAD(cabrillo_address_postalcode);      ///< ADDRESS-POSTALCODE:
  CONTEXTREAD(cabrillo_address_country);         ///< ADDRESS-COUNTRY:
  CONTEXTREAD(cabrillo_callsign);                ///< CALLSIGN:
  CONTEXTREAD(cabrillo_category_assisted);       ///< CATEGORY-ASSISTED:
  CONTEXTREAD(cabrillo_category_band);           ///< CATEGORY-BAND:
  CONTEXTREAD(cabrillo_category_mode);           ///< CATEGORY-MODE:
  CONTEXTREAD(cabrillo_category_operator);       ///< CATEGORY-OPERATOR:
  CONTEXTREAD(cabrillo_category_overlay);        ///< CATEGORY-OVERLAY:
  CONTEXTREAD(cabrillo_category_power);          ///< CATEGORY-POWER:
  CONTEXTREAD(cabrillo_category_station);        ///< CATEGORY-STATION:
  CONTEXTREAD(cabrillo_category_time);           ///< CATEGORY-TIME:
  CONTEXTREAD(cabrillo_category_transmitter);    ///< CATEGORY-TRANSMITTER:
  CONTEXTREAD(cabrillo_certificate);             ///< CERTIFICATE:
  CONTEXTREAD(cabrillo_club);                    ///< CLUB:
  CONTEXTREAD(cabrillo_contest);                 ///< CONTEST:
  CONTEXTREAD(cabrillo_eol);                     ///< EOL used in the cabrillo file; one of: "LF", "CR" or "CRLF"
  CONTEXTREAD(cabrillo_e_mail);                  ///< EMAIL: (sic)
  CONTEXTREAD(cabrillo_filename);                ///< name of Cabrillo log
  CONTEXTREAD(cabrillo_include_score);           ///< is the CLAIMED-SCORE line included in the Cabrillo file?
  CONTEXTREAD(cabrillo_location);                ///< LOCATION:
  CONTEXTREAD(cabrillo_name);                    ///< NAME:
  CONTEXTREAD(cabrillo_operators);               ///< OPERATORS:
  CONTEXTREAD(cabrillo_qso_template);            ///< format for Cabrillo QSOs

  CONTEXTREAD(call_history_bands);               ///< bands to show in CALL HISTORY window
  CONTEXTREAD(call_ok_now_message);              ///< message if call was changed
  CONTEXTREAD(callsign_mults);                   ///< mults derived from callsign; e.g., WPXPX
  CONTEXTREAD(callsign_mults_per_band);          ///< are callsign mults per-band?
  CONTEXTREAD(callsign_mults_per_mode);          ///< are callsign mults per-mode?
  CONTEXTREAD(cluster_port);                     ///< port on the cluster server
  CONTEXTREAD(cluster_server);                   ///< hostname or IP of cluster server
  CONTEXTREAD(cluster_username);                 ///< username to use on the cluster
  CONTEXTREAD(contest_name);                     ///< name of the contest
  CONTEXTREAD(country_list);                     ///< DXCC or WAE list?
  CONTEXTREAD(country_mults_filter);             ///< the command from the configuration file
  CONTEXTREAD(country_mults_per_band);           ///< are country mults per-band?
  CONTEXTREAD(country_mults_per_mode);           ///< are country mults per-mode?
  CONTEXTREAD(cq_auto_lock);                     ///< whether to lock the transmitter in CQ mode
  CONTEXTREAD(cq_auto_rit);                      ///< whether to enable RIT in CQ mode
  CONTEXTREAD(cty_filename);                     ///< filename of country file (default = "cty.dat")
  CONTEXTREAD(cw_bandwidth_narrow);              ///< narrow CW bandwidth (Hz)
  CONTEXTREAD(cw_bandwidth_wide);                ///< wide CW bandwidth (Hz)
  CONTEXTREAD(cw_priority);                      ///< priority of CW thread (-1 = non-RT; 0 = middle RT; otherwise priority number)
  CONTEXTREAD(cw_speed);                         ///< speed in WPM
  CONTEXTREAD(cw_speed_change);                  ///< change in CW speed in WPM when pressing PAGE UP or PAGE DOWN

  CONTEXTREAD(decimal_point);                    ///< character to use as decimal point
  CONTEXTREAD(display_communication_errors);     ///< whether to display errors communicating with rig
  CONTEXTREAD(display_grid);                     ///< whether grid will be shown in GRID and INFO windows
  CONTEXTREAD(do_not_show);                      ///< do not show these calls when spotted (MY CALL is automatically not shown)
  CONTEXTREAD(do_not_show_filename);             ///< filename of calls (one per line) not to be shown
  CONTEXTREAD(drmaster_filename);                ///< filename of drmaster file (default = "drmaster")

  CONTEXTREAD(exchange);                         ///< comma-delimited received exchange
  CONTEXTREAD(exchange_cq);                      ///< exchange in CQ mode
  CONTEXTREAD(exchange_fields_filename);         ///< file that holds regex templates of values of exchange fields
  CONTEXTREAD(exchange_mults);                   ///< comma-delimited exchange fields that are mults
  CONTEXTREAD(exchange_mults_per_band);          ///< are exchange mults per-band?
  CONTEXTREAD(exchange_mults_per_mode);          ///< are exchange mults per-mode?
  CONTEXTREAD(exchange_per_country);             ///< per-country exchanges; key = prefix-or-call; value = exchange
  CONTEXTREAD(exchange_prefill_files);           ///< external prefill files for exchange fields

  CONTEXTREAD(exchange_sap);                     ///< exchange in SAP mode

  CONTEXTREAD(fast_cq_bandwidth);                ///< fast CW bandwidth in CQ mode, in Hz
  CONTEXTREAD(fast_sap_bandwidth);               ///< fast CW bandwidth in SAP mode, in Hz

  CONTEXTREAD(geomagnetic_indices_command);      ///< command to get geomagnetic indices


/*! \brief      Get the guard band for a particular mode
    \param  m   target mode
    \return     guard band for mode <i>m</i>, in Hz
*/
  inline unsigned int guard_band(const MODE m)
  { SAFELOCK(_context);

    return MUM_VALUE(_guard_band, m, 1000);
  }

  CONTEXTREAD(home_exchange_window);         ///< whether to move cursor to left of exchange window (and insert space if necessary)

  CONTEXTREAD(inactivity_timer);             ///< duration in seconds before audio recording ceases in auto mode
  CONTEXTREAD(individual_messages_file);     ///< name of file that contains per-call individual messages

  CONTEXTREAD(keyer_port);                   ///< the device that is to be used as a keyer

  CONTEXTREAD(logfile);                      ///< name of the log file
  CONTEXTREAD(long_t);                       ///< whether to extend length of initial Ts in serial number

  CONTEXTREAD(mark_frequencies);             ///< frequency ranges to be marked on-screen
  CONTEXTREAD(mark_mode_break_points);       ///< whether to mark the mode break points on the bandmap
  CONTEXTREAD(match_minimum);                ///< number of characters before SCP or fuzzy match kicks in
  CONTEXTREAD(max_qsos_without_qsl);         ///< cutoff for the N7DR matches_criteria() algorithm
  CONTEXTREAD(messages);                     ///< CW messages
  CONTEXTREAD(message_cq_1);                 ///< CQ message #1 (generally, a short CQ)
  CONTEXTREAD(message_cq_2);                 ///< CQ message #2 (generally, a long CQ)
  CONTEXTREAD(mm_country_mults);             ///< can /MM QSOs be mults?
  CONTEXTREAD(modes);                        ///< comma-delimited modes CW, SSB
  CONTEXTREAD(mode_break_points);            ///< override default mode break points
  CONTEXTREAD(my_call);                      ///< my call
  CONTEXTREAD(my_continent);                 ///< my continent
  CONTEXTREAD(my_cq_zone);                   ///< my CQ zone
  CONTEXTREAD(my_grid);                      ///< my grid square identifier
  CONTEXTREAD(my_ip);                        ///< my IP address
  CONTEXTREAD(my_itu_zone);                  ///< my ITU zone
  CONTEXTREAD(my_latitude);                  ///< my latitude in degrees (north +ve)
  CONTEXTREAD(my_longitude);                 ///< my longitude in degrees (east +ve)

  CONTEXTREAD(nearby_extract);               ///< whether to display NEARBY calls in EXTRACT window
  CONTEXTREAD(normalise_rate);               ///< whether to display rates as per-hour
  CONTEXTREAD(not_country_mults);            ///< comma-separated list of countries that are explicitly NOT country mults
  CONTEXTREAD(no_default_rst);               ///< is there no default assignment for received RST ?
  CONTEXTREAD(n_memories);                   ///< number of rig memories

  CONTEXTREAD(old_adif_log_name);            ///< name of ADIF file that contains old QSOs
  CONTEXTREAD(old_qso_age_limit);            ///< include old QSOs newer than this value (in years) [0 => all no limit]  

  CONTEXTREAD(path);                         ///< directories to search, in order
  CONTEXTREAD(per_band_country_mult_factor); ///< country mult factor structure for each band
  CONTEXTREAD(per_band_points);              ///< points structure for each band and mode

//  std::map<std::string /* exchange field */, decltype(_per_band_points) > _per_band_points_with_exchange_field;              ///< points structure for each band and mode, if a particular exchange field is present

  CONTEXTREAD(post_monitor_calls);           ///< calls to be monitored
  CONTEXTREAD(posted_by_continents);         ///< continents for POSTED BY window (empty => all DX continents)
  CONTEXTREAD(ptt_delay);                    ///< PTT delay in milliseconds ( 0 => PTT disabled)
  CONTEXTREAD(p3);                           ///< is a P3 available?
  CONTEXTREAD(p3_ignore_checksum_error);     ///< should checksum errors be ignored when acquiring P3 screendumps?
  CONTEXTREAD(p3_snapshot_file);             ///< base name of file for P3 snapshot
  CONTEXTREAD(p3_span_cq);                   ///< P3 span in CQ mode, in kHz
  CONTEXTREAD(p3_span_sap);                  ///< P3 span in SAP mode, in kHz

  CONTEXTREAD(qsl_message);                  ///< confirm at end of QSO
  CONTEXTREAD(qso_multiple_bands);           ///< whether OK to work station on another band
  CONTEXTREAD(qso_multiple_modes);           ///< whether OK to work station on another mode
  CONTEXTREAD(qsy_on_startup);               ///< whether to go to START BAND on startup
  CONTEXTREAD(qtcs);                         ///< whether QTCs are enabled
  CONTEXTREAD(qtc_double_space);             ///< whether to leave a longer pause between elements of a QTC
  CONTEXTREAD(qtc_filename);                 ///< name of file where QTCs are stored
  CONTEXTREAD(qtc_long_t);                   ///< whether and amount to extend length of initial Ts in serial number in QTCs
  CONTEXTREAD(qtc_qrs);                      ///< WPM decrease when sending QTC
  CONTEXTREAD(qthx);                         ///< allowed exchanges values as a function of country

  CONTEXTREAD(rate_periods);                     ///< periods (in minutes) over which rates should be calculated
  CONTEXTREAD(rbn_beacons);                      ///< whether to place RBN posts from beacons on the bandmap
  CONTEXTREAD(rbn_port);                         ///< port number on the RBN server
  CONTEXTREAD(rbn_server);                       ///< hostname or IP address of RBN server
  CONTEXTREAD(rbn_threshold);                    ///< number of different stations that have to post a station to the RBN before it shows on the bandmap
  CONTEXTREAD(rbn_username);                     ///< username to use on the RBN server
  CONTEXTREAD(reject_colour);                    ///< colour for calls that are dupes
  CONTEXTREAD(remaining_callsign_mults_list);    ///< callsign mults to display
  CONTEXTREAD(remaining_country_mults_list);     ///< country mults to display
  CONTEXTREAD(require_dot_in_replacement_call);  ///< whether to require a dot in a replacement call in the EXCHANGE window
  CONTEXTREAD(rig1_baud);                        ///< baud rate for rig
  CONTEXTREAD(rig1_data_bits);                   ///< number of data bits for rig
  CONTEXTREAD(rig1_name);                        ///< name of rig
  CONTEXTREAD(rig1_port);                        ///< port over which to communicate with rig
  CONTEXTREAD(rig1_stop_bits);                   ///< number of stop bits for rig
  CONTEXTREAD(rig1_type);                        ///< model name of rig
  CONTEXTREAD(russian_filename);                 ///< filename of russian location file (default = "russian-data")

  CONTEXTREAD(score_bands);                      ///< which bands are going to be scored?
  CONTEXTREAD(score_modes);                      ///< which modes are going to be scored?
  CONTEXTREAD(scoring_enabled);                  ///< is scoring enabled?
  CONTEXTREAD(screen_snapshot_file);             ///< base name of file for screenshot
  CONTEXTREAD(screen_snapshot_on_exit);          ///< whether to take a screenshot on exit

/*! \brief      Get names and values of sent exchange fields for a particular mode
    \param  m   target mode
    \return     the names and values of all the fields in the sent exchange when the mode is <i>m</i>
*/
  decltype(_sent_exchange) sent_exchange(const MODE m);        // doxygen complains about the decltype; I have no idea why

  CONTEXTREAD(sent_exchange_cw);                 ///< names and values of sent exchange fields, CW
  CONTEXTREAD(sent_exchange_ssb);                ///< names and values of sent exchange fields, SSB
  CONTEXTREAD(serno_spaces);                     ///< number of half-length spaces
  CONTEXTREAD(shift_delta_cw);                   ///< how many hertz to QSY per poll of the shift key on CW
  CONTEXTREAD(shift_delta_ssb);                  ///< how many hertz to QSY per poll of the shift key on SSB
  CONTEXTREAD(shift_poll);                       ///< how frequently is the shift key polled during an RIT QSY, in milliseconds
  CONTEXTREAD(short_serno);                      ///< whether to omit leading Ts
  CONTEXTREAD(society_list_filename);            ///< name of file containing IARU society exchanges
  CONTEXTREAD(start_audio_recording);            ///< whether to start recording of audio (if _allow_audio_recording is true)
  CONTEXTREAD(start_band);                       ///< on what band do we start?
  CONTEXTREAD(start_mode);                       ///< on which mode do we start?
  CONTEXTREAD(static_windows);                   ///< size, position and content information for each static window
  CONTEXTREAD(sync_keyer);                       ///< whether to synchronise the rig keyer speed with the computer

  CONTEXTREAD(test);                             ///< whether to put rig in TEST mode
  CONTEXTREAD(thousands_separator);              ///< character used as thousands separator in numbers

  CONTEXTREAD(uba_bonus);                        ///< whether to add UBA bonus QSO points

  CONTEXTREAD(worked_mults_colour);              ///< colour of worked mults in the mult windows

/*! \brief      Get the points string for a particular band and mode
    \param  b   band
    \param  m   mode
    \return     the points string corresponding to band <i>b</i> and mode <i>m</i>
*/
  std::string points_string(const BAND b, const MODE m) const;

#if 0
/*! \brief                  Get the points string for a particular band and mode, if a particular exchange field is present
    \param  exchange_field  exchange field
    \param  b               band
    \param  m               mode
    \return                 the points string corresponding to band <i>b</i> and mode <i>m</i> when exchange field <i>exchange_field</i> is present
*/
//  const std::string points(const std::string& exchange_field, const BAND b, const MODE m) const;
#endif

/*! \brief          Get the information pertaining to a particular window
    \param  name    name of window
    \return         location, size and colour information
*/
  inline window_information window_info(const std::string& name) const
    { return MUM_VALUE(_windows, name); }

/*! \brief          Is a particular window defined
    \param  name    name of window
    \return         whether the window <i>name</i> is defined in the configuration file
*/
  inline bool window_defined(const std::string& name) const
    { return window_info(name).defined(); } 

/*! \brief          Get a vector of the names of the legal bands for the contest (e.g., "160", "80", etc.)
    \return         the bands that are legal for the context
*/
  inline std::vector<std::string> band_names(void) const
  { SAFELOCK(_context);
    return split_string(_bands, ',');
  }

/*! \brief          Get a vector of the names of the legal modes for the contest (e.g., "CW", "SSB", etc.)
    \return         the modes that are legal for the context
*/
  inline std::vector<std::string> mode_names(void) const
  { SAFELOCK(_context);
    return split_string(_modes, ',');
  }

/// how many bands are used in this contest?
  inline unsigned int n_bands(void) const
  { SAFELOCK(_context);
    return band_names().size();
  }

/// how many modes are used in this contest?
  inline unsigned int n_modes(void) const
  { SAFELOCK(_context);
    return mode_names().size();
  }

/*! \brief          All the windows whose name contains a particular substring
    \param  substr  substring for which to search
    \return         all the window names that include <i>substr</i>
*/
  std::vector<std::string> window_name_contains(const std::string& substr) const;

/*! \brief      Is a particular frequency within any marked range?
    \param  m   mode
    \param  f   frequency to test
    \return     whether <i>f</i> is in any marked range for the mode <i>m</i>
*/
  bool mark_frequency(const MODE, const frequency& f);

/*! \brief      Get all the field names in the sent exchange
    \return     the names of all the fields in the sent exchange
*/
  std::vector<std::string> sent_exchange_names(void) const;

/*! \brief      Get all the field names in the exchange sent for a particular mode
    \param  m   target mode
    \return     the names of all the fields in the sent exchange for mode <i>m</i>
*/
  std::vector<std::string> sent_exchange_names(const MODE m) const;

/// swap QSL and ALTERNATIVE QSL messages
  inline void swap_qsl_messages(void)
  { SAFELOCK(_context);
    swap(_qsl_message, _alternative_qsl_message);
  }

/// are multiple modes permitted?
  inline bool multiple_modes(void) const
  { SAFELOCK(_context);
    return (_modes.size() != 1);
  }

/*! \brief      Change the amount of QRS associated with sending a QTC
    \param  n   the amount, in WPM, to decrease the CW speed while sending a QTC
*/
  inline void qtc_qrs(const unsigned int n)
  { SAFELOCK(_context);
    _qtc_qrs = n;
  }
};

#endif    // DRLOG_CONTEXT_H
