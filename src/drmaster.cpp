// $Id: drmaster.cpp 259 2025-01-19 15:44:33Z  $

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

// -----------------------------------------------------  master_dta  ---------------------------------

/*! \class  master_dta
    \brief  Manipulate a K1EA MASTER.DTA file

    The first thing in the file is an array that has the locations of the
    various cells.

    SCPIndexArrayType    = ARRAY [0..36, 0..36] OF LONGINT;

*/

/*! \brief              private function to return the call starting at <i>posn</i> in <i>contents</i>
    \param  contents    contents of a MASTER.DTA file
    \param  posn        location at which to start returning call
    \return             call that starts at <i>posn</i> in <i>contents</i>

    <i>posn</i> is updated to point at the start of the next call
*/
string master_dta::_get_call(const string_view contents, uint32_t& posn) const
{ string rv;

  while ((posn < contents.length()) and static_cast<int>(contents[posn]) != 0)
    rv += contents[posn++];

  ++posn;       // increment past the null

  return rv;
}

/*! \brief              construct from file
    \param  filename    file name

    Also default constructor, with filename "master.dta"
*/
master_dta::master_dta(const string_view filename)
{ const string contents { read_file(filename) };                // throws exception if there's a problem

  if (contents.length() < sizeof(uint32_t))
    return;

// point to the start of the calls
  uint32_t pointer { static_cast<uint32_t>(contents[0] + ((contents[1] + ((contents[2] + (contents[3] << 8)) << 8)) << 8)) };

  vector<string> tmp_calls;

// extract all the calls
  while (pointer < contents.length())
    tmp_calls += _get_call(contents, pointer);      // modified pointer

// remove duplicates
  SORT(tmp_calls);

  vector<string>::iterator pos { unique(tmp_calls.begin(), tmp_calls.end()) };

  ranges::copy(tmp_calls.begin(), pos, back_inserter(_calls));          // put calls into the permanent container
}

// -----------------------------------------------------  trmaster_line  ---------------------------------

/*! \class  trmaster_line
    \brief  Manipulate a line from an N6TR TRMASTER.ASC file
*/

/*! \brief          extract the value of a parameter from a TRMASTER.ASC line
    \param  line    line from the TRMASTER.ASC file
    \param  param   parameter to extract, e.g., "=H"
    \return         the value associated with <i>param</i>

    Returns empty string if there is no value associated with <i>param</i>
*/
string __extract_value(const string_view line, const string& param)
{ if (!contains(line, param))
    return string { };

  const size_t      start_position { line.find(param) };
  const string_view short_line     { substring <std::string_view> (line, start_position + param.length()) };
  const size_t      space_posn     { short_line.find(' ') };
  const size_t      end_position   { ( (space_posn == string_view::npos) ? short_line.length() : space_posn ) };

  return substring <std::string> (short_line, 0, end_position);
}

/* According to Tree, the following are the keys in a TRMASTER file:

A - ARRL section
C - CQ Zone
F - FOC
G - Grid
H - Hits
I - ITU Zone    *** WHICH MAY CONTAIN LETTERS
K - Check
N - Name
O - Old Call
Q - QTH
S - Speed
T - TenTen
U - User 1
V - User 2
W - User 3
X - User 4
Y - User 5

*/

/* extensions

z = date
y = CW power
x = SSB power
w = IOTA island number
v = IARU society

t = temporary

*/

// We use:
//   User1 => SS precedence
//   User2 => ARRL DX power

/*! \brief          Construct from a TRMASTER.ASC line
    \param  line    line from the TRMASTER.ASC file
*/
trmaster_line::trmaster_line(const string_view line)
{
// parsing the line is tricky because items are in no particular order, except for the call;
// call
  const size_t length_of_call { (contains(line, ' ') ? line.find(' ') : line.length()) };

  _call = to_upper(substring <std::string> (line, 0, length_of_call));

  _check     = from_string<int>( __extract_value(line, "=K"s) );
  _cq_zone   = from_string<int>( __extract_value(line, "=C"s) );
  _foc       = from_string<int>( __extract_value(line, "=F"s) );
  _grid      = __extract_value(line, "=G"s);
  _hit_count = from_string<int>( __extract_value(line, "=H"s) );
  _itu_zone  = __extract_value(line, "=I"s);
  _name      = __extract_value(line, "=N"s);
  _qth       = __extract_value(line, "=Q"s);
  _section   = __extract_value(line, "=A"s);
  _ten_ten   = from_string<int>( __extract_value(line, "=T"s) );

// user parameters
  char user_parameter { 'U' };

  for (unsigned int n_user_parameter { 0 }; n_user_parameter < TRMASTER_N_USER_PARAMETERS; ++n_user_parameter)
  { char marker[3];

    marker[0] = '=';
    marker[1] = user_parameter;
    marker[2] = 0;                // marker == "=x"

    _user[n_user_parameter] = __extract_value(line, marker);
    user_parameter++;
  }
}

