// $Id: cty_data.h 271 2025-06-23 16:32:50Z  $

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
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/// country lists
enum class COUNTRY_LIST { DXCC,     ///< DXCC list
                          WAEDC     ///< DARC WAEDC list
                        };

/// alternative prexies and alternative callsigns are /almost/ the same
enum class ALTERNATIVES { CALLSIGNS,
                          PREFIXES
                        };
                       
// error numbers
constexpr int CTY_INCORRECT_NUMBER_OF_FIELDS { -1 },    ///< Wrong number of fields in a record
              CTY_INVALID_CQ_ZONE            { -2 },    ///< Invalid CQ zone
              CTY_INVALID_ITU_ZONE           { -3 },    ///< Invalid ITU zone
              CTY_INVALID_CONTINENT          { -4 },    ///< Invalid continent
              CTY_INVALID_LATITUDE           { -5 },    ///< Invalid latitude
              CTY_INVALID_LONGITUDE          { -6 },    ///< Invalid longitude
              CTY_INVALID_UTC_OFFSET         { -7 },    ///< Invalid UTC offset
              CTY_INVALID_PREFIX             { -8 };    ///< Invalid country prefix
          
constexpr int LOCATION_NO_PREFIX_MATCH       { -1 },    ///< unable to find a prefix match in the database
              LOCATION_TOO_MANY_SLASHES      { -2 };    ///< more than two slashes in the call

constexpr int RUSSIAN_INVALID_SUBSTRING      { -1 },    ///< source substring does not match target line in constructor
              RUSSIAN_INVALID_FORMAT         { -2 };    ///< format of file is invalid

// -----------  alternative_country_info  ----------------

/*! \class  alternative_country_info
    \brief  A single alternative prefix or callsign for a country

    CTY files may contain "alias" information. This encapsulates that information.
*/

class alternative_country_info
{
protected:

  std::string  _country;                ///< canonical country prefix
  unsigned int _cq_zone     { 0 };      ///< alternative CQ zone
  std::string  _identifier;             ///< the alternative prefix or callsign
  unsigned int _itu_zone    { 0 };      ///< alternative ITU zone

public:

/*! \brief                      Construct from a string and a canonical country prefix
    \param  record              record from which to construct the alternative information
    \param  canonical_prefix    canonical country prefix

    <i>record</i> looks something like "=G4AMJ(14)[28]" or like "3H0(23)[42], where the delimited information
    is optional
*/
  alternative_country_info(const std::string_view record, const std::string& canonical_prefix = std::string { });

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

class cty_record
{
protected:

  using ACI_DBTYPE = UNORDERED_STRING_MAP<alternative_country_info>;

  ACI_DBTYPE    _alt_callsigns;         ///< alternative callsigns used by this country
  ACI_DBTYPE    _alt_prefixes;          ///< alternative prefixes used by this country
  std::string   _continent;             ///< two-letter abbreviation for continent
  std::string   _country_name;          ///< official name of the country
  unsigned int  _cq_zone;               ///< CQ zone
  unsigned int  _itu_zone;              ///< ITU zone
  float         _latitude;              ///< latitude in degrees (+ve north)
  float         _longitude;             ///< longitude in degrees (+ve west)
  std::string   _prefix;                ///< official DXCC prefix, in upper case
  int           _utc_offset;            ///< local-time offset from UTC, in minutes
  bool          _waedc_country_only;    ///< Is this only a country in the WAEDC (DARC) list?

public:
  
/*! \brief          Construct from a string
    \param  record  a record from the CTY file

    The string is assumed to contain a single record. We don't catch all
    possible errors, but we do test for the most obvious ones.
*/
  explicit cty_record(const std::string_view record);

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
  inline std::string canonical_prefix(void) const
    { return prefix(); }
    
/*! \brief          Remove an alternative callsign
    \param  call    alternative call to remove
        
    It is not an error to attempt to remove a call that does not exist
*/
  inline void remove_alternative_callsign(const std::string& call)
    { _alt_callsigns -= call; }

/*! \brief          Remove an alternative prefix
    \param  prefix  alternative prefix to remove
        
    It is not an error to attempt to remove a prefix that does not exist
*/
  inline void remove_alternative_prefix(const std::string& prefix)
    { _alt_prefixes -= prefix; }   

/*! \brief          Is a string an alternative callsign?
    \param  call    string to check
    \return         whether <i>call</i> is an alternative callsign
*/
  inline bool is_alternative_callsign(const std::string& call) const
    { return _alt_callsigns.contains(call); }

/*! \brief  is a string an alternative prefix?
    \param  pfx    prefix to check
    \return        whether <i>pfx</i> is an alternative prefix
*/
  inline bool is_alternative_prefix(const std::string& pfx) const
    { return _alt_prefixes.contains(pfx); }
    
