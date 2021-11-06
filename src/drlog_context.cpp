// $Id: drlog_context.cpp 195 2021-11-01 01:21:22Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   drlog_context.cpp

    The basic context for operation of drlog
*/

#include "diskfile.h"
#include "drlog_context.h"
#include "exchange.h"
#include "string_functions.h"

#include <fstream>
#include <iostream>

using namespace std;
//using namespace std::ranges;

extern message_stream   ost;                        ///< for debugging, info
extern bool             QSO_DISPLAY_COUNTRY_MULT;   ///< controls whether country mults are written on the log line
extern int              QSO_MULT_WIDTH;             ///< controls width of zone mults field in log line

/// example: cabrillo qso = template: CQ WW
static const map<string, string> cabrillo_qso_templates { { "ARRL DX"s, "ARRL DX"s }, // placeholder; mode chosen before we exit this function
                                                          { "ARRL DX CW"s, "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-STATE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CWPOWER:74:6:R, TXID:81:1"s },
                                                          { "ARRL DX SSB"s, "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RS:45:3:R, TEXCH-STATE:49:6:R, RCALL:56:13:R, REXCH-RS:70:3:R, REXCH-SSBPOWER:74:6:R, TXID:81:1"s },

                                                          { "CQ WW"s,      "CQ WW"s }, // placeholder; mode chosen before we exit this function
                                                          { "CQ WW CW"s,   "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1"s },
                                                          { "CQ WW SSB"s,  "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RS:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RS:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1"s },

                                                          { "JIDX"s,      "JIDX"s }, // placeholder; mode chosen before we exit this function
                                                          { "JIDX CW"s,   "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-JAPREF:74:6:R, TXID:81:1"s },
                                                          { "JIDX SSB"s,  "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RS:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RS:70:3:R, REXCH-JAPREF:74:6:R, TXID:81:1"s }
                                                        };

/*! \brief          Write an error message to the output file, then exit
    \param  line    the line that caused the error
*/
void print_error_and_exit(const string& line)
{ ost << "Parse error in line: " << line << endl;
  exit(-1);
}

// -----------  drlog_context  ----------------

/*! \class  drlog_context
    \brief  The variables and constants that comprise the context for drlog
*/

pt_mutex _context_mutex { "DRLOG CONTEXT"s };                    ///< mutex for the context

/*! \brief              Set the value of points, using the POINTS [CW|SSB] command
    \param  command     the complete line from the configuration file
    \param  m           mode
*/
void drlog_context::_set_points(const string& command, const MODE m)
{ if (command.empty())
    return;

  const vector<string> str_vec { split_string(command, "="s) };

  if (str_vec.size() != 2)
  { ost << "Invalid command: " << command << endl;
    exit(0);
  }

  const string RHS { to_upper(remove_peripheral_spaces(str_vec[1])) };

  if (!str_vec.empty())
  { const string lhs { str_vec[0] };

    if (auto& pbb { _per_band_points[m] }; !contains(lhs, "["s) or contains(lhs, "[*]"s))            // for all bands
    { for (unsigned int n { 0 }; n < NUMBER_OF_BANDS; ++n)
        pbb += { static_cast<BAND>(n), RHS };
    }
    else                                                        // not all bands
    { const size_t left_bracket_posn  { lhs.find('[') };
      const size_t right_bracket_posn { lhs.find(']') };
      const bool   valid              { (left_bracket_posn != string::npos) and (right_bracket_posn != string::npos) and (left_bracket_posn < right_bracket_posn) };

      if (valid)
      { const string bands_str     { lhs.substr(left_bracket_posn + 1, (right_bracket_posn - left_bracket_posn - 1)) };
//        const vector<string> bands { remove_peripheral_spaces(split_string(bands_str, ","s)) };

//        FOR_ALL(bands, [=, &pbb] (const string& b_str) { pbb += { BAND_FROM_NAME[b_str], RHS }; } );
//        std::ranges::for_each(bands, [=, &pbb] (const string& b_str) { pbb += { BAND_FROM_NAME[b_str], RHS }; } );
//        ranges::for_each(remove_peripheral_spaces(split_string(bands_str, ","s)), [=, &pbb] (const string& b_str) { pbb += { BAND_FROM_NAME[b_str], RHS }; } );
        FOR_ALL(clean_split_string(bands_str, ','), [=, &pbb] (const string& b_str) { pbb += { BAND_FROM_NAME[b_str], RHS }; } );
      }
    }
  }
}