/*! \brief      Convert to a string
    \return     the line as a string suitable for use in a TRMASTER file
*/
string trmaster_line::to_string(void) const
{ string rv { call() };

  using VOID_PARAM = const string& (trmaster_line::*)(void) const&;     // the signature for the getter functions

  auto insert_if_not_empty = [&rv, this] (VOID_PARAM fn, const char c) { if (const string value { std::invoke(fn, *this) }; !value.empty())
                                                                           rv += ( " ="s + c + value );
                                                                       };

  insert_if_not_empty(&trmaster_line::section,  'A');
  insert_if_not_empty(&trmaster_line::grid,     'G');
  insert_if_not_empty(&trmaster_line::itu_zone, 'I');
  insert_if_not_empty(&trmaster_line::name,     'N');
  insert_if_not_empty(&trmaster_line::qth,      'Q');

  using VOID_PARAM_INT = const int& (trmaster_line::*)(void) const&;     // the signature for the getter functions

  auto insert_if_not_empty_int = [&rv, this] (VOID_PARAM_INT fn, const char c) { if (const int value { std::invoke(fn, *this) }; (value != 0))
                                                                                   rv += ( " ="s + c + ::to_string(value) );
                                                                               };

  insert_if_not_empty_int(&trmaster_line::cq_zone,   'C');
  insert_if_not_empty_int(&trmaster_line::foc,       'F');
  insert_if_not_empty_int(&trmaster_line::hit_count, 'H');
  insert_if_not_empty_int(&trmaster_line::check,     'K');
  insert_if_not_empty_int(&trmaster_line::ten_ten,   'T');

  char user_letter { 'U' };

  for (unsigned int n { 0 }; n < TRMASTER_N_USER_PARAMETERS; n++)
  { if (!user(n + 1).empty())
    { rv += " ="s;
      rv += user_letter;
      rv += user(n + 1);
    }

    user_letter++;
  }

  return rv;
}

/*! \brief          Merge with another trmaster_line
    \param  trml    line to be merged
    \return         line merged with <i>trml</i>

    New values (i.e., values in <i>trml</i>) take precedence if there's a conflict
*/
trmaster_line trmaster_line::operator+(const trmaster_line& trml) const
{ if (call() != trml.call())        // check that the calls match
    return *this;

  trmaster_line rv { *this };                                  // copy the old line

  using VOID_PARAM = const string& (trmaster_line::*) (void) const&;
  using STR_PARAM = void (trmaster_line::*) (const string&);

// is this right, or do we need an insert_if_not_empty? or should I ditch trmaster entirely, as I dont use it?
  auto insert_if_empty = [&rv, this] (VOID_PARAM fn1, STR_PARAM fn2) { if (std::invoke(fn1, rv).empty())
                                                                         std::invoke( fn2, rv, std::invoke(fn1, *this) );
                                                                     };

#define INSERT_IF_EMPTY(F) insert_if_empty( &trmaster_line::F, &trmaster_line::F )

// go through the fields seriatim
  rv.hit_count(rv.hit_count() + trml.hit_count() + 1);      // may change this

  INSERT_IF_EMPTY(qth);

//  if ((!trml.qth().empty()))
//    rv.qth(trml.qth());

  for (unsigned int n { 1 }; n <= TRMASTER_N_USER_PARAMETERS; n++)
  { if (!trml.user(n).empty())
      rv.user(n, trml.user(n));
  }

  if (trml.cq_zone() != 0)
    rv.cq_zone(trml.cq_zone());

  if ((!trml.itu_zone().empty()))
    rv.itu_zone(trml.itu_zone());

  if ((!trml.section().empty()))
    rv.section(trml.section());

  if (trml.check() != 0)
    rv.check(trml.check());

  if ((!trml.name().empty()))
    rv.name(trml.name());

  if ((!trml.grid().empty()))
    rv.grid(trml.grid());

  if (trml.ten_ten() != 0)
    rv.ten_ten(trml.ten_ten());

  if (trml.foc() != 0)
    rv.foc(trml.foc());

#undef INSERT_IF_EMPTY

  return rv;
}

