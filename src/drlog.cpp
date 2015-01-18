// $Id: drlog.cpp 91 2015-01-17 18:18:31Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html


// Principal author: N7DR

/*!     \file drlog.cpp

        The main program for drlog
 */

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
#include "keyboard.h"
#include "log.h"
#include "log_message.h"
#include "parallel_port.h"
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
#include "trlog.h"
#include "version.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include <cstdlib>

#include <png++/png.hpp>

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   placeholders;  // std::placeholders
using namespace   this_thread;   // std::this_thread

extern const set<string> CONTINENT_SET;     ///< two-letter abbreviations of continents

enum DRLOG_MODE { CQ_MODE = 0,              ///< I'm calling the other station
                  SAP_MODE                  ///< the other station is calling me
                };

string VERSION;         ///< version string
string DP("·");         ///< character for decimal point
string TS(",");         ///< character for thousands separator

static const set<string> variable_exchange_fields { "SERNO" };  ///< mutable exchange fields

const bool DISPLAY_EXTRACT = true,
           DO_NOT_DISPLAY_EXTRACT = !DISPLAY_EXTRACT;

// some forward declarations; others, that depend on these, occur later
const string active_window_name(void);      ///< Return the name of the active window in printable form
void add_qso(const QSO& qso);               ///< Add a QSO into the all the objects that need to know about it
void alert(const string& msg);              ///< Alert the user
void allow_for_callsign_mults(QSO& qso);    ///< Add info to QSO if callsign mults are in use; may change qso
void archive_data(void);                    ///< Send data to the archive file

const string bearing(const string& callsign);   ///< Return the bearing to a station

const bool calculate_exchange_mults(QSO& qso, const contest_rules& rules);                  ///< Populate QSO with correct exchange mults
const string callsign_mult_value(const string& callsign_mult_name, const string& callsign); ///< Obtain value corresponding to a type of callsign mult from a callsign
void cw_speed(const unsigned int new_speed);                                                ///< Set speed of computer keyer

void debug_dump(void);                                                                      ///< Dump useful information to disk
void display_band_mode(window& win, const BAND current_band, const enum MODE current_mode); ///< Display band and mode
void display_call_info(const string& callsign, const bool display_extract = true);          ///< Update several call-related windows
void display_nearby_callsign(const string& callsign);                                       ///< Display a callsign in the NEARBY window, in the correct colour
void display_statistics(const string& summary_str);                                         ///< Display the current statistics
const string dump_screen(const string& filename = string());                                ///< Dump a screen image to PNG file

void enter_cq_mode(void);                           ///< Enter CQ mode
void enter_sap_mode(void);                          ///< Enter SAP mode
void exit_drlog(void);                              ///< Cleanup and exit
const string expand_cw_message(const string& msg);  ///< Expand a CW message, replacing special characters

const string hhmmss(void);                          ///< Obtain the current time in HHMMSS format

const string match_callsign(const vector<pair<string /* callsign */,
                            int /* colour pair number */ > >& matches);   ///< Get best fuzzy or SCP match

void populate_win_info(const string& str);      ///< Populate the information window
void p3_screenshot(void);                       ///< Start a thread to take a snapshot of a P3

void rebuild_history(const logbook& logbk,
                     const contest_rules& rules,
                     running_statistics& statistics,
                     call_history& q_history,
                     rate_meter& rate);             ///< Rebuild the history (and statistics and rate), using the logbook
void rescore(const contest_rules& rules);           ///< Rescore the entire contest
void restore_data(const string& archive_filename);  ///< Extract the data from the archive file
void rit_control(const keyboard_event& e);          ///< Control RIT using the SHIFT keys
void rig_error_alert(const string& msg);            ///< Alert the user to a rig-related error

void start_of_thread(void);                             ///< Increase the counter for the number of running threads
const string sunrise_or_sunset(const string& callsign, const bool calc_sunset); ///< Calculate the sunrise or sunset time for a station
void swap_rit_xit(void);                                                        ///< Swap the states of RIT and XIT

void test_exchange_templates(const contest_rules&, const string& test_filename);    ///< Debug exchange templates
void toggle_drlog_mode(void);                                                       ///< Toggle between CQ mode and SAP mode

void update_batch_messages_window(const string& callsign = string());       ///< Update the batch_messages window with the message (if any) associated with a call
//void update_fuzzy_window(const string& callsign);
void update_individual_messages_window(const string& callsign = string());  ///< Update the individual_messages window with the message (if any) associated with a call
void update_known_callsign_mults(const string& callsign);                   ///< Possibly add a new callsign mult
void update_known_country_mults(const string& callsign);                    ///< Possibly add a new country to the known country mults
void update_local_time(void);                                               ///< Write the current local time to <i>win_local_time</i>
void update_mult_value(void);                                               ///< Calculate the value of a mult and update <i>win_mult_value</i>
void update_rate_window(void);                                              ///< Update the QSO and score values in <i>win_rate</i>
//void update_scp_window(const string& callsign);

// functions for processing input to windows
void process_CALL_input(window* wp, const keyboard_event& e);               ///< Process an event in CALL window
void process_EXCHANGE_input(window* wp, const keyboard_event& e);           ///< Process an event in EXCHANGE window
void process_LOG_input(window* wp, const keyboard_event& e);                ///< Process an event in LOG window
void process_QTC_input(window* wp, const keyboard_event& e);                ///< Process an event in QTC window

// thread functions
void* auto_backup(void* vp);                                                ///< Copy a file to a backup directory
void* auto_screenshot(void* vp);                                            ///< Write a screenshot to a file
void* display_rig_status(void* vp);                                         ///< Thread function to display status of the rig
void* display_date_and_time(void* vp);                                      ///< Thread function to display the date and time
void* get_cluster_info(void* vp);                                           ///< Thread function to obtain data from the cluster
void* keyboard_test(void* vp);                                              ///< Thread function to simulate keystrokes
void* prune_bandmap(void* vp);                                              ///< Thread function to prune the bandmaps once per minute
void* process_rbn_info(void* vp);                                           ///< Thread function to process data from the cluster or the RBN
void* p3_screenshot_thread(void* vp);                                       ///< Thread function to generate a screenshot of a P3 and store it in a BMP file
void* reset_connection(void* vp);                                           ///< Thread function to reset the RBN or cluster connection
void* simulator_thread(void* vp);                                           ///< Thread function to simulate a contest from an extant log
void* spawn_dx_cluster(void*);                                              ///< Thread function to spawn the cluster
void* spawn_rbn(void*);                                                     ///< Thread function to spawn the RBN

// functions that include thread safety
const BAND safe_get_band(void);                             ///< get value of <i>current_band</i>
void safe_set_band(const BAND b);                           ///< set value of <i>current_band</i>
const MODE safe_get_mode(void);                             ///< get value of <i>current_mode</i>
void safe_set_mode(const MODE m);                           ///< set value of <i>current_mode</i>

// more forward declarations (dependent on earlier ones)
const bool is_needed_qso(const string& callsign, const BAND b, const MODE m);

//void update_remaining_callsign_mults_window(running_statistics&, const string& mult_name = string(), const BAND b = safe_get_band());
void update_remaining_callsign_mults_window(running_statistics&, const string& mult_name, const BAND b, const MODE m);

//inline void update_remaining_callsign_mults_window(running_statistics& statistics, const BAND b, const MODE m)
//  { update_remaining_callsign_mults_window(statistics, string(), b, m);}

//void update_remaining_country_mults_window(running_statistics&, const BAND b = safe_get_band(), const MODE m = safe_get_mode());
void update_remaining_country_mults_window(running_statistics&, const BAND b, const MODE m);

void update_remaining_exch_mults_window(const string&,
                                        const contest_rules&,
                                        running_statistics&,
                                        const BAND b = safe_get_band(),
                                        const MODE m = safe_get_mode());
void update_remaining_exchange_mults_windows(const contest_rules&,
                                             running_statistics&,
                                             const BAND b = safe_get_band(),
                                             const MODE m = safe_get_mode());

string last_call_inserted_with_space;  // probably should be per band
pt_mutex dupe_check_mutex;

// values that are used by multiple threads
// mostly these are essentially RO, so locking is overkill; but we do it anyway,
// otherwise Murphy dictates that we'll hit a race condition at the worst possible time

pt_mutex alert_mutex;
time_t   alert_time = 0;

pt_mutex  cq_mode_frequency_mutex;
frequency cq_mode_frequency;

pt_mutex  last_exchange_mutex;
string    last_exchange;

pt_mutex            thread_check_mutex;                       ///< both the following variables are under this mutex
int                 n_running_threads = 0;
bool                exiting = false;

pt_mutex            current_band_mutex;                       ///< mutex for setting/getting the current band
BAND                current_band;                             ///< the current band

pt_mutex            current_mode_mutex;                       ///< mutex for setting/getting the current mode
MODE                current_mode;                             ///< the current mode

exchange_field_database exchange_db;                          ///< dynamic database of exchange field values for calls; automatically thread-safe

string              my_continent;                             ///< what continent am I on?

running_statistics  statistics;                               ///< all the QSO statistics to date

pt_mutex            drlog_mode_mutex;                         ///< mutex for accessing the drlog_mode
DRLOG_MODE          drlog_mode = SAP_MODE;                    ///< CQ_MODE or SAP_MODE

// callsign mults we know about in AUTO mode
pt_mutex            known_callsign_mults_mutex;
set<string>         known_callsign_mults;

unsigned int        next_qso_number = 1;                      ///< actual number of next QSO
unsigned int        n_modes = 0;                              ///< number of modes allowed in the contest
unsigned int        octothorpe = 1;                           ///< serial number of next QSO
string              at_call;                                  ///< call that should replace comat in "call ok now" message

vector<pair<string, string> > sent_exchange;    //name/value pairs for the sent exchange

logbook             logbk;                      // can't be called log if mathcalls.h is in the compilation path

drlog_context       context;

bool                filter_remaining_country_mults(false);
bool                restored_data(false);                           ///< did we restore from an archive?

pt_mutex            individual_messages_mutex;
map<string, string> individual_messages;

pt_mutex            batch_messages_mutex;
map<string, string> batch_messages;

cached_data<string, string> WPX_DB(&wpx_prefix);

qtc_database qtc_db;    ///< sent QTCs
qtc_buffer   qtc_buf;   ///< all sent and unsent QTCs
bool send_qtcs = false;  // whether QTCs are used; set from rules later

EFT CALLSIGN_EFT("CALLSIGN");

/* The K3's handling of commands from the computer is rubbish. This variable
   is a simple way to cease polling when we are moving RIT with the shift keys,
   because there's a pause in the RIT adjustment if we happen to poll the
   rig while adjusting RIT
*/
bool ok_to_poll_k3 = true;

// windows -- these should automatically be thread_safe
window win_band_mode,               ///< the band and mode indicator
       win_bandmap,                 ///< the bandmap for the current band
       win_bandmap_filter,          ///< bandmap filter information
       win_batch_messages,          ///< messages from the batch messages file
       win_call,                    ///< callsign of other station, or command
       win_cluster_line,            ///< last line received from cluster
       win_cluster_mult,            ///< mults received from cluster
       win_cluster_screen,          ///< interactive screen on to the cluster
       win_date,                    ///< the date
       win_drlog_mode,              ///< indicate whether in CQ or SAP mode
       win_exchange,                ///< QSO exchange received from other station
       win_log_extract,             ///< to show earlier QSOs
       win_fuzzy,                   ///< fuzzy lookups
       win_individual_messages,     ///< messages from the individual messages file
       win_individual_qtc_count,    ///< number of QTCs sent to an individual
       win_info,                    ///< summary of info about current station being worked
       win_local_time,              ///< window for local time
       win_log,                     ///< main visible log
       win_message,                 ///< messages from drlog to the user
       win_mult_value,              ///< value of a mult
       win_nearby,                  ///< nearby station
       win_qso_number,              ///< number of the next QSO
       win_qtc_status,              ///< status of QTCs
       win_rate,                    ///< QSO and point rates
       win_rbn_line,                ///< last line received from the RBN
       win_remaining_callsign_mults, ///< the remaining callsign mults
       win_remaining_country_mults, ///< the remaining country mults
       win_rig,                     ///< rig status
       win_score,                   ///< total number of points
       win_score_bands,             ///< which bands contribute to score
       win_score_modes,             ///< which modes contribute to score
       win_scp,                     ///< SCP lookups
       win_scratchpad,              ///< scratchpad
       win_serial_number,           ///< next serial number (octothorpe)
       win_srss,                    ///< my sunrise/sunset
       win_summary,                 ///< overview of score
       win_time,                    ///< current UTC
       win_title,                   ///< title of the contest
       win_wpm;                     ///< CW speed in WPM

map<string /* name */, window*> win_remaining_exch_mults_p;

vector<pair<string /* contents*/, window*> > static_windows_p;  ///< static windows

// the visible bits of logs
log_extract editable_log(win_log);
log_extract extract(win_log_extract);

// some windows are accessed from multiple threads
pt_mutex band_mode_mutex;
pt_mutex bandmap_mutex;

cw_messages cwm;

contest_rules rules;
cw_buffer*       cw_p  = nullptr;
drmaster*       drm_p  = nullptr;
dx_cluster* cluster_p = nullptr;
dx_cluster*     rbn_p = nullptr;

location_database location_db;
rig_interface rig;

thread_attribute attr_detached(PTHREAD_DETACHED);

window* win_active_p = &win_call;             // start with the CALL window active
window* last_active_win_p = nullptr;

const string OUTPUT_FILENAME("output.txt");
message_stream ost(OUTPUT_FILENAME);

typedef array<bandmap, NUMBER_OF_BANDS>  BM_ARRAY;

// create one bandmap per band
BM_ARRAY  bandmaps;

// history of calls worked
call_history q_history;

rate_meter rate;

extern cpair colours;

vector<string> win_log_snapshot;

scp_database  scp_db,
              scp_dynamic_db;
scp_databases scp_dbs;

// SCP matches colour pair = COLOUR_GREEN => worked on a different band; OK to work on this band
vector<pair<string /* callsign */, int /* colour pair number */ > > scp_matches;
vector<pair<string /* callsign */, int /* colour pair number */ > > fuzzy_matches;

fuzzy_database  fuzzy_db,
                fuzzy_dynamic_db;
fuzzy_databases fuzzy_dbs;

// thread IDs
pthread_t thread_id_display_date_and_time,
          thread_id_rig_status;

/// define wrappers to pass parameters to threads

WRAPPER_7_NC(cluster_info, 
    window*, wclp,
    window*, wcmp,
    dx_cluster*, dcp,
    running_statistics*, statistics_p,
    location_database*, location_database_p,
    window*, win_bandmap_p,
    BM_ARRAY*, bandmaps_p);

WRAPPER_3_NC(big_cluster_info,
    drlog_context*, context_p,
    POSTING_SOURCE*, source_p,
    cluster_info*, info_p);

WRAPPER_2_NC(bandmap_info,
    window*, win_bandmap_p,
    BM_ARRAY*, bandmaps_p);

WRAPPER_2_NC(rig_status_info,
    unsigned int, poll_time,
    rig_interface*, rigp);

// prepare for terminal I/O
screen monitor;                            // declare at global scope solely so that its destructor is called when exit() is executed
keyboard_queue keyboard;

// quick access to whether particular types of mults are in use; these are written only once, so we don't bother to protect them
bool callsign_mults_used(false);
bool country_mults_used(false);
bool exchange_mults_used(false);

// update the SCP or fuzzy window and vector of matches
template <typename T>
void update_matches_window(const T& matches, vector<pair<string, int>>& match_vector, window& win, const string& callsign)
{ if (callsign.length() >= context.match_minimum())
  {
// put in right order and also get the colours right
    vector<string> vec_str;

    copy(matches.cbegin(), matches.cend(), back_inserter(vec_str));
    sort(vec_str.begin(), vec_str.end(), compare_calls);
    match_vector.clear();

// this is experimental, to see if I like it in a contest;
// I found it hard to see an exact match (for V6A) far down the list;
// also, putting it first ensures that it can't disappear off the end
// put an exact match at the front (this will never happen with a fuzzy match)
    vector<string> tmp_matches;

    for (const auto& cs : vec_str)
      if (cs == callsign)
        tmp_matches.push_back(cs);

    for (const auto& cs : vec_str)
      if (cs != callsign)
        tmp_matches.push_back(cs);

    for (const auto& cs : tmp_matches)
    { const bool qso_b4 = logbk.qso_b4(cs);
      const bool dupe = logbk.is_dupe(cs, safe_get_band(), safe_get_mode(), rules);
      int colour_pair_number = colours.add(win.fg(), win.bg());

      if (qso_b4)
        colour_pair_number = colours.add(COLOUR_GREEN, win.bg());

      if (dupe)
        colour_pair_number = colours.add(COLOUR_RED, win.bg());

      match_vector.push_back( { cs, colour_pair_number } );
    }

    win < WINDOW_CLEAR <= match_vector;
  }
  else
    win <= WINDOW_CLEAR;
}

// simple inline functions

/*! \brief      Convert a serial number to a string
    \param  n   serial number
    \return     <i>n</i> as a zero-padded string of three digits, or a four-digit string if <i>n</i> is greater than 999
*/
inline const string serial_number_string(const unsigned int n)
  { return ( (n < 1000) ? pad_string(to_string(n), 3, PAD_LEFT, '0') : to_string(n) ); }

/*! \brief              Calculate the sunrise time for a station
    \param  callsign    call of the station for which sunset is desired
    \return             sunrise in the form HHMM

    Returns "9999" if it's always dark, and "8888" if it's always light
 */
inline const string sunrise(const string& callsign)
  { return sunrise_or_sunset(callsign, false); }

/*! \brief              Calculate the sunset time for a station
    \param  callsign    call of the station for which sunset is desired
    \return             sunset in the form HHMM

    Returns "9999" if it's always dark, and "8888" if it's always light
 */
inline const string sunset(const string& callsign)
  { return sunrise_or_sunset(callsign, true); }

/*! \brief              Update the fuzzy window with matches for a particular call
    \param  callsign    callsign against which to generate the fuzzy matches
*/
inline void update_fuzzy_window(const string& callsign)
  { update_matches_window(fuzzy_dbs[callsign], fuzzy_matches, win_fuzzy, callsign); }

/*! \brief              Update the SCP window with matches for a particular call
    \param  callsign    callsign against which to generate the SCP matches
*/
inline void update_scp_window(const string& callsign)
  { update_matches_window(scp_dbs[callsign], scp_matches, win_scp, callsign); }

