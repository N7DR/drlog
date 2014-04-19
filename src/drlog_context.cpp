// $Id: drlog_context.cpp 58 2014-04-12 17:23:28Z  $

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

extern message_stream ost;
extern bool QSO_DISPLAY_COUNTRY_MULT;        ///< controls whether country mults are written on the log line
extern int  QSO_MULT_WIDTH;                  ///< controls width of zone mults field in log line

//extern map<string, int>    key_names;    ///< The names of the keys on the keyboard; maps names to X KeySyms

// -----------  drlog_context  ----------------

/*! \class drlog_context
    \brief The variables and constants that comprise the context for drlog
*/

pt_mutex _context_mutex;

// we have a separate routine so that it can be called recursively for the rules
void drlog_context::_process_configuration_file(const string& filename)
{ string entire_file;

  try
  { entire_file = remove_char(read_file(filename), CR_CHAR);    // remove_peripheral_character(remove_char(read_file(filename), CR_CHAR), LF_CHAR);
  }

  catch (...)
  { cerr << "Unable to open configuration file " << filename << endl;
    exit(-1);
  }

  static const std::string LF_STR   = "\n";      ///< LF as string

// split into lines
  const vector<string> lines = split_string(entire_file, LF_STR);

// process each line
  for (const auto& line : lines)
  {
// generate a number of useful variables to save writing them explicitly many times
// this is only processed at start-up so we don't need to be efficient
    const string testline = remove_leading_spaces(to_upper(line));
    const vector<string> fields = split_string(line, "=");
    const string rhs = ((fields.size() > 1) ? remove_peripheral_spaces(fields[1]) : "");      // the stuff to the right of the "="
    const string RHS = to_upper(rhs);                                                         // converted to upper case
    const bool is_true = (RHS == "TRUE");                                                     // is right hand side == "TRUE"?
    const string lhs = ( (fields.empty()) ? "" : remove_peripheral_spaces(fields[0]) );       // the stuff to the left of the "="
    const string LHS = to_upper(lhs);                                                         // converted to upper case

// ARCHIVE
    if (starts_with(testline, "ARCHIVE") and !rhs.empty())
      _archive_name = rhs;

// AUTO BACKUP
    if (starts_with(testline, "AUTO BACKUP") and !rhs.empty())
      _auto_backup = rhs;

// BAND MAP DECAY TIME CLUSTER
    if (starts_with(testline, "BAND MAP DECAY TIME CLUSTER") or starts_with(testline, "BANDMAP DECAY TIME CLUSTER"))
      _bandmap_decay_time_cluster = from_string<int>(rhs);

// BAND MAP DECAY TIME LOCAL
    if (starts_with(testline, "BAND MAP DECAY TIME LOCAL") or starts_with(testline, "BANDMAP DECAY TIME LOCAL"))
      _bandmap_decay_time_local = from_string<int>(rhs);

// BAND MAP DECAY TIME RBN
    if (starts_with(testline, "BAND MAP DECAY TIME RBN") or starts_with(testline, "BANDMAP DECAY TIME RBN"))
      _bandmap_decay_time_rbn = from_string<int>(rhs);

// BAND MAP FADE COLOURS
    if (starts_with(testline, "BAND MAP FADE COLOURS") or starts_with(testline, "BANDMAP FADE COLOURS") or
        starts_with(testline, "BAND MAP FADE COLORS") or starts_with(testline, "BANDMAP FADE COLORS"))
    { _bandmap_fade_colours.clear();

      if (!RHS.empty())
      { const vector<string> colour_names = remove_peripheral_spaces(split_string(RHS, ","));

        for_each(colour_names.cbegin(), colour_names.cend(), [&] (const string& name) { _bandmap_fade_colours.push_back(string_to_colour(name)); } );
      }
    }

// BAND MAP FILTER
    if ( (starts_with(testline, "BAND MAP FILTER") or starts_with(testline, "BANDMAP FILTER")) and
        !(starts_with(testline, "BAND MAP FILTER ENABLE") or starts_with(testline, "BANDMAP FILTER ENABLE")) and
        !(starts_with(testline, "BAND MAP FILTER MODE") or starts_with(testline, "BANDMAP FILTER MODE")) and
        !(starts_with(testline, "BAND MAP FILTER COLOURS") or starts_with(testline, "BAND MAP FILTER COLORS")) and
        !(starts_with(testline, "BANDMAP FILTER COLOURS") or starts_with(testline, "BANDMAP FILTER COLORS")) )
    { if (!RHS.empty())
      { vector<string> filters = remove_peripheral_spaces(split_string(RHS, ","));

        sort(filters.begin(), filters.end(), compare_calls);    // put the entries into callsign order
        _bandmap_filter = filters;
      }
    }

// BAND MAP FILTER ENABLED
    if (starts_with(testline, "BAND MAP FILTER COLOURS") or starts_with(testline, "BAND MAP FILTER COLORS") or
        starts_with(testline, "BANDMAP FILTER COLOURS") or starts_with(testline, "BANDMAP FILTER COLORS"))
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

// BAND MAP FILTER ENABLED
    if (starts_with(testline, "BAND MAP FILTER ENABLE") or starts_with(testline, "BANDMAP FILTER ENABLE"))
      _bandmap_filter_enabled = is_true;

// BAND MAP FILTER MODE
    if (starts_with(testline, "BAND MAP FILTER MODE") or starts_with(testline, "BANDMAP FILTER MODE"))
      _bandmap_filter_show = (RHS == "SHOW");

// BAND MAP GUARD BAND CW
    if (starts_with(testline, "BAND MAP GUARD BAND CW") or starts_with(testline, "BANDMAP GUARD BAND CW"))
      _guard_band[MODE_CW] = from_string<int>(rhs);

// BAND MAP GUARD BAND SSB
    if (starts_with(testline, "BAND MAP GUARD BAND SSB") or starts_with(testline, "BANDMAP GUARD BAND SSB"))
      _guard_band[MODE_SSB] = from_string<int>(rhs);

// BANDS
    if (starts_with(testline, "BANDS"))
      _bands = RHS;

// BATCH MESSAGES FILE
    if (starts_with(testline, "BATCH MESSAGES FILE"))
      _batch_messages_file = rhs;

// CABRILLO FILENAME
    if (starts_with(testline, "CABRILLO FILENAME"))
      _cabrillo_filename = rhs;

// CALL OK NOW MESSAGE
    if (starts_with(testline, "CALL OK NOW MESSAGE"))
      _call_ok_now_message = rhs;

// CALLSIGN MULTS
    if (starts_with(testline, "CALLSIGN MULTS") and !starts_with(testline, "CALLSIGN MULTS PER BAND"))
    { const vector<string> callsign_mults_vec = remove_peripheral_spaces(split_string(RHS, ","));

      move(callsign_mults_vec.cbegin(), callsign_mults_vec.cend(), inserter(_callsign_mults, _callsign_mults.begin()));
    }

// CALLSIGN MULTS PER BAND
    if (starts_with(testline, "CALLSIGN MULTS PER BAND"))
      _callsign_mults_per_band = is_true;

// CLUSTER PORT
    if (starts_with(testline, "CLUSTER PORT"))
      _cluster_port = from_string<int>(rhs);

// CLUSTER SERVER
    if (starts_with(testline, "CLUSTER SERVER"))
      _cluster_server = rhs;

// CLUSTER USERNAME
    if (starts_with(testline, "CLUSTER USERNAME"))
      _cluster_username = rhs;

// CONTEST
    if (starts_with(testline, "CONTEST"))
      _contest_name = RHS;

// COUNTRY EXCEPTION
    if (starts_with(testline, "COUNTRY EXCEPTION"))
    { if (testline.length() > 18)
      { string exception_string = testline.substr(18);  // callsign = country
        const vector<string> fields = remove_peripheral_spaces(split_string(exception_string, "="));

        if (fields.size() == 2)
          _country_exceptions.push_back( { fields[0], fields[1] } );
      }
    }

// COUNTRY FILENAME
    if (starts_with(testline, "COUNTRY FILENAME"))
      _cty_filename = rhs;

// COUNTRY LIST
    if (starts_with(testline, "COUNTRY LIST"))
    { if (RHS == "DXCC")
        _country_list = COUNTRY_LIST_DXCC;

      if (RHS == "WAEDC")
        _country_list = COUNTRY_LIST_WAEDC;
    }

// COUNTRY MULTS PER BAND
    if (starts_with(testline, "COUNTRY MULTS PER BAND"))
      _country_mults_per_band = is_true;

// CQ AUTO LOCK
    if (starts_with(testline, "CQ AUTO LOCK"))
      _cq_auto_lock = is_true;

// CQ AUTO RIT
    if (starts_with(testline, "CQ AUTO RIT"))
      _cq_auto_rit = is_true;

// CQ MEMORY n (wrt 1)
    for (unsigned int memory = 1; memory <= CQ_MEMORY_MESSAGES; ++memory)
      if (starts_with(testline, "CQ MEMORY " + to_string(memory)))
        _cq_memory[memory] = RHS;

// CQ MENU
    if (starts_with(testline, "CQ MENU"))
      _cq_menu = (split_string(line, "="))[1];

// CW SPEED
    if (starts_with(testline, "CW SPEED"))
      _cw_speed = from_string<unsigned int>(RHS);

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
    for (unsigned int memory = 1; memory <= EX_MEMORY_MESSAGES; ++memory)
      if (starts_with(testline, "EX MEMORY F" + to_string(memory)))
        _ex_memory[memory - 1] = remove_peripheral_spaces((split_string(line, "="))[1]);

// EX MENU
    if (starts_with(testline, "EX MENU"))
      _ex_menu = (split_string(line, "="))[1];

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
    if (starts_with(testline, "EXCHANGE MULTS") and !starts_with(testline, "EXCHANGE MULTS PER BAND"))
    { _exchange_mults = RHS;

      if (contains(_exchange_mults, ","))       // if there are more than one exchange mult
        QSO_MULT_WIDTH = 4;                     // make them all the same width, so that the log line looks OK
    }

// EXCHANGE MULTS PER BAND
    if (starts_with(testline, "EXCHANGE MULTS PER BAND"))
      _exchange_mults_per_band = is_true;

// EXCHANGE SAP
    if (starts_with(testline, "EXCHANGE SAP"))
      _exchange_sap = RHS;

// EXCHANGE SENT std::vector<std::pair<std::string, std::string> >  sent_exchange(void) const
// exchange sent = RST:599, CQZONE:4
    if (starts_with(testline, "EXCHANGE SENT"))
    { const string comma_delimited_list = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));    // RST:599, CQZONE:4
      const vector<string> fields = split_string(comma_delimited_list, ",");

      for (size_t n = 0; n < fields.size(); ++n)
      { vector<string> field = split_string(fields[n], ":");

        _sent_exchange.push_back( { remove_peripheral_spaces(field[0]), remove_peripheral_spaces(field[1]) } );
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
      _normalise_rate = is_true;;

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
    if (starts_with(testline, "POINTS"))  // there may be an "=" in the points definitions
    { const vector<string> str_vec = split_string(line, "=");

      if (!str_vec.empty())
      { string tmp_points_str;
        const string lhs = str_vec[0];

        if (!contains(lhs, "[") or contains(lhs, "[*]"))             // for all bands
        { string new_str;

          for (unsigned int n = 1; n < str_vec.size(); ++n)          // reconstitute rhs; why not just _points = RHS ? I think that comes to the same thing
          { new_str += str_vec[n];
            if (n != str_vec.size() - 1)
              new_str += "=";
          }

          tmp_points_str = to_upper(remove_peripheral_spaces(new_str));

          for (unsigned int n = 0; n < NUMBER_OF_BANDS; ++n)
            _per_band_points.insert( {static_cast<BAND>(n), tmp_points_str} );
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

              _per_band_points.insert( {b, tmp_points_str} );
            }
          }
        }
      }
    }

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

      for_each(vec_rates.cbegin(), vec_rates.cend(), [&new_rates] (const string& str) { new_rates.push_back(from_string<unsigned int>(str)); } );

      if (!new_rates.empty())
        _rate_periods = new_rates;
    }

