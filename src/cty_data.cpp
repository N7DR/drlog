// $Id: cty_data.cpp 271 2025-06-23 16:32:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   cty_data.cpp

    Objects and functions related to CTY.DAT files
*/

#include "cty_data.h"
#include "drlog_context.h"
#include "string_functions.h"

#include <utility>

#include <fstream>
#include <iostream>

using namespace std;

extern const STRING_SET CONTINENT_SET { "AF"s, "AS"s, "EU"s, "NA"s, "OC"s, "SA"s, "AN"s };  ///< abbreviations for continents; // see https://stackoverflow.com/questions/177437/const-static

constexpr unsigned int   CTY_FIELDS_PER_RECORD { 9 };                                                           ///< Number of fields in a single CTY record
constexpr array<int, 10> VE_CQ                 { 5 /* probably not correct */, 5, 5, 4, 4, 4, 4, 3, 1, 5 };     ///< default CQ zones for VE call areas; 0 to 9
constexpr array<int, 10> W_CQ                  { 4, 5, 5, 5, 5, 4, 3, 3, 4, 4 };                                ///< default CQ zones for W call areas; 0 to 9
constexpr array<int, 10> VE_ITU                { 8 /* probably not correct */, 9, 4, 4, 3, 3, 2, 2, 4, 9 } ;    ///< default ITU zones for VE call areas; 0 to 9
constexpr array<int, 10> W_ITU                 { 7, 8, 4, 4, 4, 7, 6, 6, 4, 4 };                                ///< default ITU zones for W call areas; 0 to 9

// decltype(cty_record::_cq_zone) is protected
constexpr unsigned int MIN_CQ_ZONE  { 1 };      ///< minimum CQ zone number
constexpr unsigned int MAX_CQ_ZONE  { 40 };     ///< maximum CQ zone number
constexpr unsigned int MIN_ITU_ZONE { 1 };      ///< minimum ITU zone number
constexpr unsigned int MAX_ITU_ZONE { 90 };     ///< maximum ITU zone number

// -----------  cty_record  ----------------

/*! \class  cty_record
    \brief  A single record in the CTY.DAT file
  
    The official page describing the format is:
      http://www.country-files.com/cty/format.htm.

    210930: the format now seems to be at:
      https://www.country-files.com/cty-dat-format/

----
  
Column 	Length 	Description
1 	26 	Country Name
27 	5 	CQ Zone
32 	5 	ITU Zone
37 	5 	2-letter continent abbreviation
42 	9 	Latitude in degrees, + for North
51 	10 	Longitude in degrees, + for West
61 	9 	Local time offset from GMT
70 	6 	Primary DXCC Prefix (A “*” preceding this prefix indicates that the country is on the DARC WAEDC list, and counts in CQ-sponsored contests, but not ARRL-sponsored contests).

----

    The above is wrong, as it ignores the ninth field, which comprises
    at least one prefix and various alternative calls and prefixes.

    Spacing in the file is "for readability only", so
    we use the official delimiter ":" for fields, and
    ";" for records.
*/

