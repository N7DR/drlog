// $Id: drmaster.cpp 223 2023-07-30 13:37:25Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   drmaster.cpp

    Classes associated with MASTER.DTA, TRMASTER.[DTA, ASC] and drmaster files.

    Parts of this file are based on the very old trm.cpp for TRLOG manipulation
*/

#include "drmaster.h"
#include "log_message.h"

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
string master_dta::_get_call(const string& contents, uint32_t& posn) const
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
master_dta::master_dta(const string& filename)
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
//string __extract_value(const string& line, const string& param)
string __extract_value(string_view line, const string& param)
{ if (!contains(line, param))
    return string();

  const size_t start_position  { line.find(param) };
  const string_view short_line { substring <std::string_view> (line, start_position + param.length()) };
  const size_t space_posn      { short_line.find(' ') };
  const size_t end_position    { ( (space_posn == string_view::npos) ? short_line.length() : space_posn ) };

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
//trmaster_line::trmaster_line(const string& line)
trmaster_line::trmaster_line(string_view line)
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

  if (!section().empty())
    rv += (" =A"s + section());

  if (cq_zone())
    rv += (" =C"s + ::to_string(cq_zone()));

  if (foc())
    rv += (" =F"s + ::to_string(foc()));

  if (!grid().empty())
    rv += (" =G"s + grid());

  if (hit_count())
    rv += (" =H"s + ::to_string(hit_count()));

  if (!itu_zone().empty())
    rv += (" =I"s + itu_zone());

  if (check())
    rv += (" =K"s + ::to_string(check()));

  if (!name().empty())
    rv += (" =N"s + name());

  if (!qth().empty())
    rv += (" =Q"s + qth());

  if (ten_ten())
    rv += (" =T"s +  ::to_string(ten_ten()));

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
{ trmaster_line rv(*this);                                  // copy the old line

// check that the calls match
  if (trml.call() != call())
    return rv;

// go through the fields seriatim
  rv.hit_count(rv.hit_count() + trml.hit_count() + 1);      // may change this

  if ((!trml.qth().empty()))
    rv.qth(trml.qth());

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
trmaster_line trmaster::_get_binary_record(const string& contents, uint32_t& posn)
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

  while (static_cast<int>(contents[posn]) != 0)                                           // marks end of record
  { while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
      { callsign += contents[posn++]; }

    switch (static_cast<int>(contents[posn]))
    { case 1 :              // ctrl-A
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { section += contents[posn++]; }
        break;

      case 3 :             // ctrl-C
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { cqzone += contents[posn++]; }
        break;

      case 6 :             // ctrl-F
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { foc += contents[posn++]; }
        break;

      case 7 :             // ctrl-G
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { grid += contents[posn++]; }
        break;

      case 8 :             // ctrl-H
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { hits += contents[posn++]; }
        break;

      case 9 :             // ctrl-I
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { ituzone += contents[posn++]; }
        break;

      case 11 :             // ctrl-K
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { check += contents[posn++]; }
        break;

      case 14 :             // ctrl-N
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { name += contents[posn++]; }
        break;

      case 15 :             // ctrl-O
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { oldcall += contents[posn++]; }
        break;

      case 17 :             // ctrl-Q
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { qth += contents[posn++]; }
        break;

      case 19 :             // ctrl-S
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { speed += contents[posn++]; }
        break;

      case 20 :             // ctrl-T
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { ten_ten += contents[posn++]; }
        break;

      case 21 :             // ctrl-U
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { user_1 += contents[posn++]; }
        break;

      case 22 :             // ctrl-V
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { user_2 += contents[posn++]; }
        break;

      case 23 :             // ctrl-W
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { user_3 += contents[posn++]; }
        break;

      case 24 :             // ctrl-X
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { user_4 += contents[posn++]; }
        break;

      case 25 :             // ctrl-Y
        ++posn;
        while ((posn < contents.length()) and static_cast<int>(contents[posn]) > CTRL_Y)
          { user_5 += contents[posn++]; }
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

  FOR_ALL(_records, [&rv](const auto& rec) { rv += rec.first; } );

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
*/

/*! \brief                      Extract a single field from the record
    \param  fields              all the fields
    \param  field_indicator     indicator that prefixes the field (for example: "=H")
    \return                     Value of the field with the indicator <i>field_indicator</i>

    Returns empty string if no field has the indicator <i>field_indicator</i>
*/
string drmaster_line::_extract_field(const vector<string>& fields, const std::string& field_indicator)
{ for (vector<string>::const_iterator cit { fields.cbegin() }; cit != fields.cend(); ++cit)
  { if (cit->starts_with(field_indicator))
      return (cit->substr(field_indicator.length()));
  }

  return string { };
}

/*! \brief                  Construct from a call or from a line from a drmaster file
    \param  line_or_call    line from file, or a call

    Constructs an object that contains only the call if <i>line_or_call</i> contains a call
*/
//drmaster_line::drmaster_line(const string& line_or_call)
drmaster_line::drmaster_line(string_view line_or_call)
{ const vector<string> fields { split_string <std::string> (line_or_call, ' ') };

  if (fields.empty())
    return;

  _call = to_upper(fields[0]);

  _check     = _extract_field(fields, "=K"s);
  _cq_zone   = _extract_field(fields, "=C"s);
  _foc       = _extract_field(fields, "=F"s);
  _grid      = _extract_field(fields, "=G"s);
  _hit_count = _extract_field(fields, "=H"s);
  _itu_zone  = _extract_field(fields, "=I"s);
  _name      = _extract_field(fields, "=N"s);
  _qth       = _extract_field(fields, "=Q"s);
  _section   = _extract_field(fields, "=A"s);
  _ten_ten   = _extract_field(fields, "=T"s);
  _user[0]   = _extract_field(fields, "=U"s);
  _user[1]   = _extract_field(fields, "=V"s);
  _user[2]   = _extract_field(fields, "=W"s);
  _user[3]   = _extract_field(fields, "=X"s);
  _user[4]   = _extract_field(fields, "=Y"s);

// drmaster extensions
  _cw_power   = _extract_field(fields, "=y"s);
  _date       = _extract_field(fields, "=z"s);
  _iota       = _extract_field(fields, "=w"s);
  _precedence = _extract_field(fields, "=u"s);
  _qth2       = _extract_field(fields, "=o"s);
  _skcc       = _extract_field(fields, "=q"s);
  _society    = _extract_field(fields, "=v"s);
  _spc        = _extract_field(fields, "=r"s);
  _ssb_power  = _extract_field(fields, "=x"s);
  _state_160  = _extract_field(fields, "=s"s);
  _state_10   = _extract_field(fields, "=t"s);

  if (const string xscp_str { _extract_field(fields, "=p"s) }; !xscp_str.empty())
    _xscp       = from_string<decltype(_xscp)>(xscp_str);
}

/// convert to string
string drmaster_line::to_string(void) const
{ string rv;

  rv += call();

  if (!section().empty())
    rv += " =A"s + section();

  if (!cq_zone().empty())
    rv += " =C"s + cq_zone();

  if (!foc().empty())
    rv += " =F"s + foc();

  if (!grid().empty())
    rv += " =G"s + grid();

  if (!hit_count().empty())
    rv += " =H"s + hit_count();

  if (!itu_zone().empty())
    rv += " =I"s + itu_zone();

  if (!check().empty())
    rv += " =K"s + check();

  if (!name().empty())
    rv += " =N"s + name();

  if (!qth().empty())
    rv += " =Q"s + qth();

  if (!ten_ten().empty())
    rv += " =T"s + ten_ten();

  char user_letter { 'U' };

  for (unsigned int n { 0 }; n < TRMASTER_N_USER_PARAMETERS; n++)
  { if (!user(n + 1).empty())
    { rv += " ="s;
      rv += user_letter;
      rv += user(n + 1);
    }
    user_letter++;
  }

  if (!cw_power().empty())
    rv += " =y"s + cw_power();

  if (!date().empty())
    rv += " =z"s + date();

  if (!iota().empty())
    rv += " =w"s + iota();

  if (!precedence().empty())
    rv += " =u"s + precedence();

  if (!qth2().empty())
    rv += " =o"s + qth2();

  if (!skcc().empty())
    rv += " =q"s + skcc();

  if (!society().empty())
    rv += " =v"s + society();

  if (!spc().empty())
    rv += " =r"s + spc();

  if (!ssb_power().empty())
    rv += " =x"s + ssb_power();

  if (!state_160().empty())
    rv += " =s"s + state_160();

  if (!state_10().empty())
    rv += " =t"s + state_10();

  if (xscp() != 0)
    rv += " =p"s + ::to_string(xscp());

  return rv;
}

// merge with another drmaster_line; new values take precedence if there's a conflict
drmaster_line drmaster_line::operator+(const drmaster_line& drml) const
{ if (call() != drml.call())
    return *this;

  drmaster_line rv { drml };

  if (rv.section().empty())
    rv.section(section());

  if (rv.cq_zone().empty())
    rv.cq_zone(cq_zone());

  if (rv.foc().empty())
    rv.foc(foc());

  if (rv.grid().empty())
    rv.grid(grid());

  if (rv.hit_count().empty())
    rv.hit_count(hit_count());

  if (rv.itu_zone().empty())
    rv.itu_zone(itu_zone());

  if (rv.check().empty())
    rv.check(check());

  if (rv.name().empty())
    rv.name(name());

  if (rv.qth().empty())
    rv.qth(qth());

  if (rv.ten_ten().empty())
    rv.ten_ten(ten_ten());

  for (unsigned int n { 1 }; n <= TRMASTER_N_USER_PARAMETERS; n++)
  { if (rv.user(n).empty())
      rv.user(n, user(n));
  }

  if (rv.cw_power().empty())
    rv.cw_power(cw_power());

  if (rv.date().empty())
    rv.date(substring <std::string> (date_time_string(SECONDS::NO_INCLUDE), 0, 8));

  if (rv.iota().empty())
    rv.iota(iota());

  if (rv.precedence().empty())
    rv.precedence(precedence());

  if (rv.qth2().empty())
    rv.qth2(qth2());

  if (rv.skcc().empty())
    rv.skcc(skcc());

  if (rv.society().empty())
    rv.society(society());

  if (rv.spc().empty())
    rv.spc(spc());

  if (rv.ssb_power().empty())
    rv.ssb_power(ssb_power());

  if (rv.society().empty())
    rv.society(society());

  if (rv.state_160().empty())
    rv.state_160(state_160());

  if (rv.state_10().empty())
    rv.state_10(state_10());

  if (xscp() != 0)
    rv.xscp(xscp());

  return rv;
}

// -----------------------------------------------------  drmaster  ---------------------------------

/*! \class  drmaster
    \brief  Manipulate a drmaster file

    A drmaster file is a superset of a TRMASTER.ASC file
*/

/*! \brief              Prepare an object for use, from a file's contents
    \param  contents    the contents of a drmaster file
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Lines without XSCP data are always included
*/
void drmaster::_prepare_from_file_contents(const string& contents, const int xscp_limit)
{ for (const string& line : to_lines <std::string> (contents))
  { const drmaster_line record { line };

    if ( (record.xscp() == 0) or (record.xscp() >= xscp_limit) )
      _records += { record.call(), record } ;
  } 

  ost << "read " << _records.size() << " drmaster records from file" << endl;
}

/*! \brief              Construct from a file
    \param  filename    name of file to read
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Lines without XSCP data are always included

    Throws exception if the file does not exist or is incorrectly formatted;
    except creates empty object if called with default filename that does not exist
*/
drmaster::drmaster(const string& filename, const int xscp_limit)
{ if (!filename.empty())
  { try
    { _prepare_from_file_contents(read_file(filename), xscp_limit);      // throws exception if fails
    }

    catch (...)
    { if (filename != "drmaster"sv)      // don't fail if we couldn't find the default file; simply create empty object
        throw;
    }
  }
}

/*! \brief              Construct from a file
    \param  path        directories to check
    \param  filename    name of file to read
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Lines without XSCP data are always included
    Constructs from the first instance of <i>filename</i> when traversing the <i>path</i> directories.
    Throws exception if the file does not exist or is incorrectly formatted
*/
drmaster::drmaster(const vector<string>& path, const string& filename, const int xscp_limit)
{ if (!filename.empty())
    _prepare_from_file_contents(read_file(path, filename), xscp_limit);      // throws exception if fails
}

/*! \brief              Prepare the object by reading a file
    \param  filename    name of file to read
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Lines without XSCP data are always included
    Throws exception if the file does not exist or is incorrectly formatted
*/
void drmaster::prepare(const string& filename, const int xscp_limit)
{ if (!filename.empty())
    _prepare_from_file_contents(read_file(filename), xscp_limit);      // throws exception if fails
}

/*! \brief              Prepare the object by reading a file
    \param  path        directories to check
    \param  filename    name of file to read
    \param  xscp_limit  lines with XSCP data are included only if the value is >= this value

    Processes the first instance of <i>filename</i> when traversing the <i>path</i> directories
*/
void drmaster::prepare(const vector<string>& path, const string& filename, const int xscp_limit)
{ if (!filename.empty())
    _prepare_from_file_contents(read_file(path, filename), xscp_limit);      // throws exception if fails
}

/// all the calls (in alphabetical order)
vector<string> drmaster::calls(void) const
{ vector<string> rv { unordered_calls() };

  SORT(rv, compare_calls);

  return rv;
}

/// all the calls (in random order)
vector<string> drmaster::unordered_calls(void) const
{ vector<string> rv;
  rv.reserve(_records.size());

  FOR_ALL(_records, [&rv] (const auto& rec) { rv += rec.first; } );

  return rv;
}

/// format for output
string drmaster::to_string(void) const
{ vector<string> lines;
  lines.reserve(_records.size());

  FOR_ALL(_records, [&lines] (const auto& rec) { lines += rec.second.to_string() + EOL; });

  SORT(lines);

  return ( join(lines, string { }) );            // don't add the default (space) separator
}

/*! \brief          Add a callsign.
    \param  call    call to add

    If there's already an entry for <i>call</i>, then does nothing
*/
void drmaster::operator+=(const string& call)
{ if (!::contains(call, ' ') and !_records.contains(call))           // basic sanity check for a call, and whether is already in the database
    _records += { call, static_cast<drmaster_line>(call) };    // cast needed in order to keep the temporary around long enough to use
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
    \return     <i>drmaster</i> object containing only records with no xscp, and thiose for which the xscp value is >= the <i>pc</i> value 
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

  ost << "number of XSCP = 0 values = " << rv.size() << endl;
  ost << "number of non-zero values = " << xscp_values.size() << endl;

  const int breakpoint_value { value_line(xscp_values, pc) };

  ost << "breakpoint value = " << breakpoint_value << endl;

  for (const auto& [ call, line ] : _records)
    if (line.xscp() >= breakpoint_value)
      rv += line; 

  return rv;
}