// RBN PORT
    if (starts_with(testline, "RBN PORT"))
      _rbn_port = from_string<int>((split_string(line, "="))[1]);

// RBN SERVER
    if (starts_with(testline, "RBN SERVER"))
      _rbn_server = rhs;

// RBN THRESHOLD
    if (starts_with(testline, "RBN THRESHOLD"))
      _rbn_threshold = from_string<unsigned int>(rhs);

// RBN USERNAME
    if (starts_with(testline, "RBN USERNAME"))
      _rbn_username = rhs;

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

// SCREEN SNAPSHOT FILE
    if (starts_with(testline, "SCREEN SNAPSHOT FILE"))
      _screen_snapshot_file = rhs;

// SHIFT DELTA
    if (starts_with(testline, "SHIFT DELTA"))
      _shift_delta = from_string<unsigned int>(rhs);

// SHIFT POLL
    if (starts_with(testline, "SHIFT POLL"))
      _shift_poll = from_string<unsigned int>((split_string(line, "="))[1]);

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

// WORKED MULTS COLOUR
    if (starts_with(testline, "WORKED MULTS COLOUR") or starts_with(testline, "WORKED MULTS COLOR"))
      _worked_mults_colour = RHS;

// ---------------------------------------------  MULTIPLIERS  ---------------------------------