/*! \brief  Construct from a string

            The string is assumed to contain a single record. We don't catch all
            possible errors, but we do test for the most obvious ones.
*/
cty_record::cty_record(const string_view record)
{ constexpr int CTY_NAME       { 0 };
  constexpr int CTY_CQZ        { 1 };
  constexpr int CTY_ITUZ       { 2 };
  constexpr int CTY_CONTINENT  { 3 };
  constexpr int CTY_LAT        { 4 };
  constexpr int CTY_LONG       { 5 };
  constexpr int CTY_UTC_OFFSET { 6 };
  constexpr int CTY_PREFIX     { 7 };
  constexpr int CTY_ALTS       { 8 };

// map prefixes if they collide with a continent
  static const STRING_MAP<string> map_prefix { { "EU"s, "EW"s } };

  const vector<string> fields { remove_peripheral_spaces <std::string> (split_string <std::string> ( remove_chars(record, CRLF), ':' )) };   // split the record into fields instead of lines
//  const vector<string_view> fields { remove_peripheral_spaces <std::string_view> (split_string <std::string_view> ( remove_chars(record, CRLF), ':' )) };   // split the record into fields instead of lines

  if (fields.size() != CTY_FIELDS_PER_RECORD)                                       // check the number of fields
  { ost << " Error constructing cty_record; record = " << remove_chars(record, CRLF) << endl
        << " number of fields = " << fields.size() << endl;

    for (size_t n { 0 }; n < fields.size(); ++n)
      ost << "field #" << n << ": " << fields[n]<< endl;

    throw cty_error(CTY_INCORRECT_NUMBER_OF_FIELDS, "Found "s + to_string(fields.size()) + " fields in record for "s + string { fields[0] });
  }
  
  _country_name = fields[CTY_NAME];

  auto_from_string(fields[CTY_CQZ], _cq_zone);
  if (_cq_zone < MIN_CQ_ZONE or _cq_zone > MAX_CQ_ZONE)
    throw cty_error(CTY_INVALID_CQ_ZONE, "CQ zone = "s + to_string(_cq_zone) + " in record for "s + _country_name);
  
  auto_from_string(fields[CTY_ITUZ], _itu_zone);
  if (_itu_zone < MIN_ITU_ZONE or _itu_zone > MAX_ITU_ZONE)
    throw cty_error(CTY_INVALID_ITU_ZONE, "ITU zone = "s + to_string(_itu_zone) + " in record for "s + _country_name);

  _continent = fields[CTY_CONTINENT];

  if ( !(CONTINENT_SET > _continent) )
    throw cty_error(CTY_INVALID_CONTINENT, "Continent = "s + _continent + " in record for "s + _country_name);
  
  auto_from_string(fields[CTY_LAT], _latitude);
  if (_latitude < -90 or _latitude > 90)
    throw cty_error(CTY_INVALID_LATITUDE, "Latitude = "s + fields[CTY_LAT] + " in record for "s + _country_name);

  auto_from_string(fields[CTY_LONG], _longitude);
  if (_longitude < -180 or _longitude > 180)
    throw cty_error(CTY_INVALID_LONGITUDE, "Longitude = "s + fields[CTY_LONG] + " in record for "s + _country_name);
  
// map to (0, 360)
  if (_longitude < 0)
    _longitude += 360;
  
  _utc_offset = static_cast<int>(from_string<float>(fields[CTY_UTC_OFFSET]) * 60 + 0.5);  // convert to minutes

  constexpr int MAX_OFFSET { 24 * 60 };     // maximum UTC offset, in minutes

  if ( (_utc_offset < -MAX_OFFSET) or (_utc_offset > MAX_OFFSET) )                 // check that it's reasonable
    throw cty_error(CTY_INVALID_UTC_OFFSET, "UTC offset = "s + fields[CTY_UTC_OFFSET] + " in record for "s + _country_name);

  _prefix = to_upper(fields[CTY_PREFIX]);  // so that, for example, JD/o -> JD/O
  
  if (_prefix.empty())
    throw cty_error(CTY_INVALID_PREFIX, "PREFIX is empty in record for "s + _country_name);
  
  if (_prefix == "*"s)
    throw cty_error(CTY_INVALID_PREFIX, "PREFIX is '*' in record for "s + _country_name);

  _waedc_country_only = (_prefix[0] == '*');    // is this only on the WAEDC list?
  
  if (_waedc_country_only)
    _prefix = _prefix.substr(1);        // remove the asterisk
   
// separate out the alternative prefixes and the alternative calls
  vector<string> alt_callsigns;
  vector<string> alt_prefixes;
  
//  for (const auto& candidate : presumptive_prefixes)
  for (auto candidate : clean_split_string <string_view> (fields[CTY_ALTS]))
  { vector<string>* vsp { ( contains(candidate, '=') ? &alt_callsigns : &alt_prefixes ) };  // callsigns are marked with an '='
  
    (*vsp) += candidate;
  }

// set the zone info for an aci 
  auto set_zone_info = [](alternative_country_info& aci, const auto cq_info, const auto itu_info)
    { if (!aci.cq_zone())
        aci.cq_zone(cq_info);
    
      if (!aci.itu_zone())
        aci.itu_zone(itu_info);  
    };

// save the alternative info; also modify the zone info now, since it will be faster later to retrieve it
// directly from here than to check for zero here and then retrieve from the main record
  for (const auto& alt_prefix : alt_prefixes)
  { alternative_country_info aci(alt_prefix, _prefix);  // alternative prefix, canonical prefix

    set_zone_info(aci, _cq_zone, _itu_zone);  
    
    _alt_prefixes += { aci.identifier(), aci };
  }

// do the same for the alternative callsigns
  for (const auto& alt_callsign : alt_callsigns)                   // alternative callsign, prefixed with "="
  { alternative_country_info aci(remove_char(alt_callsign, '='));  // remove the '=' from the alternative call

    set_zone_info(aci, _cq_zone, _itu_zone);
  
    _alt_callsigns += { aci.identifier(), aci };
  }

// map prefix if necessary
  if (map_prefix.contains(_prefix))
    _prefix = map_prefix.at(_prefix);
}