/*! \brief              Process a configuration file
    \param  filename    name of file to process

    This routine may be called recursively (by the RULES statement in the processed file)
*/
void drlog_context::_process_configuration_file(const string& filename)
{ string entire_file;

  try
  { entire_file = remove_char(read_file(filename), CR_CHAR);
  }

  catch (...)
  { ost << "Unable to open configuration file " << filename << endl;
    exit(-1);
  }

  const vector<string> lines { split_string(entire_file, LF_STR) };   // split into lines

  for (const auto& tmpline : lines)                                   // process each line
  { const string line { remove_trailing_comment(tmpline) };           // remove any comment

// generate a number of useful variables
    const string testline       { remove_leading_spaces(to_upper(line)) };
    const vector<string> fields { split_string(line, "="s) };
    const string rhs            { ((fields.size() > 1) ? remove_peripheral_spaces(fields[1]) : ""s) };      // the stuff to the right of the "="
    const string RHS            { to_upper(rhs) };                                                          // converted to upper case
    const bool is_true          { (RHS == "TRUE"s) };                                                       // is right hand side == "TRUE"?
    const string lhs            { squash( (fields.empty()) ? ""s : remove_peripheral_spaces(fields[0]) ) }; // the stuff to the left of the "="
    const string LHS            { to_upper(lhs) };                                                          // converted to upper case

// ACCEPT COLOUR
    if ( ( (LHS == "ACCEPT COLOUR"s) or (LHS == "ACCEPT COLOR"s) ) and !rhs.empty() )
      _accept_colour = string_to_colour(RHS);

// ALLOW AUDIO RECORDING
    if (LHS == "ALLOW AUDIO RECORDING"s)
      _allow_audio_recording = is_true;

// ALTERNATIVE EXCHANGE CQ
    if (LHS == "ALTERNATIVE EXCHANGE CQ"s)
      _alternative_exchange_cq = RHS;

// ALTERNATIVE EXCHANGE SAP
    if (LHS == "ALTERNATIVE EXCHANGE SAP"s)
      _alternative_exchange_sap = RHS;

// ALTERNATIVE QSL MESSAGE
    if ( (LHS == "ALTERNATIVE QSL MESSAGE"s) or (LHS == "QUICK QSL MESSAGE"s) )
      _alternative_qsl_message = RHS;

// ARCHIVE
    if ( (LHS == "ARCHIVE"s) and !rhs.empty() )
      _archive_name = rhs;

// AUDIO CHANNELS
    if (LHS == "AUDIO CHANNELS"s)
      _audio_channels = from_string<decltype(_audio_channels)>(rhs);

// AUDIO DEVICE
    if ( (LHS == "AUDIO DEVICE"s) or (LHS == "AUDIO DEVICE NAME"s) )
      _audio_device_name = rhs;

// AUDIO DURATION
    if (LHS == "AUDIO DURATION"s)
      _audio_duration = from_string<decltype(_audio_duration)>(rhs);

// AUDIO FILE
    if (LHS == "AUDIO FILE"s)
      _audio_file = rhs;

// AUDIO RATE
    if (LHS == "AUDIO RATE"s)
      _audio_rate = from_string<decltype(_audio_rate)>(rhs);

// AUTO BACKUP
    if ( ((LHS == "AUTO BACKUP DIRECTORY"s) or (LHS == "AUTO BACKUP"s)) and !rhs.empty() )  // AUTO BACKUP was the old name for this command
      _auto_backup_directory = rhs;

// AUTO CQ MODE SSB
    if (LHS == "AUTO CQ MODE SSB"s)
      _auto_cq_mode_ssb = is_true;

// AUTO SCREENSHOT
    if ( (LHS == "AUTO SCREENSHOT"s) and !rhs.empty() )
      _auto_screenshot = is_true;

// BAND MAP CULL FUNCTION
    if ( ( (LHS == "BAND MAP CULL FUNCTION"s) or (LHS == "BANDMAP CULL FUNCTION"s) ) and !rhs.empty() )
      _bandmap_cull_function = from_string<decltype(_bandmap_cull_function)>(rhs);

// BAND MAP DECAY TIME CLUSTER
    if ( ( (LHS == "BAND MAP DECAY TIME CLUSTER"s) or (LHS == "BANDMAP DECAY TIME CLUSTER"s) ) and !rhs.empty() )
      _bandmap_decay_time_cluster = from_string<decltype(_bandmap_decay_time_cluster)>(rhs);

// BAND MAP DECAY TIME LOCAL
    if ( ( (LHS == "BAND MAP DECAY TIME LOCAL"s) or (LHS == "BANDMAP DECAY TIME LOCAL"s) ) and !rhs.empty() )
      _bandmap_decay_time_local = from_string<decltype(_bandmap_decay_time_local)>(rhs);

// BAND MAP DECAY TIME RBN
    if ( (LHS == "BAND MAP DECAY TIME RBN"s) or (LHS == "BANDMAP DECAY TIME RBN"s) )
      _bandmap_decay_time_rbn = from_string<decltype(_bandmap_decay_time_rbn)>(rhs);

// BAND MAP FADE COLOURS
    if ( (LHS == "BAND MAP FADE COLOURS"s) or (LHS == "BANDMAP FADE COLOURS"s) or
         (LHS == "BAND MAP FADE COLORS"s) or (LHS == "BANDMAP FADE COLORS"s) )
    { _bandmap_fade_colours.clear();

      if (!RHS.empty())
      { //const vector<string> colour_names { remove_peripheral_spaces(split_string(RHS, ","s)) };

        FOR_ALL(clean_split_string(RHS, ','), [&] (const string& name) { _bandmap_fade_colours += string_to_colour(name); } );
      }
    }

// BAND MAP FILTER
    if ( (LHS == "BAND MAP FILTER"s) or (LHS == "BANDMAP FILTER"s) )
    { if (!RHS.empty())
      { vector<string> filters { clean_split_string(RHS, '.') };

        SORT(filters, compare_calls);    // put the entries into callsign order
        _bandmap_filter = filters;
      }
    }

// BAND MAP FILTER COLOURS
    if ( (LHS == "BAND MAP FILTER COLOURS"s) or (LHS == "BAND MAP FILTER COLORS"s) or
         (LHS == "BANDMAP FILTER COLOURS"s) or (LHS == "BANDMAP FILTER COLORS"s) )
    { if (!RHS.empty())
      { //const vector<string> colours { split_string(RHS, ","s) };
        const vector<string> colours { clean_split_string(RHS, ',') };

        if (colours.size() >= 1)
          _bandmap_filter_foreground_colour = string_to_colour(colours[0]);

        if (colours.size() >= 2)
          _bandmap_filter_hide_colour = string_to_colour(colours[1]);

        if (colours.size() >= 3)
          _bandmap_filter_show_colour = string_to_colour(colours[2]);

        if (colours.size() >= 4)
          _bandmap_filter_disabled_colour = string_to_colour(colours[3]);
      }
    }

// BAND MAP FILTER ENABLE
    if ( (LHS == "BAND MAP FILTER ENABLE"s) or (LHS == "BANDMAP FILTER ENABLE"s) )
      _bandmap_filter_enabled = is_true;

// BAND MAP FILTER MODE
    if ( (LHS == "BAND MAP FILTER MODE"s) or (LHS == "BANDMAP FILTER MODE"s) )
      _bandmap_filter_show = (RHS == "SHOW"s);

// BAND MAP FREQUENCY UP
    if ( (LHS == "BAND MAP FREQUENCY UP"s) or (LHS == "BANDMAP FREQUENCY UP"s) )
      _bandmap_frequency_up = is_true;

// BAND MAP GUARD BAND CW
    if ( (LHS == "BAND MAP GUARD BAND CW"s) or (LHS == "BANDMAP GUARD BAND CW"s) )
      _guard_band[MODE_CW] = from_string<unsigned int>(rhs);

// BAND MAP GUARD BAND SSB
    if ( (LHS == "BAND MAP GUARD BAND SSB"s) or (LHS == "BANDMAP GUARD BAND SSB"s) )
      _guard_band[MODE_SSB] = from_string<unsigned int>(rhs);

// BAND MAP RECENT COLOUR
    if ( (LHS == "BAND MAP RECENT COLOUR"s) or (LHS == "BANDMAP RECENT COLOUR"s) or
         (LHS == "BANDMAP RECENT COLOR"s) or (LHS == "BANDMAP RECENT COLOR"s) )
    { if (!RHS.empty())
        _bandmap_recent_colour = string_to_colour(remove_peripheral_spaces(RHS));
    }

// BAND MAP SHOW MARKED FREQUENCIES
    if ( (LHS == "BAND MAP SHOW MARKED FREQUENCIES"s) or (LHS == "BANDMAP SHOW MARKED FREQUENCIES"s) )
      _bandmap_show_marked_frequencies = is_true;

// BANDS
    if (LHS == "BANDS"s)
      _bands = RHS;

// BATCH MESSAGES FILE
    if (LHS == "BATCH MESSAGES FILE"s)
      _batch_messages_file = rhs;

// BEST DX UNIT
    if ( (LHS == "BEST DX UNIT"s) or (LHS == "BEST DX UNITS"s) )
      _best_dx_unit = RHS;

// CABRILLO FILENAME
    if (LHS == "CABRILLO FILENAME"s)
      _cabrillo_filename = rhs;

// CALL HISTORY BANDS
    if (LHS == "CALL HISTORY BANDS"s)
    { //const string         bands_str     { rhs };
      //const vector<string> bands_str_vec { remove_peripheral_spaces(split_string(bands_str, ","s)) };
      
      //for (const auto& band_str : bands_str_vec)
      //  _call_history_bands += BAND_FROM_NAME[band_str]; 
      FOR_ALL(clean_split_string(rhs, ','), [this] (const string& band_str) { _call_history_bands += BAND_FROM_NAME[band_str]; });
    }

// CALL OK NOW MESSAGE
    if (LHS == "CALL OK NOW MESSAGE"s)
      _call_ok_now_message = rhs;

// CALLSIGN MULTS
    if (LHS == "CALLSIGN MULTS"s)
    { //const vector<string> callsign_mults_vec { remove_peripheral_spaces(split_string(RHS, ","s)) };

//      move(callsign_mults_vec.cbegin(), callsign_mults_vec.cend(), inserter(_callsign_mults, _callsign_mults.begin()));
//      ranges::move(callsign_mults_vec, inserter(_callsign_mults, _callsign_mults.begin()));
      ranges::move(clean_split_string(RHS, ','), inserter(_callsign_mults, _callsign_mults.begin()));
    }

// CALLSIGN MULTS PER BAND
    if (LHS == "CALLSIGN MULTS PER BAND"s)
      _callsign_mults_per_band = is_true;

// CALLSIGN MULTS PER MODE
    if (LHS == "CALLSIGN MULTS PER MODE"s)
      _callsign_mults_per_mode = is_true;

// CLUSTER PORT
    if (LHS == "CLUSTER PORT"s)
      _cluster_port = from_string<decltype(_cluster_port)>(rhs);

// CLUSTER SERVER
    if (LHS == "CLUSTER SERVER"s)
      _cluster_server = rhs;

// CLUSTER USERNAME
    if (LHS == "CLUSTER USERNAME"s)
      _cluster_username = rhs;

// CONTEST
    if (LHS == "CONTEST"s)
      _contest_name = RHS;

// COUNTRY FILENAME
    if (LHS == "COUNTRY FILENAME"s)
      _cty_filename = rhs;

// COUNTRY LIST
    if ( LHS == "COUNTRY LIST"s)
    { if (RHS == "DXCC"s)
        _country_list = COUNTRY_LIST::DXCC;

      if ( (RHS == "WAEDC"s) or (RHS == "WAE"s) )
        _country_list = COUNTRY_LIST::WAEDC;
    }

// COUNTRY MULT FACTOR
    if (LHS == "COUNTRY MULT FACTOR"s)  // there may be an "=" in the points definitions
    { const vector<string> str_vec { split_string(line, "="s) };

      if (!str_vec.empty())
      { string tmp_str;

        const string lhs { str_vec[0] };

        if (!contains(lhs, "["s) or contains(lhs, "[*]"s))             // for all bands
        { string new_str;

          for (unsigned int n { 1 }; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
          { new_str += str_vec[n];

            if (n != str_vec.size() - 1)
              new_str += "="s;
          }

          tmp_str = to_upper(remove_peripheral_spaces(new_str));

          for (unsigned int n { 0 }; n < NUMBER_OF_BANDS; ++n)
            _per_band_country_mult_factor += { static_cast<BAND>(n), from_string<int>(tmp_str) };
        }
        else    // not all bands
        { const string         bands_str { delimited_substring(lhs, '[', ']', DELIMITERS::DROP) };
          const vector<string> bands     { remove_peripheral_spaces(split_string(bands_str, ","s)) };

          for (const auto b_str: bands)
          { const BAND b { BAND_FROM_NAME[b_str] };

            string new_str;

            for (unsigned int n { 1 }; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
            { new_str += str_vec[n];

              if (n != str_vec.size() - 1)
                new_str += "="s;
            }

            tmp_str = to_upper(remove_peripheral_spaces(new_str));
            _per_band_country_mult_factor += { b, from_string<int>(tmp_str) };
          }
        }
      }
    }

// COUNTRY MULTS PER BAND
    if (LHS == "COUNTRY MULTS PER BAND"s)
      _country_mults_per_band = is_true;

// COUNTRY MULTS PER MODE
    if (LHS == "COUNTRY MULTS PER MODE"s)
      _country_mults_per_mode = is_true;

// CQ AUTO LOCK
    if (LHS == "CQ AUTO LOCK"s)
      _cq_auto_lock = is_true;

// CQ AUTO RIT
    if (LHS == "CQ AUTO RIT"s)
      _cq_auto_rit = is_true;

// CW BANDWIDTH
    if (LHS == "CW BANDWIDTH"s)
    { const vector<string> bw { remove_peripheral_spaces(split_string(RHS, "/"s)) };

      if (bw.size() == 2)
      { _cw_bandwidth_narrow = from_string<decltype(_cw_bandwidth_narrow)>(bw[0]);
        _cw_bandwidth_wide = from_string<decltype(_cw_bandwidth_wide)>(bw[1]);
      }
    }

// CW PRIORITY
    if (LHS == "CW PRIORITY"s)
      _cw_priority = from_string<decltype(_cw_priority)>(RHS);

// CW SPEED
    if (LHS == "CW SPEED"s)
      _cw_speed = from_string<decltype(_cw_speed)>(RHS);

// CW SPEED CHANGE
    if (LHS == "CW SPEED CHANGE"s)
      _cw_speed_change = from_string<decltype(_cw_speed_change)>(RHS);

// DECIMAL POINT
    if (LHS == "DECIMAL POINT"s)
      _decimal_point = rhs;

// DISPLAY COMMUNICATION ERRORS
    if (LHS == "DISPLAY COMMUNICATION ERRORS"s)
      _display_communication_errors = is_true;

// DISPLAY GRID
    if (LHS == "DISPLAY GRID"s)
      _display_grid = is_true;

// DO NOT SHOW
    if (LHS == "DO NOT SHOW"s)
      _do_not_show = remove_peripheral_spaces(split_string(RHS, ","s));

// DO NOT SHOW FILE
    if (LHS == "DO NOT SHOW FILE"s)
      _do_not_show_filename = rhs;

// DRMASTER FILENAME
    if (LHS == "DRMASTER FILENAME"s)
      _drmaster_filename = rhs;

// EXCHANGE
    if (LHS == "EXCHANGE"s)
      _exchange = RHS;

// EXCHANGE[
    if (starts_with(testline, "EXCHANGE["s))
    { const string         country_list { delimited_substring(LHS, '[', ']', DELIMITERS::DROP) };
      //const vector<string> countries    { remove_peripheral_spaces(split_string(country_list, ',')) };

//      FOR_ALL(countries, [&] (const string& str) { _exchange_per_country += { str, RHS }; } );
      FOR_ALL(clean_split_string(country_list, ','), [&] (const string& str) { _exchange_per_country += { str, RHS }; } );
    }

// EXCHANGE CQ
    if (LHS == "EXCHANGE CQ"s)
      _exchange_cq = RHS;

// EXCHANGE FIELDS FILENAME (for all the regex-based exchange fields)
    if (LHS == "EXCHANGE FIELDS FILENAME"s)
      _exchange_fields_filename = rhs;

// EXCHANGE MULTS
    if (LHS == "EXCHANGE MULTS"s)
    { _exchange_mults = RHS;

      if (contains(_exchange_mults, ","s))      // if there is more than one exchange mult
        QSO_MULT_WIDTH = 4;                     // make them all the same width, so that the log line looks OK
    }

// EXCHANGE MULTS PER BAND
    if (LHS == "EXCHANGE MULTS PER BAND"s)
      _exchange_mults_per_band = is_true;

// EXCHANGE MULTS PER MODE
    if (LHS == "EXCHANGE MULTS PER MODE"s)
      _exchange_mults_per_mode = is_true;

// EXCHANGE PREFILL FILE
//   exchange prefill file = [ exchange-field-name, filename ]
    if ( (LHS == "EXCHANGE PREFILL FILE"s) or (LHS == "EXCHANGE PREFILL FILES"s) )
    { const vector<string> files { remove_peripheral_spaces(delimited_substrings(rhs, '[', ']', DELIMITERS::DROP)) };

      for (const auto& file : files)
      { //const vector<string> fields { remove_peripheral_spaces(split_string(file, ","s)) };
        const vector<string> fields { clean_split_string(file, ',') };

        if (fields.size() == 2)
          _exchange_prefill_files[to_upper(fields[0])] = fields[1];
      }
    }

// EXCHANGE SAP
    if (LHS == "EXCHANGE SAP"s)
      _exchange_sap = RHS;

// EXCHANGE SENT
// e.g., exchange sent = RST:599, CQZONE:4
    if (LHS == "EXCHANGE SENT"s)
    { //const string         comma_delimited_list { to_upper(remove_peripheral_spaces((split_string(line, "="s))[1])) };    // RST:599, CQZONE:4
      const string         comma_delimited_list { to_upper(clean_split_string(line, '=')[1]) };    // RST:599, CQZONE:4
      const vector<string> fields               { split_string(comma_delimited_list, ","s) };

      for (const auto& this_field : fields)
      { const vector<string> field { split_string(this_field, ":"s) };

        _sent_exchange += { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) };
      }
    }

// EXCHANGE SENT CW
    if (LHS == "EXCHANGE SENT CW"s)
    { //const string         comma_delimited_list { to_upper(remove_peripheral_spaces((split_string(line, "="s))[1])) };    // RST:599, CQZONE:4
      const string         comma_delimited_list { to_upper(clean_split_string(line, '=')[1]) };    // RST:599, CQZONE:4
      const vector<string> fields               { split_string(comma_delimited_list, ","s) };

      for (const auto& this_field : fields)
      { const vector<string> field { split_string(this_field, ":"s) };

        if (fields.size() != 2)
          print_error_and_exit(testline);

        _sent_exchange_cw += { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) };
      }
    }

// EXCHANGE SENT SSB
    if (LHS == "EXCHANGE SENT SSB"s)
    { const string         comma_delimited_list { to_upper(remove_peripheral_spaces((split_string(line, "="s))[1])) };    // RST:599, CQZONE:4
      const vector<string> fields               { split_string(comma_delimited_list, ","s) };

      for (const auto& this_field : fields)
      { const vector<string> field { split_string(this_field, ":"s) };

        if (fields.size() != 2)
          print_error_and_exit(testline);

        _sent_exchange_ssb += { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) };
      }
    }

// FAST CQ BANDWIDTH; used only in CW mode
    if (LHS == "FAST CQ BANDWIDTH"s)
      _fast_cq_bandwidth = from_string<decltype(_fast_cq_bandwidth)>(RHS);

// FAST SAP BANDWIDTH; used only in CW mode
    if (LHS == "FAST SAP BANDWIDTH"s)
      _fast_sap_bandwidth = from_string<decltype(_fast_sap_bandwidth)>(RHS);

// GEOMAGNETIC INDICES COMMAND
    if (LHS == "GEOMAGNETIC INDICES COMMAND"s)
      _geomagnetic_indices_command = rhs;

// HOME EXCHANGE WINDOW
    if (LHS == "HOME EXCHANGE WINDOW"s)
      _home_exchange_window = is_true;

// INACTIVTY TIMER
    if (LHS == "INACTIVITY TIMER"s)
      _inactivity_timer = from_string<decltype(_inactivity_timer)>(rhs);

// INDIVIDUAL MESSAGES FILE
    if (LHS == "INDIVIDUAL MESSAGES FILE"s)
      _individual_messages_file = rhs;

// KEYER PORT
    if (LHS == "KEYER PORT"s)
      _keyer_port = rhs;

// LOG
    if ( (LHS == "LOG"s) and !rhs.empty() )
      _logfile = rhs;

// LONG T
    if (LHS == "LONG T"s)
      _long_t = from_string<decltype(_long_t)>(rhs);

// MARK FREQUENCIES [CW|SSB]
    if (starts_with(testline, "MARK FREQUENCIES"s) and !rhs.empty())
    { const vector<string> ranges { remove_peripheral_spaces(split_string(rhs, ","s)) };

      vector<pair<frequency, frequency>> frequencies;

      for (const string& range : ranges)
      { const vector<string> bounds { remove_peripheral_spaces(split_string(range, "-"s)) };

        try
        { frequencies += { frequency(bounds.at(0)), frequency(bounds.at(1))};
        }

        catch (...)
        { print_error_and_exit(testline);
        }
      }

      if (LHS == "MARK FREQUENCIES"s or LHS == "MARK FREQUENCIES CW"s)
        _mark_frequencies += { MODE_CW, frequencies };

      if (LHS == "MARK FREQUENCIES"s or LHS == "MARK FREQUENCIES SSB"s)
        _mark_frequencies += { MODE_SSB, frequencies };
    }

// MATCH MINIMUM
    if (LHS == "MATCH MINIMUM"s)
      _match_minimum = from_string<decltype(_match_minimum)>(RHS);

// MATCH MINIMUM
    if (LHS == "MAX QSOS WITHOUT QSL"s)
      _max_qsos_without_qsl = from_string<decltype(_max_qsos_without_qsl)>(RHS);

// MM COUNTRY MULTS
    if (LHS == "MM COUNTRY MULTS"s)
      _mm_country_mults = is_true;

// MODES
    if (LHS == "MODES"s)
    { _modes = RHS;

      if (contains(_modes, ","s))        // if more than one mode
        _mark_mode_break_points = true;
      else
      { if (_modes == "SSB"s)
          _start_mode = MODE_SSB;
      }
    }

// MODE BREAK POINTS
    if (LHS == "MODE BREAK POINTS"s)
    { const vector<string> break_points { remove_peripheral_spaces(split_string(RHS, ',')) };

      for (const auto& break_point : break_points)
      { const frequency f { break_point };
        const BAND      b { to_BAND(f) };

//        _mode_break_points.insert( { b, f } );
        _mode_break_points += { b, f };
      }
    }

// MY CALL
    if (LHS == "MY CALL"s)
      _my_call = RHS;

// MY CONTINENT
    if (LHS == "MY CONTINENT"s)
      _my_continent = RHS;

// MY CQ ZONE
    if (LHS == "MY CQ ZONE"s)
      _my_cq_zone = from_string<decltype(_my_cq_zone)>(RHS);

// MY GRID
    if (LHS == "MY GRID"s)
      _my_grid = rhs;

// MY IP
    if (LHS == "MY IP"s)
      _my_ip = rhs;

// MY LATITUDE
    if (LHS == "MY LATITUDE"s)
      _my_latitude = from_string<decltype(_my_latitude)>(rhs);

// MY LATITUDE
    if (LHS == "MY LONGITUDE"s)
      _my_longitude = from_string<decltype(_my_longitude)>(rhs);

// MY ITU ZONE
    if (LHS == "MY ITU ZONE"s)
      _my_itu_zone = from_string<decltype(_my_itu_zone)>(RHS);

// N MEMORIES
    if (LHS == "N MEMORIES"s)
      _n_memories = min(from_string<decltype(_n_memories)>(rhs), 10u);  // maximum number of memories is 10

// NEARBY EXTRACT
    if (LHS == "NEARBY EXTRACT"s)
      _nearby_extract = is_true;

// NO DEFAULT RST
    if (LHS == "NO DEFAULT RST"s)
      _no_default_rst = is_true;

// NORMALISE RATE(S)
    if ( (LHS == "NORMALISE RATE"s) or (LHS == "NORMALIZE RATE"s) or (LHS == "NORMALISE RATES"s) or (LHS == "NORMALIZE RATES"s) )
      _normalise_rate = is_true;

// NOT COUNTRY MULTS
    if (LHS == "NOT COUNTRY MULTS"s)
    { if (!rhs.empty())
        _not_country_mults = rhs;
    }

// OLD ADIF LOG NAME
    if (LHS == "OLD ADIF LOG NAME"s)
      _old_adif_log_name = rhs;

// OLD QSO AGE LIMIT
    if (LHS == "OLD QSO AGE LIMIT"s)
      _old_qso_age_limit = from_string<decltype(_old_qso_age_limit)>(rhs);

// PATH
    if (LHS == "PATH"s)
    { if (!rhs.empty())
        _path = remove_peripheral_spaces(split_string(rhs, ";"s));
    }

// POINTS
// don't use LHS here because the command might be something like "POINTS[80] ="
    if (starts_with(testline, "POINTS"s) and !starts_with(testline, "POINTS CW"s) and !starts_with(testline, "POINTS SSB"s))  // there may be an "=" in the points definitions
    { _set_points(testline, MODE_CW);
      _set_points(testline, MODE_SSB);
    }

// POINTS CW
    if (starts_with(testline, "POINTS CW"s))
      _set_points(testline, MODE_CW);

// POINTS SSB
    if (starts_with(testline, "POINTS SSB"s))
      _set_points(testline, MODE_SSB);

// PTT DELAY (0 => no PTT)
    if (LHS == "PTT DELAY"s)
      _ptt_delay = from_string<decltype(_ptt_delay)>(RHS);

// POST MONITOR
    if (LHS == "POST MONITOR"s)
    { //const vector<string> calls { remove_peripheral_spaces(split_string(RHS, ',')) };

      //FOR_ALL(calls, [&] (const string& callsign) { _post_monitor_calls += callsign; } );
      FOR_ALL(remove_peripheral_spaces(split_string(RHS, ',')), [&] (const string& callsign) { _post_monitor_calls += callsign; } );
    }

// POSTED BY CONTINENTS
    if (LHS == "POSTED BY CONTINENTS"s)
    { const set<string>    continent_abbreviations { "AF"s, "AS"s, "EU"s, "NA"s, "OC"s, "SA"s, "AN"s };
//      const vector<string> continents_from_file    { remove_peripheral_spaces(split_string(RHS, ',')) };
      
//      FOR_ALL(continents_from_file, [&] (const string& co) { if ( continent_abbreviations > co ) _posted_by_continents += co; } );
      FOR_ALL(remove_peripheral_spaces(split_string(RHS, ',')), [&] (const string& co) { if ( continent_abbreviations > co ) _posted_by_continents += co; } );
    }

// P3
    if (LHS == "P3"s)
      _p3 = is_true;

// P3 IGNORE CHECKSUM ERROR
    if (LHS == "P3 IGNORE CHECKSUM ERROR"s)
      _p3_ignore_checksum_error = is_true;

// P3 SNAPSHOT FILE
    if (LHS == "P3 SNAPSHOT FILE"s)
      _p3_snapshot_file = rhs;

// P3 SPAN CQ
    if (LHS == "P3 SPAN CQ"s)
      _p3_span_cq = from_string<decltype(_p3_span_cq)>(RHS);

// P3 SPAN SAP
    if (LHS == "P3 SPAN SAP"s)
      _p3_span_sap = from_string<decltype(_p3_span_sap)>(RHS);

// QSL MESSAGE
    if (LHS == "QSL MESSAGE"s)
      _qsl_message = RHS;

// QSO MULTIPLE BANDS
    if (LHS == "QSO MULTIPLE BANDS"s)
      _qso_multiple_bands = is_true;

// QSO MULTIPLE MODES
    if (LHS == "QSO MULTIPLE MODES"s)
      _qso_multiple_modes = is_true;

// QSY ON STARTUP
    if (LHS == "QSY ON STARTUP"s)
      _qsy_on_startup = is_true;

// QTCS
    if (LHS == "QTCS"s)
      _qtcs = is_true;

// QTC DOUBLE SPACE
    if (LHS == "QTC DOUBLE SPACE"s)
      _qtc_double_space = is_true;

// QTC FILENAME
    if (LHS == "QTC FILENAME"s)
      _qtc_filename = rhs;

// QTC LONG T
    if (LHS == "QTC LONG T"s)
      _qtc_long_t = from_string<decltype(_qtc_long_t)>(rhs);

// QTC QRS
    if (LHS == "QTC QRS"s)
      _qtc_qrs = from_string<decltype(_qtc_qrs)>(rhs);

// QTHX: QTHX[callsign-or-canonical prefix] = aa, bb, cc...
// the conversion to canonical prefix occurs later, inside contest_rules::_parse_context_qthx()
    if (starts_with(testline, "QTHX["s))
    { const vector<string> fields { remove_peripheral_spaces(split_string(testline, "="s)) };

      if (fields.size() == 2)
      { const string         canonical_prefix { delimited_substring(fields[0], '[', ']', DELIMITERS::DROP) };
        const vector<string> values           { remove_peripheral_spaces(split_string(RHS, ","s)) };
        const set<string>    ss               { values.cbegin(), values.cend() };

        _qthx += { canonical_prefix, ss };
      }
    }

// RATE
    if (LHS == "RATE"s)
    { //const vector<string> vec_rates { remove_peripheral_spaces(split_string(rhs, ","s)) };

      vector<unsigned int> new_rates;

//      FOR_ALL(vec_rates, [&new_rates] (const string& str) { new_rates += from_string<unsigned int>(str); } );
      FOR_ALL(remove_peripheral_spaces(split_string(rhs, ","s)), [&new_rates] (const string& str) { new_rates += from_string<unsigned int>(str); } );

      if (!new_rates.empty())
        _rate_periods = new_rates;
    }

// RBN BEACONS
    if (LHS == "RBN BEACONS"s)
      _rbn_beacons = is_true;

// RBN PORT
    if (LHS == "RBN PORT"s)
      _rbn_port = from_string<decltype(_rbn_port)>(rhs);

// RBN SERVER
    if (LHS == "RBN SERVER"s)
      _rbn_server = rhs;

// RBN THRESHOLD
    if (LHS == "RBN THRESHOLD"s)
      _rbn_threshold = from_string<decltype(_rbn_threshold)>(rhs);

// RBN USERNAME
    if (LHS == "RBN USERNAME"s)
      _rbn_username = rhs;

// REJECT COLOUR
    if ( ( (LHS == "REJECT COLOUR"s) or (LHS == "REJECT COLOR"s) ) and !rhs.empty() )
      _reject_colour = string_to_colour(RHS);

// REQUIRE DOT IN REPLACEMENT CALL
    if (LHS == "REQUIRE DOT IN REPLACEMENT CALL"s)
      _require_dot_in_replacement_call = is_true;

// RIG 1 BAUD
    if ( (LHS == "RIG 1 BAUD"s) or (LHS == "RIG BAUD"s) )
      _rig1_baud = from_string<decltype(_rig1_baud)>(rhs);

// RIG 1 DATA BITS
    if ( (LHS == "RIG 1 DATA BITS"s) or (LHS == "RIG DATA BITS"s) )
      _rig1_data_bits = from_string<decltype(_rig1_data_bits)>(rhs);

// RIG 1 NAME
    if ( (LHS == "RIG 1 NAME"s) or (LHS == "RADIO ONE NAME"s) )
      _rig1_name = rhs;

// RIG 1 PORT
    if ( (LHS == "RIG 1 PORT"s) or (LHS == "RADIO ONE CONTROL PORT"s) )
      _rig1_port = rhs;

// RIG 1 STOP BITS
    if ( (LHS == "RIG 1 STOP BITS"s) or (LHS == "RIG STOP BITS"s) )
      _rig1_stop_bits = from_string<decltype(_rig1_stop_bits)>(rhs);

// RIG 1 TYPE
    if (LHS == "RIG 1 TYPE"s)
      _rig1_type = RHS;

// RULES
    if (LHS == "RULES"s)
      _process_configuration_file(rhs);

// RUSSIAN DATA
    if (LHS == "RUSSIAN DATA"s)
      _russian_filename = rhs;

// SCORE BANDS
    if (starts_with(testline, "SCORE BANDS"s))
    { const vector<string> bands_str { remove_peripheral_spaces(split_string(rhs, ","s)) };

      for (const auto& band_str : bands_str)
      { try
        { //_score_bands.insert(BAND_FROM_NAME.at(band_str));
          _score_bands += BAND_FROM_NAME.at(band_str);
        }

        catch (...)
        { }
      }
    }

// SCORE MODES
    if (starts_with(testline, "SCORE MODES"s))
    { if (contains(testline, "CW"s))
        _score_modes += MODE_CW;

      if (contains(testline, "SSB"s) or contains(testline, "PH"s))
        _score_modes += MODE_SSB;
    }

// SCORING
    if (LHS == "SCORING"s)
      _scoring_enabled = is_true;

// SCREEN SNAPSHOT FILE
    if ( (LHS == "SCREEN SNAPSHOT FILE"s) or (LHS == "SCREENSHOT FILE"s) )
      _screen_snapshot_file = rhs;

// SCREEN SNAPSHOT ON EXIT
    if ( (LHS == "SCREEN SNAPSHOT ON EXIT"s) or (LHS == "SCREENSHOT ON EXIT"s) )
      _screen_snapshot_on_exit = is_true;

// SERIAL NUMBER SPACES
    if (LHS == "SERIAL NUMBER SPACES"s)
      _serno_spaces = from_string<decltype(_serno_spaces)>(rhs);

// SHIFT DELTA
    if (LHS == "SHIFT DELTA"s)
    { _shift_delta_cw = from_string<decltype(_shift_delta_cw)>(rhs);
      _shift_delta_ssb = _shift_delta_cw;
    }

// SHIFT DELTA CW
    if (LHS == "SHIFT DELTA CW"s)
      _shift_delta_cw = from_string<decltype(_shift_delta_cw)>(rhs);

// SHIFT DELTA SSB
    if (LHS == "SHIFT DELTA SSB"s)
      _shift_delta_ssb = from_string<decltype(_shift_delta_ssb)>(rhs);

// SHIFT POLL
    if (LHS == "SHIFT POLL"s)
      _shift_poll = from_string<decltype(_shift_poll)>(rhs);

// SHORT SERIAL NUMBER
    if (LHS == "SHORT SERIAL NUMBER"s)
      _short_serno = is_true;

// SOCIETY LIST FILENAME
    if (LHS == "SOCIETY LIST FILENAME"s)
      _society_list_filename = rhs;

// START AUDIO RECORDING
    if (LHS == "START AUDIO RECORDING"s)
    { if (rhs == "auto"s)
        _start_audio_recording = AUDIO_RECORDING::AUTO;

      if (rhs == "false"s)
        _start_audio_recording = AUDIO_RECORDING::DO_NOT_START;

      if (rhs == "true"s)
        _start_audio_recording = AUDIO_RECORDING::START;
    }

// START BAND
    if (LHS == "START BAND"s)
    { const auto cit { BAND_FROM_NAME.find(RHS) };

      if (cit != BAND_FROM_NAME.cend())
        _start_band = cit->second;
    }

// START MODE
    if (LHS == "START MODE"s)
    { if (RHS == "SSB"s)
        _start_mode = MODE_SSB;
    }

// SYNC KEYER
    if (LHS == "SYNC KEYER"s)
      _sync_keyer = is_true;

// TEST
    if (LHS == "TEST"s)
      _test = is_true;

// THOUSANDS SEPARATOR
    if (LHS == "THOUSANDS SEPARATOR"s)
      _thousands_separator = rhs;

// UBA BONUS
    if (LHS == "UBA BONUS"s)
      _uba_bonus = is_true;

// WORKED MULTS COLOUR
    if ( (LHS == "WORKED MULTS COLOUR"s) or (LHS == "WORKED MULTS COLOR"s) )
      _worked_mults_colour = string_to_colour(RHS);

// ---------------------------------------------  MULTIPLIERS  ---------------------------------


// COUNTRY MULTS
// Currently supported: ALL
//                      NONE
// any single continent
    if (LHS == "COUNTRY MULTS"s)
    { _country_mults_filter = RHS;

      if (_country_mults_filter == "NONE"s)
        QSO_DISPLAY_COUNTRY_MULT = false;                  // since countries aren't mults, don't take up space in the log line
    }

// REMAINING CALLSIGN MULTS
    if (LHS == "REMAINING CALLSIGN MULTS"s)
    { _auto_remaining_callsign_mults = (RHS == "AUTO"s);

      if (_auto_remaining_callsign_mults)
      { const vector<string> tokens { split_string(RHS, SPACE_STR) };

        if (tokens.size() == 2)
          _auto_remaining_callsign_mults_threshold = from_string<decltype(_auto_remaining_callsign_mults_threshold)>(tokens[1]);
      }
      else
      { const vector<string> mults { remove_peripheral_spaces(split_string(RHS, ","s)) };

        _remaining_callsign_mults_list = set<string>(mults.cbegin(), mults.cend());
      }
    }

// REMAINING COUNTRY MULTS
    if (LHS == "REMAINING COUNTRY MULTS"s)
    { _auto_remaining_country_mults = contains(RHS, "AUTO"s);

      if (_auto_remaining_country_mults)
      { const vector<string> tokens { split_string(RHS, SPACE_STR) };

        if (tokens.size() == 2)
          _auto_remaining_country_mults_threshold = from_string<decltype(_auto_remaining_callsign_mults_threshold)>(tokens[1]);
      }
      else
      { const vector<string> countries { remove_peripheral_spaces(split_string(RHS, ","s)) };

        _remaining_country_mults_list = set<string>(countries.cbegin(), countries.cend());
      }
    }

// AUTO REMAINING EXCHANGE MULTS (the exchange mults whose list of legal values can be augmented)
    if (LHS == "AUTO REMAINING EXCHANGE MULTS"s)
    { const vector<string> mult_names { remove_peripheral_spaces(split_string(RHS, ","s)) };

      for (const auto& str : mult_names)
        _auto_remaining_exchange_mults += str;
    }

// ---------------------------------------------  CABRILLO  ---------------------------------

// CABRILLO CONTEST
    if (LHS == "CABRILLO CONTEST"s)
      _cabrillo_contest = RHS;          // required to be upper case; don't limit to legal values defined in the "specification", since many contest require an illegal value

    if ( (LHS == "CABRILLO CERTIFICATE"s) and is_legal_value(RHS, "YES,NO"s, ","s) )
      _cabrillo_certificate = RHS;

 // CABRILLO EMAIL (sic)
    if ( (LHS == "CABRILLO E-MAIL"s) or (LHS == "CABRILLO EMAIL"s) )
      _cabrillo_e_mail = rhs;

// CABRILLO EOL
    if (LHS == "CABRILLO EOL"s)
      _cabrillo_eol = RHS;

// CABRILLO INCLUDE SCORE
    if (LHS == "CABRILLO INCLUDE SCORE"s)
      _cabrillo_include_score = is_true;

// CABRILLO LOCATION
    if (LHS == "CABRILLO LOCATION"s)
      _cabrillo_location = rhs;

// CABRILLO NAME
    if (LHS == "CABRILLO NAME"s)
      _cabrillo_name = rhs;

// CABRILLO CATEGORY-ASSISTED
    if ( (LHS == "CABRILLO CATEGORY-ASSISTED"s) and is_legal_value(RHS, "ASSISTED,NON-ASSISTED"s, ","s) )
      _cabrillo_category_assisted = RHS;

// CABRILLO CATEGORY-BAND
    if (LHS == "CABRILLO CATEGORY-BAND"s)
    {
// The spec calls for bizarre capitalization
      if (is_legal_value(rhs, "ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light"s, ","s))
        _cabrillo_category_band = rhs;
    }

// CABRILLO CATEGORY-MODE
    if (LHS == "CABRILLO CATEGORY-MODE"s)
    { if (is_legal_value(RHS, "CW,MIXED,RTTY,SSB"s, ","s))
        _cabrillo_category_mode = RHS;
    }

// CABRILLO CATEGORY-OPERATOR
    if (LHS == "CABRILLO CATEGORY-OPERATOR"s)
    { if (is_legal_value(RHS, "CHECKLOG,MULTI-OP,SINGLE-OP"s, ","s))
        _cabrillo_category_operator = RHS;
    }

// CABRILLO CATEGORY-OVERLAY
    if (LHS == "CABRILLO CATEGORY-OVERLAY"s)
    { if (is_legal_value(RHS, "NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES"s, ","s))
        _cabrillo_category_overlay = RHS;
    }

// CABRILLO CATEGORY-POWER
    if (LHS == "CABRILLO CATEGORY-POWER"s)
    { if (is_legal_value(RHS, "HIGH,LOW,QRP"s, ","s))
        _cabrillo_category_power = RHS;
    }

// CABRILLO CATEGORY-STATION
    if (LHS == "CABRILLO CATEGORY-STATION"s)
    { if (is_legal_value(RHS, "EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL"s, ","s))
        _cabrillo_category_station = RHS;
    }

// CABRILLO CATEGORY-TIME
    if (LHS == "CABRILLO CATEGORY-TIME"s)
    { if (is_legal_value(RHS, "6-HOURS,12-HOURS,24-HOURS"s, ","s))
        _cabrillo_category_station = RHS;
    }

// CABRILLO CATEGORY-TRANSMITTER
    if (LHS == "CABRILLO CATEGORY-TRANSMITTER"s)
    { if (is_legal_value(RHS, "LIMITED,ONE,SWL,TWO,UNLIMITED"s, ","s))
        _cabrillo_category_transmitter = RHS;
    }

// CABRILLO CLUB
    if (LHS == "CABRILLO CLUB"s)
      _cabrillo_club = RHS;

// CABRILLO ADDRESS first line
    if (LHS == "CABRILLO ADDRESS 1"s)
      _cabrillo_address_1 = rhs;

// CABRILLO ADDRESS second line
    if (LHS == "CABRILLO ADDRESS 2"s)
      _cabrillo_address_2 = rhs;

// CABRILLO ADDRESS third line
    if (LHS == "CABRILLO ADDRESS 3"s)
      _cabrillo_address_3 = rhs;

// CABRILLO ADDRESS fourth line
    if (starts_with(testline, "CABRILLO ADDRESS 4"s))
      _cabrillo_address_4 = remove_peripheral_spaces((split_string(line, "="s))[1]);

// CABRILLO ADDRESS-CITY
    if (starts_with(testline, "CABRILLO ADDRESS-CITY"))
      _cabrillo_address_city = remove_peripheral_spaces((split_string(line, "="))[1]);

// CABRILLO ADDRESS-STATE-PROVINCE
    if (starts_with(testline, "CABRILLO ADDRESS-STATE-PROVINCE"s))
      _cabrillo_address_state_province = remove_peripheral_spaces((split_string(line, "="s))[1]);

// CABRILLO ADDRESS-POSTALCODE
    if (starts_with(testline, "CABRILLO ADDRESS-POSTALCODE"s))
      _cabrillo_address_postalcode = remove_peripheral_spaces((split_string(line, "="s))[1]);

// CABRILLO ADDRESS-COUNTRY
    if (starts_with(testline, "CABRILLO ADDRESS-COUNTRY"s))
      _cabrillo_address_country = remove_peripheral_spaces((split_string(line, "="s))[1]);

// CABRILLO OPERATORS
    if (starts_with(testline, "CABRILLO OPERATORS"s))
      _cabrillo_operators = RHS;

/*
                           --------info sent------- -------info rcvd--------
QSO: freq  mo date       time call          rst exch   call          rst exch   t
QSO: ***** ** yyyy-mm-dd nnnn ************* nnn ****** ************* nnn ****** n
QSO:  3799 PH 2000-11-26 0711 N6TW          59  03     JT1Z          59  23     0

000000000111111111122222222223333333333444444444455555555556666666666777777777788
123456789012345678901234567890123456789012345678901234567890123456789012345678901
*/

// cabrillo qso = template: CQ WW
//    map<string, string> cabrillo_qso_templates { { "CQ WW",      "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1" },
//                                                 { "ARRL DX", "ARRL DX" }, // placeholder; mode chosen before we exit this function
//                                                 { "ARRL DX CW", "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-STATE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CWPOWER:74:6:R, TXID:81:1" },
//                                                 { "ARRL DX SSB", "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-STATE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-SSBPOWER:74:6:R, TXID:81:1" }
//    };

    if (LHS == "CABRILLO QSO"s)
    { _cabrillo_qso_template = RHS;

      if (contains(RHS, "TEMPLATE"s))
      { try
        { const string key { remove_peripheral_spaces(split_string(RHS, ":"s))[1] };

          _cabrillo_qso_template = cabrillo_qso_templates.at(key);
        }

        catch (...)
        { ost << "Error in CABRILLO QSO TEMPLATE" << endl;
          exit(-1);
        }
      }
    }

// ---------------------------------------------  WINDOWS  ---------------------------------

    if (LHS == "WINDOW"s)
    { const vector<string> window_info { remove_peripheral_spaces(split_string(split_string(testline, "="s)[1], ","s)) };

      if (window_info.size() >= 5)
      { const string name { window_info[0] };

        window_information winfo; // designated initializers not supported in current g++

        winfo.x(from_string<int>(window_info[1]));
        winfo.y(from_string<int>(window_info[2]));
        winfo.w(from_string<int>(window_info[3]));
        winfo.h(from_string<int>(window_info[4]));

        if (window_info.size() >= 6)
        { winfo.fg_colour(window_info[5]);

          if (window_info.size() >= 7)
            winfo.bg_colour(window_info[6]);

          winfo.colours_set(true);
        }

//        _windows.insert(make_pair(name, winfo));
        _windows += { name, winfo };
      }
    }    // end of WINDOW

// ---------------------------------------------  STATIC WINDOWS  ---------------------------------

    static map<string /* name */, bool /* whether verbatim */> verbatim;

    if (LHS == "STATIC WINDOW"s)
    { const vector<string> fields { remove_peripheral_spaces(split_string(rhs, ","s)) };

      if (fields.size() == 2)  // name, contents
      { const string name { fields[0] };

        string contents { fields[1] };      // might be actual contents, or a fully-qualified filename

        verbatim[name] = contains(fields[1], "\"");     // verbatim if contains quotation mark

        if (file_exists(contents))
          contents = read_file(contents);

        _static_windows[name] = { contents, vector<window_information>() };
      }
    }
//    std::map<std::string /* name */, std::pair<std::string /* contents */, std::vector<window_information> > > _static_windows;

    if (LHS == "STATIC WINDOW INFO"s)  // must come after the corresponding STATIC WINDOW command
    { const vector<string> window_info { remove_peripheral_spaces(split_string(split_string(testline, "="s)[1], ","s)) };

      if (!window_info.empty())
      { const string name { window_info[0] };

        if (_static_windows.count(name) != 0)  // make sure that the window exists
        { if (window_info.size() >= 3)
          { window_information winfo;

            winfo.x(from_string<int>(window_info[1]));
            winfo.y(from_string<int>(window_info[2]));

            if (window_info.size() >= 5)
            { winfo.w(from_string<int>(window_info[3]));
              winfo.h(from_string<int>(window_info[4]));
            }

            string final_contents;

            if (verbatim[name])
            { string contents { _static_windows[name].first };

              if (contents.size() >= 2)
                contents = delimited_substring(contents, '"', '"', DELIMITERS::DROP);

              vector<string> lines { to_lines(contents) };

              const string contents_1 { replace(contents, "\\n"s, EOL) };

              lines = to_lines(contents_1);

              winfo.w(longest_line(lines).length());
              winfo.h(lines.size());

              if (window_info.size() >= 4)
              { winfo.fg_colour(window_info[3]);

                if (window_info.size() >= 5)
                  winfo.bg_colour(window_info[4]);

                winfo.colours_set(true);
              }

              final_contents = contents_1;
            }                                                       // end verbatim
            else                                                    // read from file
            { const string         contents { _static_windows[name].first };
              const vector<string> lines    { to_lines(contents) };

              winfo.w(longest_line(lines).length());
              winfo.h(lines.size());

              if (window_info.size() >= 4)
              { winfo.fg_colour(window_info[3]);

                if (window_info.size() >= 5)
                  winfo.bg_colour(window_info[4]);

                winfo.colours_set(true);
              }

              final_contents = contents;
            }

            vector<window_information> vec { _static_windows[name].second };

            vec += winfo;
            _static_windows[name] = { final_contents, vec };
          }
        }
      }
    }

// ---------------------------------------------  MESSAGES  ---------------------------------

    if (starts_with(testline, "MESSAGE KEY"s))
    { vector<string> message_info { split_string(testline, SPACE_STR) };

      if (message_info.size() >= 5 and contains(testline, "="s))
      {
// a big switch to convert from text in the configuration file to a KeySym, which is what we use as the key in the message map
// this could be more efficient by generating a container of all the legal keysym names and then comparing the target to all those.
// The latter is now what we do. The legal names are in key_names.
        const string target { to_lower(message_info[2]) };

        if (const auto& cit { key_names.find(target) }; cit != key_names.cend())
        { const vector<string> vec_str { split_string(testline, "="s) };
          const string         str     { remove_leading_spaces(vec_str.at(1)) };

// everything to the right of the = -- we assume there's only one -- goes into the message, excepting any leading space
//          _messages.insert( { cit->second, str } );
          _messages += { cit->second, str };

          ost << "message associated with " << target << ", which is keysym " << hex << cit->second << dec << ", is: " << str << endl;

          const map<string, string>::const_iterator cit2 { equivalent_key_names.find(target) };

          if (cit2 != equivalent_key_names.cend())
          { ost << "found equivalent key name: " << cit2->second << endl;

            const string alternative { cit2->second };

            if (const auto& cit { key_names.find(alternative) }; cit != key_names.cend())
            { const int& keysym { cit->second };

 //             if (_messages.find(keysym) == _messages.cend())  // only if there is no message for this key
              if (!contains(_messages, keysym))  // only if there is no message for this key
              {  ost << "message associated with equivalent key is: " << str << endl;

 //               _messages.insert( { keysym, str } );
                _messages += { keysym, str };
              }
            }
          }
        }
      }
    }

    if (LHS == "MESSAGE CQ 1"s)
    { const vector<string> tokens { split_string(testline, "="s) };

      if (tokens.size() != 2)
        print_error_and_exit(testline);

      _message_cq_1 = tokens[1];
    }

    if (LHS == "MESSAGE CQ 2"s)
    { const vector<string> tokens { split_string(testline, "="s) };

      if (tokens.size() == 2)
        _message_cq_2 = tokens[1];
    }
  }

