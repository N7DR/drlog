// $Id: cty_data.cpp 141 2017-12-16 21:19:10Z  $

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

extern const set<string> CONTINENT_SET { "AF", "AS", "EU", "NA", "OC", "SA", "AN" };  // see https://stackoverflow.com/questions/177437/const-static

const unsigned int CTY_FIELDS_PER_RECORD = 9;                                                   ///< Number of fields in a single CTY record
const array<int, 10> VE_CQ = { { 5 /* probably not correct */, 5, 5, 4, 4, 4, 4, 3, 1, 5 }  };  ///< default CQ zones for VE call areas; 0 to 9
const array<int, 10> W_CQ = { { 4, 5, 5, 5, 5, 4, 3, 3, 4, 4 } };                               ///< default CQ zones for W call areas; 0 to 9
const array<int, 10> VE_ITU = { { 8 /* probably not correct */, 9, 4, 4, 3, 3, 2, 2, 4, 9 }  }; ///< default ITU zones for VE call areas; 0 to 9
const array<int, 10> W_ITU = { { 7, 8, 4, 4, 4, 7, 6, 6, 4, 4 } };                              ///< default ITU zones for W call areas; 0 to 9

// -----------  cty_record  ----------------

/*! \class cty_record
    \brief A single record in the CTY.DAT file
  
    The official page describing the format is:
      http://www.country-files.com/cty/format.htm.
  
    Spacing in the file is "for readability only", so
    we use the official delimiter ":" for fields, and
    ";" for records.
*/

/*! \brief  Construct from a string

            The string is assumed to contain a single record. We don't catch all
            possible errors, but we do test for the most obvious ones.
*/
cty_record::cty_record(const string& record)
{ const string record_copy = remove_char(remove_char(record, LF_CHAR), CR_CHAR);            // make it all one line
  const vector<string> fields = remove_peripheral_spaces(split_string(record_copy, ":"));   // split the record into fields

  if (fields.size() != CTY_FIELDS_PER_RECORD)                                       // check the number of fields
    throw cty_error(CTY_INCORRECT_NUMBER_OF_FIELDS, "Found " + to_string(fields.size()) + " fields in record for " + fields[0]); 
  
  _country_name = fields[0];

  _cq_zone = from_string<int>(fields[1]);
  if (_cq_zone < 1 or _cq_zone > 40)
    throw cty_error(CTY_INVALID_CQ_ZONE, "CQ zone = " + to_string(_cq_zone) + " in record for " + _country_name);
  
  _itu_zone = from_string<int>(fields[2]);
  if (_itu_zone < 1 or _itu_zone > 90)
    throw cty_error(CTY_INVALID_ITU_ZONE, "ITU zone = " + to_string(_itu_zone) + " in record for " + _country_name);

  _continent = fields[3];

  if ( !(CONTINENT_SET < _continent) )
    throw cty_error(CTY_INVALID_CONTINENT, "Continent = " + _continent + " in record for " + _country_name);
  
  _latitude = from_string<float>(fields[4]);
  if (_latitude < -90 or _latitude > 90)
    throw cty_error(CTY_INVALID_LATITUDE, "Latitude = " + fields[4] + " in record for " + _country_name);

  _longitude = from_string<float>(fields[5]);
  if (_longitude < -180 or _longitude > 180)
    throw cty_error(CTY_INVALID_LONGITUDE, "Longitude = " + fields[5] + " in record for " + _country_name);
  
// map to (0, 360)
  if (_longitude < 0)
    _longitude += 360;
  
  _utc_offset = static_cast<int>(from_string<float>(fields[6]) * 60 + 0.5);  // convert to minutes

  if ( (_utc_offset < -24 * 60) or (_utc_offset > 24 * 60) )                 // check that it's reasonable
    throw cty_error(CTY_INVALID_UTC_OFFSET, "UTC offset = " + fields[6] + " in record for " + _country_name);

  _prefix = to_upper(fields[7]);  // so that, for example, JD/o -> JD/O
  
  if (_prefix.empty())
    throw cty_error(CTY_INVALID_PREFIX, "PREFIX is empty in record for " + _country_name);
  
  if (_prefix == "*")
    throw cty_error(CTY_INVALID_PREFIX, "PREFIX is '*' in record for " + _country_name);

  _waedc_country_only = (_prefix[0] == '*');    // is this only on the WAEDC list?
  
  if (_waedc_country_only)
    _prefix = _prefix.substr(1);

// now we have to get tricky... start by getting the presumptive alternative prefixes
  const vector<string> presumptive_prefixes = remove_peripheral_spaces(split_string(fields[8], ","));
   
// separate out the alternative prefixes and the alternative calls
  vector<string> alt_callsigns;
  vector<string> alt_prefixes;
  
  for (const auto& candidate : presumptive_prefixes)
  { vector<string>* vsp = ( contains(candidate, "=") ? &alt_callsigns : &alt_prefixes );  // callsigns are marked with an '='
  
    vsp->push_back(candidate);
  }

// remove the '=' from all the the alternative calls
  FOR_ALL(alt_callsigns, [] (string& alt_callsign) { alt_callsign = remove_char(alt_callsign, '='); } );

// save the alternative info; also modify the zone info now, since it will be faster later to retrieve it
// directly from here than to check for zero here and then retrieve from the main record
  for (const auto& alt_prefix : alt_prefixes)
  { alternative_country_info aci(alt_prefix, _prefix);

    if (!aci.cq_zone())
      aci.cq_zone(_cq_zone);
    
    if (!aci.itu_zone())
      aci.itu_zone(_itu_zone);  
    
    _alt_prefixes.insert( { aci.identifier(), aci } );
  }

// do the same for the alternative callsigns
  for (const auto& alt_callsign : alt_callsigns)
  { alternative_country_info aci(alt_callsign);

    if (!aci.cq_zone())
      aci.cq_zone(_cq_zone);
    
    if (!aci.itu_zone())
      aci.itu_zone(_itu_zone);  
  
    _alt_callsigns.insert( { aci.identifier(), aci } );
  }
}

