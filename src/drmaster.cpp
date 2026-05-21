// $Id: drmaster.cpp 295 2026-05-17 12:40:09Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   drmaster.cpp

    Classes associated with MASTER.DTA, TRMASTER.[DTA, ASC] and drmaster files.

    Parts of this file are based on the very old trm.cpp for TRLOG manipulation
*/

#include "diskfile.h"
#include "drmaster.h"
#include "log_message.h"
#include "textfile.h"

using namespace std;

extern message_stream    ost;       ///< debugging/logging output

// 260505: support for TRMASTER ASCII and binary files removed

/*  \brief          At what value is a particular percentage point?
    \param  values  container of values
    \param  pc      percentage point at which one desires the value
    \return         value at the <i>pc</i>th percentile

    <i>pc</i> = 100 => all match
    <i>pc</i> = 0 => none match
*/
template <typename C>
typename C::value_type value_line(const C& values, const int pc)
{ if (values.empty())
    return std::numeric_limits<typename C::value_type>::max();

// put the values in the container into a vector that can be sorted
  std::vector<typename C::value_type> ordered_vector;
  ordered_vector.reserve(values.size());

  FOR_ALL(values, [&ordered_vector] (const auto& v) { ordered_vector += v; });

  SORT(ordered_vector);

  const int clamped_pc { std::clamp(pc, 0, 100) };

  if (clamped_pc == 100)
    return (ordered_vector[0]);     // all match

  if (clamped_pc == 0)
    return ordered_vector[ordered_vector.size() + 1];   // none match

  const size_t idx { static_cast<size_t>((values.size() * (100 - static_cast<float>(clamped_pc)) / 100) + 0.5) };

  return ordered_vector.at(idx);
}

// -----------------------------------------------------  drmaster_line  ---------------------------------

/*! \class  drmaster_line
    \brief  Manipulate a line from a drmaster file
*/

/* extensions

z = date
y = CW power
x = SSB power
w = IOTA island number
v = IARU society
u = Sweepstakes precedence
t = state for ARRL 10m contest (W, VE, XE only)
s = state for ARRL 160m contest (W, VE, only)
r = SKCC state/province/country
q = SKCC number
p = XSCP value
o = QTH2 (alternative QTH, as in KCJ prefectures, for example)
n = encoded age from AA CW YYYYMM:AGE
m = encoded age from AA SSB YYYYMM:AGE
*/

/*! \brief                      Extract a single field from the record
    \param  fields              all the fields (e.g., "=Xabc")
    \param  field_indicator     indicator that prefixes the field (for example: "=H")
    \return                     Value of the field with the indicator <i>field_indicator</i>

    Returns empty string if no field has the indicator <i>field_indicator</i>

TODO: make this a template that works on a vector of strings or a vector of string_views
*/
string drmaster_line::_extract_field(const vector<string>& fields, const string_view field_indicator)
{ const auto it { FIND_IF(fields, [field_indicator] (const auto& field) { return field.starts_with(field_indicator); }) };

  return ( (it == fields.end()) ? string { } : it -> substr(field_indicator.length()));
}

/*! \brief      Process a single field (such as: "=Xabc")
    \param  sv  the field to processs (e.g., "=Xabc")

    Does nothing if the field does not exist
*/
void drmaster_line::_process_field(const std::string_view sv)
{ using SETTER = void (drmaster_line::*) (const string&);

  static const map<char, SETTER> c_to_member { { 'K', &drmaster_line::check },      // the first group is from TRMASTER
                                               { 'C', &drmaster_line::cq_zone },
                                               { 'F', &drmaster_line::foc },
                                               { 'G', &drmaster_line::grid },
                                               { 'H', &drmaster_line::hit_count },
                                               { 'I', &drmaster_line::itu_zone },
                                               { 'N', &drmaster_line::name },
                                               { 'Q', &drmaster_line::qth },
                                               { 'A', &drmaster_line::section },
                                               { 'T', &drmaster_line::ten_ten },

                                               { 'n', &drmaster_line::age_aa_cw },  // drmaster extensions
                                               { 'm', &drmaster_line::age_aa_ssb },
                                               { 'y', &drmaster_line::cw_power },
                                               { 'z', &drmaster_line::date },
                                               { 'w', &drmaster_line::iota },
                                               { 'u', &drmaster_line::precedence },
                                               { 'o', &drmaster_line::qth2 },
                                               { 'q', &drmaster_line::skcc },
                                               { 'v', &drmaster_line::society },
                                               { 'r', &drmaster_line::spc },
                                               { 'x', &drmaster_line::ssb_power },
                                               { 's', &drmaster_line::state_160 },
                                               { 't', &drmaster_line::state_10 }
  };

  const char   c      { sv[1] };
  const string svalue { sv.substr(2) };

  if ( (c >= 'U') and (c <= 'Y') )          // TRMASTER User parameters
  { const int n { int(c) - int('U') };

    user(n, svalue);
    return;
  }

  if (c == 'p')
  { xscp(from_string<decltype(_xscp)>(svalue));
    return;
  }

  const auto it { c_to_member.find(c) };

  if (it == c_to_member.end())
  { ost << "Error looking up drmaster field " << sv << endl;
    return;
  }

  const SETTER pf { it -> second };

  std::invoke(pf, this, svalue);
}