// now that we've processed the file, set some new defaults if we haven't explicitly set them
  if (_cabrillo_contest == string())
    _cabrillo_contest = _contest_name;

  if (_cabrillo_callsign == string())
    _cabrillo_callsign = _my_call;

  if (_qsl_message.empty())
    _qsl_message = "tu "s + _my_call + " test"s;

  if (_alternative_qsl_message.empty())
    _alternative_qsl_message = "tu "s + _my_call;

  if (_message_cq_1.empty())
    _message_cq_1 = "test "s + _my_call + SPACE_STR + _my_call + " test"s;

  if (_message_cq_2.empty())
    _message_cq_2 = "cq cq test de  "s + _my_call + "  "s + _my_call + "  "s + _my_call + "  test"s;

  if (_call_history_bands.empty())
  { const vector<string> bands_vec { remove_peripheral_spaces( split_string(bands(), ","s) ) };

    for (const auto& str : bands_vec)
    { try
      { _call_history_bands += BAND_FROM_NAME.at(str);
      }

      catch (...)
      { }
    }
  }

// possibly fix Cabrillo template
  if ( (_cabrillo_qso_template == "ARRL DX"s) or (_cabrillo_qso_template == "CQ WW"s) )
  { const vector<string> actual_modes { remove_peripheral_spaces(split_string(_modes, ","s)) };

    if (actual_modes.size() == 1)
    { try
      { if (set<string>( { "ARRL DX"s, "CQ WW"s, "JIDX"s} ) > _cabrillo_qso_template)
        {  const string key { _cabrillo_qso_template + ( (actual_modes[0] == "CW"s) ?  " CW"s : " SSB"s) };

          _cabrillo_qso_template = cabrillo_qso_templates.at(key);
        }
      }

      catch (...)
      { ost << "Error in revised CABRILLO QSO TEMPLATE" << endl;
        exit(-1);
      }
    }
    else
    { ost << "Error in CABRILLO QSO TEMPLATE: ARRL DX" << endl;
      exit(-1);
    }
  }
}

