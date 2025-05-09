// $Id: trlog.cpp 259 2025-01-19 15:44:33Z  $

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
int tr_record::month(void) const
{ static const UNORDERED_STRING_MAP<int> month_nr { { "JAN"s, 1 },
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

  return MUM_VALUE(month_nr, substring <std::string> (_record, 10, 3), 0);
}

/// four-digit year
int tr_record::year(void) const
{ const int two_digit_year { _convert_to_int(14, 2) };

  return (two_digit_year + ( (two_digit_year < 50) ? 2000 : 1900 ) );
}

/// sent RST
int tr_record::rst(void) const
{ const string tmp { substring <std::string> (_record, 44, _record[46] == ' ' ? 2 : 3) };

  return from_string<int>(tmp);
}

/// band
BAND tr_record::band(void) const
{ const string band_string { substring <std::string> (_record, 0, 3) };
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
string tr_record::frequency(void) const
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
  const string qso_number_str { substring <std::string> (_record, 23, 4) };

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

// -----------  tr_log  ----------------

/// compare order of calls; not that this is not a member function
inline int _compare_calls(const void* a, const void* b)
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
  const vector<string> lines    { to_lines <std::string> (contents) };
  const string         title    { lines.empty() ? EMPTY_STR : remove_peripheral_spaces <std::string> (lines[0]) };

  _fp = tmpfile();
  _number_of_qsos = 0;
  _record_length = 0;

  for (size_t n { 1 }; n < lines.size(); ++n)
  { const string& this_line { lines[n] };

// is this line real data?
    bool line_of_log_info { ( (this_line != title) and (this_line.length() > 40) ) };

    const string mode { (line_of_log_info ? substring <std::string> (this_line, 3, 3) : EMPTY_STR) };

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

  for (int n { 0 }; n < _number_of_qsos; ++n)
  { table[n]._qso = n;
    table[n]._call = (this->read(n)).call();
  }

// sort the table by callsign
  qsort((void*)table, _number_of_qsos, sizeof(struct QSO_CALL), _compare_calls);

// create a revised file
  FILE* tmpfp { tmpfile() };

  for (int n { 0 }; n < _number_of_qsos; ++n)
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
tr_record tr_log::read(const int n)
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
