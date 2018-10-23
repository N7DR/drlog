// $Id: drlog_context.cpp 148 2018-05-05 20:29:09Z  $

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

extern message_stream   ost;                        ///< for debugging, info
extern bool             QSO_DISPLAY_COUNTRY_MULT;   ///< controls whether country mults are written on the log line
extern int              QSO_MULT_WIDTH;             ///< controls width of zone mults field in log line

/// example: cabrillo qso = template: CQ WW
static const map<string, string> cabrillo_qso_templates { { "ARRL DX", "ARRL DX" }, // placeholder; mode chosen before we exit this function
                                                          { "ARRL DX CW", "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-STATE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CWPOWER:74:6:R, TXID:81:1" },
                                                          { "ARRL DX SSB", "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RS:45:3:R, TEXCH-STATE:49:6:R, RCALL:56:13:R, REXCH-RS:70:3:R, REXCH-SSBPOWER:74:6:R, TXID:81:1" },

                                                          { "CQ WW",      "CQ WW" }, // placeholder; mode chosen before we exit this function
                                                          { "CQ WW CW",   "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1" },
                                                          { "CQ WW SSB",  "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RS:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RS:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1" },

                                                          { "JIDX",      "JIDX" }, // placeholder; mode chosen before we exit this function
                                                          { "JIDX CW",   "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-JAPREF:74:6:R, TXID:81:1" },
                                                          { "JIDX SSB",  "FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RS:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RS:70:3:R, REXCH-JAPREF:74:6:R, TXID:81:1" }
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

pt_mutex _context_mutex;                    ///< mutex for the context

/*! \brief              Set the value of points, using the POINTS [CW|SSB] command
    \param  command     the complete line from the configuration file
    \param  m           mode
*/
void drlog_context::_set_points(const string& command, const MODE m)
{ if (command.empty())
    return;

  const vector<string> str_vec = split_string(command, "=");

  if (str_vec.size() != 2)
  { ost << "Invalid command: " << command << endl;
    exit(0);
  }

  const string RHS = to_upper(remove_peripheral_spaces(str_vec[1]));

  if (!str_vec.empty())
  { const string lhs = str_vec[0];

    string tmp_points_str;
    auto& pbb = _per_band_points[m];

    if (!contains(lhs, "[") or contains(lhs, "[*]"))            // for all bands
    { for (unsigned int n = 0; n < NUMBER_OF_BANDS; ++n)
        pbb.insert( { static_cast<BAND>(n), RHS } );
    }
    else                                                        // not all bands
    { const size_t left_bracket_posn = lhs.find('[');
      const size_t right_bracket_posn = lhs.find(']');

      const bool valid = (left_bracket_posn != string::npos) and (right_bracket_posn != string::npos) and (left_bracket_posn < right_bracket_posn);

      if (valid)
      { const string bands_str = lhs.substr(left_bracket_posn + 1, (right_bracket_posn - left_bracket_posn - 1));
        const vector<string> bands = remove_peripheral_spaces(split_string(bands_str, ","));

        for (const auto b_str : bands)
          pbb.insert( { BAND_FROM_NAME[b_str], RHS } );
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

  const vector<string> lines = split_string(entire_file, LF_STR);   // split into lines

  for (const auto& tmpline : lines)                                    // process each line
  { const string line = remove_trailing_comment(tmpline);           // remove any comment

//    const size_t posn = line.find_first_of("//");

//    if (posn != string::npos)
//      line = substring(line, 0, posn);

// generate a number of useful variables
    const string testline = remove_leading_spaces(to_upper(line));
    const vector<string> fields = split_string(line, "=");
    const string rhs = ((fields.size() > 1) ? remove_peripheral_spaces(fields[1]) : "");      // the stuff to the right of the "="
    const string RHS = to_upper(rhs);                                                         // converted to upper case
    const bool is_true = (RHS == "TRUE");                                                     // is right hand side == "TRUE"?
    const string lhs = squash( (fields.empty()) ? "" : remove_peripheral_spaces(fields[0]) ); // the stuff to the left of the "="
    const string LHS = to_upper(lhs);                                                         // converted to upper case

// ACCEPT COLOUR
    if ( ( (LHS == "ACCEPT COLOUR") or (LHS == "ACCEPT COLOR") ) and !rhs.empty() )
      _accept_colour = string_to_colour(RHS);

// ALLOW AUDIO RECORDING
    if (LHS == "ALLOW AUDIO RECORDING")
      _allow_audio_recording = is_true;

// ALTERNATIVE EXCHANGE CQ
    if (LHS == "ALTERNATIVE EXCHANGE CQ")
      _alternative_exchange_cq = RHS;

// ALTERNATIVE EXCHANGE SAP
    if (LHS == "ALTERNATIVE EXCHANGE SAP")
      _alternative_exchange_sap = RHS;

// ALTERNATIVE QSL MESSAGE
    if ( (LHS == "ALTERNATIVE QSL MESSAGE") or (LHS == "QUICK QSL MESSAGE") )
      _alternative_qsl_message = RHS;

// ARCHIVE
    if ( (LHS == "ARCHIVE") and !rhs.empty() )
      _archive_name = rhs;

// AUDIO CHANNELS
    if (LHS == "AUDIO CHANNELS")
      _audio_channels = from_string<unsigned int>(rhs);

// AUDIO DEVICE
    if ( (LHS == "AUDIO DEVICE") or (LHS == "AUDIO DEVICE NAME") )
      _audio_device_name = rhs;

// AUDIO DURATION
    if (LHS == "AUDIO DURATION")
      _audio_duration = from_string<unsigned int>(rhs);

// AUDIO FILE
    if (LHS == "AUDIO FILE")
      _audio_file = rhs;

// AUDIO RATE
    if (LHS == "AUDIO RATE")
      _audio_rate = from_string<unsigned int>(rhs);

// AUTO BACKUP
    if ( (LHS =="AUTO BACKUP") and !rhs.empty() )
      _auto_backup = rhs;

// AUTO SCREENSHOT
    if ( (LHS == "AUTO SCREENSHOT") and !rhs.empty() )
      _auto_screenshot = is_true;

// BAND MAP DECAY TIME CLUSTER
    if ( ( (LHS == "BAND MAP DECAY TIME CLUSTER") or (LHS == "BANDMAP DECAY TIME CLUSTER") ) and !rhs.empty() )
      _bandmap_decay_time_cluster = from_string<int>(rhs);

// BAND MAP DECAY TIME LOCAL
    if ( ( (LHS == "BAND MAP DECAY TIME LOCAL") or (LHS == "BANDMAP DECAY TIME LOCAL") ) and !rhs.empty() )
      _bandmap_decay_time_local = from_string<int>(rhs);

// BAND MAP DECAY TIME RBN
    if ( (LHS == "BAND MAP DECAY TIME RBN") or (LHS == "BANDMAP DECAY TIME RBN") )
      _bandmap_decay_time_rbn = from_string<int>(rhs);

// BAND MAP FADE COLOURS
    if ( (LHS == "BAND MAP FADE COLOURS") or (LHS == "BANDMAP FADE COLOURS") or
         (LHS == "BAND MAP FADE COLORS") or (LHS == "BANDMAP FADE COLORS") )
    { _bandmap_fade_colours.clear();

      if (!RHS.empty())
      { const vector<string> colour_names = remove_peripheral_spaces(split_string(RHS, ","));

        FOR_ALL(colour_names, [&] (const string& name) { _bandmap_fade_colours.push_back(string_to_colour(name)); } );
      }
    }

// BAND MAP FILTER
    if ( (LHS == "BAND MAP FILTER") or (LHS == "BANDMAP FILTER") )
    { if (!RHS.empty())
      { vector<string> filters = remove_peripheral_spaces(split_string(RHS, ","));

        sort(filters.begin(), filters.end(), compare_calls);    // put the entries into callsign order
        _bandmap_filter = filters;
      }
    }

// BAND MAP FILTER COLOURS
    if ( (LHS == "BAND MAP FILTER COLOURS") or (LHS == "BAND MAP FILTER COLORS") or
         (LHS == "BANDMAP FILTER COLOURS") or (LHS == "BANDMAP FILTER COLORS") )
    { if (!RHS.empty())
      { const vector<string> colours = split_string(RHS, ",");

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
    if ( (LHS == "BAND MAP FILTER ENABLE") or (LHS == "BANDMAP FILTER ENABLE") )
      _bandmap_filter_enabled = is_true;

// BAND MAP FILTER MODE
    if ( (LHS == "BAND MAP FILTER MODE") or (LHS == "BANDMAP FILTER MODE") )
      _bandmap_filter_show = (RHS == "SHOW");

// BAND MAP GUARD BAND CW
    if ( (LHS == "BAND MAP GUARD BAND CW") or (LHS == "BANDMAP GUARD BAND CW") )
      _guard_band[MODE_CW] = from_string<int>(rhs);

// BAND MAP GUARD BAND SSB
    if ( (LHS == "BAND MAP GUARD BAND SSB") or (LHS == "BANDMAP GUARD BAND SSB") )
      _guard_band[MODE_SSB] = from_string<int>(rhs);

// BAND MAP RECENT COLOUR
    if ( (LHS == "BAND MAP RECENT COLOUR") or (LHS == "BANDMAP RECENT COLOUR") or
         (LHS == "BANDMAP RECENT COLOR") or (LHS == "BANDMAP RECENT COLOR") )
    { if (!RHS.empty())
        _bandmap_recent_colour = string_to_colour(remove_peripheral_spaces(RHS));
    }

// BANDS
    if (LHS == "BANDS")
      _bands = RHS;

// BATCH MESSAGES FILE
    if (LHS == "BATCH MESSAGES FILE")
      _batch_messages_file = rhs;

// BEST DX UNIT
    if ( (LHS == "BEST DX UNIT") or (LHS == "BEST DX UNITS") )
      _best_dx_unit = RHS;

// CABRILLO FILENAME
    if (LHS == "CABRILLO FILENAME")
      _cabrillo_filename = rhs;

// CALL OK NOW MESSAGE
    if (LHS == "CALL OK NOW MESSAGE")
      _call_ok_now_message = rhs;

// CALLSIGN MULTS
    if (LHS == "CALLSIGN MULTS")
    { const vector<string> callsign_mults_vec = remove_peripheral_spaces(split_string(RHS, ","));

      move(callsign_mults_vec.cbegin(), callsign_mults_vec.cend(), inserter(_callsign_mults, _callsign_mults.begin()));
    }

// CALLSIGN MULTS PER BAND
    if (LHS == "CALLSIGN MULTS PER BAND")
      _callsign_mults_per_band = is_true;

// CALLSIGN MULTS PER MODE
    if (LHS == "CALLSIGN MULTS PER MODE")
      _callsign_mults_per_mode = is_true;

// CLUSTER PORT
    if (LHS == "CLUSTER PORT")
      _cluster_port = from_string<unsigned int>(rhs);

// CLUSTER SERVER
    if (LHS == "CLUSTER SERVER")
      _cluster_server = rhs;

// CLUSTER USERNAME
    if (LHS == "CLUSTER USERNAME")
      _cluster_username = rhs;

// CONTEST
    if (LHS == "CONTEST")
      _contest_name = RHS;

// COUNTRY FILENAME
    if (LHS == "COUNTRY FILENAME")
      _cty_filename = rhs;

// COUNTRY LIST
    if ( LHS == "COUNTRY LIST")
    { if (RHS == "DXCC")
        _country_list = COUNTRY_LIST_DXCC;

      if (RHS == "WAEDC")
        _country_list = COUNTRY_LIST_WAEDC;
    }

// COUNTRY MULT FACTOR
    if (LHS == "COUNTRY MULT FACTOR")  // there may be an "=" in the points definitions
    { const vector<string> str_vec = split_string(line, "=");

      if (!str_vec.empty())
      { string tmp_str;
        const string lhs = str_vec[0];

        if (!contains(lhs, "[") or contains(lhs, "[*]"))             // for all bands
        { string new_str;

          for (unsigned int n = 1; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
          { new_str += str_vec[n];
            if (n != str_vec.size() - 1)
              new_str += "=";
          }

          tmp_str = to_upper(remove_peripheral_spaces(new_str));

          for (unsigned int n = 0; n < NUMBER_OF_BANDS; ++n)
            _per_band_country_mult_factor.insert( { static_cast<BAND>(n), from_string<int>(tmp_str) } );
        }
        else    // not all bands
        {
          { const string bands_str = delimited_substring(lhs, '[', ']');
            const vector<string> bands = remove_peripheral_spaces(split_string(bands_str, ","));

            for (const auto b_str: bands)
            { const BAND b = BAND_FROM_NAME[b_str];
              string new_str;

              for (unsigned int n = 1; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
              { new_str += str_vec[n];

                if (n != str_vec.size() - 1)
                  new_str += "=";
              }

              tmp_str = to_upper(remove_peripheral_spaces(new_str));
              _per_band_country_mult_factor.insert( { b, from_string<int>(tmp_str) } );
            }
          }
        }
      }
    }

// COUNTRY MULTS PER BAND
    if (LHS == "COUNTRY MULTS PER BAND")
      _country_mults_per_band = is_true;

// COUNTRY MULTS PER MODE
    if (LHS == "COUNTRY MULTS PER MODE")
      _country_mults_per_mode = is_true;

// CQ AUTO LOCK
    if (LHS == "CQ AUTO LOCK")
      _cq_auto_lock = is_true;

// CQ AUTO RIT
    if (LHS == "CQ AUTO RIT")
      _cq_auto_rit = is_true;

// CW PRIORITY
    if (LHS == "CW PRIORITY")
      _cw_priority = from_string<int>(RHS);

// CW SPEED
    if (LHS == "CW SPEED")
      _cw_speed = from_string<unsigned int>(RHS);

// CW SPEED CHANGE
    if (LHS == "CW SPEED CHANGE")
      _cw_speed_change = from_string<unsigned int>(RHS);

// DECIMAL POINT
    if (LHS == "DECIMAL POINT")
      _decimal_point = rhs;

// DISPLAY COMMUNICATION ERRORS
    if (LHS == "DISPLAY COMMUNICATION ERRORS")
      _display_communication_errors = is_true;

// DISPLAY GRID
    if (LHS == "DISPLAY GRID")
      _display_grid = is_true;

// DO NOT SHOW
    if (LHS == "DO NOT SHOW")
      _do_not_show = remove_peripheral_spaces(split_string(RHS, ","));

// DO NOT SHOW FILE
    if (LHS == "DO NOT SHOW FILE")
      _do_not_show_filename = rhs;

// DRMASTER FILENAME
    if (LHS == "DRMASTER FILENAME")
      _drmaster_filename = rhs;

// EXCHANGE
    if (LHS == "EXCHANGE")
      _exchange = RHS;

// EXCHANGE[
    if (starts_with(testline, "EXCHANGE["))
    { const string country_list = delimited_substring(LHS, '[', ']');
      const vector<string> countries = remove_peripheral_spaces(split_string(country_list, ','));

      FOR_ALL(countries, [&] (const string& str) { _exchange_per_country.insert( { str, RHS } ); } );
    }

// EXCHANGE CQ
    if (LHS == "EXCHANGE CQ")
      _exchange_cq = RHS;

// EXCHANGE FIELDS FILENAME (for all the regex-based exchange fields)
    if (LHS == "EXCHANGE FIELDS FILENAME")
      _exchange_fields_filename = rhs;

// EXCHANGE MULTS
    if (LHS == "EXCHANGE MULTS")
    { _exchange_mults = RHS;

      if (contains(_exchange_mults, ","))       // if there is more than one exchange mult
        QSO_MULT_WIDTH = 4;                     // make them all the same width, so that the log line looks OK
    }

// EXCHANGE MULTS PER BAND
    if (LHS == "EXCHANGE MULTS PER BAND")
      _exchange_mults_per_band = is_true;

// EXCHANGE MULTS PER MODE
    if (LHS == "EXCHANGE MULTS PER MODE")
      _exchange_mults_per_mode = is_true;

// EXCHANGE PREFILL FILE
//   exchange prefill file = [ exchange-field-name, filename ]
    if ( (LHS == "EXCHANGE PREFILL FILE") or (LHS == "EXCHANGE PREFILL FILES") )
    { const vector<string> files = remove_peripheral_spaces(delimited_substrings(rhs, '[', ']'));

      for (const auto& file : files)
      { const vector<string> fields = remove_peripheral_spaces(split_string(file, ","));

        if (fields.size() == 2)
          _exchange_prefill_files[to_upper(fields[0])] = fields[1];
      }
    }

// EXCHANGE SAP
    if (LHS == "EXCHANGE SAP")
      _exchange_sap = RHS;

// EXCHANGE SENT
// e.g., exchange sent = RST:599, CQZONE:4
    if (LHS == "EXCHANGE SENT")
    { const string comma_delimited_list = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));    // RST:599, CQZONE:4
      const vector<string> fields = split_string(comma_delimited_list, ",");

      for (const auto& this_field : fields)
      { const vector<string> field = split_string(this_field, ":");

        _sent_exchange.push_back( { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) } );
      }
    }

// EXCHANGE SENT CW
    if (LHS == "EXCHANGE SENT CW")
    { const string comma_delimited_list = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));    // RST:599, CQZONE:4
      const vector<string> fields = split_string(comma_delimited_list, ",");

      for (const auto& this_field : fields)
      { const vector<string> field = split_string(this_field, ":");

        if (fields.size() != 2)
          print_error_and_exit(testline);

        _sent_exchange_cw.push_back( { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) } );
      }
    }

// EXCHANGE SENT SSB
    if (LHS == "EXCHANGE SENT SSB")
    { const string comma_delimited_list = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));    // RST:599, CQZONE:4
      const vector<string> fields = split_string(comma_delimited_list, ",");

      for (const auto& this_field : fields)
      { const vector<string> field = split_string(this_field, ":");

        if (fields.size() != 2)
          print_error_and_exit(testline);

        _sent_exchange_ssb.push_back( { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) } );
      }
    }

// FAST CQ BANDWIDTH; used only in CW mode
    if (LHS == "FAST CQ BANDWIDTH")
      _fast_cq_bandwidth = from_string<decltype(_fast_cq_bandwidth)>(RHS);

// FAST SAP BANDWIDTH; used only in CW mode
    if (LHS == "FAST SAP BANDWIDTH")
      _fast_sap_bandwidth = from_string<decltype(_fast_sap_bandwidth)>(RHS);

// HOME EXCHANGE WINDOW
    if (LHS == "HOME EXCHANGE WINDOW")
      _home_exchange_window = is_true;

// INDIVIDUAL MESSAGES FILE
    if (LHS == "INDIVIDUAL MESSAGES FILE")
      _individual_messages_file = rhs;

// KEYER PORT
    if (LHS == "KEYER PORT")
      _keyer_port = rhs;

// LOG
    if ( (LHS == "LOG") and !rhs.empty() )
      _logfile = rhs;

// LONG T
    if (LHS == "LONG T")
      _long_t = is_true;

// MARK FREQUENCIES [CW|SSB]
    if (starts_with(testline, "MARK FREQUENCIES") and !rhs.empty())
    { const vector<string> ranges = remove_peripheral_spaces(split_string(rhs, ","));

      vector<pair<frequency, frequency>> frequencies;

      for (const string& range : ranges)
      { const vector<string> bounds = remove_peripheral_spaces(split_string(range, "-"));

        try
        { frequencies.push_back( { frequency(bounds.at(0)), frequency(bounds.at(1))} );
        }

        catch (...)
        { print_error_and_exit(testline);
        }
      }

      if (LHS == "MARK FREQUENCIES" or LHS == "MARK FREQUENCIES CW")
        _mark_frequencies.insert( { MODE_CW, frequencies } );

      if (LHS == "MARK FREQUENCIES" or LHS == "MARK FREQUENCIES SSB")
        _mark_frequencies.insert( { MODE_SSB, frequencies } );
    }

// MATCH MINIMUM
    if (LHS == "MATCH MINIMUM")
      _match_minimum = from_string<int>(RHS);

// MATCH MINIMUM
    if (LHS == "MAX QSOS WITHOUT QSL")
      _max_qsos_without_qsl = from_string<int>(RHS);

// MODES
    if (LHS == "MODES")
    { _modes = RHS;

      if (contains(_modes, ","))        // if more than one mode
        _mark_mode_break_points = true;
      else
      { if (_modes == "SSB")
          _start_mode = MODE_SSB;
      }
    }

// MODE BREAK POINTS
    if (LHS == "MODE BREAK POINTS")
    { const vector<string> break_points = remove_peripheral_spaces(split_string(RHS, ','));

      for (const auto& break_point : break_points)
      { const frequency f(break_point);
        const BAND b = to_BAND(f);

        _mode_break_points.insert( { b, f } );
      }
    }

// MY CALL
    if (LHS == "MY CALL")
      _my_call = RHS;

// MY CONTINENT
    if (LHS == "MY CONTINENT")
      _my_continent = RHS;

// MY CQ ZONE
    if (LHS == "MY CQ ZONE")
      _my_cq_zone = from_string<int>(RHS);

// MY GRID
    if (LHS == "MY GRID")
      _my_grid = rhs;

// MY IP
    if (LHS == "MY IP")
      _my_ip = rhs;

// MY LATITUDE
    if (LHS == "MY LATITUDE")
      _my_latitude = from_string<float>(rhs);

// MY LATITUDE
    if (LHS == "MY LONGITUDE")
      _my_longitude = from_string<float>(rhs);

// MY ITU ZONE
    if (LHS == "MY ITU ZONE")
      _my_itu_zone = from_string<int>(RHS);

// NEARBY EXTRACT
    if (LHS == "NEARBY EXTRACT")
      _nearby_extract = is_true;

// NO DEFAULT RST
    if (LHS == "NO DEFAULT RST")
      _no_default_rst = is_true;

// NORMALISE RATE(S)
    if ( (LHS == "NORMALISE RATE") or (LHS == "NORMALIZE RATE") or (LHS == "NORMALISE RATES") or (LHS == "NORMALIZE RATES") )
      _normalise_rate = is_true;

// NOT COUNTRY MULTS
    if (LHS == "NOT COUNTRY MULTS")
    { if (!rhs.empty())
        _not_country_mults = rhs;
    }

// OLD ADIF LOG NAME
    if (LHS == "OLD ADIF LOG NAME")
      _old_adif_log_name = rhs;

// PATH
    if (LHS == "PATH")
    { if (!rhs.empty())
        _path = remove_peripheral_spaces(split_string(rhs, ";"));
    }

// POINTS
// don't use LHS here because the command might be somthing like "POINTS[80] ="
    if (starts_with(testline, "POINTS") and !starts_with(testline, "POINTS CW") and !starts_with(testline, "POINTS SSB"))  // there may be an "=" in the points definitions
    { _set_points(testline, MODE_CW);
      _set_points(testline, MODE_SSB);
    }

// POINTS CW
    if (starts_with(testline, "POINTS CW"))
      _set_points(testline, MODE_CW);

// POINTS SSB
    if (starts_with(testline, "POINTS SSB"))
      _set_points(testline, MODE_SSB);

// PTT DELAY (0 => no PTT)
    if (LHS == "PTT DELAY")
      _ptt_delay = from_string<unsigned int>(RHS);

// POST MONITOR
    if (LHS == "POST MONITOR")
    { const vector<string> calls = remove_peripheral_spaces(split_string(RHS, ','));

      FOR_ALL(calls, [&] (const string& callsign) { _post_monitor_calls.insert(callsign); } );
    }

// P3
    if (LHS == "P3")
      _p3 = is_true;

// P3 IGNORE CHECKSUM ERROR
    if (LHS == "P3 IGNORE CHECKSUM ERROR")
      _p3_ignore_checksum_error = is_true;

// P3 SNAPSHOT FILE
    if (LHS == "P3 SNAPSHOT FILE")
      _p3_snapshot_file = rhs;

// P3 SPAN CQ
    if (LHS == "P3 SPAN CQ")
      _p3_span_cq = from_string<unsigned int>(RHS);

// P3 SPAN SAP
    if (LHS == "P3 SPAN SAP")
      _p3_span_sap = from_string<unsigned int>(RHS);

// QSL MESSAGE
    if (LHS == "QSL MESSAGE")
      _qsl_message = RHS;

// QSO MULTIPLE BANDS
    if (LHS == "QSO MULTIPLE BANDS")
      _qso_multiple_bands = is_true;

// QSO MULTIPLE MODES
    if (LHS == "QSO MULTIPLE MODES")
      _qso_multiple_modes = is_true;

// QTCS
    if (LHS == "QTCS")
      _qtcs = is_true;

// QTC DOUBLE SPACE
    if (LHS == "QTC DOUBLE SPACE")
      _qtc_double_space = is_true;

// QTC FILENAME
    if (LHS == "QTC FILENAME")
      _qtc_filename = rhs;

// QTC QRS
    if (LHS == "QTC QRS")
      _qtc_qrs = from_string<unsigned int>(rhs);

// QTHX: QTHX[callsign-or-canonical prefix] = aa, bb, cc...
// the conversion to canonical prefix occurs later, inside contest_rules::_parse_context_qthx()
    if (starts_with(testline, "QTHX["))
    { const vector<string> fields = remove_peripheral_spaces(split_string(testline, "="));

      if (fields.size() == 2)
      { const string canonical_prefix = delimited_substring(fields[0], '[', ']');
        const vector<string> values = remove_peripheral_spaces(split_string(RHS, ","));
        const set<string> ss(values.cbegin(), values.cend());

        _qthx.insert( { canonical_prefix, ss } );
      }
    }

// RATE
    if (LHS == "RATE")
    { const vector<string> vec_rates = remove_peripheral_spaces(split_string(rhs, ","));
      vector<unsigned int> new_rates;

      FOR_ALL(vec_rates, [&new_rates] (const string& str) { new_rates.push_back(from_string<unsigned int>(str)); } );

      if (!new_rates.empty())
        _rate_periods = new_rates;
    }

// RBN BEACONS
    if (LHS == "RBN BEACONS")
      _rbn_beacons = is_true;

// RBN PORT
    if (LHS == "RBN PORT")
      _rbn_port = from_string<int>(rhs);

// RBN SERVER
    if (LHS == "RBN SERVER")
      _rbn_server = rhs;

// RBN THRESHOLD
    if (LHS == "RBN THRESHOLD")
      _rbn_threshold = from_string<unsigned int>(rhs);

// RBN USERNAME
    if (LHS == "RBN USERNAME")
      _rbn_username = rhs;

// REJECT COLOUR
    if ( ( (LHS == "REJECT COLOUR") or (LHS == "REJECT COLOR") ) and !rhs.empty() )
      _reject_colour = string_to_colour(RHS);

// REQUIRE DOT IN REPLACEMENT CALL
    if (LHS == "REQUIRE DOT IN REPLACEMENT CALL")
      _require_dot_in_replacement_call = is_true;

// RIG 1 BAUD
    if ( (LHS == "RIG 1 BAUD") or (LHS == "RIG BAUD") )
      _rig1_baud = from_string<unsigned int>(rhs);

// RIG 1 DATA BITS
    if ( (LHS == "RIG 1 DATA BITS") or (LHS == "RIG DATA BITS") )
      _rig1_data_bits = from_string<unsigned int>(rhs);

// RIG 1 NAME
    if ( (LHS == "RIG 1 NAME") or (LHS == "RADIO ONE NAME") )
      _rig1_name = rhs;

// RIG 1 PORT
    if ( (LHS == "RIG 1 PORT") or (LHS == "RADIO ONE CONTROL PORT") )
      _rig1_port = rhs;

// RIG 1 STOP BITS
    if ( (LHS == "RIG 1 STOP BITS") or (LHS == "RIG STOP BITS") )
      _rig1_stop_bits = from_string<unsigned int>(rhs);

// RIG 1 TYPE
    if (LHS == "RIG 1 TYPE")
      _rig1_type = RHS;

// RULES
    if (LHS == "RULES")
      _process_configuration_file(rhs);

// RUSSIAN DATA
    if (LHS == "RUSSIAN DATA")
      _russian_filename = rhs;

// SCORE BANDS
    if (starts_with(testline, "SCORE BANDS"))
    { const vector<string> bands_str = remove_peripheral_spaces(split_string(rhs, ","));

      for (const auto& band_str : bands_str)
      { try
        { _score_bands.insert(BAND_FROM_NAME.at(band_str));
        }

        catch (...)
        { }
      }
    }

// SCORE MODES
    if (starts_with(testline, "SCORE MODES"))
    { if (contains(testline, "CW"))
        _score_modes.insert(MODE_CW);

      if (contains(testline, "SSB") or contains(testline, "PH"))
        _score_modes.insert(MODE_SSB);
    }

// SCREEN SNAPSHOT FILE
    if ( (LHS == "SCREEN SNAPSHOT FILE") or (LHS == "SCREENSHOT FILE") )
      _screen_snapshot_file = rhs;

// SCREEN SNAPSHOT ON EXIT
    if ( (LHS == "SCREEN SNAPSHOT ON EXIT") or (LHS == "SCREENSHOT ON EXIT") )
      _screen_snapshot_on_exit = is_true;

// SERIAL NUMBER SPACES
    if (LHS == "SERIAL NUMBER SPACES")
      _serno_spaces = from_string<unsigned int>(rhs);

// SHIFT DELTA
    if (LHS == "SHIFT DELTA")
      _shift_delta = from_string<unsigned int>(rhs);

// SHIFT POLL
    if (LHS == "SHIFT POLL")
      _shift_poll = from_string<unsigned int>(rhs);

// SHORT SERIAL NUMBER
    if (LHS == "SHORT SERIAL NUMBER")
      _short_serno = is_true;

// SOCIETY LIST FILENAME
    if (LHS == "SOCIETY LIST FILENAME")
      _society_list_filename = rhs;

// START AUDIO RECORDING
    if (LHS == "START AUDIO RECORDING")
      _start_audio_recording = is_true;

// START BAND
    if (LHS == "START BAND")
    { const auto cit = BAND_FROM_NAME.find(RHS);

      if (cit != BAND_FROM_NAME.cend())
        _start_band = cit->second;
    }

// START MODE
    if (LHS == "START MODE")
    { if (RHS == "SSB")
        _start_mode = MODE_SSB;
    }

// SYNC KEYER
    if (LHS == "SYNC KEYER")
      _sync_keyer = is_true;

// TEST
    if (LHS == "TEST")
      _test = is_true;

// THOUSANDS SEPARATOR
    if (LHS == "THOUSANDS SEPARATOR")
      _thousands_separator = rhs;

// UBA BONUS
    if (LHS == "UBA BONUS")
      _uba_bonus = is_true;

// WORKED MULTS COLOUR
    if ( (LHS == "WORKED MULTS COLOUR") or (LHS == "WORKED MULTS COLOR") )
      _worked_mults_colour = RHS;

// ---------------------------------------------  MULTIPLIERS  ---------------------------------


// COUNTRY MULTS
// Currently supported: ALL
//                      NONE
// any single continent
    if (LHS == "COUNTRY MULTS")
    { _country_mults_filter = RHS;

      if (_country_mults_filter == "NONE")
        QSO_DISPLAY_COUNTRY_MULT = false;                  // since countries aren't mults, don't take up space in the log line
    }

// REMAINING CALLSIGN MULTS
    if (LHS == "REMAINING CALLSIGN MULTS")
    { _auto_remaining_callsign_mults = (RHS == "AUTO");

      if (_auto_remaining_callsign_mults)
      { const vector<string> tokens = split_string(RHS, " ");

        if (tokens.size() == 2)
          _auto_remaining_callsign_mults_threshold = from_string<unsigned int>(tokens[1]);
      }
      else
      { const vector<string> mults = remove_peripheral_spaces(split_string(RHS, ","));

        _remaining_callsign_mults_list = set<string>(mults.cbegin(), mults.cend());
      }
    }

// REMAINING COUNTRY MULTS
    if (LHS == "REMAINING COUNTRY MULTS")
    { _auto_remaining_country_mults = contains(RHS, "AUTO");

      if (_auto_remaining_country_mults)
      { const vector<string> tokens = split_string(RHS, " ");

        if (tokens.size() == 2)
          _auto_remaining_country_mults_threshold = from_string<unsigned int>(tokens[1]);
      }
      else
      { const vector<string> countries = remove_peripheral_spaces(split_string(RHS, ","));

        _remaining_country_mults_list = set<string>(countries.cbegin(), countries.cend());
      }
    }

// AUTO REMAINING EXCHANGE MULTS (the exchange mults whose list of legal values can be augmented)
    if (LHS == "AUTO REMAINING EXCHANGE MULTS")
    { const vector<string> mult_names = remove_peripheral_spaces(split_string(RHS, ","));

      for (const auto& str : mult_names)
        _auto_remaining_exchange_mults.insert(str);
    }

// ---------------------------------------------  CABRILLO  ---------------------------------

// CABRILLO CONTEST
    if (LHS == "CABRILLO CONTEST")
      _cabrillo_contest = RHS;          // required to be upper case; don't limit to legal values defined in the "specification", since many contest require an illegal value

    if ( (LHS == "CABRILLO CERTIFICATE") and is_legal_value(RHS, "YES,NO", ",") )
      _cabrillo_certificate = RHS;

 // CABRILLO EMAIL (sic)
    if ( (LHS == "CABRILLO E-MAIL") or (LHS == "CABRILLO EMAIL") )
      _cabrillo_e_mail = rhs;

// CABRILLO EOL
    if (LHS == "CABRILLO EOL")
      _cabrillo_eol = RHS;

// CABRILLO INCLUDE SCORE
    if (LHS == "CABRILLO INCLUDE SCORE")
      _cabrillo_include_score = is_true;

// CABRILLO LOCATION
    if (LHS == "CABRILLO LOCATION")
      _cabrillo_location = rhs;

// CABRILLO NAME
    if (LHS == "CABRILLO NAME")
      _cabrillo_name = rhs;

// CABRILLO CATEGORY-ASSISTED
    if ( (LHS == "CABRILLO CATEGORY-ASSISTED") and is_legal_value(RHS, "ASSISTED,NON-ASSISTED", ",") )
      _cabrillo_category_assisted = RHS;

// CABRILLO CATEGORY-BAND
    if (LHS == "CABRILLO CATEGORY-BAND")
    {
// The spec calls for bizarre capitalization
      if (is_legal_value(rhs, "ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light", ","))
        _cabrillo_category_band = rhs;
    }

// CABRILLO CATEGORY-MODE
    if (LHS == "CABRILLO CATEGORY-MODE")
    { if (is_legal_value(RHS, "CW,MIXED,RTTY,SSB", ","))
        _cabrillo_category_mode = RHS;
    }

// CABRILLO CATEGORY-OPERATOR
    if (LHS == "CABRILLO CATEGORY-OPERATOR")
    { if (is_legal_value(RHS, "CHECKLOG,MULTI-OP,SINGLE-OP", ","))
        _cabrillo_category_operator = RHS;
    }

// CABRILLO CATEGORY-OVERLAY
    if (LHS == "CABRILLO CATEGORY-OVERLAY")
    { if (is_legal_value(RHS, "NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES", ","))
        _cabrillo_category_overlay = RHS;
    }

// CABRILLO CATEGORY-POWER
    if (LHS == "CABRILLO CATEGORY-POWER")
    { if (is_legal_value(RHS, "HIGH,LOW,QRP", ","))
        _cabrillo_category_power = RHS;
    }

// CABRILLO CATEGORY-STATION
    if (LHS == "CABRILLO CATEGORY-STATION")
    { if (is_legal_value(RHS, "EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL", ","))
        _cabrillo_category_station = RHS;
    }

// CABRILLO CATEGORY-TIME
    if (LHS == "CABRILLO CATEGORY-TIME")
    { if (is_legal_value(RHS, "6-HOURS,12-HOURS,24-HOURS", ","))
        _cabrillo_category_station = RHS;
    }

// CABRILLO CATEGORY-TRANSMITTER
    if (LHS == "CABRILLO CATEGORY-TRANSMITTER")
    { if (is_legal_value(RHS, "LIMITED,ONE,SWL,TWO,UNLIMITED", ","))
        _cabrillo_category_transmitter = RHS;
    }

// CABRILLO CLUB
    if (LHS == "CABRILLO CLUB")
      _cabrillo_club = RHS;

// CABRILLO ADDRESS first line
    if (LHS == "CABRILLO ADDRESS 1")
      _cabrillo_address_1 = rhs;

// CABRILLO ADDRESS second line
    if (LHS == "CABRILLO ADDRESS 2")
      _cabrillo_address_2 = rhs;

// CABRILLO ADDRESS third line
    if (LHS == "CABRILLO ADDRESS 3")
      _cabrillo_address_3 = rhs;

// CABRILLO ADDRESS fourth line
    if (starts_with(testline, "CABRILLO ADDRESS 4"))
      _cabrillo_address_4 = remove_peripheral_spaces((split_string(line, "="))[1]);

// CABRILLO ADDRESS-CITY
    if (starts_with(testline, "CABRILLO ADDRESS-CITY"))
      _cabrillo_address_city = remove_peripheral_spaces((split_string(line, "="))[1]);

// CABRILLO ADDRESS-STATE-PROVINCE
    if (starts_with(testline, "CABRILLO ADDRESS-STATE-PROVINCE"))
      _cabrillo_address_state_province = remove_peripheral_spaces((split_string(line, "="))[1]);

// CABRILLO ADDRESS-POSTALCODE
    if (starts_with(testline, "CABRILLO ADDRESS-POSTALCODE"))
      _cabrillo_address_postalcode = remove_peripheral_spaces((split_string(line, "="))[1]);

// CABRILLO ADDRESS-COUNTRY
    if (starts_with(testline, "CABRILLO ADDRESS-COUNTRY"))
      _cabrillo_address_country = remove_peripheral_spaces((split_string(line, "="))[1]);

// CABRILLO OPERATORS
    if (starts_with(testline, "CABRILLO OPERATORS"))
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

    if (LHS == "CABRILLO QSO")
    { _cabrillo_qso_template = RHS;

      if (contains(RHS, "TEMPLATE"))
      { try
        { const string key = remove_peripheral_spaces(split_string(RHS, ":"))[1];

          _cabrillo_qso_template = cabrillo_qso_templates.at(key);
        }

        catch (...)
        { ost << "Error in CABRILLO QSO TEMPLATE" << endl;
          exit(-1);
        }
      }
    }

// ---------------------------------------------  WINDOWS  ---------------------------------

    if (LHS == "WINDOW")
    { const vector<string> window_info = remove_peripheral_spaces(split_string(split_string(testline, "=")[1], ","));

      if (window_info.size() >= 5)
      { const string name = window_info[0];

        window_information winfo;

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

        _windows.insert(make_pair(name, winfo));
      }
    }    // end of WINDOW

// ---------------------------------------------  STATIC WINDOWS  ---------------------------------

    static map<string /* name */, bool /* whether verbatim */> verbatim;

    if (LHS == "STATIC WINDOW")
    { const vector<string> fields = remove_peripheral_spaces(split_string(rhs, ","));

      if (fields.size() == 2)  // name, contents
      { const string name = fields[0];
        string contents = fields[1];      // might be actual contents, or a fully-qualified filename
        verbatim[name] = contains(fields[1], "\"");     // verbatim if contains quotation mark

        if (file_exists(contents))
          contents = read_file(contents);

        _static_windows[name] = { contents, vector<window_information>() };
      }
    }
//    std::map<std::string /* name */, std::pair<std::string /* contents */, std::vector<window_information> > > _static_windows;

    if (LHS == "STATIC WINDOW INFO")  // must come after the corresponding STATIC WINDOW command
    { const vector<string> window_info = remove_peripheral_spaces(split_string(split_string(testline, "=")[1], ","));

      if (!window_info.empty())
      { const string name = window_info[0];

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
            { string contents = _static_windows[name].first;

              if (contents.size() >= 2)
                contents = delimited_substring(contents, '"', '"');

              vector<string> lines = to_lines(contents);
              const string contents_1 = replace(contents, "\\n", EOL);

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
            { const string contents = _static_windows[name].first;
              const vector<string> lines = to_lines(contents);

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

            vector<window_information> vec = _static_windows[name].second;

            vec.push_back(winfo);
            _static_windows[name] = { final_contents, vec };
          }
        }
      }
    }

// ---------------------------------------------  MESSAGES  ---------------------------------

    if (starts_with(testline, "MESSAGE KEY"))
    { vector<string> message_info = split_string(testline, " ");

      if (message_info.size() >= 5 and contains(testline, "="))
      {
// a big switch to convert from text in the configuration file to a KeySym, which is what we use as the key in the message map
// this could be more efficient by generating a container of all the legal keysym names and then comparing the target to all those.
// The latter is now what we do. The legal names are in key_names.
        const string target = to_lower(message_info[2]);
        const map<string, int>::const_iterator cit = key_names.find(target);

        if (cit != key_names.cend())
        { const vector<string> vec_str = split_string(testline, "=");
          const string str = remove_leading_spaces(vec_str.at(1));

// everything to the right of the = -- we assume there's only one -- goes into the message, excepting any leading space
          _messages.insert( { cit->second, str } );

          ost << "message associated with " << target << ", which is keysym " << hex << cit->second << dec << ", is: " << str << endl;

          const map<string, string>::const_iterator cit2 = equivalent_key_names.find(target);

          if (cit2 != equivalent_key_names.cend())
          { ost << "found equivalent key name: " << cit2->second << endl;

            const string alternative = cit2->second;
            const map<string, int>::const_iterator cit = key_names.find(alternative);

            if (cit != key_names.cend())
            { const int keysym = cit->second;

              if (_messages.find(keysym) == _messages.cend())  // only if there is no message for this key
              {  ost << "message associated with equivalent key is: " << str << endl;

                _messages.insert( { keysym, str } );
              }
            }
          }
        }
      }
    }

    if (LHS == "MESSAGE CQ 1")
    { const vector<string> tokens = split_string(testline, "=");

      if (tokens.size() != 2)
        print_error_and_exit(testline);

      _message_cq_1 = tokens[1];
    }

    if (LHS == "MESSAGE CQ 2")
    { const vector<string> tokens = split_string(testline, "=");

      if (tokens.size() == 2)
        _message_cq_2 = tokens[1];
    }
  }