// COUNTRY MULTS
// Currently supported: ALL
//                      NONE
    if (starts_with(testline, "COUNTRY MULTS") and !starts_with(testline, "COUNTRY MULTS PER BAND"))
    { _country_mults_filter = RHS; // to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

//      ost << "_country_mults_filter = " << _country_mults_filter << endl;

      if (_country_mults_filter == "NONE")
        QSO_DISPLAY_COUNTRY_MULT = false;                  // since countries aren't mults, don't take up space in the log line
    }

// REMAINING CALLSIGN MULTS
    if (starts_with(testline, "REMAINING CALLSIGN MULTS"))
    { _auto_remaining_callsign_mults = (RHS == "AUTO");

      if (!_auto_remaining_callsign_mults)
      { const vector<string> mults = remove_peripheral_spaces(split_string(RHS, ","));

        _remaining_callsign_mults_list = set<string>(mults.cbegin(), mults.cend());

//        for (vector<string>::const_iterator cit = mults.begin(); cit != mults.end(); ++cit)
//          _remaining_callsign_mults_list.insert(*cit);
      }
    }

// REMAINING COUNTRY MULTS
    if (starts_with(testline, "REMAINING COUNTRY MULTS"))
    { _auto_remaining_country_mults = (RHS == "AUTO");

      if (!_auto_remaining_country_mults)
      { const vector<string> countries = remove_peripheral_spaces(split_string(RHS, ","));

        _remaining_country_mults_list = set<string>(countries.cbegin(), countries.cend());

//        for (vector<string>::const_iterator cit = countries.begin(); cit != countries.end(); ++cit)
//          _remaining_country_mults_list.insert(to_upper(remove_peripheral_spaces(*cit)));
      }
    }

