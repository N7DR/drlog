// $Id: drlog.cpp 273 2025-07-27 13:22:36Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

/*! \file   drlog.cpp

    The main program for drlog
*/

#include "adif3.h"
#include "audio.h"
#include "autocorrect.h"
#include "bandmap.h"
#include "bands-modes.h"
#include "cluster.h"
#include "command_line.h"
#include "cty_data.h"
#include "cw_buffer.h"
#include "diskfile.h"
#include "drlog_context.h"
#include "exchange.h"
#include "functions.h"
#include "fuzzy.h"
#include "grid.h"
#include "internals.h"
#include "keyboard.h"
#include "log.h"
#include "log_message.h"
#include "memory.h"
#include "parallel_port.h"
#include "procfs.h"
#include "query.h"
#include "qso.h"
#include "qtc.h"
#include "rate.h"
#include "rig_interface.h"
#include "rules.h"
#include "scp.h"
#include "screen.h"
#include "serialization.h"
#include "socket_support.h"
#include "statistics.h"
#include "string_functions.h"
#include "time_log.h"
#include "trlog.h"
#include "version.h"

//import <chrono>;  // not yet supported

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include <future>

#include <cstdlib>

#include <png++/png.hpp>

//import std;  // not yet supported

using namespace std;
using namespace   chrono;        // std::chrono; NB g++10 library does not yet implement utc_clock
using namespace   this_thread;   // std::this_thread

using PING_TABLE_ELEMENT = pair<string, icmp_socket*>;    // label, pointer to ICMP socket
using PING_TABLE         = vector<PING_TABLE_ELEMENT>;

extern const STRING_SET CONTINENT_SET;     ///< two-letter abbreviations of continents

/// active window
enum class ACTIVE_WINDOW { CALL,
                           EXCHANGE,
                           LOG,             // last five QSOs          
                           LOG_EXTRACT      // used for QTCs
                         };

/// drlog mode
enum class DRLOG_MODE { CQ,         ///< I'm calling the other station
                        SAP         ///< the other station is calling me
                      };

/// whether to force a callsign or country mult as known; used in update_known_callsign/callsign_mults()
enum class KNOWN_MULT { FORCE_KNOWN,
                        NO_FORCE_KNOWN
                      };

// needed for WRAPPER_3 definition of memory_entry
ostream& operator<<(ostream& ost, const DRLOG_MODE& dm)
{ ost << ( dm == DRLOG_MODE::CQ ? 'C' : 'S');

  return ost;
}

constexpr string OUTPUT_FILENAME { "output.txt"s };    ///< file to which debugging output is directed

message_stream ost { OUTPUT_FILENAME };                ///< message stream for debugging output

screen monitor;                             ///< the ncurses screen;  declare at global scope solely so that its destructor is called when exit() is executed;
                                            // declare early so that it is ready when any of the colour functions are called

string VERSION;         ///< version string
string DP("·"s);        ///< character for decimal point            // things get too painful if we use a wchar_t

char TS { ',' };        ///< character for thousands separator

static const STRING_SET variable_exchange_fields { "SERNO"s };  ///< mutable exchange fields

constexpr bool DISPLAY_EXTRACT        { true },                       ///< display log extracts
               DO_NOT_DISPLAY_EXTRACT { !DISPLAY_EXTRACT };           ///< do not display log extracts

constexpr int  MILLION                { 1'000'000 };                  // syntactic sugar

// define class for memory entries
WRAPPER_3(memory_entry,
            frequency, freq,
            MODE, mode,
            DRLOG_MODE, drlog_mode);   ///< parameters for bandmap

deque<memory_entry> memories;

// some forward declarations; others, that depend on these, occur later

string active_window_name(void);                                                ///< Return the name of the active window in printable form
void   add_qso(const QSO& qso);                                                 ///< Add a QSO into the all the objects that need to know about it
void   adif3_build_old_log(void);                                               ///< build the old log from an ADIF3 file
//void   alert(const string& msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);   ///< Alert the user
void   alert(const string_view msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);   ///< Alert the user
void   allow_for_callsign_mults(QSO& qso);                                      ///< Add info to QSO if callsign mults are in use; may change qso
QSO    allow_for_callsign_mults(QSO&& qso);
void   archive_data(void);                                                      ///< Send data to the archive file
//void   audio_error_alert(const string& msg);                                    ///< Alert the user to an audio-related error
void   audio_error_alert(const string_view msg);                                    ///< Alert the user to an audio-related error

string bearing(const string_view callsign);   ///< Return the bearing to a station

bool        calculate_exchange_mults(QSO& qso, const contest_rules& rules);                    ///< Populate QSO with correct exchange mults
STRING_SET  calls_from_do_not_show_file(const BAND b);                                         ///< get the calls from a DO NOT SHOW file
void        calls_to_do_not_show_file(const STRING_SET& callsigns, const BAND b);              ///< send calls to a DO NOT SHOW file
//string      callsign_mult_value(const string& callsign_mult_name, const string& callsign);     ///< Obtain value corresponding to a type of callsign mult from a callsign
//string      callsign_mult_value(const string_view callsign_mult_name, const string& callsign);     ///< Obtain value corresponding to a type of callsign mult from a callsign
string      callsign_mult_value(const string_view callsign_mult_name, const string_view callsign);     ///< Obtain value corresponding to a type of callsign mult from a callsign
bool        change_cw_speed(const keyboard_event& e);                                          ///< change CW speed as function of keyboard event
void        cw_speed(const unsigned int new_speed);                                            ///< Set speed of computer keyer
bool        cw_toggle_bandwidth(void);                                                         ///< Toggle 50Hz/200Hz bandwidth if on CW

bool   debug_dump(void);                                                                            ///< Dump useful information to disk
//MODE   default_mode(const frequency& f);                                                            ///< get the default mode on a frequency
MODE   default_mode(const frequency f);                                                             ///< get the default mode on a frequency
void   display_bandmap_filter(bandmap& bm);                                                         ///< display the bandmap cull/filter information in win_bandmap_filter
void   display_band_mode(window& win, const BAND current_band, const enum MODE current_mode);       ///< Display band and mode
//void   display_call_info(const string& callsign, const bool display_extract = DISPLAY_EXTRACT);     ///< Update several call-related windows
void   display_call_info(const string_view callsign, const bool display_extract = DISPLAY_EXTRACT);     ///< Update several call-related windows
void   display_memories(void);                                                                      ///< Update MEMORIES window
//void   display_nearby_callsign(const string& callsign);                                             ///< Display a callsign in the NEARBY window, in the correct colour
void   display_nearby_callsign(const string_view callsign);                                             ///< Display a callsign in the NEARBY window, in the correct colour
//void   display_statistics(const string& summary_str);                                               ///< Display the current statistics
void   display_statistics(const string_view summary_str);                                               ///< Display the current statistics
void   do_not_show(const string_view callsign, const BAND b = ALL_BANDS);                           ///< Mark a callsign as not to be shown
//string dump_screen(const string& filename = string());                                              ///< Dump a screen image to PNG file
string dump_screen(const string_view filename = string { });                                              ///< Dump a screen image to PNG file

//void   end_of_thread(const string& name);
void   end_of_thread(const string_view name);
void   enter_cq_mode(void);                                     ///< Enter CQ mode
void   enter_cq_or_sap_mode(const DRLOG_MODE new_mode);         ///< enter CQ or SAP mode
void   enter_sap_mode(void);                                    ///< Enter SAP mode
void   exit_drlog(void);                                        ///< Cleanup and exit
string expand_cw_message(const string_view msg);                ///< Expand a CW message, replacing special characters
string expected_received_exchange(const string_view callsign);  ///< expected exchange field name

bool fast_cw_bandwidth(void);                           ///< set CW bandwidth to appropriate value for CQ/SAP mode

string hhmmss(void);                                    ///< Obtain the current time in HH:MM:SS format

void insert_memory(void);                                                                               ///< insert an entry into the memories
bool is_daylight(const string_view sunrise_time, const string_view sunset_time, const string_view current_time);    ///< is it currently daylight?
bool is_marked_frequency(const map<MODE, vector<pair<frequency, frequency>>>& marked_frequency_ranges, const MODE m, const frequency f); ///< Is a particular frequency within any marked range?
//bool is_needed_qso(const string& callsign, const BAND b, const MODE m);                                 ///<   Is a callsign needed on a particular band and mode?
bool is_needed_qso(const string_view callsign, const BAND b, const MODE m);                                 ///<   Is a callsign needed on a particular band and mode?

pair<float, float> latitude_and_longitude(const string_view callsign);    ///< obtain latitude and longtide associated with a call

string match_callsign(const vector<pair<string /* callsign */, PAIR_NUMBER_TYPE /* colour pair number */ > >& matches, const string_view do_not_return = EMPTY_STR);   ///< Get best fuzzy or SCP match

MINUTES_TYPE NOW_MINUTES(void);                                                         ///< time in absolute minutes

void populate_win_call_history(const string_view str);                                      ///< Populate the QSO/QSL call history window
void populate_win_info(const string_view str);                                              ///< Populate the information window
void possible_mode_change(const frequency f);                                              ///< possibly change mode in accordance with frequency
void print_thread_names(void);                                                              ///< output the names of the currently active threads
bool process_bandmap_function(BANDMAP_MEM_FUN_P fn_p, const BANDMAP_DIRECTION dirn, const int16_t nskip = 0);    ///< process a bandmap function, to jump to the next frequency returned by the function
bool process_bandmap_function(const BANDMAP_DIRECTION dirn, const int16_t nskip = 0);
bool process_change_in_bandmap_column_offset(const KeySym symbol);                      ///< change the offset of the bandmap
bool process_backspace(window& win);                                                    ///< process backspace
bool process_keypress_F1(const string_view original_contents);                          ///< process key F1
bool process_keypress_F2(void);                                                         ///< process key F2
bool process_keypress_F4(void);                                                         ///< process key F4
bool process_keypress_F5(void);                                                         ///< process key F5
bool p3_screenshot(void);                                                               ///< Start a thread to take a snapshot of a P3
void p3_span(const unsigned int khz_span);                                              ///< set the span of a P3

void rebuild_dynamic_call_databases(const logbook& logbk);                              ///< rebuild dynamic portions of SCP, fuzzy and query databases
void rebuild_history(const logbook& logbk,
                     const contest_rules& rules,
                     running_statistics& statistics,
                     call_history& q_history,
                     rate_meter& rate);             ///< Rebuild the history (and statistics and rate and greatest distance), using the logbook
memory_entry recall_memory(const unsigned int n);   ///< recall a memory
void rescore(const contest_rules& rules);           ///< Rescore the entire contest
void restore_data(const string_view archive_filename);  ///< Extract the data from the archive file
void rig_error_alert(const string_view msg);            ///< Alert the user to a rig-related error
//string run_external_command(const string& cmd);     ///< run an external command
string run_external_command(const string_view cmd);     ///< run an external command

void   send_qtc_entry(const qtc_entry& qe, const bool log_it);                  ///< send a single QTC entry (on CW)
//bool   send_to_scratchpad(const string& str);                                   ///< Send a string to the SCRATCHPAD window
bool   send_to_scratchpad(const string_view str);                                   ///< Send a string to the SCRATCHPAD window
void   set_active_window(const ACTIVE_WINDOW aw);                               ///< Set the window that is receiving input
bool   shift_control(const keyboard_event& e);                                  ///< Control RIT or main QRG using the SHIFT keys
bool   ssb_toggle_bandwidth(void);                                              ///< Toggle bandwidth and centre frequency if on SSB
void   start_recording(audio_recorder& audio, const drlog_context& context);    ///< start audio recording
//void   start_of_thread(const string& name);                                     ///< Increase the counter for the number of running threads
void   start_of_thread(const string_view name);                                     ///< Increase the counter for the number of running threads
void   stop_recording(audio_recorder& audio);                                   ///< stop audio recording
string sunrise_or_sunset(const string_view callsign, const SRSS srss);              ///< Calculate the sunrise or sunset time for a station
bool   swap_rit_xit(void);                                                      ///< Swap the states of RIT and XIT

char t_char(const unsigned short long_t);                                           ///< the character used to represent a leading T in a servo
//void test_exchange_templates(const contest_rules&, const string& test_filename);    ///< Debug exchange templates
void test_exchange_templates(const contest_rules&, const string_view test_filename);    ///< Debug exchange templates
int  time_since_last_qso(const logbook& logbk);                                     ///< time in seconds since the last QSO
int  time_since_last_qsy(void);                                                     ///< time in seconds since the last QSY
bool toggle_drlog_mode(void);                                                       ///< Toggle between CQ mode and SAP mode
bool toggle_cw(void);                                                               ///< Toggle CW on/off
bool toggle_recording_status(audio_recorder& audio);                                ///< toggle status of audio recording

void update_bandmap_size_window(void);                                                                                  ///< update the BANDMAP SIZE window
void update_bandmap_window(bandmap& bm);                                                                                ///< update the BANDMAP window
void update_based_on_frequency_change(const frequency f, const MODE m);                                                 ///< Update some windows based on a change in my frequency
//void update_batch_messages_window(const string& callsign = string());                                                   ///< Update the batch_messages window with the message (if any) associated with a call
void update_batch_messages_window(const string_view callsign = string { });                                                   ///< Update the batch_messages window with the message (if any) associated with a call
//void update_best_dx(const grid_square& dx_gs, const string& callsign);                                                  ///< Update bext DX window, if it exists
void update_best_dx(const grid_square& dx_gs, const string_view callsign);                                                  ///< Update bext DX window, if it exists
//void update_individual_messages_window(const string& callsign = string());                                              ///< Update the individual_messages window with the message (if any) associated with a call
void update_individual_messages_window(const string_view callsign = string { });                                              ///< Update the individual_messages window with the message (if any) associated with a call
void update_known_callsign_mults(const string_view callsign, const KNOWN_MULT force_known = KNOWN_MULT::NO_FORCE_KNOWN);///< Possibly add a new callsign mult
//bool update_known_country_mults(const string& callsign, const KNOWN_MULT force_known = KNOWN_MULT::NO_FORCE_KNOWN);   ///< Possibly add a new country to the known country mults
bool update_known_country_mults(const string_view callsign, const KNOWN_MULT force_known = KNOWN_MULT::NO_FORCE_KNOWN); ///< Possibly add a new country to the known country mults
void update_local_time(void);                                                                                           ///< Write the current local time to <i>win_local_time</i>
void update_mult_value(void);                                                                                           ///< Calculate the value of a mult and update <i>win_mult_value</i>
void update_pings(window& win, PING_TABLE& table);                                                                      ///< execute pings and update PING window
//void update_query_windows(const string& callsign);                                                                      ///< Update the Q1 and QN windows
void update_query_windows(const string_view callsign);                                                                      ///< Update the Q1 and QN windows
void update_quick_qsy(void);                                                                                            ///< update value of <i>quick_qsy_info</i> and <i>win_quick_qsy</i>
//void update_qsls_window(const string& = EMPTY_STR);                                                                     ///< QSL information from old QSOs
void update_qsls_window(const string_view = string { });                                                                     ///< QSL information from old QSOs
void update_qtc_queue_window(void);                                                                                     ///< the head of the QTC queue
void update_rate_window(void);                                                                                          ///< Update the QSO and score values in <i>win_rate</i>
void update_recording_status_window(void);                                                                              ///< update the RECORDING STATUS window
//void update_remaining_callsign_mults_window(running_statistics&, const string& mult_name, const BAND b, const MODE m);  ///< Update the REMAINING CALLSIGN MULTS window for a particular mult
void update_remaining_callsign_mults_window(running_statistics&, const string_view mult_name, const BAND b, const MODE m);  ///< Update the REMAINING CALLSIGN MULTS window for a particular mult
void update_remaining_country_mults_window(running_statistics&, const BAND b, const MODE m);                            ///< Update the REMAINING COUNTRY MULTS window
//void update_remaining_exch_mults_window(const string& mult_name, /* const contest_rules& rules, */
//                                        running_statistics& statistics, const BAND b, const MODE m);                    ///< Update the REMAINING EXCHANGE MULTS window for a particular mult
void update_remaining_exch_mults_window(const string_view mult_name, running_statistics& statistics, const BAND b, const MODE m);                    ///< Update the REMAINING EXCHANGE MULTS window for a particular mult
void update_remaining_exchange_mults_windows(running_statistics&, const BAND b, const MODE m);    ///< Update the REMAINING EXCHANGE MULTS windows for all exchange mults with windows
bool update_rx_ant_window(void);                                                                                        ///< get the status of the RX ant, and update <i>win_rx_ant</i> appropriately
void update_score_window(const unsigned int score);                                                                     ///< update the SCORE window
void update_system_memory(void);                                                                                        ///< update the SYSTEM MEMORY window
void update_win_posted_by(const vector<dx_post>&);                                                                      ///< update, but do not refresh, the POSTED BY window

//bool xscp_order_greater(const string& c1, const string& c2);                                                            ///< is <i>c1</i> before <i>c2</i> in XSCP order?
bool xscp_order_greater(const string_view c1, const string_view c2);                                                            ///< is <i>c1</i> before <i>c2</i> in XSCP order?

bool zoomed_xit(void);                                                                                                  ///< zoom P3 and turn off RIT, turn on XIT

// functions for processing input to windows
void process_CALL_input(window* wp, const keyboard_event& e);               ///< Process an event in CALL window
void process_EXCHANGE_input(window* wp, const keyboard_event& e);           ///< Process an event in EXCHANGE window
void process_LOG_input(window* wp, const keyboard_event& e);                ///< Process an event in LOG window
void process_QTC_input(window* wp, const keyboard_event& e);                ///< Process an event in QTC window

using BANDMAPS = array<bandmap, NUMBER_OF_BANDS>;

// thread functions -- don't use string_views here because the underlying string might be deleted before it is used in the thread
void auto_backup(const string dir, const string log_filename, const string qtc_filename);   ///< Copy a file to a backup directory
void auto_screenshot(const string filename);                                                ///< Write a screenshot to a file
void display_rig_status(const milliseconds poll_time, rig_interface* rigp);                 ///< Display status of the rig
void display_date_and_time(void);                                                           ///< Thread function to display the date and time
void get_cluster_info(dx_cluster* cluster_p);                                               ///< Thread function to obtain data from the cluster
void get_indices(const string cmd);                                                         ///< Get SFI, A, K
void keyboard_test(void);                                                                   ///< Thread function to simulate keystrokes
void process_rbn_info(window* wclp, window* wcmp, dx_cluster* dcp, running_statistics* statistics_p,
                      location_database* location_database_p, window* win_bandmap_p, BANDMAPS* bandmaps_p);       ///< Thread function to process data from the cluster or the RBN
void prune_bandmap(window* wp, array<bandmap, NUMBER_OF_BANDS>* bandmaps);                                        ///< Thread function to prune the bandmaps once per minute
void p3_screenshot_thread(void);                                                            ///< Thread function to generate a screenshot of a P3 and store it in a BMP file
void reset_connection(dx_cluster* rbn_p);                                                   ///< Thread function to reset the RBN or cluster connection
void simulator_thread(string, int);                                                         ///< Thread function to simulate a contest from an extant log
void spawn_dx_cluster(void);                                                                ///< Thread function to spawn the cluster
void spawn_rbn(void);                                                                       ///< Thread function to spawn the RBN

// values that are used by multiple threads
// mostly these are essentially RO, so locking is overkill; but we do it anyway,
// otherwise Murphy dictates that we'll hit a race condition at the worst possible time

pt_mutex alert_mutex { "USER ALERT"s };     ///< mutex for the user alert
time_t   alert_time  { 0 };                 ///< time of last alert

pt_mutex                     batch_messages_mutex { "BATCH MESSAGES"s };   ///< mutex for batch messages
UNORDERED_STRING_MAP<string> batch_messages;         ///< batch messages associated with calls

atomic<frequency> cq_mode_frequency;

pt_mutex dupe_check_mutex { "DUPE CHECK"s };                  ///< mutex for <i>last_call_inserted_with_space</i>
string   last_call_inserted_with_space;     ///< call inserted in bandmap by hitting the space bar; probably should be per band; can't be ataomic as string is not trivially copyable

pt_mutex                     individual_messages_mutex { "INDIVIDUAL MESSAGES"s };   ///< mutex for individual messages
UNORDERED_STRING_MAP<string> individual_messages;                                     ///< individual messages associated with calls

pt_mutex  last_exchange_mutex { "LAST EXCHANGE"s };              ///< mutex for getting and setting the last sent exchange
string    last_exchange;                    ///< the last sent exchange

COLOUR_TYPE log_extract_fg;                 ///< foreground colour of LOG EXTRACT window (used to restore colours after QTC)
COLOUR_TYPE log_extract_bg;                 ///< background colour of LOG EXTRACT window (used to restore colours after QTC)

pt_mutex  my_bandmap_entry_mutex { "BANDMAP ENTRY"s };          ///< mutex for changing frequency or bandmap info
time_t    time_last_qsy          { time_t(NULL) };      ///< time of last QSY

pt_mutex            thread_check_mutex { "THREAD CHECK"s };   ///< mutex for controlling threads; both the following variables are under this mutex
int                 n_running_threads { 0 };                  ///< how many additional threads are running?
atomic<bool>        exiting { false };                        ///< is the program exiting?
bool                exiting_rig_status { false };             ///< turn off the display-rig_status thread first
STRING_SET          thread_names;                             ///< the names of the threads

bool                auto_remaining_country_mults { false }; ///< whether country mults are to be automatically added (value read from context before use)

atomic<BAND>        current_band;                           ///< the current band
atomic<MODE>        current_mode;                           ///< the current mode

pt_mutex            drlog_mode_mutex { "DRLOG_MODE"s };     ///< mutex for accessing <i>drlog_mode</i>
DRLOG_MODE          drlog_mode { DRLOG_MODE::SAP };         ///< CQ or SAP
DRLOG_MODE          a_drlog_mode;                           ///< used when SO1R

pt_mutex            known_callsign_mults_mutex { "KNOWN CALLSIGN MULTS"s };     ///< mutex for the callsign mults we know about in AUTO mode
STRING_SET          known_callsign_mults;                                       ///< callsign mults we know about in AUTO mode

pt_mutex            last_polled_frequency_mutex { "LAST POLLED FREQUENCY"s }; ///< mutex for accessing <i>drlog_mode</i>
string              last_polled_frequency { };                                ///< frequency string from the most recent poll of the rig

pt_mutex            wicm_mutex {"WICM" };
deque<string>       wicm_calls;                         ///< calls in the WICM window

map<MODE, vector<pair<frequency, frequency>>> marked_frequency_ranges { };  ///< frequency ranges to be marked on-screen

pt_condition_variable frequency_change_condvar;                                             ///< condvar associated with updating windows related to a frequency change
pt_mutex              frequency_change_condvar_mutex { "FREQUENCY CHANGE CONDVAR"s };       ///< mutex associated with frequency_change_condvar

// global variables

STRING_MAP<accumulator<string>> acc_callsigns;                                          ///< accumulator for prefixes for auto callsign mults; key = mult name
accumulator<string>             acc_countries;                                          ///< accumulator for canonical prefixes for auto countries
int                             ACCEPT_COLOUR { COLOUR_GREEN };                         ///< colour for calls that have been worked, but are not dupes
UNORDERED_STRING_SET            all_country_mults;                                      ///< all the country mults from the rules
bool                            allow_audio_recording { false };                        ///< may we record audio?
string                          alternative_qsl_message { };                            ///< no default alternative QSL message
string                          alternative_sap_exchange { };                           ///< no default alternative SAP exchange
string                          at_call;                                                ///< call that should replace commat in "call ok now" message
audio_recorder                  audio;                                                  ///< provide capability to record audio
AUDIO_RECORDING                 audio_recording_mode { AUDIO_RECORDING::DO_NOT_START }; ///< mode of audio recording
atomic<bool>                    autocorrect_rbn { false };                              ///< whether to try to autocorrect posts from the RBN
string                          auto_backup_directory { };                              ///< directory into which backup log and QTC files are to be written

unsigned int            bandmap_decay_time_cluster_secs { };        ///< time in seconds for an entry to age off the bandmap (cluster entries)
unsigned int            bandmap_decay_time_rbn_secs { };            ///< time in seconds for an entry to age off the bandmap (RBN entries)
bool                    bandmap_frequency_up { false };             ///< whether increasing frequency goes upwards in the bandmap
bool                    bandmap_show_marked_frequencies { false };  ///< whether to display entries that would be marked
bool                    best_dx_is_in_miles;                        ///< whether unit for BEST DX window is miles

set<BAND>               call_history_bands;                         ///< bands displayed in CALL HISTORY window
bool                    cluster_cw;                                 ///< whether to place CW posts from the cluster on the bandmap
drlog_context           context;                                    ///< context taken from configuration file
vector<string>          context_path { };                           ///< path taken from context
string                  cq_exchange { };                            ///< no default CQ exchange
int                     cw_bandwidth_narrow;                        ///< narrow CW bandwidth, in Hz
int                     cw_bandwidth_wide;                          ///< wide CW bandwidth, in Hz
unsigned int            cw_speed_change;                            ///< amount to change CW speed when pressing PAGE UP or PAGE DOWN

dynamic_autocorrect_database dad;                                   ///< dynamic autocorrect database
bool                         debug { false };                       ///< whether to log additional information
bool                         display_grid;                          ///< whether to display the grid in GRID and INFO windows
thread::id                   display_rig_status_thread_id { };      ///< will hold the thread ID for the display_rig_status thread
string                       do_not_show_filename;                  ///< name of DO NOT SHOW file
string                       dx_spotting_text    { };                ///< text in comment portion of DX spots
bool                         dynamic_autocorrect_rbn { false };     ///< whether to try to autocorrect posts from the RBN dynamically

exchange_field_database exchange_db;                                ///< dynamic database of exchange field values for calls; automatically thread-safe

vector<COLOUR_TYPE>     fade_colours { };                           ///< the colours used for fading on bandmaps and monitored posts
bool                    filter_remaining_country_mults { false };   ///< whether to apply filter to remaining country mults

string                  geomagnetic_indices_command { };            ///< command to issue in order to obtain geomagnetic indices
float                   greatest_distance { 0 };                    ///< greatest distance in miles

bool                    home_exchange_window { false };             ///< whether to move cursor to left of exchange window (and insert space if necessary)

atomic<bool>            ignore_next_poll { false };                     ///< ignore the result of the next poll of the rig... workaround for slow/inconsistent rig state reporting
atomic<bool>            ignore_next_process_insertion_queue { false };  ///< delay processing of insertion queue of the displayed bandmap
int                     inactivity_time;                                ///< max time since the last activity (activity = QSY or QSO)
bool                    is_ss        { false };                         ///< ss is special

atomic<frequency>       last_update_frequency { };                  ///< the frequency of the last bm window update
logbook                 logbk;                                      ///< the log; can't be called "log" if mathcalls.h is in the compilation path
string                  logfile_name { };                           ///< the name of the logfile
unsigned short          long_t { 0 };                               ///< do not send long Ts at beginning of serno

unsigned int            max_qsos_without_qsl;                       ///< limit for the N7DR matches_criteria() algorithm
memory_information      meminfo;                                    ///< to monitor the state of memory
monitored_posts         mp;                                         ///< the calls being monitored
bool                    multiple_modes { false };                   ///< are multiple modes permitted in the contest?
bandmap_entry           my_bandmap_entry;                           ///< last bandmap entry that refers to me (usually from poll)
string                  my_call;                                    ///< what is my callsign?
string                  my_continent;                               ///< what continent am I on? (two-letter abbreviation)
grid_square             my_grid;                                    ///< what is my (four-character) grid square?
float                   my_latitude;                                ///< my latitude in degrees (north +ve)
float                   my_longitude;                               ///< my longitude in degrees (east +ve)

UNORDERED_STRING_MAP<string>  names;                                ///< map from call to name
unsigned int                  next_qso_number { 1 };                ///< actual number of next QSO
atomic<MINUTES_TYPE>          now_minutes { NOW_MINUTES() };        ///< current absolute number of minutes
bool                          no_default_rst { false };             ///< do we not assign a default received RST?
unsigned int                  n_modes { 0 };                        ///< number of modes allowed in the contest
unsigned int                  n_memories { 0 };                     ///< number of memories on the rig
n_posters_database            n_posters_db_cluster { };             ///< number of posters of calls from cluster (sliding window, per-band)
n_posters_database            n_posters_db_rbn { };                 ///< number of posters of calls from RBN (sliding window, per-band)

unsigned int            octothorpe { 1 };                   ///< serial number of next QSO
old_log                 olog;                               ///< old (ADIF) log containing QSO and QSL information

vector<BAND>            permitted_bands;                    ///< permitted bands, in frequency order
set<BAND>               permitted_bands_set;                ///< permitted bands
set<MODE>               permitted_modes;                    ///< the permitted modes
PING_TABLE              ping_table_p;                       ///< vector of (label, ICMP socket pointer)
STRING_SET              posted_by_continents;               ///< continents to be included in POSTED BY window
vector<dx_post>         posted_by_vector;                   ///< vector of posts of my call during a processing pass of RBN data
//atomic<bool>            preempted { false };                ///< has display of a bandmap been preempted by a manual intervention?
exchange_field_prefill  prefill_data;                       ///< exchange prefill data from external files
unsigned int            p3_span_cq { 5 };                   ///< span of P3 when in CQ mode, in kHz
unsigned int            p3_span_sap { 20 };                 ///< span of P3 when in SAP mode, in kHz

string                  qsl_message { };                    ///< no default QSL message
unsigned short          qtc_long_t { 0 };                   ///< do not send long Ts at beginning of serno in QTCs

unsigned int            rbn_threshold;                      ///< how many times must a call be posted before appearing on a bandmap?
ofstream                rbn_file { };                       ///< copy of rbn data
int                     REJECT_COLOUR { COLOUR_RED };       ///< colour for calls that are dupes
bool                    require_dot_in_replacement_call;    ///< whether a dot is required when reading replacement call from EXCHANGE window (used in exchange.cpp)
bool                    restored_data { false };            ///< did we restore from an archive?
bool                    rig_is_split { false };             ///< is the rig in split mode?

string                  sap_exchange { };                   ///< no default SAP exchange
bool                    scoring_enabled { true };           ///< are we scoring the contest?
bool                    sending_qtc_series { false };       ///< am I senting a QTC series?
unsigned int            serno_spaces { 0 };                 ///< number of additional half-spaces in serno
bool                    self_spotting_enabled { };          ///< whether self-spotting is enabled
string                  self_spotting_text    { };          ///< text in comment portion of self-spots
int                     shift_delta_cw;                     ///< step size for changing RIT (forced positive) -- CW
int                     shift_delta_ssb;                    ///< step size for changing RIT (forced positive) -- SSB
unsigned int            shift_poll           { 0 };         ///< polling interval for SHIFT keys
int                     ssb_bandwidth_narrow { 1600 };      ///< narrow SSB bandwidth, in Hz
int                     ssb_bandwidth_wide   { 1800 };      ///< wide SSB bandwidth, in Hz
int                     ssb_centre_narrow    { 1300 };      ///< narrow SSB bandwidth centre frequency, in Hz
int                     ssb_centre_wide      { 1500 };      ///< wide SSB bandwidth centre frequency, in Hz
running_statistics      statistics;                         ///< all the QSO statistics to date

map<thread::id, string> thread_map { };                     ///< map C++ thread_id to thread name

bool                    wicm_calls_is_dirty  { false };     ///< whether there has been a change requiring redisplay to wicm_calls  
size_t                  wicm_calls_size      { 0 };         ///< maximum number of calls in the WICM window
bool                    windows_overlap      { false };                  ///< whether any windows overlap

bool                    xscp_sort          { false };       ///< whether to enable XSCP ordering in matches

// QTC variables
qtc_database qtc_db;                 ///< sent QTCs
qtc_buffer   qtc_buf;                ///< all sent and unsent QTCs
bool         send_qtcs { false };    ///< whether QTCs are used; set from rules later

EFT CALLSIGN_EFT("CALLSIGN"s);           ///< EFT used in constructor for parsed_exchange (initialised from context during start-up, below)

/* The K3's handling of commands from the computer is rubbish. This variable
   is a simple way to cease polling when we are moving RIT with the shift keys:
   there is a perceptible pause in the RIT adjustment if we happen to poll the
   rig while adjusting RIT (shaking head)
*/
bool ok_to_poll_k3 { true };                  ///< is it safe to poll the K3?

cpair colours;  // must be declared before windows

// windows -- these should automatically be thread_safe
window win_band_mode,                   ///< the band and mode indicator
       win_bandmap,                     ///< the bandmap for the current band
       win_bandmap_filter,              ///< bandmap filter information
       win_bandmap_size,                ///< the sizes of the bandmaps
       win_batch_messages,              ///< messages from the batch messages file
       win_bcall,                       ///< call associated with VFO B
       win_best_dx,                     ///< best DX QSO
       win_bexchange,                   ///< exchange associated with VFO B
       win_call,                        ///< callsign of other station, or command
       win_call_history,                ///< historical QSO and QSL information
       win_cluster_line,                ///< last line received from cluster
       win_cluster_mult,                ///< mults received from cluster
       win_cluster_screen,              ///< interactive screen on to the cluster
       win_date,                        ///< the date
       win_drlog_mode,                  ///< indicate whether in CQ or SAP mode
       win_exchange,                    ///< QSO exchange received from other station
       win_fuzzy,                       ///< fuzzy lookups
       win_grid,                        ///< grid square
       win_indices,                     ///< geomagnetic indices
       win_individual_messages,         ///< messages from the individual messages file
       win_individual_qtc_count,        ///< number of QTCs sent to an individual
       win_info,                        ///< summary of info about current station being worked
       win_last_qrg,                    ///< last QRG of a posted call
       win_local_time,                  ///< window for local time
       win_log,                         ///< main visible log
       win_log_extract,                 ///< to show earlier QSOs
       win_memories,                    ///< the memory contents
       win_name,                        ///< name of operator
       win_ping,                        ///< ping information
       win_qtc_hint,                    ///< hint as to whether to send QTC
       win_system_memory,               ///< system memory
       win_message,                     ///< messages from drlog to the user
       win_mult_value,                  ///< value of a mult
       win_nearby,                      ///< nearby station
       win_monitored_posts,             ///< monitored posts
       win_posted_by,                   ///< stations posting me on the RBN
       win_putative_exchange,           ///< guess at the exchange
       win_query_1,                     ///< query 1 matches
       win_query_n,                     ///< query n matches
       win_quick_qsy,                   ///< QRG and mode for ctrl-=
       win_qsls,                        ///< QSLs from old QSOs
       win_qso_number,                  ///< number of the next QSO
       win_qtc_queue,                   ///< the head of the unsent QTC queue
       win_qtc_status,                  ///< status of QTCs
       win_rate,                        ///< QSO and point rates
       win_rbn_line,                    ///< last line received from the RBN
       win_recording_status,            ///< status of audio recording
       win_remaining_callsign_mults,    ///< the remaining callsign mults
       win_remaining_country_mults,     ///< the remaining country mults
       win_rig,                         ///< rig status
       win_rx_ant,                      ///< receive antenna: "RX" or "TX"
       win_score,                       ///< total number of points
       win_score_bands,                 ///< which bands contribute to score
       win_score_modes,                 ///< which modes contribute to score
       win_scp,                         ///< SCP lookups
       win_scratchpad,                  ///< scratchpad
//       win_sent_rst,                    ///< the sent values of RST if not the default
       win_serial_number,               ///< next serial number (octothorpe)
       win_srss,                        ///< my sunrise/sunset
       win_summary,                     ///< overview of score
       win_time,                        ///< current UTC
       win_title,                       ///< title of the contest
       win_wicm,                        ///< who is calling me?
       win_wpm;                         ///< CW speed in WPM

STRING_MAP<window*> win_remaining_exch_mults_p;     ///< map from name of an exchange mult to a pointer to the corresponding window; key = name

vector<pair<string /* contents*/, window*> > static_windows_p;  ///< static windows; contents may contain multiple lines

// the visible bits of logs
log_extract editable_log(win_log);          ///< the most recent QSOs
log_extract extract(win_log_extract);       ///< earlier QSOs with a station

// some windows are accessed from multiple threads
pt_mutex band_mode_mutex { "BAND/MODE WINDOW"s };                   ///< mutex for win_band_mode

cw_messages cwm;                        ///< pre-defined CW messages

contest_rules rules;                    ///< the rules for this contest

cw_buffer*  cw_p      { nullptr };      ///< pointer to buffer that holds outbound CW message
drmaster    drm_db    { };              ///< the drmaster database
dx_cluster* cluster_p { nullptr };      ///< pointer to cluster information
dx_cluster* rbn_p     { nullptr };      ///< pointer to RBN information

const drmaster& drm_cdb { drm_db };     ///< const version of the drmaster database

location_database location_db;              ///< global location database
rig_interface     rig;                      ///< rig control

thread_attribute attr_detached { PTHREAD_DETACHED };   ///< default attribute for threads

window*       win_active_p        { &win_call };            ///< start with the CALL window active
ACTIVE_WINDOW active_window       { ACTIVE_WINDOW::CALL };  ///< start with the CALL window active
ACTIVE_WINDOW last_active_window  { ACTIVE_WINDOW::CALL };  ///< start with the CALL window active

autocorrect_database ac_db;                     ///< the RBN autocorrection database

array<bandmap, NUMBER_OF_BANDS>                  bandmaps;                  ///< one bandmap per band (even bands that aren't going to be used)
array<BANDMAP_INSERTION_QUEUE, NUMBER_OF_BANDS>  bandmap_insertion_queues;  ///< one queue per band

array<UNORDERED_STRING_MAP<string>, NUMBER_OF_BANDS>  last_posted_qrg;          ///< per-band container of most recent posted QRG for calls
array<mutex, NUMBER_OF_BANDS>                         last_posted_qrg_mutex;    ///< mutexes for per-band container of most recent posted QRG for calls

call_history q_history;                         ///< history of calls worked

rate_meter rate;                                ///< QSO and point rates

scp_database  scp_db,                           ///< static SCP database from file
              scp_dynamic_db;                   ///< dynamic SCP database from QSOs
scp_databases scp_dbs;                          ///< container for the SCP databases

vector<string> win_log_snapshot;                ///< individual lines in the LOG window

// foreground = ACCEPT_COLOUR => worked on a different band and OK to work on this band; foreground = REJECT_COLOUR => dupe
using STR_COLOUR_PAIR = pair<string, PAIR_NUMBER_TYPE>; 
using MATCHES_TYPE    = vector<STR_COLOUR_PAIR>;  // str = callsign

array<MATCHES_TYPE, 4> matches_array;             // this allows one to cycle easily through all four types of matches

MATCHES_TYPE& scp_matches     { matches_array[0] };
MATCHES_TYPE& fuzzy_matches   { matches_array[1] };
MATCHES_TYPE& query_1_matches { matches_array[2] };
MATCHES_TYPE& query_n_matches { matches_array[3] };

fuzzy_database  fuzzy_db,                       ///< static fuzzy database from file
                fuzzy_dynamic_db;               ///< dynamic SCP database from QSOs
fuzzy_databases fuzzy_dbs;                      ///< container for the fuzzy databases

query_database  query_db;                       ///< database for query matches
          
map<BAND, pair<frequency, MODE>> quick_qsy_map;

/// variables to hold the foreground and background colours of the QTC HINT window
int win_qtc_hint_fg { COLOUR_WHITE };
int win_qtc_hint_bg { COLOUR_BLACK };

// prepare for terminal I/O
keyboard_queue keyboard;                    ///< queue of keyboard events

// quick access to whether particular types of mults are in use; these are written only during start-up, so we don't bother to protect them
bool callsign_mults_used { false };            ///< do the rules call for callsign mults?
bool country_mults_used  { false };            ///< do the rules call for country mults?
bool exchange_mults_used { false };            ///< do the rules call for exchange mults?
bool mm_country_mults    { false };            ///< can /MM stns be country mults?

/*! \brief                  Update the SCP or fuzzy window and vector of matches
    \param  matches         container of matches
    \param  match_vector    OUTPUT vector of pairs of calls and colours (in display order)
    \param  win             window to be updated
    \param  callsign        (partial) callsign to be matched

    Clears <i>win</i> if the length of <i>callsign</i> is less than the minimum specified by the MATCH MINIMUM command

  Display order (each in callsign order):
    exact match (regardless of colour)
    green matches
    ordinary matches
    red matches

  TODO?: Might want to put red matches immediately after green matches
  TODO?: Might want to put green matches before exact matches

  This has to go before the inline functions that use it
*/
template <typename T>
//void update_matches_window(const T& matches, vector<pair<string, PAIR_NUMBER_TYPE>>& match_vector, window& win, const string& callsign)
void update_matches_window(const T& matches, vector<pair<string, PAIR_NUMBER_TYPE>>& match_vector, window& win, const string_view callsign)
{ using CALL_AND_COLOURS = pair<string, PAIR_NUMBER_TYPE>;

  if (callsign.length() >= context.match_minimum())   // only process if the callsign is sufficiently long
  { const auto [win_fg, win_bg] { win.fgbg() };

// put in right order and also get the colours right
    vector<string> vec_str;
    vec_str.reserve(matches.size());

    vec_str += matches;
//    vector<string> vec_str(matches);

    SORT(vec_str, compare_calls);           // default will be to have calls in each colour sorted by call
    match_vector.clear();

// put an exact match at the front (this will never happen with a fuzzy match)
    vector<string> tmp_exact_matches;                   // variable in which to build interim ordered matches
    vector<string> tmp_green_matches;                   // variable in which to build interim ordered matches
    vector<string> tmp_red_matches;                     // variable in which to build interim ordered matches
    vector<string> tmp_ordinary_matches;                // variable in which to build interim ordered matches

    if (contains(vec_str, callsign))
      tmp_exact_matches += callsign;

    auto is_dupe = [] (const string& call) { return logbk.is_dupe(call, current_band, current_mode, rules); };

    for (const auto& cs : vec_str)
    { if (cs != callsign)
      { vector<string>& tmp_matches { (is_dupe(cs) ? tmp_red_matches : ( logbk.qso_b4(cs) ? tmp_green_matches : tmp_ordinary_matches)) };

        tmp_matches += cs;
      }
    }

//    FOR_ALL(vec_str | SRV::filter([] (const string& str) { return { str != callsign; } ), [] (const string& cs)
//                       { vector<string>& tmp_matches { (is_dupe(cs) ? tmp_red_matches : ( logbk.qso_b4(cs) ? tmp_green_matches : tmp_ordinary_matches)) };
//
//                         tmp_matches += cs;
//                       } );

//    for (auto cs : vec_str | SRV::filter([] (const string& str) { return { str != callsign; } ))
//    { //if (cs != callsign)
//      { vector<string>& tmp_matches { (is_dupe(cs) ? tmp_red_matches : ( logbk.qso_b4(cs) ? tmp_green_matches : tmp_ordinary_matches)) };
//
//        tmp_matches += cs;
//      }
//    }

    for (const auto& cs : tmp_exact_matches)
    { match_vector += (is_dupe(cs) ? CALL_AND_COLOURS { cs, colours.add(REJECT_COLOUR, win_bg) }
                                   : CALL_AND_COLOURS { cs, colours.add( (logbk.qso_b4(cs) ? ACCEPT_COLOUR : win_fg), win_bg ) });
    }

// change the order within each category if XSCP ordering is to be used
    if (xscp_sort)
    { for (auto matches_p : vector<vector<string>*> { &tmp_exact_matches, &tmp_green_matches, &tmp_ordinary_matches, &tmp_red_matches })    // step through each type of match
        if (matches_p->size() > 1)
         SORT(*matches_p, xscp_order_greater);
    }

    FOR_ALL(tmp_green_matches,    [win_bg, &match_vector]         (const string& cs) { match_vector += { cs, colours.add(ACCEPT_COLOUR, win_bg) }; });
    FOR_ALL(tmp_ordinary_matches, [win_bg, win_fg, &match_vector] (const string& cs) { match_vector += { cs, colours.add(win_fg, win_bg) }; });
    FOR_ALL(tmp_red_matches,      [win_bg, &match_vector]         (const string& cs) { match_vector += { cs, colours.add(REJECT_COLOUR, win_bg) }; });

    win < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= match_vector;
  }
  else                                        // callsign is too short
    win <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
}

// simple inline functions

// current time (in minutes since the epoch)
inline MINUTES_TYPE NOW_MINUTES(void)
  { return static_cast<MINUTES_TYPE>(NOW() / 60); }

/// recall a memory
inline memory_entry recall_memory(const unsigned int n)
  { return ( (n < memories.size()) ? memories[n] : memory_entry() ); }

/*! \brief      Am I sending CW?
    \return     whether I appear to be sending CW

    This does not need to be, and is not, either robust or clever. It's used only to control
    behaviour when recording audio, as disk writes can cause minor, occasional CW stutter on
    a very slow machine if the CW is not being sent on a thread with RT scheduling.
*/
inline bool sending_cw(void)
  { return (cw_p != nullptr) and !(cw_p->empty()); }

/// get the frequency and mode
inline pair<frequency, MODE> get_frequency_and_mode(void)
  { return { rig.rig_frequency(), current_mode }; }

/*! \brief      Convert a serial number to a string
    \param  n   serial number
    \return     <i>n</i> as a zero-padded string of three digits, or a four-digit string if <i>n</i> is greater than 999
*/
inline string serial_number_string(const unsigned int n)
  { return ( (n < 1000) ? pad_leftz(n, 3) : to_string(n) ); }

/*! \brief              Calculate the sunrise time for a station
    \param  callsign    call of the station for which sunset is desired
    \return             sunrise in the form HH:MM

    Returns "DARK" if it's always dark, and "LIGHT" if it's always light
 */
inline string sunrise(const string_view callsign)
  { return sunrise_or_sunset(callsign, SRSS::SUNRISE); }

/*! \brief              Calculate the sunset time for a station
    \param  callsign    call of the station for which sunset is desired
    \return             sunset in the form HH:MM

    Returns "DARK" if it's always dark, and "LIGHT" if it's always light
*/
inline string sunset(const string_view callsign)
  { return sunrise_or_sunset(callsign, SRSS::SUNSET); }

/// return the name of the current thread
inline string my_thread_name(void)
  { return MUM_VALUE(thread_map, my_thread_id(), "UNKNOWN THREAD NAME"s); }

/*! \brief              Update the fuzzy window with matches for a particular call
    \param  callsign    callsign against which to generate the fuzzy matches
*/
inline void update_fuzzy_window(const string& callsign)
  { update_matches_window(fuzzy_dbs[callsign], fuzzy_matches, win_fuzzy, callsign); }

/*! \brief  Update <i>win_recording_status</i>
*/
inline void update_recording_status_window(void)
  { win_recording_status < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= ( (allow_audio_recording and audio.recording()) ? "REC"s : "---"s ); }

/*! \brief              Update the SCP window with matches for a particular call
    \param  callsign    callsign against which to generate the SCP matches
*/
inline void update_scp_window(const string& callsign)
  { update_matches_window(scp_dbs[callsign], scp_matches, win_scp, callsign); }

/*! \brief      Is one call before another when ordered according to the number of XSCP entries for each call?
    \param  c1  first call
    \param  c2  second call
    \return     whether the XSCP value for <i>c1</i> is less than the XSCP value for <i>c2</i>
*/
//inline bool xscp_order_greater(const string& c1, const string& c2)
inline bool xscp_order_greater(const string_view c1, const string_view c2)
  { return (drm_db[c1].xscp() > drm_db[c2].xscp()); }

int main(int argc, char** argv)
{ 
// generate version information
  try
  { const STRING_MAP<string> MONTH_NAME_TO_NUMBER( { { "Jan"s, "01"s },
                                                     { "Feb"s, "02"s },
                                                     { "Mar"s, "03"s },
                                                     { "Apr"s, "04"s },
                                                     { "May"s, "05"s },
                                                     { "Jun"s, "06"s },
                                                     { "Jul"s, "07"s },
                                                     { "Aug"s, "08"s },
                                                     { "Sep"s, "09"s },
                                                     { "Oct"s, "10"s },
                                                     { "Nov"s, "11"s },
                                                     { "Dec"s, "12"s }
                                                   } );

    const string date_str { DATE_STR.substr(DATE_STR.length() - 4) + "-"s + MONTH_NAME_TO_NUMBER.at(DATE_STR.substr(0, 3)) + "-"s +
                            (DATE_STR[4] == ' ' ? "0"s + DATE_STR.substr(5, 1) : DATE_STR.substr(4, 2)) };

    VERSION = VERSION_TYPE + SPACE_STR + date_str + SPACE_STR + TIME_STR.substr(0, 5);

    ost << "Running drlog version " << VERSION << endl;
  }

  catch (...)
  { ost << "Error: Unable to generate drlog version information" << endl;
    VERSION = "Unknown version "s + VERSION;  // because VERSION may be used elsewhere
  }

  thread_map += { my_thread_id(), "MAIN"s };    // add main thread name to known threads

#if 0
  { constexpr size_t ITERATIONS { 1'000'000 };

    vector<string> vs { };

    for (size_t n { 0 }; n < ITERATIONS; ++n)
      vs += to_string(n);

    vector<string_view> vsv { };

    for (size_t n { 0 }; n < ITERATIONS; ++n)
      vsv += vs[n];

    { time_log <std::chrono::milliseconds> tl;
//FOR_ALL(clean_split_string <string_view> (RHS), [this] (const string_view str) { _auto_remaining_exchange_mults += str; });

      STRING_SET acc { };

      FOR_ALL(vsv, [&acc] (const string_view str) { acc += str; });

      tl.end_now();
      ost << "old time = " << tl.time_span<int>() << " milliseconds" << endl;
    }

    { time_log <std::chrono::milliseconds> tl;
//FOR_ALL(clean_split_string <string_view> (RHS), [this] (const string_view str) { _auto_remaining_exchange_mults += str; });

      STRING_SET acc { };
      STRING_SET& accref { acc };
//      STRING_SET* accp { &acc };

//      acc = SR::fold_left(vsv, STRING_SET { }, [] (STRING_SET acc, const string_view sv) { return acc + sv; });
      acc = SR::fold_left(vsv, accref, [] (auto accref, const auto sv) { return (accref + sv); });

      tl.end_now();
      ost << "new time = " << tl.time_span<int>() << " milliseconds" << endl;
    }

//        _auto_remaining_exchange_mults = std::ranges::fold_left(clean_split_string <string_view> (RHS), STRING_SET { }, [] (STRING_SET acc, const string_view sv) { return acc + sv; }); // can't use a STRING_SET&

    exit(0);
  }
#endif

// output the number of colours available
  ost << "Number of colours supported on screen = " << COLORS << endl;

// rename the mutexes in the bandmaps and the mutexes in the container of last qrgs
  for (FORTYPE(NUMBER_OF_BANDS) n { 0 }; n < NUMBER_OF_BANDS; ++n)
     bandmaps[n].rename_mutex("BANDMAP: "s + BAND_NAME.at(n));

// add band info to each bandmap
  for (size_t n { 0 }; n < NUMBER_OF_BANDS; ++n)
    bandmaps[n].band( static_cast<BAND>(n) );

  const command_line cl              { argc, argv };                                                              ///< for parsing the command line
  const string       config_filename { (cl.value_present("-c"s) ? cl.value("-c"s) : "logcfg.dat"s) };

  try    // put it all in one big try block (one of the few things in C++ I have hated ever since we introduced it)
  {
// read configuration data (typically from logcfg.dat)
    drlog_context* context_p { nullptr };

    try
    { context_p = new drlog_context(config_filename);
    }

    catch (...)
    { ost << "Error reading configuration data from " << config_filename << endl;
      exit(-1);
    }

// make the context available globally and cleanup the context pointer
    context = *context_p;
    delete context_p;   // we no longer need this

// do any windows overlap?
    for (const auto& [win_name_1, win_name_2] : window_overlaps(context.windows()))
    { windows_overlap = true;   // mark that we have overlapping windows

      ost << "ERROR: WINDOW OVERLAP: " << win_name_1 << " + " << win_name_2 << endl;
      cerr << "ERROR: WINDOW OVERLAP: " << win_name_1 << " + " << win_name_2 << endl;
    }

// run any "execute at start" program
    if (const auto cmd { context.execute_at_start() }; !cmd.empty())
    { ost << "Executing external command: " << cmd << endl
          << "output:" << endl;

      ost << run_external_command(cmd) << endl;
    }

// set some immutable variables from the context
    DP            = context.decimal_point();            // correct decimal point indicator
    TS            = context.thousands_separator();      // correct thousands separator
    ACCEPT_COLOUR = context.accept_colour();            // colour for calls it is OK to work
    REJECT_COLOUR = context.reject_colour();            // colour for calls it is not OK to work

    allow_audio_recording           = context.allow_audio_recording();
    alternative_qsl_message         = context.alternative_qsl_message();
    alternative_sap_exchange        = context.alternative_exchange_sap();
    audio_recording_mode            = context.start_audio_recording();
    auto_remaining_country_mults    = context.auto_remaining_country_mults();
    autocorrect_rbn                 = context.autocorrect_rbn();
    auto_backup_directory           = context.auto_backup_directory();

    bandmap_decay_time_cluster_secs = context.bandmap_decay_time_cluster() * 60;
    bandmap_decay_time_rbn_secs     = context.bandmap_decay_time_rbn() * 60;
    bandmap_frequency_up            = context.bandmap_frequency_up();
    bandmap_show_marked_frequencies = context.bandmap_show_marked_frequencies();
    best_dx_is_in_miles             = (context.best_dx_unit() == "MILES"sv);

    cluster_cw                      = context.cluster_cw();
    call_history_bands              = context.call_history_bands();
    context_path                    = context.path();
    cq_exchange                     = context.exchange_cq();
    cw_bandwidth_narrow             = context.cw_bandwidth_narrow();
    cw_bandwidth_wide               = context.cw_bandwidth_wide();
    cw_speed_change                 = context.cw_speed_change();

    display_grid                    = context.display_grid();
    do_not_show_filename            = context.do_not_show_filename();
    dx_spotting_text                = context.dx_spotting_text();
    dynamic_autocorrect_rbn         = context.dynamic_autocorrect_rbn();

    fade_colours                    = context.bandmap_fade_colours();

    geomagnetic_indices_command     = context.geomagnetic_indices_command();

    home_exchange_window            = context.home_exchange_window();

    inactivity_time                 = context.inactivity_time();

    logfile_name                    = context.logfile();
    long_t                          = context.long_t();

    marked_frequency_ranges         = context.mark_frequencies();
    max_qsos_without_qsl            = context.max_qsos_without_qsl();
    multiple_modes                  = context.multiple_modes();
    my_call                         = context.my_call();
    my_continent                    = context.my_continent();
    my_grid                         = grid_square(context.my_grid());
    my_latitude                     = context.my_latitude();
    my_longitude                    = context.my_longitude();

    no_default_rst                  = context.no_default_rst();
    n_memories                      = context.n_memories();

// configure table for checking connectivity to other machines
    if (const auto& targets { context.ping_targets() }; !targets.empty())
    { ost << "Number of ping targets = " << targets.size() << endl;

      for (const auto& [addr, label] : targets)
      { ost << "  ping target: " << label << " at " << addr << endl;

        try
        { ping_table_p += pair { label, new icmp_socket(addr, context.my_ip()) };

          ost << "    created ping_table entry for target " << label << " at " << addr << endl;
        }

        catch (...)
        { ost << "exception caught; unable to create ping_table entry for target " << label << " at " << addr << endl;
        }
      }

      ost << "Created ping table for " << ping_table_p.size() << " targets" << endl;
    }

    posted_by_continents            = context.posted_by_continents();
    p3_span_cq                      = context.p3_span_cq();
    p3_span_sap                     = context.p3_span_sap();

    qsl_message                     = context.qsl_message();
    qtc_long_t                      = context.qtc_long_t();

    rbn_threshold                   = context.rbn_threshold();
    require_dot_in_replacement_call = context.require_dot_in_replacement_call();

    sap_exchange                    = context.exchange_sap();
    scoring_enabled                 = context.scoring_enabled();
    self_spotting_enabled           = context.self_spotting_enabled();
    self_spotting_text              = context.self_spotting_text();
    serno_spaces                    = context.serno_spaces();
    shift_delta_cw                  = static_cast<int>(context.shift_delta_cw());   // forced positive int
    shift_delta_ssb                 = static_cast<int>(context.shift_delta_ssb());  // forced positive int
    shift_poll                      = context.shift_poll();
    ssb_bandwidth_narrow            = context.ssb_bandwidth_narrow();
    ssb_bandwidth_wide              = context.ssb_bandwidth_wide();
    ssb_centre_narrow               = context.ssb_centre_narrow();
    ssb_centre_wide                 = context.ssb_centre_wide();

    xscp_sort                       = context.xscp_sort();

    n_posters_db_cluster.min_posters(context.cluster_threshold());
    n_posters_db_rbn.min_posters(context.rbn_threshold());
    prefill_data.insert_prefill_filename_map(context.exchange_prefill_files());   

// set up initial quick qsy information
    for (int n { static_cast<int>(MIN_BAND) }; n <= static_cast<int>(MAX_BAND); ++n)
      quick_qsy_map[static_cast<BAND>(n)] = pair<frequency, MODE> { BOTTOM_OF_BAND.at(static_cast<BAND>(n)), MODE_CW };

// possibly configure audio recording
    if (allow_audio_recording and (context.start_audio_recording() != AUDIO_RECORDING::DO_NOT_START))
    { start_recording(audio, context);
      alert("audio recording started due to activity"s);
    }

    { SAFELOCK(my_bandmap_entry);

      time_last_qsy = NOW();  // initialise with a dummy QSY
    }

// set up the calls to be monitored
    mp.callsigns(context.post_monitor_calls());

// read the country data
    cty_data* country_data_p { nullptr };

    try
    { country_data_p = new cty_data(context_path, context.cty_filename());
    }

    catch (...)
    { ost << "Error reading country data: does the file " << context.cty_filename() << " exist?" << endl;
      exit(-1);
    }

    const cty_data& country_data { *country_data_p };

// read the drmaster database
    try
    { time_log <std::chrono::milliseconds> tl;

      drm_db = drmaster { context_path, context.drmaster_filename(), context.xscp_cutoff() };

      tl.end_now();
      ost << "time taken to prepare drmaster = " << tl.time_span<int>() << " milliseconds" << endl;

      ost << "drmaster database contains " << css(drm_db.size()) << " entries" << endl;

      if (context.xscp_percent_cutoff())                              // prune the database of low XSCP numbers
      { drm_db = drm_db.prune(context.xscp_percent_cutoff().value());

        ost << "pruned drmaster database contains " << css(drm_db.size()) << " entries" << endl;
      }
    }

    catch (...)
    { cerr << "Error reading drmaster database file " << context.drmaster_filename() << endl;
      exit(-1);
    }

// location database
    try
    { location_db.prepare(country_data, context.country_list());
    }

    catch (...)
    { cerr << "Error generating location database" << endl;
      exit(-1);
    }

    location_db.add_russian_database(context_path, context.russian_filename());  // add Russian information

// build super check partial database from the drmaster information
    try
    { scp_db.init_from_calls(drm_cdb.calls());
    }

    catch (...)
    { cerr << "Error initialising scp database" << endl;
      exit(-1);
    }

    scp_dbs += scp_db;            // incorporate into multiple-database version
    scp_dbs += scp_dynamic_db;    // add the (empty) dynamic SCP database

// build fuzzy database from the drmaster information
    try
    { fuzzy_db.init_from_calls(drm_cdb.calls());
    }

    catch (...)
    { cerr << "Error generating fuzzy database" << endl;
      exit(-1);
    }

    fuzzy_dbs += fuzzy_db;            // incorporate into multiple-database version
    fuzzy_dbs += fuzzy_dynamic_db;    // add the (empty) dynamic fuzzy database

// build autocorrect database from the drmaster information, regardless of whether it is currently set to be used
    try
    { ac_db.init_from_calls(drm_cdb.calls());

      ost << "number of calls in autocorrect database = " << css(ac_db.n_calls()) << endl;
      ost << "autocorrect is " << (autocorrect_rbn ? "ON"s : "OFF"s) << endl;
    }

    catch (...)
    { cerr << "Error initialising autocorrect database" << endl;
      exit(-1);
    }

// build query database from the drmaster information
    query_db = drm_cdb.unordered_calls();

// possibly build name database from the drmaster information (not the same as the names used in exchanges)
    if (context.window_info("NAME"s).defined())                   // does the config file define a NAME window?
      FOR_ALL(drm_cdb.unordered_calls(), [] (const auto& this_call) { names[this_call] = drm_cdb[this_call].name(); } );

// define the rules for this contest
    try
    { rules.prepare(context, location_db);
    }

    catch (...)
    { cerr << "Error generating rules" << endl;
      exit(-1);
    }

// set some more-or-less immutable variables from the rules
    permitted_bands     = rules.permitted_bands();
    permitted_bands_set = rules.permitted_bands_set();
    permitted_modes     = rules.permitted_modes();
    all_country_mults   = rules.country_mults();

// is it SS?
    if (rules.n_modes() == 1)
    { const vector<exchange_field> exchange_template { rules.unexpanded_exch("K"s, *(rules.permitted_modes().cbegin())) };

      is_ss = ANY_OF(exchange_template, [] (const exchange_field& ef) { return (ef.name() == "PREC"sv); });                // if there's a PREC field, it must be SS
    }

// MESSAGE window (do this as early as is reasonable so that it's available for messages)
    win_message.init(context.window_info("MESSAGE"s), WINDOW_NO_CURSOR);
    win_message < WINDOW_ATTRIBUTES::WINDOW_BOLD <= EMPTY_STR;                                       // use bold in this window

// is there a log of old QSOs? If so, read and process it (in a separate thread)
// start a block so that a lot of variables are auto-destroyed at the end of the block, when the intialisation phase is over
    { thread thr;

      bool running_old_log_thread { false };

      if (!context.old_adif_log_name().empty())
      { thr = thread(adif3_build_old_log);
        running_old_log_thread = true;
      }

// make some things available file-wide
// make callsign parser available now that we can create it
      CALLSIGN_EFT = EFT(CALLSIGN_EFT.name(), context_path, context.exchange_fields_filename(), context, location_db);

      send_qtcs = rules.send_qtcs();    // grab it once
      n_modes = rules.n_modes();        // grab this once too

// define types of mults that are in use; after this point these should be treated as read-only
      callsign_mults_used = rules.callsign_mults_used();
      country_mults_used  = rules.country_mults_used();
      exchange_mults_used = rules.exchange_mults_used();
      mm_country_mults    = rules.mm_country_mults();

// possibly get a list of IARU society exchanges; note that we normally do this with a prefill file instead
      if (!context.society_list_filename().empty())
        exchange_db.set_values_from_file(context_path, context.society_list_filename(), "SOCIETY"s);

// possibly test regex exchanges; this will exit if it executes
      if (cl.value_present("-test-exchanges"s))
        test_exchange_templates(rules, cl.value("-test-exchanges"s));

// real-time statistics
      try
      { statistics.prepare(country_data, context, rules);
      }

      catch (...)
      { cerr << "Error generating real-time statistics" << endl;
        exit(-1);
      }

// possibly open communication with the rig
      rig.register_error_alert_function(rig_error_alert);

      if (!context.rig1_port().empty() and !context.rig1_type().empty())
      { try
        { rig.prepare(context);
        }
      
        catch (const rig_interface_error& e)
        { const string msg { "Error initialising rig; error code = " + to_string(e.code()) + ", reason = " + e.reason() };
      
          alert(msg, SHOW_TIME::NO_SHOW);
          sleep_for(5s);
          exit(-1);
        }
      }
    
// possibly put rig into TEST mode
      if (context.test())
        rig.test(true);

// possibly set up CW buffer
      if (contains(to_upper(context.modes()), "CW"s) and !context.keyer_port().empty())
      { try
        { cw_p = new cw_buffer(context.keyer_port(), context.ptt_delay(), context.cw_speed(), context.cw_priority());
        }

        catch (const parallel_port_error& e)
        { ost << "Failed to open CW port: " << e.reason() << endl;
          exit(-1);
        }

        if (rig.valid())
          cw_p->associate_rig(&rig);

        cwm.init(context.messages());
      }

// set the initial band and mode from the configuration file
      if (context.qsy_on_startup())
      { current_band = ( (rules.score_bands().size() == 1) ? *(rules.score_bands().cbegin()) : context.start_band() );
        current_mode = ( (rules.score_modes().size() == 1) ? *(rules.score_modes().cbegin()) : context.start_mode() );

// see whether the rig is on the right band and mode (as defined in the configuration file); if not, then move it
        if (current_band != static_cast<BAND>(rig.rig_frequency()))
        { rig.rig_frequency(DEFAULT_FREQUENCIES.at( { current_band, current_mode } ));
          sleep_for(2s);                                                       // give things time to settle on the rig
        }
      }
      else                // do not QSY on startup
      { current_band = to_BAND(rig.rig_frequency());
        current_mode = ( (rules.score_modes().size() == 1) ? *(rules.score_modes().cbegin()) : context.start_mode() );
      }

// the rig might have changed mode if we just changed bands
      if (current_mode != rig.rig_mode())
        rig.rig_mode(current_mode);

      fast_cw_bandwidth();    // set to default SAP bandwidth if on CW
      rig.base_state();       // put rig in known base state

// configure bandmaps so user's call and calls in the do-not-show list do not display
      FOR_ALL(bandmaps, [dns = context.do_not_show()] (bandmap& bm) { bm.do_not_add(my_call);
                                              
                                                                      if (!dns.empty())
                                                                        bm.do_not_add(dns); 
                                                                    } );

// ditto for other calls in the do-not-show files
      if (!do_not_show_filename.empty())
      { if (find_file(context_path, do_not_show_filename).empty())
        { ost << "Fatal error: unable to read do-not-show file: " << do_not_show_filename << endl;      // the all-band file MUST exist; maybe change this later?
          cerr << "Fatal error: unable to read do-not-show file: " << do_not_show_filename << endl;      // the all-band file MUST exist; maybe change this later?

          sleep_for(5s);    // give it time to clean up
          exit(-1);
        }

        for (const auto& callsign : calls_from_do_not_show_file(ALL_BANDS))
          FOR_ALL(bandmaps, [&callsign] (bandmap& bm) { bm.do_not_add(callsign); } );

// now the individual bands
        for (BAND b { MIN_BAND }; b <= MAX_BAND; b = (BAND)((int)b + 1))
          FOR_ALL(calls_from_do_not_show_file(b), [&bm = bandmaps[b]] (const auto& callsign) { bm.do_not_add(callsign); });
      }

// set the RBN threshold for each bandmap
      if (rbn_threshold != 1)               // 1 is the default in a pristine bandmap, so there may be no need to change
        FOR_ALL(bandmaps, [] (bandmap& bm) { bm.rbn_threshold(rbn_threshold); } );

// set the initial cull function for each bandmap
      if (const int cull_function_nr { context.bandmap_cull_function() }; cull_function_nr)           // if not cull function number zero
        FOR_ALL(bandmaps, [cull_function_nr] (bandmap& bm) { bm.cull_function(cull_function_nr); } );

// initialise some immutable information in my_bandmap_entry; do not bother to acquire the lock
// this must be the only place that we access my_bandmap_entry outside the update_based_on_frequency_change() function
      my_bandmap_entry.callsign(MY_MARKER);
      my_bandmap_entry.source(BANDMAP_ENTRY_SOURCE::LOCAL);
      my_bandmap_entry.expiration_time(my_bandmap_entry.time() + MILLION);    // a million seconds in the future

// add my marker to each bandmap
      ost << "initialising bandmap with my info" << endl;

      for (const auto b : permitted_bands)
      { bandmap&      bm { bandmaps[b] };
        bandmap_entry be { my_bandmap_entry };

        if (b == current_band)
          be.freq(rig.rig_frequency());
        else
//          be.freq(DEFAULT_FREQUENCIES.at( { current_band, current_mode } ));
          be.freq(DEFAULT_FREQUENCIES.at( { b, current_mode } ));

        ost << "band number: " << b << endl;
        ost << "band number for bandmap: " << bm.band() << endl;
        ost << "bandmap for band: " << BAND_NAME[bm.band()] << endl;

        bm += be;

        ost << "be: " << be << endl;
      }

// possibly add a mode marker bandmap entry to each bandmap (only in multi-mode contests)
      if (context.mark_mode_break_points())
      { for (const auto b : permitted_bands)
        { bandmap& bm { bandmaps[b] };

          bandmap_entry be;

          be.callsign(MODE_MARKER);
          be.source(BANDMAP_ENTRY_SOURCE::LOCAL);
          be.expiration_time(be.time() + MILLION);        // expiration is a long time in the future
          be.freq(MODE_BREAK_POINT[b]);

          bm += be;

          bm.mode_marker_frequency(MODE_BREAK_POINT[b]);  // tell the bandmap that this is a mode marker
        }
      }

// create and populate windows; do static windows first
      const STRING_MAP<pair<string /* contents */, vector<window_information> > >& swindows { context.static_windows() };

// static windows may contain either defined information or the contents of a file
      for (const auto& [ win_name, pr_content_vec_win_info ] : swindows)
      { const auto& [win_contents, vec_win_info] { pr_content_vec_win_info };

        for (const auto& winfo : vec_win_info)
        { window* window_p { new window() };

          window_p -> init(winfo);
          static_windows_p += { win_contents, window_p };
        }
      }

      for (const auto& [ contents, swin_p ] : static_windows_p)
        *(swin_p) <= reformat_for_wprintw(contents, swin_p -> width());       // display contents of the static window, working around wprintw weirdness

// BAND/MODE window
      win_band_mode.init(context.window_info("BAND/MODE"s), WINDOW_NO_CURSOR);

// BATCH MESSAGES window
      win_batch_messages.init(context.window_info("BATCH MESSAGES"s), WINDOW_NO_CURSOR);

      if (!context.batch_messages_file().empty())
      { try
        { //const vector<string> messages { to_lines <std::string> (read_file(context_path, context.batch_messages_file())) };

          string current_message { };

          SAFELOCK(batch_messages);

//          for (const auto& messages_line : messages)
          for (const auto& messages_line : to_lines <std::string> (read_file(context_path, context.batch_messages_file())))
          { if (!messages_line.empty())
            { if (contains(messages_line, '['))
                current_message = delimited_substring <std::string> (messages_line, '[', ']', DELIMITERS::DROP);       // extract this batch message
              else
                batch_messages += { remove_peripheral_spaces <std::string> (messages_line) /* callsign */, current_message };       // associate this message with the callsign on the line
            }
          }

          ost << "read " << batch_messages.size() << " batch messages" << endl;
        }

       catch (...)
        { cerr << "Unable to read batch messages file: " << context.batch_messages_file() << endl;
          exit(-1);
        }
      }

// BCALL window
      win_bcall.init(context.window_info("BCALL"s), COLOUR_YELLOW, COLOUR_MAGENTA, WINDOW_NO_CURSOR);
      win_bcall < WINDOW_ATTRIBUTES::WINDOW_BOLD <= EMPTY_STR;
//  win_call.process_input_function(process_CALL_input);

// BEST DX window
      win_best_dx.init(context.window_info("BEST DX"s), WINDOW_NO_CURSOR);
      win_best_dx.enable_scrolling();

// BEXCHANGE window
      win_bexchange.init(context.window_info("BEXCHANGE"s), COLOUR_YELLOW, COLOUR_MAGENTA, WINDOW_NO_CURSOR);
      win_bexchange <= WINDOW_ATTRIBUTES::WINDOW_BOLD;
//  win_exchange.process_input_function(process_EXCHANGE_input);

// CALL window
      win_call.init(context.window_info("CALL"s), COLOUR_YELLOW, COLOUR_MAGENTA, WINDOW_INSERT);
      win_call < WINDOW_ATTRIBUTES::WINDOW_BOLD <= EMPTY_STR;
      win_call.process_input_function(process_CALL_input);

// CALL HISTORY window
      win_call_history.init(context.window_info("CALL HISTORY"s), WINDOW_NO_CURSOR);
      win_call_history <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;                                        // make it visible

// CLUSTER LINE window
      win_cluster_line.init(context.window_info("CLUSTER LINE"s), WINDOW_NO_CURSOR);

// DATE window
      win_date.init(context.window_info("DATE"s), WINDOW_NO_CURSOR);

// DRLOG MODE window
      win_drlog_mode.init(context.window_info("DRLOG MODE"s), WINDOW_NO_CURSOR);

// EXCHANGE window
      win_exchange.init(context.window_info("EXCHANGE"s), COLOUR_YELLOW, COLOUR_MAGENTA, WINDOW_INSERT);
      win_exchange <= WINDOW_ATTRIBUTES::WINDOW_BOLD;
      win_exchange.process_input_function(process_EXCHANGE_input);

// FUZZY window
      win_fuzzy.init(context.window_info("FUZZY"s), WINDOW_NO_CURSOR);

// GRID window
      win_grid.init(context.window_info("GRID"s), WINDOW_NO_CURSOR);

// INDICES window
      win_indices.init(context.window_info("INDICES"s), WINDOW_NO_CURSOR);

// possibly get the indices data (in a separate thread)
      if (!geomagnetic_indices_command.empty())
        jthread(get_indices, geomagnetic_indices_command).detach();

// INDIVIDUAL MESSAGES window; now also handles files that contain an initial (unused) date field; i.e., date: callsign: message
      win_individual_messages.init(context.window_info("INDIVIDUAL MESSAGES"s), WINDOW_NO_CURSOR);

      if (!context.individual_messages_file().empty())
      { try
        { SAFELOCK(individual_messages);

          for (const auto& messages_line : to_lines <std::string> (read_file(context_path, context.individual_messages_file())))
          { const vector<string> fields { clean_split_string <std::string> (messages_line, ':') };

            if (fields.size() >= 2)
            { const string& f_0 { fields[0] };

// is it a date or a call?
              const string& callsign { (is_digits(f_0) ? fields[1] : fields[0]) };

              string_view msg { remove_peripheral_spaces <std::string_view> (after_first <std::string_view> (messages_line, ':')) };

              if (is_digits(f_0))
                msg = remove_peripheral_spaces <std::string_view> (after_first <std::string_view> (msg, ':'));

              if (!msg.empty())
                individual_messages += { callsign, string { msg } };
            }
          }
        }

        catch (...)
        { cerr << "Unable to read individual messages file: " << context.individual_messages_file() << endl;
          exit(-1);
        }
      }

// INDIVIDUAL QTC COUNT window
      if (send_qtcs)                                                                        // only if it's a contest with QTCs
      { win_individual_qtc_count.init(context.window_info("INDIVIDUAL QTC COUNT"s), WINDOW_NO_CURSOR);
        win_individual_qtc_count <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      }

// INFO window
      win_info.init(context.window_info("INFO"s), WINDOW_NO_CURSOR);
      win_info <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;                                          // make it visible

// LAST QRG window
      win_last_qrg.init(context.window_info("LAST QRG"s), WINDOW_NO_CURSOR);

// LOCAL TIME window
      win_local_time.init(context.window_info("LOCAL TIME"s), WINDOW_NO_CURSOR);

// LOG window
      win_log.init(context.window_info("LOG"s), WINDOW_NO_CURSOR);
      win_log.enable_scrolling();
      win_log.process_input_function(process_LOG_input);

// LOG EXTRACT window; also used for QTCs
      win_log_extract.init(context.window_info("LOG EXTRACT"s), WINDOW_NO_CURSOR);
      log_extract_fg = win_log_extract.fg();
      log_extract_bg = win_log_extract.bg();
      editable_log.prepare();                       // now we can size the editable log
      extract.prepare();

      if (send_qtcs)
        win_log_extract.process_input_function(process_QTC_input);  // set the input function for the window

// MEMORIES window
      win_memories.init(context.window_info("MEMORIES"s), WINDOW_NO_CURSOR);

// MULT VALUE window
      win_mult_value.init(context.window_info("MULT VALUE"s), WINDOW_NO_CURSOR);
      update_mult_value();

// NAME window (for names from drmaster file; not the same as when name is in the exchange)
      win_name.init(context.window_info("NAME"s), WINDOW_NO_CURSOR);

// NEARBY window
      win_nearby.init(context.window_info("NEARBY"s), WINDOW_NO_CURSOR);

// PING window
      win_ping.init(context.window_info("PING"s), WINDOW_NO_CURSOR);

      if (win_ping.valid() and !ping_table_p.empty())
        jthread(update_pings, ref(win_ping), ref(ping_table_p)).detach();

// POST MONITOR window
      win_monitored_posts.init(context.window_info("POST MONITOR"s), WINDOW_NO_CURSOR);
      mp.max_entries(win_monitored_posts.height());               // set the size of the queue

// POSTED BY window
      win_posted_by.init(context.window_info("POSTED BY"s), WINDOW_NO_CURSOR);

// PUTATIVE EXCHANGE window
      win_putative_exchange.init(context.window_info("PUTATIVE EXCHANGE"s), WINDOW_NO_CURSOR);

// QTC HINT window
      win_qtc_hint.init(context.window_info("QTC HINT"s), WINDOW_NO_CURSOR);
      win_qtc_hint_fg = win_qtc_hint.fg();
      win_qtc_hint_bg = win_qtc_hint.bg();
  
// QUERY 1 window
      win_query_1.init(context.window_info("QUERY 1"s), WINDOW_NO_CURSOR);

// QUERY N window
      win_query_n.init(context.window_info("QUERY N"s), WINDOW_NO_CURSOR);

// QUICK QSY window
      win_quick_qsy.init(context.window_info("QUICK QSY"s), WINDOW_NO_CURSOR);
    
      { const auto& [ freq, m ] { quick_qsy_map.at(current_band) };

        win_quick_qsy < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE
                      <= pad_left(freq.display_string(), 7) + SPACE_STR + MODE_NAME[m];
      }
  
// QSLs window
      win_qsls.init(context.window_info("QSLS"s), WINDOW_NO_CURSOR);
      update_qsls_window();

// QSO NUMBER window
      win_qso_number.init(context.window_info("QSO NUMBER"s), WINDOW_NO_CURSOR);
      win_qso_number <= pad_left(to_string(next_qso_number), win_qso_number.width());

// QTC QUEUE window
      win_qtc_queue.init(context.window_info("QTC QUEUE"s), WINDOW_NO_CURSOR);

// QTC STATUS window
      win_qtc_status.init(context.window_info("QTC STATUS"s), WINDOW_NO_CURSOR);
      win_qtc_status <= "Last QTC: None";

// RATE window
      win_rate.init(context.window_info("RATE"s), WINDOW_NO_CURSOR);
      update_rate_window();

// RECORDING STATUS window
      win_recording_status.init(context.window_info("RECORDING STATUS"s), WINDOW_NO_CURSOR);
      update_recording_status_window();

// REMAINING CALLSIGN MULTS window
      win_remaining_callsign_mults.init(context.window_info("REMAINING CALLSIGN MULTS"s), COLOUR_WHITE, COLOUR_BLUE, WINDOW_NO_CURSOR);

      if (restored_data)
        update_remaining_callsign_mults_window(statistics, EMPTY_STR, current_band, current_mode);
      else
        win_remaining_callsign_mults <= (context.remaining_callsign_mults_list());

// REMAINING COUNTRY MULTS window
      win_remaining_country_mults.init(context.window_info("REMAINING COUNTRY MULTS"s), COLOUR_WHITE, COLOUR_BLUE, WINDOW_NO_CURSOR);
      update_remaining_country_mults_window(statistics, current_band, current_mode);

// REMAINING EXCHANGE MULTS window(s)
      { const string         window_name_start          { "REMAINING EXCHANGE MULTS"s };
        const vector<string> exchange_mult_window_names { context.window_name_contains(window_name_start) };

        for (auto& window_name : exchange_mult_window_names)
        { const string exchange_mult_name { substring <std::string> (window_name, window_name_start.size() + 1 /* 25 */)  }; // skip the first part of the window name

          window* wp { new window() };

          wp->init(context.window_info(window_name), COLOUR_WHITE, COLOUR_BLUE, WINDOW_NO_CURSOR);
          win_remaining_exch_mults_p += { exchange_mult_name, wp };

          (*wp) <= rules.exch_canonical_values(exchange_mult_name);                                   // display all the canonical values (which are in alphabetical order)
        }
      }

// RIG window (rig status)
      win_rig.init(context.window_info("RIG"s), WINDOW_NO_CURSOR);

// RX ANT window
      win_rx_ant.init(context.window_info("RX ANT"s), WINDOW_NO_CURSOR);

// SCORE window
      win_score.init(context.window_info("SCORE"s), WINDOW_NO_CURSOR);
      update_score_window(statistics.points(rules));

// SCORE BANDS window
      win_score_bands.init(context.window_info("SCORE BANDS"s), WINDOW_NO_CURSOR);
      { string bands_str { };

        FOR_ALL(rules.score_bands(), [&bands_str] (const BAND b) { bands_str += (BAND_NAME[b] + SPACE_STR); } );
        //const string bands_str { SR::fold_left(rules.score_bands(), string { }, [] (const BAND b) { return (BAND_NAME[b] + SPACE_STR); } ) };   // but how to define plus?

        win_score_bands < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < "Score Bands: "s <= bands_str;
      }

// SCORE MODES window
      win_score_modes.init(context.window_info("SCORE MODES"s), WINDOW_NO_CURSOR);
      { string modes_str { };

        FOR_ALL(rules.score_modes(), [&modes_str] (const MODE m) { modes_str += (MODE_NAME[m] + SPACE_STR); } );

        win_score_modes < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < "Score Modes: "s <= modes_str;
      }

// SCP window
      win_scp.init(context.window_info("SCP"s), WINDOW_NO_CURSOR);

// SCRATCHPAD window
      win_scratchpad.init(context.window_info("SCRATCHPAD"s), WINDOW_NO_CURSOR);
      win_scratchpad.enable_scrolling();

// SENT RST window
//  win_sent_rst.init(context.window_info("SENT RST"), WINDOW_NO_CURSOR);

// SERIAL NUMBER window
      win_serial_number.init(context.window_info("SERIAL NUMBER"s), WINDOW_NO_CURSOR);
      win_serial_number <= pad_left(serial_number_string(octothorpe), win_serial_number.width());

// SRSS window
      win_srss.init(context.window_info("SRSS"s), WINDOW_NO_CURSOR);
      win_srss <= ( "SR/SS: "s + sunrise(my_latitude, my_longitude) + "/"s + sunset(my_latitude, my_longitude) );

// SUMMARY window
      win_summary.init(context.window_info("SUMMARY"s), COLOUR_WHITE, COLOUR_BLUE, WINDOW_NO_CURSOR);
      display_statistics(statistics.summary_string(rules));

// SYSTEM MEMORY window
      win_system_memory.init(context.window_info("SYSTEM MEMORY"s), COLOUR_BLACK, COLOUR_GREEN, WINDOW_NO_CURSOR);
      update_system_memory();

// TIME window
      win_time.init(context.window_info("TIME"s), WINDOW_NO_CURSOR);  // WHITE / BLACK are default anyway, so don't actually need them

// TITLE window
      win_title.init(context.window_info("TITLE"s), COLOUR_BLACK, COLOUR_GREEN, WINDOW_NO_CURSOR);
      win_title <= centre(context.contest_name(), 0);

// WICM window
      win_wicm.init(context.window_info("WICM"s), WINDOW_NO_CURSOR);
      if (win_wicm.valid())
        wicm_calls_size = win_wicm.height();

// WPM window
      if (rules.permitted_modes().contains(MODE_CW))                                 // don't have a WPM window if CW is not permitted, even if the window is defined in the config file
      { win_wpm.init(context.window_info("WPM"s), WINDOW_NO_CURSOR);
        win_wpm <= ( to_string(context.cw_speed()) + " WPM"s );

        if (cw_p)
          cw_p->speed(context.cw_speed());                    // set computer keyer speed
      }

// possibly set the auto country mults and auto callsign mults thresholds
      if (context.auto_remaining_callsign_mults())
        FOR_ALL(rules.callsign_mults(), [threshold = context.auto_remaining_callsign_mults_threshold()] (const string& callsign_mult_name) { acc_callsigns[callsign_mult_name].threshold(threshold); } );

      if (auto_remaining_country_mults)
        acc_countries.threshold(context.auto_remaining_country_mults_threshold());

// possibly set speed of internal keyer. Direct quote from N2IC: Contesters don't use internal keyers. [Message-ID: <515996A2.9060800@arrl.net>]
      try
      { if (context.sync_keyer())
          rig.keyer_speed(context.cw_speed());
      }

      catch (const rig_interface_error& e)
      { alert("Error setting CW speed on rig"s);
      }

      display_band_mode(win_band_mode, current_band, current_mode);

// start to display the date and time
      jthread(display_date_and_time).detach();

// start to display the rig status (in the RIG window); also get rig frequency for bandmap
      jthread(display_rig_status, 1000ms /* poll time */, &rig).detach();

// CLUSTER MULT window
      win_cluster_mult.init(context.window_info("CLUSTER MULT"s), WINDOW_NO_CURSOR);
      win_cluster_mult.enable_scrolling();

// CLUSTER SCREEN window
      win_cluster_screen.init(context.window_info("CLUSTER SCREEN"s), WINDOW_NO_CURSOR);
      win_cluster_screen.enable_scrolling();

// RBN LINE window
      win_rbn_line.init(context.window_info("RBN LINE"s), WINDOW_NO_CURSOR);

// BANDMAP window
      win_bandmap.init(context.window_info("BANDMAP"s), WINDOW_NO_CURSOR);

// set recent and fade colours for each bandmap
      FOR_ALL(bandmaps, [rc = context.bandmap_recent_colour(), fc = context.bandmap_fade_colours()] (bandmap& bm) { bm.fade_colours(fc);
                                                                                                                    bm.recent_colour(rc);
                                                                                                                  } );

// create thread to prune the bandmaps every minute
      jthread(prune_bandmap, &win_bandmap, &bandmaps).detach();

// BANDMAP FILTER window
      win_bandmap_filter.init(context.window_info("BANDMAP FILTER"s), WINDOW_NO_CURSOR);

// set up correct colours for bandmap filter window
      if (!context.bandmap_filter_enabled())
        win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_disabled_colour());
      else
        win_bandmap_filter.default_colours(win_bandmap_filter.fg(), (context.bandmap_filter_hide() ? context.bandmap_filter_hide_colour() : context.bandmap_filter_show_colour()));

      BAND cur_band { current_band };
      MODE cur_mode { current_mode };

      if (bandmaps.size() > static_cast<int>(cur_band))     // should always be true; test is to ensure next line is OK
      { bandmap& bm { bandmaps[cur_band] };                   // use map for current band, so column offset is correct

        bm.filter_enabled(context.bandmap_filter_enabled());
        bm.filter_hide(context.bandmap_filter_hide());

        FOR_ALL( context.bandmap_filter(), [&bm] (const string& filter) { bm.filter_add_or_subtract(filter); } );  // incorporate each filter string

        display_bandmap_filter(bm);
      }

// BANDMAP SIZE window
      win_bandmap_size.init(context.window_info("BANDMAP SIZE"s), WINDOW_NO_CURSOR);

// if any windows overlap, alert the user and exit
     if (windows_overlap)
     { alert("ERROR: WINDOWS OVERLAP; consult log or stderr file for details.");

       sleep_for(5s);

       exit_drlog();
     }

// read a Cabrillo log
  //  logbook cablog;

  /*
    CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1 
   */
  /*
  vector<string> cabrillo_fields;
  cabrillo_fields.push_back("FREQ");
  cabrillo_fields.push_back("MODE");
  cabrillo_fields.push_back("DATE");
  cabrillo_fields.push_back("TIME");
  cabrillo_fields.push_back("TCALL");
  cabrillo_fields.push_back("TEXCH-RST");
  cabrillo_fields.push_back("TEXCH-IOTA");
  cabrillo_fields.push_back("RCALL");
  cabrillo_fields.push_back("REXCH-RST");
  cabrillo_fields.push_back("REXCH-IOTA");
  cabrillo_fields.push_back("TXID");
   */

  // const vector<string> cabrillo_fields = context.cabrillo_qso_fields();

  //  cablog.read_cabrillo("/home/n7dr/tmp/dr/iota.log", context.cabrillo_qso_template());
  //  win_call < WINDOW_CLEAR < CURSOR_START_OF_LINE <= "GW4AMJ";

  //  vector<QSO> vec = cablog.as_vector();
  //  win_call < WINDOW_CLEAR < CURSOR_START_OF_LINE <= vec.size();

// backup the last-used log, if one exists
      if (const string filename { context.logfile() }; file_exists(filename))
      { int index { 0 };

        while (file_exists(filename + "-"s + to_string(index)))
          index++;

        file_copy(filename, filename + "-"s + to_string(index));
      }

      const bool clean   { cl.parameter_present("-clean"s) };
      const bool rebuild { !clean };                            // => rebuild is the default

// now we can restore data from the last run
//  if (!cl.parameter_present("-clean"s))
//  { if (!cl.parameter_present("-rebuild"s))    // if -rebuild is present, then don't restore from archive; rebuild only from logbook
//    { const string archive_filename { context.archive_name() };

//      if (file_exists(archive_filename) and !file_empty(archive_filename))
//        restore_data(archive_filename);
//      else
//        alert("No archive data present"s);
//    }
//    else     // rebuild
      if (rebuild)
      { ost << "rebuilding from: " << context.logfile() << endl;

        string file;

        try
        { file = read_file(context.logfile());    // in current directory
        }

        catch (...)
        { alert("Error reading log file: " + context.logfile());
        }

        if (!file.empty())
        { static const string rebuilding_msg { "Rebuilding..."s };

          win_message < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= rebuilding_msg;

          for (auto line : to_lines <std::string_view> (file))
          { QSO qso { allow_for_callsign_mults( QSO { context, line, rules, statistics } ) };

// possibly add the call to the known prefixes
            update_known_callsign_mults(qso.callsign());

// country mults
            update_known_country_mults(qso.callsign(), KNOWN_MULT::FORCE_KNOWN);
            qso.is_country_mult( statistics.is_needed_country_mult(qso.callsign(), qso.band(), qso.mode(), rules) );

// add exchange info for this call to the exchange db
            const vector<received_field>& received_exchange { qso.received_exchange() };

            for (const auto& exchange_field : received_exchange)
            { if (!variable_exchange_fields.contains(exchange_field.name()))
                exchange_db.set_value(qso.callsign(), exchange_field.name(), exchange_field.value());   // add it to the database of exchange fields
            }

            statistics.add_qso(qso, logbk, rules);
            logbk += qso;
            rate += { qso.epoch_time(), statistics.points(rules) };
          }

// rebuild the history
          rebuild_history(logbk, rules, statistics, q_history, rate);

// rescore the log
          rescore(rules);
          update_rate_window();

          rebuild_dynamic_call_databases(logbk);

          if (remove_peripheral_spaces <std::string> (win_message.read()) == rebuilding_msg)    // clear MESSAGE window if we're showing the "rebuilding" message
            win_message <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
        }

// octothorpe
        if (logbk.size() >= 1)
        { if (const QSO& last_qso { logbk[logbk.size()] }; rules.sent_exchange_includes("SERNO"s, last_qso.mode()))   // logbook is wrt 1
            octothorpe = from_string<unsigned int>(last_qso.sent_exchange("SERNO"s)) + 1;
        }
        else
          octothorpe = 1;

// display most-recent lines from log
//        editable_log.recent_qsos(logbk, true);
        editable_log.recent_qsos(logbk, LOG_EXTRACT::DISPLAY);

// correct QSO number (and octothorpe)
        if (!logbk.empty())
        { next_qso_number = logbk[logbk.n_qsos()].number() /* logbook is wrt 1 */  + 1;
          win_qso_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(to_string(next_qso_number), win_qso_number.width());
          win_serial_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(serial_number_string(octothorpe), win_serial_number.width());

// go to band and mode of last QSO
          const QSO& last_qso { logbk[logbk.size()] };
          const BAND b        { last_qso.band() };
          const MODE m        { last_qso.mode() };

          rig.rig_frequency(frequency(last_qso.freq()));
          rig.rig_mode(m);

          current_mode = m;
          current_band = b;

          cur_band = b;
          cur_mode = m;

          rig.base_state();  // turn off RIT, split, sub-rx
        }

        update_remaining_callsign_mults_window(statistics, string(), cur_band, cur_mode);
        update_remaining_country_mults_window(statistics, cur_band, cur_mode);
        update_remaining_exchange_mults_windows(statistics, cur_band, cur_mode);

// QTCs
        if (send_qtcs)
        {
// number of EU QSOs from logbook
          const size_t n_eu_qsos { logbk.filter([] (const QSO& q) { return (q.continent() == "EU"sv); } ).size() };

          try
          { qtc_db.read(context.qtc_filename());    // it is not an error if the file doesn't exist
          }

          catch (const qtc_error& e)
          { ost << "Error reading QTC file: " << e.reason() << endl;
            exit(-1);
          }

          qtc_buf += logbk;  // add all the QSOs in the log to the unsent buffer

          if (n_eu_qsos != qtc_buf.size())
            alert("WARNING: INCONSISTENT NUMBER OF QTC-ABLE QSOS"s);

// move the sent ones to the sent buffer
          const vector<qtc_series>& vec_qs { qtc_db.qtc_db() };    ///< the QTC series

          FOR_ALL(vec_qs, [] (const qtc_series& qs) { qtc_buf.unsent_to_sent(qs); } );

          statistics.qtc_qsos_sent(qtc_buf.n_sent_qsos());
          statistics.qtc_qsos_unsent(qtc_buf.n_unsent_qsos());

          if (!vec_qs.empty())
          { const qtc_series& last_qs { vec_qs[vec_qs.size() - 1] };

            win_qtc_status < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < "Last QTC: "s < last_qs.id() < " to "s <= last_qs.target();
          }

          update_qtc_queue_window();
        }

// display the current statistics
        display_statistics(statistics.summary_string(rules));
        update_score_window(statistics.points(rules));
        update_mult_value();
      }     // end of rebuild

// now delete the archive file if it exists, regardless of whether we've used it
      file_delete(context.archive_name());

      if (clean)                                          // start with clean slate
      { int index { 0 };

        const string target { OUTPUT_FILENAME + "-"s + to_string(index) };

        while (file_exists(target))
          file_delete(OUTPUT_FILENAME + "-"s + to_string(index++));

        file_truncate(context.logfile());
        file_truncate(context.archive_name());

        if (send_qtcs)
          file_truncate(context.qtc_filename());
      }

// now we can start the cluster/RBN threads, since we know what we've worked if this was a rebuild
      if (!context.cluster_server().empty() and !context.cluster_username().empty() and !context.my_ip().empty())
        jthread(spawn_dx_cluster).detach();

// ditto for the RBN
      if (!context.rbn_server().empty() and !context.rbn_username().empty() and !context.my_ip().empty())
        jthread(spawn_rbn).detach();

      enter_sap_mode();                                       // explicitly enter SAP mode
      win_call <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;    // explicitly force the cursor into the call window

// if necessary, wait for the adif3 old log information to finish building
      if (running_old_log_thread)
      { thr.join();
        alert("Completed build of old log"s);
      }
    }

// some testing stuff
//  keyboard.push_key_press("g4amt", 1000);  // hi, Terry

//  XSynchronize(keyboard.display_p(), true);

//  keyboard.test();
////  pthread_t keyboard_thread_id;
////  pthread_create(&keyboard_thread_id, NULL, keyboard_test, NULL);
// jthread(keyboard_test).detach();

 // int pending = XPending(keyboard.display_p());  // need to understand internal: what is the X queue, exactly, and what is the output buffer?

//  ost << "Pending events = " << pending << endl;

//  sleep(5);

//  keyboard.push_key_press('/');

// test SS echange stuff
#if 0
    { string line = "1A NQ0I 71 CO";
      string callsign = "NQ0I";
      vector<string> fields = split_string(line, " ");

      ost << line << endl;
      parsed_ss_exchange pse(callsign, fields);

      ost << pse << endl;

      line = "1A N7DR 71 CO";
      ost << line << endl;
      fields = split_string(line, " ");
      pse = parsed_ss_exchange(callsign, fields);
      ost << pse << endl;

      line = "10A N7DR 71 CO";
      ost << line << endl;
      fields = split_string(line, " ");
      pse = parsed_ss_exchange(callsign, fields);
      ost << pse << endl;

      line = "10A N7DR 81 CO 71";
      ost << line << endl;
      fields = split_string(line, " ");
      pse = parsed_ss_exchange(callsign, fields);
      ost << pse << endl;

      line = "71 CO 10A N7DR";
      ost << line << endl;
      fields = split_string(line, " ");
      pse = parsed_ss_exchange(callsign, fields);
      ost << pse << endl;

      line = "B 71 CO 10 N7DR";
      ost << line << endl;
      fields = split_string(line, " ");
      pse = parsed_ss_exchange(callsign, fields);
      ost << pse << endl;

      line = "B 71 CO 10A N7DR";
      ost << line << endl;
      fields = split_string(line, " ");
      pse = parsed_ss_exchange(callsign, fields);
      ost << pse << endl;

      line = "B 71 CO 10 A N7DR";
      ost << line << endl;
      fields = split_string(line, " ");
      pse = parsed_ss_exchange(callsign, fields);
      ost << pse << endl;

      line = "B 71 CO 10 N7DR 72";
      ost << line << endl;
      fields = split_string(line, " ");
      pse = parsed_ss_exchange(callsign, fields);
      ost << pse << endl;

      sleep_for(seconds(1));
      exit(0);
    }
#endif

// possibly set up the simulator
    if (cl.value_present("-sim"s))
      jthread(simulator_thread, cl.value("-sim"s), cl.value_present("-n"s) ? from_string<int>(cl.value("-n"s)) : 0).detach();

// force multithreaded
    keyboard.x_multithreaded(true);    // because we might perform an auto backup whilst doing other things with the display

    alert("drlog READY"s);

// everything is set up and running. Now we simply loop and process the keystrokes.
    while (1)
    { while (keyboard.empty())
        sleep_for(10ms);

      win_active_p -> process_input(keyboard.pop());    // does nothing if there is nothing in the keyboard buffer
    }
  }

// handle some specific errors that might occur
  catch (const socket_support_error& e)
  { ost << "Socket support error # " << e.code() << "; reason = " << e.reason() << endl;
    exit(-1);
  }

  catch (const drlog_error& e)
  { ost << "drlog error # " << e.code() << "; reason = " << e.reason() << endl;
    exit(-1);
  }
}

/*! \brief          Display band and mode
    \param  win     window in which to display information
    \param  b       band
    \param  m       mode
*/
void display_band_mode(window& win, const BAND b, const enum MODE m)
{ static constinit BAND last_band  { BAND_20 };  // start band
  static constinit MODE last_mode  { MODE_CW };  // start mode
  static constinit bool first_time { true };

  SAFELOCK(band_mode);

  if ((b != last_band) or (m != last_mode) or first_time)
  { first_time = false;
    last_band = b;
    last_mode = m;

    win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= (BAND_NAME[b] + SPACE_STR + MODE_NAME[m]);
  }
}

/*! \brief  Thread function to display the date and time, and perform other periodic functions

     CHangename ot "tick"?
*/
void display_date_and_time(void)
{ const string THREAD_NAME { "display date and time"s };

  start_of_thread(THREAD_NAME);

  int last_second { -1 };                       // so that we can tell when the time has changed

  array<char, 26> buf;                          // buffer to hold the ASCII date/time info; see man page for gmtime()
  string          last_date;                    // ASCII version of the last date

  update_local_time();                          // update the LOCAL TIME window

  while (true)                                  // forever
  { const time_t now { NOW() };                 // get the time from the kernel

    struct tm structured_time;                  // for holding the time

    bool new_second { false };                  // is it a new second?

    gmtime_r(&now, &structured_time);           // convert to UTC in a thread-safe manner

    if (last_second != structured_time.tm_sec)                // update the time if the second has changed
    {
// this is a good opportunity to check for exiting
      { SAFELOCK(thread_check);

        if (exiting)
        { end_of_thread(THREAD_NAME);

          return; //nullptr;
        }
      }

      new_second = true;
      asctime_r(&structured_time, buf.data());                // convert to ASCII

      win_time < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= substring <std::string> (string(buf.data(), 26), 11, 8);  // extract HH:MM:SS and display it

      last_second = structured_time.tm_sec;

// check for dirty bandmap?


// if a new minute, then update rate window, and do other stuff
      if (last_second % 60 == 0)
      { ost << "Time: " << substring <std::string> (string(buf.data(), 26), 11, 8) << endl;

        now_minutes = NOW_MINUTES();        // update global time in minutes

        update_local_time();
        update_rate_window();
        update_mult_value();
        update_bandmap_size_window();

        if (cluster_p and (n_posters_db_cluster.min_posters() != 1))
          n_posters_db_cluster.prune();

        if (rbn_p and (n_posters_db_rbn.min_posters() != 1))
          n_posters_db_rbn.prune();

// prune monitored posts
        { SAFELOCK(monitored_posts);

          if (!mp.empty())
            mp.prune();
        }

// possibly prune dynamic autocorrect databases
        if (dynamic_autocorrect_rbn)
          dad.prune(10);

// possibly run thread to perform auto backup
        if (!exiting and !context.auto_backup_directory().empty())
          jthread(auto_backup, auto_backup_directory, logfile_name, (context.qtcs() ? context.qtc_filename() : string { })).detach();

// possibly execute pings and update ping window
        if (!exiting and win_ping.valid() and !ping_table_p.empty())
          jthread(update_pings, ref(win_ping), ref(ping_table_p)).detach();

// possibly clear alert window
        { SAFELOCK(alert);

          if ( (alert_time != 0) and ( (now - alert_time) > 60 ) )
          { win_message <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
            alert_time = 0;
          }
        }

// possibly update SYSTEM MEMORY window
        if (win_system_memory.wp())
          update_system_memory();

// possibly turn off audio recording
        if ((drlog_mode == DRLOG_MODE::SAP) and allow_audio_recording and (audio_recording_mode == AUDIO_RECORDING::AUTO) and audio.recording() )
        { if (inactivity_time > 0)        // only if value has been set
          { const auto time_since_qso { time_since_last_qso(logbk) };
            const auto time_since_qsy { time_since_last_qsy() };
            const bool inactive_qso   { (time_since_qso > inactivity_time) or logbk.empty() };
            const bool inactive_qsy   { time_since_qsy > inactivity_time };

            if ( inactive_qso and inactive_qsy )
            { stop_recording(audio);
              alert("audio recording halted due to inactivity"s);
              ost << "  time_since_qso = " << time_since_qso << endl;
              ost << "  time_since_qsy = " << time_since_qsy << endl;
            }
          }
        }
      }

      const string dts { date_time_string(SECONDS::NO_INCLUDE) };

// if a new hour, then possibly create screenshot
      if ( (last_second % 60 == 0) and (structured_time.tm_min == 0) )
      { if (!exiting and context.auto_screenshot())
          jthread(auto_screenshot, "auto-screenshot-"s + dts.substr(0, 13) + "-"s + dts.substr(14)).detach();

// possibly run thread to get geomagnetic indices
        if (!exiting and !geomagnetic_indices_command.empty())
          jthread(get_indices, geomagnetic_indices_command).detach();
      }

// if a new day, then update date window
      const string date_string { substring <std::string> (dts, 0, 10) };

      if (date_string != last_date)
      { win_date < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= date_string;

        last_date = date_string;
        ost << "Date: " << date_string << endl;
      }
    }

    sleep_for(new_second ? 800ms : 100ms);
//    sleep_until();
  }
}

/*! \brief  Thread function to display status of the rig

    Also displays bandmap if the frequency changes. The bandmap is actually updated on screen before any change in status
    NB It doesn't matter *how* the rig's frequency came to change; it could be manual
*/
void display_rig_status(const milliseconds poll_period, rig_interface* rigp)
{ const string THREAD_NAME { "display rig status"s };

  start_of_thread(THREAD_NAME);

  display_rig_status_thread_id = my_thread_id();    // make this thread ID globally available

  DRLOG_MODE last_drlog_mode { DRLOG_MODE::SAP };
  bool       locked_warning  { false };            // is a warning about XIT while locked being displayed?

  bandmap_entry be;

// populate the bandmap entry stuff that won't change
  be.callsign(MY_MARKER);
  be.source(BANDMAP_ENTRY_SOURCE::LOCAL);
  be.expiration_time(be.time() + MILLION);    // a million seconds in the future

  while (true)                                // forever
  { try
    { try
      { while ( rigp -> is_transmitting() )  // K3 idiocy: don't poll while transmitting; although this check is not foolproof
          sleep_for(poll_period / 10);
      }

      catch (const rig_interface_error& e)
      { alert("Error communicating with rig during poll loop"s);
        sleep_for(poll_period / 2);                      // wait a (relatively) long time if there was an error
      }

// if it's a K3 we can get a lot of info with just one or two queries -- for now just assume it's a K3
      if (ok_to_poll_k3)
      { const bool is_ssb { current_mode == MODE_SSB };

        constexpr size_t DS_REPLY_LENGTH     { 13 };          // K3 returns 13 characters
        constexpr size_t STATUS_REPLY_LENGTH { 38 };          // K3 returns 38 characters

// force into extended mode -- it's ridiculous to do this all the time, but there seems to be no way to be sure we haven't turned the rig on/off
// currently needed in order to obtain notch info, hence only SSB, but force it anyway
        if ( is_ssb and ( rigp -> k3_command_mode() == K3_COMMAND_MODE::NORMAL ) )  // NB this is the mode drlog /thinks/ is correct, but it saves time
          rigp -> k3_command_mode(K3_COMMAND_MODE::EXTENDED);

//        const string   status_str    { rigp -> raw_command("IF;"s, RESPONSE::EXPECTED, STATUS_REPLY_LENGTH) };               // K3 returns 38 characters
        const string   status_str    { rigp -> raw_command("IF;"s, RESPONSE::EXPECTED /*, STATUS_REPLY_LENGTH */) };               // K3 returns 38 characters
//        const string   ds_reply_str  { is_ssb ? rigp -> raw_command("DS;"s, RESPONSE::EXPECTED, DS_REPLY_LENGTH) : ""s };  // K3 returns 13 characters; currently needed only in SSB
        const string   ds_reply_str  { is_ssb ? rigp -> raw_command("DS;"s, RESPONSE::EXPECTED /*, DS_REPLY_LENGTH */) : ""s };  // K3 returns 13 characters; currently needed only in SSB

        ost << NOW_TP() << ": polled rig status: " << status_str << endl;

        if ( (status_str.length() == STATUS_REPLY_LENGTH) and (ds_reply_str.length() == (is_ssb ? DS_REPLY_LENGTH : 0)) )              // do something only if it's the correct length
        { const frequency  f                  { from_string<double>(substring <std::string> (status_str, 2, 11)) };     // frequency of VFO A
          const frequency  target             { cq_mode_frequency };                                                    // frequency in CQ mode
          const frequency  f_b                { rig.rig_frequency_b() };                                                // frequency of VFO B
          const DRLOG_MODE current_drlog_mode { SAFELOCK_GET(drlog_mode_mutex, drlog_mode) };                           // explicitly set to SAP mode if we have QSYed
          const bool       notch              { is_ssb ? rigp -> notch_enabled(ds_reply_str) : false };   // only needed in SSB

          if ( (current_drlog_mode == DRLOG_MODE::CQ) and (last_drlog_mode == DRLOG_MODE::CQ) and (target != f) )
            enter_sap_mode();                                                                   // switch to SAP if we've moved

          last_drlog_mode = current_drlog_mode;                                                 // keep track of drlog mode

          MODE m { current_mode };                                        // mode as determined by drlog, not by rig

          m = rigp -> rig_mode();      // actual mode of rig (in case there's been a manual mode change); note that this might fail, which is why we set the mode in the prior line

// have we changed band (perhaps manually)?
          if (const BAND sgb { current_band }; sgb != to_BAND(f))
          { ost << "Band mismatch during poll; sgb (current_band) = " << sgb << ", f = " << f << ", BAND(f) = " << to_BAND(f) << endl;

// changing bands is ssslllloooowwww, and maybe the rig is in transition in another thread -- holding mutex locks
// is no good because it's too slow
            bool need_to_set_band { true };

            frequency new_f;
            BAND      new_sgb;

            for ( [[maybe_unused]] int n { 1 }; need_to_set_band and (n != 5); ++n)
            { sleep_for(500ms);

              new_f = rig.rig_frequency();

// 250509
              new_sgb = to_BAND(new_f);
              need_to_set_band = (current_band != to_BAND(new_f));

              ost << "  attempt number " << n << ": new_f = " << new_f << "; new_sgb = " << new_sgb << "; current_band = " << static_cast<BAND>(current_band) << endl;
//              new_sgb = current_band;
//              need_to_set_band = (new_sgb != to_BAND(new_f));
            }

            if (need_to_set_band)               // it looks like this was probably a manual band change
            { ost << "Consistent band mismatch during poll; new_sgb = " << new_sgb << ", new_f = " << new_f << ", BAND(new_f) = " << to_BAND(new_f) << "; setting band" << endl;

              current_band = to_BAND(new_f);

              update_remaining_callsign_mults_window(statistics, string(), current_band, m);
              update_remaining_country_mults_window(statistics, current_band, m);
              update_remaining_exchange_mults_windows(/* rules, */statistics, current_band, m);

              update_based_on_frequency_change(f, m);   // changes windows, including bandmap
            }
          }
          else          // we haven't changed band
          { if (ignore_next_poll)
              ignore_next_poll = false;
            else
              update_based_on_frequency_change(f, m);   // changes windows, including bandmap; NB this is called even if there is no change
          }

// mode: the K3 is its usual rubbish self; sometimes the mode returned by the rig is incorrect
// following a recent change of mode. By the next poll it seems to be OK, though, so for now
// it seems like the effort of trying to work around the (Yet Another Elecraft) bug is not worth it
          constexpr unsigned int RIT_XIT_PM_ENTRY  { 18 };      // position of the RIT/XIT +/- indicator; compiler does not allow "±" in variable names

          constexpr unsigned int RIT_XIT_OFFSET_ENTRY  { 19 };  // position of the RIT/XIT offset
          constexpr unsigned int RIT_XIT_OFFSET_LENGTH { 4 };   // length of the RIT/XIT offset

          constexpr unsigned int RIT_ENTRY   { 23 };             // position of the RIT status byte in the K3 status string
          constexpr unsigned int XIT_ENTRY   { 24 };             // position of the XIT status byte in the K3 status string
          constexpr unsigned int MODE_ENTRY  { 29 };             // position of the mode byte in the K3 status string
          constexpr unsigned int SPLIT_ENTRY { 32 };             // position of the SPLIT status byte in the K3 status string

          constexpr unsigned int RIT_XIT_DISPLAY_LENGTH  { 7 }; // display length of RIT/XIT info

          const char   mode_char  { status_str[MODE_ENTRY] };
          const string mode_str   { ( (mode_char == '1') ? "LSB "s : ( (mode_char == '2') ? "USB "s : ( (mode_char == '3') ? " CW "s : "UNK "s ) ) ) };
          const bool   rit_is_on  { (status_str[RIT_ENTRY] == '1') };
          const bool   xit_is_on  { (status_str[XIT_ENTRY] == '1') };

          string rit_xit_str { };

          if (xit_is_on)
            rit_xit_str += 'X';

          if (rit_is_on)
            rit_xit_str += 'R';

          if (rit_is_on or xit_is_on)
          { string rit_xit_value_str { remove_leading <std::string> (substring <std::string_view> (status_str, RIT_XIT_OFFSET_ENTRY, RIT_XIT_OFFSET_LENGTH), '0') };  // removes leading zeros

            if (rit_xit_value_str.empty())
              rit_xit_value_str = '0';

            rit_xit_str += (status_str[RIT_XIT_PM_ENTRY] + rit_xit_value_str);
            rit_xit_str = pad_left(rit_xit_str, RIT_XIT_DISPLAY_LENGTH);
          }

          if (rit_xit_str.empty())
            rit_xit_str = space_string(RIT_XIT_DISPLAY_LENGTH);

          rig_is_split = (status_str[SPLIT_ENTRY] == '1');

          const string bandwidth_str   { rigp -> bandwidth_str() };
          const string frequency_b_str { f_b.display_string() };
          const string centre_str      { rigp -> centre_frequency_str() };

// now display the status
          win_rig.default_colours(win_rig.fg(), is_marked_frequency(marked_frequency_ranges, m, f) ? COLOUR_RED : COLOUR_BLACK);  // red if the contest doesn't want us to be on this QRG

          const bool is_locked { rigp -> is_locked() };

          if (xit_is_on and is_locked and !locked_warning)
          { locked_warning = true;

            alert("XIT ENABLED WHILE LOCKED; DISABLING");

            rigp -> xit_disable();
          }

          if (locked_warning)
          { if (!xit_is_on or !is_locked)
            { locked_warning = false;

              win_message <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;       // clear the MESSAGE window

// force RIT to be enabled if we are locked
              if (is_locked and !(rigp -> rit_enabled()) )
                rigp -> rit_enable();
            }
          }

          const bool sub_rx    { rigp -> sub_receiver_enabled() };
          const auto fg        { win_rig.fg() };                                          // original foreground colour

          win_rig < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT
                  < ( rig_is_split ? WINDOW_ATTRIBUTES::WINDOW_NOP : WINDOW_ATTRIBUTES::WINDOW_BOLD)
                  < pad_left(f.display_string(), 7)
                  < ( rig_is_split ? WINDOW_ATTRIBUTES::WINDOW_NOP : WINDOW_ATTRIBUTES::WINDOW_NORMAL)
                  < ( is_locked ? "L "s : "  "s )
                  < mode_str
                  < ( rig_is_split ? WINDOW_ATTRIBUTES::WINDOW_BOLD : WINDOW_ATTRIBUTES::WINDOW_NORMAL);

          if (sub_rx)
            win_rig < COLOURS(COLOUR_GREEN, win_rig.bg());

          win_rig < frequency_b_str
                  < ( rig_is_split ? WINDOW_ATTRIBUTES::WINDOW_NORMAL : WINDOW_ATTRIBUTES::WINDOW_NOP);

          if (sub_rx)
            win_rig < COLOURS(fg, win_rig.bg());

          win_rig < WINDOW_ATTRIBUTES::CURSOR_DOWN
                  < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;

          if (rigp -> test())
            win_rig < "T ";

          if (const size_t x_posn { rit_xit_str.find('X') }; x_posn == string::npos)
            win_rig < rit_xit_str < "  ";
          else
            win_rig < substring <std::string> (rit_xit_str, 0, x_posn) < WINDOW_ATTRIBUTES::WINDOW_BOLD < COLOURS(COLOUR_YELLOW, win_rig.bg()) < "X"s
                    < WINDOW_ATTRIBUTES::WINDOW_NORMAL < COLOURS(fg, win_rig.bg()) < substring <std::string> (rit_xit_str, x_posn + 1) < "  ";

           win_rig < centre_str;

// don't change the bandwidth if the rig has returned a ridiculous value, which happens occasionally with the K3 (!!)
          if (bandwidth_str.size() <= 4)
          { win_rig < ":"s < bandwidth_str;

            if (notch)
              win_rig < " N"s;    // effectively, the notch status is part of the bandwidth; we can't just write the N, because we might not have written a bandwidth
          }

          win_rig.refresh();

          SAFELOCK(last_polled_frequency);
          last_polled_frequency = f.display_string();   // store for quick lookup without interrogating the rig
        }

// possibly check the RX ANT status
        update_rx_ant_window();
      }
    }

// be silent if there was an error communicating with the rig
    catch (const rig_interface_error& e)
    {
    }

    sleep_for(poll_period);

    { SAFELOCK(thread_check);

      if (exiting_rig_status)
      { end_of_thread(THREAD_NAME);
        exiting = true;
        return;
      }
    }
  }
}

/*! \brief  Thread to process data from the cluster or the RBN.

    Must start the thread to obtain data before trying to process it with this one;
    pulls the data from the cluster object [and removes the data from it]
*/
void process_rbn_info(window* wclp, window* wcmp, dx_cluster* dcp, running_statistics* statistics_p, location_database* location_database_p, window* win_bandmap_p, BANDMAPS* bandmaps_p)
{ const string THREAD_NAME { "process rbn info"s };

  start_of_thread(THREAD_NAME);

  constexpr int       POLL_SECS { 10 };         // seconds between the end of one processing pass and the start of the next
  constexpr frequency MAX_FREQ_SKEW { 800_Hz };     // maximum change in frequency considered as NOT a QSY

// get access to the information that's been passed to the thread
  window&                          cluster_line_win { *wclp };                 // the window to which we will write each line from the cluster/RBN
  window&                          cluster_mult_win { *wcmp };                 // the window in which to write mults
  dx_cluster&                      rbn              { *dcp };                  // the DX cluster or RBN; this is a misleading name
  running_statistics&              statistics       { *statistics_p };         // the statistics
  location_database&               location_db      { *location_database_p };  // database of locations
  window&                          bandmap_win      { *win_bandmap_p };        // bandmap window
  array<bandmap, NUMBER_OF_BANDS>& bandmaps         { *bandmaps_p };           // bandmaps

  const bool is_rbn                 { (rbn.source() == POSTING_SOURCE::RBN) };
  const bool is_cluster             { !is_rbn };
  const bool rbn_beacons            { context.rbn_beacons() };
  const int  my_cluster_mult_colour { string_to_colour("COLOUR_17"sv) }; // the colour of my call in the CLUSTER MULT window

  string                         unprocessed_input;     // data from the cluster that have not yet been processed by this thread
  deque<pair<string, frequency>> recent_mult_calls;     // the queue of recent calls posted to the mult window (can't be a std::queue)

  const int highlight_colour { static_cast<int>(colours.add(COLOUR_WHITE, COLOUR_RED)) };             // colour that will mark that we are processing a ten-second pass
  const int original_colour  { static_cast<int>(colours.add(cluster_line_win.fg(), cluster_line_win.bg())) };

  if (is_cluster)
    win_cluster_screen < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_BOTTOM_LEFT;  // probably unused

  while (1)                                                 // forever; process a POLL_INTERVAL pass
  { set<BAND> changed_bands                { };             // the bands that have been changed by this ten-second pass
    bool      cluster_mult_win_was_changed { false };       // has cluster_mult_win been changed by this pass?

    string last_processed_line { };                             // the last line processed during this pass

    const string new_input { rbn.get_unprocessed_input() }; // get any unprocessed info from the cluster; deletes the data from the cluster

    posted_by_vector.clear();                               // prepare the posted_by vector

// a visual marker that we are processing a pass; this should appear only briefly
    const string win_contents { cluster_line_win.read() };
    const char   first_char   { (win_contents.empty() ? ' ' : win_contents[0]) };

    cluster_line_win < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < colour_pair(highlight_colour) < first_char <= colour_pair(original_colour);

    if (is_cluster and !new_input.empty())
    { const string              no_cr { remove_char(new_input, CR_CHAR) };
      const vector<string_view> lines { to_lines <std::string_view> (no_cr) };

// I don't understand why the scrolling occurs automatically... in particular, I don't know what causes it to scroll
      for (size_t n { 0 }; n < lines.size(); ++n)
      { win_cluster_screen < lines[n];                       // THIS causes the scroll, but I don't know why

        if ( (n != (lines.size() - 1)) or no_cr.ends_with(LF_CHAR) )
          win_cluster_screen < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
        else
          win_cluster_screen < WINDOW_ATTRIBUTES::WINDOW_SCROLL_DOWN;

        win_cluster_screen < WINDOW_ATTRIBUTES::WINDOW_REFRESH;
      }
    }

    if (new_input.empty())
    { const auto time_since_data_last_received { rbn.time_since_data_last_received() };

      if (time_since_data_last_received > 60s)
      { const string msg       { "NO DATA RECEIVED FOR "s + to_string(N_SECONDS(time_since_data_last_received)) + " SECONDS"s };
        const int    bg_colour { cluster_line_win.bg() };
        const int    fg_colour { cluster_line_win.fg() };

        ost << (is_cluster ? "CLUSTER" : "RBN") << ": " << msg << endl;

        cluster_line_win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < COLOURS(COLOUR_RED, COLOUR_BLACK) < centre(msg, 0) <= COLOURS(fg_colour, bg_colour);

// insert connection reset here if time_since_data_last_received.count() is "too large"
        if (time_since_data_last_received > context.cluster_timeout())
        { ost << "WARNING: cluster timeout exceeded; connection status = " << endl
                                                                           << "----------" << endl
                                                                           << rbn.connection_status()
                                                                           << "----------" << endl;

          rbn.reset_connection();

          ost << "attempted to reset connection; connection_status = " << endl
                                                                       << "----------" << endl
                                                                       << rbn.connection_status()
                                                                       << "----------" << endl;
        }
        else
          ost << "cluster timeout of " << context.cluster_timeout() << " seconds not yet exceeded; reset not attempted" <<  endl;
      }
    }

    unprocessed_input += move(new_input);         // append the new unprocessed data

    string_view unprocessed_input_sv { unprocessed_input };

    while (contains(unprocessed_input_sv, CRLF))                               // look for EOL markers
    { const size_t      posn { unprocessed_input_sv.find(CRLF) };                   // guaranteed to succeed
      const string_view line { substring <std::string_view> (unprocessed_input_sv, 0, posn) };      // store the next unprocessed line

      unprocessed_input_sv = substring <std::string_view> (unprocessed_input_sv, line.length() + 2);  // delete the line (including the CRLF) from the unprocessed buffer

      if (!line.empty())
      { static const vector<string_view> beacon_markers { " BCN ",
                                                          " BEACON ",
                                                          "/B ",
                                                          "/B2 ",
                                                          " NCDXF "
                                                        };

        if (rbn_file.is_open())
          rbn_file << line << endl;

        if (!rbn_beacons and ANY_OF(beacon_markers, [&line] (const string_view sv) { return contains(line, sv); }))   // if beacon and we don't want it
        { rbn.increment_n_posts();        // keep track of the number of posts processed from the cluster/rbn
          continue;                       // go to end of while loop; these are not counted as posts
        }

        last_processed_line = line;

// display if this is a new mult on any band, and if the poster is on our own continent
        dx_post post { line, location_db, rbn.source() };                // no longer const to allow for RBN autocorrection

        rbn.increment_n_posts();        // keep track of the number of posts processed from the cluster/rbn

        const bool wrong_mode { (is_rbn and (!post.mode_str().empty() and (post.mode_str() != "CW"sv))) or
                                (is_cluster and !cluster_cw and (putative_mode(post.freq()) == MODE_CW))
                              };      // don't process if RBN and not CW, or cluster and CW and CW is not to be processed

        if (post.valid() and !wrong_mode)                   // a valid post in the correct mode
        { if (is_rbn and autocorrect_rbn)                   // possibly autocorrect
            post.callsign(ac_db.corrected_call(post.callsign()));

// eliminate some obvious busts
          const string call  { post.callsign() };
          const char   first { call[0] };

          if ( (first == '/') or (first == 'Q') or (first == '0') )    // 1A0C is a valid call
            continue;

          if (call.ends_with('/'))
            continue;

          const BAND dx_band { post.band() };

// is this station being monitored? *** move this after correction? and correct call in post if necessary?? ***
          if (mp.is_monitored(post.callsign()))
            mp += post;

          if (permitted_bands_set.contains(dx_band))      // process only if is on a band we care about
          { if (is_rbn and dynamic_autocorrect_rbn)       // possibly apply dynamic autocorrection
            { if (!dad.contains_band(dx_band))            // add the band if this is the very first occurrence of this band
                dad += dx_band;

              dad += post;                   // add it even though it might be wrong... wrongness is determined by voting

              const string old_call { post.callsign() };

              post.callsign(dad.autocorrect(post));       // replace call in the post, so that subsequent reads return the possibly-autocorrected call

              if (post.callsign() != old_call)
                ost << "RBN DX call " << old_call << " autocorrected to " << post.callsign() << " on " << BAND_NAME[post.band()] << "m" << endl;
            }

            const BAND                    cur_band    { current_band };
            const string                  dx_callsign { post.callsign() };
            const string                  poster      { post.poster() };
            const pair<string, frequency> target      { dx_callsign, post.freq() };
            const bool                    is_me       { (dx_callsign == my_call) };

            const auto& [target_call, target_freq ] { target };

// POSTED BY
            if (is_me and is_rbn)
            { const bool add_post { ( posted_by_continents.empty() ? (post.poster_continent() != my_continent) : (posted_by_continents.contains(post.poster_continent())) ) };

              if (add_post)
                posted_by_vector += post;
            }

// Possibly process WICM info
            { if ( (dx_band == cur_band) and (drlog_mode == DRLOG_MODE::CQ) and (current_mode == MODE_CW) and !is_me and (post.freq().difference(cq_mode_frequency) <= 200_Hz) )
              { SAFELOCK(wicm);

                if (!contains(wicm_calls, dx_callsign))
                { wicm_calls += dx_callsign;

                  while (wicm_calls.size() > wicm_calls_size) // should just be one too many
                    --wicm_calls;

                  wicm_calls_is_dirty = true;
                }
              }
            }

// record as the most recent QRG for this station
            const int band_nr { static_cast<int>(BAND(post.freq())) };

            UNORDERED_STRING_MAP<string>& qrg_map { last_posted_qrg[band_nr] };

            { lock_guard lck(last_posted_qrg_mutex[band_nr]);

              qrg_map[dx_callsign] = post.frequency_str();
            }

// generate a bandmap_entry for this post
            bandmap_entry be { post.from_cluster() ? BANDMAP_ENTRY_SOURCE::CLUSTER : BANDMAP_ENTRY_SOURCE::RBN };

            be.callsign(dx_callsign);
            be.freq(post.freq());        // also sets band and mode

            if (rules.score_modes().contains(be.mode()))
            { be.frequency_str_decimal_places(1);
              be.expiration_time(be.time() + (post.from_cluster() ? bandmap_decay_time_cluster_secs : bandmap_decay_time_rbn_secs) ); // don't rely on the time in the post
              be.is_needed( is_needed_qso(dx_callsign, dx_band, be.mode()) );             // do we still need this guy?

// update known mults before we test to see if this is a needed mult

// possibly add the call to the known prefixes
              update_known_callsign_mults(dx_callsign);

// possibly add the call to the known countries
              if (auto_remaining_country_mults)
                update_known_country_mults(dx_callsign);

// possibly add exchange mult value
              const vector<string> exch_mults { rules.expanded_exchange_mults() };                                      // the exchange multipliers

              for (const auto& exch_mult_name : exch_mults)
              { if (context.auto_remaining_exchange_mults(exch_mult_name))           // this means that for any mult that is not completely determined, it needs to be listed in AUTO REMAINING EXCHANGE MULTS
// *** consider putting the regex into the multiplier object (in addition to the list of known values)
                { const vector<string> exchange_field_names       { rules.expanded_exchange_field_names(be.canonical_prefix(), be.mode()) };
                  const bool           is_possible_exchange_field { contains(exchange_field_names, exch_mult_name) };

                  if (is_possible_exchange_field)
                  { if (const string guess { exchange_db.guess_value(dx_callsign, exch_mult_name) }; !guess.empty())
                    { if ( statistics.add_known_exchange_mult(exch_mult_name, MULT_VALUE(exch_mult_name, guess)) )
                        update_remaining_exch_mults_window(exch_mult_name, statistics, current_band, current_mode);    // update if we added a new value of the mult
                    }
                  }
                }
              }

              be.calculate_mult_status(rules, statistics);

              bool is_recent_call { false };

              for (const auto& [recent_mult_call, recent_mult_freq] : recent_mult_calls)      // look to see if this is already in the deque
                if (!is_recent_call)
//                  is_recent_call = (recent_mult_call == target_call) and (target_freq.difference(recent_mult_freq).hz() <= MAX_FREQ_SKEW); // allow for frequency skew
                  is_recent_call = (recent_mult_call == target_call) and (target_freq.difference(recent_mult_freq) <= MAX_FREQ_SKEW); // allow for frequency skew

              const bool is_interesting_mode { (rules.score_modes().contains(be.mode())) };

// CLUSTER MULT window
              if (cluster_mult_win.defined())
              { if (is_interesting_mode and !is_recent_call and (be.is_needed_callsign_mult() or be.is_needed_country_mult() or be.is_needed_exchange_mult() or is_me))  // if it's a mult and not recently posted...
                { if (location_db.continent(poster) == my_continent)                                      // heard on our continent?
                  { const size_t QUEUE_SIZE { static_cast<size_t>(cluster_mult_win.height()) };           // make the queue length the same as the height of the window

                    cluster_mult_win_was_changed = true;             // keep track of the fact that we're about to write changes to the window
                    recent_mult_calls += target;

                    while (recent_mult_calls.size() > QUEUE_SIZE)    // keep the list of recent calls to a reasonable size
                      --recent_mult_calls;

                    cluster_mult_win < WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT < WINDOW_ATTRIBUTES::WINDOW_SCROLL_DOWN;

                    const int bg_colour { cluster_mult_win.bg() };
                    const int fg_colour { cluster_mult_win.fg() };

                    if (is_me)
                      cluster_mult_win < COLOURS(COLOUR_YELLOW, my_cluster_mult_colour);  // darkish blue

                    const string frequency_str { pad_left(be.frequency_str(), 7) };

// highlight it if it's on our current band
                    if ( (dx_band == cur_band) or is_me)
                      cluster_mult_win < WINDOW_ATTRIBUTES::WINDOW_HIGHLIGHT;       // swaps fg/bg

                    if (is_me)
                      cluster_mult_win < WINDOW_ATTRIBUTES::WINDOW_BOLD;            // call in bold darkish blue

                    cluster_mult_win < pad_right(frequency_str + SPACE_STR + dx_callsign, cluster_mult_win.width());  // display it -- removed refresh

                    if (is_me)
                      cluster_mult_win < COLOURS(fg_colour, bg_colour);

                    if ( (dx_band == cur_band) or is_me)
                      cluster_mult_win < WINDOW_ATTRIBUTES::WINDOW_NORMAL;
                  }
                }
              }

// add the post to the correct bandmap unless it's a marked frequency
              if ( is_interesting_mode and (bandmap_show_marked_frequencies or !is_marked_frequency(marked_frequency_ranges, be.mode(), be.freq())) )
              { auto insert_be { [&changed_bands] (const BAND dx_band, const bandmap_entry& be) { bandmap_insertion_queues[dx_band] += be;
                                                                                                  changed_bands += dx_band;      // mark band as changed
                                                                                                } };

                switch (be.source())
                { case BANDMAP_ENTRY_SOURCE::CLUSTER :
                  case BANDMAP_ENTRY_SOURCE::RBN :
                  { n_posters_database* dbp { (be.source() == BANDMAP_ENTRY_SOURCE::CLUSTER) ? &n_posters_db_cluster : &n_posters_db_rbn };   // choose correct n_posters database

                    (*dbp) += { be.callsign(), post.poster() };        // associate poster with the call

                    if (dbp -> test_call(be.callsign()))              // if good call
                      insert_be(dx_band, be);

                    break;
                  }

                  default :                                       // neither cluster nor RBN
                    insert_be(dx_band, be);
                }
              }
            }
          }
        }   // no else; if it's an invalid post, do nothing
      }
    }

    unprocessed_input = unprocessed_input_sv;       // copy to the original owner of the data

// update displayed bandmap if there was a change
    const BAND cur_band { current_band };

/// THINK ABOUT IF WE HAVE CHANGED BANDS WHILE PROCESSING THIS... => DISPLAY DIFFERENT BAND

// perhaps don't display anything if we've changed bands
        while (ignore_next_process_insertion_queue)
        { ignore_next_process_insertion_queue = false;
          ost << NOW_TP() << ": pausing ALL processing insertion queue" << endl;
          sleep_for(1s);
        }

    for (const BAND b : changed_bands)
    { if (b == cur_band)
      { ost << NOW_TP() << ": preparing to process insertion queue for " << (is_rbn ? "RBN"sv : "CLUSTER"sv) << endl;

        while (ignore_next_process_insertion_queue)
        { ignore_next_process_insertion_queue = false;
          ost << NOW_TP() << ": delaying processing insertion queue" << endl;
          sleep_for(1s);
        }

        ost << NOW_TP() << ": processing insertion queue for " << (is_rbn ? "RBN"sv : "CLUSTER"sv) << endl;
        bandmaps[b].process_insertion_queue(bandmap_insertion_queues[b], bandmap_win);
      }
      else
        bandmaps[b].process_insertion_queue(bandmap_insertion_queues[b]);
    }

    if (cluster_mult_win_was_changed)    // update the window on the screen
      cluster_mult_win.refresh();

// possibly update WICM window
    { SAFELOCK(wicm);

      if (wicm_calls_is_dirty)
      { win_wicm < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

        unsigned int y { static_cast<unsigned int>(win_wicm.height() - 1) };

        for (const auto& wicm_call : wicm_calls)
        { win_wicm < cursor(0, y--);
          win_wicm < wicm_call;
        }

        win_wicm.refresh();
        wicm_calls_is_dirty = false;
      }
    }

// update monitored posts if there was a change
    if (mp.is_dirty())
    { const deque<monitored_posts_entry> entries { mp.entries() };

      win_monitored_posts < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

      unsigned int y { static_cast<unsigned int>( (win_monitored_posts.height() - 1) - (entries.size() - 1) ) }; // oldest entry

      const PAIR_NUMBER_TYPE default_colours { colours.add(win_monitored_posts.fg(), win_monitored_posts.bg()) };

// oldest to newest
      for (const monitored_posts_entry& entry : entries)
      { win_monitored_posts < cursor(0, y++);

        static const value_map monitored_posts_vm(fade_colours, minutes { 0 }, duration_cast<minutes> (MONITORED_POSTS_DURATION));

        auto age = [] (const monitored_posts_entry& mpe) { return duration_cast<minutes> (NOW_TP() - (mpe.expiration() - MONITORED_POSTS_DURATION) ); };

        const auto             clr { monitored_posts_vm[age(entry)] };
        const PAIR_NUMBER_TYPE cpu { colours.add(clr, win_monitored_posts.bg()) };

        win_monitored_posts < colour_pair(cpu)
                            < entry.to_string() < colour_pair(default_colours);
      }

      win_monitored_posts.refresh();
    }

// remove marker that we are processing a pass
// we assume that the height of cluster_line_win is one
    if (last_processed_line.empty())
      cluster_line_win < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= first_char;
    else
      cluster_line_win < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= last_processed_line;  // display the last processed line on the screen

    if (auto_remaining_country_mults)
      update_remaining_country_mults_window(statistics, current_band, current_mode);  // might have added a new one if in auto mode

    if (!posted_by_vector.empty())
      update_win_posted_by(posted_by_vector);

// see also repeat() suggestion at: https://stackoverflow.com/questions/17711655/c11-range-based-for-loops-without-loop-variable
    for ( [[maybe_unused]] auto _ : RANGE(1, POLL_SECS) )    // wait POLL_SECS seconds before getting any more unprocessed info
    {
      { SAFELOCK(thread_check);

        if (exiting)
        { if (rbn_file.is_open())
            rbn_file.close();

          ost << "Number of posts processed by " << ( (rbn.source() == POSTING_SOURCE::CLUSTER) ? "CLUSTER"s : "RBN"s ) << " in processing pass = " << css(rbn.n_posts()) << endl;

          end_of_thread(THREAD_NAME);
          return;
        }
      }

      sleep_for(1s);
    }
  }
}

/// thread function to obtain data from the cluster
void get_cluster_info(dx_cluster* cluster_p)
{ constexpr int READ_INTERVAL_SEC { 2 };                  // number of seconds to pause between reads from the socket; longer pauses can accumulate large numbers of posts

  const string THREAD_NAME { "get cluster info"s };

  start_of_thread(THREAD_NAME);

  dx_cluster& cluster { *cluster_p };    // make the cluster available

  while (1)                                                 // forever
  { cluster.read();                                         // reads the socket and stores the data inside the cluster object

    for ( [[maybe_unused]] auto _ : RANGE(1, READ_INTERVAL_SEC) )    // check the socket every READ_INTERVAL_SEC seconds
    {
      { SAFELOCK(thread_check);

        if (exiting)
        { end_of_thread(THREAD_NAME);
          return;
        }
      }

      sleep_for(1s);
    }
  }
}

/// thread function to prune the bandmaps once per minute
void prune_bandmap(window* win_bandmap_p, array<bandmap, NUMBER_OF_BANDS>* bandmaps_p)
{ constexpr int PRUNE_INTERVAL_SEC { 60 };

  const string THREAD_NAME { "prune bandmap"s };

  start_of_thread(THREAD_NAME);

  window&                          bandmap_win { *win_bandmap_p };         // bandmap window
  array<bandmap, NUMBER_OF_BANDS>& bandmaps    { *bandmaps_p };            // bandmaps

  while (1)
  { FOR_ALL(bandmaps, [] (bandmap& bm) { bm.prune(); } );

    bandmap_win <= bandmaps[current_band];

    for ( [[maybe_unused]] auto _ : RANGE(1, PRUNE_INTERVAL_SEC) )    // check once per second for a minute
    {
      { SAFELOCK(thread_check);

        if (exiting)
        { end_of_thread(THREAD_NAME);
          return;
        }
      }

      sleep_for(1s);
    }
  }
}

// -------------------------------------------------  functions to process input to various windows  -----------------------------

/*! \brief      Process input to the CALL window
    \param  wp  pointer to window associated with the event
    \param  e   keyboard event to process
*/
/*  KP numbers    -- CW messages
    ALT-KPDel     -- Add call to single-band DO NOT SHOW list
    ALT-C         -- send post to cluster
    ALT-D         -- screenshot and dump all bandmaps to output file [for debugging purposes]
    ALT-G go to the frequency in win_last_qrg
    ALT-K         -- toggle CW
    ALT-M         -- change mode
    ALT-N         -- toggle notch status if on SSB
    ALT-P         -- Dump P3
    ALT-Q         -- send QTC
    ALT-R         -- toggle RX antenna
    ALT-X         -- enter zoomed XIT mode
    ALT-Y         -- delete last QSO
    ALT-KP_4      -- decrement bandmap column offset
    ALT-KP_6      -- increment bandmap column offset
    ALT-KP+       -- increment octothorpe
    ALT-KP-       -- decrement octothorpe
    ALT-CTRL-LEFT-ARROW, ALT-CTRL-RIGHT-ARROW: up or down to next stn with zero QSOs on this band and mode. Uses filtered bandmap
    ALT-CTRL-KP-LEFT-ARROW, ALT-CTRL-KP-RIGHT-ARROW: up or down to next stn with zero QSOs, or who has previously QSLed on this band and mode. Uses filtered bandmap
    ALT-CTRL-KP-DOWN-ARROW, ALT-CTRL-KP-UP-ARROW: up or down to next stn that matches the N7DR criteria
    ALT-F4        -- toggle DEBUG state
    ALT-<-        -- VFO B -> VFO A
    ALT-->        -- VFO A -> VFO B
    BACKSLASH     -- send to the scratchpad
    CTRL-A        -- immediate termination, if enabled
    CTRL-B        -- fast bandwidth
    CTRL-C        -- EXIT (same as .QUIT)
    CTRL-F        -- find matches for exchange in log
    CTRL-G        -- display QRG of call
    CTRL-I        -- refresh geomagnetic indices
    CTRL-M        -- Monitor call
    CTRL-P         -- dump screen
    CTRL-Q        -- swap QSL and QUICK QSL messages
    CTRL-R        -- toggle audio recording
    CTRL-S        -- send to scratchpad
    CTRL-U        -- Unmonitor call (i.e., stop monitoring call)
    CTRL-KP+      -- increment qso number
    CTRL-KP-      -- decrement qso number
    CTRL-CURSOR DOWN -- possibly replace call with fuzzy info
    CTRL-ENTER    -- assume it's a call or partial call and go to the call if it's in the bandmap
    CTRL-KP-ENTER -- look for, and then display, entry in all the bandmaps
    CTRL-LEFT-ARROW, CTRL-RIGHT-ARROW, ALT-LEFT_ARROW, ALT-RIGHT-ARROW: up or down to next needed QSO or next needed mult. Uses filtered bandmap
    CTRL-=        -- quick QSY
    CURSOR UP     -- go to log window
    CURSOR DOWN   -- possibly replace call with SCP info
    ENTER, ALT-ENTER -- log the QSO
    ESCAPE
    F1 -- first step in SAP QSO during run
    F4 -- swap contents of CALL and BCALL windows
    F10           -- toggle filter_remaining_country_mults
    F11           -- band map filtering
    KP Del        -- remove from bandmap and add to do-not-add list (like .REMOVE)
    KP-DOWN-ARROW, KP-UP-ARROW: up or down to next stn that matches the N7DR criteria
    KP ENTER      -- send CQ #2
    KP-           -- toggle 50Hz/200Hz bandwidth if on CW
    KP-              -- toggle 1300:1600/1500:1800 centre/bandwidth if on SSB
    PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
    SHIFT (RIT control)
    SPACE -- generally, dupe check
    TAB -- switch between CQ and SAP mode
    ' -- Place NEARBY call into CALL window and update QSL window
    ; -- down to next stn that matches the N7DR criteria
    ' -- up to next stn that matches the N7DR criteria
*/
void process_CALL_input(window* wp, const keyboard_event& e)
{ using enum BANDMAP_DIRECTION;

  window& win { *wp };                  // syntactic sugar

  constexpr char COMMAND_CHAR { '.' };                                 // the character that introduces a command

  const string original_contents { remove_peripheral_spaces <std::string> (win.read()) };   // the original contents of the window, before we respond to a keypress

// keyboard_queue::process_events() has already filtered out uninteresting events
  bool processed { win.common_processing(e) };

// CTRL-A -- immediate termination, if enabled
  if (!processed and e.is_control('a') and context.allow_ctrl_a())
    throw exception();

// [ and ] (for regex)
  if (!processed and e.is_unmodified() and (e.is_char('[') or e.is_char(']')))
    processed = (win <= e.str(), true);

// BACKSPACE
  if (!processed and e.is_unmodified() and e.symbol() == XK_BackSpace)
    processed = process_backspace(win);

// . + -
  if (!processed and ( e.is_char('.') or (e.symbol() == XK_minus) or (e.is_unmodified() and ( (e.symbol() == XK_KP_Add)) ))) // XK_KP_Subtract is now CW bandwidth toggle
    processed = (win <= e.str(), true);

// need comma and asterisk for rescore options, backslash for scratchpad
  if (!processed and (e.is_char(',') or e.is_char('*') or e.is_char('\\')))
    processed = (win <= e.str(), true);

// question mark, which is displayed in response to pressing the equals sign
  if (!processed and e.is_unmodified() and e.is_char('='))
    processed = (win <= "?"s, true);

  const string call_contents { remove_peripheral_spaces <std::string> (win.read()) };
  const BAND   cur_band      { current_band };
  const MODE   cur_mode      { current_mode };

// populate the info and extract windows if we have already processed the input
  if (processed and !win_call.empty())
    display_call_info(call_contents);    // display information in the INFO window

// ALT-X -- possibly enter zoomed XIT mode
  if (!processed and e.is_alt('x') and (drlog_mode == DRLOG_MODE::SAP))
    processed = zoomed_xit();

// ALT-F4 -- toggle DEBUG state
  if (!processed and e.is_alt() and (e.symbol() == XK_F4))
  { debug = !debug;
    alert( "DEBUG STATE NOW: "s + (debug ? "TRUE"s : "FALSE"s) );
    processed = true;
  }

// KP numbers -- CW messages
  if (!processed and cw_p and (cur_mode == MODE_CW))
  { if (e.is_unmodified() and (keypad_numbers > e.symbol()) )
    { if (original_contents.empty())               // may need to temporarily reduce octothorpe for when SAP asks for repeat of serno
        octothorpe--;

      (*cw_p) << expand_cw_message(cwm[e.symbol()]);    // send the message

      if (original_contents.empty())               // restore octothorpe
        octothorpe++;

      processed = true;
    }
  }

// CTRL-C -- EXIT (same as .QUIT)
  if (!processed and (e.is_control('c')))
    exit_drlog();

// ALT-C -- send post to cluster
  if (!processed and e.is_alt('c') and (cluster_p != nullptr))
  { ost << "ALT-C pressed" << endl;

// get the frequency for the post
    auto get_frequency = [] (void)
    {
      { SAFELOCK(last_polled_frequency);        // allow us to spot while transmitting on a K3

        if (!last_polled_frequency.empty())
          return last_polled_frequency;        // use this value if we're a K3
      }

      return rig.rig_frequency().display_string();
    };

// In CQ mode, this is a self-spot
    if ( (drlog_mode == DRLOG_MODE::CQ) and self_spotting_enabled )
    { const string& callsign { my_call };
      const string& comment  { self_spotting_text };
      const string  qrg      { get_frequency() };

      ost << "about to send self spot" << endl;

      const bool spot_status { cluster_p -> spot(callsign, qrg, comment) };

      if (spot_status)
        alert("posted self spot: " + callsign + ", " + qrg + ", " + comment);
      else
        alert("error posting self spot: " + callsign + ", " + qrg + ", " + comment);
    }

// if call window is not empty and we are in SAP mode, send the contents of the call window, which is expected to be "call qrg"
    if ((drlog_mode == DRLOG_MODE::SAP) and !call_contents.empty())
    { if (contains(call_contents, ' '))         // if it contains a space, just send the contents; currently there is no way for the CALL window to contain a space
      { const bool spot_status { cluster_p -> spot(call_contents) };

        if (spot_status)
          alert("posted spot: " + call_contents);
        else
          alert("error posting spot: " + call_contents);
      }
      else                // call window contains no space; treat it as a call
      { const string& callsign { call_contents };
        const string& comment  { dx_spotting_text };
        const string qrg       { get_frequency() };

        const bool spot_status { cluster_p -> spot(callsign, qrg, comment) };

        if (spot_status)
          alert("posted spot: " + callsign + ", " + qrg + ", " + comment);
        else
          alert("error posting spot: " + callsign + ", " + qrg + ", " + comment);
      }
    }

// if call window is empty and we are in SAP mode, spot the last QSO
    if ( call_contents.empty() and (drlog_mode == DRLOG_MODE::SAP) )
    { ost << "testing whether to send spot" << endl;

      const QSO last_qso { logbk.last_qso() };

      if (!last_qso.empty())
      { const string  callsign { last_qso.callsign() };
        const string  qrg      { last_qso.freq() };
        const string& comment  { self_spotting_text };

        ost << "about to send spot" << endl;

        const bool spot_status { cluster_p -> spot(callsign, qrg, comment) };

        if (spot_status)
          alert("posted spot: " + callsign + ", " + qrg + ", " + comment);
        else
          alert("error posting spot: " + callsign + ", " + qrg + ", " + comment);
      }
    }

    processed = true;
  }

// ALT-B and ALT-V (band up and down)
  if ( !processed and (e.is_alt('b') or e.is_alt('v')) )
  { if (rules.n_bands() == 1)
    { alert("SINGLE BAND CONTEST: no band changes allowed"s, SHOW_TIME::NO_SHOW);
      processed = true;
    }
    else
    { ok_to_poll_k3 = false;          // halt polling; this is a lot cleaner, although less certain, than trying to use a mutex with its attendant nesting problems; this method should be good enough

      ignore_next_poll = true;                                      // briefly inhibit window updates from rig polling; possibly move this into process_bandmap_function()
      ignore_next_process_insertion_queue = true;

      ost << "Band change commanded: BAND " << (e.is_alt('b') ? "UP"s : "DOWN"s) << endl;

      time_log <std::chrono::milliseconds> tl;

      try
      { const frequency set_last_f { rig.rig_frequency() };

        rig.set_last_frequency(cur_band, cur_mode, set_last_f);             // save current frequency

        { if ( BAND(rig.get_last_frequency(cur_band, cur_mode)) != cur_band )
          { alert("ERROR: inconsistency in frequency/band info"s);

            ost << "  cur_band = " << cur_band << endl;
            ost << "  current_band = " << static_cast<BAND>(current_band) << endl;
            ost << "  cur_mode = " << cur_mode << endl;
            ost << "  get_last_frequency = " << rig.get_last_frequency(cur_band, cur_mode) << endl;
            ost << "  BAND(get_last_frequency) = " << BAND(rig.get_last_frequency(cur_band, cur_mode)) << endl;
            ost << "  set_last_f = " << set_last_f << endl;
          }
        }

        const BAND new_band { ( e.is_alt('b') ? set_last_f.next_band_up(permitted_bands_set) : set_last_f.next_band_down(permitted_bands_set) ) };    // move up or down one band

        current_band = new_band;

        const bandmode bmode { new_band, cur_mode };

        frequency last_frequency { rig.get_last_frequency(bmode) };   // go to saved frequency for this band/mode (if any)

        if (last_frequency.hz() == 0)                                 // go to default frequency if there is no prior frequency for this band
          last_frequency = DEFAULT_FREQUENCIES.at(bmode);

// check that we're about to go to the correct band
        { if (BAND(last_frequency) != new_band)
          { ost << "Error when attempting to change band; new band = " << new_band << ", band name = " << BAND_NAME[new_band] << ", new frequency = " << last_frequency << endl;
            alert("FREQUENCY ERROR WHEN CHANGING BAND");
          }
        }

        rig.rig_frequency(last_frequency);

// confirm that it's really happened
        { const auto f { rig.rig_frequency() };

          ost << "new frequency we have moved to appears to be: " << f << endl;
          ost << "new band is supposed to be: " << new_band << ", band name = " << BAND_NAME[new_band] << "m" << endl;
          ost << "new band is actually: " << BAND(f) << ", band name = " << BAND_NAME[BAND(f)] << "m" << endl;
          ost << "the value of current_band is: " << static_cast<BAND>(current_band) << endl;
        }

// make sure that it's in the right mode, since rigs can do weird things depending on what mode it was in the last time it was on this band
// these are the commands that take a lot of time
        rig.rig_mode(cur_mode);
        enter_sap_mode();
        rig.base_state();    // turn off RIT, split and sub-rx

// clear the call window (since we're now on a new band)
        win < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
        display_band_mode(win_band_mode, new_band, cur_mode);

// update bandmap
        FOR_ALL(bandmaps, [] (bandmap& bm) { bm.increment_version(); });  // to handle case of multiple quick band changes

        bandmap& bm { bandmaps[new_band] };

        ost << "displaying band map for band: " << BAND_NAME[new_band] << "m" << endl;

        time_log <std::chrono::milliseconds> t2;

        win_bandmap <= bm;
        t2.end_now();
        ost << "time taken to display band = " << t2.time_span<int>() << " milliseconds" << endl;

// is there a station close to our frequency?
        const string nearby_callsign { bm.nearest_displayed_callsign(last_frequency, context.guard_band(cur_mode)) };

        display_nearby_callsign(nearby_callsign);  // clears NEARBY window if call is empty

// clear the LAST QRG window
        win_last_qrg < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;

// update displays of needed mults
        update_remaining_callsign_mults_window(statistics, string(), new_band, cur_mode);
        update_remaining_country_mults_window(statistics, new_band, cur_mode);
        update_remaining_exchange_mults_windows(/* rules, */ statistics, new_band, cur_mode);

        display_bandmap_filter(bm);

        tl.end_now();
        ost << "time taken to change bands = " << tl.time_span<int>() << " milliseconds" << endl;
      }

      catch (const rig_interface_error& e)
      { ok_to_poll_k3 = true;
        alert(e.reason());
      }

      ok_to_poll_k3 = true;       // this is the only reasonable exit, so OK to do this here
      processed = true;
    }
  }

// ALT-M -- change mode
  if (!processed and e.is_alt('m') and (n_modes > 1))
  { rig.set_last_frequency(cur_band, cur_mode, rig.rig_frequency());             // save current frequency

    const MODE new_mode { rules.next_mode(cur_mode) };

    current_mode = new_mode;

    const bandmode bmode { cur_band, new_mode };

    rig.rig_frequency(rig.get_last_frequency(bmode).hz() ? rig.get_last_frequency(bmode) : DEFAULT_FREQUENCIES.at(bmode));
    rig.rig_mode(new_mode);

    display_band_mode(win_band_mode, cur_band, new_mode);

// update displays of needed mults
    update_remaining_country_mults_window(statistics, cur_band, new_mode);

    processed = true;
  }

// ALT-N -- toggle notch status if on SSB
  if (!processed and e.is_alt('n'))
  { if (current_mode == MODE_SSB)
      rig.toggle_notch_status();  

    processed = true;
  }

// PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
  if (!processed and ((e.symbol() == XK_Next) or (e.symbol() == XK_Prior)))
    processed = change_cw_speed(e);

// CTRL-S -- send to scratchpad
  if (!processed and e.is_control('s'))
    processed = send_to_scratchpad(original_contents);

// ALT-K -- toggle CW
  if (!processed and e.is_alt('k'))
    processed = toggle_cw();

// ESCAPE
  if (!processed and e.symbol() == XK_Escape)
  {
// abort sending CW if we are currently sending
    if (cw_p)
    { if (!cw_p->empty())
      { cw_p->abort();
        processed = true;
      }
    }

// clear the call window if there's something in it
    if (!processed and (!remove_peripheral_spaces <std::string_view> (win.read()).empty()))
    { win <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      win.insert(true);                         // force into INSERT mode
      processed = true;
    }

    processed = true;
  }

// TAB -- switch between CQ and SAP mode
  if (!processed and (e.symbol() == XK_Tab))
    processed = toggle_drlog_mode();

// F10 -- toggle filter_remaining_country_mults
  if (!processed and (e.symbol() == XK_F10))
  { filter_remaining_country_mults = !filter_remaining_country_mults;
    update_remaining_country_mults_window(statistics, current_band, current_mode);
    processed = true;
  }

// F11 -- band map filtering
  if (!processed and (e.symbol() == XK_F11))
  { const string contents { remove_peripheral_spaces <std::string> (win.read()) };  // can't use string_view here

    bandmap& bm = bandmaps[cur_band];        // use current bandmap to make it easier to display column offset

    if (contents.empty())     // cycle amongst OFF / HIDE / SHOW
    { if (bm.filter_enabled() and bm.filter_show())
      { bm.filter_enabled(false);

        win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_disabled_colour());
        display_bandmap_filter(bm);
        processed = true;
      }

      if (!processed and !bm.filter_enabled())
      { bm.filter_enabled(true);
        bm.filter_hide(true);

        win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_hide_colour());
        display_bandmap_filter(bm);
        processed = true;
      }

      if (!processed and bm.filter_enabled() and bm.filter_hide())
      { bm.filter_show(true);

        win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_show_colour());
        display_bandmap_filter(bm);
        processed = true;
      }
    }
    else   // treat the contents as something to add to or subtract from the filter
    { const string str { CONTINENT_SET.contains(contents) ? contents : location_db.canonical_prefix(contents) };

      bm.filter_add_or_subtract(str);
      display_bandmap_filter(bm);
      processed = true;         //  processed even if haven't been able to do anything with it
    }

    win_bandmap <= bm;
  }

// ALT-KP_4: decrement bandmap column offset; ALT-KP_6: increment bandmap column offset
  if (!processed and e.is_alt_and_not_control() and ( (e.symbol() == XK_KP_4) or (e.symbol() == XK_KP_6)
                                                  or  (e.symbol() == XK_KP_Left) or (e.symbol() == XK_KP_Right) ) )
    processed = process_change_in_bandmap_column_offset(e.symbol());

// ENTER, ALT-ENTER -- a lot of complicated stuff
  if (!processed and (e.is_unmodified() or e.is_alt()) and (e.symbol() == XK_Return))
  { const string      contents_str  { remove_peripheral_spaces <std::string> ( win.read() ) };
//    const string_view contents      { contents_str };
    const string& contents { contents_str };      // temporary re-name, until we have full sv support

// if empty, send CQ #1, if in CQ mode
    if (contents.empty())
    { if ( (current_mode == MODE_CW) and (cw_p) and (drlog_mode == DRLOG_MODE::CQ))
        (*cw_p) << context.message_cq_1();

      processed = true;
    }

// process a command if the first character is the COMMAND_CHAR
    if (!processed and (contents[0] == COMMAND_CHAR))
    { const string_view command { substring <std::string_view> (contents, 1) };

// .ABORT -- immediate exit, simulating power failure
      if (command.starts_with("ABORT"sv))
        exit(-1);

// .AC ON|OFF -- control autocorrecting RBN posts
      if (command.starts_with("AC"sv))
      { const vector<string_view> words { clean_split_string <std::string_view> (command, ' ') };

        if (words.size() == 2)
        { if (words[1] == "ON"sv)
          { autocorrect_rbn = true;
            ost << "AUTOCORRECT RBN turned ON" << endl;
          }

          if (words[1] == "OFF"sv)
          { autocorrect_rbn = false;
            ost << "AUTOCORRECT RBN turned OFF" << endl;
          }
        }
      }

// .ADD <call> -- remove call from the do-not-show list
      if ( command.starts_with("ADD"sv) or command.starts_with("SHOW"sv) )
      { if (contains(command, ' '))
          FOR_ALL(bandmaps, [callsign = remove_peripheral_spaces <std::string> (substring <std::string> (command, command.find(' ')))] (bandmap& bm) { bm.remove_from_do_not_add(callsign); } );
      }

// .CABRILLO
      if (command == "CABRILLO"sv)
      { const string cabrillo_filename { context.cabrillo_filename().empty() ? "cabrillo"s : context.cabrillo_filename() };
        const string log_str           { logbk.cabrillo_log(context, context.cabrillo_include_score() ? statistics.points(rules) : 0) };    // 0 indicates that score is not to be included

        write_file(log_str, cabrillo_filename);
        alert("Cabrillo file "s + context.cabrillo_filename() + " written"s);
      }

      win <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;

// .CLEAR
      if (command == "CLEAR"sv)
        win_message <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;

// .CULL <n>
      if (command.starts_with("CULL"sv))
      { if (const auto posn { command.find(' ') }; posn != string_view::npos)
          FOR_ALL(bandmaps, [cull_function = from_string<int>(substring <std::string> (command, posn))] (bandmap& bm) { bm.cull_function(cull_function); } );

        bandmap& bm { bandmaps[current_band] };

        win_bandmap <= bm;
        display_bandmap_filter(bm);
        win <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      }

// .INST
      if (command == "INST"sv)
      { rig.instrument();
        alert("rig exchanges now instrumented");
      }

// .M : insert memory
      if (command == "M"sv)
        insert_memory();

// .MONITOR <call> -- add <call> to those being monitored
      if (command.starts_with("MON"sv))
      { if (const auto posn { command.find(' ') }; posn != string::npos)
        { const string callsign { remove_peripheral_spaces <std::string> (substring <std::string> (command, posn)) };

          mp += callsign;

          alert("MONITORING: "s + callsign);
        }
      }

// .QTC QRS <n>
      if (command.starts_with("QTC QRS "sv))
      { const unsigned int new_qrs { from_string<unsigned int>(substring <std::string> (command, 8)) };

        context.qtc_qrs(new_qrs);
        alert((string)"QTC QRS set to: "s + to_string(new_qrs), SHOW_TIME::NO_SHOW);
      }

// .QUIT
      if (command == "QUIT"sv)
        exit_drlog();

// .R[n] : recall memory and go there
      if ( ((command.size() == 2) and (command[0] == 'R')) or (command == "R"sv) )
      { const unsigned int number { (command.size() == 2 ? from_string<unsigned int>(create_string(command[1])) : 0) };
        const memory_entry me     { recall_memory(number) };
        const frequency&   freq   { me.freq() };

        if (freq.hz())    // if valid
        { ok_to_poll_k3 = false;        // we might be partway through a poll, but that should be OK

          rig.rig_frequency(freq);
          current_band = static_cast<BAND>(freq);

          rig.rig_mode(me.mode());
          current_mode = me.mode();

          display_band_mode(win_band_mode, current_band, me.mode());
          enter_cq_or_sap_mode(me.drlog_mode());
          update_based_on_frequency_change(freq, me.mode());

          ok_to_poll_k3 = true;
        }
      }

// .REMOVE <call> -- remove call from bandmap and add it to the do-not-show list
      if ( command.starts_with("REMOVE"sv) or command.starts_with("RM"sv))
      { if (const auto posn { command.find(' ') }; posn != string_view::npos)
        { const string callsign { remove_peripheral_spaces <std::string> (substring <std::string> (command, posn)) };
          
          do_not_show(callsign);

          win_bandmap <= bandmaps[current_band];
        }
      }

// .RESCOREB or .SCOREB
      if ( command.starts_with("RESCOREB"sv) or command.starts_with("SCOREB"sv) )
      { if (const auto posn { command.find(' ') }; posn != string_view::npos)
        { const string rhs { substring <std::string> (command, posn) };

          set<BAND> score_bands;

          for (const auto& band_str : clean_split_string <string> (rhs))
          { try
            { score_bands += BAND_FROM_NAME.at(band_str);     // at() does not yet support heterogeneous lookup
            }

            catch (...)
            { if (band_str == "*"sv)
                score_bands = permitted_bands_set;
              else
                alert("Error parsing [RE]SCOREB command"s);
            }
          }

          rules.score_bands(score_bands);
        }
        else    // no band information
          rules.restore_original_score_bands();

        string bands_str { };

        FOR_ALL(rules.score_bands(), [&bands_str] (const BAND& b) { bands_str += (BAND_NAME[b] + ' '); } );

        win_score_bands < WINDOW_ATTRIBUTES::WINDOW_CLEAR < "Score Bands: "s <= bands_str;

        rescore(rules);
        update_rate_window();
        display_statistics(statistics.summary_string(rules));
        update_score_window(statistics.points(rules));
      }

// .RESCOREM or .SCOREM
      if ( command.starts_with("RESCOREM"sv) or command.starts_with("SCOREM"sv) )
      { if (const auto posn { command.find(' ') }; posn != string::npos)
        { set<MODE> score_modes;

          for (const auto& mode_str : clean_split_string <std::string> (substring <std::string> (command, posn)))
          { try
            { score_modes += MODE_FROM_NAME.at(mode_str);     // at() does not yet support heterogeneous lookup
            }

            catch (...)
            { if (mode_str == "*"sv)
                score_modes = set<MODE> { rules.permitted_modes().cbegin(), rules.permitted_modes().cend() };
              else
                alert("Error parsing [RE]SCOREM command"s);
            }
          }

          rules.score_modes(score_modes);
        }
        else    // no mode information
          rules.restore_original_score_modes();

        string modes_str { };

        FOR_ALL(rules.score_modes(), [&modes_str] (const MODE m) { modes_str += (MODE_NAME[m] + ' '); } );

        win_score_modes < WINDOW_ATTRIBUTES::WINDOW_CLEAR < "Score Modes: "s <= modes_str;

        rescore(rules);
        update_rate_window();
        display_statistics(statistics.summary_string(rules));
        update_score_window(statistics.points(rules));
      }

// .RESET RBN -- get a new connection
      if (command == "RESET RBN"sv)
        jthread(reset_connection, rbn_p).detach();

// .UNINST
      if (command == "UNINST"sv)
      { rig.uninstrument();
        alert("rig exchanges now uninstrumented");
      }

// .UNMONITOR <call> -- remove <call> from those being monitored
      if (command.starts_with("UNMON"sv))
      { if (const auto posn { command.find(' ') }; posn != string::npos)
        { const string callsign { remove_peripheral_spaces <std::string> (substring <std::string> (command, posn)) };

          mp -= callsign;

          alert("UNMONITORING: "s + callsign);
        }
      }

      processed = true;
    }

// BACKSLASH -- send to the scratchpad
    if (!processed and contains(contents, '\\'))
    { processed = send_to_scratchpad(remove_char(contents, '\\'));
      win <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
    }

// is it a frequency? Could check exhaustively with a horrible regex, but this is clearer and we would have to parse it anyway
// something like (\+|-)?([0-9]+(\.[0-9]+))
// if there's a letter, it's not a frequency; otherwise we try to parse as a frequency

    if (!processed)
    { const bool contains_letter { contains_upper_case_letter(contents) };

      if (!contains_letter)    // try to parse as frequency
      { const bool contains_plus  { (contents[0] == '+') };        // this can be entered from the keypad w/o using shift
        const bool contains_minus { (contents[0] == '-') };

        double value { from_string<double>(contents) };      // what happens when contents can't be parsed as a number?

// if there's a plus or minus we interpret the value as kHz and move up or down from the current QRG
        {
// handle frequency without the MHz part: [n][n]n.n
          if (!contains_plus and !contains_minus and (value < 1000))
          { const bool possible_qsy { ( (contents.length() >= 3) and (contents[contents.size() - 2] == '.') ) };

            if (possible_qsy)
            { float band_edge_in_khz { rig.rig_frequency().lower_band_edge().khz() };

              switch (cur_band)
              { case BAND_160 :
                  value += (value < 100 ? 1800 : 1000);
                  break;

                case BAND_80 :
                  value += (value < 100 ? 3500 : 3000);
                  break;

                case BAND_40 :
                  value += band_edge_in_khz;
                  break;

                case BAND_30 :
                  value += 10100;
                  break;

                case BAND_20 :
                  value += band_edge_in_khz;
                  break;

                case BAND_17 :
                  value += 18000;
                  break;

                case BAND_15 :
                  value += band_edge_in_khz;
                  break;

                case BAND_12 :
                  value += 24000;
                  break;

                case BAND_10 :
                  value += band_edge_in_khz;
                  break;

                default :
                  break;
              }
            }
          }

          const frequency cur_rig_frequency { rig.rig_frequency() };
          const frequency new_frequency     { ( (contains_plus or contains_minus) ? cur_rig_frequency.hz() + (value * 1000) : value ) };
          const BAND      new_band          { to_BAND(new_frequency) };

          bool valid { permitted_bands_set.contains(new_band) };

          if ( (valid) and (new_band == BAND_160))                                                  // check that it's not just BAND_160 because there's been a problem
            valid = ( (new_frequency >= 1'800_kHz) and (new_frequency <= 2'000_kHz) );

          if (valid)
          { ok_to_poll_k3 = false;
            
            const BAND cur_band { to_BAND(cur_rig_frequency) };                     // hide old cur_band

            rig.set_last_frequency(cur_band, cur_mode, cur_rig_frequency);             // save current frequency
            rig.rig_frequency(new_frequency);

            if (new_band != cur_band)
              rig.base_state();

            const MODE m { default_mode(new_frequency) };

            rig.rig_mode(m);
            current_mode = m;

            display_band_mode(win_band_mode, new_band, m);

            if (new_band != cur_band)
            { current_band = new_band;

              bandmap& bm { bandmaps[new_band] };
              win_bandmap <= bm;

// update displays of needed mults
              update_remaining_callsign_mults_window(statistics, string(), cur_band, m);
              update_remaining_country_mults_window(statistics, cur_band, m);
              update_remaining_exchange_mults_windows(/* rules, */ statistics, cur_band, m);
            }

            enter_sap_mode();    // we want to be in SAP mode after a frequency change

            win <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;

            ok_to_poll_k3 = true;
          }
          else // not valid frequency
            alert("Invalid frequency: "s + to_string(new_frequency.hz()) + " Hz"s);

          processed = true;
        }
      }
    }

// don't treat as a call if it contains weird characters
    if (!processed)
      processed = (contents.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/?"sv) != string::npos);

// assume it's a call
    if (!processed)
    { const string& callsign { contents };
      //const string_view callsign { contents };      // could use a reference hr, since this just duplicates a variable

      const bool    is_dupe  { logbk.is_dupe(callsign, cur_band, cur_mode, rules) };

// if we're in SAP mode, don't call him if he's a dupe
      if ((drlog_mode == DRLOG_MODE::SAP) and is_dupe)
      { const cursor posn { win.cursor_position() };

        win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < (contents + " DUPE"s) <= posn;

        extract = logbk.worked(callsign);
        extract.display();

        bandmap_entry be;

        be.is_needed(false);
        be.callsign(contents);
        be.freq(rig_is_split ? rig.rig_frequency_b() : rig.rig_frequency());  // the TX frequency
        be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);
        be.calculate_mult_status(rules, statistics);

        bandmap& bandmap_this_band { bandmaps[cur_band] };

        bandmap_this_band += be;
        win_bandmap <= bandmap_this_band;

        { SAFELOCK(dupe_check);
          last_call_inserted_with_space = contents;
        }

        processed = true;
      }
      else    // CQ_MODE or not a dupe
      {
// send the call
        if ( (cur_mode == MODE_CW) and (cw_p) )
        { if (drlog_mode == DRLOG_MODE::CQ)
          { (*cw_p) << callsign;

            SAFELOCK(last_exchange);  // we need to keep track of the last message, so it can be re-sent using the special character "*"

            last_exchange = expand_cw_message( e.is_unmodified() ? context.exchange_cq() : context.alternative_exchange_cq() );
            (*cw_p) << last_exchange;
            last_exchange = callsign + last_exchange;  // add the call so that we can re-send the entire sequence easily
          }
          else
            (*cw_p) << cwm[XK_KP_0];    // send contents of KP0, which is assumed to be my call
        }

// what exchange do we expect?
        const string                 canonical_prefix  { location_db.canonical_prefix(contents) };
        const vector<exchange_field> expected_exchange { rules.unexpanded_exch(canonical_prefix, cur_mode) };

        string             exchange_str;
        STRING_MAP<string> mult_exchange_field_value;                                                 // the values of exchange fields that are mults

//        ost << "length of exchange vector = " << expected_exchange.size() << endl;

        for (const auto& exf : expected_exchange)
        { bool processed_field { false };

// if it's a choice, try to figure out which one to display; in IARU, it's the zone unless the society isn't empty;
// need to figure out a way to generalise all this
          if (exf.is_choice())
          { if (exf.name() == "ITUZONE+SOCIETY"sv)
            { string iaru_guess { exchange_db.guess_value(contents, "SOCIETY"s) };      // start with guessing it's a society

              if (iaru_guess.empty())
                iaru_guess = to_upper(exchange_db.guess_value(contents, "ITUZONE"s));   // try ITU zone if no society

              exchange_str += iaru_guess;
              processed_field = true;
            }

            if (!processed_field and (exf.name() == "10MSTATE+SERNO"sv))
            { static const STRING_SET state_multiplier_countries { "K"s, "VE"s, "XE"s };

              const string canonical_prefix { location_db.canonical_prefix(contents) };
              const string state_guess      { state_multiplier_countries.contains(canonical_prefix) ? exchange_db.guess_value(contents, "10MSTATE"s) : string() };

              exchange_str += state_guess;
              processed_field = true;
            }
          }

          if (!processed_field and (exf.name() == "DOK"sv))
          { const string guess { exchange_db.guess_value(contents, "DOK"s) };

            if (!guess.empty())
            { exchange_str += (guess + SPACE_STR);
              processed_field = true;
            }
          }

          if (!processed_field and !no_default_rst and (exf.name() == "RST"sv) and !exf.is_optional())
          { exchange_str += ( (cur_mode == MODE_CW) ? "599 "s : "59 "s );
            processed_field = true;
          }

          if (!processed_field and exf.name() == "RS"sv)
          { exchange_str += "59 "s;
            processed_field = true;
          }

          if (!processed_field and (exf.name() == "GRID"sv))
          { const string guess { exchange_db.guess_value(contents, "GRID"s) };

            if (!guess.empty())
            { exchange_str += (guess + SPACE_STR);
              processed_field = true;
            }
          }

          if (!processed_field)
          { if (!variable_exchange_fields.contains(exf.name()))    // if not a variable field
            { if (const string guess { rules.canonical_value(exf.name(), exchange_db.guess_value(contents, exf.name())) }; !guess.empty())
              { if ((exf.name() == "RDA"sv) and (guess.length() == 2))                   // RDA guess might just have first two characters
                  exchange_str += guess;
                else
                { exchange_str += (guess + ' ');

                  if (exf.is_mult())                 // save the expected value of this field
                    mult_exchange_field_value += { exf.name(), guess };
                }
              }
            }
          }

          processed = true;
        }

        update_known_callsign_mults(callsign);
        update_known_country_mults(callsign, KNOWN_MULT::FORCE_KNOWN);

        win_exchange <= exchange_str;

        if (home_exchange_window and !exchange_str.empty())
          win_exchange < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < SPACE_STR <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;

        win_exchange.insert(true);          // force EXCHANGE window into INSERT mode
        set_active_window(ACTIVE_WINDOW::EXCHANGE);
      }

// add to bandmap if we're in SAP mode
      if (drlog_mode == DRLOG_MODE::SAP)
      { bandmap_entry be;

        be.callsign(callsign);
        be.is_needed(!is_dupe);
        be.freq(rig_is_split ? rig.rig_frequency_b() : rig.rig_frequency());        // also sets band; TX frequency
        be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);
        be.calculate_mult_status(rules, statistics);

        bandmap& bandmap_this_band { bandmaps[be.band()] };

        const bandmap_entry old_be { bandmap_this_band[callsign] };

        if ( (old_be.callsign().empty()) or ( old_be.frequency_str() != be.frequency_str()) )  // update bandmap only if there's a change
        { bandmap_this_band += be;

          win_bandmap <= bandmap_this_band;
        }
      }
    }
  }                 // end of ENTER

// CTRL-ENTER -- assume it's a call or partial call and go to the call if it's in the bandmap
  if (!processed and e.is_control() and (e.symbol() == XK_Return))
  { update_quick_qsy();
  
    bool found_call { false };

    frequency new_frequency;

// define what needs to be done for a QSY
    auto ctrl_enter_actions = [&new_frequency] (const bandmap_entry& be)
      { new_frequency = be.freq();

        rig.rig_frequency(be.freq());
        enter_sap_mode();

// we may require a mode change
        possible_mode_change(be.freq());
      };

// assume it's a call -- look for the same call in the current bandmap, as displayed
    bandmap_entry be;               // default entry

    const BM_ENTRIES entries { bandmaps[current_band].displayed_entries() };

    if (const auto cit { FIND_IF(entries, [&original_contents] (const bandmap_entry& be) { return (be.callsign() == original_contents); }) }; cit != entries.cend())
    { found_call = true;
      be = *cit;

      ctrl_enter_actions(be);
    }
    else    // didn't find an exact match; try a substring search
    { if (const auto cit { FIND_IF(entries, [&original_contents] (const bandmap_entry& be) { return contains(be.callsign(), original_contents); }) }; cit != entries.cend())     // if we found a match
      { found_call = true;
        be = *cit;
        win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= be.callsign();  // put callsign into CALL window

        ctrl_enter_actions(be);
      }
    }

    if (found_call)
    { const string& callsign { be.callsign() };

      win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= callsign;  // put callsign into CALL window

      update_based_on_frequency_change(new_frequency, current_mode);
      display_call_info(callsign);

      SAFELOCK(dupe_check);
      last_call_inserted_with_space = callsign;
    }

    processed = true;
  }

// KP ENTER -- send CQ #2
  if (!processed and !e.is_control() and (e.symbol() == XK_KP_Enter))
  { if (original_contents.empty() and (current_mode == MODE_CW) and (cw_p) and (drlog_mode == DRLOG_MODE::CQ) )   // perhaps send CQ #2
      (*cw_p) << context.message_cq_2();

    processed = true;
  }

// CTRL-KP-ENTER -- look for, and then display, entry in all the bandmaps
  if (!processed and e.is_control() and (e.symbol() == XK_KP_Enter))
  { string results;

    for (const auto& b : permitted_bands_set)
    { bandmap& bm { bandmaps[b] };        // use current bandmap to make it easier to display column offset

      if (const bandmap_entry be { bm[original_contents] }; !be.empty())
      { if (!results.empty())
          results += ' ';
        results += be.frequency_str();
      }
    }

    results = original_contents + ( results.empty() ? ": No posts found"s : ( ": "s + results ) );
    alert(results, SHOW_TIME::NO_SHOW);

    processed = true;
  }

// SPACE -- generally, dupe check
  if (!processed and e.is_char(' '))
  {
// if we're inside a command, just insert a space in the window; also if we are writing a comment
    if ( ((original_contents.size() > 1) and (original_contents[0] == '.')) or contains(original_contents, "\\"s))
      win <= ' ';
    else        // not inside a command
    {
// possibly put a bandmap call into the call window
      if (original_contents.empty() and drlog_mode == DRLOG_MODE::SAP)
      { const string nearby_contents { remove_peripheral_spaces <std::string> (win_nearby.read()) };

        if (!nearby_contents.empty())
        { win < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= nearby_contents;
          display_call_info(nearby_contents);
        }
      }

      const string current_contents { remove_peripheral_spaces <std::string> (win.read()) };    // contents of CALL window may have changed, so we may need to re-insert/refresh call in bandmap

// dupe check; put call into bandmap
      if (!current_contents.empty() and (drlog_mode == DRLOG_MODE::SAP) and !contains(current_contents, " DUPE"sv))
      {
// possibly add the call to known mults
        update_known_callsign_mults(current_contents);
        update_known_country_mults(current_contents, KNOWN_MULT::FORCE_KNOWN);

        bandmap_entry be;                        // default source is BANDMAP_ENTRY_LOCAL

        be.callsign(current_contents);
        be.freq(rig.rig_frequency());
        be.mode(cur_mode);                          // also sets band
        be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);

// do we still need this guy?
        const bool is_needed { is_needed_qso(current_contents, cur_band, current_mode) };

        if (!is_needed)
        { const cursor posn { win.cursor_position() };

          win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < (current_contents + " DUPE"s) <= posn;
        }

        be.calculate_mult_status(rules, statistics);
        be.is_needed(is_needed);

        bandmap& bandmap_this_band { bandmaps[cur_band] };

        bandmap_this_band += be;
        win_bandmap <= bandmap_this_band;

        { SAFELOCK(dupe_check);
          last_call_inserted_with_space = current_contents;
        }

        update_remaining_callsign_mults_window(statistics, string(), cur_band, cur_mode);
        update_remaining_country_mults_window(statistics, cur_band, cur_mode);
        update_remaining_exchange_mults_windows(/* rules, */ statistics, cur_band, cur_mode);
      }
    }

    processed = true;
  }

// CTRL-LEFT-ARROW, CTRL-RIGHT-ARROW, ALT-LEFT_ARROW, ALT-RIGHT-ARROW: up or down to next needed QSO or next needed mult. Uses filtered bandmap
  if (!processed and (e.is_control_and_not_alt() or e.is_alt_and_not_control()) and ( (e.symbol() == XK_Left) or (e.symbol() == XK_Right)))
  { if (drlog_mode == DRLOG_MODE::SAP)                              // do nothing in CQ mode
    { update_quick_qsy();
      processed = process_bandmap_function(e.is_control() ? &bandmap::needed_qso : &bandmap::needed_mult, (e.symbol() == XK_Left) ? DOWN : UP);
    }
    else
      processed = true;
  }

// CTRL-ALT-LEFT-ARROW, CTRL-ALT-RIGHT-ARROW
  if (!processed and (e.is_control() and e.is_alt()) and ( (e.symbol() == XK_Left) or (e.symbol() == XK_Right)))
  { if (drlog_mode == DRLOG_MODE::SAP)                              // do nothing in CQ mode
    { update_quick_qsy();
      processed = process_bandmap_function(&bandmap::needed_all_time_new_and_needed_qso, (e.symbol() == XK_Left) ? DOWN : UP);
    }
    else
      processed = true;
  }

// ALT-CTRL-KEYPAD-LEFT-ARROW, ALT-CTRL-KEYPAD-RIGHT-ARROW: up or down to next stn with zero QSOs, or who has previously QSLed on this band and mode. Uses filtered bandmap
  if (!processed and e.is_alt_and_control() and ( (e.symbol() == XK_KP_4) or (e.symbol() == XK_KP_6)
                                                                          or  (e.symbol() == XK_KP_Left) or (e.symbol() == XK_KP_Right) ) )
  { if (drlog_mode == DRLOG_MODE::SAP)                              // do nothing in CQ mode
    { update_quick_qsy();
      processed = process_bandmap_function(&bandmap::needed_all_time_new_or_qsled, (e.symbol() == XK_KP_Left or e.symbol() == XK_KP_4) ? DOWN : UP);
    }
    else
      processed = true;
  }

// ALT-CTRL-KEYPAD-DOWN-ARROW, ALT-CTRL-KEYPAD-UP-ARROW: up or down to next stn that matches the N7DR criteria
  if (!processed and e.is_alt_and_control() and ( (e.symbol() == XK_KP_2) or (e.symbol() == XK_KP_8)
                                                                          or  (e.symbol() == XK_KP_Down) or (e.symbol() == XK_KP_Up) ) )
  { if (drlog_mode == DRLOG_MODE::SAP)                              // do nothing in CQ mode
    { ignore_next_poll = true;                                      // briefly inhibit window updates from rig polling; possibly move this into process_bandmap_function()
      ignore_next_process_insertion_queue = true;
      update_quick_qsy();
      processed = process_bandmap_function((e.symbol() == XK_KP_Down or e.symbol() == XK_KP_2) ? DOWN : UP);
    }
    else
      processed = true;
  }

// and unmodified ; and ': also up or down to next stn that matches the N7DR criteria
  if (!processed and e.is_unmodified() and ( e.is_char(';') or e.is_char('\'') ) )  
  { if (drlog_mode == DRLOG_MODE::SAP)                              // do nothing in CQ mode
    { ost << "UP or DOWN using N7DR criteria" << endl;
      ignore_next_poll = true;                                      // briefly inhibit window updates from rig polling; possibly move this into process_bandmap_function()
      ignore_next_process_insertion_queue = true;
      update_quick_qsy();
      processed = process_bandmap_function(e.is_char(';') ? DOWN : UP);
    }
    else
      processed = true;
  }

// CTRL-; and CTRL-' -- up or down to stn that matches the N7DR criteria, in increments of 5 stns
  if (!processed and (e.is_control(';') or e.is_control('\'')))
  { if (drlog_mode == DRLOG_MODE::SAP)                              // do nothing in CQ mode
    { ignore_next_poll = true;                                      // briefly inhibit window updates from rig polling; possibly move this into process_bandmap_function()
      ignore_next_process_insertion_queue = true;
      update_quick_qsy();
      processed = process_bandmap_function(e.is_control(';') ? DOWN : UP, 4);  // move by 5 stations
    }
    else
      processed = true;
  }

// ALT-; and ALT-' -- up or down to stn that matches the N7DR criteria, in increments of a large number of stns
  if (!processed and (e.is_alt(';') or e.is_alt('\'')))
  { if (drlog_mode == DRLOG_MODE::SAP)                              // do nothing in CQ mode
    { ignore_next_poll = true;                                      // briefly inhibit window updates from rig polling; possibly move this into process_bandmap_function()
      ignore_next_process_insertion_queue = true;
      update_quick_qsy();

      const int16_t nskip { static_cast<int16_t>( (bandmaps[cur_band].cull_function() == 1) ? (win_bandmap.height() - 1) : 24 ) };

      processed = process_bandmap_function(e.is_alt(';') ? BANDMAP_DIRECTION::DOWN : BANDMAP_DIRECTION::UP, nskip);  // move by (nskip + 1) stations
    }
    else
      processed = true;
  }

// SHIFT (RIT control)
// RIT changes via hamlib, at least on the K3, are testudine
  if (!processed and (e.event() == KEY_EVENT::PRESS) and ( (e.symbol() == XK_Shift_L) or (e.symbol() == XK_Shift_R) ) )
    processed = shift_control(e);

// ALT-Y -- delete last QSO
  if (!processed and e.is_alt('y'))
  { if (original_contents.empty())               // process only if empty
    { if (!logbk.empty())
      { const QSO qso { logbk.remove_last_qso() };

        if (send_qtcs)
        { qtc_buf -= qtc_entry(qso);
          update_qtc_queue_window();
        }

// remove the visual indication in the visible log
        bool cleared { false };

        for (auto line_nr { 0 }; line_nr < win_log.height() and !cleared; ++line_nr)
        { if (!win_log.line_empty(line_nr))
            cleared = (win_log.clear_line(line_nr), true);
        }

        rebuild_history(logbk, rules, statistics, q_history, rate);
        rescore(rules);
        update_rate_window();
        rebuild_dynamic_call_databases(logbk);
        display_statistics(statistics.summary_string(rules));
        update_score_window(statistics.points(rules));

        if (octothorpe)
          octothorpe--;

        win_serial_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(serial_number_string(octothorpe), win_serial_number.width());

        if (next_qso_number)
          next_qso_number--;

        win_qso_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(next_qso_number, win_qso_number.width());

        update_remaining_callsign_mults_window(statistics, string(), cur_band, cur_mode);
        update_remaining_country_mults_window(statistics, cur_band, cur_mode);
        update_remaining_exchange_mults_windows(/* rules, */ statistics, cur_band, cur_mode);

// removal of a Q might change the colour indication of stations
        for (auto& bm : bandmaps)
        { FOR_ALL(bm.entries(), [&bm] (auto& be) { if (be.remark(rules, q_history, statistics))
                                                     bm += be; 
                                                 });

          if (&bm == &(bandmaps[current_band]))   // re-display current bandmap
            win_bandmap <= bm;
        }

// remove the last line from the log on disk
        const vector<string> disk_log_lines { to_lines <std::string> (read_file(context.logfile())) };

        if (!disk_log_lines.empty())
        { FILE* fp { fopen(context.logfile().c_str(), "w") };

          for (size_t n { 0 }; n < disk_log_lines.size() - 1; ++n)  // don't include last QSO
          { const string line_to_write { disk_log_lines[n] + EOL };

            fwrite(line_to_write.c_str(), line_to_write.length(), 1, fp);
          }

          fclose(fp);
        }
      }
    }

    processed = true;
  }

  const bool cursor_down { (e.is_unmodified() and e.symbol() == XK_Down) }; ///< is the event a CURSOR DOWN?
  const bool cursor_up   { (e.is_unmodified() and e.symbol() == XK_Up) };   ///< is the event a CURSOR DOWN?

  static bool in_scp_matching { false };  ///< are we walking through the calls?
  static int  scp_index       { -1 };     ///< index into matched calls

  if (!cursor_down and !cursor_up)                 // clear memory of walking through matched calls every time we press a different key
  { in_scp_matching = false;
    scp_index = -1;
  }

// CURSOR UP -- go to log window
  if (!processed and cursor_up and !in_scp_matching)
  { ost << "ENTERING EDITABLE LOG WINDOW" << endl;
  
    set_active_window(ACTIVE_WINDOW::LOG);

    win_log_snapshot = win_log.snapshot();
    win_log.toggle_hidden();

    processed = ( win_log <= cursor(0, 0), true );
  }

// CURSOR DOWN -- possibly replace call with SCP info
// some trickery needed to provide capability to walk through SCP calls after trying to find an obvious match
// includes fuzzy matches after SCP matches
  if (!processed and (cursor_down or cursor_up))
  { bool   found_match  { false };
    string new_callsign { };

    if ( (!in_scp_matching) and cursor_down)            // first down arrow; select best match, according to match_callsign() algorithm
    { const string current_contents { remove_peripheral_spaces <std::string> (win.read()) }; 

      new_callsign = match_callsign(scp_matches, current_contents);       // match_callsign returns the empty string if there is NO OBVIOUS BEST MATCH

      if (new_callsign.empty())
      { 
// if there was no obvious best match, but there were some matches on offer, choose the first
        new_callsign = (new_callsign.empty() and !scp_matches.empty()) ? scp_matches[0].first : match_callsign(fuzzy_matches);

        if (new_callsign.empty())
        { new_callsign = match_callsign(query_1_matches);

          if (new_callsign.empty())
            new_callsign = match_callsign(query_n_matches);
        }
      }
      
      if (!new_callsign.empty())
      { win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= new_callsign;
        display_call_info(new_callsign);
        found_match = true;
      }
      
      in_scp_matching = true;
    }

    if (in_scp_matching and !found_match)        // no "best" callsign, or we didn't like it and want to walk through the matches
    { static vector<string>  all_matches;

      if (scp_index == -1)                  // if we haven't created the list of matches
      { all_matches.clear();

        for (const auto& these_matches : matches_array)
        { FOR_ALL(these_matches, [] (const pair<string, PAIR_NUMBER_TYPE>& psi) { all_matches += psi.first; } );
//          FOR_ALL(these_matches, [] (const auto& [call, clrs]) { all_matches += call; } );                        // for non-obvious reasons, this isn't allowed in C++ (yet)
        }

// remove any duplicates from all_matches while maintaining ordering
        const vector<string> all_matches_copy { all_matches };

        UNORDERED_STRING_SET already_present;

        all_matches.clear();

        for (const auto& match : all_matches_copy)
        { if (!already_present.contains(match))
          { all_matches += match;
            already_present += match;
          }
        }
      }

      if (!all_matches.empty())                         // if there are some matches
      { if (scp_index == -1)
        { scp_index = 0;                                // go to first call
          
          if (all_matches[scp_index] == remove_peripheral_spaces <std::string> (win.read()))               // there was a best match and it's the same as the first SCP/fuzzy match
            scp_index = min(scp_index + 1, static_cast<int>(all_matches.size() - 1));
        }
        else
        { if (cursor_down)
            scp_index = min(scp_index + 1, static_cast<int>(all_matches.size() - 1));

          if (cursor_up)                                  // one of cursor_up and cursor_down should be true
            scp_index = max(scp_index - 1, 0);
        }
        
        new_callsign = all_matches[scp_index];
        win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= new_callsign;
        display_call_info(new_callsign);
      }
// do nothing if the matches are empty      
    }

    processed = true;
  }

// CTRL-CURSOR DOWN -- possibly replace call with fuzzy info
  if (!processed and e.is_ctrl() and e.symbol() == XK_Down)
  { if (const string new_callsign { match_callsign(fuzzy_matches) }; !new_callsign.empty())
    { win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= new_callsign;
      display_call_info(new_callsign);
    }

    processed = true;
  }

// ALT-KP+ -- increment octothorpe
  if (!processed and e.is_alt_and_not_ctrl() and e.symbol() == XK_KP_Add)
    processed = ( win_serial_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(serial_number_string(++octothorpe), win_serial_number.width()), true );

// ALT-KP- -- decrement octothorpe
  if (!processed and e.is_alt_and_not_ctrl() and e.symbol() == XK_KP_Subtract)
    processed = ( win_serial_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(serial_number_string(--octothorpe), win_serial_number.width()), true );

// CTRL-KP+ -- increment qso number
  if (!processed and e.is_ctrl_and_not_alt() and e.symbol() == XK_KP_Add)
//    processed = ( win_qso_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(to_string(++next_qso_number), win_qso_number.width()), true );
    processed = ( win_qso_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(++next_qso_number, win_qso_number.width()), true );

// CTRL-KP- -- decrement qso number
  if (!processed and e.is_ctrl_and_not_alt() and e.symbol() == XK_KP_Subtract)
    processed = ( win_qso_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(--next_qso_number, win_qso_number.width()), true );

// KP Del -- remove from bandmap and add to do-not-add list and file (like .REMOVE)
  if (!processed and e.symbol() == XK_KP_Delete and e.is_unmodified())
  { do_not_show(original_contents);

    processed = ( win_bandmap <= (bandmaps[current_band]), true );
  }

// Alt KP Del -- remove from bandmap and add to do-not-add list and file, for this band only
  if (!processed and e.symbol() == XK_KP_Delete and e.is_alt())
  { do_not_show(original_contents, current_band);

    processed = ( win_bandmap <= (bandmaps[current_band]), true );
  }

// ` -- SWAP RIT and XIT
  if (!processed and e.is_char('`'))
    processed = swap_rit_xit();

// ALT-P -- Dump P3
  if (!processed and e.is_alt('p') and context.p3())
    processed = p3_screenshot();

// CTRL-P -- dump screen
  if (!processed and e.is_control('p'))
    processed = (dump_screen(), true);

//  ALT-D         -- screenshot and dump all bandmaps to output file [for debugging purposes]
  if (!processed and e.is_alt('d'))
    processed = debug_dump();

// ALT-Q -- send QTC
  if (!processed and e.is_alt('q') and send_qtcs)
  { last_active_window = active_window;
    set_active_window(ACTIVE_WINDOW::LOG_EXTRACT);
    sending_qtc_series = false;       // initialise variable
    win_active_p-> process_input(e);  // reprocess the alt-q

    processed = true;
  }

// CTRL-S -- toggle split
  if (!processed and e.is_control('s'))
  { try
    { rig.split_enabled() ? rig.split_disable() : rig.split_enable();
    }

    catch (const rig_interface_error& e)
    { alert( "Error toggling split: "s + e.reason());
    }

    processed = true;
  }

// ALT-S -- toggle sub receiver
  if (!processed and e.is_alt('s'))
  { try
    { rig.sub_receiver_toggle();
    }

    catch (const rig_interface_error& e)
    { alert("Error toggling SUBRX: "s + e.reason());
    }

    processed = true;
  }

// ALT-ENTER; VFO B
  if (!processed and e.is_alt() and (e.symbol() == XK_Return))
  {
// try to parse as frequency
    const bool      contains_letter { contains_upper_case_letter(original_contents) };
    const frequency f_b             { rig.rig_frequency_b() };

    if (!contains_letter)    // try to parse as frequency
    { const bool contains_plus  { (original_contents[0] == '+') };        // this can be entered from the keypad w/o using shift
      const bool contains_minus { (original_contents[0] == '-') };

      double value { from_string<double>(original_contents) };      // what happens when contents can't be parsed as a number?

// if there's a plus or minus we interpret the value as kHz and move up or down from the current QRG
      {
// handle frequency without the MHz part: [n][n]n.n
        if (!contains_plus and !contains_minus and (value < 1000))
        { bool possible_qsy { (original_contents.length() >= 3) };

          possible_qsy = possible_qsy and (original_contents[original_contents.size() - 2] == '.');

          if (possible_qsy)
          {
// get band of VFO B
            const BAND  band_b           { to_BAND(f_b) };
            const float band_edge_in_khz { f_b.lower_band_edge().khz() };

            switch (band_b)
            { case BAND_160 :
                value += (value < 100 ? 1800 : 1000);
                break;

              case BAND_80 :
                value += (value < 100 ? 3500 : 3000);
                break;

              case BAND_40 :
              case BAND_20 :
              case BAND_15 :
              case BAND_10 :
                value += band_edge_in_khz;
                break;

              default :
                break;
            }
          }
        }

        const frequency new_frequency_b { ( (contains_plus or contains_minus) ? f_b.hz() + (value * 1000) : value ) };

        rig.rig_frequency_b(new_frequency_b); // don't call set_last_frequency for VFO B
      }

      processed = ( win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE, true );
    }

// don't treat as a call if it contains weird characters
    if (!processed)
      processed = (original_contents.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/?"sv) != string::npos);

// assume it's a call or partial call and go to the call if it's in the bandmap
    if (!processed)
    { const BAND band_b { to_BAND(f_b) };

// assume it's a call -- look for the same call in the VFO B bandmap
      bandmap_entry be { bandmaps[band_b][original_contents] };

      if (!(be.callsign().empty()))
        rig.rig_frequency_b(be.freq());
      else                                          // didn't find an exact match; try a substring search
      { be = bandmaps[band_b].substr(original_contents);

        if (!(be.callsign().empty()))
        { rig.rig_frequency_b(be.freq());

          win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
        }
      }
    }

    processed = true;
  }

// ALT--> -- VFO A -> VFO B
  if (!processed and (e.is_alt('>') or e.is_alt('.')))
    processed = ( rig.rig_frequency_b(rig.rig_frequency()), true );

// ALT-<- -- VFO B -> VFO A
  if (!processed and (e.is_alt('<') or e.is_alt(',')))
    processed = ( rig.rig_frequency(rig.rig_frequency_b()), true );

// CTRL-Q -- swap QSL and QUICK QSL messages
  if (!processed and (e.is_control('q')))
  { swap(alternative_qsl_message, qsl_message);
    alert("QSL messages swapped"s, SHOW_TIME::NO_SHOW);
    processed = true;
  }

// CTRL-F -- find matches for exchange in log
  if (!processed and (e.is_control('f')))
  { if (!original_contents.empty())
      extract.match_exchange(logbk, original_contents);

    processed = true;
  }

// CTRL-B -- fast CW bandwidth
  if (!processed and (e.is_control('b')))
    processed = fast_cw_bandwidth();

// F1 -- first step in SAP QSO during run
//    turn on sub-receiver, put sub-receiver on frequency of call (or substring of call)
//    moves call to BCALL window
// intent here is to see whether we can hear the station whose call is in the CALL window
#if 0
  if (!processed and (e.symbol() == XK_F1))
  { if (drlog_mode == DRLOG_MODE::CQ)      // don't do anything in SAP mode
    { if (!original_contents.empty())
      {
// assume it's a call -- look for the same call in the current bandmap
        bandmap_entry be { bandmaps[current_band][original_contents] };

        if (be.callsign().empty())          // didn't find an exact match; try a substring search
          be = bandmaps[current_band].substr(original_contents);

//        ost << "bandmap_entry: " << be.to_brief_string() << endl;

        const BAND old_b_band { to_BAND(rig.rig_frequency_b()) };

        rig.rig_frequency_b(be.freq());
        win_bcall < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= be.callsign();     // put call in BCALL

        if (old_b_band != to_BAND(be.freq())) // stupid K3 swallows the following sub-receiver command if it's changed bands; may be able to remove this now
          sleep_for(100ms);

        rig.sub_receiver_enable();

// move contents of CALL to BCALL, clear CALL
        if (win_bcall.defined() and !win_call.empty())
        { win_call <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
        }
      }
      else      // nothing in CALL: emtpy bcall and turn off sub-receiver
      { win_bcall <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
        rig.sub_receiver_disable();
      }
    }

    processed = true;
  }
#endif

  if (!processed and (e.symbol() == XK_F1))
    processed = process_keypress_F1(original_contents);

// F2 toggle: split and force SAP mode
// intent here is to be ready to call the station: the TX is on the other station's QRG (which is VFO B)
  if (!processed and (e.symbol() == XK_F2))
    processed = process_keypress_F2();

// F4 -- swap contents of CALL and BCALL windows, EXCHANGE and BEXCHANGE windows
  if (!processed and (e.symbol() == XK_F4))
    processed = process_keypress_F4();

// F5 -- combine F2 and F4
  if (!processed and (e.symbol() == XK_F5))
    processed = process_keypress_F5();

// CTRL-M -- monitor call
  if (!processed and (e.is_control('m')))
  { if (!original_contents.empty())
    { mp += original_contents;

      alert("MONITORING: "s + original_contents);
    }

    processed = true;
  }

// CTRL-U -- unmonitor call
  if (!processed and (e.is_control('u')))
  { if (!original_contents.empty())
    { mp -= original_contents;

      alert("UNMONITORING: "s + original_contents);
    }

    processed = true;
  }

// ' -- Place NEARBY call into CALL window and update QSL window
  if (!processed and e.is_unmodified() and e.symbol() == XK_apostrophe)
  { if (win_call.empty() and !win_nearby.empty())
    { const string new_call { remove_peripheral_spaces <std::string> (win_nearby.read()) };

      win_call < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= new_call;
      update_qsls_window(new_call);
    }

    processed = true;
  }

// CTRL-R -- toggle audio recording
  if (!processed and (e.is_control('r')) and allow_audio_recording)
    processed = toggle_recording_status(audio);

// ALT-R -- toggle RX antenna
  if (!processed and (e.is_alt('r')))
  { rig.toggle_rx_ant();
    processed = update_rx_ant_window();
  }
  
// CTRL-= -- quick QSY
  if (!processed and (e.is_control('=')))
  { auto                  [old_frequency, old_mode] { quick_qsy_map.at(current_band) };
//    pair<frequency, MODE> new_quick_qsy_info        { get_frequency_and_mode() };

    const auto [ f, m ] { get_frequency_and_mode() };

    rig.rig_frequency(old_frequency);
    rig.rig_mode(old_mode);

//    const pair<frequency, MODE>& quick_qsy_info { new_quick_qsy_info };        // where we were, so we can quickly return

//    quick_qsy_map[current_band] = new_quick_qsy_info;
//    quick_qsy_map[current_band] = tie(f, m);
    quick_qsy_map[current_band] = pair { f, m };

//    win_quick_qsy < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE
//                  <= pad_left(quick_qsy_info.first.display_string(), 7) + SPACE_STR + MODE_NAME[quick_qsy_info.second];
    win_quick_qsy < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE
                  <= pad_left(f.display_string(), 7) + SPACE_STR + MODE_NAME[m];
    
//    update_based_on_frequency_change(old_frequency, old_mode);
    bandmap& bm { bandmaps[current_band] };

    win_bandmap <= bm;

    processed = true;
  }

// CTRL-I -- refresh geomagnetic indices
  if (!processed and (e.is_control('i')))
  { if (!geomagnetic_indices_command.empty())
      jthread(get_indices, geomagnetic_indices_command).detach();

    processed = true;
  }

// KP- -- toggle 50Hz/200Hz bandwidth if on CW; centre RIT if on SSB and RIT is on
  if (!processed and e.is_unmodified() and e.symbol() == XK_KP_Subtract)
  { switch (current_mode)
    { case MODE_CW :
        processed = cw_toggle_bandwidth();
        break;

      case MODE_SSB :
        processed = ssb_toggle_bandwidth();
        break;

      default :
        processed = true;
    }
  }

// KP5 -- centre RIT if on SSB and RIT is on
  if (!processed and e.is_unmodified() and ( (e.symbol() == XK_KP_5) or (e.symbol() == XK_KP_Begin) ) )     // XK_KP_Begin seems to be non-Num_lock KP5
  { if ( (current_mode == MODE_SSB) and rig.rit_enabled() )
      rig.rit(0);

    processed = true;
  }

// CTRL-G -- display QRG of call
  if (!processed and (e.is_control('g')))
  { const int band_nr { static_cast<int>(current_band) };

    lock_guard lg(last_posted_qrg_mutex[band_nr]);

    if (const auto it { last_posted_qrg[band_nr].find(original_contents) }; it != last_posted_qrg[band_nr].end())
      win_last_qrg < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < original_contents < ": "s <= it->second;
    else
      win_last_qrg <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;

    processed = true;
  }

// ALT-G go to the frequency in win_last_qrg
  if (!processed and (e.is_alt('g')))
  { const string contents { win_last_qrg.read() };

    if (const size_t posn { contents.find(':') }; (posn != string::npos) and (posn != (contents.size() - 1)))
    { const frequency f { remove_peripheral_spaces <std::string> (substring <std::string> (contents, posn + 1)) };

      rig.rig_frequency(f);

      bandmap& bm { bandmaps[current_band] };

      win_bandmap <= bm;
    }

    processed = true;
  }

// finished processing a keypress
  if (processed and (win_active_p == &win_call))  // we might have changed the active window (if sending a QTC)
  { if (win_call.empty())
    { win_call.insert(true);                       // force INSERT mode
      win_info <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      win_batch_messages <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      win_call_history <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      win_qtc_hint < colour_pair(colours.add(win_qtc_hint_bg, win_qtc_hint_bg)) < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= " "s;
      win_individual_messages <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      update_qsls_window();                     // clears the window, except for the preliminary string

      if (display_grid)
        win_grid <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
    }
    else
    { if (const string current_contents { remove_char(remove_peripheral_spaces <std::string> (win.read()), BACKSLASH_CHAR) }; current_contents != original_contents)   // remove any \ characters
      { display_call_info(current_contents);

        if (!in_scp_matching)
        { update_scp_window(current_contents);
          update_fuzzy_window(current_contents);
          update_query_windows(current_contents);
        }
        else
        { //ost << "NOT UPDATING" << endl;
          //ost << "in_scp_matching = " << in_scp_matching << endl;
        }
      }
    }
  }
}

/*! \brief      Process input to the EXCHANGE window
    \param  wp  pointer to window associated with the event
    \param  e   keyboard event to process
*/
/*  ALT-D         -- screenshot and dump all bandmaps to output file [for debugging purposes]
    ALT-K         -- toggle CW
    ALT-N         -- toggle notch status if on SSB
    ALT-KP_4      -- decrement bandmap column offset; ALT-KP_6: increment bandmap column offset
    ALT-R         -- toggle RX antenna
    ALT-S         -- toggle sub receiver
    ALT-X         -- enter zoomed XIT mode

    CTRL-B -- fast bandwidth
    CTRL-S -- send contents of CALL window to scratchpad

    ENTER, KP_ENTER, ALT-Q -- thanks and log the contact; also perhaps start QTC process
    ESCAPE
    FULL STOP

    PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed

    COMMA -- place contents of call window into this window, preceeded by a dot

    SHIFT -- RIT control

    F4 -- swap contents of CALL and BCALL windows, EXCHANGE and BEXCHANGE windows
    KP- -- toggle 50Hz/200Hz bandwidth if on CW
*/
void process_EXCHANGE_input(window* wp, const keyboard_event& e)
{ window& win = *wp;                // syntactic sugar

  bool processed { win.common_processing(e) };

// BACKSPACE
  if (!processed and e.is_unmodified() and e.symbol() == XK_BackSpace)
    processed = process_backspace(win);

// SPACE
  if (!processed and e.is_char(' '))
    processed = (win <= e.str(), true);

// APOSTROPHE
  if (!processed and e.is_char('\''))
    processed = (win <= e.str(), true);

// CW messages
  if (!processed and cw_p and (current_mode == MODE_CW))
  { if (e.is_unmodified() and keypad_numbers.contains(e.symbol()) )
    { if (cw_p)
        (*cw_p) << expand_cw_message(cwm[e.symbol()]);

      processed = true;
    }
  }

// PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
  if (!processed and ((e.symbol() == XK_Next) or (e.symbol() == XK_Prior)))
    processed = change_cw_speed(e);

// ALT-K -- toggle CW
  if (!processed and e.is_alt('k'))
    processed = toggle_cw();

// ALT-N -- toggle notch status if on SSB
  if (!processed and e.is_alt('n'))
  { if (current_mode == MODE_SSB)
      rig.toggle_notch_status();

    processed = true;
  }

// ALT-X -- possibly enter zoomed XIT mode
  if (!processed and e.is_alt('x') and (drlog_mode == DRLOG_MODE::SAP))
    processed = zoomed_xit();

// CTRL-S -- send contents of CALL window to scratchpad
  if (!processed and e.is_control('s'))
    processed = send_to_scratchpad(remove_peripheral_spaces <std::string> (win_call.read()));

// ESCAPE
  if (!processed and e.symbol() == XK_Escape)
  { if (cw_p)
    { if (!cw_p->empty())    // abort sending CW if we are currently sending
        processed = (cw_p->abort(), true);
    }

 // clear the exchange window if there's something in it
    if (!processed and (!remove_peripheral_spaces <std::string> (win.read()).empty()))
      processed = (win <= WINDOW_ATTRIBUTES::WINDOW_CLEAR, true);

// go back to the call window
    if (!processed)
    { set_active_window(ACTIVE_WINDOW::CALL);
      processed = (win_call <= WINDOW_ATTRIBUTES::CURSOR_END_OF_LINE, true);
    }
  }

// COMMA -- place contents of call window into this window, preceded by a dot
  if (!processed and (e.is_char(',')))
    processed = (win <= (FULL_STOP + remove_peripheral_spaces <std::string> (win_call.read())), true);

// FULL STOP
  if (!processed and (e.is_char('.')))
    processed = (win <= FULL_STOP, true);

// ALT-KP_4: decrement bandmap column offset; ALT-KP_6: increment bandmap column offset
  if (!processed and e.is_alt() and ( (e.symbol() == XK_KP_4) or (e.symbol() == XK_KP_6)
                                     or (e.symbol() == XK_KP_Left) or (e.symbol() == XK_KP_Right) ) )
    processed = process_change_in_bandmap_column_offset(e.symbol());

// ENTER, KP_ENTER, ALT-Q -- thanks and log the contact; also perhaps start QTC process
  bool log_the_qso { !processed and ( e.is_unmodified() or e.is_alt() ) and ( (e.symbol() == XK_Return) or (e.symbol() == XK_KP_Enter) ) };
  bool send_qtc    { false };

  if (!log_the_qso)
  { log_the_qso = ( !processed and e.is_alt('q') and rules.send_qtcs() );
    send_qtc = log_the_qso;
  }

  if (log_the_qso)
  { const BAND   cur_band      { current_band };
    const MODE   cur_mode      { current_mode };
    const string call_contents { remove_peripheral_spaces <std::string> (win_call.read()) };

    string         exchange_contents     { squash(remove_peripheral_spaces <std::string> (win_exchange.read())) };
    vector<string> exchange_field_values { split_string <std::string> (exchange_contents, ' ') };

    string new_rst;

// figure out whether we have sent a different RST (in SKCC)
    constexpr char rst_character { '\'' };    // apostrophe

    if (contains(exchange_contents, rst_character))
    { const size_t last_apostrophe { exchange_contents.find_last_of(rst_character) };
      const size_t next_space      { exchange_contents.find_first_of(' ', last_apostrophe + 1) };

      size_t word_length;

      word_length = ( ( (next_space == string::npos) ? exchange_contents.length() : (next_space - 1) ) - last_apostrophe );
      new_rst     = substring <std::string> (exchange_contents, last_apostrophe + 1, word_length);

// remove all fields containing an apostrophe
      const vector<string> fields { split_string <std::string> (exchange_contents, ' ') };

      vector<string> new_fields;

      for (const string& str : exchange_field_values)
        if (!contains(str, '\''))
          new_fields += str;

      exchange_field_values = new_fields;
      exchange_contents = join(new_fields, SPACE_STR);
    }

    string from_callsign { call_contents };

// if there's an explicit replacement call, we might need to change the template
    for (const auto& value : exchange_field_values)
      if ( contains(value, '.') and (value.size() != 1) )    // ignore a field that is just "."
        from_callsign = remove_char(value, '.');

    const string                 canonical_prefix  { location_db.canonical_prefix(from_callsign) };
    const vector<exchange_field> exchange_template { rules.unexpanded_exch(canonical_prefix, cur_mode) };

    unsigned int n_optional_fields { 0 };

    FOR_ALL(exchange_template, [&n_optional_fields] (const exchange_field& ef) { if (ef.is_optional())
                                                                                   n_optional_fields++;
                                                                               } );

    bool sent_acknowledgement { false };

    if (!exchange_contents.empty())
    { size_t n_fields_without_new_callsign { 0 };

      for (const auto& values : exchange_field_values)
        if (!contains(values, '.'))
          n_fields_without_new_callsign++;

      if ( (!is_ss and (exchange_template.size() - n_optional_fields) > n_fields_without_new_callsign) )
      { ost << "mismatched template and exchange fields: Expected " << exchange_template.size() << " exchange fields; found " << to_string(n_fields_without_new_callsign) << " non-replacement-callsign fields" << endl;
        alert("Expected "s + to_string(exchange_template.size()) + " exchange fields; found "s + to_string(n_fields_without_new_callsign));
        processed = true;

        ost << "  exchange_template.size() = " << exchange_template.size() << endl;
        ost << "  n_optional_fields = " << n_optional_fields << endl;
        ost << "  n_fields_without_new_callsign = " << n_fields_without_new_callsign << endl;

        for (const auto& etf : exchange_template)
          ost << etf << endl;
      }

      if (!processed)
      { const parsed_exchange pexch { from_callsign, canonical_prefix, rules, cur_mode, exchange_field_values };  // this is relatively slow, but we can't send anything until we know that we have a valid exchange

        if (pexch.valid())
        { if ( (cur_mode == MODE_CW) and (cw_p) )         // don't acknowledge yet if we're about to send a QTC
          { if (exchange_field_values.size() == exchange_template.size())   // 1:1 correspondence between expected and received fields
            { if (drlog_mode == DRLOG_MODE::CQ)                             // send QSL
              { const bool quick_qsl { (e.symbol() == XK_KP_Enter) };

                if (!send_qtc)
                  (*cw_p) << expand_cw_message( quick_qsl ? alternative_qsl_message : qsl_message );
              }
              else                                                         // SAP exchange
              { if (!send_qtc)
                { (*cw_p) << expand_cw_message( e.is_unmodified() ? sap_exchange : alternative_sap_exchange );   // allow for an alternative SAP exchange
                  last_exchange = expand_cw_message(cq_exchange);       // the CQ exchange doesn't have a "TU" in it
                }
              }
              sent_acknowledgement = true;  // should rename this variable now that we've added QTC processing here
            }
          }

          if (!sent_acknowledgement)
          { if ( (cur_mode == MODE_CW) and (cw_p) and (drlog_mode == DRLOG_MODE::SAP))    // in SAP mode, he doesn't care that we might have changed his call
            { if (!send_qtc)
              { (*cw_p) << expand_cw_message(sap_exchange);
                last_exchange = expand_cw_message(cq_exchange);       // the CQ exchange doesn't have a "TU" in it
              }
            }

            if ( (cur_mode == MODE_CW) and (cw_p) and (drlog_mode == DRLOG_MODE::CQ))    // in CQ mode, he does
            { const vector<string> call_contents_fields { split_string <std::string> (call_contents, ' ') };
              const string         original_callsign    { call_contents_fields[call_contents_fields.size() - 1] };

              string callsign { original_callsign };

              if (pexch.has_replacement_call())
                callsign = pexch.replacement_call();

              if (callsign != original_callsign)    // callsign did not change
              { at_call = callsign;

                if (!send_qtc)
                  (*cw_p) << expand_cw_message(context.call_ok_now_message());
              }

// now send ordinary TU message
              const bool quick_qsl { (e.symbol() == XK_KP_Enter) };

              if (!send_qtc)
                (*cw_p) << expand_cw_message( quick_qsl ? alternative_qsl_message : qsl_message );
            }
          }

// generate the QSO info, then log it
          QSO qso;                        // automatically fills in date and time
          qso.number(next_qso_number);    // set QSO number

          set<pair<string /* field name */, string /* field value */> > exchange_mults_this_qso;    // needed when we do the bandmaps

// callsign is the last entry in the CALL window
          if (!call_contents.empty()) // to speed things up, probably should make this test earlier, before we send the exchange info
          { const vector<string> call_contents_fields { split_string <std::string> (call_contents, ' ') };
            const string         original_callsign    { call_contents_fields[call_contents_fields.size() - 1] };

            string callsign { original_callsign };

            if (pexch.has_replacement_call())
              callsign = pexch.replacement_call();

            qso.callsign(callsign);
            qso.canonical_prefix(location_db.canonical_prefix(callsign));
            qso.continent(location_db.continent(callsign));
            qso.mode(cur_mode);
            qso.band(cur_band);
            qso.my_call(context.my_call());
            qso.freq( frequency( rig_is_split ? rig.rig_frequency_b() : rig.rig_frequency() ).display_string() );    // in kHz; 1dp

// build name/value pairs for the sent exchange
            vector<pair<string, string> > sent_exchange { context.sent_exchange(qso.mode()) };

            for (auto& sent_exchange_field : sent_exchange)
            { if (sent_exchange_field.second == "#"sv)
                sent_exchange_field.second = serial_number_string(octothorpe);
            }

// in SKCC, we aren't using the computer to send CW, so we can determine the sent RST now, long after the fact
            switch (new_rst.length())
            { case 1 :                  // S
                new_rst = "5"s + new_rst + "9"s;
                break;

              case 2 :                  // RS
                new_rst += "9"s;
                break;

              case 3 :
              default :
                break;
            }

            if (!new_rst.empty())
            { for (auto& sent_exchange_field : sent_exchange)
              { if (sent_exchange_field.first == "RST"sv)
                  sent_exchange_field.second = new_rst;
              }
            }

            qso.sent_exchange(sent_exchange);

// build name/value pairs for the received exchange
            vector<received_field>        received_exchange { };
            vector<parsed_exchange_field> vec_pef           { pexch.chosen_fields(rules) };

// keep track of which fields are mults
            FOR_ALL(vec_pef, [] (auto& pef) { pef.is_mult(rules.is_exchange_mult(pef.name())); } );

            for (auto& pef : vec_pef)
            { const bool is_mult_field { pef.is_mult() };

              if (!variable_exchange_fields.contains(pef.name()))
                exchange_db.set_value(callsign, pef.name(), rules.canonical_value(pef.name(), pef.value()));   // add it to the database of exchange fields

// possibly add it to the canonical list, if it's a mult and the value is otherwise unknown
              if (is_mult_field)
              { if (rules.canonical_value(pef.name(), pef.value()).empty())
                  rules.add_exch_canonical_value(pef.name(), pef.mult_value());
                else                                                          // replace value with canonical value
                  pef.value(rules.canonical_value(pef.name(), pef.value()));
              }

              received_exchange += { pef.name(), pef.value(), is_mult_field, false };
            }

            qso.received_exchange(received_exchange);

// is this a country mult?
            if (country_mults_used and all_country_mults.contains(qso.canonical_prefix()))  // is it even possible that this is a country mult?
            { if (mm_country_mults or !is_maritime_mobile(qso.call()))
              { update_known_country_mults(qso.callsign(), KNOWN_MULT::FORCE_KNOWN);                                     // does nothing if not auto remaining country mults
                qso.is_country_mult( statistics.is_needed_country_mult(qso.callsign(), cur_band, cur_mode, rules) );     // set whether it's a country mult
              }
            }

// if callsign mults matter, add more to the qso
            allow_for_callsign_mults(qso);

// get the current list of country mults
            const MULT_SET old_worked_country_mults { statistics.worked_country_mults(cur_band, cur_mode) };

// and any exchange multipliers
            const STRING_MAP<MULT_SET /* values */ > old_worked_exchange_mults { statistics.worked_exchange_mults(cur_band, cur_mode) };
            const vector<exchange_field>                                        exchange_fields           { rules.expanded_exch(canonical_prefix, qso.mode()) };

            for (const auto& exch_field : exchange_fields)
            { const string& name  { exch_field.name() };
              const string  value { qso.received_exchange(name) };

              if (!value.empty())                       // don't add if a field was not received; e.g., if a CHOICE, don't add the unreceived field
              { if (context.auto_remaining_exchange_mults(name))
                  statistics.add_known_exchange_mult(name, MULT_VALUE(name, value));

                if (statistics.add_worked_exchange_mult(name, value, qso.band(), qso.mode()))
                  qso.set_exchange_mult(name);
              }
            }

            add_qso(qso);  // should also update the rates (but we don't display them yet; we do that after writing the QSO to disk)

// write the log line
            win_log < WINDOW_ATTRIBUTES::CURSOR_BOTTOM_LEFT < WINDOW_ATTRIBUTES::WINDOW_SCROLL_UP <= qso.log_line();
            win_exchange <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
            win_call <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
            win_nearby <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;

            if (send_qtcs)
            { qtc_buf += qso;
              statistics.qtc_qsos_unsent(qtc_buf.n_unsent_qsos());
              update_qtc_queue_window();
            }

// display the current statistics
            display_statistics(statistics.summary_string(rules));
            update_score_window(statistics.points(rules));
            set_active_window(ACTIVE_WINDOW::CALL);
            win_call <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;

// remaining mults: callsign, country, exchange
            update_known_callsign_mults(qso.callsign());
            update_remaining_callsign_mults_window(statistics, string { }, current_band, current_mode);

            if (old_worked_country_mults.size() != statistics.worked_country_mults(cur_band, cur_mode).size())
            { update_remaining_country_mults_window(statistics, cur_band, cur_mode);
              update_known_country_mults(qso.callsign(), KNOWN_MULT::FORCE_KNOWN);
            }

// was the just-logged QSO an exchange mult?
            const STRING_MAP<MULT_SET /* values */ > new_worked_exchange_mults { statistics.worked_exchange_mults(cur_band, cur_mode) };

            bool no_exchange_mults_this_qso { true };

            for (STRING_MAP<MULT_SET>::const_iterator cit { old_worked_exchange_mults.cbegin() }; cit != old_worked_exchange_mults.cend() and no_exchange_mults_this_qso; ++cit)
            { const size_t old_size { (cit->second).size() };

              if (const auto ncit { new_worked_exchange_mults.find(cit->first) }; ncit != new_worked_exchange_mults.cend())    // should never be equal
              { no_exchange_mults_this_qso = ( old_size == ncit->second.size() );   // has the size changed?

                if (!no_exchange_mults_this_qso)
                  update_remaining_exchange_mults_windows(/* rules, */ statistics, cur_band, cur_mode);
              }
            }

// what exchange mults came from this qso? there ought to be a better way of doing this
            if (!no_exchange_mults_this_qso)
            { for (const auto& current_exchange_mult : new_worked_exchange_mults)
              { STRING_SET difference;

                const MULT_SET& current_values { current_exchange_mult.second };

                if (const auto tmp { old_worked_exchange_mults.find(current_exchange_mult.first) }; tmp != old_worked_exchange_mults.end())
                { const MULT_SET& old_values { tmp->second };

                  set_difference(current_values.begin(), current_values.end(), old_values.begin(), old_values.end(), inserter(difference, difference.end()));
                }

                if (!difference.empty())  // assume that there's at most one entry
                  exchange_mults_this_qso += { current_exchange_mult.first, *(difference.begin()) };
              }
            }

// writing to disk is slow, so start the QTC now, if applicable
            if (send_qtc)
            { sending_qtc_series = false;           // initialise variable
              last_active_window = active_window;
              set_active_window(ACTIVE_WINDOW::LOG_EXTRACT);
              win_active_p->process_input(e);       // reprocess the alt-q
            }

// write to disk
            append_to_file(context.logfile(), (qso.verbose_format() + EOL) );
            update_rate_window();
          }

// possibly switch automatically to CQ mode
          if ( (logbk.size() > 1) and context.auto_cq_mode_ssb() and (drlog_mode == DRLOG_MODE::SAP) and (cur_mode == MODE_SSB) )
            if (logbk[logbk.size() - 1].mode() == MODE_SSB)
              if (abs(from_string<float>(logbk.last_qso().freq()) - from_string<float>(logbk[logbk.size() - 1].freq())) < 0.5)  // 500 Hz
                enter_cq_mode();

// perform any changes to the bandmaps

// if we are in CQ mode, then remove this call if it's present elsewhere in the bandmap ... we assume he isn't CQing and SAPing simultaneously on the same band
          bandmap& bandmap_this_band { bandmaps[cur_band] };

          if (drlog_mode == DRLOG_MODE::CQ)
          { bandmap_this_band -= qso.callsign();

// possibly change needed status of this call on other bandmaps
            if (!rules.work_if_different_band())
              FOR_ALL(bandmaps, [&qso] (bandmap& bm) { bm.not_needed(qso.callsign()); } );
          }
          else    // SAP; if we are in SAP mode, we may need to change the work/mult designation in the bandmap
          {
// add the stn to the bandmap
             bandmap_entry be;

             be.freq( rig_is_split ? rig.rig_frequency_b() : rig.rig_frequency() );
             be.callsign(qso.callsign());
             be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);
             be.is_needed(false);

             bandmap_this_band += be;
          }

// callsign mult status
          if (callsign_mults_used)
          { if (rules.callsign_mults_per_band())
            { for (const auto& callsign_mult_name : rules.callsign_mults())
              { const string target_value { callsign_mult_value(callsign_mult_name, qso.callsign()) };

                bandmap_this_band.not_needed_callsign_mult(&callsign_mult_value, callsign_mult_name, target_value);
              }
            }
            else
            { for (const auto& callsign_mult_name : rules.callsign_mults())
              { const string target_value { callsign_mult_value(callsign_mult_name, qso.callsign()) };

                FOR_ALL(bandmaps, [&callsign_mult_name, &target_value] (bandmap& bm) { bm.not_needed_callsign_mult( &callsign_mult_value, callsign_mult_name, target_value ); } );
              }
            }
          }

// country mult status
          if (country_mults_used)
          { if (const string canonical_prefix { location_db.canonical_prefix(qso.callsign()) }; rules.country_mults_per_band())
            { if (rules.country_mults_per_mode())
                bandmap_this_band.not_needed_country_mult(canonical_prefix, qso.mode());    // just change mults for this mode on the bandmap
              else
                bandmap_this_band.not_needed_country_mult(canonical_prefix);                // change mults for all modes on the bandmap
            }
            else                                                                            // country mults are not per band
              FOR_ALL(bandmaps, [&canonical_prefix] (bandmap& bm) { bm.not_needed_country_mult(canonical_prefix); } );
          }

// exchange mult status
          if (exchange_mults_used and !exchange_mults_this_qso.empty())
          { if (rules.exchange_mults_per_band())
            { for (const auto& [field_name, field_value] : exchange_mults_this_qso)
                bandmap_this_band.not_needed_exchange_mult(field_name, field_value);
            }
            else
            { for (const auto& [field_name, field_value] : exchange_mults_this_qso)
                FOR_ALL(bandmaps, [&field_name, &field_value] (bandmap& bm) { bm.not_needed_exchange_mult(field_name, field_value); } );
            }
          }

          win_bandmap <= bandmap_this_band;

// keep track of QSO number
          win_serial_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(serial_number_string(++octothorpe), win_serial_number.width());
          next_qso_number = logbk.n_qsos() + 1;
          win_qso_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(next_qso_number, win_qso_number.width());

          display_call_info(qso.callsign(), DO_NOT_DISPLAY_EXTRACT);
          update_mult_value();

// possibly update BEST DX window
          if (win_best_dx.valid())
            update_best_dx(grid_square { qso.received_exchange("GRID"sv) }, qso.callsign());
        }                                                     // end pexch.valid()
        else        // unable to parse exchange
          alert("Unable to parse exchange"sv);

        processed = true;
      }
    }

// ensure that CALL and EXCHANGE windows are in INSERT mode
    win_call.insert(true);
    win_exchange.insert(true);

// possibly start audio recording; perhaps this should go elsewhere?
    if ( allow_audio_recording and (context.start_audio_recording() == AUDIO_RECORDING::AUTO) and !audio.recording())
    { start_recording(audio, context);
      alert("audio recording started due to activity"s);
    }

// if on SSB and using RIT, centre the RIT
    if ( (current_mode == MODE_SSB) and rig.rit_enabled() )
      rig.rit(0);
  }        // end ENTER [log_the_qso]

// SHIFT -- RIT control
// RIT changes via hamlib, at least on the K3, are *very* slow
  if (!processed and (e.event() == KEY_EVENT::PRESS) and ( (e.symbol() == XK_Shift_L) or (e.symbol() == XK_Shift_R) ) )
    processed = shift_control(e);

// ALT-S -- toggle sub receiver
  if (!processed and e.is_alt('s'))
  { try
    { rig.sub_receiver_toggle();
    }

    catch (const rig_interface_error& e)
    { alert( (string)"Error toggling SUBRX: "s + e.reason());
    }

    processed = true;
  }

// ` -- SWAP RIT and XIT
  if (!processed and e.is_char('`'))
    processed = swap_rit_xit();

// ALT-D -- debug dump
  if (!processed and e.is_alt('d'))
    processed = debug_dump();

// CTRL-CURSOR LEFT -- left one word
  if (!processed and e.is_ctrl() and e.symbol() == XK_Left)
  { if (const cursor original_posn { win.cursor_position() }; original_posn.x() != 0)    // do nothing if at start of line
    { const string contents { win.read(0, original_posn.y()) };

      if (const vector<size_t> word_posn { starts_of_words(contents) }; word_posn.empty())                              // there are no words
        win <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
      else                                                // there are some words
      { size_t index;

        for (index = 0; index < word_posn.size(); ++index)
        { if (static_cast<int>(word_posn[index]) == original_posn.x())
          { if (index == 0)                  // we are at the start of the first word
            { win <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
              break;
            }
            else
            { win <= cursor(word_posn[index - 1], original_posn.y());  // are at the start of a word (but not the first word)
              break;
            }
          }

          if (static_cast<int>(word_posn[index]) > original_posn.x())
          { if (index == 0)                          // should never happen; cursor is to left of first word
            { win <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
              break;
            }
            else
            { win <= cursor(word_posn[index - 1], original_posn.y());  // go to the start of the current word
              break;
            }
          }
        }

        if (index == word_posn.size())
          win <= cursor(word_posn[word_posn.size() - 1], original_posn.y());  // go to the start of the current word
      }
    }

    processed = true;
  }

// CTRL-CURSOR RIGHT -- right one word
  if (!processed and e.is_ctrl() and e.symbol() == XK_Right)
  { const cursor original_posn      { win.cursor_position() };
    const string contents           { win.read(0, original_posn.y()) };
    const string truncated_contents { remove_trailing_spaces <std::string> (contents) };

    if (truncated_contents.empty())                // there are no words
      win <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
    else                                           // there is at least one word
    { const size_t         last_filled_posn { truncated_contents.size() - 1 };

      if (const vector<size_t> word_posn { starts_of_words(contents) }; word_posn.empty())                // there are no words; should never be true at this point
        win <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
      else
      { if (original_posn.x() >= static_cast<int>(word_posn[word_posn.size() - 1]))  // at or after start of last word
          win <= cursor(last_filled_posn + 2, original_posn.y());
        else
        { for (size_t index { 0 }; index < word_posn.size(); ++index)
          { if (static_cast<int>(word_posn[index]) == original_posn.x())        // at the start of a word (but not the last word)
            { win <= cursor(word_posn[index + 1], original_posn.y());
              break;
            }

            if (static_cast<int>(word_posn[index]) > original_posn.x())
            { win <= cursor(word_posn[index], original_posn.y());
              break;
            }
          }
        }
      }
    }

    processed = true;
  }

// CTRL-T -- delete word
  if (!processed and e.is_control('t'))
  { const cursor         original_posn { win.cursor_position() };
    const string         contents      { win.read(0, original_posn.y()) };

    if (const vector<size_t> word_posn { starts_of_words(contents) }; !word_posn.empty())
    { const bool is_space { (contents[original_posn.x()] == ' ') };

      if (!is_space)
      { size_t start_current_word { 0 };

        for (size_t n { 0 }; n < word_posn.size(); ++n)
          if (static_cast<int>(word_posn[n]) <= original_posn.x())
            start_current_word = word_posn[n];

        if (const size_t end_current_word { contents.find_first_of(' ', original_posn.x()) }; end_current_word != string::npos)  // word is followed by space
        { string new_contents { (start_current_word != 0) ? substring <std::string> (contents, 0, start_current_word) : string() };

          new_contents += substring <std::string> (contents, end_current_word + 1);

          win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < new_contents <= cursor(start_current_word, original_posn.y());
        }
        else
        { const string new_contents { (start_current_word != 0) ? substring <std::string> (contents, 0, start_current_word - 1) : string { } };

          win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < new_contents <= cursor(start_current_word, original_posn.y());
        }
      }
      else  // we are at a space
      { if (const size_t next_start { next_word_posn(contents, original_posn.x()) }; next_start != string::npos)
        { if (const size_t next_end { contents.find_first_of(' ', next_start) }; next_end != string::npos)    // there is a complete word following
          { const string new_contents { substring <std::string> (contents, 0, next_start) + substring <std::string> (contents, next_end + 1) };

            win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < new_contents <= cursor(original_posn.x() + 1, original_posn.y());
          }
          else
          { const string new_contents { substring <std::string> (contents, 0, next_start - 1) };

            win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < new_contents <= original_posn;
          }
        }
      }
    }

    processed = true;
  }

// F5 -- combine F2 and F4
  if (!processed and (e.symbol() == XK_F5))
    processed = process_keypress_F5();

// CTRL-P -- dump screen
  if (!processed and e.is_control('p'))
    processed = (dump_screen(), true);
    
// CTRL-ENTER -- repeat last message if in CQ mode
  if (!processed and e.is_control() and (e.symbol() == XK_Return) and (drlog_mode == DRLOG_MODE::CQ))
  { if (cw_p)
      (*cw_p) << expand_cw_message("*"s);

    processed = true;
  }

// CTRL-B -- fast CW bandwidth
  if (!processed and (e.is_control('b')))
    processed = fast_cw_bandwidth();

// F2 toggle: split and force SAP mode
  if (!processed and (e.symbol() == XK_F2))
  { if (rig.split_enabled())
    { rig.split_disable();

      (a_drlog_mode == DRLOG_MODE::CQ) ? enter_cq_mode() : enter_sap_mode();
    }
    else
    { rig.split_enable();
      a_drlog_mode = drlog_mode;
      enter_sap_mode();
    }

    processed = true;
  }

// F4 -- swap contents of CALL and BCALL windows, EXCHANGE and BEXCHANGE windows
  if (!processed and (e.symbol() == XK_F4))
  { if (win_bcall.defined())
    { const string tmp   { win_call.read() };
      const string tmp_b { win_bcall.read() };

      win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp_b;
      win_bcall < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp;

      const string call_contents { tmp_b };

      string exchange_contents;

      if (win_bexchange.defined())
      { const string tmp   { win_exchange.read() };
        const string tmp_b { win_bexchange.read() };

        win_exchange < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp_b;

        exchange_contents = tmp_b;

        win_bexchange < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp;
      }

// put cursor in correct window
      if (remove_peripheral_spaces <std::string> (win_exchange.read()).empty())        // go to the CALL window
      { const size_t posn { call_contents.find(' ') };            // first empty space

        win_call.move_cursor(posn, 0);
        win_call.refresh();
        set_active_window(ACTIVE_WINDOW::CALL);
        win_exchange.move_cursor(0, 0);
      }
      else
      { if (const size_t posn { exchange_contents.find_last_of(DIGITS_AND_UPPER_CASE_LETTERS) }; posn != string::npos)  // first empty space
        { win_exchange.move_cursor(posn + 1, 0);
          win_exchange.refresh();
          set_active_window(ACTIVE_WINDOW::EXCHANGE);
        }
      }
    }

    processed = true;
  }

// ALT-R -- toggle RX antenna
  if (!processed and (e.is_alt('r')))
  { rig.toggle_rx_ant();
    processed = update_rx_ant_window();
  }

// KP- -- toggle 50Hz/200Hz bandwidth if on CW
  if (!processed and e.is_unmodified() and e.symbol() == XK_KP_Subtract)
    processed = cw_toggle_bandwidth();
}

/*! \brief      Process input to the (editable) LOG window
    \param  wp  pointer to window associated with the event
    \param  e   keyboard event to process
*/
void process_LOG_input(window* wp, const keyboard_event& e)
{
// syntactic sugar
  window& win { *wp };

  bool processed { win.common_processing(e) };

// BACKSPACE -- just move cursor to left
  if (!processed and e.is_unmodified() and e.symbol() == XK_BackSpace)
    processed = (win <= cursor_relative(-1, 0), true);

// SPACE
  if (!processed and e.is_char(' '))
    processed = (win <= e.str(), true);

// CURSOR UP
  if (!processed and e.is_unmodified() and e.symbol() == XK_Up)
    processed = (win <= WINDOW_ATTRIBUTES::CURSOR_UP, true);

// CURSOR DOWN
  if (!processed and e.is_unmodified() and e.symbol() == XK_Down)
  { const cursor posn { win.cursor_position() };

    if (posn.y() != 0)
      win <= WINDOW_ATTRIBUTES::CURSOR_DOWN;
    else                                    // bottom line
    { win_log.toggle_hidden();              // hide cursor
      win_log < WINDOW_ATTRIBUTES::WINDOW_REFRESH;

      const vector<string> new_win_log_snapshot { win_log.snapshot() };  // [0] is the top line

// has anything been changed?
      bool changed { false };

      for (size_t n { 0 }; (!changed and n < new_win_log_snapshot.size()); ++n)
        changed = (new_win_log_snapshot[n] != win_log_snapshot[n]);

      if (changed)
      {
// get the original QSOs that were in the window
        int number_of_qsos_in_original_window { 0 };

        for (size_t n { 0 }; n < win_log_snapshot.size(); ++n)
          if (!remove_peripheral_spaces <std::string_view> (win_log_snapshot[n]).empty())
            number_of_qsos_in_original_window++;

        deque<QSO> original_qsos;

        unsigned int qso_number  { static_cast<unsigned int>(logbk.size()) };
        unsigned int n_to_remove { 0 };

        for (size_t n { 0 }; n < win_log_snapshot.size(); ++n)
        { if (remove_peripheral_spaces <std::string_view> (win_log_snapshot[win_log_snapshot.size() - 1 - n]).empty())
            original_qsos.push_front(QSO());
          else
          { ost << "adding original QSO: " << "----------" << endl << logbk[qso_number] << endl;
            ost << "----------" << endl;
            original_qsos.push_front(logbk[qso_number--]);
            n_to_remove++;
          }
        }

// debug: print out the original QSOs
        ost << "Original QSOs:" << endl;
        for (const auto& qso : original_qsos)
        { if (!qso.empty())
          { ost << "QSO with " << qso.callsign() << endl;
            ost << "    " << qso << endl;
          }
          else
            ost << "Blank QSO" << endl;
        }
        ost << "New QSOs: " << endl << endl;

        logbk.remove_last_qsos(n_to_remove);                        // remove that number of QSOs from the log
        rebuild_history(logbk, rules, statistics, q_history, rate);

// we will work out the replacement QSOs, sort out all the statistics, then put them in the
// log window at:  editable_log.recent_qsos(logbk, true); about 100 lines below

// add the new QSOs
        for (size_t n { 0 }; n < new_win_log_snapshot.size(); ++n)
        { if (!remove_peripheral_spaces <std::string_view> (new_win_log_snapshot[n]).empty())
          { QSO qso { original_qsos[n] };           // start with the original QSO as a basis *** THIS IS A PROBLEM, AS THE MEANING OF AN EXCHANGE COLUMN MIGHT CHANGE

            qso.log_line();                         // populate _log_line_fields

// fills some fields in the QSO
            qso.populate_from_log_line(remove_peripheral_spaces <std::string_view> (new_win_log_snapshot[n]));  // note that this doesn't fill all fields (e.g. _my_call), which are carried over from original QSO
            ost << "QSO after populate_from_log_line: " << qso << endl;

// we can't assume anything about the mult status
            update_known_callsign_mults(qso.callsign());
            update_known_country_mults(qso.callsign(), KNOWN_MULT::FORCE_KNOWN);

// is this a country mult?
            qso.is_country_mult(statistics.is_needed_country_mult(qso.callsign(), qso.band(), qso.mode(), rules));
              
// exchange mults
            if (exchange_mults_used)
              calculate_exchange_mults(qso, rules);

// if callsign mults matter, add more to the qso
            allow_for_callsign_mults(qso);

            ost << "QSO to be added back into log: " << qso << endl;

// add it to the running statistics; do this before we add it to the log so we can check for dupes against the current log
            statistics.add_qso(qso, logbk, rules);
            logbk += qso;

// possibly change values in the exchange database
            const vector<received_field> fields { qso.received_exchange() };

            for (const auto& field : fields)
              if (!variable_exchange_fields.contains(field.name()))
               exchange_db.set_value(qso.callsign(), field.name(), rules.canonical_value(field.name(), field.value()));   // add it to the database of exchange fields

// pretend that we just entered this station on the bandmap by hand
// do this just for QSOs that have changed
            if (!contains(original_qsos, qso))
            { bandmap& bm { bandmaps[qso.band()] };

              bandmap_entry be;                       // defaults to manual entry

              be.freq(frequency(qso.freq()));
              be.mode(qso.mode());
              be.callsign(qso.callsign());
              be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);
              be.calculate_mult_status(rules, statistics);
              be.is_needed(false);                    // assumes that can't work him again on same band/mode

              bm += be;
            }
          }
        }

// the logbook is now rebuilt
        if (send_qtcs)
        { qtc_buf.rebuild_unsent_list(logbk);
          update_qtc_queue_window();
        }

// re-write the logfile
        { FILE* fp { fopen(context.logfile().c_str(), "w") };

          if (fp)
          { for (const auto& qso : logbk.as_vector())
            { const string line_to_write { qso.verbose_format() + EOL };

              fwrite(line_to_write.c_str(), line_to_write.length(), 1, fp);
            }

            fclose(fp);
          }
          else
            alert("Unable to open log file "s + context.logfile() + " for writing"s);
        }

        rebuild_history(logbk, rules, statistics, q_history, rate);
        rescore(rules);
        update_rate_window();
        rebuild_dynamic_call_databases(logbk);

// all the QSOs are added; now display the last few in the log window
//        editable_log.recent_qsos(logbk, true);
        editable_log.recent_qsos(logbk, LOG_EXTRACT::DISPLAY);

        display_statistics(statistics.summary_string(rules));   // display the current statistics
        update_score_window(statistics.points(rules));

        const BAND cur_band { current_band };
        const MODE cur_mode { current_mode };

        update_remaining_callsign_mults_window(statistics, string(), cur_band, cur_mode);
        update_remaining_country_mults_window(statistics, cur_band, cur_mode);
        update_remaining_exchange_mults_windows(/* rules, */ statistics, cur_band, cur_mode);

        next_qso_number = logbk.n_qsos() + 1;
        win_qso_number < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= pad_left(to_string(next_qso_number), win_qso_number.width());

// perform any changes to the bandmaps;
// there is trickiness here because of threading
// the following is safe, but may lose information if it comes in from the RBN/cluster
// threads while we are processing the bandmap here
//        for (auto& bm : bandmaps)
//          bm.remark();

//        ost << "LOOKING AT BANDMAPS" << endl;

// test --- change current bandmap
        for (auto& bm : bandmaps)
        { for (bandmap_entry& be : bm.entries())
          { if (be.remark(rules, q_history, statistics))
              bm += be;
            else
            { //ost << "NOT remarked following log edit: " << be << endl;
            }
          }

          if (&bm == &(bandmaps[current_band]))
            win_bandmap <= bm;
        }
      }

      set_active_window(ACTIVE_WINDOW::CALL);

      const string call_contents { remove_trailing_spaces <std::string> (win_call.read()) };

      win_call.move_cursor(call_contents.size(), 0);
      win_call.refresh();
    }

    processed = true;
  }

// ALT-Y -- delete current line
  if (!processed and e.is_alt('y'))
  { const cursor posn { win.cursor_position() };

    processed = (win < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < WINDOW_ATTRIBUTES::WINDOW_CLEAR_TO_EOL <= posn, true);
  }

// ESCAPE
  if (!processed and e.symbol() == XK_Escape)
  {
// go back to CALL window, without making any changes
    set_active_window(ACTIVE_WINDOW::CALL);

    win_log.hide_cursor();
//    editable_log.recent_qsos(logbk, true);
    editable_log.recent_qsos(logbk, LOG_EXTRACT::DISPLAY);

    processed = (win_call < WINDOW_ATTRIBUTES::WINDOW_REFRESH, true);
  }

// ALT-D -- debug dump
  if (!processed and e.is_alt('d'))
    processed = debug_dump();

// CTRL-P -- dump screen
  if (!processed and e.is_control('p'))
    processed = (dump_screen(), true);
}

// functions that include thread safety

/// enter CQ mode
void enter_cq_mode(void)
{
  { SAFELOCK(drlog_mode);  // don't use SAFELOCK_SET because we want to set the frequency and the mode as an atomic operation

    cq_mode_frequency = rig.rig_frequency();  // nested inside other lock
    drlog_mode = DRLOG_MODE::CQ;
  }

  win_drlog_mode < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= "CQ"s;

  try
  { if (context.cq_auto_lock())
      rig.lock();

    rig.disable_xit();

    if (context.cq_auto_rit())
    { rig.enable_rit();
      rig.rit(0);
    }

    p3_span(p3_span_cq);
  }

  catch (const rig_interface_error& e)
  { alert("Error communicating with rig when entering CQ mode"s);
  }
}

/// enter SAP mode
void enter_sap_mode(void)
{ SAFELOCK_SET(drlog_mode_mutex, drlog_mode, DRLOG_MODE::SAP);
  win_drlog_mode < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= "SAP"s;

  try
  { rig.unlock();
    rig.rit(0);
    rig.disable_xit();
    rig.disable_rit();
    p3_span(p3_span_sap);
  }

  catch (const rig_interface_error& e)
  { alert("Error communicating with rig when entering SAP mode"s);
  }

  SAFELOCK(wicm);

  win_wicm.clear();
  wicm_calls.clear();
  wicm_calls_is_dirty = false;
}

/// toggle between CQ mode and SAP mode
bool toggle_drlog_mode(void)
{ (SAFELOCK_GET(drlog_mode_mutex, drlog_mode) == DRLOG_MODE::CQ) ? enter_sap_mode() : enter_cq_mode();

  return true;
}

/// enter CQ or SAP mode
void enter_cq_or_sap_mode(const DRLOG_MODE new_mode)
{ if (new_mode == DRLOG_MODE::CQ)
    enter_cq_mode();
  else
    enter_sap_mode();
}

/*! \brief              Update the REMAINING CALLSIGN MULTS window for a particular mult
    \param  statistics  current statistics for the contest
    \param  mult_name   name of the callsign mult
    \param  b           current band
    \param  m           current mode
*/
//void update_remaining_callsign_mults_window(running_statistics& statistics, const string& mult_name, const BAND b, const MODE m)
void update_remaining_callsign_mults_window(running_statistics& statistics, const string_view mult_name, const BAND b, const MODE m)
{ const MULT_SET worked_callsign_mults { statistics.worked_callsign_mults(mult_name, b, m) };

// the original list of callsign mults
  STRING_SET original;

  if (context.auto_remaining_callsign_mults())
    SAFELOCK_SET(known_callsign_mults_mutex, original, known_callsign_mults);
  else
    original = context.remaining_callsign_mults_list();

  if (filter_remaining_country_mults)
    erase_if(original, [&worked_callsign_mults] (const string& s) { return (worked_callsign_mults.contains(s)); } );

// put in right order and get the colours right
//  vector<string> vec_str(original.begin(), original.end());

//  vector<string> vec_str { SR::to<vector<string>>(original) };
//  SORT(vec_str, compare_calls);

  vector<pair<string /* prefix */, PAIR_NUMBER_TYPE /* colour pair number */ > > vec;

//  for (const auto& canonical_prefix : vec_str)
  for (const auto& canonical_prefix : SR::to<CALL_SET>(original))
  { const bool             is_needed          { !worked_callsign_mults.contains(canonical_prefix) };
    const PAIR_NUMBER_TYPE colour_pair_number { colours.add( ( is_needed ? win_remaining_callsign_mults.fg() : context.worked_mults_colour() ), win_remaining_callsign_mults.bg()) };

    vec += { canonical_prefix, colour_pair_number };
  }

  win_remaining_callsign_mults < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::WINDOW_TOP_LEFT <= vec;
}

/*! \brief              Update the REMAINING COUNTRY MULTS window
    \param  statistics  current statistics for the contest
    \param  b           current band
    \param  m           current mode
*/
void update_remaining_country_mults_window(running_statistics& statistics, const BAND b, const MODE m)
{ const MULT_SET worked_country_mults { statistics.worked_country_mults(b, m) };
  const MULT_SET known_country_mults  { statistics.known_country_mults() };

// put in right order and get the colours right
//  vector<string> vec_str(known_country_mults.begin(), known_country_mults.end());
//  vector<string> vec_str(known_country_mults.begin(), known_country_mults.end());
//  SORT(vec_str, compare_calls);

  vector<pair<string /* country */, PAIR_NUMBER_TYPE /* colour pair number */ > > vec;

//  for (const auto& canonical_prefix : vec_str)
  for (const auto& canonical_prefix : SR::to<CALL_SET>(known_country_mults))            // actually, CALL_SET is currently the same as MULT_SET
  { const bool             is_needed          { !worked_country_mults.contains(canonical_prefix) };
    const PAIR_NUMBER_TYPE colour_pair_number { colours.add( is_needed ? win_remaining_country_mults.fg() : context.worked_mults_colour(), win_remaining_country_mults.bg()) };

    vec += { canonical_prefix, colour_pair_number };
  }

  win_remaining_country_mults < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::WINDOW_TOP_LEFT <= vec;
}

/*! \brief                  Update the REMAINING EXCHANGE MULTS window for a particular mult
    \param  exch_mult_name  name of the exchange mult
    \param  rules           rules for the contest
    \param  statistics      current statistics for the contest
    \param  b               current band
    \param  m               current mode

    Does nothing if there is no window for this exchange mult
*/
//void update_remaining_exch_mults_window(const string& exch_mult_name, /* const contest_rules& rules, */ running_statistics& statistics, const BAND b, const MODE m)
void update_remaining_exch_mults_window(const string_view exch_mult_name, running_statistics& statistics, const BAND b, const MODE m)
{ if (!win_remaining_exch_mults_p.contains(exch_mult_name))
    return;

  const MULT_SET       known_exchange_values_set { statistics.known_exchange_mult_values(exch_mult_name) };
  const vector<string> known_exchange_values     { known_exchange_values_set.cbegin(), known_exchange_values_set.cend() };

  window& win { ( *(win_remaining_exch_mults_p[ string { exch_mult_name } ]) ) };

// get the colours right
  vector<pair<string /* exch value */, PAIR_NUMBER_TYPE /* colour pair number */ > > vec;

  for (const auto& known_value : known_exchange_values)
  { const bool             is_needed          { statistics.is_needed_exchange_mult(exch_mult_name, known_value, b, m) };
    const PAIR_NUMBER_TYPE colour_pair_number { ( is_needed ? colours.add(win.fg(), win.bg()) : colours.add(context.worked_mults_colour(),  win.bg())) };

    vec += { known_value, colour_pair_number };
  }

  win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::WINDOW_TOP_LEFT <= vec;
}

/*! \brief              Update the REMAINING EXCHANGE MULTS windows for all exchange mults with windows
    \param  rules       rules for the contest
    \param  statistics  current statistics for the contest
    \param  b           current band
    \param  m           current mode
*/
void update_remaining_exchange_mults_windows(running_statistics& statistics, const BAND b, const MODE m)
{ for (const auto& [exch_mult_name, _] : win_remaining_exch_mults_p)
    update_remaining_exch_mults_window(exch_mult_name, statistics, b, m);
}

/*! \brief              Return the bearing to a station
    \param  callsign    target call
    \return             bearing, in degrees, to <i>callsign</i>

    Returned string includes the degree sign. Uses grid if known, otherwise
    cty.dat
*/
string bearing(const string_view callsign)
{ static const string degree { "°"s };

  if (callsign.empty())
    return string { };

  const auto  [lat2, long2] { latitude_and_longitude(callsign) };
  const float b             { bearing(my_latitude, my_longitude, lat2, long2) };
  
  int ibearing { static_cast<int>(b + 0.5) };

  if (ibearing < 0)
    ibearing += 360;

  return (to_string(ibearing) + degree);
}

/*! \brief                  Calculate the sunrise or sunset time for a station
    \param  callsign        call of the station for which sunrise or sunset is desired
    \param  calc_sunset     whether to calculate sunset
    \return                 sunrise or sunset in the form HH:MM

    Returns "DARK" if it's always dark, and "LIGHT" if it's always light
 */
string sunrise_or_sunset(const string_view callsign, const SRSS srss)
{ const auto [lat, lon] { latitude_and_longitude(callsign) };

  return sunrise_or_sunset(lat, lon, srss);
}

/*! \brief              Populate the information window
    \param  callsign    full or partial call

    Called multiple times as a call is being typed. Also populates the following windows as appropriate:
      CALL HISTORY
      GRID
      INDIVIDUAL QTC COUNT
      NAME
      PUTATIVE EXCHANGE
 */
void populate_win_info(const string_view callsign)
{ if (win_call_history.valid())
    populate_win_call_history(callsign);

  if (send_qtcs)
  { const string qtc_str { "["s + to_string(qtc_db.n_qtcs_sent_to(callsign)) + "]"s };

    win_info < WINDOW_ATTRIBUTES::WINDOW_CLEAR < qtc_str <= centre(callsign, win_info.height() - 1);    // write the (partial) callsign
    win_individual_qtc_count < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= pad_left(qtc_str, 4);   // right justify
  }
  else
    win_info < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= centre(callsign, win_info.height() - 1);    // write the (partial) callsign
  
  if (display_grid)
  { const string grid_name { exchange_db.guess_value(callsign, "GRID"s) };

    win_grid < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

    if (!grid_name.empty())
      win_grid < grid_name;

    win_grid.refresh();
  }

  if (!names.empty())    // if we have some names from the drmaster file
  { win_name < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

    if (const string this_name { MUM_VALUE(names, callsign) }; !this_name.empty())
      win_name < this_name;

    win_name.refresh();
  }
  
  const string name_str { location_db.country_name(callsign) };            // name of the country

  if (to_upper(name_str) != "NONE"sv)
  { const string sunrise_time { sunrise(callsign) };
    const string sunset_time  { sunset(callsign) };
    const string current_time { substring <std::string> (hhmmss(), 0, 5) };
    const bool   daylight     { is_daylight(sunrise_time, sunset_time, current_time) };

    win_info < cursor(0, win_info.height() - 2) < location_db.canonical_prefix(callsign) < ": "s
                                                < pad_left(bearing(callsign), 5)         < SPACE_STR
                                                < sunrise_time                           < "/"s      < sunset_time
                                                < (daylight ? "(D)"s : "(N)"s);
    const string name_plus_continent_str { name_str + " ["s + location_db.continent(callsign) + "]"s };
    const size_t len                     { name_plus_continent_str.size() };

    win_info < cursor(win_info.width() - len, win_info.height() - 2) <= name_plus_continent_str;

    constexpr unsigned int FIRST_FIELD_WIDTH { 15 };     // "QTHX[ON] [XXX*]", for UBA contest
    constexpr unsigned int FIELD_WIDTH       { 4 };      // width of other fields

    int next_y_value { win_info.height() - 3 };                 // keep track of where we are vertically in the window

    for (const auto this_mode : rules.permitted_modes())
    { if (n_modes > 1)
        win_info < cursor(0, next_y_value--) < WINDOW_ATTRIBUTES::WINDOW_REVERSE < centred_string(MODE_NAME[this_mode], win_info.width()) < WINDOW_ATTRIBUTES::WINDOW_NORMAL;

// QSOs
      string line { pad_right("QSO"s, FIRST_FIELD_WIDTH) };

      for (const BAND b : permitted_bands)
        line += pad_left( ( q_history.worked(callsign, b, this_mode) ? "-"s : BAND_NAME[b] ), FIELD_WIDTH);

      win_info < cursor(0, next_y_value--) < line;

// country mults
      const string canonical_prefix { location_db.canonical_prefix(callsign) };

      if (!all_country_mults.empty() or auto_remaining_country_mults)
      { if (all_country_mults.contains(canonical_prefix) )                                          // all_country_mults is from rules, and has all the valid mults for the contest
        { const MULT_SET known_country_mults { statistics.known_country_mults() };

          line = pad_right("Country ["s + canonical_prefix + "]"s, FIRST_FIELD_WIDTH);

          for (const BAND b : permitted_bands)
          { const string per_band_indicator { known_country_mults.contains(canonical_prefix) ? (statistics.is_needed_country_mult(callsign, b, this_mode, rules) ? BAND_NAME[b] : "-"s )
                                                                                             : BAND_NAME.at(b)
                                            };

            line += pad_left(per_band_indicator, FIELD_WIDTH);
          }

          win_info < cursor(0, next_y_value--) < line;
        }
      }

// exch mults
      for (const string& exch_mult_field : rules.exchange_mults())
      { if (const bool output_this_mult { rules.is_exchange_field_used_for_country(exch_mult_field, canonical_prefix) }; output_this_mult)
        { const string exch_mult_value { exchange_db.guess_value(callsign, exch_mult_field) };      // make best guess as to the value of this field

          line = pad_right(exch_mult_field + " ["s + exch_mult_value + "]"s, FIRST_FIELD_WIDTH);

          for (const BAND b : permitted_bands)
            line += pad_left( ( statistics.is_needed_exchange_mult(exch_mult_field, exch_mult_value, b, this_mode) ? BAND_NAME.at(b) : "-"s ), FIELD_WIDTH);

          win_info < cursor(0, next_y_value-- ) < line;
        }
      }

// PUTATIVE EXCHANGE window
      if (win_putative_exchange.valid())
      { if (const string expected_exchange { expected_received_exchange(callsign) }; !expected_exchange.empty())
        { const string msg { centred_string("["s + expected_exchange + "]"s, win_putative_exchange.width()) };
        
          win_putative_exchange < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= msg;
        }
        else
          win_putative_exchange <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      }

// mults based on the callsign (typically a prefix mult)
/*! \brief              Set the callsign mult value, according to rules for a particular prefix-based contest
    \param  val         the value to set; this gets changed
    \param  b           whether to perform the calculation
    \param  pf          pointer to function that calculates the prefix
    \param  callsign    call for which the prefix is to be calculated
*/
      auto SET_CALLSIGN_MULT_VALUE = [] (string& val, const bool b, string (*pf)(const string_view), const string_view callsign)
                                         {  if (b and val.empty())
                                              val = pf(callsign);
                                         };

      const STRING_SET callsign_mults { rules.callsign_mults() };  // in practice, has at most one element

// callsign mults
      const vector<BAND> bands { ( rules.callsign_mults_per_band() ? permitted_bands : vector<BAND> { current_band } ) };  // the bands that have callsign mults

      for (const string& callsign_mult : callsign_mults)
      { string callsign_mult_value;

        SET_CALLSIGN_MULT_VALUE(callsign_mult_value, (callsign_mult == "AAPX"sv) and (location_db.continent(callsign) == "AS"sv), wpx_prefix, callsign);           // All Asian
        SET_CALLSIGN_MULT_VALUE(callsign_mult_value, (callsign_mult == "OCPX"sv) and (location_db.continent(callsign) == "OC"sv), wpx_prefix, callsign);           // Oceania
        SET_CALLSIGN_MULT_VALUE(callsign_mult_value, (callsign_mult == "SACPX"sv), sac_prefix, callsign);                                                          // SAC
        SET_CALLSIGN_MULT_VALUE(callsign_mult_value, (callsign_mult == "UBAPX"sv) and (location_db.canonical_prefix(callsign) == "ON"sv), wpx_prefix, callsign);   // UBA
        SET_CALLSIGN_MULT_VALUE(callsign_mult_value, (callsign_mult == "WPXPX"sv), wpx_prefix, callsign);                                                          // WPX

        if (!callsign_mult_value.empty())
        { line = pad_right(callsign_mult + " ["s + callsign_mult_value + "]"s, FIRST_FIELD_WIDTH);

          for (const BAND b : bands)
            line += pad_left( ( statistics.is_needed_callsign_mult(callsign_mult, callsign_mult_value, b, this_mode) ? BAND_NAME[b] : "-"s ), FIELD_WIDTH);

          win_info < cursor(0, next_y_value-- ) < line;
        }
      }
    }
  }

  win_info.refresh();
}

/*! \brief          Expand a CW message, replacing special characters
    \param  msg     the original message
    \return         <i>msg</i> with special characters replaced by their intended values

    As written, this function is simple but inefficient.

    # maps to octothorpe_str
    @ maps to at_call
    * maps to last_exchange
*/
string expand_cw_message(const string_view msg)
{ string octothorpe_replaced;

  if (contains(msg, '#'))
  { string octothorpe_str { to_string(octothorpe) };

    if (!context.short_serno())
      octothorpe_str = pad_left(octothorpe_str, (octothorpe < 1000 ? 3 : 4), 'T');  // always send at least three characters in a serno, because predictability in exchanges is important

    if (serno_spaces)
    { const string spaces { create_string('^', serno_spaces) };
      const string tmp    { octothorpe_str };

      octothorpe_str.clear();

      for_each(tmp.cbegin(), prev(tmp.cend()), [spaces, &octothorpe_str] (const char c) { octothorpe_str += (c + spaces); } );  // add spaces after all except last character

      octothorpe_str += tmp[tmp.size() - 1];
    }

    if ( (long_t > 0) and (octothorpe < 100) )
    { const int  n_to_find    { (octothorpe < 10 ? 2 : 1) };
      const char char_to_send { t_char(long_t) };               // insert the correct char

      bool found_all { false };
      int  n_found   { 0 };

      for (size_t n { 0 }; !found_all and (n < octothorpe_str.size() - 1); ++n)
      { if ( !found_all and (octothorpe_str[n] == 'T') )
        { octothorpe_str[n] = char_to_send;
          found_all = (++n_found == n_to_find);
        }
      }
    }
    
    octothorpe_replaced = replace(msg, "#"s, octothorpe_str);
  }

  const string at_replaced { replace( (octothorpe_replaced.empty() ? msg : octothorpe_replaced), "@"s, at_call) };

  SAFELOCK(last_exchange);

  const string asterisk_replaced { replace(at_replaced, "*"s, last_exchange) };

  return asterisk_replaced;
}

/// Thread function to simulate keystrokes
void keyboard_test(void)
{ XFlush(keyboard.display_p());

  keyboard.push_key_press('g');
  XFlush(keyboard.display_p());
  sleep_for(1s);

  keyboard.push_key_press('4');
  XFlush(keyboard.display_p());
  sleep_for(2s);

  keyboard.push_key_press('a');
  XFlush(keyboard.display_p());
  sleep_for(4s);

  keyboard.push_key_press('m');
  XFlush(keyboard.display_p());
  sleep_for(8s);

  keyboard.push_key_press('t');
  XFlush(keyboard.display_p());
  sleep_for(1s);
}

/// Thread function to simulate a contest from an extant log
void simulator_thread(string filename, int max_n_qsos)
{ start_of_thread("simulator thread"s);

  tr_log trl(filename);
  string last_frequency;

  const unsigned int n_qso_limit { static_cast<unsigned int>(max_n_qsos ? max_n_qsos : trl.number_of_qsos()) };    // either apply a limit or run them all

  for (unsigned int n { 0 }; n < n_qso_limit; ++n)
  { const tr_record& rec           { trl.read(n) };
    const string     str_frequency { rec.frequency() };

    if (str_frequency != last_frequency)
    { rig.rig_frequency(frequency(str_frequency));

      ost << "QSY to " << frequency(str_frequency).hz() << " Hz" << endl;

      if (static_cast<BAND>(frequency(last_frequency)) != static_cast<BAND>(frequency(str_frequency)))
      { current_band = static_cast<BAND>(frequency(str_frequency));

        const BAND cur_band { current_band };
        const MODE cur_mode { current_mode };

        update_remaining_country_mults_window(statistics, cur_band, cur_mode);
        update_remaining_exchange_mults_windows(/* rules, */ statistics, cur_band, cur_mode);
      }

      last_frequency = str_frequency;
    }

    if (rec.sap_mode())
      enter_sap_mode();
    else
      enter_cq_mode();

    keyboard.push_key_press(rec.call(), 1000);

    ost << "Wkg " << rec.call() << endl;

    keyboard.push_key_press(static_cast<KeySym>(XK_Return));

    sleep_for(1s);

    if (cw_p)
      while (!cw_p->empty())
        sleep_for(500ms);

    sleep_for(1s);

    keyboard.push_key_press(static_cast<KeySym>(XK_Return));

    sleep_for(1s);

    if (cw_p)
      while (!cw_p->empty())
        sleep_for(500ms);

    sleep_for(1s);

    { SAFELOCK(thread_check);

      if (exiting)
      { end_of_thread("simulator thread"s);
        return;
      }
    }
  }
}

/*! \brief                  Possibly add a new callsign mult
    \param  callsign        callsign
    \param  force_known     whether to force the mult as being known

    Supports: AA, OC, SAC, UBA. Updates as necessary the container of
    known callsign mults. Also updates the window that displays the
    known callsign mults.

    Does not add the mult if the mult is unknown, unless <i>force_known</i> is known
    force_known is always CALLSIGN_MULT::NO_FORCE_KNOWN in all known contests
*/
//void update_known_callsign_mults(const string& callsign, const KNOWN_MULT force_known)
void update_known_callsign_mults(const string_view callsign, const KNOWN_MULT force_known)
{ if (callsign.empty())
    return;

// local function to perform the update
  auto perform_update = [force_known] (const string& callsign_mult_name, const string& prefix)    // must be string, not string_view because of [] operator used below
    { if (!prefix.empty())      // because sac_prefix() can return an empty string
      { bool is_known;          // we use the is_known variable because we don't want to perform a window update while holding a lock

        { SAFELOCK(known_callsign_mults);

          is_known = known_callsign_mults.contains(prefix);
        }

        if (!is_known)
        {
          { SAFELOCK(known_callsign_mults);

            if (context.auto_remaining_callsign_mults())
            { if ( acc_callsigns[callsign_mult_name].add(prefix, (force_known == KNOWN_MULT::FORCE_KNOWN) ? context.auto_remaining_callsign_mults_threshold() : 1) )
                known_callsign_mults += prefix;
            }
          }

          update_remaining_callsign_mults_window(statistics, callsign_mult_name, current_band, current_mode);
        }
      }
    };

  if (context.auto_remaining_callsign_mults())
  { const string      continent      { location_db.continent(callsign) };
    const string      country        { location_db.canonical_prefix(callsign) };
    const STRING_SET  callsign_mults { rules.callsign_mults() };           ///< collection of types of mults based on callsign (e.g., "WPXPX")

    if ((continent == "AS"sv) and callsign_mults.contains("AAPX"s))
      perform_update("AAPX"s, wpx_prefix(callsign));

    if ((continent == "OC"sv) and callsign_mults.contains("OCPX"s))
      perform_update("OCPX"s, wpx_prefix(callsign));

    if (callsign_mults.contains("SACPX"s))
      perform_update("SACPX"s, sac_prefix(callsign));

    if ((country == "ON"sv) and callsign_mults.contains("UBAPX"s))
      perform_update("UBAPX"s, wpx_prefix(callsign));
  }
}

/*! \brief                  Possibly add a new country to the known country mults
    \param  callsign        callsign from the country possibly to be added
    \param  force_known     whether to force this mult to be known, regardless of threshold
    \return                 whether the country was added

    Adds only if REMAINING COUNTRY MULTS has been set to AUTO in the configuration file,
    and if the accumulator has reached the threshold
*/
//bool update_known_country_mults(const string& callsign, const KNOWN_MULT force_known)
bool update_known_country_mults(const string_view callsign, const KNOWN_MULT force_known)
{ if (callsign.empty())
    return false;

  bool rv { false };

  if (auto_remaining_country_mults)
  { const string canonical_prefix { location_db.canonical_prefix(callsign) };

    if ( acc_countries.add(canonical_prefix, (force_known == KNOWN_MULT::FORCE_KNOWN) ? context.auto_remaining_country_mults_threshold() : 1) )
      rv = statistics.add_known_country_mult(canonical_prefix, rules);                  // don't add if the rules don't recognise it as a country mult
  }

  return rv;
}

/*!     \brief  Send data to the archive file
*/
void archive_data(void)
{ ofstream                        ofs { context.archive_name() };            // the destination archive
  boost::archive::binary_oarchive ar  { ofs };

  ost << "Starting archive" << endl;

// miscellaneous variables
  alert("Archiving miscellaneous variables"s);

  const BAND cb { current_band };
  const MODE cm { current_mode };

  ar & cb & cm
     & next_qso_number & octothorpe
     & rig.rig_frequency();

// bandmap filter
  alert("Archiving bandmap filter"s);
  ar & BMF;

// bandmaps
  alert("Archiving bandmaps"s);
  ar & bandmaps;

// log
  alert("Archiving log"s);
  ar & logbk;

// rate variables
  alert("Archiving rate information"s);
  ar & rate;

// rules (which includes [possibly-auto] canonical exchange values)
  alert("Archiving rules"s);
  ar & rules;

// QSO history of each call
  alert("Archiving per-call QSO history"s);
  ar & q_history;

// statistics
  alert("Archiving statistics"s);
  ar & statistics;

  ost << "Archive complete" << endl;
}

/*! \brief                      Extract the data from the archive file
    \param  archive_filename    name of the file that contains the archive
*/
//void restore_data(const string& archive_filename)
void restore_data(const string_view archive_filename)
{ if (file_exists(archive_filename))
  { try
    { ifstream                        ifs { string { archive_filename } };            // the source archive; there is no string_view constructor!
      //ifstream                        ifs(archive_filename);            // the source archive
      boost::archive::binary_iarchive ar  { ifs };

// miscellaneous variables
      frequency rig_frequency;
      alert("Restoring miscellaneous variables"s);

      BAND cb;
      MODE cm;

      ar & cb & cm
         & next_qso_number & octothorpe
         & rig_frequency;

      current_band = cb;
      current_mode = cm;

// bandmap filter
      alert("Restoring bandmap filter"s);
      ar & BMF;

// bandmaps
      alert("Restoring bandmaps"s);
      ar & bandmaps;

// log
      alert("Restoring log"s);
      ar & logbk;

// rate variables
      alert("Restoring rate information"s);
      ar & rate;

// rules (which includes [possibly-auto] canonical exchange values)
      alert("Restoring rules"s);
      ar & rules;

// QSO history of each call
      alert("Restoring per-call QSO history"s);
      ar & q_history;

// statistics
      alert("Restoring statistics"s);
      ar & statistics;

      alert("Finished restoring data"s);
      restored_data = true;                             // so that main() knows that we restored from an archive

      rig.rig_frequency(rig_frequency);
    }

    catch (...)                                         // typically we get here because there was no archive from which to restore
    { }
  }
}

/*! \brief          Rescore the entire contest
    \param  rules   the rules for the contest

    Recomputes all the history and statistics, based on the logbook
*/
void rescore(const contest_rules& rules)
{ statistics.clear_info();

  logbook new_logbk;

  rate.clear();

  for (const auto& qso : logbk.as_list())
  { statistics.add_qso(qso, new_logbk, rules);  // need new logbook so that dupes are calculated correctly
    new_logbk += qso;

// redo the historical Q-count and score... this is relatively time-consuming
    rate += { qso.epoch_time(), statistics.points(rules) };
  }
}

/*! \brief  Obtain the current time in HH:MM:SS format
*/
string hhmmss(void)
{ const time_t now { NOW() };

  struct tm       structured_time;
  array<char, 26> buf;                       // buffer to hold the ASCII date/time info; see man page for gmtime()

  gmtime_r(&now, &structured_time);          // convert to UTC in a thread-safe manner
  asctime_r(&structured_time, buf.data());   // convert to ASCII

  return substring <std::string> (string(buf.data(), 26), 11, 8);
}

/*! \brief              Alert the user
    \param  msg         message to display
    \param  show_time   whether to prepend HHMMSS

    Also logs the message (always with the time)
*/
//void alert(const string& msg, const SHOW_TIME show_time)
void alert(const string_view msg, const SHOW_TIME show_time)
{
  { SAFELOCK(alert);
    alert_time = NOW();
  }

  const string now { hhmmss() };

  win_message < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

  if (show_time == SHOW_TIME::SHOW)
    win_message < now < SPACE_STR;

  win_message <= msg;
  ost << "ALERT: " << now << " " << msg << endl;
}

/*! \brief          Logs a rig-related error
    \param  msg     message to display

    Also alerts on the screen if <i>context.display_communication_errors()</i> is <i>true</i>
*/
//void rig_error_alert(const string& msg)
void rig_error_alert(const string_view msg)
{ ost << "Rig error: " << msg << endl;

  if (context.display_communication_errors())
    alert(msg);
}

/// update the QSO and score values in <i>win_rate</i>
void update_rate_window(void)
{ constexpr int RATE_PERIOD_WIDTH { 3 };
  constexpr int QS_WIDTH          { 3 };
  constexpr int SCORE_WIDTH       { 10 };
  
  const vector<unsigned int> rate_periods { context.rate_periods() };    // in minutes

  string rate_str { pad_left(EMPTY_STR, RATE_PERIOD_WIDTH) + pad_left("Qs"s, QS_WIDTH) };

  if (scoring_enabled)
    rate_str += pad_left("Score"s, SCORE_WIDTH);

  if (rate_str.length() != static_cast<unsigned int>(win_rate.width()))    // LF is added automatically if a string fills a line
    rate_str += LF;

  for (const auto& rate_period : rate_periods)
  { string str { pad_right(rate_period, RATE_PERIOD_WIDTH) };

    const auto [n_qsos, points] { rate.calculate_rate(rate_period * 60, context.normalise_rate() ? 3600 : 0) };

    str += pad_left(n_qsos, QS_WIDTH);

    if (scoring_enabled)
      str += pad_left(separated_string(points, TS), SCORE_WIDTH);

    rate_str += (str + (str.length() == static_cast<unsigned int>(win_rate.width()) ? EMPTY_STR : LF) );      // LF is added automatically if a string fills a line
  }

  win_rate < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT < centre("RATE"s, win_rate.height() - 1)
           < WINDOW_ATTRIBUTES::CURSOR_DOWN < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= rate_str;
}

/// Thread function to reset the RBN or cluster connection
void reset_connection(dx_cluster* rbn_p)
{ ost << "Resetting RBN/cluster connection" << endl;    // for now only RESET RBN exists

// no start_of_thread for this one, as it's all asynchronous
  if (rbn_p)
  { rbn_p-> reset_connection();
    ost << "RBN/cluster connection has been reset" << endl;
  }
  else
  { alert("ERROR: reset_connection() passed nullptr");
  }
}

/*! \brief          Populate QSO with correct exchange mults
    \param  qso     QSO to poulate
    \param  rules   rules for this contest
    \return         whether any exchange fields are new mults
*/
bool calculate_exchange_mults(QSO& qso, const contest_rules& rules)
{ const vector<exchange_field> exchange_template { rules.expanded_exch(qso.canonical_prefix(), qso.mode()) };        // exchange_field = name, is_mult
  const vector<received_field> received_exchange { qso.received_exchange() };

  vector<received_field> new_received_exchange;

  bool rv { false };

  for (auto field : received_exchange)                          // use a copy
  { if (field.is_possible_mult())                               // see if it's really a mult
    { if (context.auto_remaining_exchange_mults(field.name()))
        statistics.add_known_exchange_mult(field.name(), field.value());

      const bool is_needed_exchange_mult { statistics.is_needed_exchange_mult(field.name(), field.value(), qso.band(), qso.mode()) };

      field.is_mult(is_needed_exchange_mult);

      if (is_needed_exchange_mult)
        rv = true;
    }

    new_received_exchange += field;  // leave it unchanged
  }

  qso.received_exchange(new_received_exchange);

  return rv;
}

/*! \brief              Rebuild the history (and statistics, rate and greatest_distance if necessary), using the logbook
    \param  logbk       logbook of QSOs
    \param  rules       rules for this contest
    \param  statistics  global statistics
    \param  q_history   history of QSOs
    \param  rate        QSO and point rates
*/
void rebuild_history(const logbook& logbk, const contest_rules& rules,
                     running_statistics& statistics,
                     call_history& q_history,
                     rate_meter& rate)
{
// clear the histories
  statistics.clear_info();
  q_history.clear();
  rate.clear();
  
  const bool using_best_dx { win_best_dx.valid() };

  if (using_best_dx)  
  { greatest_distance = 0;
    win_best_dx < WINDOW_ATTRIBUTES::WINDOW_CLEAR;
  }

  logbook lgb;

  for (const auto& qso : logbk.as_vector())
  { statistics.add_qso(qso, lgb, rules);
    q_history += qso;
    rate += { qso.epoch_time(), statistics.points(rules) };
    
    if (using_best_dx)
      update_best_dx( grid_square { qso.received_exchange("GRID"sv) }, qso.callsign());
    
    lgb += qso;
  }
}

/*! \brief                Copy log and, optionally, QTC files to a backup directory
    \param  dir           destination directory
    \param  log_filename  destination filename of log
    \param  qtc_filename  destination filename of QTC file

    Does not attempt to copy a QTC file if <i>qtc_filename</i> is empty
*/
void auto_backup(const string dir, const string log_filename, const string qtc_filename)
{
  { start_of_thread("auto backup"s);

    try
    { const string dts           { date_time_string(SECONDS::NO_INCLUDE) };
      const string suffix        { dts.substr(0, 13) + '-' + dts.substr(14) }; // replace : with -
      const string complete_name { dir + "/"s + log_filename + "-"s + suffix };

      ofstream(complete_name) << ifstream( string { log_filename } ).rdbuf();          // perform the copy

      if (!qtc_filename.empty())
      { const string qtc_complete_name { dir + "/"s + qtc_filename + "-"s + suffix };

        ofstream(qtc_complete_name) << ifstream( string { qtc_filename } ).rdbuf();  // perform copy of QTC file
      }
    }

    catch (...)
    { ost << "CAUGHT EXCEPTION IN AUTO_BACKUP" << endl;
    }
  }  // ensure that all objects call destructors, whatever the implementation

  end_of_thread("auto backup"s);
}

/// write the current local time to <i>win_local_time</i>
void update_local_time(void)
{ if (win_local_time.wp())                                         // don't do it if we haven't defined this window
  { struct tm       structured_local_time;
    array<char, 26> buf_local_time;

    const time_t now { NOW() };                               // get the time from the kernel

    localtime_r(&now, &structured_local_time);                     // convert to local time
    asctime_r(&structured_local_time, buf_local_time.data());      // and now to ASCII

    win_local_time < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= substring <std::string_view> (string(buf_local_time.data(), 26), 11, 5);  // extract HH:MM and display it
  }
}

/// Increase the counter for the number of running threads
//void start_of_thread(const string& name)
void start_of_thread(const string_view name)
{ ost << "thread [" << name  << "] is starting" << endl;

  SAFELOCK(thread_check);

  n_running_threads++;

  const string name_str { name };

  const auto [ it, success ] { thread_names.insert(name_str) };

  if (!success)
    ost << "failed to insert thread name: " << name << endl;
  else
    thread_map += { my_thread_id(), name_str };
}

/// Cleanup and exit
void exit_drlog(void)
{ ost << "Inside exit_drlog()" << endl;

  const string dts    { date_time_string(SECONDS::NO_INCLUDE) };
  const string suffix { dts.substr(0, 13) + '-' + dts.substr(14) }; // replace : with -

  dump_screen("screenshot-EXIT-"s + suffix);

  if (const auto xruns { audio.xrun_counter() }; xruns)
    ost << "Total number of audio XRUN errors = " << xruns << endl;

  archive_data();

  ost << "finished archiving" << endl;

  { SAFELOCK(thread_check);

    ost << "have the lock" << endl;

    ost << boolalpha << "first value of exiting = " << bool { exiting } << endl;

    exiting_rig_status = true;
    ost << "exiting_rig_status now true; number of threads = " << n_running_threads << endl;
  }

  ost << boolalpha << "second value of exiting = " << bool { exiting } << endl;

  ost << "starting exit tests" << endl;

  for (unsigned int n { 0 }; n < 10; ++n)
  { ost << "running exit test number " << n << endl;

    { SAFELOCK(thread_check);

      if (exiting)        // will be true when rig display thread ends
      { ost << "exiting is true" << endl;

        const int local_copy { n_running_threads };

        ost << "n_running_threads = " << local_copy << endl;

        print_thread_names();

        if (local_copy == 0)
        { ost << "all threads stopped; exiting" << endl;
          sleep_for(1s);                     // just allow some extra time
          exit(0);
        }

      }
      else
        ost << "exiting is not yet true" << endl;
    }

    ost << "after exit test; about to sleep for one second" << endl;

    sleep_for(1s);
  }

  ost << "Exiting even though some threads still appear to be running" << endl;

  exit(0);
}

/*! \brief                  Get best fuzzy or SCP match
    \param  matches         vector of fuzzy or SCP matches, colour coded
    \param  do_not_return   do not return this value, even if the algorithm selects it as the best
    \return                 best of the matches, or the empty string

    The returned value, if any, is the match that is most likely to be the correct call, according to the algorithm in this routine.
    This routine is very conservative. Hence, it should almost always return a value with which
    the operator would agree. In the absence of an obvious candidate for "best match", the
    empty string is returned.
*/
string match_callsign(const vector<pair<string /* callsign */, PAIR_NUMBER_TYPE /* colour pair number */ > >& matches, const string_view do_not_return)
{ string new_callsign { };

  if (matches.size() == 1)
  { const auto& only_match { matches[0] };

    if ( const auto& [ only_call, only_colour_pair_number] { only_match }; colours.fg(only_colour_pair_number) != REJECT_COLOUR )
      if (do_not_return != only_call)
        new_callsign = only_call;
  }

  if (new_callsign.empty())       // if there's more than one acceptable match
  { int n_green { 0 };      // count the number of green entries

    string tmp_callsign;

    for (const auto& [callsign, pair_number] : matches)
    { if (colours.fg(pair_number) == ACCEPT_COLOUR)
      { n_green++;
        tmp_callsign = callsign;
      }
    }

    if (n_green == 1)
      if (do_not_return != tmp_callsign)
        new_callsign = tmp_callsign;
  }

  return new_callsign;
}

/*! \brief              Is a callsign needed on a particular band and mode?
    \param  callsign    target callsign
    \param  b           target band
    \param  m           target mode
    \return             whether we still need to work <i>callsign</i> on <i>b</i> and <i>m</i>
*/
//bool is_needed_qso(const string& callsign, const BAND b, const MODE m)
bool is_needed_qso(const string_view callsign, const BAND b, const MODE m)
{ if (const bool worked_at_all { q_history.worked(callsign) }; !worked_at_all)
    return true;

  if (const bool worked_this_band_mode { q_history.worked(callsign, b, m) }; worked_this_band_mode)
    return false;

// worked on same band, different mode
  if (q_history.worked(callsign, b))
    return rules.work_if_different_mode();

// different band, same mode
  if (q_history.worked(callsign, m))
    return rules.work_if_different_band();

// different band, different mode
  if (q_history.worked_on_another_band_and_mode(callsign, b, m))
    return ( rules.work_if_different_band() and rules.work_if_different_mode() );

// should never get here
  ost << "ERROR in is_needed_qso for " << callsign << ", " << BAND_NAME[b] << ", " << MODE_NAME[m] << endl;

  return false;
}

/*! \brief      Control RIT using the SHIFT keys
    \param  e   keyboard event to process

    RIT changes via hamlib, at least on the K3, are *very* slow
*/
bool shift_control(const keyboard_event& e)
{ const int shift_delta { (current_mode == MODE_CW) ? shift_delta_cw : shift_delta_ssb };    // get the right shift for the mode
  const int change      { (e.symbol() == XK_Shift_L)   ? -shift_delta   : shift_delta };        // are we going up or down in QRG?

  try
  { if (rig.rit_enabled())
    { int last_rit { rig.rit() };

      ok_to_poll_k3 = false;                // stop polling a K3

      do
      { rig.rit(last_rit + change);                 // this takes forever through hamlib
        last_rit += change;

        if (shift_poll)
          sleep_for(milliseconds(shift_poll));
      } while (keyboard.empty());                      // the next event should be a key-release, but anything will do

      ok_to_poll_k3 = true;             // restart polling a K3
    }
    else  // main frequency, not RIT
    { if (active_window == ACTIVE_WINDOW::CALL)         // don't do anything if we aren't in the CALL window
      { frequency last_qrg { rig.rig_frequency() };

        ok_to_poll_k3 = false;                // stop polling a K3

        do
        { const frequency new_qrg { static_cast<double>(last_qrg.hz() + change) };

          rig.rig_frequency(new_qrg);
          last_qrg = new_qrg;

          if (shift_poll)
            sleep_for(milliseconds(shift_poll));
        } while (keyboard.empty());                      // the next event should be a key-release, but anything will do

        ok_to_poll_k3 = true;             // restart polling a K3
      }
    }
  }

  catch (const rig_interface_error& e)
  { alert("Error in rig communication while setting RIT offset"s);
    ok_to_poll_k3 = true;             // restart polling a K3
  }

  return true;
}

/// switch the states of RIT and XIT
bool swap_rit_xit(void)
{ try
  { if (rig.rit_enabled())
    { rig.xit_enable();
      rig.rit_disable();
    }
    else
    { if (rig.xit_enabled())
      { rig.rit_enable();
        rig.xit_disable();
      }
      else
        rig.rit_enable();
    }
  }
  
  catch (const rig_interface_error& e)
  { alert("Invalid rig response in swap_rit_xit(): "s + e.reason());
  }

  return true;
}

/*! \brief          Add a QSO into the all the objects that need to know about it
    \param  qso     the QSO to add
*/
void add_qso(const QSO& qso)
{ statistics.add_qso(qso, logbk, rules);    // add it to the running statistics before we add it to the log so that we can check for dupes against the current log
  logbk += qso;

// add it to the QSO history
  q_history += qso;

// possibly add it to the dynamic SCP database
  if (!scp_db.contains(qso.callsign()) and !scp_dynamic_db.contains(qso.callsign()))
    scp_dynamic_db += qso.callsign();

// and the fuzzy database
  if (!fuzzy_db.contains(qso.callsign()) and !fuzzy_dynamic_db.contains(qso.callsign()))
    fuzzy_dynamic_db += qso.callsign();

// and the query database
  query_db += qso.callsign();

// add to the rates
  rate += { qso.epoch_time(), statistics.points(rules) };
}

/*! \brief              Update the individual_messages window with the message (if any) associated with a call
    \param  callsign    callsign with which the message is associated

    Clears the window if there is no individual message associated with <i>callsign</i>
*/
//void update_individual_messages_window(const string& callsign)
void update_individual_messages_window(const string_view callsign)
{ bool message_written { false };

  if (!callsign.empty())
  { SAFELOCK(individual_messages);

    if ( const string msg { MUM_VALUE(individual_messages, callsign) }; !msg.empty() )
    { win_individual_messages < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= msg;
      message_written = true;
    }
  }

  if (!message_written and !win_individual_messages.empty())
    win_individual_messages < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
}

/*! \brief              Update the batch_messages window with the message (if any) associated with a call
    \param  callsign    callsign with which the message is associated

    Clears the window if there is no batch message associated with <i>callsign</i>. Reverses the
    colours of the window if there is a message, in order to make it stand out.
*/
//void update_batch_messages_window(const string& callsign)
void update_batch_messages_window(const string_view callsign)
{ bool message_written { false };

  if (!callsign.empty())
  { SAFELOCK(batch_messages);       // this is really overkill, as it should be immutable once we're up and running

    if ( const string msg { MUM_VALUE(batch_messages, callsign) }; !msg.empty() )
    { const string spaces { create_string(' ', win_batch_messages.width()) };

      win_batch_messages < WINDOW_ATTRIBUTES::WINDOW_REVERSE < WINDOW_ATTRIBUTES::WINDOW_CLEAR < spaces < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE
                         < msg <= WINDOW_ATTRIBUTES::WINDOW_NORMAL;               // REVERSE < CLEAR does NOT set the entire window to the original fg colour!

      message_written = true;
    }
  }

  if (!message_written and !win_batch_messages.empty())
    win_batch_messages < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
}

/*! \brief                      Obtain value corresponding to a type of callsign mult from a callsign
    \param  callsign_mult_name  the type of the callsign mult
    \param  callsign            the call for which the mult value is required
    \return                     value of <i>callsign_mult_name</i> for <i>callsign</i>

    Returns the empty string if no sensible result can be returned
*/
//string callsign_mult_value(const string& callsign_mult_name, const string& callsign)
//string callsign_mult_value(const string_view callsign_mult_name, const string& callsign)
string callsign_mult_value(const string_view callsign_mult_name, const string_view callsign)
{ if ( (callsign_mult_name == "AAPX"sv) and (location_db.continent(callsign) == "AS"sv) )  // All Asian
    return wpx_prefix(callsign);

  if ( (callsign_mult_name == "OCPX"sv) and (location_db.continent(callsign) == "OC"sv) )  // Oceania
    return wpx_prefix(callsign);

  if (callsign_mult_name == "SACPX"sv)      // SAC
    return sac_prefix(callsign);

  if ( (callsign_mult_name == "UBAPX"sv)  and (location_db.canonical_prefix(callsign) == "ON"sv) )  // UBA
    return wpx_prefix(callsign);

  if (callsign_mult_name == "WPXPX"sv)
    return wpx_prefix(callsign);

  return string { };
}

/*! \brief                      Update several call-related windows
    \param  callsign            call to use as a basis for the updated windows
    \param  display_extract     whether to update the LOG EXTRACT window

    Updates the following windows:
      info (see populate_win_info for other populated windows)
      batch messages
      individual messages
      extract
      QSLs
 */
//void display_call_info(const string& callsign, const bool display_extract)
void display_call_info(const string_view callsign, const bool display_extract)
{ populate_win_info( callsign );
  update_batch_messages_window( callsign );
  update_individual_messages_window( callsign );

  if (display_extract)
  { extract = logbk.worked( callsign );
    extract.display();
  }

  update_qsls_window(callsign);
}

/*! \brief Start a thread to take a snapshot of a P3.

    Even though we use a separate thread to obtain the actual screenshot, it takes so long to transfer the data to the computer
    that one should not use this function except when it will be OK for communication with the rig to be paused for ~30 seconds.

    The user should check that a P3 is available (context.p3()) before calling this function.
*/
bool p3_screenshot(void)
{ jthread(p3_screenshot_thread).detach();

  return true;
}

/// Thread function to generate a screenshot of a P3 and store it in a BMP file
// #BMP (Bitmap upload, GET only)
//RSP format: [bmp]cc where [bmp] is 131,638 bytes of binary image data in standard .BMP file format
//and cc is a two-byte checksum. Note that the response does not include the command name and has no
//terminating semicolon. The checksum is the modulo-65,536 sum of all 131,638 bytes, sent least significant
//byte first.

/*

 */
void p3_screenshot_thread(void)
{ alert("Dumping P3 image"s);

  const string image { rig.raw_command("#BMP;"s, RESPONSE::EXPECTED) };

//  write_file(image, "complete-response");

//  const string checksum_str { image.substr(image.length() - 2, 2) };
  const string checksum_str { substring <string> (image, image.length() - 2, 2) };

//  ost << "image length with checksum = " << image.length() << endl;
//  ost << "image length without checksum = " << image.length() - 2 << endl;

//  const unsigned char c0 = static_cast<unsigned char>(checksum_str[0]);
//  const unsigned char c1 = static_cast<unsigned char>(checksum_str[1]);

//  ost << "chars as numbers: " << dec << static_cast<unsigned int>(static_cast<uint8_t>(c0)) << " " << static_cast<unsigned int>(static_cast<uint8_t>(c1)) << endl;

//  ost << "chars: " << hex << static_cast<uint8_t>(c0) << " " << static_cast<uint8_t>(c1) << dec << endl;

// print most significant byte first
//  const uint16_t received_checksum = (static_cast<uint8_t>(checksum_str[1]) << 8) + static_cast<uint8_t>(checksum_str[0]);
//  ost << "received checksum = " << hex << received_checksum << dec << endl;

//  ost << "components = " << hex << (static_cast<uint8_t>(checksum_str[1]) << 8) << " " << static_cast<uint8_t>(checksum_str[0]) << endl;

  const char c1 { checksum_str[1] };
  const char c0 { checksum_str[0] };

//  const uint16_t ui1 { static_cast<uint16_t>(c1 << 8) };
//  const uint16_t ui0 { static_cast<uint16_t>(c0) };

//  uint16_t ui1 { static_cast<unsigned char>(checksum_str[1]) };
//  uint16_t ui0 { checksum_str[0] };

//  ui1 <<= 8;

// the messy casts are needed because bitor by definition converts parameters to int
  const int      ui1               { static_cast<int>(c1 << 8) };
  const int      ui0               { static_cast<int>(c0) };
  const uint16_t received_checksum { static_cast<uint16_t>(ui1 bitor ui0) };

//  const uint16_t received_checksum { (static_cast<uint16_t>(checksum_str[1]) << 8) bitor static_cast<uint16_t>(checksum_str[0]) };


//  ost << "received checksum bitor = " << hex << received_checksum_bitor << dec << endl;

  uint16_t calculated_checksum { 0 };
  long     tmp                 { 0 };

  for (size_t n = 0; n < image.length() - 2; ++n)
    tmp += static_cast<int>(static_cast<unsigned char>(image[n]));

  calculated_checksum = static_cast<uint16_t>(tmp % 65536);
//  ost << "tmp: " << tmp << " " << hex << tmp << " " << dec << tmp % 65536 << " " << hex << tmp % 65536 << dec << endl;

#if 0
  uint16_t received_checksum { 0 };

  for (size_t n = 0; n < checksum_str.length(); ++n)
  { const size_t        index { 2 - n - 1 };
    const unsigned char uch   { static_cast<unsigned char>(checksum_str[index]) };
    const uint16_t      uint  { static_cast<uint16_t>(uch) };

    ost << n << ": " << hex << uint << endl;

    received_checksum = (received_checksum << 8) + uint;  // 256 == << 8
  }
#endif

//  ost << "calculated checksum = " << hex << calculated_checksum << dec << endl;
//  ost << "received checksum = " << hex << received_checksum << dec << endl;

  const string base_filename { context.p3_snapshot_file() + (((calculated_checksum == received_checksum) or context.p3_ignore_checksum_error()) ? ""s : "-error"s) };

  int  index        { 0 };
  bool file_written { false };

  while (!file_written)
  { if (const string filename { base_filename + "-"s + to_string(index) }; !file_exists(filename))
    { //write_file(image.substr(0, image.length() - 2), filename);
      write_file(substring <string> (image, 0, image.length() - 2), filename);
      file_written = true;

      alert("P3 image file "s + filename + " written"s);
    }
    else
      ++index;
  }
}

/// Thread function to spawn the cluster
void spawn_dx_cluster(void)
{ win_cluster_line <= "UNCONNECTED"s;

  bool cluster_started   { false };
  bool signalled_failure { false };

  cluster_p = nullptr;          // should be unnecessary

  while (!cluster_started)
  { ost << "Attempting to create cluster connection" << endl;

    try
    { cluster_p = new dx_cluster(context, POSTING_SOURCE::CLUSTER);

      ost << "Cluster connection: " << cluster_p->connection_status() << endl;
      cluster_started = true;
    }

    catch (...)
    { ost << "UNABLE TO CREATE CLUSTER" << endl;
      if (!signalled_failure)
      { alert("UNABLE TO CREATE CLUSTER; PROCEEDING WITHOUT CLUSTER"s);
        signalled_failure = true;
      }

      if (cluster_p)
      { delete cluster_p;
        cluster_p = nullptr;
      }

      sleep_for(1min);
    }
  }

  ost << "cluster connection appears to have been created" << endl;

  win_cluster_line < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= "CONNECTED"s;

  jthread(get_cluster_info, cluster_p).detach();
  jthread(process_rbn_info, &win_cluster_line, &win_cluster_mult, cluster_p, &statistics, &location_db, &win_bandmap, &bandmaps).detach();
}

/// Thread function to spawn the RBN
void spawn_rbn(void)
{ win_rbn_line <= "UNCONNECTED"s;

  bool rbn_started       { false };
  bool signalled_failure { false };

  rbn_p = nullptr;          // should be unnecessary

  while (!rbn_started)
  { try
    { rbn_p = new dx_cluster(context, POSTING_SOURCE::RBN);

      ost << "RBN connection: " << rbn_p->connection_status() << endl;
      rbn_started = true;
    }

    catch (const x_error& e)
    { ost << "UNABLE TO CREATE RBN: error = " << e.reason() << endl;
      if (!signalled_failure)
      { alert("UNABLE TO CREATE RBN; PROCEEDING WITHOUT RBN"s);
        signalled_failure = true;
      }

      if (rbn_p)
      { delete rbn_p;
        rbn_p = nullptr;
      }

      sleep_for(1min);
    }

    catch (...)
    { ost << "UNABLE TO CREATE RBN" << endl;

      if (!signalled_failure)
      { alert("UNABLE TO CREATE RBN; PROCEEDING WITHOUT RBN"s);
        signalled_failure = true;
      }

      if (rbn_p)
      { delete rbn_p;
        rbn_p = nullptr;
      }

      sleep_for(1min);
    }
  }

  win_rbn_line < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= "CONNECTED"s;

  if ( const string rbn_filename { context.rbn_file() }; !rbn_filename.empty() )
    rbn_file.open(rbn_filename, std::ofstream::app);

  jthread(get_cluster_info, rbn_p).detach();
  jthread(process_rbn_info, &win_rbn_line, &win_cluster_mult, rbn_p, &statistics, &location_db, &win_bandmap, &bandmaps).detach();
}

/*! \brief  Dump useful information to disk

    Performs a screenshot dump, and then dumps useful information
    to the debug file.
*/
bool debug_dump(void)
{ ost << "*** DEBUG DUMP ***" << endl;
  ost << "Screenshot dumped to: " << dump_screen() << endl;

  int index { 0 };

  for (auto& bm : bandmaps)
  { ost << "dumping bandmap # " << index++ << endl;

    const string str { bm.to_str() };

    ost << str;
  }

  alert("DEBUG info written"s);

  return true;
}

/*! \brief                  Dump a screen image to BMP file
    \param  dump_filename   name of the destination file
    \return                 name of the file actually written

    If <i>dump_filename</i> is empty, then a base name is taken
    from the context and a string "-<n>" is appended.
*/
//string dump_screen(const string& dump_filename)
string dump_screen(const string_view dump_filename)
{ const bool multithreaded { keyboard.x_multithreaded() };

  Display* display_p { keyboard.display_p() };

  const Window window_id { keyboard.window_id() };

  XWindowAttributes win_attr;

  if (multithreaded)
    XLockDisplay(display_p);

  const Status status { XGetWindowAttributes(display_p, window_id, &win_attr) };

  if (multithreaded)
    XUnlockDisplay(display_p);

  if (status == 0)
  { ost << hhmmss() << ": ERROR returned by XGetWindowAttributes(): " << status << endl;
    alert("ERROR from XGetWindowAttributes()");
    return "ERROR"s;
  }

  const int width  { win_attr.width };
  const int height { win_attr.height };

  if (multithreaded)
    XLockDisplay(display_p);

  int int_x_y { 0 };

  XImage* xim_p { XGetImage(display_p, window_id, int_x_y, int_x_y, width, height, AllPlanes, ZPixmap) };
  
  if (multithreaded)
    XUnlockDisplay(display_p);

  if (xim_p == NULL)
  { ost << "NULL returned from XGetImage(); screen not written to file" << endl;
    alert("Internal error: screen not dumped to file");

    return string();
  }

#if 0
 typedef struct XImage XImage;
 struct XImage {
   int width, height;		/* size of image */
   int xoffset;			/* number of pixels offset in X direction */
   int format;			/* XYBitmap, XYPixmap, ZPixmap */
   char *data;			/* pointer to image data */
   int byte_order;		/* data byte order, LSBFirst, MSBFirst */
   int bitmap_unit;		/* quant. of scanline 8, 16, 32 */
   int bitmap_bit_order;		/* LSBFirst, MSBFirst */
   int bitmap_pad;		/* 8, 16, 32 either XY or ZPixmap */
   int depth;			/* depth of image */
   int bytes_per_line;		/* accelerator to next scanline */
   int bits_per_pixel;		/* bits per pixel (ZPixmap) */
   unsigned long red_mask;	/* bits in z arrangement */
   unsigned long green_mask;
   unsigned long blue_mask;
   XPointer obdata;		/* hook for the object routines to hang on */
   struct funcs {		/* image manipulation routines */
     XImage *(*create_image)();
     int (*destroy_image)();
     unsigned long (*get_pixel)();
     int (*put_pixel)();
     XImage *(*sub_image)();
     int (*add_pixel)();
   } f;
 }; 
#endif
/*
  ost << "XImage: " << endl
      << "  width = " << xim_p -> width << endl
      << "  height = " << xim_p -> height << endl
      << "  xoffset = " << xim_p -> xoffset << endl
      << "  format = " << xim_p -> format << endl
      << "  byte order = " << xim_p -> byte_order << endl
      << "  bitmap unit = " << xim_p -> bitmap_unit << endl
      << "  bitmap bit order = " << xim_p -> bitmap_bit_order << endl
      << "  bitmap pad = " << xim_p -> bitmap_pad
      << endl;
*/

  png::image< png::rgb_pixel > image(width, height);

  constexpr unsigned int FF         { 0xff };
  constexpr unsigned int BLUE_MASK  { FF };
  constexpr unsigned int GREEN_MASK { FF << 8 };
  constexpr unsigned int RED_MASK   { FF << 16 };

  for (size_t y { 0 }; y < image.get_height(); ++y)
  { for (size_t x { 0 }; x < image.get_width(); ++x)
    { const unsigned long pixel { XGetPixel (xim_p, x, y) };
      const unsigned char blue  { static_cast<unsigned char>(pixel bitand BLUE_MASK) };
      const unsigned char green { static_cast<unsigned char>((pixel bitand GREEN_MASK) >> 8)};
      const unsigned char red   { static_cast<unsigned char>((pixel bitand RED_MASK) >> 16) };

      image[y][x] = png::rgb_pixel(red, green, blue);
    }
  }

  XDestroyImage(xim_p);

  string filename;

  if (dump_filename.empty())
  { const string base_filename { context.screen_snapshot_file() };

    int index { 0 };

    filename = base_filename + "-"s + to_string(index++);

    while (file_exists(filename))
      filename = base_filename + "-"s + to_string(index++);
  }
  else
    filename = dump_filename;

  image.write(filename);

  alert("screenshot file "s + filename + " written"s);

  return filename;
}

/// add info to a QSO if callsign mults are in use; may change <i>qso</i>
void allow_for_callsign_mults(QSO& qso)
{ if (callsign_mults_used)
  { string mult_name;

    if ( rules.callsign_mults().contains("AAPX"s) and (location_db.continent(qso.callsign()) == "AS"sv) )  // All Asian
    { qso.prefix(wpx_prefix(qso.callsign()));
      mult_name = "AAPX"s;
    }

    if ( rules.callsign_mults().contains("OCPX"s) and (location_db.continent(qso.callsign()) == "OC"sv) )  // Oceania
    { qso.prefix(wpx_prefix(qso.callsign()));
      mult_name = "OCPX"s;
    }

    if ( rules.callsign_mults().contains("SACPX"s) )      // SAC
    { qso.prefix(sac_prefix(qso.callsign()));
      mult_name = "SACPX"s;
    }

    if ( rules.callsign_mults().contains("UBAPX"s) and (location_db.canonical_prefix(qso.callsign()) == "ON"sv) )  // UBA
    { qso.prefix(wpx_prefix(qso.callsign()));
      mult_name = "UBAPX"s;
    }

    if (rules.callsign_mults().contains("WPXPX"s))
    { qso.prefix(wpx_prefix(qso.callsign()));
      mult_name = "WPXPX"s;
    }

// see if it's a mult... requires checking whether mults are per-band
    if (!qso.prefix().empty() and !mult_name.empty())
    { if (rules.callsign_mults_per_band())
      { if (statistics.is_needed_callsign_mult(mult_name, qso.prefix(), qso.band(), qso.mode()))
          qso.is_prefix_mult(true);
      }
      else
      { if (statistics.is_needed_callsign_mult(mult_name, qso.prefix(), static_cast<BAND>(ALL_BANDS), qso.mode()) )
          qso.is_prefix_mult(true);
      }
    }
  }
}

/*! \brief          Add info to a QSO if callsign mults are in use
    \param  qso     input QSO
    \return         <i>qso</i> modified if necessary so as to include info about callsign multts 
*/
QSO allow_for_callsign_mults(QSO&& qso)
{ QSO rv { move(qso) };

  allow_for_callsign_mults(rv);

  return rv;
}

/*! \brief      Function to process input to the QTC window
    \param  wp  pointer to window
    \param  e   keyboard event to process

   ALT-K --  toggle CW
   ALT-Q --  start process of sending QTC batch
   ALT-Y --  mark most-recently sent QTC as unsent
   ESCAPE -- abort CW
   R --      repeat introduction (i.e., no QTCs sent)
   ENTER --  send next QSO or finish
   CTRL-P -- dump screen
   CTRL-X, ALT-X -- Abort and go back to prior window

   T, U -- repeat time
   C -- repeat call
   N, S -- repeat number
   A, R -- repeat all

   PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
*/
void process_QTC_input(window* wp, const keyboard_event& e)
{ constexpr int MAX_QTC_ENTRIES_PER_STN { 10 };

  static unsigned int original_cw_speed;
  static string       qtc_id;
  static unsigned int qtcs_sent;
  static qtc_series   series;
  static unsigned int total_qtcs_to_send;

  static const string EU { "EU"s };

  const unsigned int qtc_qrs { context.qtc_qrs() };
  const bool         cw      { current_mode == MODE_CW };  // just to keep it easy to determine if we are on CW

  auto send_msg = [cw] (const string& msg)
    { if (cw)
        (*cw_p) << msg;  // don't use cw_speed because that executes asynchronously, so the speed will be back to full speed before the message is sent
    };

  window& win { *wp };   // syntactic sugar

// ALT-Q - start process of sending QTC batch
  if (!sending_qtc_series and e.is_alt('q'))
  { ost << "processing ALT-Q to send QTC" << endl;

// destination for the QTC is the callsign in the CALL window; or, if the window is empty, the call of the last logged EU QSO
    const string call_window_contents { remove_peripheral_spaces <std::string> (win_call.read()) };

    string destination_callsign { call_window_contents };

    if (destination_callsign.empty())
      destination_callsign = logbk.last_qso().callsign();

    if (!destination_callsign.empty() and (location_db.continent(destination_callsign) != EU) )  // got a call, but it's not EU
      destination_callsign = logbk.last_worked_eu_call();

    if (destination_callsign.empty())       // we have no destination
    { alert("No valid destination for QTC"s);
      set_active_window(ACTIVE_WINDOW::CALL);

      return;
    }

    if (location_db.continent(destination_callsign) != EU)    // send QTCs only to EU stations
    { alert("No EU destination for QTC"s);
      set_active_window(ACTIVE_WINDOW::CALL);

      return;
    }

// check that it's OK to send a QTC to this call
    const unsigned int n_already_sent { qtc_db.n_qtcs_sent_to(destination_callsign) };

    ost << "n already sent to " << destination_callsign << " = " << n_already_sent << endl;

    if (n_already_sent >= MAX_QTC_ENTRIES_PER_STN)
    { alert(to_string(MAX_QTC_ENTRIES_PER_STN) + " QSOs already sent to "s + destination_callsign);
      set_active_window(ACTIVE_WINDOW::CALL);

      return;
    }

// check that we have at least one QTC that can be sent to this call
    const unsigned int      n_to_send           { MAX_QTC_ENTRIES_PER_STN - n_already_sent };
    const vector<qtc_entry> qtc_entries_to_send { qtc_buf.get_next_unsent_qtc(n_to_send, destination_callsign) };

    ost << "n to be sent to " << destination_callsign << " = " << qtc_entries_to_send.size() << endl;

    if (/* !processed and */ qtc_entries_to_send.empty())
    { alert("No QSOs available to send to "s + destination_callsign);
      set_active_window(ACTIVE_WINDOW::CALL);

      return;
    }

    { const string mode_str { (current_mode == MODE_CW ? "CW"s : "PH"s) };

      series = qtc_series(qtc_entries_to_send, mode_str, context.my_call());

// populate info in the series
      series.destination(destination_callsign);

// check that we still have entries (this should always pass)
      if (series.empty())
      { alert("Error: empty QTC object for "s + destination_callsign);
        set_active_window(ACTIVE_WINDOW::CALL);

        return;
      }
      else                                      // OK; we're going to send at least one QTC
      { sending_qtc_series = true;

        if (cw)
          original_cw_speed = cw_p->speed();

        const unsigned int number_of_qtc { static_cast<unsigned int>(qtc_db.size() + 1) };

        qtc_id = to_string(number_of_qtc) + "/"s + to_string(qtc_entries_to_send.size());
        series.id(qtc_id);

        if (cw)
          send_msg( (cw_p->empty() ? "QTC "s : " QTC "s) + qtc_id + " QRV?"s);

        win_qtc_status < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < "Sending QTC "s < qtc_id < " to "s <= destination_callsign;
        ost << "Sending QTC batch " << qtc_id << " to " << destination_callsign << endl;

// display the QTC entries; we use the "log extract" window
        win <= series;

        total_qtcs_to_send = qtc_entries_to_send.size();
        qtcs_sent = 0;

        if (cw and qtc_qrs)
          cw_speed(original_cw_speed - qtc_qrs);

        return;
      }
    }
  }

// ESCAPE - abort CW
  if (e.symbol() == XK_Escape)
  {
// abort sending CW if we are currently sending
    if (cw)
    { if (!cw_p->empty())
        cw_p->abort();

      cw_speed(original_cw_speed);
    }

    return;
  }

// R -- repeat introduction (i.e., no QTCs sent)
  if ( (qtcs_sent == 0) and e.is_char('r') )
  { if (cw)
      send_msg("QTC "s + qtc_id + " QRV?"s);

    return;
  }

// ENTER - send next QSO or finish
  if (e.is_unmodified() and (e.symbol() == XK_Return))
  { if (qtcs_sent != total_qtcs_to_send)
    { if (cw)
        send_qtc_entry(series.entry(qtcs_sent), true);

// before marking this as sent, record the last acknowledged QTC
      if (qtcs_sent != 0)
        qtc_buf.unsent_to_sent(series.entry(qtcs_sent - 1));

      series.mark_as_sent(qtcs_sent++);
      win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::WINDOW_TOP_LEFT <= series;

      return;
    }
    else    // we have sent the last QTC; cleanup
    { if (cw)
      { cw_speed(original_cw_speed);    // always set the speed, just to be safe

        if (drlog_mode == DRLOG_MODE::CQ)                                   // send QSL immediately
          (*cw_p) << expand_cw_message( context.qsl_message() );
      }

      qtc_buf.unsent_to_sent(series.entry(series.size() - 1));

      win_qtc_status < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < "Sent QTC "s < qtc_id < " to "s <= series.destination();
      ost << "Sent QTC batch " << qtc_id << " to " << series.destination() << endl;

      series.date(substring <std::string> (date_time_string(SECONDS::NO_INCLUDE), 0, 10));
      series.utc(hhmmss());
      series.frequency_str(rig.rig_frequency());

      qtc_db += series;                  // add to database of sent QTCs

      (*win_active_p) < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::WINDOW_NORMAL <= COLOURS(log_extract_fg, log_extract_bg);

// log the QTC series
      append_to_file(context.qtc_filename(), series.complete_output_string());

      set_active_window(last_active_window);

// update statistics, summary and QTC queue window
      statistics.qtc_qsos_sent(qtc_buf.n_sent_qsos());
      statistics.qtc_qsos_unsent(qtc_buf.n_unsent_qsos());
      display_statistics(statistics.summary_string(rules));
      update_qtc_queue_window();

      return;
    }
  }

// CTRL-X, ALT-X -- Abort and go back to prior window
  if ( e.is_control('x') or e.is_alt('x') )
  { if (series.n_sent() != 0)
    { qtc_buf.unsent_to_sent(series[series.size() - 1].first);

      win_qtc_status < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < "Aborted sending QTC "s < qtc_id < " to "s <= series.destination();

      series.date(substring <std::string> (date_time_string(SECONDS::NO_INCLUDE), 0, 10));
      series.utc(hhmmss());
      series.frequency_str(rig.rig_frequency());

      qtc_db += series;                  // add to database of sent QTCs

      (*win_active_p) < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::WINDOW_NORMAL <= COLOURS(log_extract_fg, log_extract_bg);

      append_to_file(context.qtc_filename(), series.complete_output_string());
      set_active_window(last_active_window);

// update statistics and summary window
      statistics.qtc_qsos_sent(qtc_buf.n_sent_qsos());
      statistics.qtc_qsos_unsent(qtc_buf.n_unsent_qsos());
      display_statistics(statistics.summary_string(rules));
    }
    else  // none sent
    { win_qtc_status < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < "Completely aborted; QTC "s < qtc_id < " not sent to "s <= series.destination();

      (*win_active_p) < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::WINDOW_NORMAL <= COLOURS(log_extract_fg, log_extract_bg);

      set_active_window(last_active_window);
      display_statistics(statistics.summary_string(rules));
    }

    if (cw)
    { cw_p->abort();                  // stop sending any CW
      cw_speed(original_cw_speed);    // always set the speed, just to be safe
    }

    return;
  }

// ALT-Y -- mark most-recently sent QTC as unsent
  if (e.is_alt('y'))
  { if (qtcs_sent != 0)
    { series.mark_as_unsent(qtcs_sent--);
      win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::WINDOW_TOP_LEFT <= series;
    }

    return;
  }

  auto valid_qtc_nr = [] (const unsigned int qtcs_sent) { const int qtc_nr { static_cast<int>(qtcs_sent) - 1 };

                                                          return ( ( (qtc_nr >= 0) and (qtc_nr < static_cast<int>(series.size())) ) ? optional<int>(qtc_nr) : nullopt );
                                                        };

// T, U -- repeat time
  if ( e.is_char('t') or e.is_char('u') )
  { if (cw)
    { if (const optional<int> vqn { valid_qtc_nr(qtcs_sent) }; vqn)
        send_msg(series[vqn.value()].first.utc());
    }

    return;
  }

// C -- repeat call
  if ( e.is_char('c') )
  { if (cw)
    { if (const optional<int> vqn { valid_qtc_nr(qtcs_sent) }; vqn)
        send_msg(series[vqn.value()].first.callsign());
    }

    return;
  }

// N, S -- repeat number
  if ( e.is_char('n') or e.is_char('s') )
  { if (cw)
    { if (const optional<int> vqn { valid_qtc_nr(qtcs_sent) }; vqn)
      { const string serno { pad_left(remove_leading <std::string> (remove_peripheral_spaces <std::string> (series[vqn.value()].first.serno()), '0'), 3, 'T') };

        send_msg(serno);
      }
    }

    return;
  }

// A, R -- repeat all
  if ( e.is_char('a') or e.is_char('r') )
  { if (cw)
    { if (const optional<int> vqn { valid_qtc_nr(qtcs_sent) }; vqn)
      { const qtc_entry& qe { series[vqn.value()].first };

        send_qtc_entry(qe, false);
      }
    }

    return;
  }

// PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
  if ( (e.symbol() == XK_Next) or (e.symbol() == XK_Prior) )
    return static_cast<void>(change_cw_speed(e));

// ALT-K -- toggle CW
  if (cw and e.is_alt('k'))
    return static_cast<void>(toggle_cw());

// CTRL-P -- dump screen
  if (e.is_control('p'))
    return static_cast<void>(dump_screen());  // dump_screen returns a string
}

/*! \brief              Set speed of computer keyer
    \param  new_speed   speed to assign to the keyer

    Also sets key of rig keyer if SYNC KEYER was true in the configuration file
*/
void cw_speed(const unsigned int new_speed)
{ if (cw_p)
  { cw_p->speed(new_speed);
    win_wpm < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= (to_string(new_speed) + " WPM"s);

    try
    { if (context.sync_keyer())
        rig.keyer_speed(new_speed);
    }

    catch (const rig_interface_error& e)
    { alert("Error setting CW speed on rig"s);
    }
  }
}

/// return the name of the active window in printable form
string active_window_name(void)
{ switch (active_window)
  { case ACTIVE_WINDOW::CALL :
      return "CALL"s;

    case ACTIVE_WINDOW::EXCHANGE :
      return "EXCHANGE"s;

    case ACTIVE_WINDOW::LOG :
      return "LOG"s;

    case ACTIVE_WINDOW::LOG_EXTRACT :
      return "LOG EXTRACT"s;
  }

  return "UNKNOWN"s;        // keep the silly compiler happy
}

/*! \brief              Display a callsign in the NEARBY window, in the correct colour
    \param  callsign    call to display

    If the CALL window is empty, displays:
      log extract data if context.nearby_extract() is true
      QSL information 
*/
//void display_nearby_callsign(const string& callsign)
void display_nearby_callsign(const string_view callsign)
{ if (callsign.empty())
  { win_nearby <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;

    if (win_call.empty() and context.nearby_extract())
      win_log_extract <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
  }
  else
  { const bool dupe       { logbk.is_dupe(callsign, current_band, current_mode, rules) };
    const bool worked     { q_history.worked(callsign, current_band, current_mode) };
    const int  foreground { win_nearby.fg() };                      // save the default colours
    const int  background { win_nearby.bg() };

// in what colour should we display this call?
    PAIR_NUMBER_TYPE colour_pair_number { colours.add(foreground, background) };

    if (!worked)
      colour_pair_number = colours.add(ACCEPT_COLOUR,  background);

    if (dupe)
      colour_pair_number = colours.add(REJECT_COLOUR,  background);

    win_nearby < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
    win_nearby.set_colour_pair(colour_pair_number);
    win_nearby < callsign <= COLOURS(foreground, background);

    if (win_call.empty())
    { if (context.nearby_extract())               // possibly display the callsign in the LOG EXTRACT window
      { extract = logbk.worked(callsign);
        extract.display();
      }

// display QSL information if the CALL window is empty
      update_qsls_window(callsign);
    }
  }
}

/*! \brief                  Debug exchange templates
    \param  rules           rules for the contest
    \param  test_filename   name of file to test

    Prints out which lines in <i>test_filename</i> match the exchange templates defined by <i>rules</i>
*/
//void test_exchange_templates(const contest_rules& rules, const string& test_filename)
void test_exchange_templates(const contest_rules& rules, const string_view test_filename)
{ ost << "executing -test-exchanges" << endl;

  const STRING_SET field_names { rules.all_known_field_names() };

  ost << "reading file: " << test_filename << endl;

  try
  { const vector<string> targets { to_lines <std::string> (read_file(test_filename)) };

    ost << "contents: " << endl;

    for (const auto& target : targets)
      ost << "  " << target << endl;

    for (const auto& target : targets)
    { vector<string> matches;

      for (const auto& field_name : field_names)
        if (const EFT exchange_field_eft { field_name }; exchange_field_eft.is_legal_value(target))
          matches += field_name;

      ost << "matches for " << target << ": " << endl;

      for (const auto& match : matches)
        ost << "  " << match << endl;
    }
  }

  catch (const string_function_error& e)
  { ost << "Error: unable to read file: " << test_filename << endl;
  }

  exit(0);
}

/// calculate the time/QSO value of a mult and update <i>win_mult_value</i>
void update_mult_value(void)
{ const float        mult_value    { statistics.mult_to_qso_value(rules, current_band, current_mode) };
  const unsigned int mult_value_10 { static_cast<unsigned int>( (mult_value * 10) + 0.5) };
  const string       term_1        { to_string(mult_value_10 / 10) };
  const string       term_2        { substring <std::string> (to_string(mult_value_10 - (10 * (mult_value_10 / 10) )), 0, 1) };

  string msg { "M ≡ "s + term_1 + DP + term_2 + "Q"s };

  const pair<unsigned int, unsigned int> qs            { rate.calculate_rate(900 /* seconds */, 3600) };  // rate per hour
  const unsigned int                     qs_per_hour   { qs.first };
  const float                            mins_per_q    { (qs_per_hour ? 60.0f / static_cast<float>(qs_per_hour) : 3600.0f) };
  const float                            mins_per_mult { mins_per_q * mult_value };

  string mins { "∞"s };                                     // mins is the number of minutes per QSO

  if (mins_per_mult < 60)
  { const unsigned int mins_value_10 { static_cast<unsigned int>( (mins_per_mult * 10) + 0.5) };
    const string       term_1_m      { to_string(mins_value_10 / 10) };
    const string       term_2_m      { substring <std::string> (to_string(mins_value_10 - (10 * (mins_value_10 / 10) )), 0, 1) };

    mins = term_1_m + DP + term_2_m;
  }

  msg += " ≡ "s + mins + "′"s;

  try
  { win_mult_value < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= centre(msg, 0);
  }

  catch (const string_function_error& e)
  { alert(e.reason());
  }
}

/*! \brief            Thread function to write a screenshot to a file
    \param  filename  destination filename
*/
void auto_screenshot(const string filename)
{
  { start_of_thread("auto screenshot"s);

    try
    { ost << hhmmss() << " calling dump_screen() with filename = " << filename << endl;

      dump_screen(filename);
      ost << hhmmss() << " finished dump_screen() with filename = " << filename << endl;
    }

    catch (...)
    { ost << "CAUGHT EXCEPTION IN AUTO_SCREENSHOT" << endl;
    }
  }  // ensure that all objects call destructors, whatever the implementation

  end_of_thread("auto screenshot"s);
}

/*! \brief                  Display the current statistics
    \param  summary_str     summary string from the global running_statistics object
*/
//void display_statistics(const string& summary_str)
void display_statistics(const string_view summary_str)
{ static const STRING_SET MODE_STRINGS { "CW"s, "SSB"s, "All"s };

// write the string, but don't refresh the window
  win_summary < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT < summary_str;

  if (rules.permitted_modes().size() > 1)
  { for (unsigned int n { 0 }; n < static_cast<unsigned int>(win_summary.height()); ++n)
    {
// we have to be a bit complicated because we need to have spaces after the string, so that the colours for the entire line are handled correctly
      if (const string line { remove_peripheral_spaces <std::string> (win_summary.getline(n)) }; MODE_STRINGS.contains(line))
        win_summary < cursor(0, n) < WINDOW_ATTRIBUTES::WINDOW_REVERSE <  centred_string(line, win_summary.width()) < WINDOW_ATTRIBUTES::WINDOW_NORMAL;
    }
  }

  win_summary.refresh();        // now OK to refresh
}

/*! \brief              Set the span of a P3
    \param  khz_span    span in kHz
*/
void p3_span(const unsigned int khz_span)
{ if (context.p3())
  { if ( (khz_span >= 2) and (khz_span <= 200) )
    { const string span_str { pad_leftz((khz_span * 10), 6) };

      rig.raw_command("#SPN"s + span_str + ";"s);
    }
  }
}

/// set CW bandwidth to appropriate value for CQ/SAP mode
bool fast_cw_bandwidth(void)
{ if (current_mode == MODE_CW)
  { const DRLOG_MODE current_drlog_mode { SAFELOCK_GET(drlog_mode_mutex, drlog_mode) };

    rig.bandwidth( (current_drlog_mode == DRLOG_MODE::CQ) ? context.fast_cq_bandwidth() : context.fast_sap_bandwidth() );
  }

  return true;
}

/*! \brief          Process a change in the offset of the bandmaps
    \param  symbol  symbol corresponding to key that was pressed
    \return         <i>true</i>

    Performs an immediate update on the screen
*/
bool process_change_in_bandmap_column_offset(const KeySym symbol)
{ bandmap& bm { bandmaps[current_band] };

  const bool is_increment { ( (symbol == XK_KP_6) or (symbol == XK_KP_Right) ) };

  bool should_increment { false };                                     // value doesn't actually matter

  if (is_increment)
  {
// don't let it increment if there is space in the last column
    const unsigned int number_of_columns                     { bm.n_columns(win_bandmap) };
    const unsigned int maximum_number_of_displayable_entries { number_of_columns * win_bandmap.height() };
    const unsigned int n_entries_in_bandmap                  { static_cast<unsigned int>(bm.displayed_entries().size()) };
    const unsigned int start_entry                           { static_cast<unsigned int>( (n_entries_in_bandmap > maximum_number_of_displayable_entries) ? bm.column_offset() * win_bandmap.height() : 0 ) };
    const unsigned int column_of_last_entry                  { ( ((n_entries_in_bandmap - start_entry) - 1) / win_bandmap.height() ) + 1 };    // wrt 1

    should_increment = !(column_of_last_entry < number_of_columns);
  }

  if ( should_increment or (bm.column_offset() != 0) )
  { bm.column_offset( bm.column_offset() + ( should_increment ? 1 : -1 ) ) ;

    alert("Bandmap column offset set to: "s + to_string(bm.column_offset()));

    bm.increment_version();  // to force immediate on-screen update
    win_bandmap <= bm;
    display_bandmap_filter(bm);
  }

  return true;
}

/// get the default mode on a frequency
//MODE default_mode(const frequency& f)
MODE default_mode(const frequency f)
{ const BAND b { to_BAND(f) };

  try
  { const auto break_points { context.mode_break_points() };

    return ( (f < break_points.at(b)) ? MODE_CW : MODE_SSB );
  }

  catch (...)                                               // use default break points (defined in bands-modes.h)
  { return ( (f < MODE_BREAK_POINT[b]) ? MODE_CW : MODE_SSB );
  }
}

/*! \brief          Update the QSLS window
    \param  str     callsign to use for the update

    Uses the data from the old ADIF log.
    The contents of the QSLS window are in the form aaa/bbb/ccc:
      aaa number of QSLs for the call <i>str</i>
      bbb number of QSOs for the call <i>str</i>
      ccc number of QSOs on the current band and mode for the call <i>str</i>

    If <i>str</i> contains more than one word (e.g., "G4AMJ DUPE") only the first word
    is used.
*/
//void update_qsls_window(const string& str)
void update_qsls_window(const string_view str)
{ static tuple<string, BAND, MODE> last_target;

  const string                    callsign    { nth_word(str, 1, 1) };  // remove "DUPE" if necessary
  const BAND                      b           { current_band };
  const MODE                      m           { current_mode };
  const tuple<string, BAND, MODE> this_target { callsign, b, m };

  if (this_target != last_target)   // only change contents of win_qsls if the target has changed
  { last_target = this_target;

    win_qsls < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= "QSLs: "s;

    if (callsign.length() >= 3)
    { const unsigned int     n_qsls                   { olog.n_qsls(callsign) };
      const unsigned int     n_qsos                   { olog.n_qsos(callsign) };
      const unsigned int     n_qsos_this_band_mode    { olog.n_qsos(callsign, b, m) };
      const bool             confirmed_this_band_mode { olog.confirmed(callsign, b, m) };
      const PAIR_NUMBER_TYPE default_colour_pair      { colours.add(win_qsls.fg(), win_qsls.bg()) };
 
      PAIR_NUMBER_TYPE new_colour_pair     { default_colour_pair };

      if ( (n_qsls == 0) and (n_qsos != 0) )
        new_colour_pair = colours.add(COLOUR_RED, win_qsls.bg());

      if (n_qsls != 0)
        new_colour_pair = colours.add(COLOUR_GREEN, win_qsls.bg());

      if (new_colour_pair != default_colour_pair)
        win_qsls.set_colour_pair(new_colour_pair);

      win_qsls < pad_leftz(n_qsls, 3)
               < colour_pair(default_colour_pair) < "/"s
               < colour_pair(new_colour_pair) < pad_leftz(n_qsos, 3)
               < colour_pair(default_colour_pair) < "/"s;

      if (n_qsos_this_band_mode != 0)
        win_qsls < colour_pair(colours.add( (confirmed_this_band_mode ? COLOUR_GREEN : COLOUR_RED), win_qsls.bg() ) );

      win_qsls <= pad_leftz(n_qsos_this_band_mode, 3);

      win_qsls.set_colour_pair(default_colour_pair);
    }
  }
}

/*! \brief                    Process an F1 keystroke in the CALL window
    \param original_contents  contents of the CALL window
    \return                   always returns <i>true</i>

 F1 -- first step in SAP QSO during run
    turn on sub-receiver, put sub-receiver on frequency of call (or substring of call)
    moves call to BCALL window
 intent here is to see whether we can hear the station whose call is in the CALL window
*/
bool process_keypress_F1(const string_view original_contents)
{ if (drlog_mode == DRLOG_MODE::CQ)      // don't do anything in SAP mode
  { if (original_contents.empty())      // nothing in CALL: emtpy bcall and turn off sub-receiver
    { win_bcall <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
      rig.sub_receiver_disable();
    }
    else
    {
// assume it's a call -- look for the same call in the current bandmap
      bandmap_entry be { bandmaps[current_band][original_contents] };

      if (be.callsign().empty())          // didn't find an exact match; try a substring search
        be = bandmaps[current_band].substr(original_contents);

      const BAND old_b_band { to_BAND(rig.rig_frequency_b()) };

      rig.rig_frequency_b(be.freq());
      win_bcall < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= be.callsign();     // put call in BCALL

      if (old_b_band != to_BAND(be.freq())) // stupid K3 swallows the following sub-receiver command if it's changed bands; may be able to remove this now
        sleep_for(100ms);

      rig.sub_receiver_enable();

// move contents of CALL to BCALL, clear CALL
      if (win_bcall.defined() and !win_call.empty())
        win_call <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;
    }
  }

  return true;
}

/*! \brief                    Process an F2 keystroke
    \return                   always returns <i>true</i>

  F2 toggle: split and force SAP mode
  intent here is to be ready to call the station: the TX is on the other station's QRG (which is VFO B)
*/
bool process_keypress_F2(void)
{ if (rig.split_enabled())
  { rig.split_disable();
    enter_cq_or_sap_mode(a_drlog_mode);
  }
  else
  { rig.split_enable();
    a_drlog_mode = drlog_mode;
    enter_sap_mode();
  }

  return true;
}

bool process_keypress_F4(void)
{ if (win_bcall.defined())
  { const string tmp   { win_call.read() };
    const string tmp_b { win_bcall.read() };

    win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp_b;
    win_bcall < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp;

    const string call_contents { tmp_b };

    string exchange_contents;

    if (win_bexchange.defined())
    { const string tmp   { win_exchange.read() };
      const string tmp_b { win_bexchange.read() };

      win_exchange < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp_b;

      exchange_contents = tmp_b;

      win_bexchange < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp;
    }

// put cursor in correct window
    if (remove_peripheral_spaces <std::string> (win_exchange.read()).empty())        // go to CALL window
    { win_call.move_cursor(call_contents.find(' '), 0);               // first empty space
      win_call.refresh();
      set_active_window(ACTIVE_WINDOW::CALL);
      win_exchange.move_cursor(0, 0);
    }
    else                                                              // go to EXCHANGE window
    { if (const size_t posn { exchange_contents.find_last_of(DIGITS_AND_UPPER_CASE_LETTERS) }; posn != string::npos)  // posn of first empty space
      { win_exchange.move_cursor(posn + 1, 0);
        win_exchange.refresh();
        set_active_window(ACTIVE_WINDOW::EXCHANGE);
      }
    }
  }

    return true;
}

/*! \brief      Process an F5 keystroke in the CALL or EXCHANGE windows
    \return     always returns <i>true</i>
*/
bool process_keypress_F5(void)
{
#if 0
  if (rig.split_enabled())      // split is enabled; go back to situation before it was enabled
  { rig.split_disable();
    enter_cq_or_sap_mode(a_drlog_mode);
  }
  else                          // split is disabled, enable and prepare to work stn on B VFO
  { rig.split_enable();
    a_drlog_mode = drlog_mode;
    enter_sap_mode();
  }
#endif

  process_keypress_F2();

#if 0
  if (win_bcall.defined())                      // swap contents of CALL and BCALL
  { const string tmp   { win_call.read() };
    const string tmp_b { win_bcall.read() };

    win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp_b;
    win_bcall < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp;

    const string call_contents { tmp_b };

    string exchange_contents;

    if (win_bexchange.defined())        // swap contents of EXCHANGE and BEXCHANGE
    { const string tmp   { win_exchange.read() };
      const string tmp_b { win_bexchange.read() };

      win_exchange < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp_b;
      win_bexchange < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= tmp;

      exchange_contents = tmp_b;
    }

// put cursor in correct window
    if (remove_peripheral_spaces <std::string> (win_exchange.read()).empty())        // go to the CALL window
    { const size_t posn { call_contents.find(' ') };                    // first empty space

      win_call.move_cursor(posn, 0);
      win_call.refresh();
      set_active_window(ACTIVE_WINDOW::CALL);

      win_exchange.move_cursor(0, 0);
    }
    else
    { if (const size_t posn { exchange_contents.find_last_of(DIGITS_AND_UPPER_CASE_LETTERS) }; posn != string::npos)    // posn of first empty space
      { win_exchange.move_cursor(posn + 1, 0);
        win_exchange.refresh();
        set_active_window(ACTIVE_WINDOW::EXCHANGE);
      }
    }
  }

  return true;
#endif

  process_keypress_F4();

  return true;
}

/// update the QTC QUEUE window
void update_qtc_queue_window(void)
{ win_qtc_queue < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

  if (qtc_buf.n_unsent_qsos())
  { const unsigned int      win_height          { static_cast<unsigned int>(win_qtc_queue.height()) };
    const unsigned int      n_to_display        { min(qtc_buf.n_unsent_qsos(), win_height) };
    const vector<qtc_entry> qtc_entries_to_send { qtc_buf.get_next_unsent_qtc(n_to_display) };

    unsigned int index { 1 };

    win_qtc_queue.move_cursor(0, win_height - 1);

    for (const auto& qe : qtc_entries_to_send)
      win_qtc_queue <  reformat_for_wprintw(pad_left(to_string(index++), 2) + SPACE_STR + qe.to_string(), win_qtc_queue.width());
  }

  win_qtc_queue.refresh();
}

/*! \brief      Toggle whether CW is sent
    \return     whether toggle was performed

    Updates WPM window
*/
bool toggle_cw(void)
{ if (cw_p)
  { cw_p->toggle();

    win_wpm < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= (cw_p->disabled() ? "NO CW"s : (to_string(cw_p->speed()) + " WPM"s) );   // update display

    return true;
  }
  else
    return false;
}

/*! \brief      Change CW speed as a function of a keyboard event
    \param  e   keyboard event
    \return     whether speed was changed

    Updates WPM window
*/
bool change_cw_speed(const keyboard_event& e)
{ if (cw_p)
  { int change { (e.is_control() ? 1 : static_cast<int>(cw_speed_change)) };

    if (e.symbol() == XK_Prior)
      change = -change;

    cw_speed(cw_p->speed() - change);  // effective immediately
    return true;
  }

  return false;
}

/*! \brief          Send a string to the SCRATCHPAD window
    \param  str     string to add
    \return         true
*/
//bool send_to_scratchpad(const string& str)
bool send_to_scratchpad(const string_view str)
{ const string scratchpad_str { substring <std::string> (hhmmss(), 0, 5) + SPACE_STR + rig.rig_frequency().display_string() + SPACE_STR + str };

  win_scratchpad < WINDOW_ATTRIBUTES::WINDOW_SCROLL_UP < WINDOW_ATTRIBUTES::WINDOW_BOTTOM_LEFT <= scratchpad_str;

  return true;
}

/// print the names of all the running threads
void print_thread_names(void)
{ ost << "Running threads:" << endl;

  SAFELOCK(thread_check);

  FOR_ALL(thread_names, [] (const string& thread_name) { ost << "  " << thread_name << endl; } );
}

/// decrease the counter for the number of running threads
//void end_of_thread(const string& name)
void end_of_thread(const string_view name)
{ SAFELOCK(thread_check);

  ost << "thread [" << name  << "] is exiting" << endl;

  n_running_threads--;

//  const auto n_removed { thread_names.erase(name) };    // erase does not yet support heterogeneous lookup
  const auto n_removed { thread_names.erase( string { name } ) };    // erase() does not yet support heterogeneous lookup

  if (n_removed)
    ost << "removed: " << name << endl;
  else
    ost << "unable to remove: " << name << endl;

  ost << "concluding end_of_thread for thread " << name << "; " << n_running_threads << " still running" << endl;
  
// debug
  print_thread_names();
}

/*! \brief          Change windows because of a change in my frequency
    \param  f       my new frequency
    \param  m       mode

    If the relevant bandmap has changed, but my frequency hasn't changed since the last time that this was called, THEN DO NOTHING

    I suspect that there is still a bug in here somewhere, leading to the occasional inconsistent update that (rarely) appears
*/
//void update_based_on_frequency_change(const frequency& f, const MODE m)
void update_based_on_frequency_change(const frequency f, const MODE m)
{ //static frequency last_update_frequency { };
  ost << NOW_TP() << ": update_based_on_frequency_change() called from THREAD NAME: " << my_thread_name() << endl;

 // if (my_thread_id() == display_rig_status_thread_id)
 //   ost << "update_based_on_frequency_change() called from display_rig_status() thread" << endl;

//  if (debug)
  { ost << "update_based_on_frequency_change() to: " << f.hz() << endl;
    ost << "last_update_frequency = " << static_cast<frequency>(last_update_frequency) << endl;
  }

  if (f == last_update_frequency)   // don't update if the frequency hasn't changed
  { //ost << "frequency has not changed: " << f << "; NO UPDATE" << endl;
    return;
  }

    //ost << "updating to: " << f << endl;

  const frequency mx_f { rig.rig_frequency() };

  if (f != mx_f)
  { ost << "WARNING! f = " << f.hz() << "; mx = " << mx_f.hz() << "; last update = " << static_cast<frequency>(last_update_frequency) << endl;
    ost << "ignoring update" << endl;
    ost << "backtrace in ignored call: " << endl << std_backtrace(BACKTRACE::ACQUIRE) << endl;

    return;
  }
  else
    ost << "f = mx = " << mx_f.hz() << "; last update = " << static_cast<frequency>(last_update_frequency) << "; looks OK; proceeding" << endl;

// trying to fix problem identified in 2023 LZ DX wherein audio was auto-restarted 2 seconds after it was auto-stopped

// we can enter a state where the rig is on the QRG of a post, and was on the same QRG last time this routine was executed,
// but the marker is not in the correct position
//  const bool tmp_changed_frequency { (f != last_update_frequency) or ((old_f.hz() != 0) and (old_f.hz() != my_bandmap_entry.freq())) };

// we move up or down to the next entry, even if we are very close to its frequency. Might want to skip
// an entry if we are close to it (say, within a couple of hundred Hz); in fact, right now we don't
// actually look at any entry frequencies, we just go where we are told to go

  bool tmp_changed_frequency { f != last_update_frequency };  // this should always be true (see the above immediate return path)

  if (debug)
  { ost << "time = " << hhmmss() << endl;
    ost << "inside update_based...; f = " << f.hz() << "; last_update_frequency = " << static_cast<frequency>(last_update_frequency).hz() << endl;
    ost << "tmp_changed_frequency = " << boolalpha << tmp_changed_frequency << endl;
  }

// the following ensures that the bandmap entry doesn't change while we're using it.
// It does not, however, ensure that this routine doesn't execute simultaneously from two
// threads. To do that would require holding a lock for a very long time, and would nest
// lock acquisitions dangerously

  bandmap_entry mbe_copy;

  { SAFELOCK(my_bandmap_entry);
    
    mbe_copy = my_bandmap_entry;

    if (debug)
    { ost << "my_bandmap_entry = " << my_bandmap_entry.freq().hz() << endl;
      ost << "f = " << f.hz() << endl;
    }

    tmp_changed_frequency = tmp_changed_frequency or (my_bandmap_entry.freq() != f);    // this should always be true, and remain true

    if (debug)
    { ost << "second term: " << boolalpha << (my_bandmap_entry.freq() != f) << endl;
      ost << "tmp_changed_frequency after OR = " << boolalpha << tmp_changed_frequency << endl;
    }

    if (tmp_changed_frequency)
    { if (debug)
        ost << "tmp changed frequency from " << static_cast<frequency>(last_update_frequency) << " to " << f << endl;
      last_update_frequency = f;
    }

    if (debug)
      ost << "frequency of my_bandmap_entry = " << mbe_copy.freq() << endl;
  }

  if (tmp_changed_frequency)
  { time_last_qsy = NOW();
    mbe_copy.freq(f);               // also updates the band; stores frequency as (f - MY_MARKER_BIAS)

    {
// really need a way to say that if a pre-emptive change (; or ') has occurred since we started the routine, we should
// abort and not display anything.

      bandmap& bm { bandmaps[mbe_copy.band()] };
      safelock bm_lock(bm._bandmap_mutex);             // attempt to stop race condition with ; and '

      display_band_mode(win_band_mode, mbe_copy.band(), mbe_copy.mode());

      bm += mbe_copy;         // changes bm version

      { SAFELOCK(my_bandmap_entry);
    
        my_bandmap_entry = mbe_copy;
      }

      update_bandmap_window(bm /*, bm_version */);    // really shouldn't do this if there's been a manual change via, for example, ; or '
      display_bandmap_filter(bm);

// is there a station close to our frequency?
// use the filtered bandmap (maybe should make this controllable? but used to use unfiltered version, and it was annoying
// to have invisible calls show up when I went to a frequency
      const string nearby_callsign { bm.nearest_displayed_callsign(f, context.guard_band(m)) };

      if (!nearby_callsign.empty())
      { ost << "displaying nearby callsign: " << nearby_callsign << " for QRG: " << f.khz() << endl;
        display_nearby_callsign(nearby_callsign);
      }
      else                                        // no nearby callsign; possibly clear windows
      { const bool in_call_window { (active_window == ACTIVE_WINDOW::CALL) };  // never update call window if we aren't in it

        if (in_call_window)
// see if we are within twice the guard band before we clear the call window
        { const string        call_contents { remove_peripheral_spaces <std::string> (win_call.read()) };
          const bandmap_entry be            { bm[call_contents] };
          const unsigned int  f_diff        { static_cast<unsigned int>(abs(be.freq().hz() - f.hz())) };

          if (f_diff > 2 * context.guard_band(m))    // delete this and prior three lines to return to old code
          { if (!win_nearby.empty())
              win_nearby <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;

            if (!call_contents.empty())
            { string last_call;

              { SAFELOCK(dupe_check);

                last_call = last_call_inserted_with_space;
              }

              if ((call_contents == last_call) or (call_contents == (last_call + " DUPE"s)) )
                win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;
            }
          }
        }
      }
    }       // release bm lock

// possibly start audio recording
    if ( allow_audio_recording and (audio_recording_mode == AUDIO_RECORDING::AUTO) /* (context.start_audio_recording() == AUDIO_RECORDING::AUTO) */ and !audio.recording())
    { start_recording(audio, context);
      alert("audio recording started due to change in frequency"s);
    }
  }                 // end of changed frequency
}

/*! \brief          Process a bandmap function, to jump to the next frequency returned by the function
    \param  fn_p    pointer to function
    \param  dirn    direction in which the function is to be applied
    \param  nskip   number of stations to skip (default = 0)
    \return         always returns true

    This is a friend function to the bandmap class, to allow us to lock the bandmap here.
    This is NOT the version of process_bandmap_function() that is used with the ";" and "'" keys.
*/
bool process_bandmap_function(BANDMAP_MEM_FUN_P fn_p, const BANDMAP_DIRECTION dirn, const int16_t nskip)
{ ost << "explicit process_band_map_function() called" << endl;

  bandmap& bm { bandmaps[current_band] };

  safelock bm_lock(bm._bandmap_mutex);    // hold the lock for this entire routine; this essentially forces this update to occur on-screen

  const frequency     f_rig { rig.rig_frequency() };
  const bandmap_entry be    { (bm.*fn_p)( f_rig, dirn, nskip ) };       // get bandmap entry for destination

  if (debug)
  { ost << "DEBUG process_bandmap_function(): " << endl
        << "current actual frequency from rig = " << rig.rig_frequency() << endl
        << "; bandmap version: " << bm.version_str() << endl
        << "; my bandmap entry(): " << bm.my_bandmap_entry().to_brief_string() << endl
        << "; next bandmap entry: " << be.to_brief_string() << endl;
  }

// 240929
  if ( be.empty() and (f_rig.difference(bm.my_bandmap_entry().freq()) > (2 * MY_MARKER_BIAS)) )  // update bm even if be is empty (i.e., at lowest or highest entry in bm)
  { if (debug)
      ost << "forcing bm change to frequency: " << f_rig << "; bm says freq is: " << bm.my_bandmap_entry().freq() << endl;

    ok_to_poll_k3 = false;  // since we're going to be updating things anyway, briefly inhibit polling of a K3

    bandmap_entry my_be { bm.my_bandmap_entry() };

    my_be.freq(f_rig);
    bm += my_be;    // changes version of the bm

    update_based_on_frequency_change(f_rig, current_mode);   // update win_bandmap, and other windows

    if (debug)
      ost << "after update: my bandmap entry now: " << bm.my_bandmap_entry() << endl;

    ok_to_poll_k3 = true;
  }

  if (!be.empty())  // get and process the next non-empty stn/mult, according to the function
  { if (debug)
      ost << "Setting frequency to: " << be.freq() << endl;

    ok_to_poll_k3 = false;  // since we're going to be updating things anyway, briefly inhibit polling of a K3

    rig.rig_frequency(be.freq());
    win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= be.callsign();

    enter_sap_mode();

// we may require a mode change
    possible_mode_change(be.freq());

    update_based_on_frequency_change(be.freq(), current_mode);   // update win_bandmap, and other windows

    if (debug)
      ost << "after update: my bandmap entry now: " << bm.my_bandmap_entry() << endl;

    ok_to_poll_k3 = true;

    SAFELOCK(dupe_check);                                   // nested w/ bm_lock
    last_call_inserted_with_space = be.callsign();
  }

// check that we aren't somehow in an inconsistent state:
// in 2024 CQ WPX CW I noticed a few times that the rig didn't seem to move,
// although the CALL window had the contents as if it had moved
  bandmap_entry mbe_copy;

  { SAFELOCK(my_bandmap_entry);

    mbe_copy = my_bandmap_entry;
  }

  if ( (be.freq() - mbe_copy.freq()) > 100)   // if we're more than 100Hz from where we expect
  { ost << "INCONSISTENT BANDMAP STATE" << endl
        << "be: " << be << endl
        << "mbe = " << mbe_copy << endl
        << "actual measured frequency of rig = " << rig.rig_frequency() << endl;

    debug = true;     // automatically turn on debugging
  }

  return true;
}

/*! \brief          Jump to the next frequency up or down on the displayed bandmap, possibly skipping some entries
    \param  dirn    direction in which the function is to be applied
    \param  nskip   number of stations to skip (default = 0)
    \return         always returns true

    This is a friend function to the bandmap class, to allow us to lock the bandmap here.
    This is the version of process_bandmap_function() that is used with the ";" and "'" keys.
*/
bool process_bandmap_function(const BANDMAP_DIRECTION dirn, const int16_t nskip)
{ ost << endl << endl << "inside process_bandmap_function; dirn = " << ((dirn == BANDMAP_DIRECTION::DOWN) ? "DOWN"s : "UP"s) << endl;

  constexpr frequency MAX_SKEW { 95_Hz };

  bandmap& bm { bandmaps[current_band] };

  safelock bm_lock(bm._bandmap_mutex);

  const frequency     f_rig { rig.rig_frequency() };
  const bandmap_entry be    { bm.next_displayed_be(f_rig, dirn, nskip, MAX_SKEW) };   // get next displayed be

  if (debug)
  { ost << "DEBUG process_bandmap_function(): " << endl
        << "f_rig = " << f_rig
        << "; current actual frequency from rig = " << rig.rig_frequency()
        << "; bandmap version: " << bm.version_str() << endl
        << "; my bandmap entry(): " << bm.my_bandmap_entry().to_brief_string() << endl
        << "; next bandmap entry: " << be.to_brief_string()
        << endl;
  }
  else
  { ost << "inside process_bandmap_function(): " << endl
        << "f_rig = " << f_rig
        << "; current actual frequency from rig = " << rig.rig_frequency()
        << "; bandmap version: " << bm.version_str() << endl
        << "; my bandmap entry(): " << bm.my_bandmap_entry().to_brief_string() << endl
        << "; next bandmap entry: " << be.to_brief_string()
        << endl;
  }

  if (!be.empty())  // get and process the next non-empty stn/mult, according to the function; this tests for non-empty callsign
  { ok_to_poll_k3 = false;  // since we're going to be updating things anyway, briefly inhibit polling of a K3

//if (debug)
//      ost << "at time " << NOW_TP() << ": " << endl;
      ost << "moving from: " << f_rig << endl;
      ost << "next bandmap entry: setting frequency to: " << be.freq() << endl;

    rig.rig_frequency(be.freq());                                   // QSY to next station
    win_call < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= be.callsign();    // display call of next station

    enter_sap_mode();

// we may require a mode change
    possible_mode_change(be.freq());
    update_based_on_frequency_change(be.freq(), current_mode);   // update win_bandmap, and other windows

    //if (debug)
      ost << "after window update based on frequency change purportedly to " << be.freq() << "; my bandmap entry now: " << bm.my_bandmap_entry().to_brief_string() << endl;

//    { const bandmap_entry my_be { bm[MY_MARKER] };
//
//      ost << "bandmap_entry at MY_MARKER: " << my_be << endl;
//    }

    ok_to_poll_k3 = true;

    SAFELOCK(dupe_check);                                   // nested w/ bm_lock
    last_call_inserted_with_space = be.callsign();
  }

  ost << "end of process_bandmap_function" << endl << endl << endl;

  return true;
}

/*! \brief      Possibly change mode in accordance with a frequency
    \param  f   frequency

    Changes to the default mode associated with <i>f</i>, if multiple
    modes are supported in this contest
*/
//void possible_mode_change(const frequency& f)
void possible_mode_change(const frequency f)
{ if (multiple_modes)
  { if (const MODE m { default_mode(f) }; m != current_mode)
    { rig.rig_mode(m);
      current_mode = m;
      display_band_mode(win_band_mode, current_band, m);
    }
  }
}

/*! \brief          Toggle the state of audio recording
    \param  audio   the audio recorder
    \return         <i>true</i>

    Does nothing if recording is not permitted.
*/
bool toggle_recording_status(audio_recorder& audio)
{ if (allow_audio_recording)      // toggle only if recording is allowed
  { if (audio.recording())
      stop_recording(audio);
    else                                  // not recording
      start_recording(audio, context);

    update_recording_status_window();
  }
  else
    alert("toggling audio not permitted"s);

  return true;
}

/*! \brief              Start audio recording
    \param  audio       the audio recorder
    \param  context     context for the contest

    Updates recording status window
*/
void start_recording(audio_recorder& audio, const drlog_context& context)
{ 
// don't do anything if recording is not allowed, or if we're already recording
  if (!allow_audio_recording or audio.recording())
    return;  
 
// configure some stuff
  audio.base_filename(context.audio_file());
  audio.maximum_duration(context.audio_duration() * 60);    // convert minutes to seconds
  audio.pcm_name(context.audio_device_name());
  audio.n_channels(context.audio_channels());
  audio.samples_per_second(context.audio_rate());
 //   audio.log("audio-log"s);                                  // must come before initialise()
  audio.register_error_alert_function(audio_error_alert);
  audio.initialise();
  
  audio.capture();      // turn on the capture

  update_recording_status_window();
}

/*! \brief              Stop audio recording
    \param  context     context for the contest

    Updates recording status window
*/
void stop_recording(audio_recorder& audio)
{ if (allow_audio_recording)                    // don't do anything if recording is not allowed
  { audio.abort();

    update_recording_status_window();
  }
}

/*! \brief      Get the status of the RX ant, and update <i>win_rx_ant</i> appropriately
    \return     <i>true</i>
*/
bool update_rx_ant_window(void)
{ if (win_rx_ant.defined())                     // don't do anything if the window isn't defined
  { const bool   rx_ant_in_use   { rig.rx_ant() };
    const string window_contents { win_rx_ant.read() };

    if ( rx_ant_in_use and (window_contents != "RX"sv) )
      win_rx_ant < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= "RX"s;

    if ( !rx_ant_in_use and (window_contents != "TX"sv) )
      win_rx_ant < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= "TX"s;
  }

  return true;
}

/*! \brief          Process backspace
    \param  win     window
    \return         <i>true</i>
 */
bool process_backspace(window& win)
{ win.delete_character(win.cursor_position().x() - 1);
  win.refresh();

  return true;
}

/*! \brief          Run an external command
    \param  cmd     command to run
    \return         output of command <i>cmd</i?

    https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c-using-posix
    note that the code there contains an extra right curly bracket
 */
//string run_external_command(const string& cmd)
string run_external_command(const string_view cmd)
{ constexpr size_t BUFLEN { 128 };      // reasonable size for read buffer

  array<char, BUFLEN> buffer;
  string              result { };

  unique_ptr<FILE, decltype(&pclose)> pipe(popen(string(cmd).c_str(), "r"), pclose);

  if (!pipe)
    return ( alert("WARNING: Error executing command: "s + cmd), string { } );

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    result += buffer.data();

  return result;
}

/*! \brief        Thread function to get SFI, A, K indices
    \param  cmd   the command to run
*/
void get_indices(const string cmd)    ///< Get SFI, A, K
{
  { start_of_thread("get indices"s);

    try
    { const string indices { run_external_command(cmd) };

      win_indices < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT < "Last lookup at: " < substring <std::string_view> (hhmmss(), 0, 5) < EOL
                  <= indices;
    }

    catch (...)
    { ost << "CAUGHT EXCEPTION IN GET_INDICES" << endl;
    }
  }  // ensure that all objects call destructors, whatever the implementation

  end_of_thread("get indices"s);
}

/*! \brief          Time in seconds since the last QSO
    \param  logbk   the current log
    \return         the time, in seconds, since the most recent QSO in <i>logbk</i>

    Returns zero if <i>logbk</i> is empty.
*/
int time_since_last_qso(const logbook& logbk)
{ const QSO last_qso { logbk.last_qso() };

  return ( last_qso.empty() ? 0 : (NOW() - last_qso.epoch_time()) );      // get the time from the kernel
}

/*! \brief      Time in seconds since the last QSY
    \return     the time, in seconds, since the most recent QSY

    Returns time since program start if there has been no QSY
*/
int time_since_last_qsy(void)
{ SAFELOCK(my_bandmap_entry);

  return (NOW() - time_last_qsy);
}

/*! \brief              Possibly update the variable that holds the greatest distance
    \param  dx_gs       DX grid square
    \param  callsign    DX callsign

    Also updates <i>win_best_dx</i> if necessary
*/
//void update_best_dx(const grid_square& dx_gs, const string& callsign)
void update_best_dx(const grid_square& dx_gs, const string_view callsign)
{ static const string INVALID_GRID { "AA00"s };                 // the way to mark a bad grid in the log; don't calculate distance to this square

  if (win_best_dx.valid())              // check even though it should have been checked before being called
  { if (!dx_gs.designation().empty() and dx_gs.designation() != INVALID_GRID)
    { float distance_in_units { ( my_grid - grid_square { dx_gs.designation() } ) };    // km

      if (best_dx_is_in_miles)
        distance_in_units = kilometres_to_miles(distance_in_units);

      if (distance_in_units >= greatest_distance)
      { string str { pad_left(css(static_cast<int>(distance_in_units + 0.5)), 6) };

        str = pad_right( (str + SPACE_STR + callsign), win_best_dx.width());

        win_best_dx < WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT < WINDOW_ATTRIBUTES::WINDOW_SCROLL_DOWN <= str;

        greatest_distance = distance_in_units;
      }
    }
  }
}

/*! \brief              Populate the call history window
    \param  callsign    full or partial call

    Also handles the colour of the QTC HINT window if that window exists
*/
void populate_win_call_history(const string_view callsign)
{ static const set<MODE> call_history_modes { MODE_CW, MODE_SSB };

  if (win_call_history.valid())              // check even though it should have been checked before being called
  { win_call_history < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= centre(callsign, win_call_history.height() - 1);    // write the (partial) callsign

    const int  bg                  { win_call_history.bg() };
    const auto default_colour_pair { colours.add(win_call_history.fg(), bg) };

    WIN_INT_TYPE line_nr { 0 };

// keep track of which slots have QSOs and QSLs
    int n_green { 0 };
    int n_red   { 0 };

    for (const auto b : call_history_bands)
    { const cursor c_posn { 0, line_nr++ };

      win_call_history < c_posn < pad_left(BAND_NAME[b], 3);            // low band is on bottom

      for (const auto m : call_history_modes)
      { const unsigned int n_qsos           { olog.n_qsos(callsign, b, m) };
        const int          fg               { ( (n_qsos == 0) ?  COLOUR_WHITE : ( olog.confirmed(callsign, b, m) ? COLOUR_GREEN : COLOUR_RED ) ) };
        const auto         this_colour_pair { colours.add(fg, bg) };

        win_call_history < colour_pair(this_colour_pair) < pad_left(to_string(n_qsos), 4) < colour_pair(default_colour_pair);

        if (fg == COLOUR_GREEN)
          n_green++;

        if (fg == COLOUR_RED)
          n_red++;
      }
    }

    win_call_history.refresh();

// QTC hint
    if (win_qtc_hint.valid())
    { const bool send_qtc         { ((n_green + n_red) > 1) and (n_green > n_red) };    // must have at least two slots confirmed
      const int  window_colour    { send_qtc ? win_qtc_hint_fg : win_qtc_hint_bg };
      const auto this_colour_pair { colours.add(window_colour, window_colour) };

      win_qtc_hint < colour_pair(this_colour_pair) < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= " "s;
    }
  }
}

/*! \brief  Insert the current rig configuration into the memories

    Removes the oldest memory if the memory is full.
    Also displays the (updated) contents of the memories.
*/
void insert_memory(void)
{ if (n_memories)
  { memory_entry me;

    me.freq(rig.rig_frequency());
    me.mode(current_mode);
    me.drlog_mode(drlog_mode);

    memories.push_front(me);        // NB this deque is pushed to front, popped from back

    while (memories.size() > n_memories)
      memories.pop_back();

    display_memories();
  }
}

/// display all the memories
void display_memories(void)
{ win_memories < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

  WIN_INT_TYPE line_nr { static_cast<WIN_INT_TYPE>(win_memories.height() - 1) };
  int          number  { 0 };

  for (const auto& me : memories)
  { const cursor c_posn { 0, line_nr-- };

    win_memories < c_posn < to_string(number++) < SPACE_STR < (me.freq()).display_string()
                          < pad_left(MODE_NAME[me.mode()], 5)
                          < (me.drlog_mode() == DRLOG_MODE::CQ ? "  CQ"s : "  SAP"s);
  }

  win_memories.refresh();
}

/*! \brief          Update the SCORE window
    \param  score   the score to write to the window
*/
void update_score_window(const unsigned int score)
{ if (scoring_enabled)
  { const static string RUBRIC { "Score: "s };

    win_score < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < RUBRIC
              <= (pad_left(separated_string(score, TS), win_score.width() - RUBRIC.length()));
  }
}

/*! \brief      Update the BANDMAP FILTER window
    \param  bm  the bandmap that contains the filter information to be written
*/
void display_bandmap_filter(bandmap& bm)                                                       ///< display the bandmap cull/filter information in win_bandmap_filter
{ //SAFELOCK(bm);

  win_bandmap_filter < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE;

  if (bm.cull_function())
    win_bandmap_filter < "(C"s < to_string(bm.cull_function()) < ") "s;

  win_bandmap_filter < "["s < to_string(bm.column_offset()) < "] "s <= bm.filter();
}

/*! \brief  Update the SYSTEM MEMORY window
*/
void update_system_memory(void)
{ static procfs proc_fs;

  static const long page_size { sysconf(_SC_PAGESIZE) };

  try
  { const auto   rss           { (proc_fs.stat_rss() * page_size) / MILLION };
    const auto   mem_available { meminfo.mem_available()  / MILLION };
    const auto   mem_total     { meminfo.mem_total() / MILLION };
    const string contents      { to_string(rss) + "M / "s + to_string(mem_available) + "M / "s + to_string(mem_total) + "M"s };

    win_system_memory < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE <= centre(contents, 0);
  }

  catch (const string_function_error& e)        // in 2019 RDA, the open in read_file() failed and threw an exception!!
  { ost << "meminfo threw string error: code = " << e.code() << "; reason = " << e.reason() << endl;
    alert("Exception in meminfo!"s);
  }

  catch (...)
  { ost << "meminfo threw non-string error" << endl;
    alert("Non-string exception in meminfo!!"s);
  }
}

/// update value of <i>quick_qsy_info</i> and write it to <i>win_quick_qsy</i>
void update_quick_qsy(void)
{ const pair<frequency, MODE> quick_qsy_info { get_frequency_and_mode() };
//  const frequency&            f              { quick_qsy_info.first };
//  const MODE                  m              { quick_qsy_info.second };
  const auto [ f, m ] { quick_qsy_info };

  quick_qsy_map[BAND(f)] = quick_qsy_info;

  win_quick_qsy < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE
                <= pad_left(f.display_string(), 7) + SPACE_STR + MODE_NAME[m];
}

/// update the window containing the sizes of the bandmaps
void update_bandmap_size_window(void)
{ if (win_bandmap_size.valid())
  { win_bandmap_size < WINDOW_ATTRIBUTES::WINDOW_CLEAR < centre("BM SIZE"s, win_bandmap_size.height() - 1);

// modelled after populate_win_call_history()
    WIN_INT_TYPE line_nr { 0 };

    for (const auto b : permitted_bands)
    { const cursor c_posn { 0, line_nr++ };

      win_bandmap_size < c_posn < pad_left(BAND_NAME[b], 3)            // low band is on bottom
                       < pad_left(bandmaps[b].displayed_entries().size(), 5);
    }

    win_bandmap_size.refresh();
  }
}

/*! \brief              Return latitude and longitude of a call or partial call
    \param  callsign    call or partial call
    \return             latitude (+ve north) and longitude (+ve west) of <i>callsign</i>
    
    Returns (0, 0) if latitude or longitude cannot be calculated
*/
pair<float, float> latitude_and_longitude(const string_view callsign)
{ if (const string grid_name { exchange_db.guess_value(callsign, "GRID"s) }; is_valid_grid_designation(grid_name))
    return grid_square(grid_name).latitude_and_longitude();

  return (location_db.info(callsign) == location_info { }) ? pair<float, float> { }
                                                           : pair { location_db.latitude(callsign), -location_db.longitude(callsign) }; // minus sign to get in the correct direction
}

/*! \brief              Mark a callsign as not to be shown
    \param  callsign    call
    \param  b           band
    
    Removes <i>callsign</i> from bandmap, adds it to the do-not-show list, and
    appends it to the do-not-show file if one has been defined
*/
void do_not_show(const string_view callsign, const BAND b)
{ if (b == ALL_BANDS)
  { FOR_ALL(bandmaps, [&callsign] (bandmap& bm) { bm -= callsign;
                                                  bm.do_not_add(callsign);
                                                } );
  }
  else                          // single band
  { bandmap& bm = bandmaps[b];

    bm -= callsign;
    bm.do_not_add(callsign);
  }

  calls_to_do_not_show_file(calls_from_do_not_show_file(b) + callsign, b);
}

/*! \brief                  Find the first QSO in a chronological vector of ADIF3 records that occurs on or after a target date
    \param  vqsos           chronological vector of QSOs
    \param  target_idate    target date [YYYYMMDD]
    \return                 first QSO in <i>vqsos</i> whose date is on or after <i>idate</i>, and index of the QSO into <i>vqsos</i>

    A returned index of -1 implies that there are no QSOs after the target date 
*/
pair<adif3_record, int> first_qso_after(const vector<adif3_record>& vqsos, const int target_idate)
{ for (int n { 0 }; n < static_cast<int>(vqsos.size()); ++n)
    if (vqsos[n].idate() >= target_idate)
      return { vqsos[n], n };
      
  return { adif3_record { }, -1 };
};

/*! \brief                          Find the first QSO in a chronological vector of ADIF3 records that occurs on or after a target date; or the first confirmed QSO after a given index number
    \param  vqsos                   chronological vector of QSOs
    \param  target_idate            target date [YYYYMMDD]
    \param  index_last_marked_qso   the index of the most recently marked QSO
    \return                         first QSO in <i>vqsos</i> whose date is on or after <i>idate</i>, and index of the QSO into <i>vqsos</i>

    A returned index of -1 implies that there are no QSOs after the target date, and no confirmed QSOs after the index <i>index_last_marked_qso</i>
*/
pair<adif3_record, int> first_qso_after_or_confirmed_qso(const vector<adif3_record>& vqsos, const int target_idate, const int index_last_marked_qso)
{ if (index_last_marked_qso != static_cast<int>(vqsos.size() - 1))                                    // make sure the last marked qso wasn't the last qso in the vector
    for (int n { index_last_marked_qso + 1 }; n < static_cast<int>(vqsos.size()); ++n)
      if ( (vqsos[n].idate() >= target_idate) or vqsos[n].confirmed() )
        return { vqsos[n], n };
      
  return { adif3_record { }, -1 };
};

/*! \brief Build the old log info from an ADIF3 file

  If context.old_qso_age_limit() is non-zero (here, N), the algorithm is a bit tricky:
    on a per-call per-band per-mode basis:
      1. Find oldest QSO; mark it;
      2. Go forward N years, marking confirmed QSOs
      3. if not past current date, find oldest QSO past the date found in 2; mark it
          3a. Go forward N years, marking confirmed QSOs
          repeat steps 3 and 3a until past current date
      4. If most recent marked QSO is more than N years old, don't add any QSOs to the old log;
      5.   else add the marked QSO and any more recent ones to the old log


 How about the case where we received a QSL for a non-marked QSO? Maybe make that marked? After all,
if we received a QSL -- even if I didn't send one -- that should count for something.
*/
void adif3_build_old_log(void)
{ time_log <std::chrono::milliseconds> tl;

// calculate current and (roughly) 10-years-ago dates [note that we are probably running this shortly prior to a date change, so it's not precise]      
  const string dts            { date_time_string(SECONDS::NO_INCLUDE) };
  const string today          { substring <std::string> (dts, 0, 4) + substring <std::string> (dts, 5, 2) + substring <std::string> (dts, 8, 2) };
  const int    itoday         { from_string<int>(today) };
  const auto   old_qso_limit  { context.old_qso_age_limit() };          // number of years
  const string cutoff_date    { to_string(from_string<int>(today) - (old_qso_limit * 10'000)) }; // date before which QSOs don't count
  const bool   limit_old_qsos { old_qso_limit != 0 };                   // whether to limit old qsos

  auto add_record_to_olog = [] (const adif3_record& rec)
    { const string callsign { rec.callsign() };
      const BAND   b        { BAND_FROM_ADIF3_NAME[rec.band()] };
      const MODE   m        { MODE_FROM_NAME.at(rec.mode()) };

      olog.increment_n_qsos(callsign);
      olog.increment_n_qsos(callsign, b, m);

      if (rec.confirmed())
      { olog.increment_n_qsls(callsign);
        olog.qsl_received(callsign, b, m);
      }
    };

  alert("reading old log file: "s + context.old_adif_log_name(), SHOW_TIME::NO_SHOW);
  
  try
  { const adif3_file old_adif3_log { context_path,  context.old_adif_log_name(), STRING_SET { "BAND"s, "CALL"s, "MODE"s, "QSL_RCVD"s, "QSO_DATE"s } };    // this is not necessarily in chronological order; takes about 3--5 seconds to execute for 100,000 records

    alert("read "s + comma_separated_string(old_adif3_log.size()) + " ADIF records from file: "s + context.old_adif_log_name(), SHOW_TIME::NO_SHOW);
    
    if (!limit_old_qsos)
      FOR_ALL(old_adif3_log, [&add_record_to_olog] (const adif3_record& rec) { add_record_to_olog(rec); });
    else                                  // limit old QSOs
    { STRING_SET processed_calls;

      for (const adif3_record& rec : old_adif3_log)
      { if (const string& callsign { rec.callsign() }; !processed_calls.contains(callsign))   // if not yet processed this call
        { vector<adif3_record> matching_qsos { old_adif3_log.matching_qsos(callsign) };       // don't make const because it's going to be sorted

          if (!matching_qsos.empty())           // should always be true
          { SORT(matching_qsos, compare_adif3_records);    // in chronological order

            unordered_map<bandmode, vector<adif3_record>> bmode_records;

            FOR_ALL(matching_qsos, [&bmode_records] (const auto& rec) { bmode_records[{ BAND_FROM_ADIF3_NAME.at(rec.band()), MODE_FROM_NAME.at(rec.mode()) }] += rec; } );

// now for each different band/mode
            for ( const auto& [_, vrec] : bmode_records )
            { adif3_record last_marked_qso       { vrec[0] };
              int          index_last_marked_qso { 0 };

              int idate_last_marked_qso;
              int forward_idate_limit;

              pair<adif3_record, int> rec_index;  // next_marked_qso, index

              do
              { idate_last_marked_qso = last_marked_qso.idate();
                forward_idate_limit = ( idate_last_marked_qso + (old_qso_limit * 10'000) );                       // forward the required number of years
                rec_index = first_qso_after_or_confirmed_qso(vrec, forward_idate_limit, index_last_marked_qso);   // rec_index is pair: record and index of the record
     
                if (rec_index.second != -1)
                  tie(last_marked_qso, index_last_marked_qso) = rec_index;          // next marked QSO, index of next marked QSO
              } while ( (forward_idate_limit < itoday) and (rec_index.second != -1) );

              if (last_marked_qso.date() >= cutoff_date)  // one or more QSOs are sufficiently recent to add to the old log
                for (int n { index_last_marked_qso }; n < static_cast<int>(vrec.size()); ++n)
                  add_record_to_olog(vrec[n]);
            }
          }
          else  // no matching QSOs; should never happen
          { ost << "ERROR: NO MATCHING QSOS" << endl;
            exit(-1);
          }
        
          processed_calls += callsign;
        }   // end of processing for this call; do nothing if already processed
      }
    }
  }
      
  catch (const string_function_error& e)
  { ost << "Unable to read old log file: " << context.old_adif_log_name() << "code = " << e.code() << ", reason = " << e.reason() << endl;
    exit(-1);
  }
      
  catch (...)
  { ost << "Undefined error reading old log file: " << context.old_adif_log_name() << endl;
    exit(-1);
  }

  tl.end_now();
  ost << "time taken to prepare old log = " << tl.time_span<int>() << " milliseconds" << endl;

  win_message <= WINDOW_ATTRIBUTES::WINDOW_CLEAR;       // clear the MESSAGE window
}

/*! \brief          Send a single QTC entry (on CW)
    \param  qe      QTC entry to send
    \param  logit   whether to log the message that was sent
*/
void send_qtc_entry(const qtc_entry& qe, const bool log_it)
{ if (cw_p)
  { const string space        { (context.qtc_double_space() ? "  "s : SPACE_STR) };
    const char   char_to_send { t_char(qtc_long_t) };
    const string serno_str    { pad_left(remove_leading <std::string> (remove_peripheral_spaces <std::string> (qe.serno()), '0'), 3, char_to_send) };
    const string msg          { qe.utc() + space + qe.callsign() + space + serno_str };

    (*cw_p) << msg;  // don't use cw_speed because that executes asynchronously, so the speed would be back to full speed before the message is sent
    
    if (log_it)
      ost << "QTC sent: " << msg << endl;
  }
};

/*! \brief          Logs an audio-related error
    \param  msg     message to display

    Also alerts on the screen
*/
//void audio_error_alert(const string& msg)
void audio_error_alert(const string_view msg)
{ ost << "Audio error: " << msg << endl;

  alert(msg);
}

/*! \brief                  Is it currently daylight?
    \param  sunrise_time    time of sunrise (HH:MM)
    \param  sunset_time     time of sunset (HH:MM)
    \param  current_time    HH:MM
    \return                 whether <i>current_time</i> is in daylight
*/
//bool is_daylight(const string& sunrise_time, const string& sunset_time, const string& current_time)
//bool is_daylight(const string_view sunrise_time, const string_view sunset_time, const string& current_time)
bool is_daylight(const string_view sunrise_time, const string_view sunset_time, const string_view current_time)
{ if (sunrise_time == "DARK"sv)
    return false;

  if (sunrise_time == "LIGHT"sv)
    return true;

  if (sunset_time == sunrise_time)
    return false;              // keep it dark if sunrise and set are at same time

  if (sunset_time > sunrise_time) 
    return ( (current_time > sunrise_time) and (current_time < sunset_time) );

  if (sunset_time < sunrise_time)
    return !( (current_time > sunset_time) and (current_time < sunrise_time) );

  return false;  // should never reach here
}

/*! \brief      Toggle narrow/wide bandwidth if on CW
    \return     true

    Sets bandwidth to the wide bandwidth if it's not equal to the narrow bandwidth
*/
bool cw_toggle_bandwidth(void)
{ if (current_mode == MODE_CW)
    rig.bandwidth( (rig.bandwidth() == cw_bandwidth_wide) ? cw_bandwidth_narrow : cw_bandwidth_wide );

  return true;
}

/*! \brief      Toggle narrow/wide centre/bandwidth values if on SSB
    \return     true

    Sets bandwidth and centre frequency to the wide settings if not equal to the those settings; otherwise set to narrow settings
*/
bool ssb_toggle_bandwidth(void)
{ constexpr int BANDWIDTH_PRECISION { 50 };        // K3 can set only to 50 Hz boundaries

    ost << "ssb_bandwidth_narrow = " << ssb_bandwidth_narrow << endl;
    ost << "ssb_bandwidth_wide = " << ssb_bandwidth_wide << endl;
    ost << "ssb_centre_narrow = " << ssb_centre_narrow << endl;
    ost << "ssb_centre_wide = " << ssb_centre_wide << endl;

  if (current_mode == MODE_SSB)
  { enum SSB_AUDIO { SSB_WIDE,
                     SSB_NARROW
                   };

    const SSB_AUDIO bw { (abs(rig.bandwidth() - ssb_bandwidth_wide) < BANDWIDTH_PRECISION) ? SSB_NARROW : SSB_WIDE };

    ost << "bw = " << bw << endl;

    rig.bandwidth( (bw == SSB_NARROW) ? ssb_bandwidth_narrow : ssb_bandwidth_wide );
    rig.centre_frequency( (bw == SSB_NARROW) ? ssb_centre_narrow : ssb_centre_wide );
  }

  return true;
}

/*! \brief      Set the window that is receiving input
    \param  aw  the active window
*/
void set_active_window(const ACTIVE_WINDOW aw)
{ using enum ACTIVE_WINDOW;

  active_window = aw;

  switch (aw)
  { case CALL :
      win_active_p = &win_call;
      break;

    case EXCHANGE :
      win_active_p = &win_exchange;
      break;

    case LOG :
      win_active_p = &win_log;
      break;

    case LOG_EXTRACT :
      win_active_p = &win_log_extract;
      break;
  }
}

/*! \brief              Update the query windows with Q1 and QN matches for a particular call
    \param  callsign    callsign against which to generate the query matches

    Q1 = each question mark represents a single character
    QN = each question mark represents more than one character
*/
//void update_query_windows(const string& callsign)
void update_query_windows(const string_view callsign)
{ if (win_query_1 or win_query_n)
  { const auto [ q_1_matches, q_n_matches ] { query_db[callsign] };

    update_matches_window(q_1_matches, query_1_matches, win_query_1, callsign);
    update_matches_window(q_n_matches, query_n_matches, win_query_n, callsign);
  }
}

/*! \brief          Rebuild the dynamic SCP, fuzzy and query databases
    \param  logbk   the logbook to be used to rebuild the databases
*/
void rebuild_dynamic_call_databases(const logbook& logbk)
{ scp_dynamic_db.clear();             // clears cache of parent
  fuzzy_dynamic_db.clear();
  query_db.clear_dynamic_database();

  for ( const string& callsign : logbk.calls() )
  { if (!scp_db.contains(callsign) and !scp_dynamic_db.contains(callsign))
      scp_dynamic_db += callsign;

    if (!fuzzy_db.contains(callsign) and !fuzzy_dynamic_db.contains(callsign))
      fuzzy_dynamic_db += callsign;

    query_db += callsign;
  }
}

/*! \brief              Update the POSTED BY window
    \param  post_vec    the posts to be added to the window
*/
void update_win_posted_by(const vector<dx_post>& post_vec)
{ if (post_vec.empty() or !win_posted_by.valid())
    return;

  auto win_height { win_posted_by.height() };

  vector<string> new_contents;

  for ( int n { static_cast<int>(post_vec.size()) - 1 }; n >= 0; --n )
  { const dx_post& post { post_vec[n] };

// use the current time rather than relying on any timestamp in the data -- 
// this should be roughly the same time as the timestamp we've put in the dx_post
    new_contents += substring <std::string> (hhmmss(), 0, 5) + " "s + post.frequency_str() + "  "s + post.poster();
  }

  if (static_cast<int>(new_contents.size()) < win_height)
    new_contents += win_posted_by.snapshot();

  win_posted_by < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

  int y { win_height - 1 };

  for (size_t n { 0 }; ( (n < new_contents.size()) and (y >= 0) ); ++n)
    win_posted_by < cursor(0, y--) < new_contents.at(n);

  win_posted_by.refresh();
}

/*! \brief      Get all the calls in a DO NOT SHOW file
    \param  b   the band (ALL_BANDS or a single band)
    \return     the calls in the DO NOT SHOW file for band <i>b</i>
*/
STRING_SET calls_from_do_not_show_file(const BAND b)
{ const string filename_suffix { (b == ALL_BANDS) ? EMPTY_STR : "-"s + BAND_NAME[b] };
  const string filename        { context.do_not_show_filename() + filename_suffix };

  STRING_SET rv;

  try
  { FOR_ALL( remove_peripheral_spaces <std::string_view> (to_lines <std::string_view> (to_upper(read_file(context_path, filename)))), [&rv] (const auto& callsign) { rv += callsign; } );
  }

  catch (...)     // not an error if a do-not-show file does not exist
  { }

  return rv;
}

/*! \brief              Write a set of calls to a DO NOT SHOW file, overwriting the file
    \param  callsigns   the calls to be written
    \param  b           the band (ALL_BANDS or a single band)

    The file is written in the current directory
*/

void calls_to_do_not_show_file(const STRING_SET& callsigns, const BAND b)
{ if (callsigns.empty())
    return;

  const CALL_SET output_set      { SR::to<CALL_SET>(callsigns) };
  const string   filename_suffix { (b == ALL_BANDS) ? string { } : "-"s + BAND_NAME[b] };
  const string   filename        { context.do_not_show_filename() + filename_suffix };

//  CALL_SET output_set;    // define the ordering to be callsign order

//  FOR_ALL(callsigns, [&output_set] (const string& callsign) { output_set += callsign; });

  ofstream outfile(filename);

  FOR_ALL(output_set, [&outfile] (const auto& callsign) { outfile << callsign << endl; });
}

/*! \brief          Obtain the char used to represent a leading zero in a serial number
    \param  long_t  the value of the length of the T
    \return         the char to represent a leading zer
*/
char t_char(const unsigned short long_t)
{ constexpr char LONG_T_CHAR       { 23 };                      // character number that represents a long T (125%) -- see cw_buffer.cpp... sends a LONG_DAH
  constexpr char LONG_LONG_T_CHAR  { 24 };                      // character that represents a long long T (150%) -- see cw_buffer.cpp
  constexpr char EXTRA_LONG_T_CHAR { 25 };                      // character that represents a double T (175%) -- see cw_buffer.cpp

  switch (long_t)
  { case 1 :
      return LONG_T_CHAR;

    case 2 :
      return LONG_LONG_T_CHAR;

    case 3 :
      return EXTRA_LONG_T_CHAR;

    default :
      return 'T';
  }
}

// temporary
void update_bandmap_window(bandmap& bm /*, const int bm_version */)
{ //const bool proceed { ( bm_version == 0) or (bm.version() == bm_version) };

//  if (proceed)
  { const int highlight_colour { static_cast<int>(colours.add(COLOUR_YELLOW, COLOUR_WHITE)) };             // colour that will mark that we are processing an update
    const int original_colour  { static_cast<int>(colours.add(win_bandmap_filter.fg(), win_bandmap_filter.bg())) };

    const string win_contents { win_bandmap_filter.read() };
    const char   first_char   { (win_contents.empty() ? ' ' : win_contents[0]) };

// mark that we are processing on the screen
    win_bandmap_filter < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < colour_pair(highlight_colour) < first_char <= colour_pair(original_colour);

    win_bandmap <= bm;

// clear the mark that we are processing
    win_bandmap_filter < WINDOW_ATTRIBUTES::CURSOR_START_OF_LINE < WINDOW_ATTRIBUTES::WINDOW_CLEAR <= win_contents;
  }
//  else
//  { ost << "asked to write bm version " << bm_version << " but actual version is: " << bm.version() << endl;
//  }
}

/*! \brief                              Is a particular frequency within any marked range?
    \param  marked_frequency_ranges     the ranges to be marked, per mode
    \param  m                           mode
    \param  f                           frequency to test
    \return                             whether <i>f</i> is in any marked range for the mode <i>m</i>
*/
bool is_marked_frequency(const map<MODE, vector<pair<frequency, frequency>>>& marked_frequency_ranges, const MODE m, const frequency f)
{ try
  { for ( const auto& [ low_f, high_f ] : marked_frequency_ranges.at(m) )
      if ( (f >= low_f) and (f <= high_f) )
        return true;

    return false;
  }

  catch (...)
  { return false;
  }
}

/*! \brief              What exchange do we expect to receive from a particular callsign?
    \param  callsign    target callsign
    \return             the exchange value we expect to receive from <i>callsign</i>

    Returns empty string if nothing sensible can be said
*/
string expected_received_exchange(const string_view callsign)
{ const string                 canonical_prefix  { location_db.canonical_prefix(callsign) };
  const vector<exchange_field> expected_exchange { rules.unexpanded_exch(canonical_prefix, current_mode) };

  for (const auto& exf : expected_exchange)
  {
// if it's a choice, try to figure out which one to display; in IARU, it's the zone unless the society isn't empty;
// need to figure out a way to generalise all this
    if (exf.is_choice())
    { if (exf.name() == "ITUZONE+SOCIETY"sv)
      { string iaru_guess { exchange_db.guess_value(callsign, "SOCIETY"s) };      // start with guessing it's a society

        if (iaru_guess.empty())
          iaru_guess = exchange_db.guess_value(callsign, "ITUZONE"s);   // try ITU zone if no society

        return iaru_guess;
      }

      if (exf.name() == "10MSTATE+SERNO"sv)
      { static const STRING_SET state_multiplier_countries { "K"s, "VE"s, "XE"s };

        const string canonical_prefix { location_db.canonical_prefix(callsign) };
        const string state_guess      { state_multiplier_countries.contains(canonical_prefix) ? exchange_db.guess_value(callsign, "10MSTATE"s) : string { } };

        return state_guess;
      }
    }

    if (exf.name() == "DOK"sv)
    { const string guess { exchange_db.guess_value(callsign, "DOK"s) };

      return guess;
    }

    if (!no_default_rst and (exf.name() == "RST"sv) and !exf.is_optional())
      continue;

    if (exf.name() == "RS"sv)
      continue;

    if ((exf.name() == "GRID"sv))
    { const string guess { exchange_db.guess_value(callsign, "GRID"s) };

      return guess;
    }

    { if (!variable_exchange_fields.contains(exf.name()))    // if not a variable field (i.e., currently, not SERNO)
      { if (const string guess { rules.canonical_value(exf.name(), exchange_db.guess_value(callsign, exf.name())) }; !guess.empty())
          return guess;
      }
    }
  }

  return  string { };
}

/*! \brief          Execute pings and update PING window
    \param  win     the window to update
    \param  table   vector of addresses/FQDNs and labels to ping

    Expects to be run as a distinct thread, although nothing bad happens if this
    expectation is not met.
*/
void update_pings(window& win, PING_TABLE& table)
{ constexpr int FG_COLOUR     { COLOUR_WHITE };
  constexpr int NOPING_COLOUR { COLOUR_RED };

  static const int PING_COLOUR { string_to_colour("COLOUR_28"sv) };

  { start_of_thread("pings"s);

    unsigned int y { static_cast<unsigned int>(win.height() - 1) };

//    for (const auto& [label, socket_p] : ping_table_p)
    for (const auto& [label, socket_p] : table)
    { const string line_string { centred_string(label, win.width()) };
//      ost << "About to ping: " << label << endl;

      const bool   success     { socket_p -> ping() };
//      ost << "ping status: " << boolalpha << success << endl;

      win.move_cursor(0, y--);

      win < colour_pair(colours.add(FG_COLOUR, success ? PING_COLOUR : NOPING_COLOUR));
      win <= line_string;
    }
  }

  end_of_thread("pings"s);
}

/// zoom P3, turn off RIT, turn on XIT
bool zoomed_xit(void)
{ p3_span(p3_span_cq);
  rig.disable_rit();
  rig.enable_xit();

  return true;
}
