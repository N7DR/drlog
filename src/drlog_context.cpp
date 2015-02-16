// $Id: drlog_context.cpp 95 2015-02-15 22:41:49Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file drlog_context.cpp

        The basic context for operation of drlog
*/

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

// -----------  drlog_context  ----------------

/*! \class drlog_context
    \brief The variables and constants that comprise the context for drlog
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

  if (!str_vec.empty())
  { string tmp_points_str;
    const string lhs = str_vec[0];
    auto& pbb = _per_band_points[m];

    if (!contains(lhs, "[") or contains(lhs, "[*]"))             // for all bands
    { string new_str;

      for (unsigned int n = 1; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
      { new_str += str_vec[n];
        if (n != str_vec.size() - 1)
          new_str += "=";
      }

      tmp_points_str = to_upper(remove_peripheral_spaces(new_str));

      for (unsigned int n = 0; n < NUMBER_OF_BANDS; ++n)
        pbb.insert( {static_cast<BAND>(n), tmp_points_str} );
    }
    else    // not all bands
    { size_t left_bracket_posn = lhs.find('[');
      size_t right_bracket_posn = lhs.find(']');

      const bool valid = (left_bracket_posn != string::npos) and (right_bracket_posn != string::npos) and (left_bracket_posn < right_bracket_posn);

      if (valid)
      { string bands_str = lhs.substr(left_bracket_posn + 1, (right_bracket_posn - left_bracket_posn - 1));
        vector<string> bands = split_string(bands_str, ",");

        for (size_t n = 0; n < bands.size(); ++n)
          bands[n] = remove_peripheral_spaces(bands[n]);

        for (size_t n = 0; n < bands.size(); ++n)
        { int wavelength = from_string<size_t>(bands[n]);
          BAND b;

          switch (wavelength)
          { case 160 :
              b = BAND_160;
              break;
            case 80 :
              b = BAND_80;
              break;
            case 60 :
              b = BAND_60;
              break;
            case 40 :
              b = BAND_40;
              break;
            case 30 :
              b = BAND_30;
              break;
            case 20 :
              b = BAND_20;
              break;
            case 17 :
              b = BAND_17;
              break;
            case 15 :
              b = BAND_15;
              break;
            case 12 :
              b = BAND_12;
              break;
            case 10 :
              b = BAND_10;
              break;
            default :
              continue;
          }

          string new_str;

          for (unsigned int n = 1; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
          { new_str += str_vec[n];

            if (n != str_vec.size() - 1)
              new_str += "=";
          }

          tmp_points_str = to_upper(remove_peripheral_spaces(new_str));

          pbb.insert( {b, tmp_points_str} );
        }
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
  { cerr << "Unable to open configuration file " << filename << endl;
    exit(-1);
  }

  static const string LF_STR   = "\n";      ///< LF as string

  const vector<string> lines = split_string(entire_file, LF_STR);   // split into lines

  for (const auto& line : lines)                                    // process each line
  {
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

// ARCHIVE
    if ( (LHS == "ARCHIVE") and !rhs.empty() )
      _archive_name = rhs;

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
      { vector<string> colours = split_string(RHS, ",");

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
//    if (starts_with(testline, "BAND MAP FILTER MODE") or starts_with(testline, "BANDMAP FILTER MODE"))
    if ( (LHS == "BAND MAP FILTER MODE") or (LHS == "BANDMAP FILTER MODE") )
      _bandmap_filter_show = (RHS == "SHOW");

// BAND MAP GUARD BAND CW
//    if (starts_with(testline, "BAND MAP GUARD BAND CW") or starts_with(testline, "BANDMAP GUARD BAND CW"))
    if ( (LHS == "BAND MAP GUARD BAND CW") or (LHS == "BANDMAP GUARD BAND CW") )
      _guard_band[MODE_CW] = from_string<int>(rhs);

// BAND MAP GUARD BAND SSB
//    if (starts_with(testline, "BAND MAP GUARD BAND SSB") or starts_with(testline, "BANDMAP GUARD BAND SSB"))
    if ( (LHS == "BAND MAP GUARD BAND SSB") or (LHS == "BANDMAP GUARD BAND SSB") )
      _guard_band[MODE_SSB] = from_string<int>(rhs);

// BAND MAP RECENT COLOUR
//    if (starts_with(testline, "BAND MAP RECENT COLOUR") or starts_with(testline, "BANDMAP RECENT COLOUR") or
//        starts_with(testline, "BAND MAP RECENT COLOR") or starts_with(testline, "BANDMAP RECENT COLOR"))
    if ( (LHS == "BAND MAP RECENT COLOUR") or (LHS == "BANDMAP RECENT COLOUR") or
         (LHS == "BANDMAP RECENT COLOR") or (LHS == "BANDMAP RECENT COLOR") )
    { if (!RHS.empty())
      { string name = remove_peripheral_spaces(RHS);

        _bandmap_recent_colour = string_to_colour(name);
      }
    }

// BANDS
    if (LHS == "BANDS")
      _bands = RHS;

// BATCH MESSAGES FILE
    if (LHS == "BATCH MESSAGES FILE")
      _batch_messages_file = rhs;

// CABRILLO FILENAME
    if (LHS == "CABRILLO FILENAME")
      _cabrillo_filename = rhs;

// CALL OK NOW MESSAGE
    if (LHS == "CALL OK NOW MESSAGE")
      _call_ok_now_message = rhs;

// CALLSIGN MULTS
//    if (starts_with(testline, "CALLSIGN MULTS") and !starts_with(testline, "CALLSIGN MULTS PER BAND"))
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
      _cluster_port = from_string<int>(rhs);

// CLUSTER SERVER
    if (LHS == "CLUSTER SERVER")
      _cluster_server = rhs;

// CLUSTER USERNAME
    if (LHS == "CLUSTER USERNAME")
      _cluster_username = rhs;

// CONTEST
    if (LHS == "CONTEST")
      _contest_name = RHS;

// COUNTRY EXCEPTION -- SEEMS NOT TO BE USED
//    if (starts_with(testline, "COUNTRY EXCEPTION"))
//    { if (testline.length() > 18)
//      { string exception_string = testline.substr(18);  // callsign = country
//        const vector<string> fields = remove_peripheral_spaces(split_string(exception_string, "="));
//
//        if (fields.size() == 2)
//          _country_exceptions.push_back( { fields[0], fields[1] } );
//      }
//    }

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
          { string bands_str = delimited_substring(lhs, '[', ']');
            vector<string> bands = split_string(bands_str, ",");

            for (size_t n = 0; n < bands.size(); ++n)
              bands[n] = remove_peripheral_spaces(bands[n]);

            for (size_t n = 0; n < bands.size(); ++n)
            { int wavelength = from_string<size_t>(bands[n]);
              BAND b;

              switch (wavelength)
              { case 160 :
                  b = BAND_160;
                  break;
                case 80 :
                  b = BAND_80;
                  break;
                case 60 :
                  b = BAND_60;
                  break;
                case 40 :
                  b = BAND_40;
                  break;
                case 30 :
                  b = BAND_30;
                  break;
                case 20 :
                  b = BAND_20;
                  break;
                case 17 :
                  b = BAND_17;
                  break;
                case 15 :
                  b = BAND_15;
                  break;
                case 12 :
                  b = BAND_12;
                  break;
                case 10 :
                  b = BAND_10;
                  break;
                default :
                  continue;
              }

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
    if (starts_with(testline, "CQ AUTO LOCK"))
      _cq_auto_lock = is_true;

// CQ AUTO RIT
    if (starts_with(testline, "CQ AUTO RIT"))
      _cq_auto_rit = is_true;

// CQ MEMORY n (wrt 1)
//    for (unsigned int memory = 1; memory <= CQ_MEMORY_MESSAGES; ++memory)
//      if (starts_with(testline, "CQ MEMORY " + to_string(memory)))
//        _cq_memory[memory] = RHS;

// CW SPEED
    if (starts_with(testline, "CW SPEED"))
      _cw_speed = from_string<unsigned int>(RHS);

// DECIMAL POINT
    if (starts_with(testline, "DECIMAL POINT"))
      _decimal_point = rhs;

// DO NOT SHOW
    if (starts_with(testline, "DO NOT SHOW") and !starts_with(testline, "DO NOT SHOW FILE"))
      _do_not_show = remove_peripheral_spaces(split_string(RHS, ","));

// DO NOT SHOW FILE
    if (starts_with(testline, "DO NOT SHOW FILE"))
      _do_not_show_filename = rhs;

// DRMASTER FILENAME
    if (starts_with(testline, "DRMASTER FILENAME"))
      _drmaster_filename = rhs;

// EX MEMORY Fn
//    for (unsigned int memory = 1; memory <= EX_MEMORY_MESSAGES; ++memory)
//      if (starts_with(testline, "EX MEMORY F" + to_string(memory)))
//        _ex_memory[memory - 1] = remove_peripheral_spaces(RHS);

// EXCHANGE
    if (starts_with(testline, "EXCHANGE"))
      if (starts_with(remove_leading_spaces(testline.substr(8)), "="))  // several commands start with "EXCHANGE"
        _exchange = RHS;

// EXCHANGE[
    if (starts_with(testline, "EXCHANGE["))
    { const string country = delimited_substring(LHS, '[', ']');

      _exchange_per_country.insert( { country, RHS  } );
    }

// EXCHANGE CQ
    if (starts_with(testline, "EXCHANGE CQ"))
      _exchange_cq = RHS;

// EXCHANGE FIELDS FILENAME
    if (starts_with(testline, "EXCHANGE FIELDS FILENAME"))
      _exchange_fields_filename = rhs;

// EXCHANGE MULTS
    if (starts_with(testline, "EXCHANGE MULTS") and !starts_with(testline, "EXCHANGE MULTS PER"))
    { _exchange_mults = RHS;

      if (contains(_exchange_mults, ","))       // if there is more than one exchange mult
        QSO_MULT_WIDTH = 4;                     // make them all the same width, so that the log line looks OK
    }

// EXCHANGE MULTS PER BAND
    if (starts_with(testline, "EXCHANGE MULTS PER BAND"))
      _exchange_mults_per_band = is_true;

// EXCHANGE MULTS PER MODE
    if (starts_with(testline, "EXCHANGE MULTS PER MODE"))
      _exchange_mults_per_mode = is_true;

// EXCHANGE SAP
    if (starts_with(testline, "EXCHANGE SAP"))
      _exchange_sap = RHS;

// EXCHANGE SENT std::vector<std::pair<std::string, std::string> >  sent_exchange(void) const
// exchange sent = RST:599, CQZONE:4
    if (starts_with(testline, "EXCHANGE SENT") and !starts_with(testline, "EXCHANGE SENT CW ") and !starts_with(testline, "EXCHANGE SENT SSB "))
    { const string comma_delimited_list = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));    // RST:599, CQZONE:4
      const vector<string> fields = split_string(comma_delimited_list, ",");

      for (size_t n = 0; n < fields.size(); ++n)
      { vector<string> field = split_string(fields[n], ":");

        _sent_exchange.push_back( { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) } );
      }
    }

// EXCHANGE SENT CW
    if (starts_with(testline, "EXCHANGE SENT CW"))
    { const string comma_delimited_list = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));    // RST:599, CQZONE:4
      const vector<string> fields = split_string(comma_delimited_list, ",");

      for (size_t n = 0; n < fields.size(); ++n)
      { vector<string> field = split_string(fields[n], ":");

        _sent_exchange_cw.push_back( { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) } );
      }
    }

// EXCHANGE SENT SSB
    if (starts_with(testline, "EXCHANGE SENT SSB"))
    { const string comma_delimited_list = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));    // RST:599, CQZONE:4
      const vector<string> fields = split_string(comma_delimited_list, ",");

      for (size_t n = 0; n < fields.size(); ++n)
      { vector<string> field = split_string(fields[n], ":");

        _sent_exchange_ssb.push_back( { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) } );
      }
    }

// INDIVIDUAL MESSAGES FILE
    if (starts_with(testline, "INDIVIDUAL MESSAGES FILE"))
      _individual_messages_file = rhs;

// KEYER PORT
    if (starts_with(testline, "KEYER PORT") or starts_with(testline, "KEYER OUTPUT PORT"))
      _keyer_port = rhs;

// LOG
    if (starts_with(testline, "LOG") and !rhs.empty())
      _logfile = rhs;

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
        { ost << "Parse error in " << LHS << endl;
          exit(-1);
        }
      }

      if (LHS == "MARK FREQUENCIES" or LHS == "MARK FREQUENCIES CW")
        _mark_frequencies.insert( { MODE_CW, frequencies } );

      if (LHS == "MARK FREQUENCIES" or LHS == "MARK FREQUENCIES SSB")
        _mark_frequencies.insert( { MODE_SSB, frequencies } );
    }

// MATCH MINIMUM
    if (starts_with(testline, "MATCH MINIMUM"))
      _match_minimum = from_string<int>(RHS);

// MODES
    if (starts_with(testline, "MODES"))
    { _modes = RHS;

      if (!contains(_modes, ","))
      { if (to_upper(_modes) == "SSB")
          _start_mode = MODE_SSB;
      }
    }

// MY CALL
    if (starts_with(testline, "MY CALL"))
      _my_call = RHS;

// MY CONTINENT
    if (starts_with(testline, "MY CONTINENT"))
      _my_continent = RHS;

// MY CQ ZONE
    if (starts_with(testline, "MY CQ ZONE"))
      _my_cq_zone = from_string<int>(RHS);

// MY GRID
    if (starts_with(testline, "MY GRID"))
      _my_grid = rhs;

// MY IP
    if (starts_with(testline, "MY IP"))
      _my_ip = rhs;

// MY LATITUDE
    if (starts_with(testline, "MY LATITUDE"))
      _my_latitude = from_string<float>(rhs);

// MY LATITUDE
    if (starts_with(testline, "MY LONGITUDE"))
      _my_longitude = from_string<float>(rhs);

// MY ITU ZONE
    if (starts_with(testline, "MY ITU ZONE"))
      _my_itu_zone = from_string<int>(RHS);

// NORMALISE RATE
    if (starts_with(testline, "NORMALISE RATE") or starts_with(testline, "NORMALIZE RATE"))
      _normalise_rate = is_true;

// NOT COUNTRY MULTS
    if (starts_with(testline, "NOT COUNTRY MULTS"))
    { if (!rhs.empty())
        _not_country_mults = rhs;
    }

// PATH
    if (starts_with(testline, "PATH"))
    { if (!rhs.empty())
        _path = remove_peripheral_spaces(split_string(rhs, ";"));
    }

// POINTS
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
    if (starts_with(testline, "PTT DELAY"))
      _ptt_delay = from_string<unsigned int>(RHS);

// P3 SNAPSHOT FILE
    if (starts_with(testline, "P3 SNAPSHOT FILE"))
      _p3_snapshot_file = rhs;

// QUICK QSL MESSAGE
    if (starts_with(testline, "QUICK QSL MESSAGE"))
      _quick_qsl_message = RHS;

// QSL MESSAGE
    if (starts_with(testline, "QSL MESSAGE"))
      _qsl_message = RHS;

// QSO MULTIPLE BANDS
    if (starts_with(testline, "QSO MULTIPLE BANDS"))
      _qso_multiple_bands = is_true;

// QSO MULTIPLE MODES
    if (starts_with(testline, "QSO MULTIPLE MODES"))
      _qso_multiple_modes = is_true;

// QTCS
    if (starts_with(testline, "QTCS"))
      _qtcs = is_true;

// QTC DOUBLE SPACE
    if (starts_with(testline, "QTC DOUBLE SPACE"))
      _qtc_double_space = is_true;

// QTC FILENAME
    if (starts_with(testline, "QTC FILENAME"))
      _qtc_filename = rhs;

// QTC QRS
    if (starts_with(testline, "QTC QRS"))
      _qtc_qrs = from_string<unsigned int>(rhs);

// QTHX: QTHX[callsign-or-canonical prefix] = aa, bb, cc...
// the conversion to canonical prefix occurs later, inside contest_rules::_parse_context_qthx()
    if (starts_with(testline, "QTHX["))
    { const vector<string> fields = remove_peripheral_spaces(split_string(testline, "="));

      if (fields.size() == 2)
      { const string canonical_prefix = delimited_substring(fields[0], '[', ']');
        const vector<string> values = remove_peripheral_spaces(split_string(RHS, ","));
        set<string> ss;

        copy(values.cbegin(), values.cend(), inserter(ss, ss.end()));

        _qthx.insert( { canonical_prefix, ss } );
      }
    }

// RATE
    if (starts_with(testline, "RATE"))
    { const vector<string> vec_rates = remove_peripheral_spaces(split_string(rhs, ","));
      vector<unsigned int> new_rates;

      FOR_ALL(vec_rates, [&new_rates] (const string& str) { new_rates.push_back(from_string<unsigned int>(str)); } );

      if (!new_rates.empty())
        _rate_periods = new_rates;
    }

// RBN BEACONS
    if (starts_with(testline, "RBN BEACONS"))
      _rbn_beacons = is_true;

// RBN PORT
    if (starts_with(testline, "RBN PORT"))
      _rbn_port = from_string<int>(rhs);

// RBN SERVER
    if (starts_with(testline, "RBN SERVER"))
      _rbn_server = rhs;

// RBN THRESHOLD
    if (starts_with(testline, "RBN THRESHOLD"))
      _rbn_threshold = from_string<unsigned int>(rhs);

// RBN USERNAME
    if (starts_with(testline, "RBN USERNAME"))
      _rbn_username = rhs;

// REJECT COLOUR
    if ( ( (LHS == "REJECT COLOUR") or (LHS == "REJECT COLOR") ) and !rhs.empty() )
      _reject_colour = string_to_colour(RHS);

// RIG 1 BAUD
    if (starts_with(testline, "RIG 1 BAUD") or starts_with(testline, "RIG BAUD"))
      _rig1_baud = from_string<unsigned int>(rhs);

// RIG 1 DATA BITS
    if (starts_with(testline, "RIG 1 DATA BITS") or starts_with(testline, "RIG DATA BITS"))
      _rig1_data_bits = from_string<unsigned int>(rhs);

// RIG 1 NAME
    if (starts_with(testline, "RIG 1 NAME") or starts_with(testline, "RADIO ONE NAME"))
      _rig1_name = rhs;

// RIG 1 PORT
    if (starts_with(testline, "RIG 1 PORT") or starts_with(testline, "RADIO ONE CONTROL PORT"))
      _rig1_port = rhs;

// RIG 1 STOP BITS
    if (starts_with(testline, "RIG 1 STOP BITS") or starts_with(testline, "RIG STOP BITS"))
      _rig1_stop_bits = from_string<unsigned int>(rhs);

// RIG 1 TYPE
    if (starts_with(testline, "RIG 1 TYPE"))
      _rig1_type = RHS;

// RULES
    if (starts_with(testline, "RULES"))
      _process_configuration_file(rhs);

// RULES
    if (starts_with(testline, "RUSSIAN DATA"))
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
    if (starts_with(testline, "SCREEN SNAPSHOT FILE"))
      _screen_snapshot_file = rhs;

// SHIFT DELTA
    if (starts_with(testline, "SHIFT DELTA"))
      _shift_delta = from_string<unsigned int>(rhs);

// SHIFT POLL
    if (starts_with(testline, "SHIFT POLL"))
      _shift_poll = from_string<unsigned int>((split_string(line, "="))[1]);

#if 0
    static std::map<std::string, BAND> BAND_FROM_NAME { { "160", BAND_160 },
                                                        { "80",  BAND_80 },
                                                        { "60",  BAND_60 },
                                                        { "40",  BAND_40 },
                                                        { "30",  BAND_30 },
                                                        { "20",  BAND_20 },
                                                        { "17",  BAND_17 },
                                                        { "15",  BAND_15 },
                                                        { "12",  BAND_12 },
                                                        { "10",  BAND_10 }
                                                      };
#endif

// START BAND
    if (starts_with(testline, "START BAND"))
    { const string str = RHS;

      if (BAND_FROM_NAME.find(str) != BAND_FROM_NAME.cend())
        _start_band = BAND_FROM_NAME.find(str)->second;
    }

// START MODE
    if (starts_with(testline, "START MODE"))
    { if (RHS == "SSB")
        _start_mode = MODE_SSB;
    }

// SYNC KEYER
    if (starts_with(testline, "SYNC KEYER"))
      _sync_keyer = is_true;

// TEST
    if (starts_with(testline, "TEST"))
      _test = is_true;

// THOUSANDS SEPARATOR
    if (starts_with(testline, "THOUSANDS SEPARATOR"))
      _thousands_separator = rhs;

// WORKED MULTS COLOUR
    if (starts_with(testline, "WORKED MULTS COLOUR") or starts_with(testline, "WORKED MULTS COLOR"))
      _worked_mults_colour = RHS;

// ---------------------------------------------  MULTIPLIERS  ---------------------------------


// COUNTRY MULTS
// Currently supported: ALL
//                      NONE
// any single continent
//    if (starts_with(testline, "COUNTRY MULTS") and !starts_with(testline, "COUNTRY MULTS PER"))
    if (LHS == "COUNTRY MULTS")
    { _country_mults_filter = RHS;

      if (_country_mults_filter == "NONE")
        QSO_DISPLAY_COUNTRY_MULT = false;                  // since countries aren't mults, don't take up space in the log line
    }

// REMAINING CALLSIGN MULTS
    if (starts_with(testline, "REMAINING CALLSIGN MULTS"))
    { _auto_remaining_callsign_mults = (RHS == "AUTO");

      if (!_auto_remaining_callsign_mults)
      { const vector<string> mults = remove_peripheral_spaces(split_string(RHS, ","));

        _remaining_callsign_mults_list = set<string>(mults.cbegin(), mults.cend());
      }
    }

// REMAINING COUNTRY MULTS
    if (starts_with(testline, "REMAINING COUNTRY MULTS"))
    { _auto_remaining_country_mults = (RHS == "AUTO");

      if (!_auto_remaining_country_mults)
      { const vector<string> countries = remove_peripheral_spaces(split_string(RHS, ","));

        _remaining_country_mults_list = set<string>(countries.cbegin(), countries.cend());
      }
    }

// REMAINING EXCHANGE MULTS
      if (starts_with(testline, "AUTO REMAINING EXCHANGE MULTS"))
      { const vector<string> mult_names = remove_peripheral_spaces(split_string(RHS, ","));

        for (const auto& str : mult_names)
          _auto_remaining_exchange_mults.insert(str);
      }

// ---------------------------------------------  CABRILLO  ---------------------------------

// CABRILLO CONTEST
    if (starts_with(testline, "CABRILLO CONTEST") and
        (is_legal_value(RHS, "AP-SPRINT,ARRL-10,ARRL-160,ARRL-DX-CW,ARRL-DX-SSB,ARRL-SS-CW,ARRL-SS-SSB,ARRL-UHF-AUG,ARRL-VHF-JAN,ARRL-VHF-JUN,ARRL-VHF-SEP,ARRL-RTTY,BARTG-RTTY,CQ-160-CW,CQ-160-SSB,CQ-WPX-CW,CQ-WPX-RTTY,CQ-WPX-SSB,CQ-VHF,CQ-WW-CW,CQ-WW-RTTY,CQ-WW-SSB,DARC-WAEDC-CW,DARC-WAEDC-RTTY,DARC-WAEDC-SSB,FCG-FQP,IARU-HF,JIDX-CW,JIDX-SSB,NAQP-CW,NAQP-RTTY,NAQP-SSB,NA-SPRINT-CW,NA-SPRINT-SSB,NCCC-CQP,NEQP,OCEANIA-DX-CW,OCEANIA-DX-SSB,RDXC,RSGB-IOTA,SAC-CW,SAC-SSB,STEW-PERRY,TARA-RTTY", ",")))
          _cabrillo_contest = RHS;                        // required to be upper case

 // CABRILLO EMAIL (sic)
    if (starts_with(testline, "CABRILLO E-MAIL") or starts_with(testline, "CABRILLO EMAIL"))
      _cabrillo_e_mail = rhs;

// CABRILLO LOCATION
    if (starts_with(testline, "CABRILLO LOCATION"))
      _cabrillo_location = rhs;

// CABRILLO NAME
    if (starts_with(testline, "CABRILLO NAME"))
      _cabrillo_name = rhs;

// CABRILLO CATEGORY-ASSISTED
    if (starts_with(testline, "CABRILLO CATEGORY-ASSISTED") and is_legal_value(RHS, "ASSISTED,NON-ASSISTED", ","))
      _cabrillo_category_assisted = RHS;

// CABRILLO CATEGORY-BAND
    if (starts_with(testline, "CABRILLO CATEGORY-BAND"))
    { const string value = rhs; // remove_peripheral_spaces((split_string(line, "="))[1]);

// The spec calls for bizarre capitalization
      if (is_legal_value(value, "ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light", ","))
        _cabrillo_category_band = value;
    }

// CABRILLO CATEGORY-MODE
    if (starts_with(testline, "CABRILLO CATEGORY-MODE"))
    { const string value = RHS; // to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "CW,MIXED,RTTY,SSB", ","))
        _cabrillo_category_mode = value;
    }

// CABRILLO CATEGORY-OPERATOR
    if (starts_with(testline, "CABRILLO CATEGORY-OPERATOR"))
    { const string value = RHS; // to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "CHECKLOG,MULTI-OP,SINGLE-OP", ","))
        _cabrillo_category_operator = value;
    }

// CABRILLO CATEGORY-OVERLAY
    if (starts_with(testline, "CABRILLO CATEGORY-OVERLAY"))
    { const string value = RHS; //to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES", ","))
        _cabrillo_category_operator = value;
    }

// CABRILLO CATEGORY-POWER
    if (starts_with(testline, "CABRILLO CATEGORY-POWER"))
    { const string value = RHS; // to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "HIGH,LOW,QRP", ","))
        _cabrillo_category_power = value;
    }

// CABRILLO CATEGORY-STATION
    if (starts_with(testline, "CABRILLO CATEGORY-STATION"))
    { const string value = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL", ","))
        _cabrillo_category_station = value;
    }

// CABRILLO CATEGORY-TIME
    if (starts_with(testline, "CABRILLO CATEGORY-TIME"))
    { const string value = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "6-HOURS,12-HOURS,24-HOURS", ","))
        _cabrillo_category_station = value;
    }

// CABRILLO CATEGORY-TRANSMITTER
    if (starts_with(testline, "CABRILLO CATEGORY-TRANSMITTER"))
    { const string value = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "LIMITED,ONE,SWL,TWO,UNLIMITED", ","))
        _cabrillo_category_transmitter = value;
    }

// CABRILLO CLUB
    if (starts_with(testline, "CABRILLO CLUB"))
      _cabrillo_club = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

// CABRILLO ADDRESS first line
    if (starts_with(testline, "CABRILLO ADDRESS 1"))
      _cabrillo_address_1 = remove_peripheral_spaces((split_string(line, "="))[1]);

// CABRILLO ADDRESS second line
    if (starts_with(testline, "CABRILLO ADDRESS 2"))
      _cabrillo_address_2 = remove_peripheral_spaces((split_string(line, "="))[1]);

// CABRILLO ADDRESS third line
    if (starts_with(testline, "CABRILLO ADDRESS 3"))
      _cabrillo_address_3 = remove_peripheral_spaces((split_string(line, "="))[1]);

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

    if (starts_with(testline, "CABRILLO QSO"))
    { _cabrillo_qso_template = RHS;

      if (contains(RHS, "TEMPLATE"))
      { try
        { string key = remove_peripheral_spaces(split_string(RHS, ":"))[1];

          _cabrillo_qso_template = cabrillo_qso_templates.at(key);
        }

        catch (...)
        { ost << "Error in CABRILLO QSO TEMPLATE" << endl;
          exit(-1);
        }
      }
    }

// ---------------------------------------------  WINDOWS  ---------------------------------

    if (starts_with(testline, "WINDOW"))
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

    if (starts_with(testline, "STATIC WINDOW") and !starts_with(testline, "STATIC WINDOW INFO"))
    { const vector<string> fields = remove_peripheral_spaces(split_string(rhs, ","));

      if (fields.size() == 2)  // name, contents
      { const string name = fields[0];
        string contents = fields[1];

        _static_windows[name] = { contents, vector<window_information>() };
      }
    }
//    std::map<std::string /* name */, std::pair<std::string /* contents */, std::vector<window_information> > > _static_windows;

    if (starts_with(testline, "STATIC WINDOW INFO"))  // must come after the corresponding STATIC WINDOW command
    { const vector<string> window_info = remove_peripheral_spaces(split_string(split_string(testline, "=")[1], ","));

      if (!window_info.empty())
      { const string name = window_info[0];

        if (_static_windows.count(name) != 0)  // make sure that the window exists
        { if (window_info.size() >= 3)
          { window_information winfo;

            winfo.x(from_string<int>(window_info[1]));
            winfo.y(from_string<int>(window_info[2]));

            string contents = _static_windows[name].first;

            if (contents.size() >= 2)
            { const char delimiter = contents[0];
              const size_t posn = contents.find_first_of(delimiter, 1);

              if (posn != 0)
                contents = substring(contents, 1, posn - 1);
            }

            winfo.w(contents.length());
            winfo.h(1);

            if (window_info.size() >= 4)
            { winfo.fg_colour(window_info[3]);

              if (window_info.size() >= 5)
                winfo.bg_colour(window_info[4]);

              winfo.colours_set(true);
            }

            vector<window_information> vec = _static_windows[name].second;

            vec.push_back(winfo);
            _static_windows[name] = { contents, vec };
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

    if (starts_with(testline, "MESSAGE CQ 1"))
    { const vector<string> tokens = split_string(testline, "=");

      if (tokens.size() == 2)
        _message_cq_1 = tokens[1];
    }

    if (starts_with(testline, "MESSAGE CQ 2"))
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

#if !defined(NEW_CONSTRUCTOR)
  if (!_exchange_fields_filename.empty())
    EXCHANGE_FIELD_TEMPLATES.prepare(_path, _exchange_fields_filename);
#endif

  if (_qsl_message.empty())
    _qsl_message = "tu " + _my_call + " test";

  if (_quick_qsl_message.empty())
    _quick_qsl_message = "tu " + _my_call;

  if (_message_cq_1.empty())
    _message_cq_1 = "test " + _my_call + " " + _my_call + " test";

  if (_message_cq_2.empty())
    _message_cq_2 = "cq cq test de  " + _my_call + "  " + _my_call + "  " + _my_call + "  test";

// possibly fix Cabrillo template
  if ( (_cabrillo_qso_template == "ARRL DX") or (_cabrillo_qso_template == "CQ WW") )
  { vector<string> actual_modes = remove_peripheral_spaces(split_string(_modes, ","));

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
  _accept_colour(COLOUR_GREEN),                                  // green for calls that are OK to work
  _archive_name("drlog-restart"),                                // name for the archive written when leaving drlog
  _auto_backup(""),                                              // no auto backup directory
  _auto_remaining_country_mults(false),                          // do not add country mults as we detect them
  _auto_remaining_exchange_mults( { } ),                         // do not add any exchange mults as we detect them
  _auto_screenshot(false),                                       // do not generate horal screenshots
  _bandmap_decay_time_local(60),                                 // stay on bandmap for one hour
  _bandmap_decay_time_cluster(60),                               // stay on bandmap for one hour
  _bandmap_decay_time_rbn(60),                                   // stay on bandmap for one hour
  _bandmap_fade_colours( { 255, 200, 150, 100 } ),               // four colour steps
  _bandmap_filter(),                                             // empty bandmap filter
  _bandmap_filter_disabled_colour(string_to_colour("BLACK")),    // background colour when bandmap filter is disabled
  _bandmap_filter_enabled(false),                                // do not filter bandmap
  _bandmap_filter_foreground_colour(string_to_colour("WHITE")),  // letters are white
  _bandmap_filter_hide_colour(string_to_colour("RED")),          // background colour when bandmap filter is in HIDE mode
  _bandmap_filter_show(false),                                   // filter is used to hide entrie
  _bandmap_filter_show_colour(string_to_colour("GREEN")),        // background colour when bandmap filter is in SHOW mode
  _bandmap_recent_colour(string_to_colour("BLACK")),             // do not indicate recent postings with special colour
  _bands("160, 80, 40, 20, 15, 10"),                             // legal bands for the contest
  _batch_messages_file(),                                        // name for batch messages file
  _cabrillo_address_1(),
  _cabrillo_address_2(),
  _cabrillo_address_3(),
  _cabrillo_address_4(),
  _cabrillo_address_city(),
  _cabrillo_address_country(),
  _cabrillo_address_postalcode(),
  _cabrillo_address_state_province(),
  _cabrillo_callsign(),                       // CALLSIGN in Cabrillo file
  _cabrillo_category_assisted("ASSISTED"),
  _cabrillo_category_band("ALL"),
  _cabrillo_category_mode(),
  _cabrillo_category_operator("SINGLE-OP"),
  _cabrillo_category_overlay(),
  _cabrillo_category_power("HIGH"),
  _cabrillo_category_station(),
  _cabrillo_category_time(),
  _cabrillo_category_transmitter("ONE"),
  _cabrillo_club(),
  _cabrillo_contest(),                        // CONTEST in Cabrillo file
  _cabrillo_e_mail(),
  _cabrillo_filename("cabrillo"),             // name of file that will store Cabrillo log
  _cabrillo_location(),
  _cabrillo_name(),                           // NAME in Cabrillo file
  _cabrillo_qso_template(),
  _callsign_mults(),                          // callsign is not the basis for any mults
  _callsign_mults_per_band(false),            // any callsign mults are once-only
  _callsign_mults_per_mode(false),            // any callsign mults are once-only
  _cluster_port(23),                          // standard telnet server port
  _country_list(COUNTRY_LIST_WAEDC),          // WAE country list
  _country_mults_filter("ALL"),               // all countries are mults
  _country_mults_per_band(true),              // any country mults are per-band
  _country_mults_per_mode(false),             // any country mults are not per-mode
  _cq_auto_lock(false),                       // don't lock the TX frequency in CQ mode
  _cq_auto_rit(false),                        // don't enable RIT in CQ mode
  _cty_filename("cty.dat"),                   // filename for country data
  _cw_speed(29),                              // 29 WPM
  _decimal_point("·"),                        // use centred dot as decimal point
  _do_not_show(),                             // all calls (apart from my_call()) should be shown on the bandmap
  _do_not_show_filename(),                    // no do-not-show file
  _drmaster_filename("drmaster"),             // name of the drmaster file
  _exchange("RST"),                           // exchange is just RST
//  _exchange_cw(),                             // no default CW exchange
//  _exchange_ssb(),                            // no default SSB exchange
  _exchange_fields_filename(),                // file that holds regex templates for exchange fields
  _exchange_mults_per_band(false),            // any exchange mults are once-only
  _exchange_mults_per_mode(false),            // any exchange mults are once-only
  _guard_band( { { MODE_CW, 500 }, { MODE_SSB, 2000 } } ),  // 500 Hz guard band on CW, 2 kHz on slopbucket
  _individual_messages_file(),                // no file of individual messages
  _logfile("drlog.dat"),                      // name of log file
  _mark_frequencies(),                        // don't mark any frequencies
  _match_minimum(4),                          // 4 characters required for SCP or fuzzy match
  _message_cq_1(),                            // no short CQ (default is changed once configuration file has been read)
  _message_cq_2(),                            // no long CQ (default is changed once configuration file has been read)
  _modes("CW"),                               // only valid mode is CW
  _my_call("NONE"),                           // set callsign to an invalid value
  _my_continent("XX"),                        // set continent to an invalid value
  _my_latitude(0),                            // at the equator
  _my_longitude(0),                           // Greenwich meridian
  _normalise_rate(false),                     // do not normalise rates to one-hour values
  _not_country_mults(),                       // no countries are explicitly not country mults
  _path( { "." } ),                           // search only the current directory
  _per_band_points( {} ),                     // no points awarded anywhere
  _ptt_delay(25),                             // PTT delay
  _p3_snapshot_file("P3"),                    // P3 snapshots will be in P3-<n>
  _quick_qsl_message(),                       // no quick QSL message (default is changed once configuration file has been read)
  _qsl_message(),                             // no QSL message (default is changed once configuration file has been read)
  _qso_multiple_bands(false),                 // each station may be worked on only one band
  _qso_multiple_modes(false),                 // each station may be worked on only one mode
  _qtcs(false),                               // QTCs are disabled
  _qtc_double_space(false),                   // QTC elements are single-spaced
  _qtc_filename("QTCs"),                      // QTC filename
  _qtc_qrs(3),                                // WPM decrease when sending QTC
  _rate_periods( { 15, 30, 60 } ),            // 15-, 30-, 60-minute periods for rates
  _rbn_beacons(false),                        // do not place RBN posts from beacons on bandmap
  _rbn_port(7000),                            // telnet port for the reverse beacon network
  _rbn_server("telnet.reversebeacon.net"),    // domain name of the reverse beacon network telnet server
  _rbn_threshold(1),                          // all received spots are posted
  _rbn_username(""),                          // no default name to access the RBN
  _reject_colour(COLOUR_RED),                 // red for dupes
  _remaining_country_mults_list(),            // no remaining country mults
  _rig1_baud(4800),                           // 4800 baud
  _rig1_data_bits(8),                         // 8-bit data
  _rig1_port("/dev/ttyS0"),                   // first serial port
  _rig1_stop_bits(1),                         // one stop bit
  _rig1_type(""),                             // no default rig type
  _russian_filename("russian-data"),          // default file for Russian location information
  _screen_snapshot_file("screen"),            // screen snapshots will be in screen-<n>
  _sent_exchange(),                           // no default sent exchange
  _sent_exchange_cw(),                        // no default sent CW exchange
  _sent_exchange_ssb(),                       // no default sent SSB exchange
  _shift_delta(10),                           // shift RIT by 10 Hz
  _shift_poll(50),                            // poll every 50 milliseconds
  _start_band(BAND_20),                       // start on 20m
  _start_mode(MODE_CW),                       // start on CW
  _sync_keyer(false),                         // do not synchronise rig keyer with computer
  _test(false),                               // transmit is not disabled
  _thousands_separator(","),                  // numbers are written with ","
  _worked_mults_colour("RED")                 // worked mults are in red
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

/*! \brief              information pertaining to a particular window
    \param      name    name of window
    \return             location, size and colour information
*/
const window_information drlog_context::window_info(const string& name) const
{ const auto cit = _windows.find(name);

  return (cit == _windows.cend() ? window_information() : cit->second);
}

/*! \brief              all the windows whose name contains a particular substring
    \param      subst   substring for which to search
    \return             all the window names that include <i>substr</i>
*/
const vector<string> drlog_context::window_name_contains(const string& substr) const
{ vector<string> rv;

  for (auto cit = _windows.cbegin(); cit != _windows.cend(); ++cit)
    if (contains(cit->first, substr))
      rv.push_back(cit->first);

  return rv;
}

/*! \brief              is a particular frequency within any marked range?
    \param      m       mode
    \param      f       frequency to test
    \return             whether <i>f</i> is in any marked range for the mode <i>m</i>
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

/*! \brief              Get all the names in the sent exchange
    \return             the names of all the fields in the sent exchange
*/
const vector<string> drlog_context::sent_exchange_names(void) const
{ vector<string> rv;

  for (const auto& pss : _sent_exchange)
    rv.push_back(pss.first);

  return rv;
}

/*! \brief              Get all the names in the sent CW exchange
    \return             the names of all the fields in the sent CW exchange
*/
const vector<string> drlog_context::sent_exchange_cw_names(void) const
{ vector<string> rv;

  for (const auto& pss : _sent_exchange_cw)
    rv.push_back(pss.first);

  return rv;
}

/*! \brief              Get all the names in the sent SSB exchange
    \return             the names of all the fields in the sent SSB exchange
*/
const vector<string> drlog_context::sent_exchange_ssb_names(void) const
{ vector<string> rv;

  for (const auto& pss : _sent_exchange_ssb)
    rv.push_back(pss.first);

  return rv;
}

const decltype(drlog_context::_sent_exchange) drlog_context::sent_exchange(const MODE m)
{ SAFELOCK(_context);

  decltype(_sent_exchange) rv = ( (m == MODE_CW) ? _sent_exchange_cw : _sent_exchange_ssb);

  if (rv.empty())
  { rv = _sent_exchange;

// fix RST/RS if using non-mode-specific exchange
    for (int n = 0; n < rv.size(); ++n)
    { pair<string, string>& pss = rv[n];

      if ( (m == MODE_SSB) and (pss.first == "RST") )
      { pss.first = "RS";
        pss.second = "59";  // hardwire!
      }

      if ( (m == MODE_CW) and (pss.first == "RS") )
      { pss.first = "RST";
        pss.second = "599";  // hardwire!
      }
    }
  }

  return rv;
}