// ---------------------------------------------  CABRILLO  ---------------------------------

// CABRILLO CONTEST
    if (starts_with(testline, "CABRILLO CONTEST"))
    { if (is_legal_value(RHS, "AP-SPRINT,ARRL-10,ARRL-160,ARRL-DX-CW,ARRL-DX-SSB,ARRL-SS-CW,ARRL-SS-SSB,ARRL-UHF-AUG,ARRL-VHF-JAN,ARRL-VHF-JUN,ARRL-VHF-SEP,ARRL-RTTY,BARTG-RTTY,CQ-160-CW,CQ-160-SSB,CQ-WPX-CW,CQ-WPX-RTTY,CQ-WPX-SSB,CQ-VHF,CQ-WW-CW,CQ-WW-RTTY,CQ-WW-SSB,DARC-WAEDC-CW,DARC-WAEDC-RTTY,DARC-WAEDC-SSB,FCG-FQP,IARU-HF,JIDX-CW,JIDX-SSB,NAQP-CW,NAQP-RTTY,NAQP-SSB,NA-SPRINT-CW,NA-SPRINT-SSB,NCCC-CQP,NEQP,OCEANIA-DX-CW,OCEANIA-DX-SSB,RDXC,RSGB-IOTA,SAC-CW,SAC-SSB,STEW-PERRY,TARA-RTTY", ","))
        _cabrillo_contest = RHS;                        // required to be upper case
    }

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
    if (starts_with(testline, "CABRILLO CATEGORY-ASSISTED"))
    { const string value = RHS;

      if (is_legal_value(value, "ASSISTED,NON-ASSISTED", ","))
        _cabrillo_category_assisted = value;
    }