// ostream << cty_record
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

/*! \brief                      Construct from a string
    \param  record              record from which to construct the alternative information
    \param  canonical_prefix    canonical country prefix
        
  <i>record</i> looks something like "G4AMJ(14)[28]", where the delimited information
  is optional
*/
alternative_country_info::alternative_country_info(const string& record, const string& canonical_prefix) :
  _cq_zone(0),
  _itu_zone(0),
  _country(canonical_prefix)
{ size_t end_identifier = record.find_first_of("([");

  if (end_identifier == string::npos)
    _identifier = record;                                // no change
  else
  { _identifier = substring(record, 0, end_identifier);      // read up to the first delimiter
  
    const string cq_zone_str = delimited_substring(record, '(', ')');
  
    if (!cq_zone_str.empty())
    { _cq_zone = from_string<int>(cq_zone_str);

      if (_cq_zone < 1 or _cq_zone > 40)
        throw cty_error(CTY_INVALID_CQ_ZONE, "CQ zone = " + to_string(_cq_zone) + " in alternative record for " + _identifier);
    }
       
    const string itu_zone_str = delimited_substring(record, '[', ']');
  
    if (!itu_zone_str.empty())
    { _itu_zone = from_string<int>(itu_zone_str);

      if (_itu_zone < 1 or _itu_zone > 90)
        throw cty_error(CTY_INVALID_ITU_ZONE, "ITU zone = " + to_string(_itu_zone) + " in alternative record for " + _identifier);
    }
  }
}

/// ostream << alternative_country_info
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
cty_data::cty_data(const string& filename)
{ const string entire_file = remove_char(remove_char(read_file(filename), LF_CHAR), CR_CHAR);   // read file and remove EOL markers
  const vector<string> records = split_string(entire_file, ";");                                // split into records
  
  FOR_ALL(records, [&] (const string& record) { _data.push_back(static_cast<cty_record>(record)); } );
}

/*! \brief              Construct from a file
    \param  path        directories in which to search for <i>filename</i>, in order
    \param  filename    name of file
*/
cty_data::cty_data(const vector<string>& path, const string& filename)
{ const string entire_file = remove_char(remove_char(read_file(path, filename), LF_CHAR), CR_CHAR);     // read file and remove EOL markers
  const vector<string> records = split_string(entire_file, ";");                                        // split into records

  FOR_ALL(records, [&] (const string& record) { _data.push_back(static_cast<cty_record>(record)); } );
}

// -----------  location_info  ----------------

/*! \class  location_info
    \brief  Location information associated with a call, prefix or country
        
    This is basically just a simple tuple
*/

/// default constructor
location_info::location_info(void) :
  _canonical_prefix("NONE"),
  _continent("XX"),
  _country_name("None"),
  _cq_zone(0),
  _itu_zone(0),
  _latitude(0),
  _longitude(0),
  _utc_offset(0)
{ }
  
/// construct from a cty.dat record
location_info::location_info(const cty_record& rec) :
  _canonical_prefix(rec.prefix()),
  _continent(rec.continent()),
  _country_name(rec.country_name()),
  _cq_zone(rec.cq_zone()),
  _itu_zone(rec.itu_zone()),
  _latitude(rec.latitude()),
  _longitude(rec.longitude()),
  _utc_offset(rec.utc_offset())
{ }