int main(int argc, char** argv)
{ try
  { const map<string, string> MONTH_NAME_TO_NUMBER( { { "Jan", "01" },
                                                      { "Feb", "02" },
                                                      { "Mar", "03" },
                                                      { "Apr", "04" },
                                                      { "May", "05" },
                                                      { "Jun", "06" },
                                                      { "Jul", "07" },
                                                      { "Aug", "08" },
                                                      { "Sep", "09" },
                                                      { "Oct", "10" },
                                                      { "Nov", "11" },
                                                      { "Dec", "12" }
                                                    } );

    const string date_str = DATE_STR.substr(DATE_STR.length() - 4) + "-" + MONTH_NAME_TO_NUMBER.at(DATE_STR.substr(0, 3)) + "-" +
                            (DATE_STR[4] == ' ' ? string("0") + DATE_STR.substr(5, 1) : DATE_STR.substr(4, 2));

    VERSION = VERSION_TYPE + string(" ") + date_str + " " + TIME_STR.substr(0, 5);

    ost << "Running drlog version " << VERSION << endl;
  }

  catch (...)
  { ost << "Error: Unable to generate drlog version information" << endl;
    VERSION = string("Unknown version ") + VERSION;  // because VERSION may be used elsewhere
  }

  command_line cl(argc, argv);
  const string config_filename = (cl.value_present("-c") ? cl.value("-c") : "logcfg.dat");

  try    // put it all in one big try block
  {
// read configuration data (typically from logcfg.dat)
    drlog_context* context_p = nullptr;

    try
    { context_p = new drlog_context(config_filename);
    }

    catch (...)
    { cerr << "Error reading configuration data" << endl;
      exit(-1);
    }

// make the context available globally and cleanup the context pointer
    context = *context_p;
    delete context_p;

    DP = context.decimal_point();
    TS = context.thousands_separator();

// read the country data
    cty_data* country_data_p = nullptr;

    try
    { country_data_p = new cty_data(context.path(), context.cty_filename());
    }

    catch (...)
    { cerr << "Error reading country data: does the file " << context.cty_filename() << " exist?" << endl;
      exit(-1);
    }

    const cty_data& country_data = *country_data_p;

// make some things available file-wide
    my_continent = context.my_continent();

// read drmaster database
    try
    { drm_p = new drmaster(context.path(), context.drmaster_filename());
    }

    catch (...)
    { cerr << "Error reading drmaster database file " << context.drmaster_filename() << endl;
      exit(-1);
    }

    const drmaster& drm = *drm_p;

// read (optional) secondary QTH database
    drlog_qth_database* qth_db_p = nullptr;

    try
    { qth_db_p = new drlog_qth_database();
    }

    catch (...)
    { cerr << "Error reading secondary QTH file" << endl;
      exit(-1);
    }

    const drlog_qth_database& qth_db = *qth_db_p;

// location database
    try
    { location_db.prepare(country_data, context.country_list(), qth_db);
    }

    catch (...)
    { cerr << "Error generating location database" << endl;
      exit(-1);
    }

    location_db.add_russian_database(context.path(), context.russian_filename());

// make callsign parser available now that we can create it
    CALLSIGN_EFT = EFT(CALLSIGN_EFT.name(), context.path(), context.exchange_fields_filename(), context, location_db);

// build super check partial database from the drmaster information
    try
    { scp_db.init_from_calls(drm.calls());
    }

    catch (...)
    { cerr << "Error initialising scp database" << endl;
      exit(-1);
    }

    scp_dbs += scp_db;            // incorporate into multiple-database version
    scp_dbs += scp_dynamic_db;    // add the (empty) dynamic SCP database

// build fuzzy database from the drmaster information
    try
    { fuzzy_db.init_from_calls(drm.calls());
    }

    catch (...)
    { cerr << "Error generating fuzzy database" << endl;
      exit(-1);
    }

    fuzzy_dbs += fuzzy_db;            // incorporate into multiple-database version
    fuzzy_dbs += fuzzy_dynamic_db;    // add the (empty) dynamic fuzzy database

// define the rules for this contest
    try
    { rules.prepare(context, location_db);
    }

    catch (...)
    { cerr << "Error generating rules" << endl;
      exit(-1);
    }

    send_qtcs = rules.send_qtcs();    // grab it once
    n_modes = rules.n_modes();        // grab this once too

// define types of mults that are in use; after this point these should be treated as read-only
    callsign_mults_used = rules.callsign_mults_used();
    country_mults_used = rules.country_mults_used();
    exchange_mults_used = rules.exchange_mults_used();

// possibly test regex exchanges; this will exit if it executes
#if defined(NEW_CONSTRUCTOR)
    if (cl.value_present("-test-exchanges"))
      test_exchange_templates(rules, cl.value("-test-exchanges"));
#else
    if (cl.value_present("-test-exchanges"))
      test_exchange_templates(cl.value("-test-exchanges"));
#endif    // NEW_CONSTRUCTOR

// real-time statistics
    try
    { statistics.prepare(country_data, context, rules);
    }

    catch (...)
    { cerr << "Error generating real-time statistics" << endl;
      exit(-1);
    }

// MESSAGE window (do this as early as is reasonable so that it's available for messages)
    win_message.init(context.window_info("MESSAGE"), WINDOW_NO_CURSOR);
    win_message < WINDOW_BOLD <= "";                                       // use bold in this window

// possibly open communication with the rig
    rig.register_error_alert_function(rig_error_alert);
    if (!context.rig1_port().empty() and !context.rig1_type().empty())
      rig.prepare(context);

// possibly put rig into TEST mode
    if (context.test())
      rig.test(true);

// possibly set up CW buffer
    if (contains(to_upper(context.modes()), "CW") and !context.keyer_port().empty())
    { const string cw_port = context.keyer_port();
      const unsigned int ptt_delay = context.ptt_delay();
      const unsigned int cw_speed = context.cw_speed();

      try
      { cw_p = new cw_buffer(cw_port, ptt_delay, cw_speed);
      }

      catch (const parallel_port_error& e)
      { ost << "Failed to open CW port: ";
        ost << e.reason() << endl;
        exit(-1);
      }

      if (rig.valid())
        cw_p->associate_rig(&rig);
      cwm = cw_messages(context.messages());
    }

// set the initial band and mode from the configuration file
    safe_set_band(context.start_band());
    safe_set_mode(context.start_mode());

// see if the rig is on the right band and mode (as defined in the configuration file), and if not then move it
    { const frequency rf = rig.rig_frequency();
      const MODE rm = rig.rig_mode();
      const bool mode_matches = ((current_mode == MODE_CW and rm == MODE_CW ) or
                                 (current_mode == MODE_SSB and (rm == MODE_SSB )));
      const bool band_matches = (current_band == static_cast<BAND>(rf));

      if (!band_matches or !mode_matches)
      { ost << "mismatch; setting frequency" << endl;

        rig.rig_frequency(DEFAULT_FREQUENCIES[ { current_band, current_mode } ]);

        if (!mode_matches)
          rig.rig_mode(current_mode);
      }
    }

// configure bandmaps so user's call does not display
  { const string my_call = context.my_call();

//    for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm.do_not_add(my_call); } );
    FOR_ALL(bandmaps, [=] (bandmap& bm) { bm.do_not_add(my_call); } );
  }

  for (const auto& callsign : context.do_not_show())
//    for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm.do_not_add(callsign); } );
    FOR_ALL(bandmaps, [=] (bandmap& bm) { bm.do_not_add(callsign); } );

  if (!context.do_not_show_filename().empty())
  { try
    { const vector<string> lines = remove_peripheral_spaces(to_lines(to_upper(read_file(context.path(), context.do_not_show_filename()))));

      for (const auto& callsign : lines)
//        for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm.do_not_add(callsign); } );
      FOR_ALL(bandmaps, [=] (bandmap& bm) { bm.do_not_add(callsign); } );
    }

    catch (...)
    { cerr << "Unable to read do-not-show file: " << context.do_not_show_filename() << endl;
      exit(-1);
    }
  }

// set the RBN threshold for each bandmap
  { const unsigned int rbn_threshold = context.rbn_threshold();

    if (rbn_threshold != 1)        // 1 is the default in a pristine bandmap, so may be no need to change
//      for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm.rbn_threshold(rbn_threshold); } );
      FOR_ALL(bandmaps, [=] (bandmap& bm) { bm.rbn_threshold(rbn_threshold); } );
  }

// create and populate windows; do static windows first
  const map<string /* name */, pair<string /* contents */, vector<window_information> > >& swindows = context.static_windows();;

  for (const auto& this_static_window : swindows)
  { const string& contents = this_static_window.second.first;
    const vector<window_information>& vec_win_info = this_static_window.second.second;

    for (const auto& winfo : vec_win_info)
    { window* window_p = new window();

      window_p -> init(winfo);
      static_windows_p.push_back( { contents, window_p } );
    }
  }

  for (auto& swin : static_windows_p)
    *(swin.second) <= swin.first;

// BAND/MODE window
  win_band_mode.init(context.window_info("BAND/MODE"), WINDOW_NO_CURSOR);

// BATCH MESSAGES window
  win_batch_messages.init(context.window_info("BATCH MESSAGES"), WINDOW_NO_CURSOR);

  if (!context.batch_messages_file().empty())
  { try
    { const string all_messages = read_file(context.path(), context.batch_messages_file());
      const vector<string> messages = to_lines(all_messages);

      SAFELOCK(batch_messages);

      string current_message;

      for (const auto& messages_line : messages)
      { if (!messages_line.empty())
        { if (contains(messages_line, "["))
            current_message = substring(messages_line, 1, messages_line.length() - 2);
          else
          { const string callsign = remove_peripheral_spaces(messages_line);

            batch_messages.insert( { callsign, current_message } );
          }
        }
      }

      ost << "read " << batch_messages.size() << " batch messages" << endl;
    }

    catch (...)
    { cerr << "Unable to read batch messages file: " << context.batch_messages_file() << endl;
      exit(-1);
    }
  }

// CALL window
  win_call.init(context.window_info("CALL"), COLOUR_YELLOW, COLOUR_MAGENTA, WINDOW_INSERT);
  win_call < WINDOW_BOLD <= "";
  win_call.process_input_function(process_CALL_input);

// CLUSTER LINE window
  win_cluster_line.init(context.window_info("CLUSTER LINE"), WINDOW_NO_CURSOR);

// DATE window
  win_date.init(context.window_info("DATE"), WINDOW_NO_CURSOR);

// DRLOG MODE window
  win_drlog_mode.init(context.window_info("DRLOG MODE"), COLOUR_WHITE, COLOUR_BLACK, WINDOW_NO_CURSOR);

// EXCHANGE window
  win_exchange.init(context.window_info("EXCHANGE"), COLOUR_YELLOW, COLOUR_MAGENTA, WINDOW_INSERT);
  win_exchange <= WINDOW_BOLD;
  win_exchange.process_input_function(process_EXCHANGE_input);

// FUZZY window
  win_fuzzy.init(context.window_info("FUZZY"), WINDOW_NO_CURSOR);

// INDIVIDUAL MESSAGES window
  win_individual_messages.init(context.window_info("INDIVIDUAL MESSAGES"), WINDOW_NO_CURSOR);

  if (!context.individual_messages_file().empty())
  { try
    { const string all_messages = read_file(context.path(), context.individual_messages_file());
      const vector<string> messages = to_lines(all_messages);

      SAFELOCK(individual_messages);

      for (const auto& messages_line : messages)
      { vector<string> fields = split_string(messages_line, ":");

        if (!fields.empty())
        { const string callsign = fields[0];
          const size_t posn = messages_line.find(":");

          if (posn != messages_line.length() - 1)    // if the colon isn't the last character
          { const string message = remove_peripheral_spaces(substring(messages_line, posn + 1));

            individual_messages.insert( { callsign, message } );
          }
        }
      }
    }

    catch (...)
    { cerr << "Unable to read individual messages file: " << context.individual_messages_file() << endl;
      exit(-1);
    }
  }

// INDIVIDUAL QTC COUNT window
//  ost << "send_qtcs = " << send_qtcs << endl;
  if (send_qtcs)
  { win_individual_qtc_count.init(context.window_info("INDIVIDUAL QTC COUNT"), WINDOW_NO_CURSOR);
    win_individual_qtc_count <= WINDOW_CLEAR;
  }

// INFO window
  win_info.init(context.window_info("INFO"), WINDOW_NO_CURSOR);
  win_info <= WINDOW_CLEAR;                                        // make it visible

// LOCAL TIME window
  win_local_time.init(context.window_info("LOCAL TIME"), WINDOW_NO_CURSOR);

// LOG window
  win_log.init(context.window_info("LOG"), WINDOW_NO_CURSOR);
  win_log.enable_scrolling();
  win_log.process_input_function(process_LOG_input);

// LOG EXTRACT window; also used for QTCs
  win_log_extract.init(context.window_info("LOG EXTRACT"), WINDOW_NO_CURSOR);
  editable_log.prepare();    // now we can size the editable log
  extract.prepare();

  if (send_qtcs)
    win_log_extract.process_input_function(process_QTC_input);

// MULT VALUE window
  win_mult_value.init(context.window_info("MULT VALUE"), WINDOW_NO_CURSOR);
  update_mult_value();

// NEARBY window
  win_nearby.init(context.window_info("NEARBY"), WINDOW_NO_CURSOR);

// QSO NUMBER window
  win_qso_number.init(context.window_info("QSO NUMBER"), WINDOW_NO_CURSOR);
  win_qso_number <= pad_string(to_string(next_qso_number), win_qso_number.width());

// QTC STATUS window
  win_qtc_status.init(context.window_info("QTC STATUS"), WINDOW_NO_CURSOR);
  win_qtc_status <= "Last QTC: None";

// RATE window
  win_rate.init(context.window_info("RATE"), WINDOW_NO_CURSOR);
  update_rate_window();

// REMAINING CALLSIGN MULTS window
  win_remaining_callsign_mults.init(context.window_info("REMAINING CALLSIGN MULTS"), COLOUR_WHITE, COLOUR_BLUE, WINDOW_NO_CURSOR);
  if (restored_data)
    update_remaining_callsign_mults_window(statistics, "", safe_get_band(), safe_get_mode());
  else
    win_remaining_callsign_mults <= (context.remaining_callsign_mults_list());

// REMAINING COUNTRY MULTS window
  win_remaining_country_mults.init(context.window_info("REMAINING COUNTRY MULTS"), COLOUR_WHITE, COLOUR_BLUE, WINDOW_NO_CURSOR);
  if (restored_data)
    update_remaining_country_mults_window(statistics, safe_get_band(), safe_get_mode());
  else
  { const set<string> set_from_context = context.remaining_country_mults_list();
    const string& target_continent = *(set_from_context.cbegin());

    if ((set_from_context.size() == 1) and (CONTINENT_SET < target_continent))
      win_remaining_country_mults <= location_db.countries(target_continent);
    else
      win_remaining_country_mults <= (context.remaining_country_mults_list());
  }

// REMAINING EXCHANGE MULTS window(s)
  const vector<string> exchange_mult_window_names = context.window_name_contains("REMAINING EXCHANGE MULTS");

  for (auto& window_name : exchange_mult_window_names)
  { window* wp = new window();
    const string exchange_mult_name = substring(window_name, 25);

    wp->init(context.window_info(window_name), COLOUR_WHITE, COLOUR_BLUE, WINDOW_NO_CURSOR);
    win_remaining_exch_mults_p.insert( { exchange_mult_name, wp } );

    (*wp) <= rules.exch_canonical_values(exchange_mult_name);
  }

// RIG window (rig status)
  win_rig.init(context.window_info("RIG"), WINDOW_NO_CURSOR);

// SCORE window
  win_score.init(context.window_info("SCORE"), WINDOW_NO_CURSOR);
  { const string score_str = pad_string(separated_string(statistics.points(rules), TS), win_score.width() - string("Score: ").length());

    win_score < CURSOR_START_OF_LINE < "Score: " <= score_str;
  }

// SCORE BANDS window
  win_score_bands.init(context.window_info("SCORE BANDS"), WINDOW_NO_CURSOR);
  { const set<BAND> score_bands = rules.score_bands();
    string bands_str;

    for (const auto& b : score_bands)
      bands_str += (BAND_NAME[b] + " ");

    win_score_bands < CURSOR_START_OF_LINE < "Score Bands: " <= bands_str;
  }

// SCORE MODES window
  win_score_modes.init(context.window_info("SCORE MODES"), WINDOW_NO_CURSOR);
  { const set<MODE> score_modes = rules.score_modes();
    string modes_str;

    if (score_modes < MODE_CW)
      modes_str += "CW ";

    if (score_modes < MODE_SSB)
      modes_str += "SSB ";

    win_score_modes < CURSOR_START_OF_LINE < "Score Modes: " <= modes_str;
  }

// SCP window
  win_scp.init(context.window_info("SCP"), WINDOW_NO_CURSOR);

// SCRATCHPAD window
  win_scratchpad.init(context.window_info("SCRATCHPAD"), WINDOW_NO_CURSOR);
  win_scratchpad.enable_scrolling();

// SERIAL NUMBER window
  win_serial_number.init(context.window_info("SERIAL NUMBER"), WINDOW_NO_CURSOR);
  win_serial_number <= serial_number_string(octothorpe);

// SRSS window
  win_srss.init(context.window_info("SRSS"), WINDOW_NO_CURSOR);
  win_srss <= ( (string)"SR/SS: " + sunrise(context.my_latitude(), context.my_longitude()) + "/" + sunset(context.my_latitude(), context.my_longitude()) );

// SUMMARY window
  win_summary.init(context.window_info("SUMMARY"), COLOUR_WHITE, COLOUR_BLUE, WINDOW_NO_CURSOR);
//  win_summary < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
  display_statistics(statistics.summary_string(rules));

// TITLE window
  win_title.init(context.window_info("TITLE"), COLOUR_BLACK, COLOUR_GREEN, WINDOW_NO_CURSOR);
  win_title <= centre(context.contest_name(), 0);

// TIME window
  win_time.init(context.window_info("TIME"), COLOUR_WHITE, COLOUR_BLACK, WINDOW_NO_CURSOR);  // WHITE / BLACK are default anyway, so don't actually need them

// WPM window
  win_wpm.init(context.window_info("WPM"), WINDOW_NO_CURSOR);
  win_wpm <= to_string(context.cw_speed()) + " WPM";
  if (cw_p)
    cw_p->speed(context.cw_speed());

  try
  { if (context.sync_keyer())
      rig.keyer_speed(context.cw_speed());
  }

  catch (const rig_interface_error& e)
  { alert("Error setting CW speed on rig");
  }

// CW window
  //win_cw.init(context.window_info("CW BUFFER"), WINDOW_NO_CURSOR);
  //cw_buffer cw(win_cw);

  display_band_mode(win_band_mode, safe_get_band(), safe_get_mode());

// start to display the date and time
  try
  { create_thread(&thread_id_display_date_and_time, &(attr_detached.attr()), display_date_and_time, NULL, "date/time");
  }

  catch (const pthread_error& e)
  { ost << e.reason() << endl;
    exit(-1);
  }

// start to display the rig status (in the RIG window); also get rig frequency for bandmap
  rig_status_info rig_status_thread_parameters(1000 /* poll time */, &rig);

  try
  { create_thread(&thread_id_rig_status, &(attr_detached.attr()), display_rig_status, &rig_status_thread_parameters, "rig status");
  }

  catch (const pthread_error& e)
  { ost << e.reason() << endl;
    exit(-1);
  }

// CLUSTER MULT window
  win_cluster_mult.init(context.window_info("CLUSTER MULT"), WINDOW_NO_CURSOR);
  win_cluster_mult.enable_scrolling();

// CLUSTER SCREEN window
  win_cluster_screen.init(context.window_info("CLUSTER SCREEN"), WINDOW_NO_CURSOR);
  win_cluster_screen.enable_scrolling();

// RBN LINE window
  win_rbn_line.init(context.window_info("RBN LINE"), WINDOW_NO_CURSOR);

// BANDMAP window
  win_bandmap.init(context.window_info("BANDMAP"), WINDOW_NO_CURSOR);

  { const vector<int> fc = context.bandmap_fade_colours();

    for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm.fade_colours(fc); } );

    const int rc = context.bandmap_recent_colour();

    for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm.recent_colour(rc); } );
  }

// create thread to prune the bandmaps every minute
  static bandmap_info bandmap_info_for_thread(&win_bandmap, &bandmaps); // doesn't really need to be static

  { static pthread_t thread_id_4;

    try
    { create_thread(&thread_id_4, &(attr_detached.attr()), prune_bandmap, (void*)(&bandmap_info_for_thread), "prune bandmap");
    }

    catch (const pthread_error& e)
    { ost << e.reason() << endl;
      exit(-1);
    }
  }

// BANDMAP FILTER window
  win_bandmap_filter.init(context.window_info("BANDMAP FILTER"), WINDOW_NO_CURSOR);

// set up correct colours for bandmap filter window
  bool bandmap_filtering_enabled = context.bandmap_filter_enabled();

  if (!bandmap_filtering_enabled)                                                                          // disabled
    win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_disabled_colour());
  else
  { if (context.bandmap_filter_hide())                                                                     // hide
      win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_hide_colour());
    else                                                                                                   // show
      win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_show_colour());
  }

  BAND cur_band = safe_get_band();
  MODE cur_mode = safe_get_mode();

  if (bandmaps.size() > static_cast<int>(cur_band))
  { bandmap& bm = bandmaps[cur_band];         // use map for current band, so column offset is correct

    bm.filter_enabled(context.bandmap_filter_enabled());
    bm.filter_hide(context.bandmap_filter_hide());

    const vector<string>& original_filter = context.bandmap_filter();

//    for_each(original_filter.begin(), original_filter.end(), [&bm] (const string& filter) { bm.filter_add_or_subtract(filter); } );  // incorporate each filter string
    FOR_ALL(original_filter, [&bm] (const string& filter) { bm.filter_add_or_subtract(filter); } );  // incorporate each filter string

    win_bandmap_filter < WINDOW_CLEAR < CURSOR_START_OF_LINE < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();
  }

  /*
  dx_post dxa("DX de RX9WN:     14001.0  UE9WFA       see UE9WURC  QRZ.COM           1932Z ");
  bandmaps[BAND_20] += bandmap_entry(dxa);
  win_bandmap <= bandmaps[BAND_20];

  dx_post dxb("DX de RX9WN:     14002.0  UE9WFB       see UE9WURC  QRZ.COM           1932Z ");
  bandmaps[BAND_20] += bandmap_entry(dxb);
  win_bandmap <= bandmaps[BAND_20];
   */

  //  ost << "About to read Cabrillo log" << endl;


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

// create the cluster, and package it for use by the process_cluster_info() thread
// coinstructor for cluster has to be in a diffreent thread, so that we don't block this one
  if (!context.cluster_server().empty() and !context.cluster_username().empty() and !context.my_ip().empty())
  { static pthread_t spawn_thread_id;

    try
    { create_thread(&spawn_thread_id, &(attr_detached.attr()), spawn_dx_cluster, nullptr, "cluster spawn");
    }

    catch (const pthread_error& e)
    { ost << e.reason() << endl;
      exit(-1);
    }
  }

// ditto for the RBN
  if (!context.rbn_server().empty() and !context.rbn_username().empty() and !context.my_ip().empty())
  { static pthread_t spawn_thread_id;

    try
    { create_thread(&spawn_thread_id, &(attr_detached.attr()), spawn_rbn, nullptr, "RBN spawn");
    }

    catch (const pthread_error& e)
    { ost << e.reason() << endl;
      exit(-1);
    }
  }

// backup the last-used log, if one exists
  { string fn = context.logfile();
    int index = 0;

    ost << "fn is: " << fn << endl;

    while (file_exists(fn + "-" + to_string(index)))
      index++;

    ost << "index = " << index << endl;

    if (file_exists(fn))
    { ost << "file " << fn << " exists" << endl;
      file_copy(fn, fn + "-" + to_string(index));
      ost << "copied to " << fn + "-" + to_string(index) << endl;
    }
  }

// now we can restore data from the last run
    if (!cl.parameter_present("-clean"))
    { if (!cl.parameter_present("-rebuild"))    // if -rebuild is present, then don't restore from archive; rebuild only from logbook
      { const string archive_filename = context.archive_name();

        if (file_exists(archive_filename) and !file_empty(archive_filename))
          restore_data(archive_filename);
        else
          alert("No archive data present");
      }
      else     // rebuild
      { string file;

        try
        { file = read_file(context.logfile());    // in current directory
        }

        catch (...)
        { alert("Error reading log file: " + context.logfile());
        }

        if (!file.empty())
        { win_message < WINDOW_CLEAR <= "Rebuilding...";

          const vector<string> lines = to_lines(file);

          for (const auto& line : lines)
          { QSO qso;

            qso.populate_from_verbose_format(context, line, rules, statistics);  // updates exchange mults if auto

            ost << "QSO: " << qso << endl;

// callsign mults
            allow_for_callsign_mults(qso);

// country mults
            update_known_country_mults(qso.callsign());
            qso.is_country_mult( statistics.is_needed_country_mult(qso.callsign(), qso.band(), qso.mode()) );

// add exchange info for this call to the exchange db
            const vector<received_field>& received_exchange = qso.received_exchange();

            for (const auto& exchange_field : received_exchange)
            { if (!(variable_exchange_fields < exchange_field.name()))
                exchange_db.set_value(qso.callsign(), exchange_field.name(), exchange_field.value());   // add it to the database of exchange fields
            }

//            ost << "size of exchange_db = " << exchange_db.size() << endl;
//             ost << "adding to statistics" << endl;

            statistics.add_qso(qso, logbk, rules);

//            ost << "added to statistics" << endl;

            logbk += qso;
            rate.insert(qso.epoch_time(), statistics.points(rules));

            win_message <= WINDOW_CLEAR;
          }

//          ost << "CW QSOs = " << statistics.n_qsos(rules, MODE_CW) << endl;

// rebuild the history
          rebuild_history(logbk, rules, statistics, q_history, rate);

//          ost << "CW QSOs [1] = " << statistics.n_qsos(rules, MODE_CW) << endl;

// rescore the log
          rescore(rules);
          update_rate_window();

          scp_dynamic_db.clear();  // clears cache of parent
          fuzzy_dynamic_db.clear();

          const vector<QSO> qso_vec = logbk.as_vector();

          for (const auto& qso : qso_vec)
          { if (!scp_db.contains(qso.callsign()) and !scp_dynamic_db.contains(qso.callsign()))
              scp_dynamic_db.add_call(qso.callsign());
          }

          for (const auto& qso : qso_vec)
          { if (!fuzzy_db.contains(qso.callsign()) and !fuzzy_dynamic_db.contains(qso.callsign()))
              fuzzy_dynamic_db.add_call(qso.callsign());
          }
        }

// octothorpe
        if (logbk.size() >= 1)
        { const QSO last_qso = logbk[logbk.size()];    // wrt 1
          const MODE m = last_qso.mode();

          if (rules.sent_exchange_includes("SERNO", m))
            octothorpe = from_string<unsigned int>(last_qso.sent_exchange("SERNO")) + 1;
        }
        else
          octothorpe = 1;
      }

//    ost << "CW QSOs [2] = " << statistics.n_qsos(rules, MODE_CW) << endl;

// display most recent lines from log
      editable_log.recent_qsos(logbk, true);

// correct QSO number (and octothorpe)
      if (!logbk.empty())
      { next_qso_number = logbk[logbk.n_qsos()].number() /* logbook is wrt 1 */  + 1;
        win_qso_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= pad_string(to_string(next_qso_number), win_qso_number.width());
        win_serial_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= serial_number_string(octothorpe);

// go to band and mode of last QSO
        const QSO& last_qso = logbk[logbk.size()];
        const BAND b = last_qso.band();
        const MODE m = last_qso.mode();

        rig.rig_frequency(frequency(last_qso.freq()));
        rig.rig_mode(m);

        safe_set_mode(m);
        safe_set_band(b);

        cur_band = b;
      }

      update_remaining_callsign_mults_window(statistics, string(), cur_band, cur_mode);
      update_remaining_country_mults_window(statistics, cur_band, cur_mode);
      update_remaining_exchange_mults_windows(rules, statistics);

// QTCs
      if (send_qtcs)
      {
// number of EU QSOs from logbook
        const unsigned int n_eu_qsos = logbk.filter([] (const QSO& q) { return (q.continent() == string("EU")); } ).size();
        ost << "number of EU QSOs in log = " << n_eu_qsos << endl;

        try
        { qtc_db.read(context.qtc_filename());    // it is not an error if the file doesn't exist
        }

        catch (const qtc_error& e)
        { ost << "Error reading QTC file: " << e.reason() << endl;
          exit(-1);
        }

//        ost << "Number of QTC series read from QTC file= " << qtc_db.size() << endl;
//        ost << "Total number of QTC QSOs already sent = " << qtc_db.n_qtc_entries_sent() << endl;

        qtc_buf += logbk;  // add all the QSOs in the log to the unsent buffer

//        ost << "Total QTC-able QSOs in QTC buffer = " << qtc_buf.size() << endl;

        if (n_eu_qsos != qtc_buf.size())
          alert("WARNING: INCONSISTENT NUMBER OF QTC-ABLE QSOS");

// move the sent ones to the sent buffer
        const vector<qtc_series>& vec_qs = qtc_db.qtc_db();    ///< the QTC series

//        ost << "Number of QTC series to be processed = " << vec_qs.size() << endl;

//        ost << "Before unsent_to_sent, number of QTC QSOs sent = " << qtc_buf.n_sent_qsos() << endl;
//        ost << "Before unsent_to_sent, number of QTC QSOs unsent = " << qtc_buf.n_unsent_qsos() << endl;

        for_each(vec_qs.cbegin(), vec_qs.cend(), [] (const qtc_series& qs) { qtc_buf.unsent_to_sent(qs); } );

//        ost << "Number of QTC QSOs sent = " << qtc_buf.n_sent_qsos() << endl;
//        ost << "Number of QTC QSOs unsent = " << qtc_buf.n_unsent_qsos() << endl;

        statistics.qtc_qsos_sent(qtc_buf.n_sent_qsos());
        statistics.qtc_qsos_unsent(qtc_buf.n_unsent_qsos());

        if (!vec_qs.empty())
        { const qtc_series& last_qs = vec_qs[vec_qs.size() - 1];

//          ost << "Last QTC series: " << last_qs.complete_output_string() << endl;

          win_qtc_status < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Last QTC: " < last_qs.id() < " to " <= last_qs.target();
        }
      }

// display the current statistics
//      win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
      display_statistics(statistics.summary_string(rules));

//      const string score_str = pad_string(comma_separated_string(statistics.points(rules)), win_score.width() - string("Score: ").length());
      const string score_str = pad_string(separated_string(statistics.points(rules), TS), win_score.width() - string("Score: ").length());

      win_score < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Score: " <= score_str;

      update_mult_value();
    }

// now delete the archive file if it exists, regardless of whether we've used it
    if (file_exists(context.archive_name()))
      file_delete(context.archive_name());

    if (cl.parameter_present("-clean"))                          // start with clean slate
    { int index = 0;
      string target = OUTPUT_FILENAME + "-" + to_string(index);

      while (file_exists(target))
        file_delete(OUTPUT_FILENAME + "-" + to_string(index++));

      FILE* fp_log = fopen(context.logfile().c_str(), "w");
      fclose(fp_log);

      FILE* fp_archive = fopen(context.archive_name().c_str(), "w");
      fclose(fp_archive);

      if (send_qtcs)
      { FILE* fp_qtc = fopen(context.qtc_filename().c_str() , "w");
        fclose(fp_qtc);
      }
    }

// explicitly enter SAP mode
    enter_sap_mode();

    win_active_p = &win_call;

// explicitly force the cursor into the call window
    win_call <= CURSOR_START_OF_LINE;

//  keyboard.push_key_press("g4amt", 1000);  // hi, Terry

//  XSynchronize(keyboard.display_p(), true);

//  keyboard.test();
//  pthread_t keyboard_thread_id;
//  pthread_create(&keyboard_thread_id, NULL, keyboard_test, NULL);

 // int pending = XPending(keyboard.display_p());  // need to understand internal: what is the X queue, exactly, and what is the output buffer?

//  ost << "Pending events = " << pending << endl;

//  sleep(5);

//  keyboard.push_key_press('/');

    if (cl.value_present("-sim"))
    { static pthread_t simulator_thread_id;
      static tuple<string, int> params {cl.value("-sim"), cl.value_present("-n") ? from_string<int>(cl.value("-n")) : 0 };

      try
      { create_thread(&simulator_thread_id, &(attr_detached.attr()), simulator_thread, (void*)(&params), "simulator");
      }

      catch (const pthread_error& e)
      { ost << e.reason() << endl;
        exit(-1);
      }
    }
    else
      keyboard.x_multithreaded(false);    // make xlib more efficient

// everything is set up and running. Now we simply loop
    while (1)
    { while (keyboard.empty())
        sleep_for(milliseconds(10));

      const keyboard_event e = keyboard.pop();

      win_active_p -> process_input(e);
    }
  }

  catch (const socket_support_error& e)
  { cout << "Socket support error # " << e.code() << "; reason = " << e.reason() << endl;
    exit(-1);
  }

  catch (const drlog_error& e)
  {   cout << "drlog error # " << e.code() << "; reason = " << e.reason() << endl;
    exit(-1);
  }
}

/*! \brief          Display band and mode
    \param  win     window in which to display information
    \param  b       band
    \param  m       mode
*/
void display_band_mode(window& win, const BAND b, const enum MODE m)
{ static BAND last_band = BAND_20;  // start state
  static MODE last_mode = MODE_CW;
  static bool first_time = true;

  SAFELOCK(band_mode);

  if ((b != last_band) or (m != last_mode) or first_time)
  { first_time = false;
    last_band = b;
    last_mode = m;

    win < WINDOW_CLEAR < CURSOR_START_OF_LINE <= string(BAND_NAME[b] + " " + MODE_NAME[m]);
  }
}

/// Thread function to display the date and time
void* display_date_and_time(void* vp)
{ start_of_thread();

  int last_second;                             // so that we can tell when the time has changed
  array<char, 26> buf;                         // buffer to hold the ASCII date/time info; see man page for gmtime()
  string last_date;

  update_local_time();

  while (true)                                 // forever
  { const time_t now = time(NULL);             // get the time from the kernel
    struct tm    structured_time;
    bool new_second = false;

    gmtime_r(&now, &structured_time);          // convert to UTC in a thread-safe manner

    if (last_second != structured_time.tm_sec)                // update the time if the second has changed
    {
// this is a good opportunity to check for exiting
      { SAFELOCK(thread_check);

        if (exiting)
        { n_running_threads--;
          return nullptr;
        }
      }

      new_second = true;
      asctime_r(&structured_time, buf.data());                // convert to ASCII

      win_time < CURSOR_START_OF_LINE <= substring(string(buf.data(), 26), 11, 8);  // extract HH:MM:SS and display it

      last_second = structured_time.tm_sec;

// if a new minute, then update rate window, and do other stuff
      if (last_second % 60 == 0)
      { ost << "Time: " << substring(string(buf.data(), 26), 11, 8) << endl;

        update_local_time();
        update_rate_window();
        update_mult_value();

// possibly run thread to perform auto backup
        if (!context.auto_backup().empty())
        { static tuple<string, string, string> tsss;
          static pthread_t auto_backup_thread_id;

          const string filename = context.logfile();
          const string directory = context.auto_backup();
          const string qtc_filename = (context.qtcs() ? context.qtc_filename() : string());

          get<0>(tsss) = directory;
          get<1>(tsss) = filename;
          get<2>(tsss) = qtc_filename;

          try
          { create_thread(&auto_backup_thread_id, &(attr_detached.attr()), auto_backup, static_cast<void*>(&tsss), "backup");
          }

          catch (const pthread_error& e)
          { ost << e.reason() << endl;
          }
        }

// possibly clear alert window
        { SAFELOCK(alert);

          if ( (alert_time != 0) and ( (now - alert_time) > 60 ) )
          { win_message <= WINDOW_CLEAR;
            alert_time = 0;
          }
        }
      }

// if a new hour, then possibly create screenshot
      if ( (last_second % 60 == 0) and (structured_time.tm_min == 0) )
      { if (context.auto_screenshot())
        { static pthread_t auto_screenshot_thread_id;
          static string filename;

          const string dts = date_time_string();
          const string suffix = dts.substr(0, 13) + '-' + dts.substr(14); // replace : with -
          const string complete_name = string("auto-screenshot-") + suffix;

          filename = complete_name;

          ost << "dumping screenshot at time: " << hhmmss() << endl;

          try
          { create_thread(&auto_screenshot_thread_id, &(attr_detached.attr()), auto_screenshot, static_cast<void*>(&filename), "screenshot");
          }

          catch (const pthread_error& e)
          { ost << e.reason() << endl;
          }
        }
      }

// if a new day, then update date window
      const string date_string = substring(date_time_string(), 0, 10);

      if (date_string != last_date)
      { win_date < CURSOR_START_OF_LINE <= date_string;
        last_date = date_string;
        ost << "Date: " << substring(string(buf.data(), 26), 11, 8) << endl;
      }
    }

    sleep_for(deciseconds(new_second ? 8 : 1));
  }

  pthread_exit(nullptr);
}