/*! \brief          Write a <i>cty_record</i> object to an output stream
    \param  ost     output stream
    \param  rec     object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const cty_record& rec)
{ ost << "Name: " << rec.country_name() << endl
      << "CQ zone: " << rec.cq_zone() << endl
      << "ITU zone: " << rec.itu_zone() << endl
      << "Continent: " << rec.continent() << endl
      << "Latitude: " << rec.latitude() << endl
      << "Longitude: " << rec.longitude() << endl
      << "UTC offset: " << rec.utc_offset() << endl
      << "Prefix: " << rec.prefix() << endl;
      
  if (rec.waedc_country_only())
    ost << "WAEDC only" << endl;
  
  if (!rec.alt_callsigns().empty())
    ost << "Alias callsigns:" << endl
        << rec.alt_callsigns();
        
  if (!rec.alt_prefixes().empty())
    ost << "Alias prefixes:" << endl
        << rec.alt_prefixes();
         
  return ost;
}

// -----------  alternative_country_info  ----------------

/*! \class  alternative_country_info
    \brief  A single alternative prefix of callsign for a country
  
            CTY files may contain "alias" information. This encapsulates that information.
*/

/*! \brief                      Construct from a string and a canonical country prefix
    \param  record              record from which to construct the alternative information
    \param  canonical_prefix    canonical country prefix

    <i>record</i> looks something like "=G4AMJ(14)[28]" or like "3H0(23)[42], where the delimited information
    is optional
*/
alternative_country_info::alternative_country_info(const string_view record, const string& canonical_prefix) :
  _country(canonical_prefix)
{ if (const size_t end_identifier { record.find_first_of("(["s) }; end_identifier == string::npos)
    _identifier = record;                                // no change
  else
  { _identifier = substring <std::string> (record, 0, end_identifier);      // read up to the first delimiter
  
    if (const string_view cq_zone_str { delimited_substring <std::string_view> (record, '(', ')', DELIMITERS::DROP) }; !cq_zone_str.empty())
    { auto_from_string(cq_zone_str, _cq_zone);

      if (_cq_zone < MIN_CQ_ZONE or _cq_zone > MAX_CQ_ZONE)
        throw cty_error(CTY_INVALID_CQ_ZONE, "CQ zone = "s + to_string(_cq_zone) + " in alternative record for "s + _identifier);
    }
  
    if (const string_view itu_zone_str { delimited_substring <std::string_view> (record, '[', ']', DELIMITERS::DROP) }; !itu_zone_str.empty())
    { auto_from_string(itu_zone_str, _itu_zone);

      if (_itu_zone < MIN_ITU_ZONE or _itu_zone > MAX_ITU_ZONE)
        throw cty_error(CTY_INVALID_ITU_ZONE, "ITU zone = "s + to_string(_itu_zone) + " in alternative record for "s + _identifier);
    }
  }
}

/*! \brief          Write an <i>alternative_country_info</i> object to an output stream
    \param  ost     output stream
    \param  aci     object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const alternative_country_info& aci)
{ ost << "alias: " << aci.identifier() << endl
      << "CQ zone: " << aci.cq_zone() << endl
      << "ITU zone: " << aci.itu_zone() << endl
      << "Country: " << aci.country();
      
  return ost;
}

// -----------  cty_data  ----------------

/*! \class  cty_data
    \brief  All the data from a CTY.DAT file
*/

/*! \brief              Construct from a file
    \param  filename    name of file
*/
cty_data::cty_data(const string_view filename)
{ const vector<string_view> records { split_string <std::string_view> ( remove_chars(read_file(filename), CRLF), ';') };                  // read file, remove EOL markers and split into records

  FOR_ALL(records, [this] (const string_view record_str) { emplace_back(cty_record { record_str }); } );    // applies to base class
}

// -----------  location_info  ----------------

/*! \class  location_info
    \brief  Location information associated with a call, prefix or country
        
    This is basically just a simple tuple
*/

/// location_info == location_info
bool location_info::operator==(const location_info& li) const
{ return ( (_canonical_prefix == li._canonical_prefix) and
           (_continent == li._continent) and
           (_country_name == li._country_name) and
           (_cq_zone == li._cq_zone) and
           (_itu_zone == li._itu_zone) and
           (_latitude == li._latitude) and
           (_longitude == li._longitude) and
           (_utc_offset == li._utc_offset) 
        );
}

/*! \brief          Set both latitude and longitude at once
    \param  lat     latitude in degrees (+ve north)
    \param  lon     longitude in degrees (+ve west)
*/
void location_info::latitude_longitude(const float lat, const float lon)
{ latitude(lat);
  longitude(lon);
}

/*! \brief          Set both CQ and ITU zones at once
    \param  cqz     CQ zone
    \param  ituz    ITU zone
*/
void location_info::zones(const unsigned int cqz, const unsigned int ituz)
{ cq_zone(cqz);
  itu_zone(ituz);
}