// set some new defaults if we haven't explicitly set them
  if (_cabrillo_contest == string())
    _cabrillo_contest = _contest_name;

  if (_cabrillo_callsign == string())
    _cabrillo_callsign = _my_call;

  if (_qsl_message.empty())
    _qsl_message = "tu " + _my_call + " test";

  if (_alternative_qsl_message.empty())
    _alternative_qsl_message = "tu " + _my_call;

  if (_message_cq_1.empty())
    _message_cq_1 = "test " + _my_call + " " + _my_call + " test";

  if (_message_cq_2.empty())
    _message_cq_2 = "cq cq test de  " + _my_call + "  " + _my_call + "  " + _my_call + "  test";

// possibly fix Cabrillo template
  if ( (_cabrillo_qso_template == "ARRL DX") or (_cabrillo_qso_template == "CQ WW") )
  { const vector<string> actual_modes = remove_peripheral_spaces(split_string(_modes, ","));

    if (actual_modes.size() == 1)
    { try
      { if (set<string>( { "ARRL DX", "CQ WW", "JIDX"} ) < _cabrillo_qso_template)
        {  const string key = _cabrillo_qso_template + ( (actual_modes[0] == "CW") ?  " CW" : " SSB");

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
drlog_context::drlog_context(const std::string& filename) :
  _accept_colour(COLOUR_GREEN),                                     // green for calls that are OK to work
  _allow_audio_recording(false),                                    // do not allow audio recording
//  _allow_variable_sent_rst(false),                                  // do not allow the sent RST to vary
  _alternative_qsl_message(),                                       // no alternative QSL message (default is changed once configuration file has been read)
  _archive_name("drlog-restart"),                                   // name for the archive written when leaving drlog
  _audio_channels(1),                                               // monophonic
  _audio_device_name("default"),                                    // default audio device
  _audio_duration(60),                                              // record 60 minutes per file
  _audio_file("audio"),                                             // audio will be in audio-yymmdd-hhmmss
  _audio_rate(8000),                                                // 8,000 samples per second
  _auto_backup(""),                                                 // no auto backup directory
  _auto_remaining_callsign_mults_threshold(1),                      // a callsign mult must be seen only once before it becomes known
  _auto_remaining_country_mults(false),                             // do not add country mults as we detect them
  _auto_remaining_country_mults_threshold(1),                       // a canonical prefix must be seen only once before it becomes known
  _auto_remaining_exchange_mults( { } ),                            // do not add any exchange mults as we detect them
  _auto_screenshot(false),                                          // do not generate horal screenshots
  _bandmap_decay_time_local(60),                                    // stay on bandmap for one hour
  _bandmap_decay_time_cluster(60),                                  // stay on bandmap for one hour
  _bandmap_decay_time_rbn(60),                                      // stay on bandmap for one hour
  _bandmap_fade_colours( { 255, 200, 150, 100 } ),                  // four colour steps
  _bandmap_filter(),                                                // empty bandmap filter
  _bandmap_filter_disabled_colour(string_to_colour("BLACK")),       // background colour when bandmap filter is disabled
  _bandmap_filter_enabled(false),                                   // do not filter bandmap
  _bandmap_filter_foreground_colour(string_to_colour("WHITE")),     // letters are white
  _bandmap_filter_hide_colour(string_to_colour("RED")),             // background colour when bandmap filter is in HIDE mode
  _bandmap_filter_show(false),                                      // filter is used to hide entrie
  _bandmap_filter_show_colour(string_to_colour("GREEN")),           // background colour when bandmap filter is in SHOW mode
  _bandmap_recent_colour(string_to_colour("BLACK")),                // do not indicate recent postings with special colour
  _bands("160, 80, 40, 20, 15, 10"),                                // legal bands for the contest
  _batch_messages_file(),                                           // name for batch messages file
  _best_dx_unit("MILES"),                                           // DX is measured in miles
  _cabrillo_address_1(),                                            // first line of Cabrillo ADDRESS
  _cabrillo_address_2(),                                            // second line of Cabrillo ADDRESS
  _cabrillo_address_3(),                                            // third line of Cabrillo ADDRESS
  _cabrillo_address_4(),                                            // fourth line of Cabrillo ADDRESS
  _cabrillo_address_city(),                                         // CITY in Cabrillo file
  _cabrillo_address_country(),                                      // COUNTRY in Cabrillo file
  _cabrillo_address_postalcode(),                                   // POSTALCODE in Cabrillo file
  _cabrillo_address_state_province(),                               // PROVINCE in Cabrillo file
  _cabrillo_callsign(),                                             // CALLSIGN in Cabrillo file
  _cabrillo_category_assisted("ASSISTED"),                          // default is ASSISTED
  _cabrillo_category_band("ALL"),                                   // default is all bands
  _cabrillo_category_mode(),                                        // no default, must be filled in by the config file
  _cabrillo_category_operator("SINGLE-OP"),                         // default is single op
  _cabrillo_category_overlay(),                                     // no overlay
  _cabrillo_category_power("HIGH"),                                 // default is high power
  _cabrillo_category_station(),                                     // no default station category
  _cabrillo_category_time(),                                        // no default time category
  _cabrillo_category_transmitter("ONE"),                            // one transmitter
  _cabrillo_certificate("YES"),                                     // explicitly request a certificate, because of inanity in the specification
  _cabrillo_club(),                                                 // no club
  _cabrillo_contest(),                                              // CONTEST in Cabrillo file
  _cabrillo_eol("LF"),                                              // use LF as EOL in the Cabrillo file
  _cabrillo_e_mail(),                                               // no e-mail address
  _cabrillo_filename("cabrillo"),                                   // name of file that will store Cabrillo log
  _cabrillo_include_score(true),                                    // include score in the Cabrillo file
  _cabrillo_location(),                                             // LOCATION in Cabrillo file
  _cabrillo_name(),                                                 // NAME in Cabrillo file
  _cabrillo_qso_template(),                                         // empty QSO template; this must be properly defined if Cabrillo is used
  _callsign_mults(),                                                // callsign is not the basis for any mults
  _callsign_mults_per_band(false),                                  // any callsign mults are once-only
  _callsign_mults_per_mode(false),                                  // any callsign mults are once-only
  _cluster_port(23),                                                // standard telnet server port
  _country_list(COUNTRY_LIST_WAEDC),                                // WAE country list
  _country_mults_filter("ALL"),                                     // all countries are mults
  _country_mults_per_band(true),                                    // any country mults are per-band
  _country_mults_per_mode(false),                                   // any country mults are not per-mode
  _cq_auto_lock(false),                                             // don't lock the TX frequency in CQ mode
  _cq_auto_rit(false),                                              // don't enable RIT in CQ mode
  _cty_filename("cty.dat"),                                         // filename for country data
  _cw_priority(-1),                                                 // non-RT scheduling
  _cw_speed(29),                                                    // 29 WPM
  _cw_speed_change(3),                                              // change speed by 3 WPM
  _decimal_point("·"),                                              // use centred dot as decimal point
  _display_communication_errors(true),                              // display errors communicating with rig
  _display_grid(false),                                             // do not display grid info in GRID and INFO windows
  _do_not_show(),                                                   // all calls (apart from my_call()) should be shown on the bandmap
  _do_not_show_filename(),                                          // no do-not-show file
  _drmaster_filename("drmaster"),                                   // name of the drmaster file
  _exchange("RST"),                                                 // exchange is just RST (or RS)
  _exchange_fields_filename(),                                      // file that holds regex templates for exchange fields
  _exchange_mults_per_band(false),                                  // any exchange mults are once-only
  _exchange_mults_per_mode(false),                                  // any exchange mults are once-only
  _fast_cq_bandwidth(400),                                          // fast CW bandwidth in CQ mode, in Hz
  _fast_sap_bandwidth(400),                                         // fast CW bandwidth in SAP mode, in Hz
  _guard_band( { { MODE_CW, 500 }, { MODE_SSB, 2000 } } ),          // 500 Hz guard band on CW, 2 kHz on slopbucket
  _home_exchange_window(false),                                     // do not move cursor to left of window (and insert space if necessary)
  _individual_messages_file(),                                      // no file of individual QSL messages
  _logfile("drlog.dat"),                                            // name of log file
  _long_t(false),                                                   // do not extend initial Ts in serial numbers
  _mark_frequencies(),                                              // don't mark any frequencies
  _mark_mode_break_points(false),                                   // do not mark the mode break points on the bandmap
  _match_minimum(4),                                                // 4 characters required for SCP or fuzzy match
  _max_qsos_without_qsl(4),                                         // for the N7DR matches_criteria algorithm
  _message_cq_1(),                                                  // no short CQ (default is changed once configuration file has been read)
  _message_cq_2(),                                                  // no long CQ (default is changed once configuration file has been read)
  _modes("CW"),                                                     // only valid mode is CW
  _my_call("NONE"),                                                 // set callsign to an invalid value
  _my_continent("XX"),                                              // set continent to an invalid value
  _my_latitude(0),                                                  // at the equator
  _my_longitude(0),                                                 // Greenwich meridian
  _nearby_extract(false),                                           // do not display NEARBY calls in the EXTRACT window
  _normalise_rate(false),                                           // do not normalise rates to one-hour values
  _not_country_mults(),                                             // no countries are explicitly not country mults
  _no_default_rst(false),                                           // we do assign a default RST
  _old_adif_log_name(),                                             // no ADIF log of old QSOs
  _path( { "." } ),                                                 // search only the current directory
  _per_band_points( {} ),                                           // no points awarded anywhere
  _post_monitor_calls( {} ),                                        // no calls are monitored
  _ptt_delay(25),                                                   // 25ms PTT delay
  _p3(false),                                                       // no P3 is available
  _p3_ignore_checksum_error(false),                                 // don't ignore checksum errors when acquiring P3 screendumps
  _p3_snapshot_file("P3"),                                          // P3 snapshots will be in P3-<n>
  _p3_span_cq(0),                                                   // no default span in CQ mode
  _p3_span_sap(0),                                                  // no default span in SAP mode
  _qsl_message(),                                                   // no QSL message (default is changed once configuration file has been read)
  _qso_multiple_bands(false),                                       // each station may be worked on only one band
  _qso_multiple_modes(false),                                       // each station may be worked on only one mode
  _qtcs(false),                                                     // QTCs are disabled
  _qtc_double_space(false),                                         // QTC elements are single-spaced
  _qtc_filename("QTCs"),                                            // QTC filename
  _qtc_qrs(0),                                                      // no speed decrease when sending QTCs
  _rate_periods( { 15, 30, 60 } ),                                  // 15-, 30-, 60-minute periods for rates
  _rbn_beacons(false),                                              // do not place RBN posts from beacons on bandmap
  _rbn_port(7000),                                                  // telnet port for the reverse beacon network
  _rbn_server("telnet.reversebeacon.net"),                          // domain name of the reverse beacon network telnet server
  _rbn_threshold(1),                                                // all received spots are posted
  _rbn_username(""),                                                // no default name to access the RBN
  _reject_colour(COLOUR_RED),                                       // red for dupes
  _remaining_country_mults_list(),                                  // no remaining country mults
  _require_dot_in_replacement_call(false),                          // try to parse additional (non-dotted) string as call
  _rig1_baud(4800),                                                 // 4800 baud
  _rig1_data_bits(8),                                               // 8-bit data
  _rig1_port("/dev/ttyS0"),                                         // first serial port
  _rig1_stop_bits(1),                                               // one stop bit
  _rig1_type(""),                                                   // no default rig type
  _russian_filename("russian-data"),                                // default file for Russian location information
  _screen_snapshot_file("screen"),                                  // screen snapshots will be in screen-<n>
  _screen_snapshot_on_exit(false),                                  // do not take a screenshot on exit
  _sent_exchange(),                                                 // no default sent exchange
  _sent_exchange_cw(),                                              // no default sent CW exchange
  _sent_exchange_ssb(),                                             // no default sent SSB exchange
  _serno_spaces(0),                                                 // no additional spaces in serial number
  _shift_delta(10),                                                 // shift RIT by 10 Hz
  _shift_poll(50),                                                  // poll every 50 milliseconds
  _short_serno(false),                                              // send leading Ts
  _society_list_filename(""),                                       // no default file for IARU society information
  _start_audio_recording(false),                                    // do not start audio recording
  _start_band(BAND_20),                                             // start on 20m
  _start_mode(MODE_CW),                                             // start on CW
  _sync_keyer(false),                                               // do not synchronise rig keyer with computer
  _test(false),                                                     // transmit is not disabled
  _thousands_separator(","),                                        // numbers are written with ","
  _uba_bonus(false),                                                // do not add UBA bonus points
  _worked_mults_colour("RED")                                       // worked mults are in red
{ for (unsigned int n = 0; n < NUMBER_OF_BANDS; ++n)
    _per_band_country_mult_factor.insert( { static_cast<BAND>(n), 1 } );

  _process_configuration_file(filename);

// make sure that the default is to score all permitted bands
  if (_score_bands.empty())
  { vector<string> bands_str = remove_peripheral_spaces(split_string(_bands, ","));

    for (const auto& band_name : bands_str)
    { try
      { const BAND b = BAND_FROM_NAME.at(band_name);
        _score_bands.insert(b);
      }

      catch (...)
      { }
    }
  }

// default is to score all permitted modes
  if (_score_modes.empty())
  { if (contains(_modes, "CW"))
      _score_modes.insert(MODE_CW);

    if (contains(_modes, "SSB"))
      _score_modes.insert(MODE_SSB);
  }
}

/*! \brief          Get the information pertaining to a particular window
    \param  name    name of window
    \return         location, size and colour information
*/
const window_information drlog_context::window_info(const string& name) const
{ const auto cit = _windows.find(name);

  return (cit == _windows.cend() ? window_information() : cit->second);
}

/*! \brief          Get all the windows whose name contains a particular substring
    \param  substr  substring for which to search
    \return         all the window names that include <i>substr</i>
*/
const vector<string> drlog_context::window_name_contains(const string& substr) const
{ vector<string> rv;

  for (auto cit = _windows.cbegin(); cit != _windows.cend(); ++cit)
    if (contains(cit->first, substr))
      rv.push_back(cit->first);

  return rv;
}

/*! \brief      Is a particular frequency within any marked range?
    \param  m   mode
    \param  f   frequency to test
    \return     whether <i>f</i> is in any marked range for the mode <i>m</i>
*/
const bool drlog_context::mark_frequency(const MODE m, const frequency& f)
{ SAFELOCK(_context);

  try
  { const vector<pair<frequency, frequency>>& vec = _mark_frequencies.at(m);

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
const string drlog_context::points_string(const BAND b, const MODE m) const
{ SAFELOCK(_context);

  const auto& pbb = _per_band_points[m];

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
const vector<string> drlog_context::sent_exchange_names(void) const
{ vector<string> rv;

  for (const auto& pss : _sent_exchange)
    rv.push_back(pss.first);

  return rv;
}

/*! \brief      Get all the field names in the exchange sent for a particular mode
    \param  m   target mode
    \return     the names of all the fields in the sent exchange for mode <i>m</i>
*/
const vector<string> drlog_context::sent_exchange_names(const MODE m) const
{ vector<string> rv;

  const std::vector<std::pair<std::string, std::string> >* ptr_vec_pss = (m == MODE_CW ? &_sent_exchange_cw : &_sent_exchange_ssb);

  for (const auto& pss : *ptr_vec_pss)
    rv.push_back(pss.first);

  return rv;
}

/*! \brief      Get names and values of sent exchange fields for a particular mode
    \param  m   target mode
    \return     the names and values of all the fields in the sent exchange when the mode is <i>m</i>
*/
const decltype(drlog_context::_sent_exchange) drlog_context::sent_exchange(const MODE m)        // doxygen complains about the decltype; I have no idea why
{ SAFELOCK(_context);

  decltype(_sent_exchange) rv = ( (m == MODE_CW) ? _sent_exchange_cw : _sent_exchange_ssb);

  if (rv.empty())
  { rv = _sent_exchange;

// fix RST/RS if using non-mode-specific exchange
    for (unsigned int n = 0; n < rv.size(); ++n)
    { pair<string, string>& pss = rv[n];

      if ( (m == MODE_SSB) and (pss.first == "RST") )
      { //pss.first = "RS";
        //pss.second = "59";  // hardwire!
        pss = { "RS", "59" };       // hardwire report
      }

      if ( (m == MODE_CW) and (pss.first == "RS") )
      { //pss.first = "RST";
        //pss.second = "599";  // hardwire!
        pss = { "RST", "599" };     // hardwire report
      }
    }
  }

  return rv;
}

