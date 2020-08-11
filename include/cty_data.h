<<<<<<< HEAD
// $Id: cty_data.h 163 2020-08-06 19:46:33Z  $
=======
// $Id: cty_data.h 154 2020-03-05 15:36:24Z  $
>>>>>>> 0a43fe059e6587fe915f47631dbfa4e529ab7fa9

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef CTY_DATA_H
#define CTY_DATA_H

/*! \file   cty_data.h

    Objects and functions related to CTY.DAT files
*/

#include "macros.h"
#include "pthread_support.h"
#include "serialization.h"
#include "string_functions.h"
#include "x_error.h"

#include <array>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/// country lists
enum class COUNTRY_LIST { DXCC,     ///< DXCC list
                          WAEDC     ///< DARC WAEDC list
                        };
                       
// error numbers
constexpr int CTY_INCORRECT_NUMBER_OF_FIELDS       { -1 },    ///< Wrong number of fields in a record
              CTY_INVALID_CQ_ZONE                  { -2 },    ///< Invalid CQ zone
              CTY_INVALID_ITU_ZONE                 { -3 },    ///< Invalid ITU zone
              CTY_INVALID_CONTINENT                { -4 },    ///< Invalid continent
              CTY_INVALID_LATITUDE                 { -5 },    ///< Invalid latitude
              CTY_INVALID_LONGITUDE                { -6 },    ///< Invalid longitude
              CTY_INVALID_UTC_OFFSET               { -7 },    ///< Invalid UTC offset
              CTY_INVALID_PREFIX                   { -8 };    ///< Invalid country prefix
          
constexpr int LOCATION_NO_PREFIX_MATCH             { -1 },    ///< unable to find a prefix match in the database
              LOCATION_TOO_MANY_SLASHES            { -2 };    ///< more than two slashes in the call

constexpr int RUSSIAN_INVALID_SUBSTRING            { -1 },    ///< source substring does not match target line in constructor
              RUSSIAN_INVALID_FORMAT               { -2 };    ///< format of file is invalid

#if 0
// -----------  value ----------------

/*! \class  value
    \brief  A single value that may or may not be valid
*/

template <typename T>
class value
{
protected:

  bool     _is_valid { false }; ///< is the value valid?
  T        _val;                ///< the encapsulated value
    
public:
  
/// default constructor
//  value(void) :
//    _is_valid(false)
//  { }
  value(void) = default;
  
/// construct from a value
  explicit value(T v) :
    _is_valid(true),
    _val(v)
  { }
  
/// return value if valid, otherwise a default
  T operator()(T def) const
    { return (_is_valid ? _val : def); }

/// return value if valid, otherwise a default
  T get(T def) const
    { return (_is_valid ? _val : def); }

/// return the value, regardless of whether it's valid
  T operator()(void) const
    { return _val; }
    
/// is the value valid?
  const bool is_valid(void) const
    { return _is_valid; }
    
/// set value
  void set(T v)
    { _is_valid = true;
      _val = v;
    }
    
/// value = *something*
  void operator=(T v)
  { _is_valid = true;
    _val = v;
  }

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _is_valid
         & _val;
    }
};
#endif

// -----------  alternative_country_info  ----------------

/*! \class  alternative_country_info
    \brief  A single alternative prefix or callsign for a country

    CTY files may contain "alias" information. This encapsulates that information.
*/

class alternative_country_info
{
protected:

  std::string  _country;                ///< canonical country prefix
  unsigned int _cq_zone;                ///< alternative CQ zone
  std::string  _identifier;             ///< the alternative prefix or callsign
  unsigned int _itu_zone;               ///< alternative ITU zone

public:

/*! \brief                      Construct from a string and a country ID
    \param  record              record from which to construct the alternative information
    \param  canonical_prefix    canonical country prefix

    <i>record</i> looks something like "G4AMJ(14)[28]", where the delimited information
    is optional
*/
  alternative_country_info(const std::string& record, const std::string& canonical_prefix = std::string());

/// destructor
  inline virtual ~alternative_country_info(void) = default;