/*! \brief          Output a line from a TRMASTER file to a stream
    \param  ost     output stream
    \param  trml    the line to be written
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const trmaster_line& trml)
{ ost << trml.to_string();
  return ost;
}

// -----------------------------------------------------  trmaster  ---------------------------------

/*! \class  trmaster
    \brief  Manipulate an N6TR TRMASTER file
*/

/*
  The entry is still null terminated.  However after the callsign - instead of
a null - there could be a control character that defines what the next field
of data will be.  That field will be terminated by another control character
- defining a new field of data associated with the same callsign - or a null
character to indicate the end of the entry for the call has been found.  The
control characters are:

    DataBaseEntryRecord = RECORD
        Call:    CallString;      { Always first }
        Section: STRING [5];      { Control-A }
        CQZone:  STRING [2];      { Control-C }
        FOC:     STRING [5];      { Control-F }
        Grid:    STRING [6];      { Control-G }
        Hits:    BYTE;            { Control-H }
        ITUZone: STRING [5];      { Control-I }
        Check:   STRING [2];      { Control-K }
        Name:    CallString;      { Control-N }
        OldCall: CallString;      { Control-O }
        QTH:     STRING [10];     { Control-Q }
        Speed:   BYTE;            { Control-S }
        TenTen:  STRING [6];      { Control-T }
        User1:   CallString;      { Control-U }
        User2:   CallString;      { Control-V }
        User3:   CallString;      { Control-W }
        User4:   CallString;      { Control-X }
        User5:   CallString;      { Control-Y }
        END;

 *
*/

/*! \brief              Get a "line" from a TRMASTER binary file
    \param  contents    string that contains a record (plus, perhaps, much more)
    \param  posn        position of the call in <i>contents</i> (i.e., the start of the record)
    \return             The TRMASTER record that begins at <i>posn</i>

    Updates <i>posn</i> to point to the start of the next call
*/
//trmaster_line trmaster::_get_binary_record(const string& contents, uint32_t& posn)
trmaster_line trmaster::_get_binary_record(const string_view contents, uint32_t& posn)
{ constexpr int CTRL_Y { 25 };

  string callsign;
  string section;
  string cqzone;
  string foc;
  string grid;
  string hits;
  string ituzone;
  string check;
  string name;
  string oldcall;
  string qth;
  string speed;
  string ten_ten;
  string user_1, user_2, user_3, user_4, user_5;

  auto get_field = [&contents, &posn] (string& field) { while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
                                                          { field += contents[posn++]; }
                                                      };

  while (static_cast<int>(contents[posn]) != 0)                                           // marks end of record
  { get_field(callsign);

    switch (static_cast<int>(contents[posn]))
    { case 1 :              // ctrl-A
        ++posn;
        get_field(section);
        break;

      case 3 :             // ctrl-C
        ++posn;
        get_field(cqzone);
        break;

      case 6 :             // ctrl-F
        ++posn;
        get_field(foc);
        break;

      case 7 :             // ctrl-G
        ++posn;
        get_field(grid);
        break;

      case 8 :             // ctrl-H
        ++posn;
        get_field(hits);
        break;

      case 9 :             // ctrl-I
        ++posn;
        get_field(ituzone);
        break;

      case 11 :             // ctrl-K
        ++posn;
        get_field(check);
        break;

      case 14 :             // ctrl-N
        ++posn;
        get_field(name);
        break;

      case 15 :             // ctrl-O
        ++posn;
        get_field(oldcall);
        break;

      case 17 :             // ctrl-Q
        ++posn;
        get_field(qth);
        break;

      case 19 :             // ctrl-S
        ++posn;
        get_field(speed);
        break;

      case 20 :             // ctrl-T
        ++posn;
        get_field(ten_ten);
        break;

      case 21 :             // ctrl-U
        ++posn;
        get_field(user_1);
        break;

      case 22 :             // ctrl-V
        ++posn;
        get_field(user_2);
        break;

      case 23 :             // ctrl-W
        ++posn;
        get_field(user_3);
        break;

      case 24 :             // ctrl-X
        ++posn;
        get_field(user_4);
        break;

      case 25 :             // ctrl-Y
        ++posn;
        get_field(user_5);
        break;

      default:
        ++posn;
        break;
    }
  }

  trmaster_line rv;

  rv.call(callsign);
  rv.section(section);
  rv.cq_zone(from_string<int>(cqzone));
  rv.foc(from_string<int>(foc));
  rv.grid(grid);

  if (!hits.empty())
    rv.hit_count(static_cast<int>(hits[0]));

  rv.itu_zone(ituzone);
  rv.check(from_string<int>(check));
  rv.name(name);
  rv.old_call(oldcall);
  rv.qth(qth);
  rv.speed(speed);
  rv.ten_ten(from_string<int>(ten_ten));
  rv.user(1, user_1);
  rv.user(2, user_2);
  rv.user(3, user_3);
  rv.user(4, user_4);
  rv.user(5, user_5);

  return rv;
}