// ostream << location_info
ostream& operator<<(ostream& ost, const location_info& info)
{ ost << "Country name: " << info.country_name() << endl
      << "Canonical prefix: " << info.canonical_prefix() << endl
      << "CQ zone: " << info.cq_zone() << endl
      << "ITU zone: " << info.itu_zone() << endl
      << "Continent: " << info.continent() << endl
      << "Latitude: " << info.latitude() << endl
      << "Longitude: " << info.longitude() << endl
      << "UTC offset: " << info.utc_offset() << endl
      << "Region name: " << info.region_name() << endl
      << "Region abbreviation: " << info.region_abbreviation();
  
  return ost;
}

/*! \brief          Get the call area of a VE/W/VK call
    \param  call    callsign
    \return         best guess for the call area of <i>call</i>

    NB This does not appear in the header file
*/
unsigned int get_call_area(const string_view call)
{ auto to_uint = [] (const char c) { return static_cast<unsigned int>(c - '0'); };

  constexpr unsigned int rv { 0 };    // default

  const auto n_slashes { std::ranges::count(call, '/') };

  switch (n_slashes)
  { case 0 :
    { const size_t posn { call.find_last_of(DIGITS) };

      return ( (posn != string::npos) ? to_uint(call[posn]) : rv );  // should always be true, unless it's a partial call
    }

    default :  // find the shortest one with a digit
    {
// all the parts that contains a digit
      const vector<string_view> parts_with_digit { CREATE_AND_FILL <vector<string_view>> (split_string <string_view> (call, '/'), [] (const string_view sv) { return (::contains_digit(sv)); }) };

      switch (parts_with_digit.size())
      { case 0 :                              // should never happen
        default :
          return rv;

        case 1 :
          return to_uint(first_digit(parts_with_digit[0]));

        case 2 :
          return to_uint(first_digit(parts_with_digit[ ((parts_with_digit[0].size() < parts_with_digit[1].size()) ? 0 : 1) ]));
      }
    }
  }
}

/*! \brief          Guess the CQ and ITU zones if the canonical prefix indicates a country with multiple zones
    \param  call    callsign
    \param  li      location_info to use for the guess
    \return         best guess for the CQ and ITU zones

    Currently this supports just VE, VK and W for CQ zones, and VE for ITU zones
 */
location_info guess_zones(const string_view call, const location_info& li)
{ location_info rv { li };

// if it's a VE, then make a guess as to the CQ and ITU zones
  if (rv.canonical_prefix() == "VE"sv)
  { const unsigned int call_digit { get_call_area(call) };

    rv.zones(VE_CQ[call_digit], VE_ITU[call_digit]);

// lat/long for VEs; for now, assume VE, not VY or VO
    switch (call_digit)
    { case 2 :
        rv.latitude_longitude(45.57, 73.62);              // Montreal
        break;

      case 3 :
        rv.latitude_longitude(46.52, 80.97);              // Sudbury
        break;

      case 4 :
        rv.latitude_longitude(49.88, 97.15);              // Winnipeg
        break;

      case 5 :
        rv.latitude_longitude(52.15, 106.67);             // Saskatoon
        break;

      case 6 :
        rv.latitude_longitude(51.03, 114.09);             // Calgary
        break;

      case 7 :
        rv.latitude_longitude(49.31, 123.04);             // Vancouver
        break;
    }
  }

// if it's a W, then make a guess as to the CQ and ITU zones
  if (rv.canonical_prefix() == "K"sv)
  { const unsigned int call_digit { get_call_area(call) };

    rv.zones( W_CQ[call_digit], W_ITU[call_digit] );

// lat/long for W zones
    switch (rv.cq_zone())
    { case 3 :
        rv.latitude_longitude(40.79, 115.54);
        break;

      case 4 :
        rv.latitude_longitude(39.12, 101.98);
        break;

      case 5 :
        rv.latitude_longitude(36.55, 79.65);
        break;
    }
  }

  if (rv.canonical_prefix() == "VK"sv)
  { const unsigned int call_area { get_call_area(call) };

    if ( (call_area == 6) or (call_area == 8) )
      rv.cq_zone(29);                             // I don't know why this isn't in the cty.dat file
  }  

  return rv;
}

// -----------  location_database  ----------------

/*! \class  location_database
    \brief  The country-based location information packaged for use by drlog
*/

