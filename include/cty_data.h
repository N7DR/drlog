// $Id: cty_data.h 84 2014-11-15 19:20:13Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef CTY_DATA_H
#define CTY_DATA_H

/*!     \file cty_data.h

        Objects and functions related to CTY.DAT files
*/

#include "macros.h"
#include "pthread_support.h"
#include "serialization.h"
#include "x_error.h"

#include <array>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

enum country_list_type { COUNTRY_LIST_DXCC,     ///< DXCC list
                         COUNTRY_LIST_WAEDC     ///< DARC WAEDC list
                       };
                       
// error numbers
const int CTY_INCORRECT_NUMBER_OF_FIELDS       = -1,    ///< Wrong number of fields in a record
          CTY_INVALID_CQ_ZONE                  = -2,    ///< Invalid CQ zone
          CTY_INVALID_ITU_ZONE                 = -3,    ///< Invalid ITU zone
          CTY_INVALID_CONTINENT                = -4,    ///< Invalid continent
          CTY_INVALID_LATITUDE                 = -5,    ///< Invalid latitude
          CTY_INVALID_LONGITUDE                = -6,    ///< Invalid longitude
          CTY_INVALID_UTC_OFFSET               = -7,    ///< Invalid UTC offset
          CTY_INVALID_PREFIX                   = -8;    ///< Invalid country prefix
          
const int LOCATION_NO_PREFIX_MATCH             = -1,    ///< unable to find a prefix match in the database
          LOCATION_TOO_MANY_SLASHES            = -2;    ///< more than two slashes in the call

const int RUSSIAN_INVALID_SUBSTRING            = -1,    ///< source substring does not match target line in constructor
          RUSSIAN_INVALID_FORMAT               = -2;    ///< format of file is invalid

// -----------  value ----------------

/*!     \class value
        \brief A single value that may or may not be valid
*/

template <typename T>
class value
{
protected:
  bool     _is_valid;    ///< is the value valid?
  T        _val;         ///< the encapsulated value
    
public:
  
/// default constructor
  value(void) :
    _is_valid(false)
  { }
  
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
    
/// value = <something>
  void operator=(T v)
  { _is_valid = true;
    _val = v;
  }

/// serialize and unserialize using boost
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _is_valid
         & _val;
    }
};

// -----------  alternative_country_info  ----------------

/*! \class alternative_country_info
    \brief A single alternative prefix or callsign for a country

    CTY files may contain "alias" information. This encapsulates that information.
*/

class alternative_country_info
{
protected:
  std::string  _identifier;             ///< the alternative prefix or callsign
  unsigned int _cq_zone;                ///< alternative CQ zone
  unsigned int _itu_zone;               ///< alternative ITU zone
  std::string  _country;                ///< canonical country prefix

public:

/*!     \brief  construct from a string and a country ID
        \param  record from which to contruct the alternative information
        \param  id     canonical country ID

        <i>record</i> looks something like "G4AMJ(14)[28]", where the delimited information
        is optional
*/
  alternative_country_info(const std::string& record, const std::string& id = "");

/// destructor
  inline virtual ~alternative_country_info(void)
    { }

  READ(identifier);            ///< the alternative prefix or callsign
  READ_AND_WRITE(cq_zone);     ///< alternative CQ zone
  READ_AND_WRITE(itu_zone);    ///< alternative ITU zone
  READ(country);               ///< canonical country prefix
};

/// ostream << alternative_country_info
std::ostream& operator<<(std::ostream& ost, const alternative_country_info& aci);

// -----------  cty_record  ----------------

/*! \class cty_record
    \brief A single record in the CTY.DAT file
  
  The official page describing the format is:
  http://www.country-files.com/cty/format.htm.
  
  Spacing in the file is "for readability only", so 
  we use the official delimiter ":" for fields, and
  ";" for records.
*/