/// construct from file
drlog_context::drlog_context(const std::string& filename)
{ for (unsigned int n = 0; n < NUMBER_OF_BANDS; ++n)
    _per_band_country_mult_factor += { static_cast<BAND>(n), 1 };

  _process_configuration_file(filename);

// make sure that the default is to score all permitted bands
  if (_score_bands.empty())
  { vector<string> bands_str { remove_peripheral_spaces(split_string(_bands, ","s)) };

    for (const auto& band_name : bands_str)
    { try
      { //const BAND b { BAND_FROM_NAME.at(band_name) };

        _score_bands += BAND_FROM_NAME.at(band_name);
      }

      catch (...)
      { }
    }
  }

// default is to score all permitted modes
  if (_score_modes.empty())
  { if (contains(_modes, "CW"s))
      _score_modes += MODE_CW;

    if (contains(_modes, "SSB"s))
      _score_modes += MODE_SSB;
  }
}

/*! \brief          Get all the windows whose name contains a particular substring
    \param  substr  substring for which to search
    \return         all the window names that include <i>substr</i>
*/
vector<string> drlog_context::window_name_contains(const string& substr) const
{ vector<string> rv;

  for (auto cit = _windows.cbegin(); cit != _windows.cend(); ++cit)
    if (contains(cit->first, substr))
//      rv.push_back(cit->first);
      rv += (cit->first);

  return rv;
}