/*! \brief                  Initialise the database
    \param  cty             cty.dat data
    \param  country_list    type of country list
*/
void location_database::_init(const cty_data& cty, const COUNTRY_LIST country_list)
{
// re-organize the cty data according to the correct country list  
  switch (country_list)
  { case COUNTRY_LIST::DXCC:                                                       // use DXCC countries only
    { for (const cty_record& rec : cty)
      { if (!rec.waedc_country_only())    // ignore WAEDC-only entries
        { const location_info li { rec };

// insert the canonical entry for this country
          _db += { li.canonical_prefix(), li };        

// insert other prefixes and calls in the same country
          _insert_alternatives(li, rec.alt_prefixes());
          _insert_alternatives(li, rec.alt_callsigns());
        }
      }
      break;
    }
    
    case COUNTRY_LIST::WAEDC:
    {
// start by copying all the useful information for all records      
      for (const cty_record& rec : cty)
      { const location_info li { rec };

// insert the canonical entry for this country
        _db += { li.canonical_prefix(), li };

        _process_alternative(rec, ALTERNATIVES::CALLSIGNS);
        _process_alternative(rec, ALTERNATIVES::PREFIXES);
      }
      
      break;
    }    
  }
}

/*! \brief                  Insert alternatives into the database
    \param  info            the original location information
    \param  alternatives    alternative country information
*/
void location_database::_insert_alternatives(const location_info& info, const ACI_DBTYPE& alternatives)
{ location_info info_copy { info };

  for (const auto& [call_or_prefix, aci] : alternatives)
    _db += { call_or_prefix, ( info_copy.zones(aci.cq_zone(), aci.itu_zone()), info_copy) };
}

/*! \brief              Process alternatives from a record
    \param  rec         the record to process
    \param  alt_type    type of alternatives to process
*/
void location_database::_process_alternative(const cty_record& rec, const enum ALTERNATIVES alt_type)
{ const ACI_DBTYPE& alts { alt_type == ALTERNATIVES::CALLSIGNS ? rec.alt_callsigns() : rec.alt_prefixes() };

  LOCATION_DBTYPE& db { alt_type == ALTERNATIVES::CALLSIGNS ? _alt_call_db : _db };

  const bool country_is_waedc_only { rec.waedc_country_only() };
    
  auto add_to_database = [&db] (const cty_record& rec, const alternative_country_info& aci, const string& prefix_or_callsign)
    { location_info info { rec };

      info.zones(aci.cq_zone(), aci.itu_zone());
            
      db += { prefix_or_callsign, info };
    };
    
  for ( const auto& [ prefix_or_callsign, aci /* alternative_country_info */ ] : alts )
  { if (country_is_waedc_only)            // if country is WAEDC only and there's an entry already for this prefix or call, delete it before inserting
    { if (const auto db_posn { db.find(prefix_or_callsign) }; db_posn != db.cend())
        db.erase(db_posn);
    
      add_to_database(rec, aci, prefix_or_callsign);
    }
    else                                    // country is in DXCC list; don't add if there's an entry already
    { if (!(db.contains(prefix_or_callsign)))    // if it's not already in the database
        add_to_database(rec, aci, prefix_or_callsign);
    }
  }
}

// construct from CTY.DAT filename and the definition of which country list to use
location_database::location_database(const string_view filename, const COUNTRY_LIST country_list)
{ if (!filename.empty())
    _init(cty_data { filename }, country_list);
}

/*! \brief              Add Russian information
    \param  path        vector of directories to check for file <i>filename</i>
    \param  filename    name of file containing Russian information
*/
void location_database::add_russian_database(const vector<string>& path, const string_view filename)
{ if (filename.empty())
    return;

  try
  { _russian_db = russian_data(path, filename).data();
  }

  catch (...)
  { ost << "Error attempting to add Russian database from file: " << filename << endl;
  }
}