  READ(country);               ///< canonical country prefix
  READ_AND_WRITE(cq_zone);     ///< alternative CQ zone
  READ(identifier);            ///< the alternative prefix or callsign
  READ_AND_WRITE(itu_zone);    ///< alternative ITU zone
};

/*! \brief          Write an <i>alternative_country_info</i> object to an output stream
    \param  ost     output stream
    \param  aci     object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const alternative_country_info& aci);

// -----------  cty_record  ----------------

/*! \class  cty_record
    \brief  A single record in the CTY.DAT file
  
  The official page describing the format is:
  http://www.country-files.com/cty/format.htm.
  
  Spacing in the file is "for readability only", so 
  we use the official delimiter ":" for fields, and
  ";" for records.
*/

class cty_record
{
protected:

  using ACI_DBTYPE = std::unordered_map<std::string, alternative_country_info>;

  ACI_DBTYPE   _alt_callsigns;         ///< alternative callsigns used by this country
  ACI_DBTYPE   _alt_prefixes;          ///< alternative prefixes used by this country
  std::string                                       _continent;             ///< two-letter abbreviation for continent
  std::string                                       _country_name;          ///< official name of the country
  unsigned int                                      _cq_zone;               ///< CQ zone
  unsigned int                                      _itu_zone;              ///< ITU zone
  float                                             _latitude;              ///< latitude in degrees (+ve north)
  float                                             _longitude;             ///< longitude in degrees (+ve west)
  std::string                                       _prefix;                ///< official DXCC prefix, in upper case
  int                                               _utc_offset;            ///< local-time offset from UTC, in minutes
  bool                                              _waedc_country_only;    ///< Is this only a country in the WAEDC (DARC) list?

public:
  
/*! \brief          Construct from a string
    \param  record  a record from the CTY file

    The string is assumed to contain a single record. We don't catch all
    possible errors, but we do test for the most obvious ones.
*/
  cty_record(const std::string& record);
  
/// destructor
  inline virtual ~cty_record(void) = default;

  READ(alt_callsigns);          ///< alternative callsigns used by this country
  READ(alt_prefixes);           ///< alternative prefixes used by this country
  READ(continent);              ///< two-letter abbreviation for continent
  READ(country_name);           ///< official name of the country
  READ(cq_zone);                ///< CQ zone
  READ(itu_zone);               ///< ITU zone
  READ(latitude);               ///< latitude in degrees (+ve north)
  READ(longitude);              ///< longitude in degrees (+ve west)
  READ(prefix);                 ///< official DXCC prefix
  READ(utc_offset);             ///< local-time offset from UTC, in minutes
  READ(waedc_country_only);     ///< Is this only a country in the WAEDC (DARC) list?

/// return the canonical prefix for this country; prefixes such as "GM/s" or "JD/o" are rendered in upper case
  inline const std::string canonical_prefix(void) const
    { return prefix(); }
    
/*! \brief          Remove an alternative callsign
    \param  call    alternative call to remove
        
    It is not an error to attempt to remove a call that does not exist
*/
  inline void remove_alternative_callsign(const std::string& call)
    { _alt_callsigns.erase(call); }

/*! \brief          Remove an alternative prefix
    \param  prefix  alternative prefix to remove
        
    It is not an error to attempt to remove a prefix that does not exist
*/
  inline void remove_alternative_prefix(const std::string& prefix)
    { _alt_callsigns.erase(prefix); }   

/*! \brief          Is a string an alternative callsign?
    \param  call    string to check
    \return         whether <i>call</i> is an alternative callsign
*/
  inline const bool is_alternative_callsign(const std::string& call)
    { return (_alt_callsigns.find(call) != _alt_callsigns.end()); }

/*! \brief  is a string an alternative prefix?
    \param  pfx    prefix to check
    \return        whether <i>pfx</i> is an alternative prefix
*/
  inline const bool is_alternative_prefix(const std::string& pfx)
    { return (_alt_prefixes.find(pfx) != _alt_prefixes.end()); }
    
  friend class location_database;           // in order to maintain type of ACI_DBTYPE across classes
};

