// $Id: trlog.cpp 175 2020-12-06 17:44:13Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#if 1

/*! \file   trlog.cpp

    Classes and functions related to TRlog log files
*/

#include "string_functions.h"
#include "trlog.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <cstring>

using namespace std;

/*! \class  QSO_CALL
    \brief  Encapsulates a QSO number and a callsign
*/

struct QSO_CALL
{ int         _qso;         ///< QSO number
  std::string _call;        ///< callsign
};

// -----------  tr_record  ----------------

/*! \class  tr_record
    \brief  Encapsulate a single TRLOG QSO
*/

/// month of the year; 1 - 12
const int tr_record::month(void) const
{ static const unordered_map<string, int> month_nr { { "JAN"s, 1 },
                                                     { "FEB"s, 2 },
                                                     { "MAR"s, 3 },
                                                     { "APR"s, 4 },
                                                     { "MAY"s, 5 },
                                                     { "JUN"s, 6 },
                                                     { "JUL"s, 7 },
                                                     { "AUG"s, 8 },
                                                     { "SEP"s, 9 },
                                                     { "OCT"s, 10 },
                                                     { "NOV"s, 11 },
                                                     { "DEC"s, 12 }
                                                   };

  const string tmps { substring(_record, 10, 3) };

  try
  { return month_nr.at(tmps);
  }

  catch (...)
  { return 0;       // error
  }

#if 0
  if (tmps == "JAN"s)
    return 1;

  if (tmps == "FEB"s)
    return 2;

  if (tmps == "MAR"s)
    return 3;

  if (tmps == "APR"s)
    return 4;

  if (tmps == "MAY"s)
    return 5;

  if (tmps == "JUN"s)
    return 6;

  if (tmps == "JUL"s)
    return 7;

  if (tmps == "AUG"s)
    return 8;

  if (tmps == "SEP"s)
    return 9;

  if (tmps == "OCT"s)
    return 10;

  if (tmps == "NOV"s)
    return 11;

  if (tmps == "DEC"s)
    return 12;

  return 0;    // error
#endif
}

/// four-digit year
const int tr_record::year(void) const
{ const int two_digit_year { _convert_to_int(14, 2) };

  return (two_digit_year + ( (two_digit_year < 50) ? 2000 : 1900 ) );
}

/// sent RST
const int tr_record::rst(void) const
{ const string tmp { substring(_record, 44, _record[46] == ' ' ? 2 : 3) };

  return from_string<int>(tmp);
}

/// received RST
const int tr_record::rst_received(void) const
{ const string tmp { substring(_record, 49, _record[51] == ' ' ? 2 : 3) };

  return from_string<int>(tmp);
}

/// band
const BAND tr_record::band(void) const
{ const string band_string { substring(_record, 0, 3) };
  const int    b           { atoi(band_string.c_str()) };

  switch (b)
  { case 160 : return BAND_160;
    case 80  : return BAND_80;
    case 40  : return BAND_40;
    case 20  : return BAND_20;
    case 15  : return BAND_15;
    case 10  : return BAND_10;

    case 30  : return BAND_30;
    case 17  : return BAND_17;
    case 12  : return BAND_12;

    default  : throw exception();
  }
}

/// frequency in MHz
const string tr_record::frequency(void) const
{ float base_frequency;

  switch (band())
  { case BAND_160:
      base_frequency = 1.8;
      break;

     case BAND_80:
       base_frequency = 3.5;
        break;

     case BAND_40:
       base_frequency = 7;
        break;

     case BAND_20:
       base_frequency = 14;
        break;

     case BAND_15:
       base_frequency = 21;
        break;

     case BAND_10:
       base_frequency = 28;
        break;

     default:
       base_frequency = 0;
       break;
   }

// the kHz portion of the frequency might be elsewhere in the record
  const string qso_number_str { substring(_record, 23, 4) };

  if (contains(qso_number_str, "."s))
  { float to_add { from_string<float>(qso_number_str) };

    switch (band())
    { case BAND_160:
        base_frequency = 1;
        break;

      case BAND_80:
        base_frequency = 3;
        break;

      default:
        break;
    }

    base_frequency = base_frequency + to_add;
  }

  ostringstream stream;

  stream << fixed << setprecision(3) << base_frequency;

  return stream.str();
}