/*! \brief              Get location information for a particular call or partial call
    \param  callpart    call (or partial call)
    \return             location information corresponding to <i>call</i>
*/
location_info location_database::info(const string_view callpart) const
{ const string original_callsign { remove_peripheral_spaces <std::string> (callpart) };

  string callsign { original_callsign };                  // make callsign mutable, for handling case of /n
  
  lock_guard lg(_location_database_mutex);

  LOCATION_DBTYPE::const_iterator db_posn { _db_checked.find(callsign) };

// it's easy if there's already an entry
  if (const auto opt { OPT_MUM_VALUE(_db_checked, callsign) }; opt)
    return opt.value();
  
// see if there's an exact match in the alternative call db
  if (const auto opt { OPT_MUM_VALUE(_alt_call_db, callsign) }; opt)
  { _db_checked += { callsign, opt.value() };
    return opt.value();
  }

  auto insert_best_info = [&callsign, this] (const location_info& li) { _db_checked += { callsign, li };
                                                                        return li;
                                                                      };

// see if it's some guy already in the db but now signing /QRP
  if ( (callsign.length() >= 5) and callsign.ends_with("/QRP"sv) )
  { const string_view target { remove_n_chars_from_end <std::string_view> (callsign, 4u) };    // remove "/QRP"

    if (const auto opt { OPT_MUM_VALUE(_db_checked, target) }; opt)
      return insert_best_info(opt.value());

// try the alternative call db
    db_posn = _alt_call_db.find(target);

    if (db_posn != _alt_call_db.end())
      return insert_best_info(db_posn->second);
  }

// /MM and /AM are in no country
  if (callsign.ends_with("/AM"sv) or callsign.ends_with("/MM"sv))
    return insert_best_info(location_info());
  
// try to determine the canonical prefix
  if (!contains(callsign, '/') or ( (callsign.length() >= 2) and (penultimate_char(callsign) == '/') ))    // "easy" -- no portable indicator
  {
// country is determined by the longest substring starting at the start of the call and which is already
// in the database. This assumes that, for example, G4AMJ is in the same country as G4AM [if G4AM has already been worked]).
// I can think of counter-examples to do with the silly US call system, but at least this is a starting point.
    bool found_any_hits { false };              // no hits so far with this call

    string        best_fit;
    location_info best_info;

    unsigned int len { 1 };

    if ( (callsign.length() >= 2) and (penultimate_char(callsign) == '/') and isdigit(last_char(callsign)) )    // if /n; this changes callsign
    { const size_t last_digit_posn { substring <std::string_view> (callsign, 0, callsign.length() - 2).find_last_of(DIGITS) };

      if (last_digit_posn != string::npos)
      { callsign[last_digit_posn] = last_char(callsign);
        callsign = substring <std::string> (callsign, 0, callsign.length() - 2);
      }
    }

    while (len <= callsign.length())
    { const string                          target  { callsign.substr(0, len) };
      const LOCATION_DBTYPE::const_iterator db_posn { _db.find(target) };
     
      if (db_posn != _db.end())
      { found_any_hits = true;
        best_fit = target;
        best_info = db_posn->second;
      }
      else                                   // didn't find this substring
      { // delete this because of KH6, where K is a hit, KH6 is a hit, but KH is not
      }

      len++;
    }

//   auto redefine_best = [this] (const string& cp) { return pair { cp, _db.find(cp) -> second }; };
   auto redefine_best = [this] (const string_view cp) { return pair { string { cp }, _db.find(cp) -> second }; };

// Guantanamo Bay is a mess
    if ( (best_fit == "KG4"sv) and (callsign.length() != 5) )
      tie(best_fit, best_info) = redefine_best("K"sv);
    
// special stuff for Greek call areas
    if ( (best_fit == "SV"sv) and (penultimate_char(callsign) == '/') and isdigit(last_char(callsign)) )
    { const char lc { last_char(callsign) };
    
      if (lc == '5')
        tie(best_fit, best_info) = redefine_best("SV5"sv);

      if (lc == '9')
        tie(best_fit, best_info) = redefine_best("SV9"sv);
    }
    
// and Ecuador
    if ( (best_fit == "HC"sv) and (penultimate_char(callsign) == '/') and (last_char(callsign) == '8') )
      tie(best_fit, best_info) = redefine_best("HC8"sv);
    
    if (found_any_hits)                                 // return the best fit
    { best_info = guess_zones(callsign, best_info);

// insert Russian information
      static const STRING_SET RUSSIAN_COUNTRIES { "UA"s, "UA2"s, "UA9"s };

      if (RUSSIAN_COUNTRIES.contains(best_info.canonical_prefix()))
      { if (const size_t posn_first_digit { callsign.find_first_of(DIGITS) }; posn_first_digit != string::npos)
        { if (const size_t posn_first_suffix_letter { callsign.find_first_not_of(DIGITS, posn_first_digit) }; posn_first_suffix_letter != string::npos)
          { const string sub { create_string(callsign[posn_first_digit]) + create_string(callsign[posn_first_suffix_letter]) };

            if (const auto map_it { _russian_db.find(sub) }; (map_it != _russian_db.end()))
            { const russian_data_per_substring& data { map_it->second };

              best_info.zones(data.cq_zone(), data.itu_zone());
              best_info.latitude_longitude(data.latitude(), data.longitude());
              best_info.region_name(data.region_name());
              best_info.region_abbreviation(data.region_abbreviation());
            }
          }
        }
      }

      _db_checked += { original_callsign, best_info };
       
      return best_info;
    }

// we get here if there were never any hits
    return location_info();
  }  // end no portable indicator

// it looks like maybe it's a reciprocal license

  if (callsign.starts_with('/') or callsign.ends_with('/'))   // slash is first or last character
    return location_info();

// how many slashes are there?
  const vector<string> parts { split_string <std::string> (callsign, '/') };

  if (parts.size() > 3)
    throw location_error(LOCATION_TOO_MANY_SLASHES, to_string(parts.size() - 1) + " slashes in call: "s + callsign);

  if (parts.size() == 2)        // one slash
  {
// see if either part matches anything in the initial database
    const LOCATION_DBTYPE::const_iterator db_posn_0 { _db.find(parts[0]) };
    const LOCATION_DBTYPE::const_iterator db_posn_1 { _db.find(parts[1]) };
    const bool                            found_0   { (db_posn_0 != _db.end()) };
    const bool                            found_1   { (db_posn_1 != _db.end()) };

    if (found_0 and !found_1)                        // first part had an exact match
      return insert_best_info( guess_zones(callsign, db_posn_0->second) );

// we have to deal with stupid calls like K4/RU4W, where the second part is an entry in cty.dat;
// add them on a case by case basis, rather than using all possible long prefixes listed in cty.dat, since this
// should be a very rare occurrence
    static const STRING_SET russian_long_prefixes { "RU4W"s };

    if (found_1 and !found_0)                               // second part had an exact match
    { if (!(russian_long_prefixes > parts[1]))              // the normal case
        return insert_best_info( guess_zones(callsign, db_posn_1->second) );
      else                                                  // the pathological case, a call like "K4/RU4W"
        return insert_best_info( info(parts[0]) );
    }

    if (found_0 and found_1)                            // both parts had an exact match (should never happen: KH6/KP2)
      return insert_best_info( guess_zones(callsign, ((parts[0].length() > parts[1].length()) ? db_posn_0 : db_posn_1) -> second) );    // choose longest match

    if (!found_0 and !found_1)    // neither matched exactly; use one that ends with a digit if there is one
    { const bool first_ends_with_digit  { is_digit(last_char(parts[0])) };
      const bool second_ends_with_digit { is_digit(last_char(parts[1])) };
      
      if (first_ends_with_digit and !second_ends_with_digit)
        return info(parts[0]);

      if (second_ends_with_digit and !first_ends_with_digit)
        return info(parts[1]);
    }

    if (!found_0 and !found_1)    // neither matched exactly; use the one with the longest match
    {
// length of match for part 0
      auto match_info = [this] (const string& part)
        { unsigned int return_len { 0 };

          LOCATION_DBTYPE::const_iterator return_posn;

          unsigned int len { 1 };

          while (len <= part.length())
          { const string target { part.substr(0, len) };

            if (const LOCATION_DBTYPE::const_iterator db_posn { _db.find(target) }; db_posn != _db.end())
            { return_len = len;
              return_posn = db_posn;
            }

            ++len;
          }

          return pair<unsigned int, LOCATION_DBTYPE::const_iterator> { return_len, return_posn };
        };

      const auto [ len_0, db_posn_0 ] = match_info(parts[0]);
      const auto [ len_1, db_posn_1 ] = match_info(parts[1]);
      
      if (len_0 != len_1)   // if one has a longer match, use it 
        return insert_best_info( guess_zones(callsign, ((len_0 > len_1) ? db_posn_0 : db_posn_1) -> second) );

// they both match equally well; choose shortest
// neither matched at all
      if (len_0 == 0)
        return location_info();    // we know nothing about either part of the call
      
      if (parts[0].length() == parts[1].length())
        return insert_best_info( guess_zones(callsign, ((parts[0].length() < parts[1].length()) ? db_posn_0 : db_posn_1) -> second) );
 
// same length; arbitrarily choose the first
      return insert_best_info( guess_zones(callsign, db_posn_0->second) );
    }
  }
  
  if (parts.size() == 3)        // two slashes
  {
// ignore the second slash and everything after it (assume W0/G4AMJ/P or G4AMJ/VP9/M)

    const string        target    { (parts[2].length() == 1) ? (parts[0] + '/' + parts[1]) : parts[0] };
    const location_info best_info { info(target) };   // recursive, so we need ref count in safelock
    
    _db_checked -= target;
    _db_checked += { callsign, best_info };
        
    return best_info;   
  }

  throw exception();
}