/*! \brief                  Construct from a call or from a line from a drmaster file
    \param  line_or_call    line from file, or a call

    Constructs an object that contains only the call if <i>line_or_call</i> contains a call
*/
drmaster_line::drmaster_line(const string_view line_or_call)
{ if (line_or_call.empty())
    return;

  size_t space_posn { line_or_call.find(SPACE) };

  if (space_posn == string::npos) // no space
  { _call = line_or_call;

    return;
  }

  _call = substring <string> (line_or_call, 0, space_posn);

// all the other fields
  while (space_posn != string::npos)
  { if (const size_t eq_posn { line_or_call.find(EQUALS, space_posn) }; eq_posn != string::npos)      // found an equals
    { space_posn = line_or_call.find(SPACE, eq_posn);

      const string_view next_field { (space_posn == string::npos) ? substring <string_view> (line_or_call, eq_posn)
                                                                  : substring <string_view> (line_or_call, eq_posn, space_posn - eq_posn) };

      _process_field(next_field);
    }
  }
}

/// convert to string
string drmaster_line::to_string(void) const
{ string rv;

  rv += call();

  using VOID_PARAM = const string& (drmaster_line::*)(void) const&;     // the signature for the getter functions

  auto insert_if_not_empty = [&rv, this] (VOID_PARAM fn, const char c) { if (const string value { std::invoke(fn, *this) }; !value.empty())
                                                                           rv += ( " ="s + c + value );
                                                                       };

  insert_if_not_empty(&drmaster_line::section,   'A');
  insert_if_not_empty(&drmaster_line::cq_zone,   'C');
  insert_if_not_empty(&drmaster_line::foc,       'F');
  insert_if_not_empty(&drmaster_line::grid,      'G');
  insert_if_not_empty(&drmaster_line::hit_count, 'H');
  insert_if_not_empty(&drmaster_line::itu_zone,  'I');
  insert_if_not_empty(&drmaster_line::check,     'K');
  insert_if_not_empty(&drmaster_line::name,      'N');
  insert_if_not_empty(&drmaster_line::qth,       'Q');
  insert_if_not_empty(&drmaster_line::ten_ten,   'T');

// U, V, W, X, Y (user parameters)
  char user_letter { 'U' };

  for (unsigned int n { 0 }; n < TRMASTER_N_USER_PARAMETERS; n++)
  { if (!user(n + 1).empty())
    { rv += " ="s;
      rv += user_letter;
      rv += user(n + 1);
    }
    user_letter++;
  }

  insert_if_not_empty(&drmaster_line::age_aa_ssb, 'm');
  insert_if_not_empty(&drmaster_line::age_aa_cw,  'n');
  insert_if_not_empty(&drmaster_line::cw_power,   'y');
  insert_if_not_empty(&drmaster_line::date,       'z');
  insert_if_not_empty(&drmaster_line::iota,       'w');
  insert_if_not_empty(&drmaster_line::precedence, 'u');
  insert_if_not_empty(&drmaster_line::qth2,       'o');
  insert_if_not_empty(&drmaster_line::skcc,       'q');
  insert_if_not_empty(&drmaster_line::society,    'v');
  insert_if_not_empty(&drmaster_line::spc,        'r');
  insert_if_not_empty(&drmaster_line::ssb_power,  'x');
  insert_if_not_empty(&drmaster_line::state_160,  's');
  insert_if_not_empty(&drmaster_line::state_10,   't');

  if (xscp() != 0)
    rv += " =p"s + ::to_string(xscp());

  return rv;
}

