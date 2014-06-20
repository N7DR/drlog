// $Id: trlog.cpp 28 2013-07-06 13:34:10Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#include "string_functions.h"
#include "trlog.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <cstring>

using namespace std;

// -----------  tr_record  ----------------

/*! \brief  Convert part of the record to an integer
    \param  posn  position at which to commence conversion
    \param  len   number of characters to convert

    \return characters converted to an integer
*/
const int tr_record::_convert_to_int(const int posn, const int len) const
{ char tmp[len + 1];

  for (int n = 0; n < len; ++n)
    tmp[n] = _record[posn + n];

  tmp[len] = 0;

  return atoi(tmp);
}

/// month of the year; 1 - 12
const int tr_record::month(void) const
{ char tmp[4];

  tmp[0] = _record[10];
  tmp[1] = _record[11];
  tmp[2] = _record[12];
  tmp[3] = 0;

  string tmps = to_upper(tmp);

  if (tmps == "JAN")
    return 1;

  if (tmps == "FEB")
    return 2;

  if (tmps == "MAR")
    return 3;

  if (tmps == "APR")
    return 4;

  if (tmps == "MAY")
    return 5;

  if (tmps == "JUN")
    return 6;

  if (tmps == "JUL")
    return 7;

  if (tmps == "AUG")
    return 8;

  if (tmps == "SEP")
    return 9;

  if (tmps == "OCT")
    return 10;

  if (tmps == "NOV")
    return 11;

  if (tmps == "DEC")
    return 12;

  return 0;    // error
}

/// four-digit year
const int tr_record::year(void) const
{ const int two_digit_year = _convert_to_int(14, 2);

  return (two_digit_year + ( (two_digit_year < 50) ? 2000 : 1900 ) );
}

/// sent RST
const int tr_record::rst(void) const
{ char tmp[4];

  tmp[0] = _record[44];
  tmp[1] = _record[45];
  tmp[2] = _record[46];
  tmp[3] = 0;

  if (tmp[2] == ' ')
    tmp[2] = 0;

  return atoi(tmp);
}

/// received RST
const int tr_record::rst_received(void) const
{ char tmp[4];

  tmp[0] = _record[49];
  tmp[1] = _record[50];
  tmp[2] = _record[51];
  tmp[3] = 0;

  if (tmp[2] == ' ')
    tmp[2] = 0;

  return atoi(tmp);
}

/// band
const BAND tr_record::band(void) const
{ string band_string = substring(_record, 0, 3);

  const int b = atoi(band_string.c_str());

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
  const string qso_number_str = substring(_record, 23, 4);

  if (contains(qso_number_str, "."))
  { float to_add = from_string<float>(qso_number_str);

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
{ char tmp[5];

  tmp[0] = _record[53];
  tmp[1] = _record[54];
  tmp[2] = _record[55];
  tmp[3] = _record[56];
  tmp[4] = 0;

  string rv(&tmp[0]);

  return remove_peripheral_spaces(rv);
}

// -----------  tr_log  ----------------
#if 0
tr_log::tr_log(const std::string& filename)
{ ifstream original_file(filename);

  char buffer [1000];

  original_file.getline(buffer, 1000);
  string title = buffer;

  _fp = tmpfile();
  _number_of_qsos = 0;
  _record_length = 0;

  while (!original_file.eof())
  { // getline does not leave the EOL marker in buffer
    original_file.getline(buffer, 1000);

    string this_line = buffer;

// line_of_log_info is a boolean to see if this is real data
    int line_of_log_info = (this_line != title);
    line_of_log_info = line_of_log_info && (this_line.length() > 40);

    string mode;
    if (line_of_log_info)
      mode = substring(this_line, 3, 3);

    line_of_log_info = line_of_log_info && ((mode == "CW ") || (mode == "SSB"));

    if (line_of_log_info)
    {
// strip any dollar signs
      this_line = remove_char(this_line, '$');

      const int old_record_length = _record_length;
      _record_length = this_line.length();

//      cerr << "Processing line: " << this_line << endl;

      if (old_record_length)
      { if (_record_length != old_record_length)
        { cerr << "Error creating tr_log; inconsistent record length: _record_length = " << _record_length << "; old_record_length = " << old_record_length << endl;
          exit(-2);;
        }
      }
//      assert(fwrite(buffer, _record_length, 1, _fp) == 1);

      int status = fwrite(buffer, _record_length, 1, _fp);

      if (status != 1)
       { cerr << "Write error" << endl;
         throw exception();
       }

      _number_of_qsos++;
    }
  }
  fflush(_fp);
}
#endif

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
{ const string contents = remove_char(read_file(filename), '$');  // remove and dollar signs as we import file
  const vector<string> lines = to_lines(contents);

  const string title = lines.empty() ? "" : remove_peripheral_spaces(lines[0]);

  _fp = tmpfile();
  _number_of_qsos = 0;
  _record_length = 0;

  for (size_t n = 1; n < lines.size(); ++n)
  { const string& this_line = lines[n];

// is this line real data?
    bool line_of_log_info = ( (this_line != title) and (this_line.length() > 40) );
    const string mode = (line_of_log_info ? substring(this_line, 3, 3) : "");

    line_of_log_info = line_of_log_info and ((mode == "CW ") or (mode == "SSB"));

    if (line_of_log_info)
    { const int old_record_length = _record_length;
      _record_length = this_line.length();

      if (old_record_length)
      { if (_record_length != old_record_length)
        { cerr << "Error creating tr_log; inconsistent record length: _record_length = " << _record_length << "; old_record_length = " << old_record_length << endl;
          exit(-2);;
        }
      }

      int status = fwrite(this_line.c_str(), _record_length, 1, _fp);

      if (status != 1)
      { cerr << "Write error" << endl;
        throw exception();
      }

      _number_of_qsos++;
    }
  }

  fflush(_fp);
}


/// destructor
tr_log::~tr_log(void)
{ fclose(_fp);          // not really necessary
}

/// sort the log in order of callsign
void tr_log::sort_by_call(void)
{ struct QSO_CALL* table = new struct QSO_CALL [_number_of_qsos];

  for (int n = 0; n < _number_of_qsos; ++n)
  { table[n]._qso = n;
    table[n]._call = (this->read(n)).call();
  }

// sort the table by callsign
  qsort((void*)table, _number_of_qsos, sizeof(struct QSO_CALL), _compare_calls);

// create a revised file
  FILE* tmpfp = tmpfile();
  for (int n = 0; n < _number_of_qsos; ++n)
  { const int original_position = table[n]._qso;
    int status = fseek(_fp, original_position * _record_length, 0);
    char buffer[1000];

    status = fread(buffer, _record_length, 1, _fp);
    status = fwrite(buffer, _record_length, 1, tmpfp);
  }

// Now delete the old file and reference the new one
  fclose(_fp);               // automatically deletes the file
  _fp = tmpfp;
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
{ int status = fseek(_fp, n * _record_length, 0);
  status = fwrite(&trr, _record_length, 1, _fp);
}