/// get a set of all the canonical prefixes for all countries
auto location_database::countries(void) const -> UNORDERED_STRING_SET
{ std::lock_guard lg(_location_database_mutex);

//  using RT = std::invoke_result_t<decltype(&location_database::countries())>;

//  RT rv1;

//  unordered_set<string> rv;     // there's probably some horrible way to set the type using decltype and the return type of the function, but I don't know what it is
  UNORDERED_STRING_SET rv;     // there's probably some horrible way to set the type using decltype and the return type of the function, but I don't know what it is

//  using RT = std::invoke_result_t< decltype(&location_database::countries), decltype(this), void >; // error: 'decltype' cannot resolve address of overloaded function
//  using RT = std::invoke_result_t< decltype( &((location_database::countries)(void) const)), decltype(this), void >;    // error: expected primary-expression before 'void'

//  FOR_ALL(_db, [&rv] (const pair<string, location_info>& prefix_li) { rv += prefix_li.second.canonical_prefix(); } );  // there are a lot more entries in the db than there are countries
// not allowed  FOR_ALL(_db, [&rv] (const auto& [ prefix, li ]) { rv += li.canonical_prefix(); } );  // there are a lot more entries in the db than there are countries

  for (const auto& [_, li] : _db)
   { rv += li.canonical_prefix(); }

  return rv;
}