/*! \brief      Is a particular frequency within any marked range?
    \param  m   mode
    \param  f   frequency to test
    \return     whether <i>f</i> is in any marked range for the mode <i>m</i>
*/
bool drlog_context::mark_frequency(const MODE m, const frequency& f)
{ SAFELOCK(_context);

  try
  { const vector<pair<frequency, frequency>>& vec { _mark_frequencies.at(m) };

    for (const auto& pff : vec)
    { if ( (f >= pff.first) and (f <= pff.second))
        return true;
    }
  }

  catch (...)
  { return false;
  }

  return false;
}

/*! \brief      Get the points string for a particular band and mode
    \param  b   band
    \param  m   mode
    \return     the points string corresponding to band <i>b</i> and mode <i>m</i>
*/
string drlog_context::points_string(const BAND b, const MODE m) const
{ SAFELOCK(_context);

  const auto& pbb { _per_band_points[m] };

  return ( pbb.find(b) != pbb.cend() ? pbb.at(b) : string() );
}

#if 0
/*! \brief                  Get the points string for a particular band and mode, if a particular exchange field is present
    \param  exchange_field  exchange field
    \param  b               band
    \param  m               mode
    \return                 the points string corresponding to band <i>b</i> and mode <i>m</i> when exchange field <i>exchange_field</i> is present
*/