/*! \brief          Write a <i>map<key, value></i> object to an output stream
    \param  ost     output stream
    \param  mp      object to write
    \return         the output stream
*/
//template <class T1, class T2>
//std::ostream& operator<<(std::ostream& ost, const std::map<T1, T2>& mp)
//{ for (typename std::map<T1, T2>::const_iterator cit = mp.begin(); cit != mp.end(); ++cit)
//    ost << "map[" << cit->first << "]: " << cit->second << std::endl;
//
//  return ost;
//}

/*! \brief          Write a <i>cty_record</i> object to an output stream
    \param  ost     output stream
    \param  rec     object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const cty_record& rec);

// -----------  cty_data  ----------------

/*! \class  cty_data
    \brief  All the data from a CTY.DAT file
*/

class cty_data
{
protected:
  std::vector<cty_record>  _data;        ///< the (raw) data

// all the alternative calls and prefixes (these are also maintained on a per-record basis)
  std::map<std::string, alternative_country_info> _alt_callsigns;    ///< key = alternative callsign
  std::map<std::string, alternative_country_info> _alt_prefixes;     ///< key = alternative prefix

public:

/*! \brief              Construct from a file
    \param  filename    name of file
*/
  cty_data(const std::string& filename = "cty.dat"s);   // somewhere along the way the default name changed from CTY.DAT

/*! \brief              Construct from a file
    \param  path        directories in which to search for <i>filename</i>, in order
    \param  filename    name of file
*/
  cty_data(const std::vector<std::string>& path, const std::string& filename = "cty.dat"s);   // somewhere along the way the default name changed from CTY.DAT

/// destructor
  inline virtual ~cty_data(void) = default;
    
/// how many countries are present?
  inline unsigned int n_countries(void) const
    { return _data.size(); }
  
/// return a record by number, wrt 0
  inline const cty_record operator[](const unsigned int n) const
    { return _data.at(n); }
};

// -----------  russian_data_per_substring  ----------------

/*! \class  russian_data_per_substring
    \brief  Encapsulate the data from a Russian data file, for a single district's substring
*/

class russian_data_per_substring
{
protected:
  std::string   _sstring;               ///< substring that matches this district

  std::string   _continent;             ///< two-letter abbreviation for continent
  unsigned int  _cq_zone;               ///< CQ zone
  unsigned int  _itu_zone;              ///< ITU zone
  float         _latitude;              ///< latitude in degrees (+ve north)
  float         _longitude;             ///< longitude in degrees (+ve east)
  std::string   _region_abbreviation;   ///< abbreviation of district (2 letters)
  std::string   _region_name;           ///< name of district
  int           _utc_offset;            ///< offset from UTC (minutes)

public:

/*! \brief              Construct from a prefix and a line
    \param  sbstring    the prefix for the Russian district
    \param  line        line from Russian data file
*/
  russian_data_per_substring(const std::string& sbstring, const std::string& line);

  READ(sstring);               ///< substring that matches this district

  READ(continent);             ///< two-letter abbreviation for continent
  READ(cq_zone);               ///< CQ zone
  READ(itu_zone);              ///< ITU zone
  READ(latitude);              ///< latitude in degrees (+ve north)
  READ(longitude);             ///< longitude in degrees (+ve east)
  READ(region_abbreviation);   ///< abbreviation of district (2 letters)
  READ(region_name);           ///< name of district
  READ(utc_offset);            ///< offset from UTC (minutes)

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _sstring
         & _continent
         & _cq_zone
         & _itu_zone
         & _latitude
         & _longitude
         & _region_abbreviation
         & _region_name
         & _utc_offset;
    }
};

/*! \brief          Write a <i>russian_data_per_substring</i> object to an output stream
    \param  ost     output stream
    \param  info    object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const russian_data_per_substring& info);

// -----------  location_info  ----------------

/*! \class  location_info
    \brief  Location information associated with a call, prefix or country
        
    This is basically just a simple 8-tuple
*/

class location_info
{
protected:

  std::string   _canonical_prefix { "NONE"s };      ///< official prefix
  std::string   _continent        { "XX"s };        ///< two-letter abbreviation for continent
  std::string   _country_name     { "None"s };      ///< official name of the country
  unsigned int  _cq_zone          { 0 };            ///< CQ zone
  unsigned int  _itu_zone         { 0 };            ///< ITU zone
  float         _latitude         { 0 };            ///< latitude in degrees (+ve north)
  float         _longitude        { 0 };            ///< longitude in degrees (+ve west)
  int           _utc_offset       { 0 };            ///< local-time offset from UTC, in minutes
  
// used only by Russian stations
  std::string  _region_name;            ///< name of (Russian) region in which station resides
  std::string  _region_abbreviation;    ///< abbreviation for (Russian) region in which the station resides

public:

/// default constructor
  location_info(void) = default;
  
/// construct from a record from a CTY file
  inline explicit location_info(const cty_record& rec) :
    _canonical_prefix(rec.prefix()),
    _continent(rec.continent()),
    _country_name(rec.country_name()),
    _cq_zone(rec.cq_zone()),
    _itu_zone(rec.itu_zone()),
    _latitude(rec.latitude()),
    _longitude(rec.longitude()),
    _utc_offset(rec.utc_offset())
  { }

/// destructor
  inline virtual ~location_info(void) = default;

/// location_info == location_info
  const bool operator==(const location_info& li) const;

  READ(canonical_prefix);      ///< official prefix
  READ(continent);             ///< two-letter abbreviation for continent
  READ(country_name);          ///< official name of the country
  READ_AND_WRITE(cq_zone);     ///< CQ zone
  READ_AND_WRITE(itu_zone);    ///< ITU zone
  READ_AND_WRITE(latitude);    ///< latitude in degrees (+ve north)
  READ_AND_WRITE(longitude);   ///< longitude in degrees (+ve west)
  READ(utc_offset);            ///< local-time offset from UTC, in minutes

  READ_AND_WRITE(region_abbreviation);   ///< (Russian) two-letter abbreviation for region
  READ_AND_WRITE(region_name);           ///< (Russian) name of region
  
/*! \brief          Set both latitude and longitude at once
    \param  lat     latitude in degrees (+ve north)
    \param  lon     longitude in degrees (+ve west)
*/
  void latitude_longitude(const float lat, const float lon);    // set both latitude and longitude at once

/// archive using boost
   template<typename Archive>
   void serialize(Archive& ar, const unsigned version)
     { ar & _canonical_prefix
          & _continent
          & _country_name
          & _cq_zone
          & _itu_zone
          & _latitude
          & _longitude
          & _utc_offset
          & _region_abbreviation
          & _region_name;
     }
};

/*! \brief          Write a <i>location_info</i> object to an output stream
    \param  ost     output stream
    \param  info    object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const location_info& info);

/*! \brief          Guess the CQ and ITU zones if the canonical prefix indicates a country with multiple zones
    \param  call    callsign
    \param  li      location_info to use for the guess
    \return         best guess for the CQ and ITU zones

    Currently this supports just VE, VK and W for CQ zones, and VE for ITU zones
 */
const location_info guess_zones(const std::string& call, const location_info& li);

#if 0
// -----------  drlog_qth_database_record  ----------------

/*! \class  drlog_qth_database_record
    \brief  A record from the drlog-specific QTH-override database

    I believe that this is currently unused.
*/

class drlog_qth_database_record
{
protected:

  std::string         _id;          ///< ID for the record (typically, a callsign)
  value<unsigned int> _area;        ///< call area
  value<unsigned int> _cq_zone;     ///< CQ zone
  value<float>        _latitude;    ///< latitude in degrees (+ve north)
  value<float>        _longitude;   ///< longitude in degrees (+ve west)
  
public:
  