  friend class location_database;           // in order to maintain type of ACI_DBTYPE across classes
};

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

class cty_data : public std::vector<cty_record>
{
protected:

// all the alternative calls and prefixes (these are also maintained on a per-record basis)
  STRING_MAP<alternative_country_info> _alt_callsigns;    ///< key = alternative callsign
  STRING_MAP<alternative_country_info> _alt_prefixes;     ///< key = alternative prefix

public:

/*! \brief              Construct from a file
    \param  filename    name of file
*/
  explicit cty_data(const std::string_view filename = "cty.dat"sv);   // somewhere along the way the default name changed from CTY.DAT

/*! \brief              Construct from a file
    \param  path        directories in which to search for <i>filename</i>, in order
    \param  filename    name of file
*/
  inline explicit cty_data(const std::vector<std::string>& path, const std::string_view filename = "cty.dat"sv)  // somewhere along the way the default name changed from CTY.DAT
    { FOR_ALL(split_string_into_records <std::string_view> (remove_chars(read_file(path, filename), CRLF), ';', DELIMITERS::DROP), [this] (const std::string_view rec) { emplace_back(cty_record { rec }); }); }    // applies to base class

/// how many countries are present?
  inline unsigned int n_countries(void) const
    { return size(); }
  
/// return a record by number, wrt 0, with range checking
  inline cty_record operator[](const unsigned int n) const
    { return this->at(n); }
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
  russian_data_per_substring(const std::string_view sbstring, const std::string_view line);

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
        
    This is basically just a simple tuple
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
  std::string  _region_name         { };    ///< name of (Russian) region in which station resides
  std::string  _region_abbreviation { };    ///< abbreviation for (Russian) region in which the station resides

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

/// location_info == location_info
  bool operator==(const location_info& li) const;

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
  void latitude_longitude(const float lat, const float lon);

/*! \brief          Set both CQ and ITU zones at once
    \param  cqz     CQ zone
    \param  ituz    ITU zone
*/
  void zones(const unsigned int cqz, const unsigned int ituz);

/// archive using boost
   template<typename Archive>
   void serialize(Archive& ar, const unsigned int version)
     { unsigned int v { version };   // dummy; for now, version isn't used
       v = v + 0;

       ar & _canonical_prefix
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
location_info guess_zones(const std::string_view call, const location_info& li);

// -----------  location_database  ----------------

/*! \class  location_database
    \brief  The country-based location information packaged for use by drlog
*/

class location_database
{
protected:

  using ACI_DBTYPE      = decltype(cty_record::_alt_callsigns);
  using LOCATION_DBTYPE = UNORDERED_STRING_MAP<location_info>;
  using RUSSIAN_DBTYPE  = UNORDERED_STRING_MAP<russian_data_per_substring>;  // there doesn't seem to be any way to make this accessible to russian_data; so it is redefined in that class

  mutable std::recursive_mutex _location_database_mutex;  ///< to make location_database objects thread-safe;

  LOCATION_DBTYPE _db;          ///< prefix-associated info -- the original database
  LOCATION_DBTYPE _alt_call_db; ///< database of alternative calls

  mutable LOCATION_DBTYPE _db_checked;  ///< call- or prefix-associated info -- a cache of all previously checked calls

  RUSSIAN_DBTYPE _russian_db;  ///< Russian substring-indexed info

/*! \brief                  Initialise the database
    \param  cty             cty.dat data
    \param  country_list    type of country list
*/
  void _init(const cty_data& cty, const COUNTRY_LIST country_list);

/*! \brief                  Insert alternatives into the database
    \param  info            the original location information
    \param  alternatives    alternative country information
*/
  void _insert_alternatives(const location_info& info, const ACI_DBTYPE& alternatives);

/*! \brief              Process alternatives from a record
    \param  rec         the record to process
    \param  alt_type    type of alternatives to process
*/
  void _process_alternative(const cty_record& rec, const enum ALTERNATIVES alt_type);

public:

/// default constructor
  location_database(void) = default;

/*! \brief                  Constructor
    \param  filename        name of cty.dat file
    \param  country_list    type of country list
*/
  explicit location_database(const std::string_view filename, const COUNTRY_LIST country_list = COUNTRY_LIST::DXCC);

/*! \brief                  Constructor
    \param  cty             cty.dat data
    \param  country_list    type of country list
*/
  inline explicit location_database(const cty_data& cty, const COUNTRY_LIST country_list = COUNTRY_LIST::DXCC)
    { _init(cty, country_list); }

/// no copy constructor
  location_database(const location_database&) = delete;

/*! \brief                  Prepare a default-constructed object for use
    \param  cty             cty.dat data
    \param  country_list    type of country list
*/
  inline void prepare(const cty_data& cty, const COUNTRY_LIST country_list = COUNTRY_LIST::DXCC)
    { _init(cty, country_list); }

/*! \brief              Add Russian information
    \param  path        vector of directories to check for file <i>filename</i>
    \param  filename    name of file containing Russian information
*/
  void add_russian_database(const std::vector<std::string>& path, const std::string_view filename);

/// how large is the main database?
  inline size_t size(void) const
    { return (SAFELOCK_GET( _location_database_mutex, _db.size() )); }

/*! \brief          Add a call to the alt_call database
    \param  call    callsign to add
    \param  li      location information for <i>call</i>

    Overwrites any extant entry with <i>call</i> as the key
*/
  inline void add_alt_call(const std::string& call, const location_info& li)
    { _alt_call_db += { call, li }; }

/*! \brief              Get location information for a particular call or partial call
    \param  callpart    call (or partial call)
    \return             location information corresponding to <i>call</i>
*/
  location_info info(const std::string_view callpart) const;

/// return the database
  inline decltype(location_database::_db) db(void) const
    { return (SAFELOCK_GET( _location_database_mutex, _db )); }

/// create a set of all the canonical prefixes for countries
  UNORDERED_STRING_SET countries(void) const;

/// create a set of all the canonical prefixes for a particular continent
//  std::unordered_set<std::string> countries(const std::string& cont_target) const;
//  UNORDERED_STRING_SET countries(const std::string& cont_target) const;
  UNORDERED_STRING_SET countries(const std::string_view cont_target) const;

/*! \brief              Get official name of the country associated with a call or partial call
    \param  callpart    call (or partial call)
    \return             official name of the country corresponding to <i>callpart</i>
*/
  inline std::string country_name(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).country_name() )); }

/*! \brief              Get CQ zone associated with a call or partial call
    \param  callpart    call (or partial call)
    \return             CQ zone corresponding to <i>callpart</i>
*/
  inline unsigned int cq_zone(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).cq_zone() )); }