class cty_record
{
protected:
  std::string   _country_name;          ///< official name of the country
  unsigned int  _cq_zone;               ///< CQ zone
  unsigned int  _itu_zone;              ///< ITU zone
  std::string   _continent;             ///< two-letter abbreviation for continent
  float         _latitude;              ///< latitude in degrees (+ve north)
  float         _longitude;             ///< longitude in degrees (+ve west)
  int           _utc_offset;            ///< local-time offset from UTC, in minutes
  std::string   _prefix;                ///< official DXCC prefix, in upper case

  bool          _waedc_country_only;    ///< Is this only a country in the WAEDC (DARC) list?

  std::map<std::string, alternative_country_info> _alt_callsigns;  ///< alternative callsigns used by this country
  std::map<std::string, alternative_country_info> _alt_prefixes;   ///< alternative prefixes used by this country
   
public:
  
/*! \brief  construct from a string
    \param  record  a record from the CTY file

    The string is assumed to contain a single record. We don't catch all
    possible errors, but we do test for the most obvious ones.
*/
  cty_record(const std::string& record);
  
/// destructor
  inline virtual ~cty_record(void)
    { }

/// RO access
  READ(waedc_country_only);     ///< Is this only a country in the WAEDC (DARC) list?
  READ(country_name);           ///< official name of the country
  READ(cq_zone);                ///< CQ zone
  READ(itu_zone);               ///< ITU zone
  READ(continent);              ///< two-letter abbreviation for continent
  READ(latitude);               ///< latitude in degrees (+ve north)
  READ(longitude);              ///< longitude in degrees (+ve west)
  READ(utc_offset);             ///< local-time offset from UTC, in minutes
  READ(prefix);                 ///< official DXCC prefix
  READ(alt_callsigns);          ///< alternative callsigns used by this country
  READ(alt_prefixes);           ///< alternative prefixes used by this country

/// return the canonical prefix for this country; prefixes such as "GM/s" or "JD/o" are rendered in upper case
  inline const std::string canonical_prefix(void) const
    { return prefix(); }
    
/*!     \brief  remove an alternative callsign
        \param  call    call to remove
        
        It is not an error to attempt to remove a call that does not exist
*/
  inline void remove_alternative_callsign(const std::string& call)
    { _alt_callsigns.erase(call); }

/*!     \brief  remove an alternative prefix
        \param  prefix    prefix to remove
        
        It is not an error to attempt to remove a prefix that does not exist
*/
  inline void remove_alternative_prefix(const std::string& prefix)
    { _alt_callsigns.erase(prefix); }   

/*!     \brief  is a string an alternative callsign?
        \param  call    call to check
*/
  inline const bool is_alternative_callsign(const std::string& call)
    { return (_alt_callsigns.find(call) != _alt_callsigns.end()); }

/*!     \brief  is a string an alternative prefix?
        \param  pfx    prefix to check
*/
  inline const bool is_alternative_prefix(const std::string& pfx)
    { return (_alt_prefixes.find(pfx) != _alt_prefixes.end()); }
};

/// ostream << map<key, value>
template <class T1, class T2>
std::ostream& operator<<(std::ostream& ost, const std::map<T1, T2>& mp)
{ for (typename std::map<T1, T2>::const_iterator cit = mp.begin(); cit != mp.end(); ++cit)
    ost << "map[" << cit->first << "]: " << cit->second << std::endl;
  
  return ost;
}

/// ostream << cty_record
std::ostream& operator<<(std::ostream& ost, const cty_record& rec);

// -----------  cty_data  ----------------

/*! \class cty_data
    \brief All the data from a CTY.DAT file
*/

class cty_data
{
protected:
  std::vector<cty_record>  _data;        ///< the (raw) data

// all the alternative calls and prefixes (these are also maintained on a per-record basis)
  std::map<std::string, alternative_country_info> _alt_callsigns;    ///< key = alternative callsign
  std::map<std::string, alternative_country_info> _alt_prefixes;     ///< key = alternative prefix

public:

/// construct from a file
  cty_data(const std::string& filename = "cty.dat");   // somewhere along the way the default name changed from CTY.DAT

/*! \brief  construct from a file
    \param  path        The directory path to be searched in order
    \param  filename    The name of the file to be read
*/
  cty_data(const std::vector<std::string>& path, const std::string& filename = "cty.dat");   // somewhere along the way the default name changed from CTY.DA

/// destructor
  inline virtual ~cty_data(void)
    { }
    
/// how many countries are present?
  inline unsigned int n_countries(void) const
    { return _data.size(); }
  
/// return a record by number, wrt 0
  inline const cty_record operator[](const unsigned int n) const
    { return _data.at(n); }
};