/*! \brief  Thread function to display status of the rig

    Also displays bandmap if the frequency changes. The bandmap is actually updated on screen before any change in status
    NB It doesn't matter *how* the rig's frequency came to change ... it could be manual
*/
void* display_rig_status(void* vp)
{ start_of_thread();

  rig_status_info* rig_status_thread_parameters_p = static_cast<rig_status_info*>(vp);
  rig_status_info& rig_status_thread_parameters = *rig_status_thread_parameters_p;

  static long microsecond_poll_period = rig_status_thread_parameters.poll_time() * 1000;

//  ost << "microsecond_poll_period inside thread = " << microsecond_poll_period << endl;
  DRLOG_MODE last_drlog_mode = SAP_MODE;
  bandmap_entry be;

// populate the bandmap entry stuff that won't change
  be.callsign(MY_MARKER);
  be.time(::time(NULL));
  be.source(BANDMAP_ENTRY_LOCAL);
  be.expiration_time(be.time() + 1000000);    // a million seconds in the future

  while (true)
  {
    try
    { const bool in_call_window = (win_active_p == &win_call);  // never update call window if we aren't in it

       try
      { while ( rig_status_thread_parameters.rigp()-> is_transmitting() )  // don't poll while transmitting                   // don't poll when transmitting
          sleep_for(microseconds(microsecond_poll_period / 10));
      }

      catch (const rig_interface_error& e)
      { alert("Error communicating with rig during poll loop");
        sleep_for(microseconds(microsecond_poll_period * 10));                      // wait a (relatively) long time if there was an error
      }

// if it's a K3 we can get a lot of info with just one query -- for now just assume it's a K3

      if (ok_to_poll_k3)
      {
        const string status_str = (rig_status_thread_parameters.rigp())->raw_command("IF;", 38);          // K3 returns 38 characters

//        ost << "status string: " << status_str << endl;

        if (status_str.length() == 38)
        { const frequency f(from_string<unsigned int>(substring(status_str, 2, 11)));
          const frequency target = SAFELOCK_GET(cq_mode_frequency_mutex, cq_mode_frequency);

          const frequency f_b = rig.rig_frequency_b();

//        ost << "f = " << f.hz() << ", target = " << target.hz() << endl;
//        ost << "mode: " << ( (drlog_mode == CQ_MODE) ? "CQ" : "SAP") << endl;
//        ost << "equality: " << ( (target == f) ? "TRUE" : "FALSE") << endl;
//        ost << "inequality: " << ( (target != f) ? "NOT EQUAL" : "EQUAL") << endl;
//        ost << "equality of hz: " << ( (target.hz() == f.hz()) ? "TRUE" : "FALSE") << endl;

// explicitly set to SAP mode if we have QSYed.
          const DRLOG_MODE current_drlog_mode = SAFELOCK_GET(drlog_mode_mutex, drlog_mode);

          if ( (current_drlog_mode == CQ_MODE) and (last_drlog_mode == CQ_MODE) and (target != f) )
            enter_sap_mode();

          last_drlog_mode = current_drlog_mode;

          const MODE m = safe_get_mode();

//        ost << "f.display_string() = " << f.display_string() << endl;
//        ost << "be.freq().display_string() = " << be.freq().display_string() << endl;

// possibly update bandmap entry and nearby callsign, if any
          if (f.display_string() != be.freq().display_string())  // redraw if moved > 100 Hz
          { const BAND b = static_cast<BAND>(f);

            be.freq(f);
            be.band(b);
            safe_set_band(b);

//            const MODE m = safe_get_mode();

            display_band_mode(win_band_mode, b, m);

// update and display the correct bandmap
            bandmap& bandmap_this_band = bandmaps[b];

            bandmap_this_band += be;
            win_bandmap <= bandmap_this_band;

// is there a station close to our frequency?
// use the filtered bandmap (maybe should make this controllable? but used to use unfiltered version, and it was annoying
// to have invisible calls show up when I went to a frequency
            const string nearby_callsign = bandmap_this_band.nearest_rbn_threshold_and_filtered_callsign(f.khz(), context.guard_band(m));

 //         ost << "nearby callsign = " << nearby_callsign << " at " << f.display_string() << endl;

            if (!nearby_callsign.empty())
            { display_nearby_callsign(nearby_callsign);

              if (in_call_window)
              { string call_contents = remove_peripheral_spaces(win_call.read());

                if (!call_contents.empty())
                { if (last(call_contents, 5) == " DUPE")
                    call_contents = call_contents.substr(0, call_contents.length() - 5);    // reduce to actual call

                  string last_call;

                  { SAFELOCK(dupe_check);

                    last_call = last_call_inserted_with_space;
                  }

                  if (call_contents != last_call)
                    win_call < WINDOW_CLEAR <= CURSOR_START_OF_LINE;
                }
              }
            }
            else    // no nearby callsign
            { if (in_call_window)
// see if we are within twice the guard band before we clear the call window
              { const string call_contents = remove_peripheral_spaces(win_call.read());
                const bandmap_entry be = bandmap_this_band[call_contents];
                const unsigned int f_diff = abs(be.freq().hz() - f.hz());

                if (f_diff > 2 * context.guard_band(m))    // delete this and prior three lines to return to old code
                { if (!win_nearby.empty())
                    win_nearby <= WINDOW_CLEAR;

                  if (!call_contents.empty())
                  { string last_call;

                    { SAFELOCK(dupe_check);

                      last_call = last_call_inserted_with_space;
                    }

//              ost << "last_call_inserted_with_space = *" << last_call_inserted_with_space << "*; call_contents = *" << call_contents << "*" << endl;

                    if ((call_contents == last_call) or (call_contents == (last_call + " DUPE")) )
                      win_call < WINDOW_CLEAR <= CURSOR_START_OF_LINE;
                  }
                }
              }
            }
          }

// mode: the K3 is its usual rubbish self; sometimes the mode returned by the rig is incorrect
// following a recent change of mode. By the next poll it seems to be OK, though, so for now
// it seems like the effort of trying to work around the bug is not worth it
          static const unsigned int MODE_ENTRY = 29;      // position of the mode byte in the K3 status string
          const char mode_char = status_str[MODE_ENTRY];
          string mode_str;

          switch (mode_char)
          { case '1' :
              mode_str = "LSB ";
              break;

            case '2' :
              mode_str = "USB ";
              break;

            case '3' :
              mode_str = " CW ";
              break;

            default :
              mode_str = "UNK ";
              break;
          }

          static const unsigned int RIT_ENTRY = 23;      // position of the RIT status byte in the K3 status string
          static const unsigned int XIT_ENTRY = 24;      // position of the XIT status byte in the K3 status string

          const bool rit_is_on = (status_str[RIT_ENTRY] == '1');
          const bool xit_is_on = (status_str[XIT_ENTRY] == '1');

          string rit_xit_str;

          if (xit_is_on)
            rit_xit_str += "X";

          if (rit_is_on)
            rit_xit_str += "R";

          if (rit_is_on or xit_is_on)
          { const int rit_xit_value = from_string<int>(substring(status_str, 19, 4));

           rit_xit_str += status_str[18] + to_string(rit_xit_value);
            rit_xit_str = pad_string(rit_xit_str, 7);
          }

          if (rit_xit_str.empty())
            rit_xit_str = create_string(' ', 7);

          static const unsigned int SPLIT_ENTRY = 32;      // position of the SPLIT status byte in the K3 status string

          const bool is_split = (status_str[SPLIT_ENTRY] == '1');

// remove TX indicator, since we no longer poll if we're TXing (modulo a µs or so)
//          static const unsigned int TX_ENTRY = 28;      // position of the transmit-mode status byte in the K3 status string

//          const bool transmitting = (status_str[TX_ENTRY] == '1');

//          string tx_str(transmitting ? "TX " : "RX ");

          const string bandwidth_str = to_string(rig_status_thread_parameters.rigp()->bandwidth());
          const string frequency_b_str = f_b.display_string();

// now display the status
          win_rig.default_colours(win_rig.fg(), context.mark_frequency(m, f) ? COLOUR_RED : COLOUR_BLACK);  // red if this contest doesn't want us to be on this QRG

//          ost << "background colour is " << (context.mark_frequency(m, f) ? "RED" : "BLACK") << " for frequency " << f.display_string() << " and mode " << m << endl;

//          ost << "clg tx_vfo()" << endl;
//          const VFO tx_vfo = rig_status_thread_parameters.rigp()-> tx_vfo();
//          ost << "tx vfo is: " << tx_vfo << endl;
          const bool sub_rx = (rig_status_thread_parameters.rigp())->sub_receiver_enabled();
          const auto fg = win_rig.fg();    // original foreground colour

          win_rig < WINDOW_CLEAR < CURSOR_TOP_LEFT
                  < ( is_split ? WINDOW_NOP : WINDOW_BOLD)
                  < pad_string(f.display_string(), 7)
                  < ( is_split ? WINDOW_NOP : WINDOW_NORMAL)
                  < ( (rig_status_thread_parameters.rigp()->is_locked()) ? "L " : "  " )
                  < mode_str /* < tx_str */
                  < ( is_split ? WINDOW_BOLD : WINDOW_NORMAL);

          if (sub_rx)
            win_rig < COLOURS(COLOUR_GREEN, win_rig.bg());

          win_rig < frequency_b_str
                  < ( is_split ? WINDOW_NORMAL : WINDOW_NOP);

          if (sub_rx)
            win_rig < COLOURS(fg, win_rig.bg());

          win_rig < CURSOR_DOWN
                  < CURSOR_START_OF_LINE < rit_xit_str < "   " <= bandwidth_str;
        }
      }
    }

// be silent if there was an error communicating with the rig
    catch (const rig_interface_error& e)
    {
    }

    sleep_for(microseconds(microsecond_poll_period));

    { SAFELOCK(thread_check);

      if (exiting)
      { n_running_threads--;
        return nullptr;
      }
    }
  }

//  return nullptr;
  pthread_exit(nullptr);
}

/*! \brief  Thread to process data from the cluster or the RBN.

    Must start the thread to obtain data before trying to process it with this one;
    pulls the data from the cluster object [and removes the data from it]
*/
void* process_rbn_info(void* vp)
{ start_of_thread();

// get access to the information that's been passed to the thread
  cluster_info* cip = static_cast<cluster_info*>(vp);
  window& cluster_line_win = *(cip->wclp());            // the window to which we will write each line from the cluster/RBN
  window& cluster_mult_win = *(cip->wcmp());            // the window in which to write mults
  dx_cluster& rbn = *(cip->dcp());                      // the DX cluster or RBN
  running_statistics& statistics = *(cip->statistics_p());           // the statistics
  location_database& location_db = *(cip->location_database_p());    // database of locations
  window& bandmap_win = *(cip->win_bandmap_p());                     // bandmap window
  array<bandmap, NUMBER_OF_BANDS>& bandmaps = *(cip->bandmaps_p());  // bandmaps
  const bool is_rbn = (rbn.source() == POSTING_RBN);
  const bool is_cluster = !is_rbn;
  const bool rbn_beacons = context.rbn_beacons();

  const size_t QUEUE_SIZE = 100;    // size of queue of recent calls posted to the mult window
  string unprocessed_input;         // data from the cluster that have not yet been processed by this thread
  const set<BAND> permitted_bands { rules.permitted_bands().cbegin(), rules.permitted_bands().cend() };
  deque<pair<string, BAND>> recent_mult_calls;                                    // the queue of recent calls posted to the mult window

  const int highlight_colour = colours.add(COLOUR_WHITE, COLOUR_RED);             // colour that will mark that we are processing a ten-second pass
  const int original_colour = colours.add(cluster_line_win.fg(), cluster_line_win.bg());

  if (is_cluster)
    win_cluster_screen < WINDOW_CLEAR < CURSOR_BOTTOM_LEFT;  // probably unused

  while (1)                                                // forever; process a ten-second pass
  { set<BAND> changed_bands;                               // the bands that have been changed by this ten-second pass
    bool cluster_mult_win_was_changed = false;             // has cluster_mult_win been changed by this pass?
    string last_processed_line;                            // the last line processed during this pass
    const string new_input = rbn.get_unprocessed_input();  // add any unprocessed info from the cluster; deletes the data from the cluster

// a visual marker that we are processing a pass; this should appear only briefly
    const string win_contents = cluster_line_win.read();
    const char first_char = win_contents.empty() ? ' ' : win_contents[0];

    cluster_line_win < CURSOR_START_OF_LINE < colour_pair(highlight_colour) < first_char <= colour_pair(original_colour);

    if (is_cluster and !new_input.empty())
    { const string no_cr = remove_char(new_input, CR_CHAR);
      const vector<string> lines = to_lines(no_cr);

// I don't understand why the scrolling occurs automatically... in particular
// I don't know what causes it to scroll
      for (unsigned int n = 0; n < lines.size(); ++n)
      { win_cluster_screen < lines[n];                       // THIS causes the scroll, but I don't know why

        if ( (n != lines.size() - 1) or no_cr[no_cr.length() - 1] == LF_CHAR)
          win_cluster_screen < CURSOR_START_OF_LINE;
        else
          win_cluster_screen < WINDOW_SCROLL_DOWN;

        win_cluster_screen < WINDOW_REFRESH;
      }
    }

    unprocessed_input += new_input;

    while (contains(unprocessed_input, CRLF))              // look for EOL markers
    { const size_t posn = unprocessed_input.find(CRLF);
      const string line = substring(unprocessed_input, 0, posn);   // store the next unprocessed line

      unprocessed_input = substring(unprocessed_input, posn + 2);  // delete the line (including the CRLF) from the buffer

      if (!line.empty())
      { const bool is_beacon = contains(line, " BCN ");

        if (!is_beacon or rbn_beacons)
        {
//ost << "processing rbn line: " << line << endl;
//ost << "rbn line has length " << line.size() << endl;
          last_processed_line = line;

// display if this is a new mult on any band, and if the poster is on our own continent
          const dx_post post(line, location_db, rbn.source());

          if (post.valid())
          { const BAND dx_band = post.band();

            if (permitted_bands < dx_band)
            { const BAND cur_band = safe_get_band();
            //const MODE cur_mode = safe_get_mode();
              const string& dx_callsign = post.callsign();
              const string& poster = post.poster();
              const pair<string, BAND> target { dx_callsign, dx_band };
              const location_info li = location_db.info(dx_callsign);

              bandmap_entry be( (post.source() == POSTING_CLUSTER) ? BANDMAP_ENTRY_CLUSTER : BANDMAP_ENTRY_RBN );

              be.freq(post.freq());
              be.callsign(dx_callsign);
              be.canonical_prefix(li.canonical_prefix());
              be.continent(li.continent());
              be.band(dx_band);
              be.expiration_time(post.time_processed() + ( post.source() == POSTING_CLUSTER ? (context.bandmap_decay_time_cluster() * 60) :
                              (context.bandmap_decay_time_rbn() * 60 ) ) );
              if (post.source() == POSTING_RBN)     // so we can test for threshold anent RBN posts
                be.posters( { poster } );

// do we still need this guy?
              const bool is_needed = is_needed_qso(dx_callsign, dx_band, be.mode());

              be.is_needed(is_needed);

// update known mults before we test to see if this is a needed mult

// possibly add the call to the known prefixes
              update_known_callsign_mults(dx_callsign);

// possibly add the call to the known countries
              update_known_country_mults(dx_callsign);

// possibly add exchange mult value
//              if (context.auto_remaining_exchange_mults())
              { const vector<string> exch_mults = rules.exchange_mults();                                      ///< the exchange multipliers, in the same order as in the configuration file

                for (const auto& exch_mult_name : exch_mults)
                { if (context.auto_remaining_exchange_mults(exch_mult_name))
                  { const string guess = exchange_db.guess_value(dx_callsign, exch_mult_name);

//                ost << "guess in drlog = " << guess << ", MULT VALUE = " << MULT_VALUE(exch_mult_name, guess) << endl;

                  if (!guess.empty())
                    statistics.add_known_exchange_mult(exch_mult_name, MULT_VALUE(exch_mult_name, guess));
                  }
                }
              }

              be.calculate_mult_status(rules, statistics);

             const bool is_recent_call = ( find(recent_mult_calls.cbegin(), recent_mult_calls.cend(), target) != recent_mult_calls.cend() );

              if (!is_recent_call and (be.is_needed_callsign_mult() or be.is_needed_country_mult()  or be.is_needed_exchange_mult()))            // if it's a mult and not recently posted...
              { if (location_db.continent(poster) == my_continent)                                                      // heard on our continent?
                { cluster_mult_win_was_changed = true;             // keep track of the fact that we're about to write changes to the window
                  recent_mult_calls.push_back(target);

                  while (recent_mult_calls.size() > QUEUE_SIZE)    // keep the list of recent calls to a reasonable size
                    recent_mult_calls.pop_front();

                  cluster_mult_win < CURSOR_TOP_LEFT < WINDOW_SCROLL_DOWN;

// highlight it if it's on our current band
                  if (dx_band == cur_band)
                    cluster_mult_win < WINDOW_HIGHLIGHT;

                  const string frequency_str = pad_string(post.frequency_str(), 7);
                  cluster_mult_win < pad_string(frequency_str + " " + dx_callsign, cluster_mult_win.width(), PAD_RIGHT);  // display it -- removed refresh

                  if (dx_band == cur_band)
                    cluster_mult_win < WINDOW_NORMAL;    // removed refresh
                }
              }

// add the post to the correct bandmap
              bandmap& bandmap_this_band = bandmaps[dx_band];

              bandmap_this_band += be;

// prepare to display the bandmap if we just made a change for this band
              changed_bands.insert(dx_band);
            }
            else
              { //ost << "not a contest posting" << endl;
              }
          }
          else
          { //ost << "invalid post" << endl;
          }
        }
      }
    }

// update displayed bandmap if there was a change
    const BAND cur_band = safe_get_band();

    if (changed_bands < cur_band)
      bandmap_win <= bandmaps[cur_band];

    if (cluster_mult_win_was_changed)    // update the window on the screen
      cluster_mult_win.refresh();

// remove marker that we are processing a pass
// we assume that the height of cluster_line_win is one
    if (last_processed_line.empty())
      cluster_line_win < CURSOR_START_OF_LINE <= first_char;
    else
      cluster_line_win < CURSOR_START_OF_LINE < WINDOW_CLEAR <= last_processed_line;  // display the last processed line on the screen

    if (context.auto_remaining_country_mults())
      update_remaining_country_mults_window(statistics, safe_get_band(), safe_get_mode());  // might have added a new one if in auto mode

    for (const auto n : RANGE<unsigned int>(1, 10) )    // wait 10 seconds before getting any more unprocessed info
    {
      { SAFELOCK(thread_check);

        if (exiting)
        { n_running_threads--;
          return nullptr;
        }
      }

      sleep_for(seconds(1));
    }
  }

//  return nullptr;                         // keep the compiler happy
  pthread_exit(nullptr);
}

/// Thread function to obtain data from the cluster
void* get_cluster_info(void* vp)
{ start_of_thread();

  dx_cluster& cluster = *(static_cast<dx_cluster*>(vp)); // make the cluster available

  while(1)                                           // forever
  { cluster.read();                                  // reads the socket and stores the data

    for (const auto n : RANGE<unsigned int>(1, 5) )    // check the socket every 5 seconds
    {
      { SAFELOCK(thread_check);

        if (exiting)
        { n_running_threads--;
          return nullptr;
        }
      }

      sleep_for(seconds(1));
    }
  }

//  return nullptr;                         // keep the compiler happy
  pthread_exit(nullptr);
}

/// Thread function to prune the bandmaps once per minute
void* prune_bandmap(void* vp)
{ start_of_thread();

// get access to the information that's been passed to the thread
  bandmap_info* cip = static_cast<bandmap_info*>(vp);
  window& bandmap_win = *(cip->win_bandmap_p());                     // bandmap window
  array<bandmap, NUMBER_OF_BANDS>& bandmaps = *(cip->bandmaps_p());  // bandmaps

  while (1)
  { const enum BAND cur_band = safe_get_band();

    FOR_ALL(bandmaps, [](bandmap& bm) { bm.prune(); } );

    bandmap_win <= bandmaps[cur_band];

    for (const auto n : RANGE<unsigned int>(1, 60) )    // check once per second for a minute
    {
      { SAFELOCK(thread_check);

        if (exiting)
        { n_running_threads--;
          return nullptr;
        }
      }

      sleep_for(seconds(1));
    }
  }

//  return nullptr;
  pthread_exit(nullptr);
}

// -------------------------------------------------  functions to process input to various windows  -----------------------------

// function to process input to the CALL window
/*  KP numbers -- CW messages
    CTRL-C -- EXIT (same as .QUIT)
    ALT-M -- change mode
    PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
    ALT-K -- toggle CW
    ESCAPE
    TAB -- switch between CQ and SAP mode
    F10 -- toggle filter_remaining_country_mults
    F11 -- band map filtering
    ALT-KP_4 -- decrement bandmap column offset
    ALT-KP_6 -- increment bandmap column offset
    ENTER
    CTRL-ENTER -- assume it's a call or partial call and go to the call if it's in the bandmap
    KP ENTER -- send CQ #2
    CTRL-KP-ENTER -- look for, and then display, entry in all the bandmaps
    SPACE -- generally, dupe check
    CTRL-LEFT-ARROW, CTRL-RIGHT-ARROW, ALT-LEFT_ARROW, ALT-RIGHT-ARROW: up or down to next needed QSO or next needed mult. Uses filtered bandmap
    SHIFT (RIT control)
    ALT-Y -- delete last QSO
    CURSOR UP -- go to log window
    CURSOR DOWN -- possibly replace call with SCP info; NB this assumes COLOUR_GREEN and COLOUR_RED are the hardwired colours in the SCP window
    CTRL-CURSOR DOWN -- possibly replace call with fuzzy info; NB this assumes COLOUR_GREEN and COLOUR_RED are the hardwired colours in the SCP window
    ALT-KP+ -- increment octothorpe
    ALT-KP- -- decrement octothorpe
    CTRL-KP+ -- increment qso number
    CTRL-KP- -- decrement qso number
*/
void process_CALL_input(window* wp, const keyboard_event& e /* int c */ )
{
// syntactic sugar
  window& win = *wp;

  static const char COMMAND_CHAR     = '.';                            // the character that introduces a command
  const MODE cur_mode = safe_get_mode();
  const string prior_contents = remove_peripheral_spaces(win.read());  // the old contents of the window

  // keyboard_queue::process_events() has already filtered out uninteresting events

  ost << "processing CALL input; event string: " << e.str() << endl;
//  ost << "event keysym: " << hex << e.symbol() << dec << endl;

  bool processed = win.common_processing(e);

  if (!processed and ((e.is_char('/') or e.is_char('.') or e.is_char('-')) or (e.is_unmodified() and ( (e.symbol() == XK_KP_Add) or (e.symbol() == XK_KP_Subtract)) )))
  { win <= e.str();
    processed = true;
  }

// need comma and asterisk for rescore options, backslash for scratchpad
  if (!processed and (e.is_char(',') or e.is_char('*') or e.is_char('\\')))
  { win <= e.str();
    processed = true;
  }

// question mark
  if (!processed and (e.is_char('=')))
  { win <= "?";
    processed = true;
  }

// populate the info and extract windows if we have already processed the input
  if (processed and !win_call.empty())
  { const string callsign = remove_peripheral_spaces(win_call.read());

    display_call_info(callsign);
  }

// KP numbers -- CW messages
  if (!processed and cw_p and (cur_mode == MODE_CW))
  { if (e.is_unmodified() and (keypad_numbers < e.symbol()) )
    {
// may need to temporarily reduce octothorpe for when SAP asks for repeat of serno
      if (prior_contents.empty())
        octothorpe--;

//      ost << "sending CW message: " << expand_cw_message(cwm[e.symbol()]) << endl;
      (*cw_p) << expand_cw_message(cwm[e.symbol()]);

      if (prior_contents.empty())
        octothorpe++;

      processed = true;
    }
  }

// CTRL-C -- EXIT (same as .QUIT)
  if (!processed and (e.is_control('c')))
    exit_drlog();

// ALT-B and ALT-V (band up and down)
  if (!processed and (e.is_alt('b') or e.is_alt('v')) and (rules.n_bands() > 1))
  { // ost << "processing band change" << endl;

    try
    { BAND cur_band       = safe_get_band();
      const MODE cur_mode = safe_get_mode();

      rig.set_last_frequency(cur_band, cur_mode, rig.rig_frequency());             // save current frequency

      cur_band = ( e.is_alt('b') ? rules.next_band_up(cur_band) : rules.next_band_down(cur_band) );    // move up or down one band
      safe_set_band(cur_band);
      frequency last_frequency = rig.get_last_frequency(cur_band, cur_mode);  // go to saved frequency for this band/mode (if any)

      if (last_frequency.hz() == 0)
        last_frequency = DEFAULT_FREQUENCIES[ { cur_band, cur_mode } ];

      rig.rig_frequency(last_frequency);

// make sure that it's in the right mode, since rigs can do weird things depending on what mode it was in the last time it was on this band
      rig.rig_mode(cur_mode);
      enter_sap_mode();

// clear the call window (since we're now on a new band)
      win < WINDOW_CLEAR <= CURSOR_START_OF_LINE;

      display_band_mode(win_band_mode, cur_band, cur_mode);

// update bandmap; note that it will be updated at the next poll anyway (typically within one second)
      bandmap& bm = bandmaps[cur_band];

      win_bandmap <= bm;

// is there a station close to our frequency?
      const string nearby_callsign = bm.nearest_rbn_threshold_and_filtered_callsign(last_frequency.khz(), context.guard_band(cur_mode));

      display_nearby_callsign(nearby_callsign);  // clears nearby window if call is empty

// update displays of needed mults
      update_remaining_callsign_mults_window(statistics, string(), cur_band, cur_mode);
      update_remaining_country_mults_window(statistics, cur_band, cur_mode);
      update_remaining_exchange_mults_windows(rules, statistics, cur_band);

      win_bandmap_filter < WINDOW_CLEAR < CURSOR_START_OF_LINE < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();
    }

    catch (const rig_interface_error& e)
    { alert(e.reason());
    }

    processed = true;
  }

// ALT-M -- change mode
  if (!processed and e.is_alt('m') and (n_modes > 1))
  { const BAND cur_band = safe_get_band();
    MODE cur_mode = safe_get_mode();

    rig.set_last_frequency(cur_band, cur_mode, rig.rig_frequency());             // save current frequency

    cur_mode = rules.next_mode(cur_mode);
    safe_set_mode(cur_mode);

    rig.rig_frequency(rig.get_last_frequency(cur_band, cur_mode).hz() ? rig.get_last_frequency(cur_band, cur_mode) : DEFAULT_FREQUENCIES[ { cur_band, cur_mode } ]);
    rig.rig_mode(cur_mode);

    display_band_mode(win_band_mode, cur_band, cur_mode);

// update displays of needed mults
    update_remaining_country_mults_window(statistics, cur_band, cur_mode);

    processed = true;
  }

// PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
  if (!processed and ((e.symbol() == XK_Next) or (e.symbol() == XK_Prior)))
  { if (cw_p)
    { int change = (e.is_control() ? 1 : 3);

      if (e.symbol() == XK_Prior)
        change = -change;

      cw_speed(cw_p->speed() - change);  // effective immediately
    }

    processed = true;
  }

// ALT-K -- toggle CW
  if (!processed and e.is_alt('k') and cw_p)
  { cw_p->toggle();

    win_wpm < WINDOW_CLEAR < CURSOR_START_OF_LINE <= (cw_p->disabled() ? "NO CW" : (to_string(cw_p->speed()) + " WPM") );

    processed = true;
  }

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
    if (!processed and (!remove_peripheral_spaces(win.read()).empty()))
    { win <= WINDOW_CLEAR;
      processed = true;
    }

    if (!processed and drlog_mode == SAP_MODE)
    { enter_cq_mode();
      processed = true;
    }

    processed = true;
  }