/// the received exchange; maximum of four characters
const string tr_record::exchange_received(void) const
{ const string tmp { substring(_record, 53, 4) };

//  char tmp[5];

//  tmp[0] = _record[53];
//  tmp[1] = _record[54];
//  tmp[2] = _record[55];
//  tmp[3] = _record[56];
//  tmp[4] = 0;

//  string rv(&tmp[0]);

  return remove_peripheral_spaces(tmp);
}

// -----------  tr_log  ----------------

/// compare order of calls
int _compare_calls(const void* a, const void* b)
{ return (strcmp((*(QSO_CALL*)a)._call.c_str(), (*(QSO_CALL*)b)._call.c_str())); }

// -----------  tr_log  ----------------

/*!     \class tr_log
        \brief Manipulate a TRLOG file
*/

/*! \brief  Construct from a TRLOG file
    \param  filename  name of a TRLOG file
*/
tr_log::tr_log(const std::string& filename)
{ const string         contents { remove_char(read_file(filename), '$') };  // remove any dollar signs as we import file
  const vector<string> lines    { to_lines(contents) };
  const string         title    { lines.empty() ? EMPTY_STR : remove_peripheral_spaces(lines[0]) };

  _fp = tmpfile();
  _number_of_qsos = 0;
  _record_length = 0;

  for (size_t n = 1; n < lines.size(); ++n)
  { const string& this_line { lines[n] };

// is this line real data?
    bool line_of_log_info { ( (this_line != title) and (this_line.length() > 40) ) };

    const string mode { (line_of_log_info ? substring(this_line, 3, 3) : EMPTY_STR) };

    line_of_log_info = line_of_log_info and ((mode == "CW "s) or (mode == "SSB"s));

    if (line_of_log_info)
    { const auto old_record_length { _record_length };

      _record_length = this_line.length();

      if (old_record_length)
      { if (_record_length != old_record_length)
        { cerr << "Error creating tr_log; inconsistent record length: _record_length = " << _record_length << "; old_record_length = " << old_record_length << endl;
          exit(-2);
        }
      }

      const auto status { fwrite(this_line.c_str(), _record_length, 1, _fp) };

      if (status != 1)
      { cerr << "Write error" << endl;
        throw exception();
      }

      _number_of_qsos++;
    }
  }

  fflush(_fp);
}

/// sort the log in order of callsign
void tr_log::sort_by_call(void)
{ struct QSO_CALL* table { new struct QSO_CALL [_number_of_qsos] };

  for (int n = 0; n < _number_of_qsos; ++n)
  { table[n]._qso = n;
    table[n]._call = (this->read(n)).call();
  }

// sort the table by callsign
  qsort((void*)table, _number_of_qsos, sizeof(struct QSO_CALL), _compare_calls);

// create a revised file
  FILE* tmpfp { tmpfile() };

  for (int n = 0; n < _number_of_qsos; ++n)
  { const int original_position { table[n]._qso };
    int       status            { fseek(_fp, original_position * _record_length, 0) };

    if (status != 0)
      throw exception();

    char buffer[1000];

    status = fread(buffer, _record_length, 1, _fp);

    if (status != 1)
      throw exception();

    status = fwrite(buffer, _record_length, 1, tmpfp);

    if (status != 1)
      throw exception();
  }

// Now delete the old file and reference the new one
  fclose(_fp);               // automatically deletes the file
  _fp = tmpfp;

// delete the table
  delete [] table;
}

/// return record number <i>n</i>
const tr_record tr_log::read(const int n)
{ if (fseek(_fp, n * _record_length, 0))
    throw exception();

  char buffer[1000];

  if (fread(buffer, _record_length, 1, _fp) != 1)
    throw exception();

  const tr_record _current_record(&buffer[0]);

  return _current_record;
}

/*! \brief  Write a record to the file
    \param  trr  the text of the record to be written
    \param  n    the record number at which <i>trr</i> should be written
*/
void tr_log::write(const tr_record& trr, const int n)
{ int status { fseek(_fp, n * _record_length, 0) };

  if (status != 0)
    throw exception();

  status = fwrite(&trr, _record_length, 1, _fp);

  if (status != 1)
    throw exception();
}

#endif      // 0