// -----------  russian_data_per_substring  ----------------

/*! \class russian_data_per_substring
    \brief Encapsulate the data from a Russian data file, for a single substring
*/

class russian_data_per_substring
{
protected:
  std::string _sstring;               ///< substring that matches this district
  std::string _region_name;           ///< name of district
  std::string _region_abbreviation;   ///< abbreviation of district (2 letters)
  unsigned int _cq_zone;              ///< CQ zone
  unsigned int _itu_zone;             ///< ITU zone
  std::string _continent;             ///< two-letter abbreviation for continent
  int _utc_offset;                    ///< offset from UTC
  float _latitude;                    ///< latitude in degrees (+ve north)
  float _longitude;                   ///< longitude in degrees (+ve east)

public:

/*! \brief  construct from a prefix and a line
    \param  sbstring        The prefix for the Russian district
    \param  line            Line from Russian data file
*/
  russian_data_per_substring(const std::string& sbstring, const std::string& line);

  READ(sstring);               ///< substring that matches this district
  READ(region_name);           ///< name of district
  READ(region_abbreviation);   ///< abbreviation of district (2 letters)
  READ(cq_zone);               ///< CQ zone
  READ(itu_zone);              ///< ITU zone
  READ(continent);             ///< two-letter abbreviation for continent
  READ(utc_offset);            ///< offset from UTC
  READ(latitude);              ///< latitude in degrees (+ve north)
  READ(longitude);             ///< longitude in degrees (+ve east)

/// archive using boost
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _sstring
         & _region_name
         & _region_abbreviation
         & _cq_zone
         & _itu_zone
         & _continent
         & _utc_offset
         & _latitude
         & _longitude;
    }
};

/// ostream << location_info
std::ostream& operator<<(std::ostream& ost, const russian_data_per_substring& info);

// -----------  russian_data  ----------------

/*! \class russian_data
    \brief Encapsulate the data from a Russian data file

    Russian data file is based on http://www.rdxc.org/asp/pages/regions.asp?ORDER=1
*/

class russian_data
{
protected:
  std::map<std::string /* substring */, russian_data_per_substring> _data;  ///< map substring to the matching data

public:

/*! \brief  construct from a file
    \param  path        The directory path to be searched in order
    \param  filename    The name of the file to be read
*/
  russian_data(const std::vector<std::string>& path, const std::string& filename);

  READ(data);  ///< map substring to the matching data
};

// -----------  location_info  ----------------

/*!     \class location_info
        \brief Location information associated with a call, prefix or country
        
        This is basically just a simple 8-tuple
*/

class location_info
{
protected:

  std::string   _country_name;          ///< official name of the country
  unsigned int  _cq_zone;               ///< CQ zone
  unsigned int  _itu_zone;              ///< ITU zone
  std::string   _continent;             ///< two-letter abbreviation for continent
  float         _latitude;              ///< latitude in degrees (+ve north)
  float         _longitude;             ///< longitude in degrees (+ve west)
  int           _utc_offset;            ///< local-time offset from UTC, in minutes
  std::string   _canonical_prefix;      ///< official prefix
  
// used only by Russian stations
  std::string  _region_name;            ///< name of region in which station resides
  std::string  _region_abbreviation;    ///< abbreviation for region in which the station resides

public:

/// default constructor
  location_info(void);
  
/// construct from a record from a CTY file
  explicit location_info(const cty_record& rec);

/// destructor
  virtual ~location_info(void);

/// location_info == location_info
  const bool operator==(const location_info& li) const;