// CABRILLO CATEGORY-BAND
    if (starts_with(testline, "CABRILLO CATEGORY-BAND"))
    { const string value = remove_peripheral_spaces((split_string(line, "="))[1]);

// The spec calls for bizarre capitalization
      if (is_legal_value(value, "ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light", ","))
        _cabrillo_category_band = value;

    }

// CABRILLO CATEGORY-MODE
    if (starts_with(testline, "CABRILLO CATEGORY-MODE"))
    { const string value = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "CW,MIXED,RTTY,SSB", ","))
        _cabrillo_category_mode = value;
    }

// CABRILLO CATEGORY-OPERATOR
    if (starts_with(testline, "CABRILLO CATEGORY-OPERATOR"))
    { const string value = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "CHECKLOG,MULTI-OP,SINGLE-OP", ","))
        _cabrillo_category_operator = value;
    }

// CABRILLO CATEGORY-OVERLAY
    if (starts_with(testline, "CABRILLO CATEGORY-OVERLAY"))
    { const string value = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

      if (is_legal_value(value, "NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES", ","))
        _cabrillo_category_operator = value;
    }

// CABRILLO CATEGORY-POWER
    if (starts_with(testline, "CABRILLO CATEGORY-POWER"))
    { const string value = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

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

    if (starts_with(testline, "CABRILLO QSO"))
      _cabrillo_qso_template = to_upper(remove_peripheral_spaces((split_string(line, "="))[1]));

// ---------------------------------------------  WINDOWS  ---------------------------------

    if (starts_with(testline, "WINDOW"))
    { vector<string> window_info = split_string(split_string(testline, "=")[1], ",");

      for (size_t n = 0; n < window_info.size(); ++n)
        window_info[n] = remove_peripheral_spaces(window_info[n]);

      if (window_info.size() >= 5)
      { const string name = window_info[0];

        window_information winfo;
        winfo.x(from_string<int>(window_info[1]));
        winfo.y(from_string<int>(window_info[2]));
        winfo.w(from_string<int>(window_info[3]));
        winfo.h(from_string<int>(window_info[4]));

        if (window_info.size() >= 6)
        { winfo.fg_colour(window_info[5]);
//          cout << "foreground colour = " << winfo.fg_colour() << endl;

          if (window_info.size() >= 7)
          { winfo.bg_colour(window_info[6]);
 //           cout << "background colour = " << winfo.bg_colour() << endl;
          }

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
    { vector<string> window_info = remove_peripheral_spaces(split_string(split_string(testline, "=")[1], ","));

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
              { winfo.bg_colour(window_info[4]);
              }

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

//              ost << "found keysym for equivalent key = " << hex << keysym << dec << endl;

//              const map<int, string>::const_iterator cit3 = _messages.find(keysym);

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

  if (!_exchange_fields_filename.empty())
    EXCHANGE_FIELD_TEMPLATES.prepare(_path, _exchange_fields_filename);

  if (_qsl_message.empty())
    _qsl_message = "tu " + _my_call + " test";

  if (_quick_qsl_message.empty())
    _quick_qsl_message = "tu " + _my_call;

  if (_message_cq_1.empty())
    _message_cq_1 = "test " + _my_call + " " + _my_call + " test";

  if (_message_cq_2.empty())
    _message_cq_2 = "cq cq test de  " + _my_call + "  " + _my_call + "  " + _my_call + "  test";
}

/// construct from file
drlog_context::drlog_context(const std::string& filename) :
  _archive_name("drlog-restart"),                                // name for the archive written when leaving drlog
  _auto_backup(""),                                              // no auto backup directory
  _auto_remaining_country_mults(false),                          // do not add country mults as we detect them
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
  _cluster_port(23),                          // standard telnet server port
  _country_list(COUNTRY_LIST_WAEDC),          // WAE country list
  _country_mults_filter("ALL"),               // all countries are mults
  _country_mults_per_band(true),              // any country mults are per-band
  _cq_auto_lock(false),                       // don't lock the TX frequency in CQ mode
  _cq_auto_rit(false),                        // don't enable RIT in CQ mode
  _cty_filename("cty.dat"),                   // filename for country data
  _cw_speed(29),                              // 29 WPM
  _do_not_show(),                             // all calls (apart from my_call()) should be shown on the bandmap
  _do_not_show_filename(),                    // no do-not-show file
  _drmaster_filename("drmaster"),             // name of the drmaster file
  _exchange("RST"),                           // exchange is just RST
  _exchange_fields_filename(),                // file that holds regex templates for exchange fields
  _exchange_mults_per_band(false),            // any exchange mults are once-only
  _guard_band( { { MODE_CW, 500 }, { MODE_SSB, 2000 } } ),  // 500 Hz guard band on CW, 2 kHz on slopbucket
  _individual_messages_file(),                // no file of individual messages
  _logfile("drlog.dat"),                      // name of log file
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
  _qsl_message(),                             // no QSL message (default is changed once configuration file has been read)
  _qso_multiple_bands(false),                 // each station may be worked on only one band
  _qso_multiple_modes(false),                 // each station may be worked on only one mode
  _quick_qsl_message(),                       // no quick QSL message (default is changed once configuration file has been read)
  _rate_periods( { 15, 30, 60 } ),            // 15-, 30-, 60-minute periods for rates
  _rbn_port(7000),                            // telnet port for the reverse beacon network
  _rbn_server("telnet.reversebeacon.net"),    // domain name of the reverse beacon network telnet server
  _rbn_threshold(1),                          // all received spots are posted
  _rbn_username(""),                          // no default name to access the RBN
  _remaining_country_mults_list(),            // no remaining country mults
  _rig1_baud(4800),                           // 4800 baud
  _rig1_data_bits(8),                         // 8-bit data
  _rig1_port("/dev/ttyS0"),                   // first serial port
  _rig1_stop_bits(1),                         // one stop bit
  _rig1_type(""),                             // no default rig type
  _russian_filename("russian-data"),          // default file for Russian location information
  _screen_snapshot_file("screen"),            // screen snapshots will be in screen-<n>
  _shift_delta(20),                           // shift RIT by 20 Hz
  _shift_poll(10),                            // poll every 10 milliseconds (this is much too frequent for a K3 to respond)
  _start_band(BAND_20),                       // start on 20m
  _start_mode(MODE_CW),                       // start on CW
  _sync_keyer(false),                         // do not synchronise rig keyer with computer
  _test(false),                               // transmit is not disabled
  _worked_mults_colour("RED")                 // worked mults are in red
{ _process_configuration_file(filename);

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
}

/// location and size of a particular window
const window_information drlog_context::window_info(const string& name) const
{ const auto cit = _windows.find(name);

  return (cit == _windows.cend() ? window_information() : cit->second);
}

const vector<string> drlog_context::window_name_contains(const string& substr) const
{ vector<string> rv;

  for (auto cit = _windows.cbegin(); cit != _windows.cend(); ++cit)
    if (contains(cit->first, substr))
      rv.push_back(cit->first);

  return rv;
}