/// create a set of all the canonical prefixes for a particular continent
//UNORDERED_STRING_SET location_database::countries(const string& cont_target) const
UNORDERED_STRING_SET location_database::countries(const string_view cont_target) const
{ UNORDERED_STRING_SET rv { };

  ranges::copy_if(countries(), inserter(rv, rv.begin()), [cont_target, this, &rv] (const string& cp) { return (continent(cp) == cont_target); } );

  return rv;
}

// -----------  russian_data_per_substring  ----------------

/*! \class  russian_data_per_substring
    \brief  Encapsulate the data from a Russian data file, for a single district's substring
*/

/*! \brief              Construct from a prefix and a line
    \param  sbstring    the prefix for the Russian district
    \param  line        line from Russian data file
*/
russian_data_per_substring::russian_data_per_substring(const string_view ss, const string_view line) :
  _sstring(ss)
{
// check that the prefix matches the line
  const vector<string> substrings { clean_split_string <std::string> (delimited_substring <std::string_view> (line, '[', ']', DELIMITERS::DROP), ',') };

  if (!contains(substrings, ss))
    throw russian_error(RUSSIAN_INVALID_SUBSTRING, "Substring "s + ss + " not found"s);

  const size_t posn_1 { line.find(']') };
  const size_t posn_2 { line.find(':') };

  if (posn_1 == posn_2)
    throw russian_error(RUSSIAN_INVALID_FORMAT, "Delimiter not found"s);

  _region_name = ::substring <std::string> (line, posn_1 + 1, posn_2 - posn_1 - 1);

  const vector<string> fields { clean_split_string <std::string> (remove_peripheral_spaces <std::string> (squash(line.substr(posn_2 + 1), ' ')), ' ') };

  try
  { _region_abbreviation = fields.at(0);
    _cq_zone = from_string<unsigned int>(fields.at(1));
    _itu_zone = from_string<unsigned int>(fields.at(2));
    _continent = fields.at(3);
    _utc_offset = from_string<int>(fields.at(4));
    _latitude = from_string<float>(fields.at(5));
    _longitude = from_string<float>(fields.at(6));
  }

  catch (...)
  { throw russian_error(RUSSIAN_INVALID_FORMAT, "Error parsing fields"s);
  }
}

/*! \brief          Write a <i>russian_data_per_substring</i> object to an output stream
    \param  ost     output stream
    \param  info    object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const russian_data_per_substring& info)
{ ost << "substring: " << info.sstring() << endl
      << "region name: " << info.region_name() << endl
      << "region abbreviation: " << info.region_abbreviation() << endl
      << "cq zone: " << info.cq_zone() << endl
      << "itu zone: " << info.itu_zone() << endl
      << "continent: " << info.continent() << endl
      << "utc offset: " << info.utc_offset() << endl
      << "latitude: " << info.latitude() << endl
      << "longitude: " << info.longitude();

  return ost;
}

// -----------  russian_data  ----------------

/*! \class  russian_data
    \brief  Encapsulate the data from a Russian data file
*/

/*! \brief              Construct from a file
    \param  path        the directory path to be searched in order
    \param  filename    the name of the file to be read
*/
russian_data::russian_data(const vector<string>& path, const string_view filename)
{ try
  { const vector<string> lines { to_lines <std::string> (replace_char(read_file(path, filename), '\t', ' ')) };

    for (const auto& line : lines)
    { if (!line.starts_with("//"s))   // remove comments
      { const vector<string> substrings { clean_split_string <std::string> (delimited_substring <std::string> (line, '[', ']', DELIMITERS::DROP), ',') };

        FOR_ALL(substrings, [&line, this] (const auto& sstring) { _data += { sstring, russian_data_per_substring(sstring, line) }; });
      }
    }
  }

  catch (...)
  { ost << "Error processing Russian data file: " << filename << endl;
  }
}