/*! \brief              Constructor
    \param  filename    name of file from which data are to be read

    The file <i>filename</i> may be either an ASCII or a binary file
*/
trmaster::trmaster(const string& filename)
{ const string contents  { read_file(filename) };      // throws exception if fails
  const bool   is_binary { contains(contents, create_string(static_cast<char>(0))) };

  if (is_binary)
  { if (contents.length() < sizeof(uint32_t))
      return;

// point to the start of the calls
    uint32_t pointer { static_cast<uint32_t>(contents[0] + ((contents[1] + ((contents[2] + (contents[3] << 8)) << 8)) << 8)) };

// extract all the calls
    while (pointer < contents.length())
    { trmaster_line record { _get_binary_record(contents, pointer) };

// is there already a record for this call?
      string call { record.call() };

      if (_records.contains(call))
        record += _records[call];

      _records += { move(call), move(record) };
    }
  }
  else              // not binary
  { FOR_ALL( to_lines <std::string_view> (contents), [this] (auto line) { const trmaster_line record(line);

                                                                          _records += { record.call(), record };
                                                                        } );
  }
}

// all the calls (in callsign order)
vector<string> trmaster::calls(void) const
{ vector<string> rv;

  FOR_ALL(_records, [&rv] (const auto& rec) { rv += rec.first; } );

  SORT(rv, compare_calls);                                          // added compare_calls 201101

  return rv;
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

  return ( (it == fields.end()) ? string { } : it->substr(field_indicator.length()));
}

/*! \brief      Process a single field (such as: "=Xabc")
    \param  sv  the field to processs (e.g., "=Xabc")

    Does nothing if the field does not exist
*/
void drmaster_line::_process_field(const std::string_view sv)
{ using SETTER = void (drmaster_line::*) (const string&);

  static const map<char, SETTER> c_to_member { { 'K', &drmaster_line::check },
                                               { 'C', &drmaster_line::cq_zone },
                                               { 'F', &drmaster_line::foc },
                                               { 'G', &drmaster_line::grid },
                                               { 'H', &drmaster_line::hit_count },
                                               { 'I', &drmaster_line::itu_zone },
                                               { 'N', &drmaster_line::name },
                                               { 'Q', &drmaster_line::qth },
                                               { 'A', &drmaster_line::section },
                                               { 'T', &drmaster_line::ten_ten },

                                               { 'n', &drmaster_line::age_aa_cw },
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

  if ( (c >= 'U') and (c <= 'Y') )
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

  size_t space_posn { line_or_call.find(' ') };

  if (space_posn == string::npos) // no space
  { _call = line_or_call;

    return;
  }

  _call = substring <string> (line_or_call, 0, space_posn);

// all the other fields
  while (space_posn != string::npos)
  { if (const size_t eq_posn { line_or_call.find('=', space_posn) }; eq_posn != string::npos)      // found an equals
    { space_posn = line_or_call.find(' ', eq_posn);

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

// U, V, W, X
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
//void drmaster::operator+=(const string& call)
void drmaster::operator+=(const string_view call)
{ if (!::contains(call, ' ') and !_records.contains(call))     // basic sanity check for a call, and whether is already in the database
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
  { drmaster_line old_drml { _records[call] };

    old_drml += drml;
    _records -= call;
    _records += { call, old_drml };
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