const string drlog_context::points(const std::string& exchange_field, const BAND b, const MODE m) const
{ SAFELOCK(_context);

  const auto cit = _per_band_points_with_exchange_field.find(exchange_field);

  if (cit == _per_band_points_with_exchange_field.cend())    // if this exchange field has no points entry
     return string();

  const auto& points_per_band_per_mode = cit->second;
  const auto& pbb = points_per_band_per_mode[m];

  return ( pbb.find(b) != pbb.cend() ? pbb.at(b) : string() );
}
#endif

/*! \brief      Get all the names in the sent exchange
    \return     the names of all the fields in the sent exchange
*/
vector<string> drlog_context::sent_exchange_names(void) const
{ vector<string> rv;

  for (const auto& [name, value] : _sent_exchange)
    rv += name;

  return rv;
}

/*! \brief      Get all the field names in the exchange sent for a particular mode
    \param  m   target mode
    \return     the names of all the fields in the sent exchange for mode <i>m</i>
*/
vector<string> drlog_context::sent_exchange_names(const MODE m) const
{ vector<string> rv;

  const vector<pair<string, string> >* ptr_vec_pss { (m == MODE_CW ? &_sent_exchange_cw : &_sent_exchange_ssb) };

  for (const auto& pss : *ptr_vec_pss)
//    rv.push_back(pss.first);
    rv += pss.first;

  return rv;
}

/*! \brief      Get names and values of sent exchange fields for a particular mode
    \param  m   target mode
    \return     the names and values of all the fields in the sent exchange when the mode is <i>m</i>
*/
decltype(drlog_context::_sent_exchange) drlog_context::sent_exchange(const MODE m)        // doxygen complains about the decltype; I have no idea why
{ SAFELOCK(_context);

  decltype(_sent_exchange) rv { ( (m == MODE_CW) ? _sent_exchange_cw : _sent_exchange_ssb) };

  if (rv.empty())
  { rv = _sent_exchange;

// fix RST/RS if using non-mode-specific exchange
    for (unsigned int n = 0; n < rv.size(); ++n)
    { pair<string, string>& pss { rv[n] };

      if ( (m == MODE_SSB) and (pss.first == "RST"s) )
        pss = { "RS"s, "59"s };       // hardwire report

      if ( (m == MODE_CW) and (pss.first == "RS"s) )
        pss = { "RST"s, "599"s };     // hardwire report
    }
  }

  return rv;
}