/*! \brief              Get ITU zone associated with a call or partial call
    \param  callpart    call (or partial call)
    \return             ITU zone corresponding to <i>callpart</i>
*/
  inline unsigned int itu_zone(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).itu_zone() )); }

/*! \brief              Get the continent associated with a call or partial call
    \param  callpart    call (or partial call)
    \return             continent to <i>callpart</i>

    The returned continent is in the form of the two-letter code
*/
  inline std::string continent(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).continent() )); }

/*! \brief              Get the latitude for a call or partial call
    \param  callpart    call (or partial call)
    \return             latitude (in degrees) corresponding to <i>callpart</i> (+ve north)
*/
  inline float latitude(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).latitude() )); }

/*! \brief              Get the longitude for a call or partial call
    \param  callpart    call (or partial call)
    \return             longitude (in degrees) corresponding to <i>callpart</i> (+ve west)
*/
  inline float longitude(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).longitude() )); }

/*! \brief              Get the UTC offset for a call or partial call
    \param  callpart    call (or partial call)
    \return             UTC offset (in minutes) corresponding to <i>callpart</i>
*/
  inline int utc_offset(const std::string& callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).utc_offset() )); }

/*! \brief              Get the canonical prefix for a call or partial call
    \param  callpart    call (or partial call)
    \return             canonical prefix corresponding to <i>callpart</i>
*/
  inline std::string canonical_prefix(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).canonical_prefix() )); }

/*! \brief              Get name of the Russian district for a particular call or partial call
    \param  callpart    call (or partial call)
    \return             name of the Russian district corresponding to <i>callpart</i>

    Returns the empty string if <i>callpart</i> is not Russian
*/
  inline std::string region_name(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).region_name() )); }

/*! \brief              Get two-letter abbreviation for the Russian district for a particular call or partial call
    \param  callpart    call (or partial call)
    \return             two-letter Russian region code corresponding to <i>callpart</i>

    Returns the empty string if <i>callpart</i> is not Russian
*/
  inline std::string region_abbreviation(const std::string_view callpart) const
    { return (SAFELOCK_GET( _location_database_mutex, info(callpart).region_abbreviation() )); }

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned int version)
    { unsigned int v { version };   // dummy; for now, version isn't used
      v = v + 0;

      std::lock_guard lg(_location_database_mutex);

      ar & _db
         & _alt_call_db
         & _db_checked;
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

  using RUSSIAN_DBTYPE = decltype(location_database::_russian_db);

  RUSSIAN_DBTYPE _data;          ///< map substring to the matching data

public:

/*! \brief              Construct from a file
    \param  path        the directory path to be searched in order
    \param  filename    the name of the file to be read
*/
  russian_data(const std::vector<std::string>& path, const std::string_view filename);

  READ(data);  ///< map substring to the matching data
};

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(cty_error);         ///< Errors related to CTY processing

ERROR_CLASS(location_error);    ///< Errors related to location database processing

ERROR_CLASS(russian_error);     ///< Errors related to processing Russian data file

#endif  // CTY_DATA_H