/// location_info == location_info
const bool location_info::operator==(const location_info& li) const
{ if (_canonical_prefix != li._canonical_prefix)
    return false;

  if (_continent != li._continent)
    return false;

  if (_country_name != li._country_name)
    return false;

  if (_cq_zone != li._cq_zone)
    return false;

  if (_itu_zone != li._itu_zone)
    return false;

  if (_latitude != li._latitude)
    return false;

  if (_longitude != li._longitude)
    return false;

  if (_utc_offset != li._utc_offset)
    return false;

  return true;
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

/*! \brief          Guess the CQ and ITU zones if the canonical prefix indicates a country with multiple zones
    \param  call    callsign
    \param  li      location_info to use for the guess
    \return         best guess for the CQ and ITU zones

    Currently this supports just VE, VK and W for CQ zones, and VE for ITU zones
 */
const location_info guess_zones(const string& callsign, const location_info& bi)
{ location_info rv = bi;

// if it's a VE, then make a guess as to the CQ and ITU zones
   if (rv.canonical_prefix() == "VE")
   { //const size_t posn = callsign.find_last_of("0123456789");
     const size_t posn = callsign.find_last_of(DIGITS);

     if (posn != string::npos)
     { rv.cq_zone(VE_CQ[from_string<unsigned int>(string(1, callsign[posn]))]);
       rv.itu_zone(VE_ITU[from_string<unsigned int>(string(1, callsign[posn]))]);
     }
   }

// if it's a W, then make a guess as to the CQ and ITU zones
   if (rv.canonical_prefix() == "K")
   { //const size_t posn = callsign.find_last_of("0123456789");
     const size_t posn = callsign.find_last_of(DIGITS);

     if (posn != string::npos)
     { rv.cq_zone(W_CQ[from_string<unsigned int>(string(1, callsign[posn]))]);
       rv.itu_zone(W_ITU[from_string<unsigned int>(string(1, callsign[posn]))]);

// lat/long for W zones
       switch (rv.cq_zone())
       { case 3:
           rv.latitude(40.79);
           rv.longitude(115.54);
           break;

         case 4:
           rv.latitude(39.12);
           rv.longitude(101.98);
           break;

         case 5:
           rv.latitude(36.55);
           rv.longitude(79.65);
           break;
       }
     }
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
void location_database::_init(const cty_data& cty, const enum country_list_type country_list)
{
// re-organize the cty data according to the correct country list  
  switch (country_list)
  { case COUNTRY_LIST_DXCC:                                                       // use DXCC countries only
    { for (unsigned int n_country = 0; n_country < cty.n_countries(); ++n_country)
      { const cty_record& rec = cty[n_country];
    
        if (!rec.waedc_country_only())    // ignore WAEDC-only entries
        { const location_info info(rec);

// insert the canonical entry for this country
          _db.insert( { info.canonical_prefix(), info } );
        
// insert other prefixes and calls in the same country
          _insert_alternatives(info, rec.alt_prefixes());
          _insert_alternatives(info, rec.alt_callsigns());
        }
      }
      break;
    }
    
    case COUNTRY_LIST_WAEDC:
    {
// start by copying all the useful information for all records      
      for (unsigned int n_country = 0; n_country < cty.n_countries(); ++n_country)
      { const cty_record& rec = cty[n_country];
        const location_info info(rec);

// insert the canonical entry for this country
        _db.insert( { info.canonical_prefix(), info } );
      }
      
// now do the alternative prefixes
      for (unsigned int n_country = 0; n_country < cty.n_countries(); ++n_country)
      { const cty_record& rec = cty[n_country];
        const map<string, alternative_country_info>& alt_prefixes = rec.alt_prefixes();
        const bool country_is_waedc_only = rec.waedc_country_only();
        
        for (map<string, alternative_country_info>::const_iterator cit = alt_prefixes.cbegin(); cit != alt_prefixes.cend(); ++cit)
        { const string& prefix = cit->first;
          const alternative_country_info& aci = cit->second;
        
          if (country_is_waedc_only)            // if country is WAEDC only and there's an entry already for this prefix, delete it before inserting
          { map<string, location_info>::const_iterator db_posn = _db.find(prefix);
          
            if (db_posn != _db.cend())
              _db.erase(db_posn);
    
            location_info info(rec);

            info.cq_zone(aci.cq_zone());        
            info.itu_zone(aci.itu_zone());         
            
            _db.insert( { prefix, info } );
          }
          else                                  // country is in DXCC list; don't add if there's an entry already
          { map<string, location_info>::const_iterator db_posn = _db.find(prefix);
          
            if (db_posn == _db.cend())    // if it's not already in the database
            { location_info info(rec);

              info.cq_zone(aci.cq_zone());        
              info.itu_zone(aci.itu_zone());         
            
              _db.insert( { prefix, info } );
            }
          }
        }
      }

// do essentially the same for the alternative callsigns -- we should just make this a callable private function and call it twice;
// these go into the _alt_call_db
      for (unsigned int n_country = 0; n_country < cty.n_countries(); ++n_country)
      { const cty_record& rec = cty[n_country];
        const map<string, alternative_country_info>& alt_callsigns = rec.alt_callsigns();
        const bool country_is_waedc_only = rec.waedc_country_only();
        
        for (map<string, alternative_country_info>::const_iterator cit = alt_callsigns.begin(); cit != alt_callsigns.end(); ++cit)
        { const string callsign = cit->first;
          const alternative_country_info& aci = cit->second;
        
          if (country_is_waedc_only)        // if country is WAEDC only and there's an entry already for this callsign, delete it before inserting
          { const auto db_posn = _alt_call_db.find(callsign);
          
            if (db_posn != _alt_call_db.end())
              _alt_call_db.erase(db_posn);
    
            location_info info(rec);

            info.cq_zone(aci.cq_zone());        
            info.itu_zone(aci.itu_zone());         
            
            _alt_call_db.insert( { callsign, info } );
          }
          else                              // country is in DXCC list; don't add if there's an entry already for this callsign
          { const auto db_posn = _alt_call_db.find(callsign);
          
            if (db_posn == _alt_call_db.end())
            { location_info info(rec);

              info.cq_zone(aci.cq_zone());        
              info.itu_zone(aci.itu_zone()); 
             
              _alt_call_db.insert( { callsign, info } );
            }
          }
        }
      }      
      break;
    }    
  }
}

/*! \brief                  Insert alternatives into the database
    \param  info            the original location information
    \param  alternatives    alternative country information
*/
void location_database::_insert_alternatives(const location_info& info, const map<string, alternative_country_info>& alternatives)
{ location_info info_copy = info;

  for (map<string, alternative_country_info>::const_iterator cit = alternatives.begin(); cit != alternatives.end(); ++cit)
  { info_copy.cq_zone(cit->second.cq_zone());
    info_copy.itu_zone(cit->second.itu_zone());
    _db.insert( { cit->first, info_copy } );
  }
}

/// default constructor
//location_database::location_database(void)
//{ }

// construct from CTY.DAT filename and the definition of which country list to use
location_database::location_database(const string& filename, const enum country_list_type country_list)
{ if (!filename.empty())
    _init(cty_data(filename), country_list);
}

/// construct from CTY.DAT data and the definition of which country list to use
location_database::location_database(const cty_data& cty, const enum country_list_type country_list)
{ _init(cty, country_list);
}

/// construct from CTY.DAT data, the definition of which country list to use and a secondary qth database
location_database::location_database(const cty_data& cty, const enum country_list_type country_list, const drlog_qth_database& secondary) :
  _qth_db(secondary)
{ _init(cty, country_list);
}

/// prepare a default-constructed object for use
void location_database::prepare(const cty_data& cty, const enum country_list_type country_list)
{ _init(cty, country_list);
}

/// prepare a default-constructed object for use
void location_database::prepare(const cty_data& cty, const enum country_list_type country_list, const drlog_qth_database& secondary)
{ _qth_db = secondary;
  _init(cty, country_list);
}

/*! \brief              Add Russian information
    \param  path        vector of directories to check for file <i>filename</i>
    \param  filename    name of file containing Russian information
*/
void location_database::add_russian_database(const vector<string>& path, const string& filename)
{ if (filename.empty())
    return;

  try
  { _russian_db = russian_data(path, filename).data();
  }

  catch (...)
  { ost << "Error attempting to add Russian database from file: " << filename << endl;
  }
}

/*! \brief      Get location information for a particular call or partial call
    \param  cs  call (or partial call)
    \return     location information corresponding to <i>call</i>
*/
const location_info location_database::info(const string& cs)
{ const string original_callsign = remove_peripheral_spaces(cs);
  string callsign = original_callsign;                  // make callsign mutable, for handling case of /n
  
  SAFELOCK(_location_database);

  auto db_posn = _db_checked.find(callsign);

// it's easy if there's already an entry
  if (db_posn != _db_checked.end())
    return db_posn->second;
  
// see if there's an exact match in the alternative call db
  db_posn = _alt_call_db.find(callsign);

  if (db_posn != _alt_call_db.end())
  { _db_checked.insert( { callsign, db_posn->second } );
    return db_posn->second;  
  }

// see if it's some guy already in the db but now signing /QRP
  if (callsign.length() >= 5 and last(callsign, 4) == "/QRP")
  { const string target = substring(callsign, 0, callsign.length() - 4);    // remove "/QRP"
  
    db_posn = _db_checked.find(target);
    
    if (db_posn != _db_checked.end())
    { const location_info rv = db_posn->second;
      
      _db_checked.insert( { callsign, rv } );
      return rv;
    }

// try the alternative call db
    db_posn = _alt_call_db.find(target);

    if (db_posn != _alt_call_db.end())
    { _db_checked.insert( { callsign, db_posn->second } );
      return db_posn->second;
    }
  }

// /MM and /AM are in no country
  if (last(callsign, 3) == "/AM" or last(callsign, 3) == "/MM")
  { location_info rv;
  
    _db_checked.insert( { callsign, rv } );
    return rv;
  }
  
// try to determine the canonical prefix
  if (!contains(callsign, "/") or (callsign.length() >= 2 and penultimate_char(callsign) == '/'))    // "easy" -- no portable indicator
  {
// country is determined by the longest substring starting at the start of the call and which is already
// in the database. This assumes that, for example, G4AMJ is in the same country as G4AM [if G4AM has already been worked]).
// I can think of counter-examples to do with the silly US call system, but at least this is a starting point.
    bool found_any_hits = false;              // no hits so far with this call
    string best_fit;
    location_info best_info;
    unsigned int len = 1;

    if ( (callsign.length() >= 2) and (penultimate_char(callsign) == '/') and isdigit(last_char(callsign)) )    // if /n; this changes callsign
    { static const string digits( { "0123456789" } );

      const size_t last_digit_posn = substring(callsign, 0, callsign.length() - 2).find_last_of(digits);

      if (last_digit_posn != string::npos)
      { callsign[last_digit_posn] = last_char(callsign);
        callsign = substring(callsign, 0, callsign.length() - 2);
      }
    }

    while (len <= callsign.length())
    { const string target = callsign.substr(0, len);
      map<string, location_info>::iterator db_posn = _db.find(target);
     
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

// Guantanamo Bay is a mess
    if (best_fit == "KG4" and (callsign.length() != 5) )
    { best_fit = "K";

//      const map<string, location_info>::iterator db_posn = _db.find("K");

//      best_info = db_posn->second;
      best_info = _db.find(best_fit)->second;
    }

    if (found_any_hits)                                 // return the best fit
    { // bool found_in_secondary_database = false;
         
// look to see if there's an exact match in the secondary database
#if 0
      if (_qth_db.size())
      { const vector<drlog_qth_database_record> subset = _qth_db.id(callsign);
           
        if (subset.size())
        { found_in_secondary_database = true;
          best_info.cq_zone(subset[0].get_cq_zone(best_info.cq_zone()));
          best_info.latitude(subset[0].get_latitude(best_info.latitude()));
          best_info.longitude(subset[0].get_longitude(best_info.longitude()));
        }
      }

// look to see if there's a country+area match in the secondary database
      if (_qth_db.size() and !found_in_secondary_database)
      { const vector<drlog_qth_database_record> subset = _qth_db.id(best_info.canonical_prefix());
           
        if (subset.size())
        { const size_t posn = callsign.find_last_of("0123456789");

          if (posn != string::npos)
          { const unsigned int target_area = from_string<unsigned int>(string(1, callsign[posn]));
               
            for (unsigned int area_nr = 0; area_nr < subset.size(); ++area_nr)
            { const drlog_qth_database_record& record = subset[area_nr];

              if (record.get_area(10) == target_area)
              { best_info.cq_zone(record.get_cq_zone(best_info.cq_zone()));
                best_info.latitude(record.get_latitude(best_info.latitude()));
                best_info.longitude(record.get_longitude(best_info.longitude()));
              }
            }
          }
        }
      }
#endif

      best_info = guess_zones(callsign, best_info);

// insert Russian information
      static const set<string> RUSSIAN_COUNTRIES { "UA", "UA2", "UA9" };

      if (RUSSIAN_COUNTRIES < best_info.canonical_prefix())
      { const size_t posn_1 = callsign.find_first_of("0123456789");

        if (posn_1 != string::npos)
        { const size_t posn_2 = callsign.find_first_not_of("0123456789", posn_1);

          if (posn_2 != string::npos)
          { const string sub = create_string(callsign[posn_1]) + create_string(callsign[posn_2]);
            const auto map_it = _russian_db.find(sub);

            if (map_it != _russian_db.end())
            { const russian_data_per_substring& data = map_it->second;

              best_info.cq_zone(data.cq_zone());
              best_info.itu_zone(data.itu_zone());
              best_info.latitude(data.latitude());
              best_info.longitude(data.longitude());
              best_info.region_name(data.region_name());
              best_info.region_abbreviation(data.region_abbreviation());
            }
          }
        }
      }

      _db_checked.insert( { original_callsign, best_info } );
       
      return best_info;
    }

// we get here if there were never any hits
    return location_info();
  }  // end no portable indicator

// it looks like maybe it's a reciprocal license
// how many slashes are there?
  vector<string> parts = split_string(callsign, "/");

  if (parts.size() == 1)        // slash is first or last character
    return location_info();

  if (parts.size() > 3)
     throw location_error(LOCATION_TOO_MANY_SLASHES, to_string(parts.size() - 1) + " slashes in call: " + callsign);

  if (parts.size() == 2)        // one slash
  {
// see if either part matches anything in the initial database
     map<string, location_info>::iterator db_posn_0 = _db.find(parts[0]);
     map<string, location_info>::iterator db_posn_1 = _db.find(parts[1]);
     
     const bool found_0 = (db_posn_0 != _db.end());
     const bool found_1 = (db_posn_1 != _db.end());

     if (found_0 and !found_1)                        // first part had an exact match
     { const location_info best_info { guess_zones(callsign, db_posn_0->second) };

        _db_checked.insert( { callsign, best_info } );
        
        return best_info;
     }

// we have to deal with stupid calls like K4/RU4W, where the second part is an entry in cty.dat
// add them on a case by case basis, rather than using all possible long prefixes listed in cty.dat, since this
// should be a very rare occurrence
     static const set<string> russian_long_prefixes { "RU4W" };

     if (found_1 and !found_0)                        // second part had an exact match
     { if (!(russian_long_prefixes < parts[1]))         // the normal case
       { //location_info best_info = db_posn_1->second;

          //best_info = guess_zones(callsign, best_info);

          const location_info best_info { guess_zones(callsign, db_posn_1->second) };

          _db_checked.insert( { callsign, best_info } );
        
          return best_info;
       }
       else                                             // the pathological case, a call like "K4/RU4W"
       { const location_info best_info = info(parts[0]);  // recursive

         _db_checked.insert( { callsign, best_info } );

         return best_info;
       }
     }

    if (found_0 and found_1)                      // both parts had an exact match (should never happen: KH6/KP2
    { if (parts[0].length() > parts[1].length())  // choose longest match
      {  //location_info best_info = db_posn_0->second;

         //best_info = guess_zones(callsign, best_info);

         const location_info best_info { guess_zones(callsign, db_posn_0->second) };

         _db_checked.insert( { callsign, best_info } );
        
         return best_info;
      }
      else
      { //location_info best_info = db_posn_1->second;

        //best_info = guess_zones(callsign, best_info);

        const location_info best_info { guess_zones(callsign, db_posn_1->second) };

        _db_checked.insert( { callsign, best_info } );
        
        return best_info;
      }
    }

    if (!found_0 and !found_1)    // neither matched exactly; use one that ends with a digit if there is one
    { const bool first_ends_with_digit = isdigit(parts[0][parts[0].length()-1]);
      const bool second_ends_with_digit = isdigit(parts[1][parts[1].length()-1]);
      
      if (first_ends_with_digit and !second_ends_with_digit)
        return info(parts[0]);

      if (second_ends_with_digit and !first_ends_with_digit)
        return info(parts[1]);
    }

    if (!found_0 and !found_1)    // neither matched exactly; use the one with the longest match
    {
// length of match for part 0
      unsigned int len_0 = 0;
      unsigned int len_1 = 0;
      unsigned int len = 1;
     
      while (len <= parts[0].length())
      { string target = parts[0].substr(0, len);
     
        map<string, location_info>::iterator db_posn = _db.find(target);
     
        if (db_posn != _db.end())
        { len_0 = len;
          db_posn_0 = db_posn;
        }

        ++len;
      }
    
      len = 1;
    
      while (len <= parts[1].length())
      { string target = parts[1].substr(0, len);
     
        map<string, location_info>::iterator db_posn = _db.find(target);
     
        if (db_posn != _db.end())
        { len_1 = len;
          db_posn_1 = db_posn;
        }

        ++len;
      }

      if (len_0 > len_1)                // parts[0] was the better match
      { //location_info best_info = db_posn_0->second;

        //best_info = guess_zones(callsign, best_info);

        const location_info best_info = guess_zones(callsign, db_posn_0->second);

        _db_checked.insert( { callsign, best_info } );
        
        return best_info;
      }

      if (len_1 > len_0)                // parts[1] was the better match
      {  //location_info best_info = db_posn_1->second;

         //best_info = guess_zones(callsign, best_info);

        const location_info best_info = guess_zones(callsign, db_posn_1->second);

        _db_checked.insert( { callsign, best_info } );
        
        return best_info;
      }
      
// they both match equally well; choose shortest
// neither matched at all
      if (len_0 == 0)
        return location_info();    // we know nothing about either part of the call
      
      if (parts[0].length() < parts[1].length())
      { location_info best_info = db_posn_0->second;
        
        best_info = guess_zones(callsign, best_info);
        _db_checked.insert( { callsign, best_info } );
        
        return best_info;
      }

      if (parts[1].length() < parts[0].length())
      { location_info best_info = db_posn_1->second;

        best_info = guess_zones(callsign, best_info);
        _db_checked.insert( { callsign, best_info } );
        
        return best_info;
      }
 
// same length; arbitrarily choose the first
      { location_info best_info = db_posn_0->second;

        best_info = guess_zones(callsign, best_info);
        _db_checked.insert( { callsign, best_info } );
        
        return best_info;
      }
    }
  }
  
  if (parts.size() == 3)        // two slashes
  {
// ignore the second slash and everything after it (assume W0/G4AMJ/P or G4AMJ/VP9/M)
    string target = parts[0] + "/" + parts[1];
    if (parts[2].length() != 1)                    // SP/DL/SP2SWA
      target = parts[0];
    
    const location_info best_info = info(target);   // recursive, so we need ref count in safelock
    
    _db_checked.erase(target);
    _db_checked.insert( { callsign, best_info } );
        
    return best_info;   
  }

  throw exception();
}

/// get a set of all the canonical prefixes for all countries
const set<string> location_database::countries(void)
{ SAFELOCK(_location_database);

  set<string> rv;

  FOR_ALL(_db, [&rv] (const pair<string, location_info>& prefix_li) { rv.insert((prefix_li.second).canonical_prefix()); } );  // there are a lot more entries in the db than there are countries

  return rv;
}

/// create a set of all the canonical prefixes for a particular continent
const set<string> location_database::countries(const string& cont_target)
{ const set<string> all_countries = countries();
  set <string> rv;

  copy_if(all_countries.cbegin(), all_countries.cend(), inserter(rv, rv.begin()), [=, &rv] (const string& cp) { return (continent(cp) == cont_target); } );

  return rv;
}

// -----------  drlog_qth_database_record  ----------------

/*! \class  drlog_qth_database_record
    \brief  A record from the drlog-specific QTH-override database
*/

// -----------  drlog_qth_database  ----------------

/*! \class  drlog_qth_database
    \brief  drlog-specific QTH-override database
*/

/// construct from filename
drlog_qth_database::drlog_qth_database(const std::string& filename)
{ if (filename.empty())
   return;

  const string contents = read_file(filename);
  const vector<string> lines = to_lines(contents);
  
//  for (unsigned int n = 0; n < lines.size(); ++n)
  for (const auto& line : lines)
  { //const string line = remove_peripheral_spaces(lines[n]);
    const vector<string> fields = split_string(line, ',');
    drlog_qth_database_record record;
    
//    for (unsigned int field_nr = 0; field_nr < fields.size(); ++field_nr)
    for (const auto& field : fields)
    { //const string& field = fields[field_nr];
    
      const vector<string> elements = remove_peripheral_spaces(split_string(remove_peripheral_spaces(field), '='));
//      for (unsigned int element_nr = 0; element_nr < elements.size(); ++element_nr)
//        elements[element_nr] = remove_peripheral_spaces(elements[element_nr]);
      
// process the possibilities
      if (elements[0] == "id")
        record.id(elements[1]);
      
      if (elements[0] == "area")
        record.set_area(from_string<unsigned int>(elements[1]));  // problem is that I'm setting a returned object

      if (elements[0] == "cq_zone")
        record.set_cq_zone(from_string<unsigned int>(elements[1]));

      if (elements[0] == "latitude")
        record.set_latitude(from_string<float>(elements[1]));

      if (elements[0] == "longitude")
        record.set_longitude(from_string<float>(elements[1]));     
    }
    
    _db.push_back(record);    
  }  
};

/// return all the entries with a particular ID
const vector<drlog_qth_database_record> drlog_qth_database::id(const string& id_target) const
{ vector<drlog_qth_database_record> rv;

  for (size_t n = 0; n < _db.size(); ++n)
    if (_db[n].id() == id_target)
      rv.push_back(_db[n]);
    
  return rv;
}

/*! \brief                      Get the CQ zone corresponding to a call
    \param  call                callsign
    \param  initial_cq_guess    default value of CQ zone, if none is found
    \return                     CQ zone corresponding to <i>call</i>
*/
const unsigned int drlog_qth_database::cq_zone(const string& call, const unsigned int initial_cq_zone) const
{ for (size_t n = 0; n < _db.size(); ++n)
    if (_db[n].id() == call)
      return _db[n].get_cq_zone(initial_cq_zone);

  return 0;
}

/*! \brief                      Get the CQ zone corresponding to a call area in a country
    \param  country             country identifier
    \param  call_area           call area (0 - 9)
    \param  initial_cq_zone     default value of CQ zone, if none is found
    \return                     CQ zone corresponding to call area <i>call_area</i> in country <i>country</i>
*/
const unsigned int drlog_qth_database::cq_zone(const string& country, const unsigned int area, const unsigned int initial_cq_zone) const
{ for (size_t n = 0; n < _db.size(); ++n)
    if (_db[n].id() == country and _db[n].get_area(10) == area)  // 10 is an invalid area
      return _db[n].get_cq_zone(initial_cq_zone);

  return 0;
}

/*! \brief                      Get the latitude corresponding to a call
    \param  call                callsign
    \param  initial_latitude    default value of latitude, if none is found
    \return                     latitude corresponding to <i>call</i>
*/
const float drlog_qth_database::latitude(const string& call, const float initial_latitude) const
{ for (size_t n = 0; n < _db.size(); ++n)
    if (_db[n].id() == call)
      return _db[n].get_latitude(initial_latitude);

  return 0;
}

/*! \brief                      Get the latitude corresponding to a call area in a country
    \param  country             country identifier
    \param  call_area           call area (0 - 9)
    \param  initial_cq_zone     default value of latitude, if none is found
    \return                     latitude corresponding to call area <i>call_area</i> in country <i>country</i>
*/
const float drlog_qth_database::latitude(const string& country, const unsigned int call_area, const float initial_latitude) const
{ for (size_t n = 0; n < _db.size(); ++n)
    if (_db[n].id() == country and _db[n].get_area(10) == call_area)  // 10 is an invalid area
      return _db[n].get_latitude(initial_latitude);

  return 0;
}

/*! \brief                      Get the longitude corresponding to a call
    \param  call                callsign
    \param  initial_longitude   default value of longitude, if none is found
    \return                     longitude corresponding to <i>call</i>
*/
const float drlog_qth_database::longitude(const string& call, const float initial_longitude) const
{ for (size_t n = 0; n < _db.size(); ++n)
    if (_db[n].id() == call)
      return _db[n].get_longitude(initial_longitude);

  return 0;
}

/*! \brief                      Get the longitude corresponding to a call area in a country
    \param  country             country identifier
    \param  call_area           call area (0 - 9)
    \param  initial_cq_zone     default value of longitude, if none is found
    \return                     longitude corresponding to call area <i>call_area</i> in country <i>country</i>
*/
const float drlog_qth_database::longitude(const string& country, const unsigned int call_area, const float initial_longitude) const
{ for (size_t n = 0; n < _db.size(); ++n)
    if (_db[n].id() == country and _db[n].get_area(10) == call_area)  // 10 is an invalid area
      return _db[n].get_longitude(initial_longitude);

  return 0;
}

// -----------  russian_data_per_substring  ----------------

/*! \class  russian_data_per_substring
    \brief  Encapsulate the data from a Russian data file, for a single substring
*/

/*! \brief              Construct from a prefix and a line
    \param  sbstring    the prefix for the Russian district
    \param  line        line from Russian data file
*/
russian_data_per_substring::russian_data_per_substring(const string& ss, const string& line) :
  _sstring(ss)
{
// check that the prefix matches the line

  const vector<string> substrings = remove_peripheral_spaces(split_string(delimited_substring(line, '[', ']'), ","));

  if (find(substrings.cbegin(), substrings.cend(), ss) == substrings.cend())
    throw russian_error(RUSSIAN_INVALID_SUBSTRING, (string("Substring ") + ss + " not found"));

  const size_t posn_1 = line.find(']');
  const size_t posn_2 = line.find(':');

  if (posn_1 == posn_2)
    throw russian_error(RUSSIAN_INVALID_FORMAT, string("Delimiter not found"));

  _region_name = ::substring(line, posn_1 + 1, posn_2 - posn_1 - 1);

  const vector<string> fields = remove_peripheral_spaces(split_string(remove_peripheral_spaces(squash(line.substr(posn_2 + 1), ' ')), " "));

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
  { throw russian_error(RUSSIAN_INVALID_FORMAT, string("Error parsing fields"));
  }
}

/// ostream << location_info
ostream& operator<<(ostream& ost, const russian_data_per_substring& rd)
{ ost << "substring: " << rd.sstring() << endl
      << "region name: " << rd.region_name() << endl
      << "region abbreviation: " << rd.region_abbreviation() << endl
      << "cq zone: " << rd.cq_zone() << endl
      << "itu zone: " << rd.itu_zone() << endl
      << "continent: " << rd.continent() << endl
      << "utc offset: " << rd.utc_offset() << endl
      << "latitude: " << rd.latitude() << endl
      << "longitude: " << rd.longitude();

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
russian_data::russian_data(const vector<string>& path, const string& filename)
{ try
  { const vector<string> lines = to_lines(replace_char(read_file(path, filename), '\t', ' '));

    for (const auto& line : lines)
    { if (!starts_with(line, "//"))
      { const vector<string> substrings = remove_peripheral_spaces(split_string(delimited_substring(line, '[', ']'), ","));

        for (const auto& sstring : substrings)
          _data.insert( { sstring, russian_data_per_substring(sstring, line) } );
      }
    }
  }

  catch (...)
  { ost << "Error processing Russian data file: " << filename << endl;
  }
}