// merge with another drmaster_line; new values take precedence if there's a conflict
drmaster_line drmaster_line::operator+(const drmaster_line& drml) const
{ if (call() != drml.call())
    return *this;

  drmaster_line rv { drml };

  using VOID_PARAM = const string& (drmaster_line::*) (void) const&;
  using STR_PARAM = void (drmaster_line::*) (const string&);

  auto insert_if_empty = [&rv, this] (VOID_PARAM fn1, STR_PARAM fn2) { if (std::invoke(fn1, rv).empty())
                                                                         std::invoke( fn2, rv, std::invoke(fn1, *this) );
                                                                     };

#define INSERT_IF_EMPTY(F) insert_if_empty( &drmaster_line::F, &drmaster_line::F )

// trlog fields
  INSERT_IF_EMPTY(check);
  INSERT_IF_EMPTY(cq_zone);
  INSERT_IF_EMPTY(foc);
  INSERT_IF_EMPTY(grid);
  INSERT_IF_EMPTY(hit_count);
  INSERT_IF_EMPTY(itu_zone);
  INSERT_IF_EMPTY(name);
  INSERT_IF_EMPTY(qth);
  INSERT_IF_EMPTY(section);
  INSERT_IF_EMPTY(ten_ten);

// additional drlog fields
  INSERT_IF_EMPTY(age_aa_cw);
  INSERT_IF_EMPTY(age_aa_ssb);
  INSERT_IF_EMPTY(cw_power);
  INSERT_IF_EMPTY(iota);
  INSERT_IF_EMPTY(precedence);
  INSERT_IF_EMPTY(qth2);
  INSERT_IF_EMPTY(skcc);
  INSERT_IF_EMPTY(society);
  INSERT_IF_EMPTY(spc);
  INSERT_IF_EMPTY(ssb_power);
  INSERT_IF_EMPTY(society);
  INSERT_IF_EMPTY(state_160);
  INSERT_IF_EMPTY(state_10);

#undef INSERT_IF_EMPTY

  for (unsigned int n { 1 }; n <= TRMASTER_N_USER_PARAMETERS; n++)
  { if (rv.user(n).empty())
      rv.user(n, user(n));
  }

  if (rv.date().empty())
    rv.date(substring <std::string> (date_time_string(SECONDS::NO_INCLUDE), 0, 8));

  if (xscp() != 0)
    rv.xscp(xscp());

  return rv;
}

// -----------------------------------------------------  drmaster  ---------------------------------

/*! \class  drmaster
    \brief  Manipulate a drmaster file

    A drmaster file is a superset of a TRMASTER.ASC file
*/

/*! \brief              Construct from a file
    \param  filename    name of file to read
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Lines without XSCP data are always included

    Throws exception if the file does not exist or is incorrectly formatted;
    except creates empty object if called with default filename that does not exist
*/
drmaster::drmaster(const string_view filename, const int xscp_limit)
{ if (!filename.empty() and file_exists(filename))
  { auto convert_line { [this, xscp_limit] (const auto& line) { if ( const drmaster_line record { line }; (record.xscp() == 0) or (record.xscp() >= xscp_limit) )
                                                                  _records += { record.call(), record } ;
                                                              } };

    FOR_ALL(textfile(filename), convert_line);

// SEE: https://stackoverflow.com/questions/67321666/generator-called-twice-in-c20-views-pipeline     !!!!!
// AND: https://stackoverflow.com/questions/64199664/why-c-ranges-transform-filter-calls-transform-twice-for-values-that-match
// AND: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2760r1.html#cache_last

// I am increasingly coming to the conclusion that while ranges look great in theory, there are far too many "gotchas" that
// cause one's code not to behave the way one would reasonably expect. See any of Nico Josuttis' talks on ranges for some
// of those problems (although, as far as I have seen, not the one that I've hit here).

// THIS CAUSES EACH LINE TO BE WRITTEN TWICE -- I  DON'T KNOW WHY; the version above, without the pipe, prints each line only once
//    FOR_ALL( tfile | std::views::transform([&count] (const string& str) { ost << ++count << ": " << str << endl; return drmaster_line { str };})
//                   | std::views::filter   ([xscp_limit] (const drmaster_line& rec) { return (rec.xscp() == 0) or (rec.xscp() >= xscp_limit); }),
//                   [] (const drmaster_line& drml) { ost << drml.call() << endl; } ) ;

    ost << "read " << css(_records.size()) << " drmaster records from file" << endl;
  }

#if 0
  if (!filename.empty() and file_exists(filename))
  { try
    { //_prepare_from_file_contents(read_file(filename), xscp_limit);      // throws exception if fails

      _burble(filename, xscp_limit);

//      static_assert(std::ranges::input_range<textfile>);      // OK
//      static_assert(std::input_iterator<textfile_iterator>);  // OK

#if 0
      auto tfile { textfile(filename) };

// I don't know why I can't say "textfile(filename)" instead of just "t"; apparently an lvalue is needed, for reasons that I don't understand
      tfile| std::views::transform([this] (const string& str) { return drmaster_line { str }; })
           | std::views::filter([this, &xscp_limit] (const drmaster_line& rec) { return (rec.xscp() == 0) or (rec.xscp() >= xscp_limit); })
           | std::views::transform([this] (const auto& rec) { _records += { rec.call(), rec }; return true; });    // I don't know why I have to return a value; probably something to do with "transform"
#endif
//      ost << "read " << _records.size() << " drmaster records from file [1]" << endl;

//      auto t = textfile(filename);

//      t | std::views::transform([] (const string& str) { return drmaster_line { str }; })
//                         | std::ranges::views::filter([&xscp_limit] (const drmaster_line& rec) { return (rec.xscp() == 0) or (rec.xscp() >= xscp_limit); })
//                         | std::ranges::views::transform([this] (const drmaster_line& rec) { _records += { rec.call(), rec }; });


// this also works
#if 0
      auto convert_line { [this, xscp_limit] (const auto& line) { //const drmaster_line record { line };

                                                                  if ( const drmaster_line record { line }; (record.xscp() == 0) or (record.xscp() >= xscp_limit) )
                                                                    _records += { record.call(), record } ;
                                                                } };

      FOR_ALL(textfile(filename), convert_line);

      ost << "read " << _records.size() << " drmaster records from file" << endl;
#endif
    }

    catch (...)
    { if (filename != "drmaster"sv)      // don't fail if we couldn't find the default file; simply create empty object
        throw;
    }
  }
#endif

}