  READ(country_name);          ///< official name of the country
  READ_AND_WRITE(cq_zone);     ///< CQ zone
  READ_AND_WRITE(itu_zone);    ///< ITU zone
  READ(continent);             ///< two-letter abbreviation for continent
  READ_AND_WRITE(latitude);    ///< latitude in degrees (+ve north)
  READ_AND_WRITE(longitude);   ///< longitude in degrees (+ve west)
  READ(utc_offset);            ///< local-time offset from UTC, in minutes
  READ(canonical_prefix);      ///< official prefix

  READ_AND_WRITE(region_name);           ///< (Russian) name of region
  READ_AND_WRITE(region_abbreviation);   ///< (Russian) abbreviation for region

/// archive using boost
   template<typename Archive>
   void serialize(Archive& ar, const unsigned version)
     { ar & _country_name
          & _cq_zone
          & _itu_zone
          & _continent
          & _latitude
          & _longitude
          & _utc_offset
          & _canonical_prefix
          & _region_name
          & _region_abbreviation;
     }
};

/// ostream << location_info
std::ostream& operator<<(std::ostream& ost, const location_info& info);

/*!     \brief guess the CQ and ITU zones if the canonical prefix indicates a country with multiple zones
        \param  call    callsign
        \param  li      location_info to use for the guess

        Currently this supports just VE, VK and W for CQ zones, and VE for ITU zones
 */
const location_info guess_zones(const std::string& call, const location_info& li);

// -----------  drlog_qth_database_record  ----------------

/*!     \class drlog_qth_database_record
        \brief A record from the drlog-specific QTH-override database

        I believe that this is currently unused.
*/

class drlog_qth_database_record
{
protected:
  std::string         _id;          ///< ID for the record (typically, a callsign)
  value<unsigned int> _area;        ///< not sure what this is
  value<unsigned int> _cq_zone;     ///< CQ zone
  value<float>        _latitude;    ///< latitude in degrees (+ve north)
  value<float>        _longitude;   ///< longitude in degrees (+ve west)
  
public:
  
  inline const std::string get_id(void) const
    { return _id; }
    
  inline void set_id(const std::string& i)
    { _id = i; }
    
  inline const unsigned int get_area(const unsigned int def) const
    { return _area.get(def); }
    
  inline void set_area(const unsigned int val)
    { _area.set(val); }
    
  inline const unsigned int get_cq_zone(const unsigned int def) const
    { return _cq_zone.get(def); }
    
  inline void set_cq_zone(const unsigned int val)
    { _cq_zone.set(val); }    

  inline const float get_latitude(const float def) const
    { return _latitude.get(def); }
    
  inline void set_latitude(const float val)
    { _latitude.set(val); }
    
  inline const float get_longitude(const float def) const
    { return _longitude.get(def); }
    
  inline void set_longitude(const float val)
    { _longitude.set(val); }    

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _id
         & _area
         & _cq_zone
         & _latitude
         & _longitude;
    }
};

// -----------  drlog_qth_database  ----------------

/*!     \class drlog_qth_database
        \brief drlog-specific QTH-override database
*/

class drlog_qth_database
{
protected:
  
  std::vector<drlog_qth_database_record> _db;    ///< the database
    
public:
  
/// default constructor
  drlog_qth_database(void)
    { }

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
    \param  initial_cq_zone     default value of latitude, if none is found
    \return                     latitude corresponding to call area <i>call_area</i> in country <i>country</i>
*/
  const float latitude(const std::string& country, const unsigned int area, const float initial_latitude) const;
 
/*! \brief                      Get the longitude corresponding to a call
    \param  call                callsign
    \param  initial_longitude   default value of longitude, if none is found
    \return                     longitude corresponding to <i>call</i>
*/
  const float longitude(const std::string& call, const float initial_longitude) const;

  const float longitude(const std::string& country, const unsigned int area, const float initial_longitude) const;
  
  inline const size_t size(void) const
    { return _db.size(); }
  
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _db;
    }
};

// -----------  location_database  ----------------

/*!     \class location_database
        \brief The country-based location information packaged for use by drlog
*/

class location_database
{
protected:
  std::map<std::string, location_info> _db;          ///< prefix-associated info -- the original database
  std::map<std::string, location_info> _alt_call_db; ///< database of alternative calls
  std::map<std::string, location_info> _db_checked;  ///< call- or prefix-associated info -- a cache of all previously checked calls