// TAB -- switch between CQ and SAP mode
  if (!processed and (e.symbol() == XK_Tab))
  { toggle_drlog_mode();
    processed = true;
  }

// F10 -- toggle filter_remaining_country_mults
  if (!processed and (e.symbol() == XK_F10))
  { filter_remaining_country_mults = !filter_remaining_country_mults;
    update_remaining_country_mults_window(statistics, safe_get_band(), safe_get_mode());
    processed = true;
  }

//  if (!processed and (e.symbol() == XK_F1))
//  { ost << "Rig test status = " << rig.test() << endl;
//    processed = true;
//  }

// F11 -- band map filtering
  if (!processed and (e.symbol() == XK_F11))
  { const string contents = remove_peripheral_spaces(win.read());
    const BAND cur_band = safe_get_band();
    bandmap& bm = bandmaps[cur_band];        // use current bandmap to make it easier to display column offset

    if (contents.empty())     // cycle amongst OFF / HIDE / SHOW
    { if (bm.filter_enabled() and bm.filter_show())
      { bm.filter_enabled(false);

        win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_disabled_colour());
        win_bandmap_filter < WINDOW_CLEAR < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();

        processed = true;
      }

      if (!processed and !bm.filter_enabled())
      { bm.filter_enabled(true);
        bm.filter_hide(true);

        win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_hide_colour());
        win_bandmap_filter < WINDOW_CLEAR < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();

        processed = true;
      }

      if (!processed and bm.filter_enabled() and bm.filter_hide())
      { bm.filter_show(true);

        win_bandmap_filter.default_colours(win_bandmap_filter.fg(), context.bandmap_filter_show_colour());
        win_bandmap_filter < WINDOW_CLEAR < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();

        processed = true;
      }
    }
    else   // treat the contents as something to add to or subtract from the filter
    { const string str = ( (CONTINENT_SET < contents) ? contents : location_db.canonical_prefix(contents) );

      bm.filter_add_or_subtract(str);
      win_bandmap_filter < WINDOW_CLEAR < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();

      processed = true;         //  processed even if haven't been able to do anything with it
    }

    win_bandmap <= bm;
  }

// ALT-KP_4: decrement bandmap column offset; ALT-KP_6: increment bandmap column offset
  if (!processed and e.is_alt() and ( (e.symbol() == XK_KP_4) or (e.symbol() == XK_KP_6)
                                  or  (e.symbol() == XK_KP_Left) or (e.symbol() == XK_KP_Right) ) )
  { bandmap& bm = bandmaps[safe_get_band()];

    bm.column_offset(bm.column_offset() + ( ( (e.symbol() == XK_KP_6 ) or (e.symbol() == XK_KP_Right ) ) ? 1 : -1 ) );

    alert(string("Bandmap column offset set to: ") + to_string(bm.column_offset()));

    win_bandmap <= bm;
    win_bandmap_filter < WINDOW_CLEAR < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();

    processed = true;
  }

// ENTER -- a lot of complicated stuff
  if (!processed and e.is_unmodified() and (e.symbol() == XK_Return))
  { const string contents = remove_peripheral_spaces(win.read());

// if empty, send CQ #1, regardless of whether I'm in CQ or SAP mode
    if (contents.empty())
    { if ( (safe_get_mode() == MODE_CW) and (cw_p) and (drlog_mode == CQ_MODE))
      { const string msg = context.message_cq_1();

        if (!msg.empty())
          (*cw_p) << msg;
      }
      processed = true;
    }

// process a command if the first character is the COMMAND_CHAR
    if (!processed and contents[0] == COMMAND_CHAR)
    { const string command = substring(contents, 1);

ost << "processing command: " << command << endl;

// .ABORT -- immediate exit, simulating power failure
      if (substring(command, 0, 5) == "ABORT")
      { exit(-1);
      }

// .ADD <call> -- remove call from the do-not-show list
      if (substring(command, 0, 3) == "ADD")
      { if (contains(command, " "))
        { const size_t posn = command.find(" ");
          const string callsign = remove_peripheral_spaces(substring(command, posn));

          for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm.remove_from_do_not_add(callsign); } );
        }
      }

// .CABRILLO
      if (command == "CABRILLO")
      { const string cabrillo_filename = (context.cabrillo_filename().empty() ? "cabrillo" : context.cabrillo_filename());
        const string log_str = logbk.cabrillo_log(context, statistics.points(rules));    // do I really want the points?

        write_file(log_str, cabrillo_filename);
        alert((string("Cabrillo file ") + context.cabrillo_filename() + " written"));
      }

      win <= WINDOW_CLEAR;

// .CLEAR
      if (command == "CLEAR")
        win_message <= WINDOW_CLEAR;

// .QUIT
      if (command == "QUIT")
        exit_drlog();

// .REMOVE <call> -- remove call from bandmap and add it to the do-not-show list
      if (substring(command, 0, 6) == "REMOVE" or substring(command, 0, 2) == "RM")
      { if (contains(command, " "))
        { const size_t posn = command.find(" ");
          const string callsign = remove_peripheral_spaces(substring(command, posn));

          for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm -= callsign;
                                                                         bm.do_not_add(callsign);
                                                                       } );

          bandmap& bm = bandmaps[safe_get_band()];

          win_bandmap <= bm;
        }
      }

// .RESCOREB or .SCOREB
      if (substring(command, 0, 8) == "RESCOREB" or substring(command, 0, 6) == "SCOREB")
      { if (contains(command, " "))
        { size_t posn = command.find(" ");
          string rhs = substring(command, posn);
          set<BAND> score_bands;

//next bit of code is copied from drlog_context.cpp
          const vector<string> bands_str = remove_peripheral_spaces(split_string(rhs, ","));

          for (const auto& band_str : bands_str)
          { try
            { score_bands.insert(BAND_FROM_NAME.at(band_str));
            }

            catch (...)
            { if (band_str == "*")
              { for (const auto& b : rules.permitted_bands())
                  score_bands.insert(b);
              }
              else
                alert("Error parsing [RE]SCOREB command");
            }
          }

          rules.score_bands(score_bands);
        }
        else    // no band information
          rules.restore_original_score_bands();

        { const set<BAND> score_bands = rules.score_bands();
          string bands_str;

          for (const auto& b : score_bands)
            bands_str += (BAND_NAME[b] + " ");

          win_score_bands < WINDOW_CLEAR < "Score Bands: " <= bands_str;
        }

        rescore(rules);
        update_rate_window();

// display the current statistics
//        win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
        display_statistics(statistics.summary_string(rules));

//        const string score_str = pad_string(comma_separated_string(statistics.points(rules)), win_score.width() - string("Score: ").length());
        const string score_str = pad_string(separated_string(statistics.points(rules), TS), win_score.width() - string("Score: ").length());

        win_score < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Score: " <= score_str;
      }

// .RESCOREM or .SCOREM  &&& HERE
      if (substring(command, 0, 8) == "RESCOREM" or substring(command, 0, 6) == "SCOREM")
      { if (contains(command, " "))
        { size_t posn = command.find(" ");
          string rhs = substring(command, posn);
          set<MODE> score_modes;

          const vector<string> modes_str = remove_peripheral_spaces(split_string(rhs, ","));

          for (const auto& mode_str : modes_str)
          { try
            { score_modes.insert(MODE_FROM_NAME.at(mode_str));
            }

            catch (...)
            { if (mode_str == "*")
              { for (const auto& m : rules.permitted_modes())
                  score_modes.insert(m);
              }
              else
                alert("Error parsing [RE]SCOREM command");
            }
          }

          rules.score_modes(score_modes);
        }
        else    // no mode information
          rules.restore_original_score_modes();

        { const set<MODE> score_modes = rules.score_modes();
          string modes_str;

          for (const auto& m : score_modes)
            modes_str += (MODE_NAME[m] + " ");

          win_score_modes < WINDOW_CLEAR < "Score Modes: " <= modes_str;
        }

        rescore(rules);
        update_rate_window();

// display the current statistics
//        win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
        display_statistics(statistics.summary_string(rules));

        const string score_str = pad_string(separated_string(statistics.points(rules), TS), win_score.width() - string("Score: ").length());

        win_score < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Score: " <= score_str;
      }

// .RESET RBN -- get a new connection
      if (command == "RESET RBN")
      { //win_message <= "Resetting RBN connection";
        static pthread_t thread_id_reset;
        const int ret = pthread_create(&thread_id_reset, &(attr_detached.attr()), reset_connection, rbn_p);

        if (ret)
          alert("Error creating reset_connection thread");

        //win_message <= "RBN connection";
      }

      processed = true;
    }

// send to the scratchpad?
    if (!processed and contents[0] == '\\')
    { const string scratchpad_str = substring(hhmmss(), 0, 5) + " " + rig.rig_frequency().display_string() + " " + substring(contents, 1);

      win_scratchpad < WINDOW_SCROLL_UP < WINDOW_BOTTOM_LEFT <= scratchpad_str;
      win <= WINDOW_CLEAR;

      processed = true;
    }

// is it a frequency? Could check exhaustively with a horrible regex, but this is clearer and we would have to parse it anyway
// something like (\+|-)?([0-9]+(\.[0-9]+))
// if there's a letter, it's not a frequency; otherwise we try to parse as a frequency

// regex support is currently hopelessly borked in g++. It compiles and links, but then crashes at runtime
// for current status of regex support, see: http://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.tr1

    if (!processed)
    { const bool contains_letter = (contents.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != string::npos);

      if (!contains_letter)    // try to parse as frequency
      { const bool contains_plus  = (contents[0] == '+');        // this can be entered from the keypad w/o using shift
        const bool contains_minus = (contents[0] == '-');
        double value = from_string<double>(contents);      // what happens when contents can't be parsed as a number?

// if there's a plus or minus we interpret the value as kHz and move up or down from the current QRG
        {
// handle frequency without the MHz part: [n][n]n.n
          if (!contains_plus and !contains_minus and (value < 1000))
          { bool possible_qsy = (contents.length() >= 3);

            possible_qsy = possible_qsy and (contents[contents.size() - 2] == '.');

            if (possible_qsy)
            { const BAND cur_band = safe_get_band();
              float band_edge_in_khz = rig.rig_frequency().lower_band_edge().khz();

              switch (cur_band)
              { case BAND_160 :
                { value += (value < 100 ? 1800 : 1000);
                  break;
                }

                case BAND_80 :
                { value += (value < 100 ? 3500 : 3000);
                  break;
                }

                case BAND_40 :
                case BAND_20 :
                case BAND_15 :
                case BAND_10 :
                { value += band_edge_in_khz;
                  break;
                }

                default :
                  break;
              }
            }
          }

          const frequency new_frequency( (contains_plus or contains_minus) ? rig.rig_frequency().hz() + (value * 1000) : value);
          const BAND new_band = to_BAND(new_frequency);
          bool valid = ( rules.permitted_bands_set() < new_band );

//          ost << "new frequency = " << new_frequency.hz() << endl;
//          ost << "new band = " << new_band << " : " << BAND_NAME[new_band] << endl;

          if ( (valid) and (new_band == BAND_160))
            valid = ( (new_frequency.hz() >= 1800000) and (new_frequency.hz() <= 2000000) );

          if (valid)
          { BAND cur_band = safe_get_band();
            MODE cur_mode = safe_get_mode();

            rig.set_last_frequency(cur_band, cur_mode, rig.rig_frequency());             // save current frequency
            rig.rig_frequency(new_frequency);

//          const BAND new_band = static_cast<BAND>(new_frequency);

            if (new_band != cur_band)
            { cur_band = new_band;
              display_band_mode(win_band_mode, cur_band, cur_mode);

//              SAFELOCK(current_band);
//              current_band = new_band;
              safe_set_band(new_band);

              bandmap& bm = bandmaps[cur_band];
              win_bandmap <= bm;

              win_bandmap_filter < WINDOW_CLEAR < CURSOR_START_OF_LINE < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();

// update displays of needed mults
              update_remaining_callsign_mults_window(statistics, string(), cur_band, cur_mode);
              update_remaining_country_mults_window(statistics, cur_band, cur_mode);
              update_remaining_exchange_mults_windows(rules, statistics, cur_band);
            }

            enter_sap_mode();    // we want to be in SAP mode after a frequency change

            win <= WINDOW_CLEAR;
          }
          else // not valid frequency
          { alert(string("Invalid frequency: ") + to_string(new_frequency.hz()) + " Hz");
          }

          processed = true;
        }
      }
    }

// don't treat as a call if it contains weird characters
    if (!processed)
      processed = (contents.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/?") != string::npos);

// assume it's a call
    if (!processed)
    { const string& callsign = contents;
      const BAND cur_band = safe_get_band();
      const MODE cur_mode = safe_get_mode();
      const bool is_dupe = logbk.is_dupe(callsign, cur_band, cur_mode, rules);

// if we're in SAP mode, don't call him if he's a dupe
      if (drlog_mode == SAP_MODE and is_dupe)
      { const cursor posn = win.cursor_position();

        win < WINDOW_CLEAR < CURSOR_START_OF_LINE < (contents + " DUPE") <= posn;

        extract = logbk.worked( callsign );
        extract.display();

        { bandmap_entry be;

          be.freq(rig.rig_frequency());
          be.callsign(contents);

          const location_info li = location_db.info(contents);
          const string canonical_prefix = li.canonical_prefix();

          be.canonical_prefix(canonical_prefix);
          be.continent(li.continent());
          be.band(static_cast<BAND>(be.freq()));
          be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);
          be.calculate_mult_status(rules, statistics);
          be.is_needed(false);

          bandmap& bandmap_this_band = bandmaps[cur_band];

          bandmap_this_band += be;
          win_bandmap <= bandmap_this_band;

          { SAFELOCK(dupe_check);
            last_call_inserted_with_space = contents;
          }
        }

        processed = true;
      }
      else    // CQ_MODE or not a dupe
      {
// send the call
        if ( (cur_mode == MODE_CW) and (cw_p) )
        { if (drlog_mode == CQ_MODE)
          { (*cw_p) << callsign;

            SAFELOCK(last_exchange);  // we need to keep track of the last message, so it can be re-sent

            last_exchange = expand_cw_message( context.exchange_cq() );
            (*cw_p) << last_exchange;
            last_exchange = callsign /* + " " */ + last_exchange;  // add the call so that we can re-send the entire sequence easily
          }
          else
            (*cw_p) << cwm[XK_KP_0];    // send contents of KP0, which is assumed to be my call
        }

//        ost << "message sent" << endl;

// what exchange do we expect?
        string exchange_str;
        const string canonical_prefix = location_db.canonical_prefix(contents);

        const vector<exchange_field> expected_exchange = rules.exch(canonical_prefix, cur_mode);
        map<string, string> mult_exchange_field_value;                  // the values of exchange fields that are mults

        ost << "Expected exchange for " << contents << " is: ";
        for (const auto& ef : expected_exchange)
          ost << ef.name() << " ";
        ost << endl;

        for (const auto& exf : expected_exchange)
        { ost << "Exchange field: " << exf << endl;
          bool processed_field = false;

// &&& if it's a choice, try to figure out which one to display; in IARU, it's the zone unless the society isn't empty;
// need to figure out a way to generalise all this
          if (exf.is_choice())
          { ost << "Exchange field " << exf.name() << " is a choice" << endl;

            if (exf.name() == "ITUZONE+SOCIETY")
            { ost << "Attempting to handle ITUZONE+SOCIETY exchange field" << endl;

              const string society_guess = exchange_db.guess_value(contents, "SOCIETY");
              ost << "society guess for " << contents << " = " << society_guess << endl;

              string iaru_guess = society_guess;

              if (iaru_guess.empty())
              { const string itu_zone_guess = to_upper(exchange_db.guess_value(contents, "ITUZONE"));
                ost << "ITU zone guess for " << contents << " = " << itu_zone_guess << endl;

                iaru_guess = itu_zone_guess;
              }

              exchange_str += iaru_guess;
              processed_field = true;
            }

            if (exf.name() == "10MSTATE+SERNO")
            { //ost << "Attempting to handle 10MSTATE+SERNO exchange field" << endl;    // &&&

              static const set<string> state_multiplier_countries( { "K", "VE", "XE" } );
              const string canonical_prefix = location_db.canonical_prefix(contents);
              string state_guess;

              if (state_multiplier_countries < canonical_prefix)
              { state_guess = exchange_db.guess_value(contents, "10MSTATE");
              }

              ost << "state guess for " << contents << " = " << state_guess << endl;

              exchange_str += state_guess;
              processed_field = true;
            }

            if (exf.name() == "HADXC+QTHX[HA]")
            {  ost << "Attempting to handle HADXC+QTHX[HA] exchange field" << endl;

              string guess = exchange_db.guess_value(contents, "HADXC");

              ost << "HADXC guess = " << guess << endl;

              if (guess.empty())
                guess = exchange_db.guess_value(contents, "QTHX[HA]");

              ost << "best guess = " << guess << endl;

              if (!guess.empty())
              { exchange_str += guess;
                processed_field = true;
              }

            }

          }

          ost << "Before RST, exf.name() = " << exf.name() << endl;
          ost << "Before RST, exchange_str = " << exchange_str << endl;

          if (exf.name() == "RST")
          { if (cur_mode == MODE_CW /* or current_mode == MODE_DIGI */ )
              exchange_str += "599 ";
            else
              exchange_str += "59 ";

            processed_field = true;
          }

          if (!processed_field and exf.name() == "RS")
          { exchange_str += "59 ";

            processed_field = true;
          }

          if (!processed_field)
          { if (!(variable_exchange_fields < exf.name()))
            { ost << "!processed and !variable exchange field: " << exf.name() << endl;


               ost << "about to guess" << endl;

              const string guess = rules.canonical_value(exf.name(), exchange_db.guess_value(contents, exf.name()));

              ost << "guess is: " << guess << endl;

              if (!guess.empty())
              { if ((exf.name() == "RDA") and (guess.length() == 2))  // RDA guess might just have first two characters
                  exchange_str += guess;
                else
                { exchange_str += guess + " ";

                  if (exf.is_mult())                 // save the expected value of this field
                    mult_exchange_field_value.insert( { exf.name(), guess } );
                }
              }
            }
          }

          processed = true;
        }

//        ost << "exchange_str: " << exchange_str << endl;

        update_known_callsign_mults(callsign);
        update_known_country_mults(callsign);

//        ost << "about to send to EXCHANGE window" << endl;

        win_exchange <= exchange_str;
        win_active_p = &win_exchange;
      }

//      ost << "about to create bandmap entry [1]" << endl;
//      sleep_for(seconds(5));

// add to bandmap if we're in SAP mode
      if (drlog_mode == SAP_MODE)
      { //ost << "about to create bandmap entry [2]" << endl;

        bandmap_entry be;

//        ost << "about to set frequency" << endl;

        be.freq(rig.rig_frequency());  // also sets band

//        ost << "about to set callsign" << endl;

        be.callsign(callsign);

//        ost << "about to set band" << endl;

//        be.band(cur_band);

 //       ost << "about to set expiration time" << endl;

        be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);
        be.is_needed(!is_dupe);

//        ost << "about to calculate mult status" << endl;

        be.calculate_mult_status(rules, statistics);

//        ost << "mult status calculated" << endl;

        bandmap& bandmap_this_band = bandmaps[cur_band];
        const bandmap_entry old_be = bandmap_this_band[callsign];

        if (!old_be.callsign().empty() and (old_be.frequency_str() != be.frequency_str()))  // update bandmap only if there's a change
        { bandmap_this_band += be;
          win_bandmap <= bandmap_this_band;
        }
      }
    }
  }                 // end of ENTER

// CTRL-ENTER -- assume it's a call or partial call and go to the call if it's in the bandmap
  if (!processed and e.is_control() and (e.symbol() == XK_Return))
  { string contents = remove_peripheral_spaces(win.read());
    bool found_call = false;

// assume it's a call -- look for the same call in the current bandmap
    bandmap_entry be = bandmaps[safe_get_band()][contents];

    if (!(be.callsign().empty()))
    { found_call = true;
      rig.rig_frequency(be.freq());
      enter_sap_mode();
    }
    else    // didn't find an exact match; try a substring search
    { be = bandmaps[safe_get_band()].substr(contents);

      if (!(be.callsign().empty()))
      { found_call = true;
        win_call < WINDOW_CLEAR < CURSOR_START_OF_LINE <= be.callsign();  // put callsign into CALL window
        contents = be.callsign();
        rig.rig_frequency(be.freq());
        enter_sap_mode();
      }
    }

    if (found_call)
    { contents = remove_peripheral_spaces(win.read());
      display_call_info(contents);

      SAFELOCK(dupe_check);
      last_call_inserted_with_space = contents;
    }

    processed = true;
  }

// KP ENTER -- send CQ #2
  if (!processed and (e.symbol() == XK_KP_Enter))
  { const string contents = remove_peripheral_spaces(win.read());

// if empty, send CQ #2
    if (contents.empty())
    { if ( (safe_get_mode() == MODE_CW) and (cw_p) and (drlog_mode == CQ_MODE))
      { const string msg = context.message_cq_2();

        if (!msg.empty())
          (*cw_p) << msg;
      }
//      processed = true;
    }

    processed = true;
  }

// CTRL-KP-ENTER -- look for, and then display, entry in all the bandmaps
  if (!processed and e.is_control() and (e.symbol() == XK_KP_Enter))
  { const string contents = remove_peripheral_spaces(win.read());
    const set<BAND> permitted_bands { rules.permitted_bands().cbegin(), rules.permitted_bands().cend() };
    string results;

    for (const auto& b : permitted_bands)
    { bandmap& bm = bandmaps[b];        // use current bandmap to make it easier to display column offset

      const bandmap_entry be = bm[contents];

      if (!be.empty())
      { if (!results.empty())
          results += " ";
        results += be.frequency_str();
      }
    }

    if (results.empty())
      results = contents + ": No posts found";
    else
      results = contents + ": " + results;

    win_message < WINDOW_CLEAR <= results;

    processed = true;
  }

// SPACE -- generally, dupe check
  if (!processed and (e.is_char(' ')))
  {
// if we're inside a command, just insert a space in the window; also if we are writing a comment
    string contents = remove_peripheral_spaces(win.read());

    if ( (contents.size() > 1 and contents[0] == '.') or contains(contents, "\\"))
      win <= " ";
    else        // not inside a command
    {
// possibly put a bandmap call into the call window
      if (contents.empty() and drlog_mode == SAP_MODE)
      { const string dupe_contents = remove_peripheral_spaces(win_nearby.read());

        if (!dupe_contents.empty())
        { win < CURSOR_START_OF_LINE <= dupe_contents;
          display_call_info(dupe_contents);
        }
      }

      contents = remove_peripheral_spaces(win.read());    // contents of CALL window may have changed, so we may need to re-insert/refresh call in bandmap

// dupe check; put call into bandmap
      if (!contents.empty() and drlog_mode == SAP_MODE and !contains(contents, " DUPE"))
      {
// possibly add the call to known mults
        update_known_callsign_mults(contents);
        update_known_country_mults(contents);

        bandmap_entry be;                        // default source is BANDMAP_ENTRY_LOCAL
        const BAND cur_band = safe_get_band();
        const MODE cur_mode = safe_get_mode();

        be.freq(rig.rig_frequency());
        be.callsign(contents);

        const location_info li = location_db.info(contents);
        const string canonical_prefix = li.canonical_prefix();

        be.canonical_prefix(canonical_prefix);
        be.continent(li.continent());
        be.band(static_cast<BAND>(be.freq()));
        be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);

// do we still need this guy?
        const bool is_needed = is_needed_qso(contents, cur_band, safe_get_mode());

        if (!is_needed /* worked_this_band_mode */)
        { const cursor posn = win.cursor_position();
          win < WINDOW_CLEAR < CURSOR_START_OF_LINE < (contents + " DUPE") <= posn;
        }

        be.calculate_mult_status(rules, statistics);
        be.is_needed(is_needed);

        bandmap& bandmap_this_band = bandmaps[cur_band];

        bandmap_this_band += be;
        win_bandmap <= bandmap_this_band;

        { SAFELOCK(dupe_check);
          last_call_inserted_with_space = contents;
        }

        update_remaining_callsign_mults_window(statistics, string(), cur_band, cur_mode);
        update_remaining_country_mults_window(statistics, cur_band, cur_mode);
        update_remaining_exchange_mults_windows(rules, statistics);
      }
    }

    processed = true;
  }

// CTRL-LEFT-ARROW, CTRL-RIGHT-ARROW, ALT-LEFT_ARROW, ALT-RIGHT-ARROW: up or down to next needed QSO or next needed mult. Uses filtered bandmap
  if (!processed and (e.is_control() or e.is_alt()) and ( (e.symbol() == XK_Left) or (e.symbol() == XK_Right)))
  { bandmap& bm = bandmaps[safe_get_band()];

    typedef const bandmap_entry (bandmap::* MEM_FUN_P)(const enum BANDMAP_DIRECTION);    // syntactic sugar
    MEM_FUN_P fn_p = e.is_control() ? &bandmap::needed_qso : &bandmap::needed_mult;      // which predicate function will we use?

    const bandmap_entry be = (bm.*fn_p)( (e.symbol() == XK_Left) ? BANDMAP_DIRECTION_DOWN : BANDMAP_DIRECTION_UP);  // get the next stn/mult

    if (!be.empty())
    { rig.rig_frequency(be.freq());
      win_call < WINDOW_CLEAR <= be.callsign();

      display_nearby_callsign(be.callsign());
      display_call_info(be.callsign());
      last_call_inserted_with_space = be.callsign();
    }

    processed = true;
  }

// SHIFT (RIT control)
// RIT changes via hamlib, at least on the K3, are *very* slow
  if (!processed and (e.event() == KEY_PRESS) and ( (e.symbol() == XK_Shift_L) or (e.symbol() == XK_Shift_R) ) )
  { rit_control(e);

    processed = true;
  }

// ALT-Y -- delete last QSO
  if (!processed and e.is_alt('y'))
  { if (remove_peripheral_spaces(win.read()).empty())                // process only if empty
    { if (!logbk.empty())
      { const QSO qso = logbk[logbk.n_qsos()];

        logbk.remove_last_qso();

// remove the visual indication in the visible log
        bool cleared = false;

        for (auto line_nr = 0; line_nr < win_log.height() and !cleared; ++line_nr)
        { if (!win_log.line_empty(line_nr))
          { win_log.clear_line(line_nr);
            cleared = true;
          }
        }

// rebuild the history
        rebuild_history(logbk, rules, statistics, q_history, rate);

// rescore the log
        rescore(rules);
        update_rate_window();

        if (!scp_db.contains(qso.callsign()))
          scp_dbs.remove_call(qso.callsign());

        if (!fuzzy_db.contains(qso.callsign()))
          fuzzy_dbs.remove_call(qso.callsign());

// display the current statistics
//        win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
        display_statistics(statistics.summary_string(rules));

        const string score_str = pad_string(separated_string(statistics.points(rules), TS), win_score.width() - string("Score: ").length());

        win_score < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Score: " <= score_str;

        octothorpe = (octothorpe == 0 ? octothorpe : octothorpe -1);
        win_serial_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= serial_number_string(octothorpe);

        next_qso_number = (next_qso_number == 0 ? next_qso_number : next_qso_number -1);
        win_qso_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= pad_string(to_string(next_qso_number), win_qso_number.width());

        update_remaining_callsign_mults_window(statistics, string(), safe_get_band(), safe_get_mode());
        update_remaining_country_mults_window(statistics, safe_get_band(), safe_get_mode());
        update_remaining_exchange_mults_windows(rules, statistics);

// remove the last line from the log on disk
        string disk_log = read_file(context.logfile());
        const vector<string> disk_log_lines = to_lines(disk_log);

        if (!disk_log_lines.empty())
        { FILE* fp = fopen(context.logfile().c_str(), "w");

          for (size_t n = 0; n < disk_log_lines.size() - 1; ++n)  // don't include last QSO
          { const string line_to_write = disk_log_lines[n] + EOL;

            fwrite(line_to_write.c_str(), line_to_write.length(), 1, fp);
          }

          fclose(fp);
        }
      }
    }

    processed = true;
  }