/*! \brief              Construct from a file
    \param  path        directories to check
    \param  filename    name of file to read
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Lines without XSCP data are always included
    Constructs from the first instance of <i>filename</i> when traversing the <i>path</i> directories.
    Throws exception if the file does not exist or is incorrectly formatted
*/
drmaster::drmaster(const vector<string>& path, const string_view filename, const int xscp_limit)
{ if (!filename.empty())
  { if (const string fname { find_file(path, filename) }; !fname.empty())
      *this = drmaster { fname, xscp_limit };
  }
}

/// all the calls (in callsign order)
vector<string> drmaster::calls(void) const
{ vector<string> rv { unordered_calls() };

  SORT(rv, compare_calls);

  return rv;
}

/// all the calls (in random order)
vector<string> drmaster::unordered_calls(void) const
{ vector<string> rv;
  rv.reserve(_records.size());

  for (const auto& [ call, drml ] : _records)
    rv += call;

  return rv;
}

/// format for output
string drmaster::to_string(void) const
{ vector<string> lines;
  lines.reserve(_records.size());

  for (const auto& [ call, drml ] : _records)
    lines += (drml.to_string() + EOL);

  SORT(lines);

  return ( join(lines, string { }) );            // don't add the default (space) separator
}

/*! \brief          Add a callsign.
    \param  call    call to add

    If there's already an entry for <i>call</i>, then does nothing
*/
void drmaster::operator+=(const string_view call)
{ if (!call.contains(SPACE) and !_records.contains(call))     // basic sanity check for a call, and whether is already in the database
  { const string call_str { call };

    _records += { call, static_cast<drmaster_line>(call) };    // cast needed in order to keep the temporary around long enough to use
  }
}

/*! \brief          Add a drmaster_line
    \param  drml    drmaster line to add

    If there's already an entry for the call in <i>drml</i>, then performs a merge
*/
void drmaster::operator+=(const drmaster_line& drml)
{ const string call { drml.call() };

  if (!_records.contains(call))
    _records += { call, drml };
  else
  { drmaster_line new_drml { _records[call] };

    new_drml += drml;
    _records -= call;
    _records += { call, new_drml };
  }
}

/*! \brief      Return object with only records with xscp below a given percentage value
    \param  pc  percentage limit
    \return     <i>drmaster</i> object containing only records with no xscp, and those for which the xscp value is >= the value of <i>pc</i>
*/
drmaster drmaster::prune(const int pc) const
{ drmaster    rv;
  vector<int> xscp_values;

  for (const auto& [ call, line ] : _records)
  { if (line.xscp() == 0)
      rv += line;
    else
      xscp_values += line.xscp();
  }

  ost << "number of XSCP = 0 values = " << css(rv.size()) << endl;
  ost << "number of non-zero values = " << css(xscp_values.size()) << endl;

  const int breakpoint_value { value_line(xscp_values, pc) };

  ost << "breakpoint value = " << breakpoint_value << "; values >= this value are retained" << endl;

  for (const auto& [ call, line ] : _records)
    if (line.xscp() >= breakpoint_value)
      rv += line; 

  return rv;
}