  READ_AND_WRITE(id);               ///< ID for the record (typically, a callsign)
    
/*! \brief      Get the call area
    \return     the call area
*/
  inline const unsigned int get_area(const unsigned int def) const
    { return _area.get(def); }
    
/*! \brief          Set the call area
    \param  val     new value of call area
*/
  inline void set_area(const unsigned int val)
    { _area.set(val); }

/*! \brief      Get the CQ zone
    \return     the CQ zone
*/
  inline const unsigned int get_cq_zone(const unsigned int def) const
    { return _cq_zone.get(def); }
    
/*! \brief          Set the CQ zone
    \param  val     new value of CQ zone
*/
  inline void set_cq_zone(const unsigned int val)
    { _cq_zone.set(val); }    

/*! \brief      Get the latitude
    \return     the latitude
*/
  inline const float get_latitude(const float def) const
    { return _latitude.get(def); }
    
/*! \brief          Set the latitude
    \param  val     new value of latitude
*/
  inline void set_latitude(const float val)
    { _latitude.set(val); }

/*! \brief      Get the longitude
    \return     the longitude
*/
  inline const float get_longitude(const float def) const
    { return _longitude.get(def); }

/*! \brief          Set the longitude
    \param  val     new value of longitude
*/
  inline void set_longitude(const float val)
    { _longitude.set(val); }    

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _id
         & _area
         & _cq_zone
         & _latitude
         & _longitude;
    }
};
#endif

#if 0
// -----------  drlog_qth_database  ----------------

/*! \class  drlog_qth_database
    \brief  drlog-specific QTH-override database
*/

class drlog_qth_database
{
protected:
  
  std::vector<drlog_qth_database_record> _db;    ///< the database
    
public:
  
/// default constructor
  drlog_qth_database(void) = default;

/// construct from filename
  drlog_qth_database(const std::string& filename);
  
/// return all the entries with a particular ID
  const std::vector<drlog_qth_database_record> id(const std::string& id_target) const;

/*! \brief                      Get the CQ zone corresponding to a call
    \param  call                callsign
    \param  initial_cq_zone     default value of CQ zone, if none is found
    \return                     CQ zone corresponding to <i>call</i>
*/
  const unsigned int cq_zone(const std::string& call, const unsigned int initial_cq_zone) const;

/*! \brief                      Get the CQ zone corresponding to a call area in a country
    \param  country             country identifier
    \param  call_area           call area (0 - 9)
    \param  initial_cq_zone     default value of CQ zone, if none is found
    \return                     CQ zone corresponding to call area <i>call_area</i> in country <i>country</i>
*/
  const unsigned int cq_zone(const std::string& country, const unsigned int call_area, const unsigned int initial_cq_zone) const;

/*! \brief                      Get the latitude corresponding to a call
    \param  call                callsign
    \param  initial_latitude    default value of latitude, if none is found
    \return                     latitude corresponding to <i>call</i>
*/
  const float latitude(const std::string& call, const float initial_latitude) const;

/*! \brief                      Get the latitude corresponding to a call area in a country
    \param  country             country identifier
    \param  call_area           call area (0 - 9)
    \param  initial_latitude    default value of latitude, if none is found
    \return                     latitude corresponding to call area <i>call_area</i> in country <i>country</i>
*/
  const float latitude(const std::string& country, const unsigned int call_area, const float initial_latitude) const;
 
/*! \brief                      Get the longitude corresponding to a call
    \param  call                callsign
    \param  initial_longitude   default value of longitude, if none is found
    \return                     longitude in degrees corresponding to <i>call</i> (+ve west)
*/
  const float longitude(const std::string& call, const float initial_longitude) const;

/*! \brief                      Get the longitude corresponding to a call area in a country
    \param  country             country identifier
    \param  call_area           call area (0 - 9)
    \param  initial_longitude   default value of longitude, if none is found
    \return                     longitude corresponding to call area <i>call_area</i> in country <i>country</i>
*/
  const float longitude(const std::string& country, const unsigned int call_area, const float initial_longitude) const;
  
/// number of records in the database
  inline const size_t size(void) const
    { return _db.size(); }

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _db;
    }
};
#endif

// -----------  location_database  ----------------

/*! \class  location_database
    \brief  The country-based location information packaged for use by drlog
*/

class location_database
{
protected:

  using ACI_DBTYPE = decltype(cty_record::_alt_callsigns);
  using LOCATION_DBTYPE = std::unordered_map<std::string, location_info>;
  using RUSSIAN_DBTYPE  = std::unordered_map<std::string, russian_data_per_substring>;  // there doesn't seem to be any way to make this accessible to russian_data; so it is redefined in that class

//  std::map<std::string, location_info> _db;          ///< prefix-associated info -- the original database
  LOCATION_DBTYPE _db;          ///< prefix-associated info -- the original database
  
//  std::map<std::string, location_info> _alt_call_db; ///< database of alternative calls
  LOCATION_DBTYPE _alt_call_db; ///< database of alternative calls

//  mutable std::map<std::string, location_info> _db_checked;  ///< call- or prefix-associated info -- a cache of all previously checked calls
  mutable LOCATION_DBTYPE _db_checked;  ///< call- or prefix-associated info -- a cache of all previously checked calls

  RUSSIAN_DBTYPE _russian_db;  ///< Russian substring-indexed info
  
//  drlog_qth_database                   _qth_db;      ///< additional database

/*! \brief                  Initialise the database
    \param  cty             cty.dat data
    \param  country_list    type of country list
*/
  void _init(const cty_data& cty, const COUNTRY_LIST country_list);

/*! \brief                  Insert alternatives into the database
    \param  info            the original location information
    \param  alternatives    alternative country information
*/
//  void _insert_alternatives(const location_info& info, const std::map<std::string, alternative_country_info>& alternatives);
  void _insert_alternatives(const location_info& info, const ACI_DBTYPE& alternatives);

  mutable pt_mutex _location_database_mutex;  ///< to make location_database objects thread-safe;
   
public:

/// default constructor
  location_database(void) = default;

/*! \brief                  Constructor
    \param  filename        name of cty.dat file
    \param  country_list    type of country list
*/
  location_database(const std::string& filename, const COUNTRY_LIST country_list = COUNTRY_LIST::DXCC);

/*! \brief                  Constructor
    \param  cty             cty.dat data
    \param  country_list    type of country list
*/
  location_database(const cty_data& cty, const COUNTRY_LIST country_list = COUNTRY_LIST::DXCC);

/*! \brief                  Constructor
    \param  cty             cty.dat data
    \param  country_list    type of country list
    \param  secondary       secondary QTH database
*/
//  location_database(const cty_data& cty, const COUNTRY_LIST country_list, const drlog_qth_database& secondary);

/*! \brief                  Prepare a default-constructed object for use
    \param  cty             cty.dat data
    \param  country_list    type of country list
*/
  void prepare(const cty_data& cty, const COUNTRY_LIST country_list = COUNTRY_LIST::DXCC);

/*! \brief                  Prepare a default-constructed object for use
    \param  cty             cty.dat data
    \param  country_list    type of country list
    \param  secondary       secondary QTH database
*/
//  void prepare(const cty_data& cty, const COUNTRY_LIST country_list, const drlog_qth_database& secondary);

/*! \brief              Add Russian information
    \param  path        vector of directories to check for file <i>filename</i>
    \param  filename    name of file containing Russian information
*/
  void add_russian_database(const std::vector<std::string>& path, const std::string& filename);

/// how large is the main database?
  inline const size_t size(void) const
    { return (SAFELOCK_GET( _location_database_mutex, _db.size() )); }

/*! \brief          Add a call to the alt_call database
    \param  call    callsign to add
    \param  li      location information for <i>call</i>

    Overwrites any extant entry with <i>call</i> as the key
*/
  inline void add_alt_call(const std::string& call, const location_info& li)
    { _alt_call_db.insert( { call, li } ); }

/*! \brief              Get location information for a particular call or partial call
    \param  callpart    call (or partial call)
    \return             location information corresponding to <i>call</i>
*/
  const location_info info(const std::string& callpart) const;
  
/// return the database
//  inline const std::map<std::string, location_info> db(void) const
  inline const decltype(location_database::_db) db(void) const
    { return (SAFELOCK_GET( _location_database_mutex, _db )); }

/// create a set of all the canonical prefixes for countries
//  const std::unordered_set<std::string> countries(void) const;
  auto countries(void) const -> std::unordered_set<std::string>;

/// create a set of all the canonical prefixes for a particular continent
  const std::unordered_set<std::string> countries(const std::string& cont_target) const;
  
/*! \brief              Get official name of the country associated with a call or partial call
    \param  callpart    call (or partial call)
    \return             official name of the country corresponding to <i>callpart</i>
*/
  inline const std::string country_name(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).country_name() )); }