  std::map<std::string, russian_data_per_substring> _russian_db;  ///< Russian substring-indexed info

// unordered_map will not serialize with boost 1.49
//   it still doesn't serialize in boost 1.53
//  std::unordered_map<std::string, location_info> _alt_call_db; ///< database of alternative calls
//  std::unordered_map<std::string, location_info> _db_checked;  ///< call- or prefix-associated info -- checked calls
  
  drlog_qth_database                   _qth_db;      ///< additional database

  void _init(const cty_data& cty, const enum country_list_type country_list);
  void _insert_alternatives(const location_info& info, const std::map<std::string, alternative_country_info>& alternatives);

  pt_mutex                             _location_database_mutex;  ///< to make location_database objects thread-safe;
   
public:

/// default constructor
  location_database(void);

/// construct from CTY.DAT filename and the definition of which country list to use
  location_database(const std::string& filename, const enum country_list_type country_list = COUNTRY_LIST_DXCC);

/// construct from CTY.DAT data and the definition of which country list to use
  location_database(const cty_data& cty, const enum country_list_type country_list = COUNTRY_LIST_DXCC);

/// construct from CTY.DAT data, the definition of which country list to use and a secondary qth database
  location_database(const cty_data& cty, const enum country_list_type country_list, const drlog_qth_database& secondary);

/// prepare a default-constructed object for use
  void prepare(const cty_data& cty, const enum country_list_type country_list = COUNTRY_LIST_DXCC);

/// prepare a default-constructed object for use
  void prepare(const cty_data& cty, const enum country_list_type country_list, const drlog_qth_database& secondary);

// add Russian information
  void add_russian_database(const std::vector<std::string>& path, const std::string& filename);

/// how large is the main database?
  inline const size_t size(void)
    { return (SAFELOCK_GET( _location_database_mutex, _db.size() )); }

  void add_alt_call(const std::string& call, const location_info& li);

/// return all the useful information, given a callsign
  const location_info info(const std::string& callpart);
  
  inline const std::map<std::string, location_info> db(void)
    { return (SAFELOCK_GET( _location_database_mutex, _db )); }

/// create a set of all the canonical prefixes for countries
  const std::set<std::string> countries(void);

/// create a set of all the canonical prefixes for a particular continent
  const std::set<std::string> countries(const std::string& cont_target);
  
/// access individual items of information
  inline const std::string country_name(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).country_name() )); }

  inline const unsigned int cq_zone(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).cq_zone() )); }

  inline const unsigned int itu_zone(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).itu_zone() )); }

  inline const std::string continent(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).continent() )); }

  inline const float latitude(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).latitude() )); }

  inline const float longitude(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).longitude() )); }

  inline const int utc_offset(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).utc_offset() )); }

  inline const std::string canonical_prefix(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).canonical_prefix() )); }

  inline const std::string region_name(const std::string& callpart)
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).region_name() )); }

  const std::string region_abbreviation(const std::string& callpart);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(_location_database);

      ar & _db
         & _alt_call_db
         & _db_checked
         & _qth_db;
    }
};

// ostream << location_database
std::ostream& operator<<(std::ostream& ost, const location_database& db);

// -------------------------------------- Errors  -----------------------------------

/*! \class  cty_error
  \brief  Errors related to CTY processing
*/

class cty_error : public x_error
{ 
protected:

public:

/*! \brief  Construct from error code and reason
  \param  n Error code
  \param  s Reason
*/
  inline cty_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};

/*!     \class  location_error
        \brief  Errors related to location database processing
*/

class location_error : public x_error
{ 
protected:

public:

/*!     \brief  Construct from error code and reason
        \param  n       Error code
        \param  s       Reason
*/
  inline location_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};

/*! \class  russian_error
    \brief  Errors related to processing Russian data file
*/

class russian_error : public x_error
{
protected:

public:

/*! \brief  Construct from error code and reason
  \param  n Error code
  \param  s Reason
*/
  inline russian_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};

#endif  // CTY_DATA_H