// CURSOR UP -- go to log window
  if (!processed and e.is_unmodified() and e.symbol() == XK_Up)
  { win_active_p = &win_log;

    win_log_snapshot = win_log.snapshot();
    win_log.toggle_hidden();

    win_log <= cursor(0, 0);

    processed = true;
  }

// CURSOR DOWN -- possibly replace call with SCP info; NB this assumes COLOUR_GREEN and COLOUR_RED are the hardwired colours in the SCP window
  if (!processed and e.is_unmodified() and e.symbol() == XK_Down)
  { string new_callsign = match_callsign(scp_matches);

    if (new_callsign.empty())
      new_callsign = match_callsign(fuzzy_matches);

    if (!new_callsign.empty())
    { win < WINDOW_CLEAR < CURSOR_START_OF_LINE <= new_callsign;
      display_call_info(new_callsign);
    }

    processed = true;
  }

// CTRL-CURSOR DOWN -- possibly replace call with fuzzy info; NB this assumes COLOUR_GREEN and COLOUR_RED are the hardwired colours in the SCP window
  if (!processed and e.is_ctrl() and e.symbol() == XK_Down)
  { const string new_callsign = match_callsign(fuzzy_matches);

    if (!new_callsign.empty())
    { win < WINDOW_CLEAR < CURSOR_START_OF_LINE <= new_callsign;
      display_call_info(new_callsign);
    }

    processed = true;
  }

// ALT-KP+ -- increment octothorpe
  if (!processed and e.is_alt() and e.symbol() == XK_KP_Add)
  { win_serial_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= serial_number_string(++octothorpe);
    processed = true;
  }

// ALT-KP- -- decrement octothorpe
  if (!processed and e.is_alt() and e.symbol() == XK_KP_Subtract)
  { win_serial_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= serial_number_string(--octothorpe);
    processed = true;
  }

// CTRL-KP+ -- increment qso number
  if (!processed and e.is_ctrl() and e.symbol() == XK_KP_Add)
  { win_qso_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= pad_string(to_string(++next_qso_number), win_qso_number.width());
    processed = true;
  }

// CTRL-KP- -- decrement qso number
  if (!processed and e.is_ctrl() and e.symbol() == XK_KP_Subtract)
  { win_qso_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= pad_string(to_string(--next_qso_number), win_qso_number.width());
    processed = true;
  }

// KP Del -- remove from bandmap and add to do-not-add list (like .REMOVE)
  if (!processed and e.symbol() == XK_KP_Delete)
  { const string callsign = remove_peripheral_spaces(win.read());

//    for_each(bandmaps.begin(), bandmaps.end(), [=] (bandmap& bm) { bm -= callsign;
//                                                                   bm.do_not_add(callsign);
//                                                                 } );

    FOR_ALL(bandmaps, [=] (bandmap& bm) { bm -= callsign;
                                          bm.do_not_add(callsign);
                                        } );

    win_bandmap <= (bandmaps[safe_get_band()]);

    processed = true;
  }

// ` -- SWAP RIT and XIT
  if (!processed and e.is_char('`'))
  { swap_rit_xit();
    processed = true;
  }

// ALT-P -- Dump P3
  if (!processed and e.is_alt('p'))
  { p3_screenshot();

    processed = true;
  }

// CTRL-P -- dump screen
  if (!processed and e.is_control('p'))
  { dump_screen();

    processed = true;
  }

// ALT-D -- debug dump
  if (!processed and e.is_alt('d'))
  { debug_dump();
    processed = true;
  }

// ALT-Q -- send QTC
  if (!processed and e.is_alt('q') and rules.send_qtcs())
  { last_active_win_p = win_active_p;
    win_active_p = &win_log_extract;
    win_active_p-> process_input(e);  // reprocess the alt-q

    processed = true;
  }

// CTRL-S -- toggle split
  if (!processed and e.is_control('s'))
  { try
    { if (rig.split_enabled())
        rig.split_disable();
      else
        rig.split_enable();
    }

    catch (const rig_interface_error& e)
    { alert( (string)"Error toggling split: " + e.reason());
    }

    processed = true;
  }

// ALT-S -- toggle sub receiver
  if (!processed and e.is_alt('s'))
  { try
    { if (rig.sub_receiver_enabled())
      { //ost << "going to disable SUBRX" << endl;
        rig.sub_receiver_disable();
      //ost << "SUBRX is now " << (rig.sub_receiver_enabled() ? "enabled" : "disabled") << endl;
      }
      else
      { //ost << "going to enable SUBRX" << endl;
        rig.sub_receiver_enable();
      //ost << "SUBRX is now " << (rig.sub_receiver_enabled() ? "enabled" : "disabled") << endl;
      }
    }

    catch (const rig_interface_error& e)
    { alert( (string)"Error toggling SUBRX: " + e.reason());
    }

    processed = true;
  }

// ALT-ENTER; VFO B
  if (!processed and e.is_alt() and (e.symbol() == XK_Return))
  { const string contents = remove_peripheral_spaces(win.read());

// try to parse as frequency
    const bool contains_letter = (contents.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != string::npos);
    const frequency f_b = rig.rig_frequency_b();;

    if (!contains_letter)    // try to parse as frequency
    { const bool contains_plus  = (contents[0] == '+');        // this can be entered from the keypad w/o using shift
      const bool contains_minus = (contents[0] == '-');
      double value = from_string<double>(contents);      // what happens when contents can't be parsed as a number?
//      const frequency f_b = rig.rig_frequency_b();;

// if there's a plus or minus we interpret the value as kHz and move up or down from the current QRG
      {
// handle frequency without the MHz part: [n][n]n.n
        if (!contains_plus and !contains_minus and (value < 1000))
        { bool possible_qsy = (contents.length() >= 3);

          possible_qsy = possible_qsy and (contents[contents.size() - 2] == '.');

          if (possible_qsy)
          {
// get band of VFO B
//            frequency f_b = rig.rig_frequency_b();
            const BAND band_b = to_BAND(f_b);
            float band_edge_in_khz = f_b.lower_band_edge().khz();

//            ost << "f_b = " << f_b.display_string() << endl;
//            ost << "band_b = " << band_b << endl;
//            ost << "band edge = " << band_edge_in_khz << endl;

            switch (band_b)
            { case BAND_160 :
              { value += (value < 100 ? 1800 : 1000);
                break;
              }

              case BAND_80 :
              { value += (value < 100 ? 3500 : 3000);
                break;
              }

              case BAND_40 :
              case BAND_20 :
              case BAND_15 :
              case BAND_10 :
              { value += band_edge_in_khz;
                break;
              }

              default :
                break;
            }

//            ost << "value = " << value << endl;

          }
        }

        const frequency new_frequency_b( (contains_plus or contains_minus) ? f_b.hz() + (value * 1000) : value);
        rig.rig_frequency_b(new_frequency_b); // don't call set_last_frequency for VFO B
      }

      win_call < WINDOW_CLEAR <= CURSOR_START_OF_LINE;
      processed = true;
    }

// don't treat as a call if it contains weird characters
    if (!processed)
      processed = (contents.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/?") != string::npos);

// assume it's a call or partial call and go to the call if it's in the bandmap
    if (!processed)
    { const BAND band_b = to_BAND(f_b);

// assume it's a call -- look for the same call in the VFO B bandmap
      bandmap_entry be = bandmaps[band_b][contents];

      if (!(be.callsign().empty()))
        rig.rig_frequency_b(be.freq());
      else    // didn't find an exact match; try a substring search
      { be = bandmaps[band_b].substr(contents);

        if (!(be.callsign().empty()))
        { rig.rig_frequency_b(be.freq());

          win_call < WINDOW_CLEAR <= CURSOR_START_OF_LINE;
        }
      }
    }

    processed = true;
  }

// ALT-> VFO A -> VFO B
  if (!processed and (e.is_alt('>') or e.is_alt('.')))
  { rig.rig_frequency_b(rig.rig_frequency());

    processed = true;
  }

// ALT-< VFO B -> VFO A
  if (!processed and (e.is_alt('<') or e.is_alt(',')))
  { rig.rig_frequency(rig.rig_frequency_b());

    processed = true;
  }

// finished processing a keypress
  if (processed and win_active_p == &win_call)  // we might have changed the active window (if sending a QTC)
  { if (win_call.empty())
    { win_info <= WINDOW_CLEAR;
      win_batch_messages <= WINDOW_CLEAR;
      win_individual_messages <= WINDOW_CLEAR;
    }
    else
    { const string current_contents = remove_peripheral_spaces(win.read());

      if (current_contents != prior_contents)
      { update_scp_window(current_contents);
        update_fuzzy_window(current_contents);
      }
    }
  }
}

