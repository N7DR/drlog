// $Id: drlog_context.cpp 272 2025-07-13 22:28:31Z  $

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

#include <algorithm>
#include <fstream>
#include <iostream>

using namespace std;

extern message_stream   ost;                        ///< for debugging, info
extern bool             QSO_DISPLAY_COUNTRY_MULT;   ///< controls whether country mults are written on the log line
extern int              QSO_MULT_WIDTH;             ///< controls width of zone mults field in log line

/// example: cabrillo qso = template: CQ WW

// [contest, cabrillo template]
static const STRING_MAP<string> cabrillo_qso_templates { { "ARRL DX"s, "ARRL DX"s }, // placeholder; mode chosen before we exit this function
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
void drlog_context::_set_points(const string_view command, const MODE m)
{ if (command.empty())
    return;

  const vector<string_view> str_vec { split_string <std::string_view> (command, '=') };

  if (str_vec.size() != 2)
  { ost << "Invalid command in _set_points(): " << command << endl;
    ost << "size = " << str_vec.size() << endl;

    exit(0);
  }

  const string RHS { to_upper(remove_peripheral_spaces <std::string_view> (str_vec[1])) };

  if (!str_vec.empty())
  { const string lhs { str_vec[0] };

    if (auto& pbb { _per_band_points[m] }; !contains(lhs, '[') or contains(lhs, "[*]"s))            // for all bands
    { for (unsigned int n { 0 }; n < NUMBER_OF_BANDS; ++n)
        pbb += { static_cast<BAND>(n), RHS };
    }
    else                                                        // not all bands
    { const size_t left_bracket_posn  { lhs.find('[') };
      const size_t right_bracket_posn { lhs.find(']') };
      const bool   valid              { (left_bracket_posn != string::npos) and (right_bracket_posn != string::npos) and (left_bracket_posn < right_bracket_posn) };

      if (valid)
      { const string_view bands_str { delimited_substring <std::string_view> (lhs, '[', ']', DELIMITERS::DROP) };

        FOR_ALL(clean_split_string <std::string> (bands_str), [&pbb, &RHS] (const auto& b_str) { pbb += { BAND_FROM_NAME[b_str ], RHS }; } );   // keep string
      }
    }
  }
}

/*! \brief              Process a configuration file
    \param  filename    name of file to process

    This routine may be called recursively (by the RULES statement in the processed file)
*/
void drlog_context::_process_configuration_file(const string_view filename)
{ string entire_file;

  try
  { entire_file = remove_char(read_file(filename), CR_CHAR);    // CR characters are a menace
  }

  catch (...)
  { ost << "Unable to open configuration file " << filename << endl;
    exit(-1);
  }

  const vector<string_view> lines { to_lines <std::string_view> (entire_file) };   // split into lines

  for (const auto tmpline : lines)                                            // process each line; string_view is cheap to copy
  { const string_view line { remove_trailing_comment <std::string_view> (tmpline) };    // remove any comment

// generate a number of useful variables
    const string              testline { remove_leading_spaces <std::string> (to_upper(line)) };
    const vector<string_view> fields   { split_string <std::string_view> (line, '=') };
    const string              rhs      { ((fields.size() > 1) ? remove_peripheral_spaces <std::string> (fields[1]) : string { }) };    // the stuff to the right of the "="
    const string              RHS      { to_upper(rhs) };                                                                              // converted to upper case
    const bool                is_true  { (RHS == "TRUE"sv) };                                                                          // is right hand side == "TRUE"?
    const string              lhs      { squash( (fields.empty()) ? string { } : remove_peripheral_spaces <std::string> (fields[0]) ) };              // the stuff to the left of the "="
    const string              LHS      { to_upper(lhs) };                                                                              // converted to upper case

// ACCEPT COLOUR
    if ( ( (LHS == "ACCEPT COLOUR"sv) or (LHS == "ACCEPT COLOR"sv) ) and !rhs.empty() )
      _accept_colour = string_to_colour(RHS);

// ALLOW AUDIO RECORDING
    if (LHS == "ALLOW AUDIO RECORDING"sv)
      _allow_audio_recording = is_true;

// ALLOW CTRL-A
    if (LHS == "ALLOW CTRL-A"sv)
      _allow_ctrl_a = is_true;

// ALTERNATIVE EXCHANGE CQ
    if (LHS == "ALTERNATIVE EXCHANGE CQ"sv)
      _alternative_exchange_cq = RHS;

// ALTERNATIVE EXCHANGE SAP
    if (LHS == "ALTERNATIVE EXCHANGE SAP"sv)
      _alternative_exchange_sap = RHS;

// ALTERNATIVE QSL MESSAGE
    if ( (LHS == "ALTERNATIVE QSL MESSAGE"sv) or (LHS == "QUICK QSL MESSAGE"sv) )
      _alternative_qsl_message = RHS;

// ARCHIVE
    if ( (LHS == "ARCHIVE"sv) and !rhs.empty() )
      _archive_name = rhs;

// AUDIO CHANNELS
    if (LHS == "AUDIO CHANNELS"sv)
      _audio_channels = from_string<decltype(_audio_channels)>(rhs);

// AUDIO DEVICE
    if ( (LHS == "AUDIO DEVICE"sv) or (LHS == "AUDIO DEVICE NAME"sv) )
      _audio_device_name = rhs;

// AUDIO DURATION
    if (LHS == "AUDIO DURATION"sv)
      _audio_duration = from_string<decltype(_audio_duration)>(rhs);

// AUDIO FILE
    if (LHS == "AUDIO FILE"sv)
      _audio_file = rhs;

// AUDIO RATE
    if (LHS == "AUDIO RATE"sv)
      _audio_rate = from_string<decltype(_audio_rate)>(rhs);

// AUTO BACKUP
    if ( ((LHS == "AUTO BACKUP DIRECTORY"sv) or (LHS == "AUTO BACKUP"sv)) and !rhs.empty() )  // AUTO BACKUP was the old name for this command
      _auto_backup_directory = rhs;

// AUTOCORRECT
    if ( (LHS == "AUTOCORRECT"sv) or (LHS == "AUTO CORRECT"sv) or (LHS == "AUTOCORRECT RBN"sv) )
      _autocorrect_rbn = is_true;

// AUTO CQ MODE SSB
    if (LHS == "AUTO CQ MODE SSB"sv)
      _auto_cq_mode_ssb = is_true;

// AUTO SCREENSHOT
    if ( (LHS == "AUTO SCREENSHOT"sv) and !rhs.empty() )
      _auto_screenshot = is_true;

// BAND MAP CULL FUNCTION
    if ( ( (LHS == "BAND MAP CULL FUNCTION"sv) or (LHS == "BANDMAP CULL FUNCTION"sv) ) and !rhs.empty() )
      _bandmap_cull_function = from_string<decltype(_bandmap_cull_function)>(rhs);

// BAND MAP DECAY TIME CLUSTER
    if ( ( (LHS == "BAND MAP DECAY TIME CLUSTER"sv) or (LHS == "BANDMAP DECAY TIME CLUSTER"sv) ) and !rhs.empty() )
      _bandmap_decay_time_cluster = from_string<decltype(_bandmap_decay_time_cluster)>(rhs);

// BAND MAP DECAY TIME LOCAL
    if ( ( (LHS == "BAND MAP DECAY TIME LOCAL"sv) or (LHS == "BANDMAP DECAY TIME LOCAL"sv) ) and !rhs.empty() )
      _bandmap_decay_time_local = from_string<decltype(_bandmap_decay_time_local)>(rhs);

// BAND MAP DECAY TIME RBN
    if ( (LHS == "BAND MAP DECAY TIME RBN"sv) or (LHS == "BANDMAP DECAY TIME RBN"sv) )
      _bandmap_decay_time_rbn = from_string<decltype(_bandmap_decay_time_rbn)>(rhs);

// BAND MAP FADE COLOURS
    if ( (LHS == "BAND MAP FADE COLOURS"sv) or (LHS == "BANDMAP FADE COLOURS"sv) or
         (LHS == "BAND MAP FADE COLORS"sv) or (LHS == "BANDMAP FADE COLORS"sv) )
    { _bandmap_fade_colours.clear();

      if (!RHS.empty())
        FOR_ALL(clean_split_string <string> (RHS), [this] (const string& name) { _bandmap_fade_colours += string_to_colour(name); } );
    }

// BAND MAP FILTER
    if ( (LHS == "BAND MAP FILTER"sv) or (LHS == "BANDMAP FILTER"sv) )
    { if (!RHS.empty())
      { vector<string> filters { clean_split_string <string> (RHS) };

        SORT(filters, compare_calls);    // put the entries into callsign order
        _bandmap_filter = filters;
      }
    }

// BAND MAP FILTER COLOURS
    if ( (LHS == "BAND MAP FILTER COLOURS"sv) or (LHS == "BAND MAP FILTER COLORS"sv) or
         (LHS == "BANDMAP FILTER COLOURS"sv) or (LHS == "BANDMAP FILTER COLORS"sv) )
    { if (!RHS.empty())
      { const vector<string> colours { clean_split_string <string> (RHS) };

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
    if ( (LHS == "BAND MAP FILTER ENABLE"sv) or (LHS == "BANDMAP FILTER ENABLE"sv) )
      _bandmap_filter_enabled = is_true;

// BAND MAP FILTER MODE
    if ( (LHS == "BAND MAP FILTER MODE"sv) or (LHS == "BANDMAP FILTER MODE"sv) )
      _bandmap_filter_show = (RHS == "SHOW"sv);

// BAND MAP FREQUENCY UP
    if ( (LHS == "BAND MAP FREQUENCY UP"sv) or (LHS == "BANDMAP FREQUENCY UP"sv) )
      _bandmap_frequency_up = is_true;

// BAND MAP GUARD BAND CW
// rhs is in Hz
    if ( (LHS == "BAND MAP GUARD BAND CW"sv) or (LHS == "BANDMAP GUARD BAND CW"sv) )
//      _guard_band[MODE_CW] = from_string<decltype(_guard_band)::mapped_type>(rhs);
      _guard_band[MODE_CW] = frequency { from_string<double>(rhs), FREQUENCY_UNIT::HZ };

// BAND MAP GUARD BAND SSB
// rhs is in Hz
    if ( (LHS == "BAND MAP GUARD BAND SSB"sv) or (LHS == "BANDMAP GUARD BAND SSB"sv) )
//      _guard_band[MODE_SSB] = from_string<decltype(_guard_band)::mapped_type>(rhs);
      _guard_band[MODE_SSB] = frequency { from_string<double>(rhs), FREQUENCY_UNIT::HZ };

// BAND MAP RECENT COLOUR
    if ( (LHS == "BAND MAP RECENT COLOUR"sv) or (LHS == "BANDMAP RECENT COLOUR"sv) or
         (LHS == "BANDMAP RECENT COLOR"sv) or (LHS == "BANDMAP RECENT COLOR"sv) )
    { if (!RHS.empty())
        _bandmap_recent_colour = string_to_colour(remove_peripheral_spaces <std::string_view> (RHS));
    }

// BAND MAP SHOW MARKED FREQUENCIES
    if ( (LHS == "BAND MAP SHOW MARKED FREQUENCIES"sv) or (LHS == "BANDMAP SHOW MARKED FREQUENCIES"sv) )
      _bandmap_show_marked_frequencies = is_true;

// BANDS
    if (LHS == "BANDS"sv)
      _bands = RHS;

// BATCH MESSAGES FILE
    if (LHS == "BATCH MESSAGES FILE"sv)
      _batch_messages_file = rhs;

// BEST DX UNIT
    if ( (LHS == "BEST DX UNIT"sv) or (LHS == "BEST DX UNITS"sv) )
      _best_dx_unit = RHS;

// CABRILLO FILENAME
    if (LHS == "CABRILLO FILENAME"sv)
      _cabrillo_filename = rhs;

// CALL HISTORY BANDS
    if (LHS == "CALL HISTORY BANDS"sv)
      FOR_ALL(clean_split_string <string> (rhs), [this] (const string& band_str) { _call_history_bands += BAND_FROM_NAME[band_str]; });
//      FOR_ALL(clean_split_string <string_view> (rhs), [this] (const string_view band_str) { _call_history_bands += BAND_FROM_NAME[band_str]; }); // [] not yet supported

// CALL OK NOW MESSAGE
    if (LHS == "CALL OK NOW MESSAGE"sv)
      _call_ok_now_message = rhs;

// CALLSIGN MULTS
    if (LHS == "CALLSIGN MULTS"sv)
      ranges::move(clean_split_string <string> (RHS), inserter(_callsign_mults, _callsign_mults.begin()));

// CALLSIGN MULTS PER BAND
    if (LHS == "CALLSIGN MULTS PER BAND"sv)
      _callsign_mults_per_band = is_true;

// CALLSIGN MULTS PER MODE
    if (LHS == "CALLSIGN MULTS PER MODE"sv)
      _callsign_mults_per_mode = is_true;

// CLUSTER CW
    if (LHS == "CLUSTER CW"sv)
      _cluster_cw = is_true;

// CLUSTER PORT
    if (LHS == "CLUSTER PORT"sv)
      _cluster_port = from_string<decltype(_cluster_port)>(rhs);

// CLUSTER SERVER
    if (LHS == "CLUSTER SERVER"sv)
      _cluster_server = rhs;

// CLUSTER THRESHOLD
    if (LHS == "CLUSTER THRESHOLD"sv)
      _cluster_threshold = from_string<decltype(_cluster_threshold)>(rhs);

// CLUSTER TIMEOUT
    if (LHS == "CLUSTER TIMEOUT"sv)
      _cluster_timeout = static_cast<decltype(_cluster_timeout)>(from_string<unsigned int>(rhs));

// CLUSTER USERNAME
    if (LHS == "CLUSTER USERNAME"sv)
      _cluster_username = rhs;

// CONTEST
    if (LHS == "CONTEST"sv)
      _contest_name = RHS;

// COUNTRY FILENAME
    if (LHS == "COUNTRY FILENAME"sv)
      _cty_filename = rhs;

// COUNTRY LIST
    if ( LHS == "COUNTRY LIST"sv)
    { if (RHS == "DXCC"sv)
        _country_list = COUNTRY_LIST::DXCC;

      if ( (RHS == "WAEDC"sv) or (RHS == "WAE"sv) )
        _country_list = COUNTRY_LIST::WAEDC;
    }

// COUNTRY MULT FACTOR
    if (LHS == "COUNTRY MULT FACTOR"sv)  // there may be an "=" in the points definitions
    { const vector<string> str_vec { split_string <std::string> (line, '=') };

      if (!str_vec.empty())
      { string tmp_str;

        const string lhs { str_vec[0] };

        if (!contains(lhs, '[') or contains(lhs, "[*]"s))             // for all bands
        { string new_str;

          for (unsigned int n { 1 }; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
          { new_str += str_vec[n];

            if (n != str_vec.size() - 1)
              new_str += '=';
          }

          tmp_str = to_upper(remove_peripheral_spaces <std::string> (new_str));

          for (unsigned int n { 0 }; n < NUMBER_OF_BANDS; ++n)
            _per_band_country_mult_factor += { static_cast<BAND>(n), from_string<int>(tmp_str) };
        }
        else    // not all bands
        { const string bands_str { delimited_substring <std::string> (lhs, '[', ']', DELIMITERS::DROP) };

          for (const auto& b_str: clean_split_string <string> (bands_str))
          { string new_str;

            for (unsigned int n { 1 }; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
            { new_str += str_vec[n];

              if (n != str_vec.size() - 1)
                new_str += '=';
            }

            tmp_str = to_upper(remove_peripheral_spaces <std::string> (new_str));
            _per_band_country_mult_factor += { BAND_FROM_NAME[b_str], from_string<decltype(_per_band_country_mult_factor)::mapped_type>(tmp_str) };
          }
        }
      }
    }

// COUNTRY MULTS PER BAND
    if (LHS == "COUNTRY MULTS PER BAND"sv)
      _country_mults_per_band = is_true;

// COUNTRY MULTS PER MODE
    if (LHS == "COUNTRY MULTS PER MODE"sv)
      _country_mults_per_mode = is_true;

// CQ AUTO LOCK
    if (LHS == "CQ AUTO LOCK"sv)
      _cq_auto_lock = is_true;

// CQ AUTO RIT
    if (LHS == "CQ AUTO RIT"sv)
      _cq_auto_rit = is_true;

// CW BANDWIDTH
    if (LHS == "CW BANDWIDTH"sv)
    { const vector<string_view> bw { clean_split_string <string_view> (RHS, '/') };

      if (bw.size() == 2)
      { _cw_bandwidth_narrow = from_string<decltype(_cw_bandwidth_narrow)>(bw[0]);
        _cw_bandwidth_wide = from_string<decltype(_cw_bandwidth_wide)>(bw[1]);
      }
    }

// CW PRIORITY
    if (LHS == "CW PRIORITY"sv)
      _cw_priority = from_string<decltype(_cw_priority)>(RHS);

// CW SPEED
    if (LHS == "CW SPEED"sv)
      _cw_speed = from_string<decltype(_cw_speed)>(RHS);

// CW SPEED CHANGE
    if (LHS == "CW SPEED CHANGE"sv)
      _cw_speed_change = from_string<decltype(_cw_speed_change)>(RHS);

// DECIMAL POINT
    if (LHS == "DECIMAL POINT"sv)
    { if (rhs.size() >= 1)
        _decimal_point = rhs[0];
    }

// DISPLAY COMMUNICATION ERRORS
    if (LHS == "DISPLAY COMMUNICATION ERRORS"sv)
      _display_communication_errors = is_true;

// DISPLAY GRID
    if (LHS == "DISPLAY GRID"sv)
      _display_grid = is_true;

// DO NOT SHOW
    if (LHS == "DO NOT SHOW"sv)
      _do_not_show = clean_split_string <string> (RHS);

// DO NOT SHOW FILE
    if (LHS == "DO NOT SHOW FILE"sv)
      _do_not_show_filename = rhs;

// DRMASTER FILENAME
    if (LHS == "DRMASTER FILENAME"sv)
      _drmaster_filename = rhs;

// DX SPOTTING TEXT
    if (LHS == "DX SPOTTING TEXT"sv)
      _dx_spotting_text = RHS;

// DYNAMIC AUTOCORRECT RBN
    if (LHS == "DYNAMIC AUTOCORRECT RBN"sv)
      _dynamic_autocorrect_rbn = is_true;

// EXCHANGE
    if (LHS == "EXCHANGE"sv)
      _exchange = RHS;

// EXCHANGE[
    if (testline.starts_with("EXCHANGE["sv))
    { const string_view country_list { delimited_substring <std::string_view> (LHS, '[', ']', DELIMITERS::DROP) };

      FOR_ALL(clean_split_string <string_view> (country_list), [RHS, this] (const string_view str) { _exchange_per_country += { str, RHS }; } );
    }

// EXCHANGE CQ
    if (LHS == "EXCHANGE CQ"sv)
      _exchange_cq = RHS;

// EXCHANGE FIELDS FILENAME (for all the regex-based exchange fields)
    if (LHS == "EXCHANGE FIELDS FILENAME"sv)
      _exchange_fields_filename = rhs;

// EXCHANGE MULTS
    if (LHS == "EXCHANGE MULTS"sv)
    { _exchange_mults = RHS;

      if (contains(_exchange_mults, ','))      // if there is more than one exchange mult
        QSO_MULT_WIDTH = 4;                     // make them all the same width, so that the log line looks OK
    }

// EXCHANGE MULTS PER BAND
    if (LHS == "EXCHANGE MULTS PER BAND"sv)
      _exchange_mults_per_band = is_true;

// EXCHANGE MULTS PER MODE
    if (LHS == "EXCHANGE MULTS PER MODE"sv)
      _exchange_mults_per_mode = is_true;

// EXCHANGE PREFILL FILE
//   exchange prefill file = [ exchange-field-name, filename ]
    if ( (LHS == "EXCHANGE PREFILL FILE"sv) or (LHS == "EXCHANGE PREFILL FILES"sv) )
    { for (const auto& file : vector<string> { remove_peripheral_spaces <std::string> (delimited_substrings <std::string> (rhs, '[', ']', DELIMITERS::DROP)) })
      { if (const vector<string> fields { clean_split_string <std::string> (file) }; fields.size() == 2)
          _exchange_prefill_files[to_upper(fields[0])] = fields[1];
      }
    }

// EXCHANGE SAP
    if (LHS == "EXCHANGE SAP"sv)
      _exchange_sap = RHS;

    auto update_sent_exchange = [line, testline] (auto& exchange)
      { const string              comma_delimited_list { to_upper(clean_split_string <std::string_view> (line, '=')[1]) };    // RST:599, CQZONE:4
        const vector<string_view> fields               { split_string <std::string_view> (comma_delimited_list) };

        for (const auto& this_field : fields)
        { const vector<string_view> field { clean_split_string <string_view> (this_field, ':') };

          if (field.size() != 2)
            print_error_and_exit(testline);

          exchange += { string { field[0] }, string { field[1] } };
        }
      };

// EXCHANGE SENT
// e.g., exchange sent = RST:599, CQZONE:4
    if (LHS == "EXCHANGE SENT"sv)
      update_sent_exchange(_sent_exchange);

// EXCHANGE SENT CW
    if (LHS == "EXCHANGE SENT CW"sv)
      update_sent_exchange(_sent_exchange_cw);

// EXCHANGE SENT SSB
    if (LHS == "EXCHANGE SENT SSB"sv)
      update_sent_exchange(_sent_exchange_ssb);

// EXECUTE AT START
    if (LHS == "EXECUTE AT START"sv)
      _execute_at_start = remove_peripheral_spaces <std::string> (rhs);

// FAST CQ BANDWIDTH; used only in CW mode
    if (LHS == "FAST CQ BANDWIDTH"sv)
      _fast_cq_bandwidth = from_string<decltype(_fast_cq_bandwidth)>(RHS);

// FAST SAP BANDWIDTH; used only in CW mode
    if (LHS == "FAST SAP BANDWIDTH"sv)
      _fast_sap_bandwidth = from_string<decltype(_fast_sap_bandwidth)>(RHS);

// GEOMAGNETIC INDICES COMMAND
    if (LHS == "GEOMAGNETIC INDICES COMMAND"sv)
      _geomagnetic_indices_command = rhs;

// HOME EXCHANGE WINDOW
    if (LHS == "HOME EXCHANGE WINDOW"sv)
      _home_exchange_window = is_true;

// INACTIVTY TIME
    if ( (LHS == "INACTIVITY TIMER"sv) or (LHS == "INACTIVITY TIME"sv) )
      _inactivity_time = from_string<decltype(_inactivity_time)>(rhs);

// INDIVIDUAL MESSAGES FILE
    if (LHS == "INDIVIDUAL MESSAGES FILE"sv)
      _individual_messages_file = rhs;

// INSTRUMENT, INSTRUMENTED
    if ( (LHS == "INSTRUMENT"sv) or (LHS == "INSTRUMENTED"sv) )
      _instrumented = is_true;

// KEYER PORT
    if (LHS == "KEYER PORT"sv)
      _keyer_port = rhs;

// LOG
    if ( (LHS == "LOG"sv) and !rhs.empty() )
      _logfile = rhs;

// LONG T
    if (LHS == "LONG T"sv)
      _long_t = from_string<decltype(_long_t)>(rhs);

// MARK FREQUENCIES [CW|SSB]
    if (testline.starts_with("MARK FREQUENCIES"sv) and !rhs.empty())
    { vector<pair<frequency, frequency>> frequencies;

      for (auto range : clean_split_string <string_view> (rhs))
      { const vector<string_view> bounds { clean_split_string <string_view> (range, '-') };

        try
        { frequencies += { frequency(bounds.at(0)), frequency(bounds.at(1))};
        }

        catch (...)
        { print_error_and_exit(testline);
        }
      }

      if (LHS == "MARK FREQUENCIES"sv or LHS == "MARK FREQUENCIES CW"sv)
        _mark_frequencies += { MODE_CW, frequencies };

      if (LHS == "MARK FREQUENCIES"sv or LHS == "MARK FREQUENCIES SSB"sv)
        _mark_frequencies += { MODE_SSB, frequencies };
    }

// MATCH MINIMUM
    if (LHS == "MATCH MINIMUM"sv)
      _match_minimum = from_string<decltype(_match_minimum)>(RHS);

// MATCH MINIMUM
    if (LHS == "MAX QSOS WITHOUT QSL"sv)
      _max_qsos_without_qsl = from_string<decltype(_max_qsos_without_qsl)>(RHS);

// MM COUNTRY MULTS
    if (LHS == "MM COUNTRY MULTS"sv)
      _mm_country_mults = is_true;

// MODES
    if (LHS == "MODES"sv)
    { _modes = RHS;

      if (contains(_modes, ','))        // if more than one mode
        _mark_mode_break_points = true;
      else
      { if (_modes == "SSB"sv)
          _start_mode = MODE_SSB;
      }
    }

// MODE BREAK POINTS
    if (LHS == "MODE BREAK POINTS"sv)
    { for (const string_view break_point : clean_split_string <string_view> (RHS))
      { const frequency f { break_point };

        _mode_break_points += { to_BAND(f), f };
      }
    }

// MY CALL
    if (LHS == "MY CALL"sv)
      _my_call = RHS;

// MY CONTINENT
    if (LHS == "MY CONTINENT"sv)
      _my_continent = RHS;

// MY CQ ZONE
    if (LHS == "MY CQ ZONE"sv)
      _my_cq_zone = from_string<decltype(_my_cq_zone)>(RHS);

// MY GRID
    if (LHS == "MY GRID"sv)
      _my_grid = rhs;

// MY IP
    if (LHS == "MY IP"sv)
      _my_ip = rhs;

// MY LATITUDE
    if (LHS == "MY LATITUDE"sv)
      _my_latitude = from_string<decltype(_my_latitude)>(rhs);

// MY LATITUDE
    if (LHS == "MY LONGITUDE"sv)
      _my_longitude = from_string<decltype(_my_longitude)>(rhs);

// MY ITU ZONE
    if (LHS == "MY ITU ZONE"sv)
      _my_itu_zone = from_string<decltype(_my_itu_zone)>(RHS);

// N MEMORIES
    if (LHS == "N MEMORIES"sv)
      _n_memories = min(from_string<decltype(_n_memories)>(rhs), static_cast<decltype(_n_memories)>(10));  // maximum number of memories is 10

// NEARBY EXTRACT
    if (LHS == "NEARBY EXTRACT"sv)
      _nearby_extract = is_true;

// NO DEFAULT RST
    if (LHS == "NO DEFAULT RST"sv)
      _no_default_rst = is_true;

// NORMALISE RATE(S)
    if ( (LHS == "NORMALISE RATE"sv) or (LHS == "NORMALIZE RATE"sv) or (LHS == "NORMALISE RATES"sv) or (LHS == "NORMALIZE RATES"sv) )
      _normalise_rate = is_true;

// NOT COUNTRY MULTS
    if ( (LHS == "NOT COUNTRY MULTS"sv) and (!rhs.empty()) )
      _not_country_mults = rhs;

// OLD ADIF LOG NAME
    if (LHS == "OLD ADIF LOG NAME"sv)
      _old_adif_log_name = rhs;

// OLD QSO AGE LIMIT
    if (LHS == "OLD QSO AGE LIMIT"sv)
      _old_qso_age_limit = from_string<decltype(_old_qso_age_limit)>(rhs);

// PATH
    if ( (LHS == "PATH"sv) and (!rhs.empty()) )
      _path = clean_split_string <string> (rhs, ';');

// PING = [ target1, label1 ], [target2, label2] ...
    if ( (LHS == "PING"sv) and (!rhs.empty()) )
    { for (const auto& target : delimited_substrings <std::string> (rhs, '[', ']', DELIMITERS::DROP))
      { const vector<string> target_info { clean_split_string <string> (target) };;

        if (target_info.size() == 2)
          _ping_targets += { target_info[0], target_info[1] };
      }
    }

// POINTS
// don't use LHS here because the command might be something like "POINTS[80] ="
    if (testline.starts_with("POINTS"sv) and !testline.starts_with("POINTS CW"sv) and !testline.starts_with("POINTS SSB"sv))  // there may be an "=" in the points definitions
    { _set_points(testline, MODE_CW);
      _set_points(testline, MODE_SSB);
    }

// POINTS CW
    if (testline.starts_with("POINTS CW"sv))
      _set_points(testline, MODE_CW);

// POINTS SSB
    if (testline.starts_with("POINTS SSB"sv))
      _set_points(testline, MODE_SSB);

// PTT DELAY (0 => no PTT)
    if (LHS == "PTT DELAY"sv)
      _ptt_delay = from_string<decltype(_ptt_delay)>(RHS);

// POST MONITOR
    if (LHS == "POST MONITOR"sv)
      FOR_ALL(clean_split_string <string> (RHS), [this] (const string& callsign) { _post_monitor_calls += callsign; } );

// POSTED BY CONTINENTS (default is anything other than my own continent)
    if ( (LHS == "POSTED BY CONTINENTS"sv) or (LHS == "POSTED BY CONTINENT"sv) )
    { const UNORDERED_STRING_SET continent_abbreviations { "AF"s, "AS"s, "EU"s, "NA"s, "OC"s, "SA"s, "AN"s };

      if (LHS == "POSTED BY CONTINENTS"sv)    // multiple continents
        FOR_ALL(clean_split_string <string> (RHS), [continent_abbreviations, this] (const string& co) { if (continent_abbreviations.contains(co)) _posted_by_continents += co; } );
      else                                    // single continent
      { if (continent_abbreviations.contains(RHS))
          _posted_by_continents += RHS;             // _posted_by_continents now contains single value
      }
    }

// P3
    if (LHS == "P3"sv)
      _p3 = is_true;

// P3 IGNORE CHECKSUM ERROR
    if (LHS == "P3 IGNORE CHECKSUM ERROR"sv)
      _p3_ignore_checksum_error = is_true;

// P3 SNAPSHOT FILE
    if (LHS == "P3 SNAPSHOT FILE"sv)
      _p3_snapshot_file = rhs;

// P3 SPAN CQ
    if (LHS == "P3 SPAN CQ"sv)
      _p3_span_cq = from_string<decltype(_p3_span_cq)>(RHS);

// P3 SPAN SAP
    if (LHS == "P3 SPAN SAP"sv)
      _p3_span_sap = from_string<decltype(_p3_span_sap)>(RHS);

// QSL MESSAGE
    if (LHS == "QSL MESSAGE"sv)
      _qsl_message = RHS;

// QSO MULTIPLE BANDS
    if (LHS == "QSO MULTIPLE BANDS"sv)
      _qso_multiple_bands = is_true;

// QSO MULTIPLE MODES
    if (LHS == "QSO MULTIPLE MODES"sv)
      _qso_multiple_modes = is_true;

// QSY ON STARTUP
    if (LHS == "QSY ON STARTUP"sv)
      _qsy_on_startup = is_true;

// QTCS
    if (LHS == "QTCS"sv)
      _qtcs = is_true;

// QTC DOUBLE SPACE
    if (LHS == "QTC DOUBLE SPACE"sv)
      _qtc_double_space = is_true;

// QTC FILENAME
    if (LHS == "QTC FILENAME"sv)
      _qtc_filename = rhs;

// QTC LONG T
    if (LHS == "QTC LONG T"sv)
      _qtc_long_t = from_string<decltype(_qtc_long_t)>(rhs);

// QTC QRS
    if (LHS == "QTC QRS"sv)
      _qtc_qrs = from_string<decltype(_qtc_qrs)>(rhs);

// QTHX: QTHX[callsign-or-canonical prefix] = aa, bb, cc...
// the conversion to canonical prefix occurs later, inside contest_rules::_parse_context_qthx()
    if (testline.starts_with("QTHX["sv))
    { const vector<string_view> fields { clean_split_string <string_view> (testline, '=') };

      if (fields.size() == 2)
      { const string         canonical_prefix { delimited_substring <std::string> (fields[0], '[', ']', DELIMITERS::DROP) };
        const vector<string> values           { clean_split_string <string> (RHS) };
//        const STRING_SET     ss               { values.cbegin(), values.cend() };

//        _qthx += { canonical_prefix, ss };
//        _qthx += { canonical_prefix, STRING_SET { values.cbegin(), values.cend() } };
//        _qthx += { canonical_prefix, STRING_SET { values } };
        _qthx += { canonical_prefix, ranges::to<STRING_SET>(values) };
      }
    }

// RATE
    if (LHS == "RATE"sv)
    { vector<unsigned int> new_rates;

      FOR_ALL(clean_split_string <std::string_view> (rhs), [&new_rates] (const auto str) { new_rates += from_string<decltype(_rate_periods)::value_type>(str); } );

      if (!new_rates.empty())
        _rate_periods = new_rates;
    }

// RBN BEACONS
    if (LHS == "RBN BEACONS"sv)
      _rbn_beacons = is_true;

// RBN FILE
    if (LHS == "RBN FILE"sv)
    { _rbn_file = remove_peripheral_spaces <std::string> (rhs);

      ost << "RBN stream will be written to: " << _rbn_file << endl;
    }

// RBN PORT
    if (LHS == "RBN PORT"sv)
      _rbn_port = from_string<decltype(_rbn_port)>(rhs);

// RBN SERVER
    if (LHS == "RBN SERVER"sv)
      _rbn_server = rhs;

// RBN THRESHOLD
    if (LHS == "RBN THRESHOLD"sv)
      _rbn_threshold = from_string<decltype(_rbn_threshold)>(rhs);

// RBN USERNAME
    if (LHS == "RBN USERNAME"sv)
      _rbn_username = rhs;

// REJECT COLOUR
    if ( ( (LHS == "REJECT COLOUR"sv) or (LHS == "REJECT COLOR"sv) ) and !rhs.empty() )
      _reject_colour = string_to_colour(RHS);

// REQUIRE DOT IN REPLACEMENT CALL
    if (LHS == "REQUIRE DOT IN REPLACEMENT CALL"sv)
      _require_dot_in_replacement_call = is_true;

// RIG 1 BAUD
    if ( (LHS == "RIG 1 BAUD"sv) or (LHS == "RIG BAUD"sv) )
      _rig1_baud = from_string<decltype(_rig1_baud)>(rhs);

// RIG 1 DATA BITS
    if ( (LHS == "RIG 1 DATA BITS"sv) or (LHS == "RIG DATA BITS"sv) )
      _rig1_data_bits = from_string<decltype(_rig1_data_bits)>(rhs);

// RIG 1 NAME
    if ( (LHS == "RIG 1 NAME"sv) or (LHS == "RADIO ONE NAME"sv) )
      _rig1_name = rhs;

// RIG 1 PORT
    if ( (LHS == "RIG 1 PORT"sv) or (LHS == "RADIO ONE CONTROL PORT"sv) )
      _rig1_port = rhs;

// RIG 1 STOP BITS
    if ( (LHS == "RIG 1 STOP BITS"sv) or (LHS == "RIG STOP BITS"sv) )
      _rig1_stop_bits = from_string<decltype(_rig1_stop_bits)>(rhs);

// RIG 1 TYPE
    if (LHS == "RIG 1 TYPE"sv)
      _rig1_type = RHS;

// RULES
    if (LHS == "RULES"sv)
      _process_configuration_file(rhs);

// RUSSIAN DATA
    if (LHS == "RUSSIAN DATA"sv)
      _russian_filename = rhs;

// SCORE BANDS
    if (testline.starts_with("SCORE BANDS"sv))
    { for (const auto& band_str : clean_split_string <string> (rhs))
      { try
        { _score_bands += BAND_FROM_NAME.at(band_str);  // .at() does not yet support heterogenwous lookup
        }

        catch (...)
        { }
      }
    }

// SCORE MODES
    if (testline.starts_with("SCORE MODES"sv))
    { if (contains(testline, "CW"sv))
        _score_modes += MODE_CW;

      if (contains(testline, "SSB"sv) or contains(testline, "PH"sv))
        _score_modes += MODE_SSB;
    }

// SCORING
    if (LHS == "SCORING"sv)
      _scoring_enabled = is_true;

// SCREEN SNAPSHOT FILE
    if ( (LHS == "SCREEN SNAPSHOT FILE"sv) or (LHS == "SCREENSHOT FILE"sv) )
      _screen_snapshot_file = rhs;

// SCREEN SNAPSHOT ON EXIT
    if ( (LHS == "SCREEN SNAPSHOT ON EXIT"sv) or (LHS == "SCREENSHOT ON EXIT"sv) )
      _screen_snapshot_on_exit = is_true;

// SELF-SPOTTING ENABLE
    if ( (LHS == "SELF-SPOTTING ENABLE"sv) or (LHS == "SELF-SPOTTING ENABLED"sv) )
      _self_spotting_enabled = is_true;

// SELF-SPOTTING TEXT
    if (LHS == "SELF-SPOTTING TEXT"sv)
      _self_spotting_text = RHS;

// SERIAL NUMBER SPACES
    if (LHS == "SERIAL NUMBER SPACES"sv)
      _serno_spaces = from_string<decltype(_serno_spaces)>(rhs);

// SHIFT DELTA
    if (LHS == "SHIFT DELTA"sv)
    { _shift_delta_cw = from_string<decltype(_shift_delta_cw)>(rhs);
      _shift_delta_ssb = _shift_delta_cw;
    }

// SHIFT DELTA CW
    if (LHS == "SHIFT DELTA CW"sv)
      _shift_delta_cw = from_string<decltype(_shift_delta_cw)>(rhs);

// SHIFT DELTA SSB
    if (LHS == "SHIFT DELTA SSB"sv)
      _shift_delta_ssb = from_string<decltype(_shift_delta_ssb)>(rhs);

// SHIFT POLL
    if (LHS == "SHIFT POLL"sv)
      _shift_poll = from_string<decltype(_shift_poll)>(rhs);

// SHORT SERIAL NUMBER
    if (LHS == "SHORT SERIAL NUMBER"sv)
      _short_serno = is_true;

// SOCIETY LIST FILENAME
    if (LHS == "SOCIETY LIST FILENAME"sv)
      _society_list_filename = rhs;

// SSB AUDIO
    if (LHS == "SSB AUDIO"sv)
    { if (const vector<string_view> cbw { clean_split_string <string_view> (RHS, '/') }; cbw.size() == 2)
      { const vector<string_view> cbw_wide { clean_split_string <string_view> (cbw[0], ':') };

        _ssb_centre_wide = from_string<decltype(_ssb_centre_wide)>(cbw_wide[0]);
        _ssb_bandwidth_wide = from_string<decltype(_ssb_bandwidth_wide)>(cbw_wide[1]);

        const vector<string_view> cbw_narrow { clean_split_string <string_view> (cbw[1], ':') };

        _ssb_centre_narrow = from_string<decltype(_ssb_centre_narrow)>(cbw_narrow[0]);
        _ssb_bandwidth_narrow = from_string<decltype(_ssb_bandwidth_narrow)>(cbw_narrow[1]);
      }
    }

// START AUDIO RECORDING
    if (LHS == "START AUDIO RECORDING"sv)
    { if (rhs == "auto"sv)
        _start_audio_recording = AUDIO_RECORDING::AUTO;

      if (rhs == "false"sv)
        _start_audio_recording = AUDIO_RECORDING::DO_NOT_START;

      if (rhs == "true"sv)
        _start_audio_recording = AUDIO_RECORDING::START;
    }

// START BAND
    if (LHS == "START BAND"sv)
    { if (const auto opt { OPT_MUM_VALUE(BAND_FROM_NAME, RHS) }; opt)
        _start_band = opt.value();
    }

// START MODE
    if (LHS == "START MODE"sv)
    { if (RHS == "SSB"sv)
        _start_mode = MODE_SSB;
    }

// SYNC KEYER
    if (LHS == "SYNC KEYER"sv)
      _sync_keyer = is_true;

// TEST
    if (LHS == "TEST"sv)
      _test = is_true;

// THOUSANDS SEPARATOR
    if (LHS == "THOUSANDS SEPARATOR"sv)
      _thousands_separator = ( (rhs.size() >= 1) ? rhs[0] : ' ' );

// UBA BONUS
    if (LHS == "UBA BONUS"sv)
      _uba_bonus = is_true;

// WORKED MULTS COLOUR
    if ( (LHS == "WORKED MULTS COLOUR"sv) or (LHS == "WORKED MULTS COLOR"sv) )
      _worked_mults_colour = string_to_colour(RHS);

// XSCP CUTOFF
    if ( (LHS == "XSCP CUTOFF"sv) or (LHS == "XSCP LIMIT"sv) or (LHS == "XSCP MINIMUM"sv) )
    { if (rhs.ends_with('%'))           // if percentage
        _xscp_percent_cutoff = clamp(from_string<decltype(_xscp_cutoff)>(remove_char_from_end <std::string> (rhs, '%')), 0, 100);
      else
        _xscp_cutoff = from_string<decltype(_xscp_cutoff)>(rhs);    // remains at default value (== 1) if % is present
    }

// XSCP SORT
    if (LHS == "XSCP SORT"sv)
      _xscp_sort = is_true;

// ---------------------------------------------  MULTIPLIERS  ---------------------------------


// COUNTRY MULTS
// Currently supported: ALL
//                      NONE
// any single continent
    if (LHS == "COUNTRY MULTS"sv)
    { _country_mults_filter = RHS;

      if (_country_mults_filter == "NONE"sv)
        QSO_DISPLAY_COUNTRY_MULT = false;                  // since countries aren't mults, don't take up space in the log line
    }

// REMAINING CALLSIGN MULTS
    if (LHS == "REMAINING CALLSIGN MULTS"sv)
    { _auto_remaining_callsign_mults = (RHS == "AUTO"sv);

      if (_auto_remaining_callsign_mults)
      { if (const vector<string_view> tokens { split_string <std::string_view> (RHS, ' ') }; tokens.size() == 2)
          _auto_remaining_callsign_mults_threshold = from_string<decltype(_auto_remaining_callsign_mults_threshold)>(tokens[1]);
      }
      else
        _remaining_callsign_mults_list = ranges::to<STRING_SET>( clean_split_string <string> (RHS) );
    }

// REMAINING COUNTRY MULTS
    if (LHS == "REMAINING COUNTRY MULTS"sv)
    { _auto_remaining_country_mults = contains(RHS, "AUTO"s);

      if (_auto_remaining_country_mults)
      { const vector<string_view> tokens { split_string <std::string_view> (RHS, ' ') };

        if (tokens.size() == 2)
          _auto_remaining_country_mults_threshold = from_string<decltype(_auto_remaining_callsign_mults_threshold)>(tokens[1]);
      }
      else
        _remaining_country_mults_list = std::ranges::to<STRING_SET>(clean_split_string <string_view> (RHS));
    }

// AUTO REMAINING EXCHANGE MULTS (the exchange mults whose list of legal values can be augmented)
    if (LHS == "AUTO REMAINING EXCHANGE MULTS"sv)
      FOR_ALL(clean_split_string <string_view> (RHS), [this] (const string_view str) { _auto_remaining_exchange_mults += str; });
// the following line works but is very slow (see example at start of main())
//      _auto_remaining_exchange_mults = std::ranges::fold_left(clean_split_string <string_view> (RHS), STRING_SET { }, [] (STRING_SET acc, const string_view sv) { return acc + sv; }); // can't use a STRING_SET&;

// ---------------------------------------------  CABRILLO  ---------------------------------

// CABRILLO CONTEST
    if (LHS == "CABRILLO CONTEST"sv)
      _cabrillo_contest = RHS;          // required to be upper case; don't limit to legal values defined in the "specification", since many contests require an illegal value

    if ( (LHS == "CABRILLO CERTIFICATE"sv) and is_legal_value(RHS, "YES,NO"sv /*, ',' */) )
      _cabrillo_certificate = RHS;

 // CABRILLO EMAIL (sic)
    if ( (LHS == "CABRILLO E-MAIL"sv) or (LHS == "CABRILLO EMAIL"sv) )
      _cabrillo_e_mail = rhs;

// CABRILLO EOL
    if (LHS == "CABRILLO EOL"sv)
      _cabrillo_eol = RHS;

// CABRILLO INCLUDE SCORE
    if (LHS == "CABRILLO INCLUDE SCORE"sv)
      _cabrillo_include_score = is_true;

// CABRILLO LOCATION
    if (LHS == "CABRILLO LOCATION"sv)
      _cabrillo_location = rhs;

// CABRILLO NAME
    if (LHS == "CABRILLO NAME"sv)
      _cabrillo_name = rhs;

// CABRILLO CATEGORY-ASSISTED
    if ( (LHS == "CABRILLO CATEGORY-ASSISTED"sv) and is_legal_value(RHS, "ASSISTED,NON-ASSISTED"sv) )
      _cabrillo_category_assisted = RHS;

// CABRILLO CATEGORY-BAND
    if (LHS == "CABRILLO CATEGORY-BAND"sv)
    { if (is_legal_value(rhs, "ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light"sv)) // The spec calls for bizarre capitalization
        _cabrillo_category_band = rhs;
    }

// CABRILLO CATEGORY-MODE
    if (LHS == "CABRILLO CATEGORY-MODE"sv)
    { if (is_legal_value(RHS, "CW,MIXED,RTTY,SSB"sv))
        _cabrillo_category_mode = RHS;
    }

// CABRILLO CATEGORY-OPERATOR
    if (LHS == "CABRILLO CATEGORY-OPERATOR"sv)
    { if (is_legal_value(RHS, "CHECKLOG,MULTI-OP,SINGLE-OP"sv))
        _cabrillo_category_operator = RHS;
    }

// CABRILLO CATEGORY-OVERLAY
    if (LHS == "CABRILLO CATEGORY-OVERLAY"sv)
    { if (is_legal_value(RHS, "NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES"sv))
        _cabrillo_category_overlay = RHS;
    }

// CABRILLO CATEGORY-POWER
    if (LHS == "CABRILLO CATEGORY-POWER"sv)
    { if (is_legal_value(RHS, "HIGH,LOW,QRP"sv))
        _cabrillo_category_power = RHS;
    }

// CABRILLO CATEGORY-STATION
    if (LHS == "CABRILLO CATEGORY-STATION"sv)
    { if (is_legal_value(RHS, "EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL"sv))
        _cabrillo_category_station = RHS;
    }

// CABRILLO CATEGORY-TIME
    if (LHS == "CABRILLO CATEGORY-TIME"sv)
    { if (is_legal_value(RHS, "6-HOURS,12-HOURS,24-HOURS"sv))
        _cabrillo_category_station = RHS;
    }

// CABRILLO CATEGORY-TRANSMITTER
    if (LHS == "CABRILLO CATEGORY-TRANSMITTER"sv)
    { if (is_legal_value(RHS, "LIMITED,ONE,SWL,TWO,UNLIMITED"sv))
        _cabrillo_category_transmitter = RHS;
    }

// CABRILLO CLUB
    if (LHS == "CABRILLO CLUB"sv)
      _cabrillo_club = RHS;

// CABRILLO ADDRESS first line
    if (LHS == "CABRILLO ADDRESS 1"sv)
      _cabrillo_address_1 = rhs;

// CABRILLO ADDRESS second line
    if (LHS == "CABRILLO ADDRESS 2"sv)
      _cabrillo_address_2 = rhs;

// CABRILLO ADDRESS third line
    if (LHS == "CABRILLO ADDRESS 3"sv)
      _cabrillo_address_3 = rhs;

// CABRILLO ADDRESS fourth line
    if (testline.starts_with("CABRILLO ADDRESS 4"sv))
      _cabrillo_address_4 = rhs;

// CABRILLO ADDRESS-CITY
    if (testline.starts_with("CABRILLO ADDRESS-CITY"sv))
      _cabrillo_address_city = rhs;

// CABRILLO ADDRESS-STATE-PROVINCE
    if (testline.starts_with("CABRILLO ADDRESS-STATE-PROVINCE"sv))
      _cabrillo_address_state_province = rhs;

// CABRILLO ADDRESS-POSTALCODE
    if (testline.starts_with("CABRILLO ADDRESS-POSTALCODE"sv))
      _cabrillo_address_postalcode = rhs;

// CABRILLO ADDRESS-COUNTRY
    if (testline.starts_with("CABRILLO ADDRESS-COUNTRY"sv))
      _cabrillo_address_country = rhs;

// CABRILLO OPERATORS
    if (testline.starts_with("CABRILLO OPERATORS"sv))
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

    if (LHS == "CABRILLO QSO"sv)
    { _cabrillo_qso_template = RHS;

      if (contains(RHS, "TEMPLATE"sv))
      { try
        { _cabrillo_qso_template = cabrillo_qso_templates.at( clean_split_string <string> (RHS, ':')[1] );
        }

        catch (...)
        { ost << "Error in CABRILLO QSO TEMPLATE" << endl;
          exit(-1);
        }
      }
    }

// ---------------------------------------------  WINDOWS  ---------------------------------

    if (LHS == "WINDOW"sv)
    { if (vector<string> window_info { clean_split_string <string> (split_string <std::string> (testline, '=')[1]) }; window_info.size() >= 5)
      { string             name  { window_info[0] };
        window_information winfo { from_string<WIN_INT_TYPE>(window_info[1]), from_string<WIN_INT_TYPE>(window_info[2]), from_string<WIN_INT_TYPE>(window_info[3]), from_string<WIN_INT_TYPE>(window_info[4]) };

        if (window_info.size() >= 6)
        { winfo.fg_colour(move(window_info[5]));

          if (window_info.size() >= 7)
            winfo.bg_colour(move(window_info[6]));

          winfo.colours_set(true);
        }

        _windows += { move(name), move(winfo) };
      }
    }    // end of WINDOW

// ---------------------------------------------  STATIC WINDOWS  ---------------------------------

    static STRING_MAP<bool /* whether verbatim */> verbatim;   // key = name

    if (LHS == "STATIC WINDOW"sv)
    { const vector<string> fields { clean_split_string <string> (rhs) };       // avoid heterogeneous lookup (see below)

      if (fields.size() == 2)  // name, contents
      { const string& name { fields[0] };
        //const string_view name { fields[0] };       // avoid heterogeneous lookup (see below)

        string contents { fields[1] };      // might be actual contents, or a fully-qualified filename

        verbatim[name] = contains(fields[1], '"');     // verbatim if contains quotation mark; operator [] does not yet support heterogeneous lookup

        if (file_exists(contents))
          contents = read_file(contents);

        _static_windows[name] = { contents, vector<window_information> { } };
      }
    }
//    std::map<std::string /* name */, std::pair<std::string /* contents */, std::vector<window_information> > > _static_windows;

    if (LHS == "STATIC WINDOW INFO"sv)  // must come after the corresponding STATIC WINDOW command
    { const vector<string> window_info { clean_split_string <string> (split_string <std::string> (testline, '=')[1], ',') };

      if (!window_info.empty())
      { const string& name { window_info[0] };

        if (_static_windows.count(name) != 0)  // make sure that the window exists
        { if (window_info.size() >= 3)
          { window_information winfo;

            winfo.x(from_string<WIN_INT_TYPE>(window_info[1]));
            winfo.y(from_string<WIN_INT_TYPE>(window_info[2]));

            if (window_info.size() >= 5)
            { winfo.w(from_string<WIN_INT_TYPE>(window_info[3]));
              winfo.h(from_string<WIN_INT_TYPE>(window_info[4]));
            }

            string final_contents;

            auto& [ swin_contents, vec_winfo ] { _static_windows[name] };

            if (verbatim[name])
            { string contents { swin_contents };

              if (contents.size() >= 2)
                contents = delimited_substring <std::string> (contents, '"', '"', DELIMITERS::DROP);

              vector<string> lines { to_lines <std::string> (contents) };

              const string contents_1 { replace(contents, "\\n"s, EOL) };

              lines = to_lines <std::string> (contents_1);

              winfo.w(longest(lines).length());
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
            { const string&        contents { swin_contents };
              const vector<string> lines    { to_lines <std::string> (contents) };

              winfo.w(longest(lines).length());
              winfo.h(lines.size());

              if (window_info.size() >= 4)
              { winfo.fg_colour(window_info[3]);

                if (window_info.size() >= 5)
                  winfo.bg_colour(window_info[4]);

                winfo.colours_set(true);
              }

              final_contents = contents;
            }

            _static_windows[name] = { final_contents, (vec_winfo + winfo) };
          }
        }
      }
    }

// ---------------------------------------------  MESSAGES  ---------------------------------

    if (testline.starts_with("MESSAGE KEY"sv))
    { const vector<string_view> message_info { split_string <std::string_view> (testline, ' ') };

      if ( (message_info.size() >= 5) and contains(testline, '=') )
      {
// a big switch to convert from text in the configuration file to a KeySym, which is what we use as the key in the message map
// this could be more efficient by generating a container of all the legal keysym names and then comparing the target to all those.
// The latter is now what we do. The legal names are in key_names.
        const string target { to_lower(message_info[2]) };

        if (const auto& cit { key_names.find(target) }; cit != key_names.cend())    // key_names, defined in keyboard.cpp, maps names to keysyms
        { try
          { const auto&          [ keyname_str, key_symbol ] { *cit };
            const vector<string> vec_str                     { split_string <std::string> (testline, '=') };
            const string         str                         { remove_leading_spaces <std::string> (vec_str.at(1)) };

// everything to the right of the = -- we assume there's only one -- goes into the message, excepting any leading space
            _messages += { key_symbol, str };

            ost << "message associated with " << target << ", which is keysym " << hex << key_symbol << dec << ", is: " << str << endl;

            if (const auto& cit2 { equivalent_key_names.find(target) }; cit2 != equivalent_key_names.cend())
            { const auto& [ ori_keyname_str, equiv_keyname_str ] { *cit2 };

              ost << "found equivalent key name: " << equiv_keyname_str << endl;

              const string& alternative { equiv_keyname_str };

              if (const auto& cit { key_names.find(alternative) }; cit != key_names.cend())
              { const auto& [ alt_keyname_str, alt_key_symbol ] { *cit };

                if (!_messages.contains(alt_key_symbol))  // only if there is no message for this key
                {  ost << "message associated with equivalent key is: " << str << endl;

                  _messages += { alt_key_symbol, str };
                }
              }
            }
          }

          catch (...)
          { ost << "ERROR parsing line in config file: " << testline << endl;
            exit(-1);
          }
        }
      }
    }

    if (LHS == "MESSAGE CQ 1"sv)
    { const vector<string> tokens { split_string <std::string> (testline, '=') };

      if (tokens.size() != 2)
        print_error_and_exit(testline);

      _message_cq_1 = move(tokens[1]);
    }

    if (LHS == "MESSAGE CQ 2"sv)
    { const vector<string> tokens { split_string <std::string> (testline, '=') };

      if (tokens.size() != 2)
        print_error_and_exit(testline);

      if (tokens.size() == 2)
        _message_cq_2 = move(tokens[1]);
    }
  }

// now that we've processed the file, set some new defaults if we haven't explicitly set them
  if (_cabrillo_contest.empty())
    _cabrillo_contest = _contest_name;

  if (_cabrillo_callsign.empty())
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
  { for ( const auto& str : clean_split_string <std::string> (bands()) )
    { try
      { _call_history_bands += BAND_FROM_NAME.at(str);    // heterogeneous lookup not yet supported
      }

      catch (...)
      { }
    }
  }

// possibly fix Cabrillo template
  if ( (_cabrillo_qso_template == "ARRL DX"sv) or (_cabrillo_qso_template == "CQ WW"sv) )
  { const vector<string_view> actual_modes { clean_split_string <string_view> (_modes) };

    if (actual_modes.size() == 1)
    { try
      { if (STRING_SET( { "ARRL DX"s, "CQ WW"s, "JIDX"s} ).contains(_cabrillo_qso_template))
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
drlog_context::drlog_context(const string_view filename)
{ for (unsigned int n { 0 }; n < NUMBER_OF_BANDS; ++n)
    _per_band_country_mult_factor += { static_cast<BAND>(n), 1 };

  _process_configuration_file(filename);

// make sure that the default is to score all permitted bands
  if (_score_bands.empty())
  { for (const auto& band_name : clean_split_string <string> (_bands))
    //for (const auto& band_name : clean_split_string <string_view> (_bands)) // doesn't work in g++12; fails when at() is called
    { try
      { _score_bands += BAND_FROM_NAME.at(band_name);       // heteogeneous lookup is not yet implemented for at()
      }

      catch (...)
      { }
    }
  }

// default is to score all permitted modes
  if (_score_modes.empty())
  { if (_modes.contains("CW"sv))
      _score_modes += MODE_CW;

    if (_modes.contains("SSB"sv))
      _score_modes += MODE_SSB;
  }
}

/*! \brief          Get all the windows whose name contains a particular substring
    \param  substr  substring for which to search
    \return         all the window names that include <i>substr</i>
*/
vector<string> drlog_context::window_name_contains(const string_view substr) const
{ vector<string> rv;

  for (const auto& [ name, _ ] : _windows)
    if (contains(name, substr))
      rv += name;

  return rv;
}

/*! \brief      Get the names of all the fields in the sent exchange
    \return     the names of all the fields in the sent exchange
*/
vector<string> drlog_context::sent_exchange_names(void) const
{ vector<string> rv;

  for (const auto& [name, _] : _sent_exchange)
    rv += name;

  return rv;
}

/*! \brief      Get all the field names in the exchange sent for a particular mode
    \param  m   target mode
    \return     the names of all the fields in the sent exchange for mode <i>m</i>
*/
vector<string> drlog_context::sent_exchange_names(const MODE m) const
{ vector<string> rv;

  for ( const auto& [ name, _ ] : (m == MODE_CW ? _sent_exchange_cw : _sent_exchange_ssb) )
    rv += name;

  return rv;
}

/*! \brief      Get names and values of sent exchange fields for a particular mode
    \param  m   target mode
    \return     the names and values of all the fields in the sent exchange when the mode is <i>m</i>
*/
decltype(drlog_context::_sent_exchange) drlog_context::sent_exchange(const MODE m) const      // doxygen complains about the decltype; I have no idea why
{ using RT = decltype(_sent_exchange);

  SAFELOCK(_context);

  RT rv { ( (m == MODE_CW) ? _sent_exchange_cw : _sent_exchange_ssb) };

  if (rv.empty())
  { rv = _sent_exchange;

// fix RST/RS if using non-mode-specific exchange
    for (auto& [name, value] : rv)
    { if ( (m == MODE_SSB) and (name == "RST"sv) )
        tie(name, value) = pair { "RS"s, "59"s };       // hardwire report

      if ( (m == MODE_CW) and (name == "RS"sv) )
        tie(name, value) = pair { "RST"s, "599"s };     // hardwire report
    }
  }

  return rv;
}