/*! \brief              Get CQ zone associated with a call or partial call
    \param  callpart    call (or partial call)
    \return             CQ zone corresponding to <i>callpart</i>
*/
  inline const unsigned int cq_zone(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).cq_zone() )); }

/*! \brief              Get ITU zone associated with a call or partial call
    \param  callpart    call (or partial call)
    \return             ITU zone corresponding to <i>callpart</i>
*/
  inline const unsigned int itu_zone(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).itu_zone() )); }

/*! \brief              Get the continent associated with a call or partial call
    \param  callpart    call (or partial call)
    \return             continent to <i>callpart</i>

    The returned continent is in the form of the two-letter code
*/
  inline const std::string continent(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).continent() )); }

/*! \brief              Get the latitude for a call or partial call
    \param  callpart    call (or partial call)
    \return             latitude (in degrees) corresponding to <i>callpart</i> (+ve north)
*/
  inline const float latitude(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).latitude() )); }

/*! \brief              Get the longitude for a call or partial call
    \param  callpart    call (or partial call)
    \return             longitude (in degrees) corresponding to <i>callpart</i> (+ve west)
*/
  inline const float longitude(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).longitude() )); }

/*! \brief              Get the UTC offset for a call or partial call
    \param  callpart    call (or partial call)
    \return             UTC offset (in minutes) corresponding to <i>callpart</i>
*/
  inline const int utc_offset(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).utc_offset() )); }

/*! \brief              Get the canonical prefix for a call or partial call
    \param  callpart    call (or partial call)
    \return             canonical prefix corresponding to <i>callpart</i>
*/
  inline const std::string canonical_prefix(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).canonical_prefix() )); }

/*! \brief              Get name of the Russian district for a particular call or partial call
    \param  callpart    call (or partial call)
    \return             name of the Russian district corresponding to <i>callpart</i>

    Returns the empty string if <i>callpart</i> is not Russian
*/
  inline const std::string region_name(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).region_name() )); }

/*! \brief              Get two-letter abbreviation for the Russian district for a particular call or partial call
    \param  callpart    call (or partial call)
    \return             two-letter Russian region code corresponding to <i>callpart</i>

    Returns the empty string if <i>callpart</i> is not Russian
*/
  inline const std::string region_abbreviation(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).region_abbreviation() )); }

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(_location_database);

      ar & _db
         & _alt_call_db
         & _db_checked;
//         & _qth_db;
    }
    
  friend class russian_data;    // in order to keep consistent definitions of database types
};

/*! \brief          Write a <i>location_database</i> object to an output stream
    \param  ost     output stream
    \param  db      object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const location_database& db);

// -----------  russian_data  ----------------

/*! \class  russian_data
    \brief  Encapsulate the data from a Russian data file

    Russian data file is based on http://www.rdxc.org/asp/pages/regions.asp?ORDER=1
*/

class russian_data
{
protected:

  using RUSSIAN_DBTYPE  = decltype(location_database::_russian_db);

 // std::map<std::string /* substring */, russian_data_per_substring> _data;          ///< map substring to the matching data
  RUSSIAN_DBTYPE _data;          ///< map substring to the matching data

public:

/*! \brief              Construct from a file
    \param  path        the directory path to be searched in order
    \param  filename    the name of the file to be read
*/
  russian_data(const std::vector<std::string>& path, const std::string& filename);

  READ(data);  ///< map substring to the matching data
};

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(cty_error);         ///< Errors related to CTY processing

ERROR_CLASS(location_error);    ///< Errors related to location database processing

ERROR_CLASS(russian_error);     ///< Errors related to processing Russian data file

#endif  // CTY_DATA_H