// function to process input to the EXCHANGE window
void process_EXCHANGE_input(window* wp, const keyboard_event& e)
{
// syntactic sugar
  window& win = *wp;

  ost << "processing EXCHANGE input; event string: " << e.str() << endl;

  bool processed = win.common_processing(e);

  if (!processed and (e.is_char('/') or e.is_char(' ')))
  { win <= e.str();
    processed = true;
  }

// CW messages
  if (!processed and cw_p and (safe_get_mode() == MODE_CW))
  { if (e.is_unmodified() and (keypad_numbers < e.symbol()) )
    { ost << "sending CW message: " << expand_cw_message(cwm[e.symbol()]) << endl;
      (*cw_p) << expand_cw_message(cwm[e.symbol()]);
      processed = true;
    }
  }

// PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
  if (!processed and ((e.symbol() == XK_Next) or (e.symbol() == XK_Prior)))
  { int change = (e.is_control() ? 1 : 3);

    if (e.symbol() == XK_Prior)
      change = -change;

    if (cw_p)
    { cw_p->speed(cw_p->speed() - change);
      win_wpm < WINDOW_CLEAR < CURSOR_START_OF_LINE <= (to_string(cw_p->speed()) + " WPM");

      try
      { if (context.sync_keyer())
          rig.keyer_speed(cw_p->speed());
      }

      catch (const rig_interface_error& e)
      { alert("Error setting CW speed on rig");
      }
    }

    processed = true;
  }

// ALT-K -- toggle CW
  if (!processed and e.is_alt('k') and cw_p)
  { cw_p->toggle();

    win_wpm < WINDOW_CLEAR < CURSOR_START_OF_LINE <= (cw_p->disabled() ? "NO CW" : (to_string(cw_p->speed()) + " WPM") );

    processed = true;
  }

// ESCAPE
  if (!processed and e.symbol() == XK_Escape)
  { if (cw_p)
  	{ if (!cw_p->empty())    // abort sending CW if we are currently sending
  	  { cw_p->abort();
  	    processed = true;
  	  }
  	}

 // clear the exchange window if there's something in it
    if (!processed and (!remove_peripheral_spaces(win.read()).empty()))
    { win <= WINDOW_CLEAR;
      processed = true;
    }

// go back to the call window
    if (!processed)
    { win_active_p = &win_call;
      win_call <= CURSOR_END_OF_LINE;
      processed = true;
    }
  }

// COMMA -- place contents of call window into this window, preceeded by a dot
  if (!processed and (e.is_char(',')))
  { static const string full_stop(".");

    win <= (full_stop + remove_peripheral_spaces(win_call.read()));
    processed = true;
  }

// FULL STOP
  if (!processed and (e.is_char('.')))
  { static const string full_stop(".");

//    win <= (full_stop + remove_peripheral_spaces(win_call.read()));
    win <= full_stop;
    processed = true;
  }

// ALT-KP_4: decrement bandmap column offset; ALT-KP_6: increment bandmap column offset
  if (!processed and e.is_alt() and ( (e.symbol() == XK_KP_4) or (e.symbol() == XK_KP_6)
                                     or(e.symbol() == XK_KP_Left) or (e.symbol() == XK_KP_Right) ) )
  { bandmap& bm = bandmaps[safe_get_band()];

    bm.column_offset(bm.column_offset() + ( (e.symbol() == XK_KP_6 ) or (e.symbol() == XK_KP_Right ) ) ? 1 : -1);

    alert(string("Bandmap column offset set to: ") + to_string(bm.column_offset()));

    win_bandmap <= bm;
    win_bandmap_filter < WINDOW_CLEAR < "[" < to_string(bm.column_offset()) < "] " <= bm.filter();

    processed = true;
  }

// ENTER, KP_ENTER, ALT -Q -- thanks and log the contact; also perhaps start QTC process
  bool log_the_qso = !processed and e.is_unmodified() and ( (e.symbol() == XK_Return) or (e.symbol() == XK_KP_Enter) );
  bool send_qtc = false;

  if (!log_the_qso)
  { log_the_qso = !processed and e.is_alt('q') and rules.send_qtcs();
    send_qtc = log_the_qso;
  }

  if (log_the_qso)
  { const BAND cur_band = safe_get_band();
    const MODE cur_mode = safe_get_mode();
    const string call_contents = remove_peripheral_spaces(win_call.read());

    ost << "going to log QSO with " << call_contents << endl;

    const string canonical_prefix = location_db.canonical_prefix(call_contents);
    const string exchange_contents = squash(remove_peripheral_spaces(win_exchange.read()));
    const vector<string> exchange_field_values = split_string(exchange_contents, ' ');
    const vector<exchange_field> exchange_template = rules.exch(canonical_prefix, cur_mode);

    ost << "Contents of exchange_template:" << endl;

    for (unsigned int n = 0; n < exchange_template.size(); ++n)
      ost << n << ": " << exchange_template[n] << endl;

    unsigned int n_optional_fields = 0;

    FOR_ALL(exchange_template, [&] (const exchange_field& ef) { if (ef.is_optional())
                                                                  n_optional_fields++;
                                                              } );

    bool sent_acknowledgement = false;

    if (!exchange_contents.empty())
    { size_t n_fields_without_new_callsign = 0;

      for (const auto& values : exchange_field_values)
        if (!contains(values, "."))
          n_fields_without_new_callsign++;

//      if ( (exchange_template.size() - n_optional_fields) > exchange_field_values.size())
      if ( (exchange_template.size() - n_optional_fields) > n_fields_without_new_callsign)
      { ost << "mismatched template and exchange fields: Expected " << exchange_template.size() << " exchange fields; found " << exchange_template.size() << " non-replacement-callsign fields" << endl;
        alert("Expected " + to_string(exchange_template.size()) + " exchange fields; found " + to_string(n_fields_without_new_callsign));
        processed = true;
      }

      if (!processed)
      { parsed_exchange pexch(canonical_prefix, rules, cur_mode, exchange_field_values);  // this is relatively slow, but we can't send anything until we know that we have a valid exchange

        ost << "is exchange valid? " << pexch.valid() << endl;
        ost << pexch << endl;

        if (pexch.valid())
        { if ( (cur_mode == MODE_CW) and (cw_p) )  // don't acknowledge yet if we're about to send a QTC
          { if (exchange_field_values.size() == exchange_template.size())    // 1:1 correspondence between expected and received fields
            { if (drlog_mode == CQ_MODE)                                   // send QSL
              { const bool quick_qsl = (e.symbol() == XK_KP_Enter);

                if (!send_qtc)
                  (*cw_p) << expand_cw_message( quick_qsl ? context.quick_qsl_message() : context.qsl_message() );
              }
              else                                                         // SAP exchange
              { if (!send_qtc)
                  (*cw_p) << expand_cw_message( context.exchange_sap() );
                ost << "sent SAP exchange: " << expand_cw_message( context.exchange_sap() ) << endl;
              }
              sent_acknowledgement = true;  // should rename now that we've added QTC processing here
            }
          }

          if (!sent_acknowledgement)
          { if ( (cur_mode == MODE_CW) and (cw_p) and (drlog_mode == SAP_MODE))    // in SAP mode, he doesn't care that we might have changed his call
            { if (!send_qtc)
              { (*cw_p) << expand_cw_message(context.exchange_sap());
                ost << " sent: " << expand_cw_message(context.exchange_sap()) << endl;
              }
            }

            if ( (cur_mode == MODE_CW) and (cw_p) and (drlog_mode == CQ_MODE))    // in CQ mode, he does
            { const vector<string> call_contents_fields = split_string(call_contents, " ");
              const string original_callsign = call_contents_fields[call_contents_fields.size() - 1];
              string callsign = original_callsign;

              if (pexch.has_replacement_call())
                callsign = pexch.replacement_call();

              if (callsign != original_callsign)    // callsign did not change
              { at_call = callsign;
                if (!send_qtc)
                  (*cw_p) << expand_cw_message(context.call_ok_now_message());
              }

// now send ordinary TU message
            const bool quick_qsl = (e.symbol() == XK_KP_Enter);

            if (!send_qtc)
              (*cw_p) << expand_cw_message( quick_qsl ? context.quick_qsl_message() : context.qsl_message() );
          }
        }

// generate the QSO info, then log it
        QSO qso;                        // automatically fills in date and time
        qso.number(next_qso_number);    // set QSO number

        set<pair<string /* field name */, string /* field value */> > exchange_mults_this_qso;    // needed when we do the bandmaps

// callsign is the last entry in the CALL window
        if (!call_contents.empty()) // to speed things up, probably should make this test earlier, before we send the exchange info
        { const vector<string> call_contents_fields = split_string(call_contents, " ");
          const string original_callsign = call_contents_fields[call_contents_fields.size() - 1];
          string callsign = original_callsign;

          if (pexch.has_replacement_call())
            callsign = pexch.replacement_call();

          qso.callsign(callsign);
          qso.canonical_prefix(location_db.canonical_prefix(callsign));
          qso.continent(location_db.continent(callsign));
          qso.mode(cur_mode);
          qso.band(cur_band);
          qso.my_call(context.my_call());
          qso.freq( frequency(rig.rig_frequency()).display_string() );    // in kHz; 1dp

// build name/value pairs for the sent exchange
          vector<pair<string, string> > sent_exchange = context.sent_exchange(qso.mode());

          for (auto& sent_exchange_field : sent_exchange)
          { if (sent_exchange_field.second == "#")
              sent_exchange_field.second = serial_number_string(octothorpe);
          }

          qso.sent_exchange(sent_exchange);

//          ost << "QSO: " << qso << endl;

// build name/value pairs for the received exchange
          vector<received_field>  received_exchange;

#if defined(NEW_CONSTRUCTOR)
          vector<parsed_exchange_field> vec_pef = pexch.chosen_fields(rules);

// keep track of which fields are mults
          for (auto& pef : vec_pef)
            pef.is_mult(rules.is_exchange_mult(pef.name()));

//ost << "vec_pef: " << endl;
//   for (const auto& pef : vec_pef)
//     ost << pef << endl;

//   for (size_t n = 0; n < pexch.n_fields(); ++n)
   for (const auto& pef : vec_pef)
   { const bool is_mult_field = pef.is_mult();

     received_exchange.push_back( { pef.name(), pef.value(), is_mult_field /* pexch.field_is_mult(n) */, false } );

 ost << "added pef: name = " << pef.name() << ", value = " << pef.value() << ", IS " << (is_mult_field ? "" : "NOT ") << "mult" << endl;  // canonical at this point

     if (!(variable_exchange_fields < pef.name()))
       exchange_db.set_value(callsign, pef.name(), pef.value());   // add it to the database of exchange fields

   ost << "canonical value = " << rules.canonical_value(pef.name(), pef.value()) << endl;

// possibly add it to the canonical list, if it's a mult and the value is otherwise unknown
     if (is_mult_field)
     { ost << "Adding canonical value " << pef.value() << " for multiplier exchange field " << pef.name() << ", mult value = " << pef.mult_value() << endl;

       if (!rules.is_canonical_value(pef.name(), pef.mult_value()))
         rules.add_exch_canonical_value(pef.name(), pef.mult_value());
   }
 }



#else

   for (size_t n = 0; n < pexch.n_fields(); ++n)
   { const bool is_mult_field = rules.is_exchange_mult(pexch.field_name(n));

     received_exchange.push_back( { pexch.field_name(n), pexch.field_value(n), is_mult_field /* pexch.field_is_mult(n) */, false } );

 ost << "added pexch: name = " << pexch.field_name(n) << ", value = " << pexch.field_value(n) << ", IS " << (is_mult_field ? "" : "NOT ") << "mult" << endl;  // canonical at this point

     if (!(variable_exchange_fields < pexch.field_name(n)))
       exchange_db.set_value(callsign, pexch.field_name(n), pexch.field_value(n));   // add it to the database of exchange fields

   ost << "canonical value = " << rules.canonical_value(pexch.field_name(n), pexch.field_value(n)) << endl;

// possibly add it to the canonical list, if it's a mult and the value is otherwise unknown
     if (is_mult_field)
     { ost << "Adding canonical value " << pexch.field_value(n) << " for multiplier exchange field " << pexch.field_name(n) << ", mult value = " << pexch.mult_value(n) << endl;

       if (!rules.is_canonical_value(pexch.field_name(n), pexch.mult_value(n)))
         rules.add_exch_canonical_value(pexch.field_name(n), pexch.mult_value(n));
   }
 }




#endif    // NEW_CONSTRUCTOR

#if 0
   for (size_t n = 0; n < pexch.n_fields(); ++n)
          { const bool is_mult_field = rules.is_exchange_mult(pexch.field_name(n));

            received_exchange.push_back( { pexch.field_name(n), pexch.field_value(n), is_mult_field /* pexch.field_is_mult(n) */, false } );

        ost << "added pexch: name = " << pexch.field_name(n) << ", value = " << pexch.field_value(n) << ", IS " << (is_mult_field ? "" : "NOT ") << "mult" << endl;  // canonical at this point

            if (!(variable_exchange_fields < pexch.field_name(n)))
              exchange_db.set_value(callsign, pexch.field_name(n), pexch.field_value(n));   // add it to the database of exchange fields

          ost << "canonical value = " << rules.canonical_value(pexch.field_name(n), pexch.field_value(n)) << endl;

// possibly add it to the canonical list, if it's a mult and the value is otherwise unknown
            if (is_mult_field)
            { ost << "Adding canonical value " << pexch.field_value(n) << " for multiplier exchange field " << pexch.field_name(n) << ", mult value = " << pexch.mult_value(n) << endl;

              if (!rules.is_canonical_value(pexch.field_name(n), pexch.mult_value(n)))
                rules.add_exch_canonical_value(pexch.field_name(n), pexch.mult_value(n));
          }
        }
#endif

        qso.received_exchange(received_exchange);

// is this a country mult?
        if (country_mults_used)
        { update_known_country_mults(qso.callsign());  // does nothing if not auto remaining country mults

          const bool is_country_mult = statistics.is_needed_country_mult(qso.callsign(), cur_band);

//          ost << "QSO with " << qso.callsign() << "; is_country_mult = " << is_country_mult << endl;

          qso.is_country_mult(is_country_mult);
        }

// is this an exchange mult?
        if (exchange_mults_used)
        { const bool is_exchange_mult = calculate_exchange_mults(qso, rules);  // may modify qso

          ost << "QSO is " << (is_exchange_mult ? "" : "not ") << " an exchange mult" << endl;
        }

        ost << "after calculate_exchange_mults; qso = " << qso << endl;

// if callsign mults matter, add more to the qso
        allow_for_callsign_mults(qso);

// get the current list of country mults
        const set<string> old_worked_country_mults = statistics.worked_country_mults(cur_band, cur_mode);

// and any exch multipliers
//        ost << "about to add worked exchange mults to statistics" << endl;

        map<string /* field name */, set<string> /* values */ >   old_worked_exchange_mults = statistics.worked_exchange_mults(cur_band, cur_mode);

// calculate number of exchange mults
        { // ost << "old number of exchange mults = ";

          unsigned int n = 0;

          for (const auto& psss : old_worked_exchange_mults)
            n += psss.second.size();

          ost << n << endl;
        }

        const vector<exchange_field> exchange_fields = rules.expanded_exch(canonical_prefix, qso.mode());

        for (const auto& exch_field : exchange_fields)
        { const string value = qso.received_exchange(exch_field.name());

          ost << "adding " << exch_field.name() << endl;
          ost << "  " << exch_field << endl;

//          statistics.add_worked_exchange_mult(exch_field.name(), value, rules.exchange_mults_per_band() ? cur_band : ALL_BANDS);
          statistics.add_worked_exchange_mult(exch_field.name(), value, qso.band(), qso.mode());

          ost << "finished adding " << exch_field.name() << endl;

        }

        add_qso(qso);  // should also update the rates (but we don't display them yet; we do that after writing the QSO to disk)

// write the log line

        ost << "about to write log line" << endl;

        const auto se = qso.sent_exchange();

//        ost << "sent exchange: size = " << se.size() << endl;
//        for (unsigned int n = 0; n < se.size(); ++n)
//          ost<< "  " <<  se[n].first << " = " << se[n].second;

        win_log < CURSOR_BOTTOM_LEFT < WINDOW_SCROLL_UP <= qso.log_line();

        ost << "log line: " << qso.log_line() << endl;

        win_exchange <= WINDOW_CLEAR;
        win_call <= WINDOW_CLEAR;
        win_nearby <= WINDOW_CLEAR;

        if (send_qtcs)
        { qtc_buf += qso;
          statistics.qtc_qsos_unsent(qtc_buf.n_unsent_qsos());
        }

// display the current statistics
//        win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
        display_statistics(statistics.summary_string(rules));

        const string score_str = pad_string(separated_string(statistics.points(rules), TS), win_score.width() - string("Score: ").length());

        win_score < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Score: " <= score_str;
        win_active_p = &win_call;          // switch to the CALL window
        win_call <= CURSOR_START_OF_LINE;

// remaining mults: callsign, country, exchange
        update_known_callsign_mults(qso.callsign());
        update_remaining_callsign_mults_window(statistics, "", safe_get_band(), safe_get_mode());

        if (old_worked_country_mults.size() != statistics.worked_country_mults(cur_band, cur_mode).size())
        { update_remaining_country_mults_window(statistics, cur_band, cur_mode);
          update_known_country_mults(qso.callsign());
        }

// was the just-logged QSO an exchange mult?
        map<string /* field name */, set<string> /* values */ >   new_worked_exchange_mults = statistics.worked_exchange_mults(cur_band, cur_mode);

        // calculate number of exchange mults
                { ost << "new number of exchange mults = ";

                  unsigned int n = 0;

                  for (const auto& psss : new_worked_exchange_mults)
                    n += psss.second.size();

                  ost << n << endl;
                }

        bool no_exchange_mults_this_qso = true;

        for (map<string, set<string> >::const_iterator cit = old_worked_exchange_mults.begin(); cit != old_worked_exchange_mults.end() and no_exchange_mults_this_qso; ++cit)
        { const size_t old_size = (cit->second).size();
          map<string, set<string> >::const_iterator ncit = new_worked_exchange_mults.find(cit->first);

          if (ncit != new_worked_exchange_mults.end())    // should never be equal
          { const size_t new_size = (ncit->second).size();

            no_exchange_mults_this_qso = (old_size == new_size);

            if (!no_exchange_mults_this_qso)
              update_remaining_exchange_mults_windows(rules, statistics);
          }
        }

// what exchange mults came from this qso? there ought to be a better way of doing this
        ost << "exchange_mults_this_qso = " << boolalpha << !no_exchange_mults_this_qso << noboolalpha << endl;

        if (!no_exchange_mults_this_qso)
        { for (const auto& current_exchange_mult : new_worked_exchange_mults)
          { set<string> difference;

            const auto tmp = old_worked_exchange_mults.find(current_exchange_mult.first);
            const set<string>& current_values = current_exchange_mult.second;

            if (tmp != old_worked_exchange_mults.end())
            { const set<string>& old_values = tmp->second;

              set_difference(current_values.begin(), current_values.end(),
                             old_values.begin(), old_values.end(),
                             inserter(difference, difference.end()));
            }

            if (!difference.empty())  // assume that there's at most one entry
              exchange_mults_this_qso.insert( { current_exchange_mult.first, *(difference.begin()) } );
          }

          if (!exchange_mults_this_qso.empty())
          {
          }
        }

// writing to disk is slow, so start the QTC now, if applicable
        if (send_qtc)
        { last_active_win_p = win_active_p;  // this is now CALL
          win_active_p = &win_log_extract;
          win_active_p-> process_input(e);  // reprocess the alt-q
        }

// write to disk
        append_to_file(context.logfile(), (qso.verbose_format() + EOL) );

// display the current rate (including this QSO)
        update_rate_window();
      }

// perform any changes to the bandmaps

// if we are in CQ mode, then remove this call if it's present elsewhere in the bandmap ... we assume he isn't CQing and SAPing simultaneously on the same band
      bandmap& bandmap_this_band = bandmaps[cur_band];

      if (drlog_mode == CQ_MODE)
      { bandmap_this_band -= qso.callsign();

// possibly change needed status of this call on other bandmaps
        if (!rules.work_if_different_band())
        { for (auto& bmap : bandmaps)
            bmap.not_needed(qso.callsign());
        }
      }
      else    // SAP; if we are in SAP mode, we may need to change the work/mult designation in the bandmap
      {
// add the stn to the bandmap
         bandmap_entry be;

         be.freq(rig.rig_frequency());
         be.callsign(qso.callsign());

         const location_info li = location_db.info(qso.callsign());

         be.canonical_prefix(li.canonical_prefix());
         be.continent(li.continent());
         be.band(cur_band);
         be.expiration_time(be.time() + context.bandmap_decay_time_local() * 60);
         be.is_needed(false);

         bandmap_this_band += be;
      }

// callsign mult status
      if (callsign_mults_used)
      { if (rules.callsign_mults_per_band())
        { for (const auto& callsign_mult_name : rules.callsign_mults())
          { const string target_value = callsign_mult_value(callsign_mult_name, qso.callsign());

            bandmap_this_band.not_needed_callsign_mult(&callsign_mult_value, callsign_mult_name, target_value);
          }
        }
        else
        { for (const auto& callsign_mult_name : rules.callsign_mults())
          { const string target_value = callsign_mult_value(callsign_mult_name, qso.callsign());

            for (auto& bmap : bandmaps)
              bmap.not_needed_callsign_mult(&callsign_mult_value, callsign_mult_name, target_value);
          }
        }
      }

// country mult status
      if (country_mults_used)
      { const string canonical_prefix = location_db.canonical_prefix(qso.callsign());

        if (rules.country_mults_per_band())
          bandmap_this_band.not_needed_country_mult(canonical_prefix /*, location_db */);
        else                           // country mults are not per band
        { for (auto& bmap : bandmaps)
            bmap.not_needed_country_mult(canonical_prefix);
        }
      }

// exchange mult status
      if (exchange_mults_used and !exchange_mults_this_qso.empty())
      { if (rules.exchange_mults_per_band())
        { for (const auto& pss : exchange_mults_this_qso)
            bandmap_this_band.not_needed_exchange_mult(pss.first, pss.second);
        }
        else
        { for (const auto& pss : exchange_mults_this_qso)
          { for (auto& bmap : bandmaps)
              bmap.not_needed_exchange_mult(pss.first, pss.second);
          }
        }
      }

      win_bandmap <= bandmap_this_band;

// keep track of QSO number
          win_serial_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= serial_number_string(++octothorpe);
          next_qso_number = logbk.n_qsos() + 1;
          win_qso_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= pad_string(to_string(next_qso_number), win_qso_number.width());

          display_call_info(qso.callsign(), DO_NOT_DISPLAY_EXTRACT);


        } // end pexch.valid()
        else  // unable to parse exchange
          alert("Unable to parse exchange");

        processed = true;
      }
    }
  }        // end ENTER

// SHIFT -- RIT control
// RIT changes via hamlib, at least on the K3, are *very* slow
  if (!processed and (e.event() == KEY_PRESS) and ( (e.symbol() == XK_Shift_L) or (e.symbol() == XK_Shift_R) ) )
  { rit_control(e);

    processed = true;
  }

// ` -- SWAP RIT and XIT
  if (!processed and e.is_char('`'))
  { swap_rit_xit();
    processed = true;
  }

// ALT-D -- debug dump
  if (!processed and e.is_alt('d'))
  { debug_dump();
    processed = true;
  }

// CTRL-CURSOR LEFT -- left one word
  if (!processed and e.is_ctrl() and e.symbol() == XK_Left)
  { const cursor original_posn = win.cursor_position();

    if (original_posn.x() != 0)    // do nothing if at start of line
    { const string contents = win.read(0, original_posn.y());
      const vector<size_t> word_posn = starts_of_words(contents);

      if (word_posn.empty())                              // there are no words
        win <= CURSOR_START_OF_LINE;
      else                                                // there are some words
      { size_t index;

        for (index = 0; index < word_posn.size(); ++index)
        { if (word_posn[index] == original_posn.x())
          { if (index == 0)                  // we are at the start of the first word
            { win <= CURSOR_START_OF_LINE;
              break;
            }
            else
            { win <= cursor(word_posn[index - 1], original_posn.y());  // are at the start of a word (but not the first word)
              break;
            }
//            win <= ( (index == 0) ? CURSOR_START_OF_LINE : cursor(word_posn[index - 1], original_posn.y()) );  // invalid ternary op
//            break;
          }

          if (word_posn[index] > original_posn.x())
          { if (index == 0)                          // should never happen; cursor is to left of first word
            { win <= CURSOR_START_OF_LINE;
              break;
            }
            else
            { win <= cursor(word_posn[index - 1], original_posn.y());  // go to the start of the current word
              break;
            }
//            win <= ( (index == 0) ? CURSOR_START_OF_LINE : cursor(word_posn[index - 1], original_posn.y()) );  // invalid ternary op
//            break;
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
  { const cursor original_posn = win.cursor_position();
    const string contents = win.read(0, original_posn.y());
    const string truncated_contents = remove_trailing_spaces(contents);

    if (truncated_contents.empty())                // there are no words
      win <= CURSOR_START_OF_LINE;
    else                                           // there is at least one word
    { const vector<size_t> word_posn = starts_of_words(contents);
      const size_t last_filled_posn = truncated_contents.size() - 1;

      if (word_posn.empty())                // there are no words; should never be true at this point
        win <= CURSOR_START_OF_LINE;
      else
      { if (original_posn.x() >= word_posn[word_posn.size() - 1])  // at or after start of last word
          win <= cursor(last_filled_posn + 2, original_posn.y());
        else
        { for (size_t index = 0; index < word_posn.size(); ++index)
          { if (word_posn[index] == original_posn.x())        // at the start of a word (but not the last word)
            { win <= cursor(word_posn[index + 1], original_posn.y());
              break;
            }

            if (word_posn[index] > original_posn.x())
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
  { const cursor original_posn = win.cursor_position();
    const string contents = win.read(0, original_posn.y());
    const vector<size_t> word_posn = starts_of_words(contents);

    if (!word_posn.empty())
    { const bool is_space = (contents[original_posn.x()] == ' ');

      if (!is_space)
      { size_t start_current_word = 0;

        for (size_t n = 0; n < word_posn.size(); ++n)
          if (word_posn[n] <= original_posn.x())
            start_current_word = word_posn[n];

        const size_t end_current_word = contents.find_first_of(" ", original_posn.x());

        if (end_current_word != string::npos)  // word is followed by space
        { string new_contents;
          if (start_current_word != 0)
            new_contents = substring(contents, 0, start_current_word);

          new_contents += substring(contents, end_current_word + 1);

          win < WINDOW_CLEAR < new_contents <= cursor(start_current_word, original_posn.y());

//          win <= cursor(start_current_word, original_posn.y());
        }
        else
        { //const string new_contents = substring(contents, 0, next_start - 1);
          string new_contents;
          if (start_current_word != 0)
            new_contents = substring(contents, 0, start_current_word - 1);

          win < WINDOW_CLEAR < new_contents <= cursor(start_current_word, original_posn.y());
//          win <= original_posn;
//          win <= cursor(start_current_word, original_posn.y());
        }
      }
      else  // we are at a space
      { const size_t next_start = next_word_posn(contents, original_posn.x());

//        ost << "x_0 = " << original_posn.x() << endl;
//        ost << "next_start = " << next_start << endl;

        if (next_start != string::npos)
        { const size_t next_end = contents.find_first_of(" ", next_start);

          if (next_end != string::npos)    // there is a complete word following
          { const string new_contents = substring(contents, 0, next_start) + substring(contents, next_end + 1);

            win < WINDOW_CLEAR < new_contents <= cursor(original_posn.x() + 1, original_posn.y());
//            win <= cursor(original_posn.x() + 1, original_posn.y());
          }
          else
          { const string new_contents = substring(contents, 0, next_start - 1);

            win < WINDOW_CLEAR < new_contents <= original_posn;
//            win <= original_posn;
          }
        }
      }
    }

    processed = true;
  }

// CTRL-P -- dump screen
  if (!processed and e.is_control('p'))
  { dump_screen();

    processed = true;
  }
}

// function to process input to the (editable) LOG window
void process_LOG_input(window* wp, const keyboard_event& e)
{
// syntactic sugar
  window& win = *wp;

ost << "processing LOG input; event string: " << e.str() << endl;

  bool processed = win.common_processing(e);

  if (!processed and e.is_char(' '))
  { win <= e.str();
    processed = true;
  }

  if (!processed and e.is_unmodified() and e.symbol() == XK_Up)
  { win <= CURSOR_UP;
    processed = true;
  }

  if (!processed and e.is_unmodified() and e.symbol() == XK_Down)
  { const cursor posn = win.cursor_position();
    if (posn.y() != 0)
      win <= CURSOR_DOWN;
    else                                   // bottom line
    { win_log.toggle_hidden();
      win_log < WINDOW_REFRESH;

      const vector<string> new_win_log_snapshot = win_log.snapshot();  // [0] is the top line

      for (size_t n = 0; n < win_log_snapshot.size(); ++n)
        ost << "Original line #" << n << ": " << win_log_snapshot[n] << endl;

      for (size_t n = 0; n < new_win_log_snapshot.size(); ++n)
        ost << " Current line #" << n << ": " << new_win_log_snapshot[n] << endl;

// has anything been changed?
      bool changed = false;
      for (size_t n = 0; !changed and n < new_win_log_snapshot.size(); ++n)
        changed = (new_win_log_snapshot[n] != win_log_snapshot[n]);

      if (changed)
      {
ost << hhmmss() << " starting changed log" << endl;

// get the original QSOs that were in the window
        int number_of_qsos_in_original_window = 0;

        for (size_t n = 0; n < win_log_snapshot.size(); ++n)
          if (!remove_peripheral_spaces(win_log_snapshot[n]).empty())
            number_of_qsos_in_original_window++;

ost << "number of QSOs in original window = " << number_of_qsos_in_original_window << endl;

        deque<QSO> original_qsos;

        unsigned int qso_number = logbk.size();
        unsigned int n_to_remove = 0;

        for (size_t n = 0; n < win_log_snapshot.size(); ++n)
        { if (remove_peripheral_spaces(win_log_snapshot[win_log_snapshot.size() - 1 - n]).empty())
            original_qsos.push_front(QSO());
          else
          { original_qsos.push_front(logbk[qso_number--]);
            n_to_remove++;
          }
        }

// remove that number of QSOs from the log
        for (unsigned int n = 0; n < n_to_remove; ++n)
          logbk.remove_last_qso();

ost << "removed " << n_to_remove << " QSOs from log" << endl;
ost << "length of log is now: " << logbk.size() << endl;
ost << hhmmss() << " about to rebuild history" << endl;

        rebuild_history(logbk, rules, statistics, q_history, rate);

ost << hhmmss() << " finished rebuilding history" << endl;
ost << "Adding new QSO(s)" << endl;

// add the new QSOs
        for (size_t n = 0; n < new_win_log_snapshot.size(); ++n)
        { if (!remove_peripheral_spaces(new_win_log_snapshot[n]).empty())
          { ost << "Adding a qso" << endl;
            ost << "n = " << n << endl;
            ost << "size of old qsos = " << original_qsos.size();

            QSO qso = original_qsos[n];
            qso.log_line();

        ost << "qso.log_line() = " <<  qso.log_line() << endl;
        ost << " new line from snapshot: " << new_win_log_snapshot[n] << endl;
        ost << " old line from snapshot: " << win_log_snapshot[n] << endl;
        ost << " old QSO call from QSO: " << original_qsos[n].callsign() << endl;

            qso.populate_from_log_line(remove_peripheral_spaces(new_win_log_snapshot[n]));  // note that this doesn't fill all fields (e.g. _my_call), which are carried over from original QSO

        ost << " new QSO call from QSO: " << qso.callsign() << endl;

// we can't assume anything about the mult status
            const BAND b = qso.band();
//            ost << "band of reconstituted QSO = " << BAND_NAME[b] << endl;

            update_known_callsign_mults(qso.callsign());
            update_known_country_mults(qso.callsign());

// is this a country mult?
            const bool is_country_mult = statistics.is_needed_country_mult(qso.callsign(), qso.band());
            qso.is_country_mult(is_country_mult);

//            ost << "is_country_mult after QSO has been rebuilt: " << is_country_mult << endl;
//            ost << "is_exchange_mult after QSO has been rebuilt: " << qso.is_exchange_mult() << endl;

// exchange mults
            if (exchange_mults_used)
              calculate_exchange_mults(qso, rules);

// if callsign mults matter, add more to the qso
            allow_for_callsign_mults(qso);

// add it to the running statistics; do this before we add it to the log so we can check for dupes against the current log
            statistics.add_qso(qso, logbk, rules);
            logbk += qso;
          }
        }

        ost << hhmmss() << " added QSOs" << endl;

// re-write the logfile
        { FILE* fp = fopen(context.logfile().c_str(), "w");

          if (fp)
          { const vector<QSO> vec = logbk.as_vector();

            for (const auto& qso : vec)
            { const string line_to_write = qso.verbose_format() + EOL;

              fwrite(line_to_write.c_str(), line_to_write.length(), 1, fp);
            }

            fclose(fp);
          }
          else
            alert("Unable to open log file " + context.logfile() + " for writing: ");
        }
//ost << hhmmss() << " about to do second rebuild" << endl;

// rebuild the history
        rebuild_history(logbk, rules, statistics, q_history, rate);

//ost << hhmmss() << " completed second rebuild" << endl;

// rescore the log
        rescore(rules);

//ost << hhmmss() << " completed rescore" << endl;

        update_rate_window();

        scp_dynamic_db.clear();  // clears cache of parent
        fuzzy_dynamic_db.clear();

        const vector<QSO> qso_vec = logbk.as_vector();

 //       ost << "n QSOs in logbook = " << qso_vec.size() << endl;
        ost << hhmmss() << " about to re-fill databases" << endl;

        for (const auto& qso : qso_vec)
        { const string& callsign = qso.callsign();

          if (!scp_db.contains(callsign) and !scp_dynamic_db.contains(callsign))
            scp_dynamic_db.add_call(callsign);

          if (!fuzzy_db.contains(callsign) and !fuzzy_dynamic_db.contains(callsign))
            fuzzy_dynamic_db.add_call(callsign);
        }

 //       ost << hhmmss() << " re-filled first database" << endl;

//        for (const auto& qso : qso_vec)
//        { //if (!scp_db.contains(qso.callsign()) and !scp_dynamic_db.contains(qso.callsign()))
            //scp_dynamic_db.add_call(qso.callsign());

//          if (!fuzzy_db.contains(qso.callsign()) and !fuzzy_dynamic_db.contains(qso.callsign()))
//            fuzzy_dynamic_db.add_call(qso.callsign());
//        }

//        ost << hhmmss() << " completed re-filling databases" << endl;

//        ost << "About to display; number of QSOs in logbook = " << logbk.size() << endl;

// all the QSOs are added; now display the last few in the log window
        editable_log.recent_qsos(logbk, true);

// display the current statistics
//        win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
        display_statistics(statistics.summary_string(rules));

//        const string score_str = pad_string(comma_separated_string(statistics.points(rules)), win_score.width() - string("Score: ").length());
        const string score_str = pad_string(separated_string(statistics.points(rules), TS), win_score.width() - string("Score: ").length());
        win_score < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Score: " <= score_str;

        update_remaining_country_mults_window(statistics, safe_get_band(), safe_get_mode());
        update_remaining_exchange_mults_windows(rules, statistics);
        update_remaining_callsign_mults_window(statistics, "", safe_get_band(), safe_get_mode());

        next_qso_number = logbk.n_qsos() + 1;
        win_qso_number < WINDOW_CLEAR < CURSOR_START_OF_LINE <= pad_string(to_string(next_qso_number), win_qso_number.width());

// perform any changes to the bandmaps;
// there is trickiness here because of threading
// the following is safe, but may lose information if it comes in from the RBN/cluster
// threads while we are processing the bandmap here
//        for (auto& bm : bandmaps)
//          bm.remark();

// test --- change current bandmap
        for (auto& bm : bandmaps)
        { BM_ENTRIES bme = bm.entries();

          for (auto& be : bme)
          { if (be.remark(rules, q_history, statistics))
              bm += be;
          }

          if (&bm == &(bandmaps[safe_get_band()]))
            win_bandmap <= bm;
        }

//        win_bandmap <= bandmaps[safe_get_band()];

        ost << hhmmss() << " all done" << endl;
      }

      win_active_p = &win_call;
      win_call < WINDOW_REFRESH;
    }

    processed = true;
  }

// ALT-Y -- delete current line
  if (!processed and e.is_alt('y'))
  { const cursor posn = win.cursor_position();

    win < CURSOR_START_OF_LINE < WINDOW_CLEAR_TO_EOL <= posn;

    processed = true;
  }

// ESCAPE
  if (!processed and e.symbol() == XK_Escape)
  {
// go back to CALL window, without making any changes
    win_active_p = &win_call;

    win_log.hide_cursor();
    editable_log.recent_qsos(logbk, true);

    win_call < WINDOW_REFRESH;

    processed = true;
  }

// ALT-D -- debug dump
  if (!processed and e.is_alt('d'))
  { debug_dump();
    processed = true;
  }

// CTRL-P -- dump screen
  if (!processed and e.is_control('p'))
  { dump_screen();

    processed = true;
  }
}

// functions that include thread safety
/// get value of <i>current_band</i>
const BAND safe_get_band(void)
{ SAFELOCK(current_band);

  return current_band;
}

/// set value of <i>current_band</i>
void safe_set_band(const BAND b)
{ SAFELOCK(current_band);

  current_band = b;
}

/// get value of <i>current_mode</i>
const MODE safe_get_mode(void)
{ SAFELOCK(current_mode);

  return current_mode;
}

/// set value of <i>current_mode</i>
void safe_set_mode(const MODE m)
{ SAFELOCK(current_mode);

  current_mode = m;
}

/// enter CQ mode
void enter_cq_mode(void)
{
  { SAFELOCK(drlog_mode);  // don't use SAFELOCK_SET because we want to set the frequency and the mode as an atomic operation

    SAFELOCK_SET(cq_mode_frequency_mutex, cq_mode_frequency, rig.rig_frequency());  // nested inside other lock
    drlog_mode = CQ_MODE;
  }

  win_drlog_mode < WINDOW_CLEAR < CURSOR_START_OF_LINE <= "CQ";

  try
  { if (context.cq_auto_lock())
      rig.lock();

    rig.disable_xit();

    if (context.cq_auto_rit())
    { rig.enable_rit();
      rig.rit(0);
    }
  }

  catch (const rig_interface_error& e)
  { alert("Error communicating with rig when entering CQ mode");
  }
}

/// enter SAP mode
void enter_sap_mode(void)
{ SAFELOCK_SET(drlog_mode_mutex, drlog_mode, SAP_MODE);
  win_drlog_mode < WINDOW_CLEAR < CURSOR_START_OF_LINE <= "SAP";

  try
  { rig.unlock();
    rig.rit(0);
    rig.disable_xit();
    rig.disable_rit();
  }

  catch (const rig_interface_error& e)
  { alert("Error communicating with rig when entering SAP mode");
  }
}

/// Toggle between CQ mode and SAP mode
void toggle_drlog_mode(void)
{ if (SAFELOCK_GET(drlog_mode_mutex, drlog_mode) == CQ_MODE)
    enter_sap_mode();
  else
    enter_cq_mode();
}

void update_remaining_callsign_mults_window(running_statistics& statistics, const string& mult_name, const BAND b, const MODE m)
{ const set<string> worked_callsign_mults = statistics.worked_callsign_mults(mult_name, b, m);

//  ost << "update_remaining_callsign_mults_window: mult_name = " << mult_name << ", band = " << b << endl;
//  ost << "update_remaining_callsign_mults_window: number of worked callsign mults on this band = " << worked_callsign_mults.size() << endl;

// the original list of callsign mults
  set<string> original;

  if (context.auto_remaining_callsign_mults())
  { SAFELOCK(known_callsign_mults);

    original = known_callsign_mults;
  }
  else
    original = context.remaining_callsign_mults_list();

  if (filter_remaining_country_mults)
  { set<string> copy;

    copy_if(original.cbegin(), original.cend(), inserter(copy, copy.begin()), [=] (const string& s) { return (worked_callsign_mults.find(s) == worked_callsign_mults.end()); } );
    original = copy;
  }

// put in right order and get the colours right
  vector<string> vec_str;

  copy(original.cbegin(), original.cend(), back_inserter(vec_str));
  sort(vec_str.begin(), vec_str.end(), compare_calls);    // need to change the collation order

  vector<pair<string /* country */, int /* colour pair number */ > > vec;

  for (const auto& canonical_prefix : vec_str)
  { //ost << "updating callsign mults, processing: *" << canonical_prefix << "*" << endl;

    const bool is_needed = ( worked_callsign_mults.find(canonical_prefix) == worked_callsign_mults.end() );

    //ost << "is_needed = " << is_needed << "for callsign mult: " << canonical_prefix << endl;
    const int colour_pair_number = colours.add( ( is_needed ? win_remaining_callsign_mults.fg() : string_to_colour(context.worked_mults_colour()) ), win_remaining_callsign_mults.bg());

    vec.push_back( { canonical_prefix, colour_pair_number } );
  }

  win_remaining_callsign_mults < WINDOW_CLEAR < WINDOW_TOP_LEFT <= vec;
}

void update_remaining_country_mults_window(running_statistics& statistics, const BAND b, const MODE m)
{ const set<string> worked_country_mults = statistics.worked_country_mults(b, m);
  const set<string> known_country_mults = statistics.known_country_mults();

//  ost << "number of known country mults = " << known_country_mults.size() << endl;

// put in right order and get the colours right
  vector<string> vec_str;

  copy(known_country_mults.cbegin(), known_country_mults.cend(), back_inserter(vec_str));
  sort(vec_str.begin(), vec_str.end(), compare_calls);    // non-default collation order

  vector<pair<string /* country */, int /* colour pair number */ > > vec;

  for (size_t n = 0; n < vec_str.size(); ++n)
  { const string& canonical_prefix = vec_str[n];
    const bool is_needed = worked_country_mults.find(canonical_prefix) == worked_country_mults.end();

    int colour_pair_number = colours.add(win_remaining_country_mults.fg(), win_remaining_country_mults.bg());

    if (!is_needed)
       colour_pair_number = colours.add(string_to_colour(context.worked_mults_colour()),  win_remaining_country_mults.bg());

    vec.push_back( { canonical_prefix, colour_pair_number } );
  }

  win_remaining_country_mults < WINDOW_CLEAR < WINDOW_TOP_LEFT <= vec;
}

void update_remaining_exch_mults_window(const string& exch_mult_name, const contest_rules& rules, running_statistics& statistics, const BAND b, const MODE m)
{ const set<string> known_exchange_values_set = statistics.known_exchange_mults(exch_mult_name);
  const vector<string> known_exchange_values(known_exchange_values_set.cbegin(), known_exchange_values_set.cend());
  window* wp = win_remaining_exch_mults_p[exch_mult_name];
  window& win = (*wp);

// get the colours right
  vector<pair<string /* exch value */, int /* colour pair number */ > > vec;

   for (const auto& known_value : known_exchange_values)
   { const bool is_needed = statistics.is_needed_exchange_mult(exch_mult_name, known_value, b, m);
     const int colour_pair_number = ( is_needed ? colours.add(win.fg(), win.bg()) : colours.add(string_to_colour(context.worked_mults_colour()),  win.bg()) );

     vec.push_back( { known_value, colour_pair_number } );
   }

   win < WINDOW_CLEAR < WINDOW_TOP_LEFT <= vec;
}

void update_remaining_exchange_mults_windows(const contest_rules& rules, running_statistics& statistics, const BAND b, const MODE m)
{ for_each(win_remaining_exch_mults_p.begin(), win_remaining_exch_mults_p.end(), [&] (const map<string, window*>::value_type& mult)  // Josuttis 2nd ed., p. 338
    { update_remaining_exch_mults_window(mult.first, rules, statistics, b, m); } );
}

/*! \brief              Return the bearing to a station
    \param  callsign    target call
    \return             bearing, in degrees to <i>callsign</i>

    Returned string includes the degree sign
*/
const string bearing(const string& callsign)
{ static const string degree("°");

  if (callsign.empty())
    return string();

  const float lat1 = context.my_latitude();
  const float long1 = context.my_longitude();
  const location_info li = location_db.info(callsign);
  const location_info default_li;

  if (li == default_li)
    return string();

  const float lat2 = location_db.latitude(callsign);
  const float long2 = -location_db.longitude(callsign);    // minus sign to get in the correct direction
  const float b = bearing(lat1, long1, lat2, long2);
  int ibearing = static_cast<int>(b + 0.5);

  if (ibearing < 0)
    ibearing += 360;

  return (to_string(ibearing) + degree);
}

/*! \brief              Calculate the sunrise or sunset time for a station
    \param  callsign    call of the station for which sunrise or sunset is desired
    \param  calc_sunset whether to calculate sunset
    \return             sunrise or sunset in the form HHMM

    Returns "9999" if it's always dark, and "8888" if it's always light
 */
const string sunrise_or_sunset(const string& callsign, const bool calc_sunset)
{ const location_info li = location_db.info(callsign);
  const location_info default_li;

  if (li == default_li)
    return string();

  const float lat = location_db.latitude(callsign);
  const float lon = -location_db.longitude(callsign);    // minus sign to get in the correct direction
  const string rv = sunrise_or_sunset(lat, lon, calc_sunset);

  return rv;
}

/*! \brief              Calculate the sunrise time for a station
    \param  callsign    call of the station for which sunset is desired
    \return             sunrise in the form HHMM

    Returns "9999" if it's always dark, and "8888" if it's always light
 */
//const string sunrise(const string& callsign)
//  { return sunrise_or_sunset(callsign, false); }

/*! \brief              Calculate the sunset time for a station
    \param  callsign    call of the station for which sunset is desired
    \return             sunset in the form HHMM

    Returns "9999" if it's always dark, and "8888" if it's always light
 */
//inline const string sunset(const string& callsign)
//  { return sunrise_or_sunset(callsign, true); }

/*! \brief  Populate the information window
    \param  full or partial call

    Called multiple times as a call is being typed. Also populates INDIVIDUAL QTC COUNT
    window if appropriate
 */
void populate_win_info(const string& callsign)
{ if (send_qtcs)
  { const string qtc_str = string("[") + to_string(qtc_db.n_qtcs_sent_to(callsign)) + string("]");

    win_info < WINDOW_CLEAR < qtc_str <= centre(callsign, win_info.height() - 1);    // write the (partial) callsign
    win_individual_qtc_count < WINDOW_CLEAR <= qtc_str;
  }
  else
    win_info < WINDOW_CLEAR <= centre(callsign, win_info.height() - 1);    // write the (partial) callsign

  const string name_str = location_db.country_name(callsign);            // name of the country

  if (to_upper(name_str) != "NONE")
  { win_info < cursor(0, win_info.height() - 2) < location_db.canonical_prefix(callsign) < ": "
                                                < pad_string(bearing(callsign), 5)       < "  "
                                                < sunrise(callsign)                      < "/"      < sunset(callsign);

    const size_t len = name_str.size();

    win_info < cursor(win_info.width() - len, win_info.height() - 2) <= name_str;

    static const unsigned int FIRST_FIELD_WIDTH = 14;         // "Country [VP2M]"
    static const unsigned int FIELD_WIDTH       = 5;          // width of other fields
    int next_y_value = win_info.height() - 3;                 // keep track of where we are vertically in the window
    const vector<BAND>& permitted_bands = rules.permitted_bands();
    const set<MODE>& permitted_modes = rules.permitted_modes();

    for (const auto& this_mode : permitted_modes)
    { if (n_modes > 1)
        win_info < cursor(0, next_y_value--) < WINDOW_REVERSE < create_centred_string(MODE_NAME[this_mode], win_info.width()) < WINDOW_NORMAL;

// QSOs
      string line = pad_string("QSO", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

      for (const auto& b : permitted_bands)
        line += pad_string( ( q_history.worked(callsign, b, this_mode) ? "-" : BAND_NAME.at(b) ), FIELD_WIDTH);

      win_info < cursor(0, next_y_value--) < line;

// country mults
      const set<string>& country_mults = rules.country_mults();
      const string canonical_prefix = location_db.canonical_prefix(callsign);

      if (!country_mults.empty() or context.auto_remaining_country_mults())
      { if ((country_mults < canonical_prefix) or context.auto_remaining_country_mults())
        { const set<string> known_country_mults = statistics.known_country_mults();

          line = pad_string(string("Country [") + canonical_prefix +"]", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

          for (const auto& b : permitted_bands)
          { string per_band_indicator;

            if (known_country_mults < canonical_prefix)
              per_band_indicator = ( statistics.is_needed_country_mult(callsign, b) ? BAND_NAME.at(b) : "-" );
          else
            per_band_indicator = BAND_NAME.at(b);

          line += pad_string(per_band_indicator, FIELD_WIDTH);
        }
      }

      win_info < cursor(0, next_y_value--) < line;
    }

// exch mults
    const vector<string>& exch_mults = rules.exchange_mults();

//    ost << "about to guess exch mults for " << callsign << endl;

    for (const auto& exch_mult_field : exch_mults)
    { bool output_this_mult = true;

// output QTHX mults only if they apply to this callsign
//      if (starts_with(exch_mult_field, "QTHX["))
//      { const string target_canonical_prefix = delimited_substring(exch_mult_field, '[', ']');
//
//        output_this_mult = (target_canonical_prefix == canonical_prefix);
//      }

      output_this_mult = rules.is_exchange_field_used_for_country(exch_mult_field, canonical_prefix);

      ost << "mult name: " << exch_mult_field << ", output_this_mult = " << boolalpha << output_this_mult << noboolalpha << endl;

      if (output_this_mult)
      { //ost << "guessing for mult field " << exch_mult_field << endl;

        const string exch_mult_value = exchange_db.guess_value(callsign, exch_mult_field);    // make best guess as to to value of this field

        //ost << "guessed value is " << exch_mult_value << endl;

        line = pad_string(exch_mult_field + " [" + exch_mult_value + "]", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

        for (const auto& b : permitted_bands)
          line += pad_string( ( statistics.is_needed_exchange_mult(exch_mult_field, exch_mult_value, b, this_mode) ? BAND_NAME.at(b) : "-" ), FIELD_WIDTH);

        win_info < cursor(0, next_y_value-- ) < line;
      }
    }

// callsign mults
    if (rules.callsign_mults_per_band())
    { const set<string> callsign_mults = rules.callsign_mults();

      for (const auto& callsign_mult : callsign_mults)
      { string callsign_mult_value;

        if ( (callsign_mult == "AAPX")  and (location_db.continent(callsign) == "AS") )  // All Asian
          callsign_mult_value = wpx_prefix(callsign);

        if ( (callsign_mult == "OCPX")  and (location_db.continent(callsign) == "OC") )  // Oceania
          callsign_mult_value = wpx_prefix(callsign);

        if (callsign_mult == "SACPX")      // SAC
          callsign_mult_value = sac_prefix(callsign);

        if (callsign_mult == "WPXPX")
          callsign_mult_value = wpx_prefix(callsign);

        if (!callsign_mult_value.empty())
        { line = pad_string(callsign_mult + " [" + callsign_mult_value + "]", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

          for (const auto& b : permitted_bands)
            line += pad_string( ( statistics.is_needed_callsign_mult(callsign_mult, callsign_mult_value, b) ? BAND_NAME.at(b) : "-" ), FIELD_WIDTH);

          win_info < cursor(0, next_y_value-- ) < line;
        }
      }
    }
  }
  }

  win_info.refresh();
}

/*! \brief          Expand a CW message, replacing special characters
    \param  msg     The original message
    \return         <i>msg</i> with special characters replaced by their intended values

    Expands <i>#</i> and <i>@</i> characters.
    As written, this function is simple but inefficient.
    # -> octothorpe_str
    @ -> at_call
    * -> last_exchange
 */
const string expand_cw_message(const string& msg)
{ const string octothorpe_str = pad_string(to_string(octothorpe), (octothorpe < 1000 ? 3 : 4), PAD_LEFT, 'T');  // always send at least three characters in a serno, because predictability in exchanges is important
  const string octothorpe_replaced = replace(msg, "#", octothorpe_str);
  const string at_replaced = replace(octothorpe_replaced, "@", at_call);

  SAFELOCK(last_exchange);

  const string asterisk_replaced = replace(at_replaced, "*", last_exchange);

  return asterisk_replaced;
}

/// Thread function to simulate keystrokes
void* keyboard_test(void* vp)
{ XFlush(keyboard.display_p());

  keyboard.push_key_press('g');
  XFlush(keyboard.display_p());
  sleep_for(seconds(1));

  keyboard.push_key_press('4');
  XFlush(keyboard.display_p());
  sleep_for(seconds(2));

  keyboard.push_key_press('a');
  XFlush(keyboard.display_p());
//  sleep(4);
  sleep_for(seconds(4));

  keyboard.push_key_press('m');
  XFlush(keyboard.display_p());
//  sleep(8);
  sleep_for(seconds(8));

  keyboard.push_key_press('t');
  XFlush(keyboard.display_p());
//  sleep(1);
  sleep_for(seconds(1));

  return NULL;
}

/// Thread function to simulate a contest from an extant log
void* simulator_thread(void* vp)
{ start_of_thread();

  tuple<string, int>& params = *(static_cast<tuple<string, int>*>(vp));
  const string& filename = get<0>(params);
  int           max_n_qsos = get<1>(params);

  tr_log trl(filename);
  string last_frequency;

  const unsigned int n_qso_limit = (max_n_qsos ? max_n_qsos : trl.number_of_qsos());    // either apply a limit or run them all

  for (unsigned int n = 0; n < n_qso_limit; ++n)
  { const tr_record& rec = trl.read(n);
    const string str_frequency = rec.frequency();

    if (str_frequency != last_frequency)
    { rig.rig_frequency(frequency(str_frequency));

ost << "QSY to " << frequency(str_frequency).hz() << " Hz" << endl;

      if (static_cast<BAND>(frequency(last_frequency)) != static_cast<BAND>(frequency(str_frequency)))
      { safe_set_band(static_cast<BAND>(frequency(str_frequency)));
        const BAND cur_band = safe_get_band();

        update_remaining_country_mults_window(statistics, cur_band, safe_get_mode());
        update_remaining_exchange_mults_windows(rules, statistics, cur_band);
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

    sleep_for(seconds(1));

    if (cw_p)
      while (!cw_p->empty())
        sleep_for(milliseconds(500));

    sleep_for(seconds(1));

    keyboard.push_key_press(static_cast<KeySym>(XK_Return));

    sleep_for(seconds(1));

    if (cw_p)
      while (!cw_p->empty())
        sleep_for(milliseconds(500));

    sleep_for(seconds(1));

    { SAFELOCK(thread_check);

      if (exiting)
      { n_running_threads--;
        return nullptr;
      }
    }
  }

//  return nullptr;    // keep compiler happy
  pthread_exit(nullptr);
}

/*! \brief          Possibly add a new callsign mult
    \param  msg     callsign

    Supports: AA, OC, SAC. Updates as necessary the container of
    known callsign mults. Also updates the window that displays the
    known callsign mults.
*/
void update_known_callsign_mults(const string& callsign)
{ if (callsign.empty())
    return;

  if (context.auto_remaining_callsign_mults())
  { const string continent = location_db.continent(callsign);
    const string prefix = wpx_prefix(callsign);
    const set<string> callsign_mults = rules.callsign_mults();           ///< collection of types of mults based on callsign (e.g., "WPXPX")

    if ( (callsign_mults < string("AAPX")) and (continent == "AS") )
    { bool is_known;

      { SAFELOCK(known_callsign_mults);
        is_known = (known_callsign_mults < prefix);
      }

      if (!is_known)
      {
        { SAFELOCK(known_callsign_mults);
          known_callsign_mults.insert(prefix);
        }

        update_remaining_callsign_mults_window(statistics, "AAPX", safe_get_band(), safe_get_mode());
      }
    }

    if ( (callsign_mults < string("OCPX")) and (continent == "OC") )
    { bool is_known;

      { SAFELOCK(known_callsign_mults);
        is_known = (known_callsign_mults < prefix);
      }

      if (!is_known)
      {
        { SAFELOCK(known_callsign_mults);
          known_callsign_mults.insert(prefix);
        }

        update_remaining_callsign_mults_window(statistics, "OCPX", safe_get_band(), safe_get_mode());
      }
    }

    if (callsign_mults < string("SACPX"))
    { const string prefix = sac_prefix(callsign);

      bool is_known;

      if (!prefix.empty())    // SAC can return empty prefix
      {
        { SAFELOCK(known_callsign_mults);
          is_known = (known_callsign_mults < prefix);
        }

        if (!is_known)
        {
          { SAFELOCK(known_callsign_mults);
            known_callsign_mults.insert(prefix);
          }

          update_remaining_callsign_mults_window(statistics, "SACPX", safe_get_band(), safe_get_mode());
        }
      }
    }
  }
}

/*! \brief              Possibly add a new country to the known country mults
    \param  callsign    callsign from the country possibly to be added

    Adds only if REMAINING COUNTRY MULTS has been set to AUTO in the configuration file
*/
void update_known_country_mults(const string& callsign)
{ if (callsign.empty())
    return;

//  ost << "inside update_known_country_mults for " << callsign << endl;
//  ost << "initial number of known country mults = " << statistics.n_known_country_mults() << endl;

  if (context.auto_remaining_country_mults())
  { const string canonical_prefix = location_db.canonical_prefix(callsign);

//    ost << "about to add " << canonical_prefix << endl;

    if (rules.country_mults() < canonical_prefix)            // don't add if the rules don't recognise it as a country mult
    { //ost << "prefix is known to rules" << endl;
      statistics.add_known_country_mult(canonical_prefix);

      //ost << "number of known country mults = " << statistics.n_known_country_mults() << endl;
    }
//    else
//      ost << "prefix is NOT known to rules; size = " << rules.country_mults().size() << endl;
  }
}

/*!     \brief  Send data to the archive file
*/
void archive_data(void)
{ ofstream ofs(context.archive_name());            // the destination archive
  boost::archive::binary_oarchive ar(ofs);

  ost << "Starting archive" << endl;

// miscellaneous variables
  alert("Archiving miscellaneous variables");
  ar & current_band & current_mode
     & next_qso_number & octothorpe
     & rig.rig_frequency();

// bandmaps
  alert("Archiving bandmaps");
  ar & bandmaps;

// log
  alert("Archiving log");
  ar & logbk;

// rate variables
  alert("Archiving rate information");
  ar & rate;

// rules (which includes [possibly-auto] canonical exchange values)
  alert("Archiving rules");
  ar & rules;

// QSO history of each call
  alert("Archiving per-call QSO history");
  ar & q_history;

// statistics
  alert("Archiving statistics");
  ar & statistics;

  ost << "Archive complete" << endl;
}

/*! \brief                      Extract the data from the archive file
    \param  archive_filename    name of the file that contains the archive
*/
void restore_data(const string& archive_filename)
{ if (file_exists(archive_filename))
  { try
    { ifstream ifs(archive_filename);            // the source archive
      boost::archive::binary_iarchive ar(ifs);

// miscellaneous variables
      frequency rig_frequency;
      alert("Restoring miscellaneous variables");
      ar & current_band & current_mode
         & next_qso_number & octothorpe
         & rig_frequency;

// bandmaps
      alert("Restoring bandmaps");
      ar & bandmaps;

// log
      alert("Restoring log");
      ar & logbk;

// rate variables
      alert("Restoring rate information");
      ar & rate;

// rules (which includes [possibly-auto] canonical exchange values)
      alert("Restoring rules");
      ar & rules;

// QSO history of each call
      alert("Restoring per-call QSO history");
      ar & q_history;

// statistics
      alert("Restoring statistics");
      ar & statistics;

      alert("Finished restoring data");
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
  const list<QSO> qsos = logbk.as_list();

  rate.clear();

  for (const auto& qso : qsos)
  { statistics.add_qso(qso, new_logbk, rules);  // need new logbook so that dupes are calculated correctly
    new_logbk += qso;

// redo the historical Q-count and score... this is relatively time-consuming
    const time_t epoch_time = qso.epoch_time();

    rate.insert(epoch_time, statistics.points(rules));
  }
}

/*! \brief  Obtain the current time in HHMMSS format
*/
const string hhmmss(void)
{ const time_t now = ::time(NULL);           // get the time from the kernel
  struct tm    structured_time;
  array<char, 26> buf;                       // buffer to hold the ASCII date/time info; see man page for gmtime()

  gmtime_r(&now, &structured_time);          // convert to UTC in a thread-safe manner
  asctime_r(&structured_time, buf.data());   // convert to ASCII

  return (substring(string(buf.data(), 26), 11, 8));
}

/*! \brief          Alert the user
    \param  msg     Message to display

    Also logs the message
*/
void alert(const string& msg)
{
  { SAFELOCK(alert);
    alert_time = ::time(NULL);
  }

   win_message < WINDOW_CLEAR < hhmmss() < " " <= msg;
   ost << "ALERT: " << hhmmss() << " " << msg << endl;
}

/*! \brief          Alert the user to a rig-related error
    \param  msg     message to display

    Also logs the message
*/
void rig_error_alert(const string& msg)
{ ost << "Rig error: " << msg << endl;
  alert(msg);
}

/// update the QSO and score values in <i>win_rate</i>
void update_rate_window(void)
{ const vector<unsigned int> rate_periods = context.rate_periods();    // in minutes
  string rate_str = pad_string("", 3) + pad_string("Qs", 3) + pad_string("Score", 10);

  if (rate_str.length() != static_cast<unsigned int>(win_rate.width()))    // LF is added automatically if a string fills a line
    rate_str += LF;

  for (const auto& rate_period : rate_periods)
  { string str = pad_string(to_string(rate_period), 3, PAD_RIGHT);
    const pair<unsigned int, unsigned int> qs = rate.calculate_rate(rate_period * 60, context.normalise_rate() ? 3600 : 0);

    str += pad_string(to_string(qs.first), 3);
//    str += pad_string(comma_separated_string(qs.second), 10);
    str += pad_string(separated_string(qs.second, TS), 10);

    rate_str += (str + (str.length() == static_cast<unsigned int>(win_rate.width()) ? "" : LF) );      // LF is added automatically if a string fills a line
  }

  win_rate < WINDOW_CLEAR < CURSOR_TOP_LEFT < centre("RATE", win_rate.height() - 1)
           < CURSOR_DOWN < CURSOR_START_OF_LINE <= rate_str;
}

/// Thread function to reset the RBN or cluster connection
void* reset_connection(void* vp)
{ // no start_of_thread for this one, since it's all asynchronous
  dx_cluster* rbn_p = static_cast<dx_cluster*>(vp);

  rbn_p->reset();

//  return nullptr;
  pthread_exit(nullptr);
}

/*! \brief          Populate QSO with correct exchange mults
    \param  qso     QSO to poulate
    \param  rules   rules for this contest
    \return         whether any exchange fields are new mults
*/
const bool calculate_exchange_mults(QSO& qso, const contest_rules& rules)
{ ost << "Inside calculate_exchange_mults()" << endl;

  const vector<exchange_field> exchange_template = rules.expanded_exch(qso.canonical_prefix(), qso.mode());        // exchange_field = name, is_mult
  const vector<received_field> received_exchange = qso.received_exchange();
//  const BAND b = qso.band();
  vector<received_field> new_received_exchange;
  bool rv = false;
//  const bool auto_mults = (context.auto_remaining_exchange_mults());

  for (auto field : received_exchange)
  { ost << "received exchange field = " << field << endl;

    if (field.is_possible_mult())                              // see if it's really a mult
    { if (context.auto_remaining_exchange_mults(field.name()))
        statistics.add_known_exchange_mult(field.name(), field.value());

      const bool is_needed_exchange_mult = statistics.is_needed_exchange_mult(field.name(), field.value(), qso.band(), qso.mode());

 ost << qso.callsign() << " is_needed_exchange_mult value = " << is_needed_exchange_mult << " for field name " << field.name() << " and value " << field.value() << endl;

      field.is_mult(is_needed_exchange_mult);
      if (is_needed_exchange_mult)
        rv = true;
    }
//    else    // field is not allowed to be a mult by the rules
    new_received_exchange.push_back(field);  // leave it unchanged
  }

  qso.received_exchange(new_received_exchange);

  ost << "leaving calculate_exchange_mults()" << endl;

  return rv;
}

/*! \brief              Rebuild the history (and statistics and rate), using the logbook
    \param  logbk       logbook of QSOs
    \param  rules       rules for this contest
    \param  statistics  global statistics
    \param  q_history   History of QSOs
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

  logbook l;
  const vector<QSO> q_vec = logbk.as_vector();
  int n_qsos = 0;

  for (const auto& qso : q_vec)
  { statistics.add_qso(qso, l, rules);
    q_history += qso;

    const time_t& epoch_time = qso.epoch_time();

    rate.insert(epoch_time, ++n_qsos, statistics.points(rules));

    l += qso;
  }
}

/*! \brief  Copy a file to a backup directory
*
*   This is intended to be used as a separate thread, so the parameters are passed
*   in the usual void*
*/
void* auto_backup(void* vp)
{
  { start_of_thread();

    try
    { const tuple<string, string, string>* tsss_p = static_cast<tuple<string, string, string>*>(vp);;
      const string& directory = get<0>(*tsss_p);
      const string& filename = get<1>(*tsss_p);
      const string& qtc_filename = get<2>(*tsss_p);
      const string dts = date_time_string();
      const string suffix = dts.substr(0, 13) + '-' + dts.substr(14); // replace : with -
      const string complete_name = directory + "/" + filename + "-" + suffix;

      ofstream(complete_name) << ifstream(filename).rdbuf();          // perform the copy

      if (!qtc_filename.empty())
      { const string qtc_complete_name = directory + "/" + qtc_filename + "-" + suffix;

        ofstream(qtc_complete_name) << ifstream(qtc_filename).rdbuf();  // perform copy of QTC file
      }
    }

    catch (...)
    { ost << "CAUGHT EXCEPTION IN AUTO_BACKUP" << endl;
    }

// manually mark this thread as complete, since we don't want to interrupt the above copy
    { SAFELOCK(thread_check);

      n_running_threads--;
    }
  }  // ensure that all objects call destructors, whatever the implementation

  pthread_exit(nullptr);
}

/// write the current local time to <i>win_local_time</i>
void update_local_time(void)
{ if (win_local_time.wp())                                         // don't do it if we haven't defined this window
  { struct tm       structured_local_time;
    array<char, 26> buf_local_time;
    const time_t    now = ::time(NULL);                            // get the time from the kernel

    localtime_r(&now, &structured_local_time);                     // convert to local time
    asctime_r(&structured_local_time, buf_local_time.data());      // and now to ASCII

    win_local_time < CURSOR_START_OF_LINE <= substring(string(buf_local_time.data(), 26), 11, 5);  // extract HH:MM and display it
  }
}

/// Increase the counter for the number of running threads
void start_of_thread(void)
{ SAFELOCK(thread_check);

  n_running_threads++;
}

/// Cleanup and exit
void exit_drlog(void)
{ ost << "Inside exit_drlog()" << endl;

  archive_data();

  ost << "finished archiving" << endl;

  { ost << "about to lock" << endl;

    SAFELOCK(thread_check);

    ost << "have the lock" << endl;

    exiting = true;
    ost << "exiting now true; number of threads = " << n_running_threads << endl;
  }

  ost << "starting exit tests" << endl;

  for (unsigned int n = 0; n <10; ++n)
  { ost << "exit test number " << n << endl;

//    bool ok_to_exit = false;

    { SAFELOCK(thread_check);

    const int local_copy = n_running_threads;

    ost << "n_running_threads = " << local_copy << endl;

      if (local_copy == 0);
        exit(0);
    }

    ost << "after exit test; about to sleep for one second" << endl;

    sleep_for(seconds(1));
  }

  ost << "Exiting even though some threads still appear to be running" << endl;

  exit(0);
}

/*! \brief      Convert a serial number to a string
    \param  n   serial number
    \return     <i>n</i> as a zero-padded string of three digits, or a four-digit string if <i>n</i> is greater than 999
*/
//const string serial_number_string(const unsigned int n)
//{ return ( (n < 1000) ? pad_string(to_string(n), 3, PAD_LEFT, '0') : to_string(n) );
//}

//void update_scp_window(const string& callsign)
//{ update_matches_window(scp_dbs[callsign], scp_matches, win_scp, callsign);
//}

/*! \brief              Update the fuzzy window with matches for a particular call
    \param  callsign    callsign against which to generate the fuzzy matches
*/
//void update_fuzzy_window(const string& callsign)
//{ update_matches_window(fuzzy_dbs[callsign], fuzzy_matches, win_fuzzy, callsign);
//}

/*! \brief          Get best fuzzy or SCP match
    \param  matches vector of fuzzy or SCP matches, colour coded
    \return         best of the matches

    The returned value, if any, is the match that is most likely to be the correct call, according to the algorithm in this routine.
    This routine is very conservative. Hence, it should almost always return a value with which
    the operator would agree. In the absence of an obvious candidate for "best match", the
    empty string is returned.
*/
const string match_callsign(const vector<pair<string /* callsign */, int /* colour pair number */ > >& matches)
{ string new_callsign;

  if ((matches.size() == 1) and (colours.fg(matches[0].second) != COLOUR_RED))
    new_callsign = matches[0].first;

  if (new_callsign == string())
  { int n_green = 0;
    string tmp_callsign;

    for (size_t n = 0; n < matches.size(); ++n)
    { if (colours.fg(matches[n].second) == COLOUR_GREEN)
      { n_green++;
        tmp_callsign = matches[n].first;
      }
    }

    if (n_green == 1)
      new_callsign = tmp_callsign;
  }

  return new_callsign;
}

const bool is_needed_qso(const string& callsign, const BAND b, const MODE m)
{ //const BAND b = safe_get_band();
  //const MODE m = safe_get_mode();
//  const bool multiple_band_qsos = rules.work_if_different_band();    // ok to work on multiple bands?
//  const bool multiple_mode_qsos = rules.work_if_different_mode();
  const bool worked_at_all = q_history.worked(callsign);

  if (!worked_at_all)
    return true;

  const bool worked_this_band_mode = q_history.worked(callsign, b, m);

  if (worked_this_band_mode)
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

  ost << "ERROR in is_needed_qso for " << callsign << ", " << BAND_NAME[b] << ", " << MODE_NAME[m] << endl;
  return false;    // should never get here
//  if (rules.work_if_different_band() and q_history.worked(callsign, m))
//    return false;

//  if (rules.work_if_different_mode() and q_history.worked(callsign, b))
//    return false;




//  const bool is_needed = (!worked_this_band_mode and multiple_band_qsos) or (!worked_at_all and !multiple_band_qsos);

//  return is_needed;
}

/*! \brief      Control RIT using the SHIFT keys
    \param  e   keyboard event to process

    RIT changes via hamlib, at least on the K3, are *very* slow
*/
void rit_control(const keyboard_event& e)
{ const int change = (e.symbol() == XK_Shift_L ? -context.shift_delta() : context.shift_delta());
  int poll = context.shift_poll();

  try
  { int last_rit = rig.rit();

    if (rig.rit_enabled())
    { ok_to_poll_k3 = false;

      do
      { rig.rit(last_rit + change);                 // this takes forever through hamlib
        last_rit += change;

        if (poll)
          sleep_for(milliseconds(poll));
      } while (keyboard.empty());                      // the next event should be a key-release, but anything will do

      ok_to_poll_k3 = true;
    }
  }

  catch (const rig_interface_error& e)
  { alert("Error in rig communication while setting RIT offset");
    ok_to_poll_k3 = true;
  }
}

/// switch the states of RIT and XIT
void swap_rit_xit(void)
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

/*! \brief      Add a QSO into the all the objects that need to know about it
    \param  qso the QSO to add
*/
void add_qso(const QSO& qso)
{ statistics.add_qso(qso, logbk, rules);    // add it to the running statistics before we add it to the log so we can check for dupes against the current log
  logbk += qso;

// add it to the QSO history
  q_history += qso;

// possibly add it to the dynamic SCP database
  if (!scp_db.contains(qso.callsign()) and !scp_dynamic_db.contains(qso.callsign()))
    scp_dynamic_db.add_call(qso.callsign());

// and the fuzzy database
  if (!fuzzy_db.contains(qso.callsign()) and !fuzzy_dynamic_db.contains(qso.callsign()))
    fuzzy_dynamic_db.add_call(qso.callsign());

// add to the rates
  rate.insert(qso.epoch_time(), statistics.points(rules));
}

/*! \brief              Update the individual_messages window with the message (if any) associated with a call
    \param  callsign    callsign with which the message is associated

    Clears the window if there is no individual message associated with <i>callsign</i>
*/
void update_individual_messages_window(const string& callsign)
{ bool message_written = false;

  if (!callsign.empty())
  { SAFELOCK(individual_messages);

    const auto posn = individual_messages.find(callsign);

    if (posn != individual_messages.end())
    { win_individual_messages < WINDOW_CLEAR < CURSOR_START_OF_LINE <= posn->second;
      message_written = true;
    }
  }

  if (!message_written and !win_individual_messages.empty())
    win_individual_messages < WINDOW_CLEAR <= CURSOR_START_OF_LINE;
}

/*! \brief              Update the batch_messages window with the message (if any) associated with a call
    \param  callsign    callsign with which the message is associated

    Clears the window if there is no batch message associated with <i>callsign</i>
*/
void update_batch_messages_window(const string& callsign)
{ bool message_written = false;

  if (!callsign.empty())
  { SAFELOCK(batch_messages);

    const auto posn = batch_messages.find(callsign);

    if (posn != batch_messages.end())
    { win_batch_messages < WINDOW_CLEAR < CURSOR_START_OF_LINE <= posn->second;
      message_written = true;
    }
  }

  if (!message_written and !win_batch_messages.empty())
    win_batch_messages < WINDOW_CLEAR <= CURSOR_START_OF_LINE;
}

/*! \brief  Obtain value corresponding to a type of callsign mult from a callsign
    \param  callsign_mult_name  the type of the callsign mult
    \param  callsign            the call for which the mult value is required

    Returns the empty string if no sensible result can be returned
*/
const string callsign_mult_value(const string& callsign_mult_name, const string& callsign)
{ if ( (callsign_mult_name == "AAPX")  and (location_db.continent(callsign) == "AS") )  // All Asian
    return wpx_prefix(callsign);

  if ( (callsign_mult_name == "OCPX")  and (location_db.continent(callsign) == "OC") )  // Oceania
    return wpx_prefix(callsign);

  if (callsign_mult_name == "SACPX")      // SAC
    return sac_prefix(callsign);

  if (callsign_mult_name == "WPXPX")
    return wpx_prefix(callsign);

  return string();
}

#if 0
void* start_cluster_thread(void* vp)
{ big_cluster_info& bci = *(static_cast<big_cluster_info*>(vp)); // make the cluster available
  drlog_context& context = *(bci.context_p());
  POSTING_SOURCE& posting_source = *(bci.source_p());

  cluster_p = new dx_cluster(context, POSTING_CLUSTER);

  return nullptr;
}
#endif    // 0

/*! \brief          Update several call-related windows
    \param callsign call to use as a basis for the updated windows

    Updates the following windows:
      info
      batch messages
      individual messages
      extract
 */
void display_call_info(const string& callsign, const bool display_extract)
{ populate_win_info( callsign );
  update_batch_messages_window( callsign );
  update_individual_messages_window( callsign );

  if (display_extract)
  { extract = logbk.worked( callsign );
    extract.display();
  }
}

/*! \brief Start a thread to take a snapshot of a P3.

    Even though we use a separate thread to obtain the actual screenshot, it takes so long to transfer the data to the computer
    that one should not use this function except when it will be OK for communication with the rig to be interrupted.
*/
void p3_screenshot(void)
{ static pthread_t thread_id_p3_screenshot;

  try
  { create_thread(&thread_id_p3_screenshot, &(attr_detached.attr()), p3_screenshot_thread, NULL, "P3");
  }

  catch (const pthread_error& e)
  { ost << e.reason() << endl;
  }
}

/// Thread function to generate a screenshot of a P3 and store it in a BMP file
void* p3_screenshot_thread(void* vp)
{ const string image = rig.raw_command("#BMP;", true);
  const string checksum_str = image.substr(image.length() - 2, 2);
  uint16_t calculated_checksum = 0;

  for (size_t n = 0; n < image.length() - 2; ++n)
  { const unsigned char uch = static_cast<unsigned char>(image[n]);
    const uint16_t uint = static_cast<uint16_t>(uch);

    calculated_checksum += uint;
  }

  uint16_t received_checksum = 0;
  for (size_t n = 0; n < checksum_str.length(); ++n)
  { const size_t index = 2 - n - 1;
    const unsigned char uch = static_cast<unsigned char>(checksum_str[index]);
    const uint16_t uint = static_cast<uint16_t>(uch);

    ost << n << ": " << hex << uint << endl;

    received_checksum = (received_checksum << 8) + uint;  // 256 == << 8
  }

  ost << "calculated checksum = " << hex << calculated_checksum << endl;
  ost << "received checksum = " << hex << received_checksum << dec << endl;

  const string base_filename = context.p3_snapshot_file() + ((calculated_checksum == received_checksum) ? "" : "-error");
  int index = 0;
  bool file_written = false;

  while (!file_written)
  { const string filename = base_filename + "-" + to_string(index);

    if (!file_exists(filename))
    { write_file(image.substr(0, image.length() - 2), filename);
      file_written = true;

      alert("P3 image file " + filename + " written");
    }
    else
      ++index;
  }

  pthread_exit(nullptr);
}

/// Thread function to spawn the cluster
void* spawn_dx_cluster(void* vp)
{ cluster_p = new dx_cluster(context, POSTING_CLUSTER);

  static cluster_info cluster_info_for_thread(&win_cluster_line, &win_cluster_mult, cluster_p, &statistics, &location_db, &win_bandmap, &bandmaps);
  static pthread_t thread_id_2;
  static pthread_t thread_id_3;

  try
  { create_thread(&thread_id_2, &(attr_detached.attr()), get_cluster_info, (void*)(cluster_p), "cluster read");
    create_thread(&thread_id_3, &(attr_detached.attr()), process_rbn_info, (void*)(&cluster_info_for_thread), "cluster process");
  }

  catch (const pthread_error& e)
  { ost << e.reason() << endl;
    exit(-1);
  }
}

/// Thread function to spawn the RBN
void* spawn_rbn(void* vp)
{ //ost << "about to create RBN" << endl;
  rbn_p = new dx_cluster(context, POSTING_RBN);
  //ost << "RBN created" << endl;

  static cluster_info rbn_info_for_thread(&win_rbn_line, &win_cluster_mult, rbn_p, &statistics, &location_db, &win_bandmap, &bandmaps);
  static pthread_t   thread_id_2;
  static pthread_t thread_id_3;

  try
  { create_thread(&thread_id_2, &(attr_detached.attr()), get_cluster_info, (void*)(rbn_p), "RBN read");
    create_thread(&thread_id_3, &(attr_detached.attr()), process_rbn_info, (void*)(&rbn_info_for_thread), "RBN process");
  }

  catch (const pthread_error& e)
  { ost << e.reason() << endl;
    exit(-1);
  }
}

/*! \brief  Dump useful information to disk

    Performs a screenshot dump, and then dumps useful information
    to the debug file.
*/
void debug_dump(void)
{ ost << "*** DEBUG DUMP ***" << endl;

  ost << "Screenshot dumped to: " << dump_screen() << endl;
  int index = 0;

  for (auto& bm : bandmaps)
  { ost << "dumping bandmap # " << index++ << endl;

    const string str = bm.to_str();

    ost << str;
  }
}

/*! \brief                  Dump a screen image to PNG file
    \param  dump_filename   name of the destination file
    \return                 name of the file actually written

    If <i>dump_filename</i> is empty, then a base name is taken
    from the context, and a string "-<n>" is appended.
*/
const string dump_screen(const string& dump_filename)
{ //ost << "dump_screen called with parameter: " << dump_filename << endl;

  Display* display_p = keyboard.display_p();
  const Window window_id = keyboard.window_id();
  XWindowAttributes win_attr;

  //ost << hhmmss() << ": locking display 1" << endl;

  XLockDisplay(display_p);
  const Status status = XGetWindowAttributes(display_p, window_id, &win_attr);

  if (status == 0)
    ost << hhmmss() << ": ERROR returned by XGetWindowAttributes: " << status << endl;

  XUnlockDisplay(display_p);

  //ost << hhmmss() << ": unlocked display 2" << endl;

  const int width = win_attr.width;
  const int height = win_attr.height;

  //ost << hhmmss() << ": locking display 2" << endl;

  XLockDisplay(display_p);
  XImage* xim_p = XGetImage(display_p, window_id, 0, 0, width, height, XAllPlanes(), ZPixmap);
  XUnlockDisplay(display_p);

  //ost << hhmmss() << ": unlocked display 2" << endl;

  png::image< png::rgb_pixel > image(width, height);

  static const unsigned int BLUE_MASK = 0xff;
  static const unsigned int GREEN_MASK = 0xff << 8;
  static const unsigned int RED_MASK = 0xff << 16;

  for (size_t y = 0; y < image.get_height(); ++y)
  { for (size_t x = 0; x < image.get_width(); ++x)
    { const unsigned long pixel = XGetPixel (xim_p, x, y);
      const unsigned char blue = pixel bitand BLUE_MASK;
      const unsigned char green = (pixel bitand GREEN_MASK) >> 8;
      const unsigned char red = (pixel bitand RED_MASK) >> 16;

      image[y][x] = png::rgb_pixel(red, green, blue);
    }
  }

  //ost << hhmmss() << ": prepared image" << endl;

  string filename;

  if (dump_filename.empty())
  { const string base_filename = context.screen_snapshot_file();

    int index = 0;
    filename = base_filename + "-" + to_string(index++);

    while (file_exists(filename))
      filename = base_filename + "-" + to_string(index++);
  }
  else
    filename = dump_filename;

  image.write(filename);

  //ost << hhmmss() << ": image file written" << endl;

  alert("screenshot file " + filename + " written");

  return filename;
}

/// add info to QSO if callsign mults are in use; may change qso
void allow_for_callsign_mults(QSO& qso)
{ if (callsign_mults_used)
  { string mult_name;

    if ( (rules.callsign_mults() < static_cast<string>("AAPX")) and (location_db.continent(qso.callsign()) == "AS") )  // All Asian
    { qso.prefix(wpx_prefix(qso.callsign()));
      mult_name = "AAPX";
    }

    if ( (rules.callsign_mults() < static_cast<string>("OCPX")) and (location_db.continent(qso.callsign()) == "OC") )  // Oceania
    { qso.prefix(wpx_prefix(qso.callsign()));
      mult_name = "OCPX";
    }

    if ( (rules.callsign_mults() < static_cast<string>("SACPX")) )      // SAC
    { qso.prefix(sac_prefix(qso.callsign()));
      mult_name = "SACPX";
    }

    if (rules.callsign_mults() < static_cast<string>("WPXPX"))
    { qso.prefix(wpx_prefix(qso.callsign()));
      mult_name = "WPXPX";
    }

// see if it's a mult... requires checking if mults are per-band
    if (!qso.prefix().empty() and !mult_name.empty())
    { if (rules.callsign_mults_per_band())
      { if (statistics.is_needed_callsign_mult(mult_name, qso.prefix(), qso.band()))
          qso.is_prefix_mult(true);
      }
      else
      { if (statistics.is_needed_callsign_mult(mult_name, qso.prefix(), static_cast<BAND>(ALL_BANDS)) )
          qso.is_prefix_mult(true);
      }
    }
  }
}

void process_QTC_input(window* wp, const keyboard_event& e)
{ static bool sending_series = false;
  static unsigned int total_qtcs_to_send;
  static unsigned int qtcs_sent;
  static string qtc_id;
  static qtc_series series;

  const bool cw = (safe_get_mode() == MODE_CW);  // just to keep it easy to determine if we are on CW
  bool processed = false;

  auto send_msg = [=](const string& msg)
    { if (cw)
      { const unsigned int qrs = context.qtc_qrs();

        if (qrs)
          (*cw_p) << create_string('-', qrs);

        (*cw_p) << msg;  // don't use cw_speed because that executes asynchronously, so the speed will be back to full speed before the message is sent

        if (qrs)
          (*cw_p) << create_string('+', qrs);
      }
    };

  ost << "processing QTC input; event string: " << e.str() << endl;

  window& win = *wp;   // syntactic sugar

// ALT-Q - start process of sending QTC batch
  if (!sending_series and e.is_alt('q'))
  {
// destination for the QTC is the callsign in the CALL window; or, if the window is empty, the call of the last logged QSO
    const string call_window_contents = remove_peripheral_spaces(win_call.read());
    string destination_callsign = call_window_contents;

    if (destination_callsign.empty())
      destination_callsign = logbk.last_qso().callsign();

    if (!destination_callsign.empty() and (location_db.continent(destination_callsign) != "EU") )  // got a call, but it's not EU
    { vector<QSO> vec_q = logbk.filter([] (const QSO& q) { return (q.continent() == string("EU")); } );

      destination_callsign = ( vec_q.empty() ? string() : (vec_q[vec_q.size() - 1].callsign()) );
    }

    if (destination_callsign.empty())  // we have no destination
    { alert("No valid destination for QTC");
      win_active_p = &win_call;
      processed = true;
    }

    if (!processed and location_db.continent(destination_callsign) != "EU")    // send QTCs only to EU stations
    { alert("No EU destination for QTC");
      win_active_p = &win_call;
      processed = true;
    }

// check that it's OK to send a QTC to this call
    const unsigned int n_already_sent = qtc_db.n_qtcs_sent_to(destination_callsign);

    if (!processed and n_already_sent >= 10)
    { alert(string("10 QSOs already sent to ") + destination_callsign);
      win_active_p = &win_call;
      processed = true;
    }

// check that we have at least one QTC that can be sent to this call
    const unsigned int n_to_send = 10 - n_already_sent;
    const vector<qtc_entry> qtc_entries_to_send = qtc_buf.get_next_unsent_qtc(destination_callsign, n_to_send);

    if (!processed and qtc_entries_to_send.empty())
    { alert(string("No QSOs available to send to ") + destination_callsign);
      win_active_p = &win_call;
      processed = true;
    }

    if (!processed)
    { const string mode_str = (safe_get_mode() == MODE_CW ? "CW" : "PH");
      series = qtc_series(qtc_entries_to_send, mode_str, context.my_call());

// populate info in the series
      series.destination(destination_callsign);

// check that we still have entries (this should always pass)
      if (series.empty())
      { alert(string("Error: empty QTC object for ") + destination_callsign);
        win_active_p = &win_call;
        processed = true;
      }
      else

// OK; we're going to send at least one QTC
      { sending_series = true;

        const unsigned int number_of_qtc = qtc_db.size() + 1;

        qtc_id = to_string(number_of_qtc) + "/" + to_string(qtc_entries_to_send.size());
        series.id(qtc_id);

        if (cw)
          send_msg( (cw_p->empty() ? (string)"QTC " : (string)" QTC ") + qtc_id + " QRV?");

        win_qtc_status < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Sending QTC " < qtc_id < " to " <= destination_callsign;

// display the QTC entries; we use the "log extract" window
        win <= series;

        total_qtcs_to_send = qtc_entries_to_send.size();
        qtcs_sent = 0;
        processed = true;
      }
    }
  }

// ESCAPE
  if (!processed and e.symbol() == XK_Escape)
  {
// abort sending CW if we are currently sending
    if (cw)
    { if (!cw_p->empty())
      { cw_p->abort();
        processed = true;
      }
    }

    processed = true;
  }

// R -- repeat introduction (i.e., no QTCs sent)
  if (!processed and (qtcs_sent == 0) and (e.is_char('r')))
  { if (cw)
      send_msg((string)"QTC " + qtc_id + " QRV?");

    processed = true;
  }

// ENTER
  if (!processed and e.is_unmodified() and (e.symbol() == XK_Return))
  { if (qtcs_sent != total_qtcs_to_send)
    { const qtc_entry& qe = series[qtcs_sent].first;

      if (cw)
      { string msg;
        string space = (context.qtc_double_space() ? "  " : " ");

        msg = qe.utc() + space + qe.callsign() + space;

        const string serno = pad_string(remove_leading(remove_peripheral_spaces(qe.serno()), '0'), 3, PAD_LEFT, 'T');

        msg += serno;
        send_msg(msg);
      }

// before marking this as sent, record the last acknowledged QTC
      if (qtcs_sent != 0)
        qtc_buf.unsent_to_sent(series[qtcs_sent - 1].first);

      series.mark_as_sent(qtcs_sent++);
      win < WINDOW_CLEAR < WINDOW_TOP_LEFT <= series;

      processed = true;
    }
    else    // we have sent the last QTC; cleanup
    { qtc_buf.unsent_to_sent(series[series.size() - 1].first);

      ost << "Sent QTC " << qtc_id << " to " << series.destination() << endl;
      win_qtc_status < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Sent QTC " < qtc_id < " to " <= series.destination();

      series.date(substring(date_time_string(), 0, 10));
      series.utc(hhmmss());
      series.frequency_str(rig.rig_frequency());

//      ost << "QTC frequency_str set = " << rig.rig_frequency() << endl;
//      ost << "QTC frequency_str get = " << series.frequency_str() << endl;

      sending_series = false;
      qtc_db += series;                  // add to database of sent QTCs

      if (cw)
      { if (drlog_mode == CQ_MODE)                                   // send QSL
          (*cw_p) << expand_cw_message( context.qsl_message() );
      }

      (*win_active_p) <= WINDOW_CLEAR;

// log the QTC series
//      string entries_str;

      append_to_file(context.qtc_filename(), series.complete_output_string());

      win_active_p = (last_active_win_p ? last_active_win_p : &win_call);

// update statistics and summary window
      statistics.qtc_qsos_sent(qtc_buf.n_sent_qsos());
      statistics.qtc_qsos_unsent(qtc_buf.n_unsent_qsos());
//      win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
      display_statistics(statistics.summary_string(rules));

      processed = true;
    }
  }

// CTRL-X, ALT-X -- Abort and go back to prior window
  if (!processed and (e.is_control('x') or e.is_alt('x')))
  { ost << "Aborting; n_sent = " << series.n_sent() << endl;

    if (series.n_sent() != 0)
    { qtc_buf.unsent_to_sent(series[series.size() - 1].first);

      ost << "Aborted sending QTC " << qtc_id << " to " << series.destination() << endl;
      win_qtc_status < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Aborted sending QTC " < qtc_id < " to " <= series.destination();

      series.date(substring(date_time_string(), 0, 10));
      series.utc(hhmmss());
      series.frequency_str(rig.rig_frequency());

      sending_series = false;
      qtc_db += series;                  // add to database of sent QTCs

      (*win_active_p) <= WINDOW_CLEAR;

      append_to_file(context.qtc_filename(), series.complete_output_string());

      win_active_p = (last_active_win_p ? last_active_win_p : &win_call);

// update statistics and summary window
      statistics.qtc_qsos_sent(qtc_buf.n_sent_qsos());
      statistics.qtc_qsos_unsent(qtc_buf.n_unsent_qsos());
//      win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
      display_statistics(statistics.summary_string(rules));
    }
    else  // none sent
    { ost << "Completely aborted; QTC " << qtc_id << " not sent to " << series.destination() << endl;
      win_qtc_status < WINDOW_CLEAR < CURSOR_START_OF_LINE < "Completely aborted; QTC " < qtc_id < " not sent to " <= series.destination();

    (*win_active_p) <= WINDOW_CLEAR;

      win_active_p = (last_active_win_p ? last_active_win_p : &win_call);
//      win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT <= statistics.summary_string(rules);
      display_statistics(statistics.summary_string(rules));
    }
    processed = true;
  }

// ALT-Y -- mark most-recently sent QTC as unsent
  if (!processed and e.is_alt('y'))
  { if (qtcs_sent != 0)
    { series.mark_as_unsent(qtcs_sent--);
      win < WINDOW_CLEAR < WINDOW_TOP_LEFT <= series;
    }

    processed = true;
  }

// T, U -- repeat time
  if (!processed and ( (e.is_char('t')) or (e.is_char('u'))))
  { if (cw)
    { const int qtc_nr = static_cast<int>(qtcs_sent) - 1;

      if ((qtc_nr >= 0) and (qtc_nr < static_cast<int>(series.size())))
        send_msg(series[qtc_nr].first.utc());
    }

    processed = true;
  }

// C -- repeat call
  if (!processed and ( (e.is_char('c'))) )
  { if (cw)
    { const int qtc_nr = static_cast<int>(qtcs_sent) - 1;

      if ((qtc_nr >= 0) and (qtc_nr < static_cast<int>(series.size())))
        send_msg(series[qtc_nr].first.callsign());
    }

    processed = true;
  }

// N, S -- repeat number
  if (!processed and ( (e.is_char('n')) or (e.is_char('s'))))
  { if (cw)
    { const int qtc_nr = static_cast<int>(qtcs_sent) - 1;

      if ((qtc_nr >= 0) and (qtc_nr < static_cast<int>(series.size())))
      { const string serno = pad_string(remove_leading(remove_peripheral_spaces(series[qtc_nr].first.serno()), '0'), 3, PAD_LEFT, 'T');

        send_msg(serno);
      }
    }

    processed = true;
  }

// A, R -- repeat all
  if (!processed and ( (e.is_char('a')) or (e.is_char('r'))))
  { if (cw)
    { const int qtc_nr = static_cast<int>(qtcs_sent) - 1;

      if ((qtc_nr >= 0) and (qtc_nr < static_cast<int>(series.size())))
      { const qtc_entry& qe = series[qtc_nr].first;
        string msg;
        string space = (context.qtc_double_space() ? "  " : " ");

        msg = qe.utc() + space + qe.callsign() + space;

        const string serno = pad_string(remove_leading(remove_peripheral_spaces(qe.serno()), '0'), 3, PAD_LEFT, 'T');

        msg += serno;
        send_msg(msg);
      }
    }

    processed = true;
  }

// PAGE DOWN or CTRL-PAGE DOWN; PAGE UP or CTRL-PAGE UP -- change CW speed
  if (!processed and ((e.symbol() == XK_Next) or (e.symbol() == XK_Prior)))
  { if (cw)
    { int change = (e.is_control() ? 1 : 3);

      if (e.symbol() == XK_Prior)
        change = -change;

      cw_speed(cw_p->speed() - change);  // effective immediately
    }

    processed = true;
  }

// ALT-K -- toggle CW
  if (!processed and cw and e.is_alt('k'))
  { cw_p->toggle();

    win_wpm < WINDOW_CLEAR < CURSOR_START_OF_LINE <= (cw_p->disabled() ? "NO CW" : (to_string(cw_p->speed()) + " WPM") );

    processed = true;
  }

// CTRL-P -- dump screen
  if (!processed and e.is_control('p'))
  { dump_screen();

    processed = true;
  }
}

/*! \brief              Set speed of computer keyer
    \param  new_speed   speed to assign to the keyer

    Also sets key of rig keyer if SYNC KEYER was true in the configuration file
*/
void cw_speed(const unsigned int new_speed)
{ if (cw_p)
  { cw_p->speed(new_speed);
    win_wpm < WINDOW_CLEAR < CURSOR_START_OF_LINE <= (to_string(new_speed) + " WPM");

    try
    { if (context.sync_keyer())
        rig.keyer_speed(new_speed);
    }

    catch (const rig_interface_error& e)
    { alert("Error setting CW speed on rig");
    }
  }
}

/// return the name of the active window in printable form
const string active_window_name(void)
{ string name = "UNKNOWN";

  if (win_active_p == &win_call)
    name = "CALL";

  if (win_active_p == &win_exchange)
    name = "EXCHANGE";

  if (win_active_p == &win_log_extract)
    name = "LOG EXTRACT";

  return name;
}

/*! \brief              Display a callsign in the NEARBY window, in the correct colour
    \param  callsign    Call to display
*/
void display_nearby_callsign(const string& callsign)
{ if (callsign.empty())
    win_nearby <= WINDOW_CLEAR;
  else
  { const bool dupe = logbk.is_dupe(callsign, safe_get_band(), safe_get_mode(), rules);
    const bool worked = q_history.worked(callsign, safe_get_band(), safe_get_mode());
    const int foreground = win_nearby.fg();  // save the default colours
    const int background = win_nearby.bg();

// in what colour should we display this call?
    int colour_pair_number = colours.add(foreground, background);

    if (!worked)
      colour_pair_number = colours.add(COLOUR_GREEN,  background);

    if (dupe)
      colour_pair_number = colours.add(COLOUR_RED,  background);

    win_nearby < WINDOW_CLEAR < CURSOR_START_OF_LINE;
    win_nearby.cpair(colour_pair_number);
    win_nearby < callsign <= COLOURS(foreground, background);
  }
}

#if defined(NEW_CONSTRUCTOR)
/*! \brief                  Debug exchange templates
    \param  rules           rules for the contest
    \param  test_filename   name of file to test

    Prints out which lines in <i>test_filename</i> match the exchange templates defined by <i>rules</i>
*/
void test_exchange_templates(const contest_rules& rules, const string& test_filename)
{ ost << "executing -test-exchanges" << endl;

  const set<string> field_names = rules.all_known_field_names();

  ost << "reading file: " << test_filename << endl;

  try
  { const vector<string> targets = to_lines(read_file(test_filename));

    ost << "contents: " << endl;

    for (const auto& target : targets)
      ost << "  " << target << endl;

    for (const auto& target : targets)
    { vector<string> matches;

      for (const auto& field_name : field_names)
      { const EFT exchange_field_eft(field_name);

        if (exchange_field_eft.is_legal_value(target))
          matches.push_back(field_name);
      }

      ost << "matches for " << target << ": " << endl;

      for (const auto& match : matches)
      { ost << "  " << match << endl;
      }
    }
  }

  catch (const string_function_error& e)
  { ost << "Error: unable to read file: " << test_filename << endl;
  }

  exit(0);
}
#else    // !NEW_CONSTRUCTOR
void test_exchange_templates(const string& test_filename)
{ ost << "executing -test-exchanges" << endl;
  ost << "exchange templates:" << endl;

  const auto db = EXCHANGE_FIELD_TEMPLATES.db();

  for (const auto& kv : db)
    ost << "  " << kv.first << " : " << kv.second << endl;

//  const string test_filename = cl.value("-test-exchanges");

  ost << "reading file: " << test_filename << endl;

  try
  { const vector<string> targets = to_lines(read_file(test_filename));

    ost << "contents: " << endl;

    for (const auto& target : targets)
      ost << "  " << target << endl;

    for (const auto& target : targets)
    { const vector<string> matches = EXCHANGE_FIELD_TEMPLATES.valid_matches(target);

      ost << "matches for " << target << ": " << endl;
      for (const auto& match : matches)
      { ost << "  " << match << endl;
      }
    }
  }

  catch (const string_function_error& e)
  { ost << "Error: unable to read file: " << test_filename << endl;
  }

  exit(0);
}
#endif

/// calculate the value of a mult and update <i>win_mult_value</i>
void update_mult_value(void)
{ const float mult_value = statistics.mult_to_qso_value(rules, safe_get_band(), safe_get_mode());
  const unsigned int mult_value_10 = static_cast<unsigned int>( (mult_value * 10) + 0.5);
  const string term_1 = to_string(mult_value_10 / 10);
  const string term_2 = substring(to_string(mult_value_10 - (10 * (mult_value_10 / 10) )), 0, 1);
  string msg = string("M ≡ ") + term_1 + DP + term_2 + "Q";

  const pair<unsigned int, unsigned int> qs = rate.calculate_rate(900 /* seconds */, 3600);  // rate per hour
  const unsigned int qs_per_hour = qs.first;
  const float mins_per_q = (qs_per_hour ? 60.0 / static_cast<float>(qs_per_hour) : 3600.0);
  const float mins_per_mult = mins_per_q * mult_value;
  string mins("∞");

  if (mins_per_mult < 60)
  { const unsigned int mins_value_10 = static_cast<unsigned int>( (mins_per_mult * 10) + 0.5);
    const string term_1_m = to_string(mins_value_10 / 10);
    const string term_2_m = substring(to_string(mins_value_10 - (10 * (mins_value_10 / 10) )), 0, 1);

    mins = term_1_m + DP + term_2_m;
  }

  msg += string(" ≡ ") + mins + "′";

  try
  { win_mult_value < WINDOW_CLEAR <= centre(msg, 0);
  }

  catch (const string_function_error& e)
  { alert(e.reason());
  }
}

/*! \brief  Write a screenshot to a file
*
*   This is intended to be used as a separate thread, so the parameters are passed
*   in the usual void*
*/
void* auto_screenshot(void* vp)
{
  { start_of_thread();

    try
    { const string* filename_p = static_cast<string*>(vp);

      dump_screen(*filename_p);
    }

    catch (...)
    { ost << "CAUGHT EXCEPTION IN AUTO_SCREENSHOT" << endl;
    }

// manually mark this thread as complete, since we don't want to interrupt the above copy
    { SAFELOCK(thread_check);

      n_running_threads--;
    }
  }  // ensure that all objects call destructors, whatever the implementation

  pthread_exit(nullptr);
}

/*! \brief                  Display the current statistics
    \param  summary_str     summary string from the global running_statistics object
*/
void display_statistics(const string& summary_str)
{ static const set<string> MODE_STRINGS { "CW", "SSB", "All" };

// write the string, but don't refresh the window
  win_summary < WINDOW_CLEAR < CURSOR_TOP_LEFT < summary_str;

  if (rules.permitted_modes().size() > 1)
  { for (unsigned int n = 0; n < win_summary.height(); ++n)
    {
// we have to be a bit complicated because we need to have spaces after the string, so that the colours for the entire line are handled correctly
      const string line = remove_peripheral_spaces(win_summary.getline(n));

      if (MODE_STRINGS < line)
        win_summary < cursor(0, n) < WINDOW_REVERSE <  create_centred_string(line, win_summary.width()) < WINDOW_NORMAL;
    }
  }

  win_summary.refresh();        // now OK to refresh
}
