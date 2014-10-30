// $Id: rules.h 81 2014-10-27 18:31:40Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef RULES_H
#define RULES_H

/*!     \file rules.h

        Classes and functions related to the contest rules
*/

#include "bands-modes.h"
#include "cty_data.h"
#include "drlog_context.h"
#include "macros.h"
#include "pthread_support.h"
#include "serialization.h"

#include <set>
#include <tuple>
#include <utility>
#include <vector>

extern pt_mutex  rules_mutex;                           ///< mutex for rules

typedef std::map<std::string, unsigned int> MSI;        // syntactic sugar

// forward declarations
class EFT;
class QSO;

/// Some contest have very complex point structures
enum points_type { POINTS_NORMAL,                       ///< points defined in configuration file
                   POINTS_IARU                          ///< IARU contest
                 };

// -------------------------  exchange_field_values  ---------------------------

/*!     \class exchange_field_values
        \brief Encapsulates the name and legal values for an exchange field
*/

class exchange_field_values
{
protected:

  std::string _name;                           ///< name of the exchange field
  std::map<std::string,                        /* a canonical field value */
          std::set                             /* each equivalent value is a member of the set, including the canonical value */
            <std::string                       /* indistinguishable legal values */
            >> _values;                        ///< associate legal values with a canonical value

public:

/// default constructor
  exchange_field_values(void)
  { }

/*!     \brief      Construct from useful values
        \param  nm  Name of exchange field
        \param  mss Canonical field value, all equivalent values (including canonical value)
*/
  exchange_field_values(const std::string& nm, const std::map<std::string, std::set< std::string >> mss) :
    _name(nm),
    _values(mss)
  { }

  READ_AND_WRITE(name);             ///< name of the exchange field
  READ_AND_WRITE(values);           ///< associate legal values with a canonical value

/*!     \brief      Add a canonical value
        \param  cv  canonical value to add

        Also adds <i>cv</i> as a possible value
*/
  void add_canonical_value(const std::string& cv);

/*!     \brief      Add a possible value
        \param  cv  canonical value to which <i>v</i> is to be added
        \param  v   value to be added

        Also adds <i>cv</i> as a canonical value if it does not already exist
*/
  void add_value(const std::string& cv, const std::string& v);

/*!     \brief      Number of possible values for a particular canonical value
        \param  cv  canonical value
        \return     number of possible values for the canonical value <i>cv</i>

        Returns 0 if the canonical value does not exist
*/
  const size_t n_values(const std::string& cv) const;

/// Get the number of canonical values
  inline const size_t n_canonical_values(void) const
    { return _values.size(); }

/// Get all the canonical values
  const std::set<std::string> canonical_values(void) const;

/// Get all the legal values for a single canonical value
  const std::set<std::string> values(const std::string& cv) const;

/// Get all the legal values (for all canonical values)
  const std::set<std::string> all_values(void) const;

/// Is a string a known canonical value?
  inline const bool canonical_value_present(const std::string& cv) const
    { return (_values.find(cv) != _values.cend()); }

/// Is a string a known canonical value? Synonym for canonical_value_present()
  inline const bool is_legal_canonical_value(const std::string& putative_cv_value) const
    { return canonical_value_present(putative_cv_value); }

/// Is a string a legal value (for any canonical value)
  const bool is_legal_value(const std::string& value) const;

/*!     \brief                  Is a particular value legal for a given canonical value?
        \param  cv              canonical value
        \param  putative_value  value to test
        \return     Whether <i>putative_value</i> is a legal value for the canonical value <i>cv</i>
*/
  const bool is_legal_value(const std::string& cv, const std::string& putative_value) const;

/// read from and write to disk
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _name
         & _values;
    }
};

// -------------------------  exchange_field  ---------------------------

/*!     \class exchange_field
        \brief Encapsulates the name for an exchange field, and whether it's a mult
*/

class exchange_field
{
protected:

  std::string                 _name;            ///< name of field
  bool                        _is_mult;         ///< is this field a multiplier?
  bool                        _is_optional;     ///< is this an optional field?
  std::vector<exchange_field> _choice;          ///< is this field a choice?

public:

/// construct from name, multiplier and optional status; also default constructor
  exchange_field(const std::string& nm = std::string(), const bool mult = false, const bool opt = false);

  READ(name);                          ///< name of field
  READ(is_mult);                       ///< is this field a multiplier?
  READ(is_optional);                   ///< is this an optional field?
  READ_AND_WRITE(choice);              ///< is this field a choice?

/// is this field a choice?
  inline const bool is_choice(void) const
    { return !_choice.empty(); }

/// follow all trees to their leaves
  const std::vector<exchange_field> expand(void) const;

/// read from and write to disk
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _name
         & _is_mult
         & _is_optional
         & _choice;
    }
};

/// ostream << exchange_field
std::ostream& operator<<(std::ostream& ost, const exchange_field& exch_f);

// -------------------------  points_structure  ---------------------------

/*!     \class points_structure
        \brief Encapsulate the vagaries of points-per-QSO rules
*/

class points_structure
{
protected:
  unsigned int                        _default_points;      ///< default points
  std::map<std::string, unsigned int> _country_points;      ///< per-country points
  std::map<std::string, unsigned int> _continent_points;    ///< per-continent points

  enum points_type                    _points_type;         ///< is the points structure too complex for the configuration notation?

public:

/// default constructor
  points_structure(void);

  READ_AND_WRITE(default_points);      ///< default points
  READ_AND_WRITE(country_points);      ///< per-country points
  READ_AND_WRITE(continent_points);    ///< per-continent points
  READ_AND_WRITE(points_type);         ///< is the points structure too complex for the configuration notation?

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _default_points
       & _country_points
       & _continent_points
       & _points_type;
  }
};

// -------------------------  contest_rules  ---------------------------

/*!     \class contest_rules
        \brief A place to maintain all the rules
        
        This object should be created and initialized early, and from that
        point it should be treated as read-only. Being paranoid, though, there's still lots of
        internal locking, since there are ongoing debates about the true thread safety of
        strings in libstc++.
*/

class contest_rules
{
protected:
    
  std::set<MODE>    _permitted_modes;            ///< modes allowed in this contest
  std::vector<BAND> _permitted_bands;            ///< bands allowed in this contest; use a vector container in order to keep the frequency order
  
  std::map<std::string, std::vector<exchange_field>>                        _exch;             ///< details of the received exchange fields
//  canonical prefix, vector of fields in the exchange for that prefix

  std::map<std::string, std::vector<exchange_field>> _expanded_exch;    ///< details of the received exchange fields, with choices expanded (i.e., the leaves of the _exch element)

  std::vector<std::string>    _sent_exchange;    ///< names of fields in the sent exchange

// dupe rules
  bool              _work_if_different_band;     ///< whether it is OK to work the same station on different bands
  bool              _work_if_different_mode;     ///< whether it is OK to work the same station on different modes
  
  std::map<BAND, points_structure>     _points;  ///< points per QSO

  std::map<std::string, unsigned int>  _exchange_present_points;  ///< number of points if a particular exchange field is received
  std::map<std::string, unsigned int>  _exchange_value_points;    ///< number of points if a particular exchange field has a particular value

  std::set<std::string>               _countries;                     ///< collection of canonical prefixes for all the valid countries
  std::set<std::string>               _country_mults;                 ///< collection of canonical prefixes of country multipliers
  bool                                _country_mults_per_band;        ///< are country mults counted per-band?
  bool                                _country_mults_used;            ///< are country mults used?
  std::map<BAND, int>                 _per_band_country_mult_factor;  ///< factor by which to multiply number of country mults, per band

  std::set<std::string>               _callsign_mults;           ///< collection of types of mults based on callsign (e.g., "WPXPX")
  bool                                _callsign_mults_per_band;  ///< are callsign mults counted per-band?
  bool                                _callsign_mults_used;      ///< are callsign mults used?

  std::vector<std::string>            _exchange_mults;           ///< names of the exchange fields that are mults, in the same order as in the configuration file
  bool                                _exchange_mults_per_band;  ///< are exchange mults counted per-band?
  bool                                _exchange_mults_used;      ///< are exchange mults used?

// structures to hold information about the possible values of exchange fields

/// all the equivalent values for all exchange fields; the map is empty if there are no canonical values
  std::vector<exchange_field_values>  _exch_values;

/// all the legal values for each exchange field that has defined legal values
  std::map
    <std::string,                                          /* exch field name */
      std::set<std::string> >                   _permitted_exchange_values;  ///< all the legal values for each exchange field that has defined legal values

/// mapping from permitted values to canonical values
  std::map
    <std::string,                                          /* exch field name */
      std::map
        <std::string,                                      /* permitted value */
          std::string                                      /* canonical value */
           > >                                  _permitted_to_canonical;

  std::map<std::string /* field name */, EFT>   _exchange_field_eft;

// copied from context, so that we can score correctly without loading context
  std::set<BAND>                               _score_bands;            ///< bands currently used to calculate score
  std::set<BAND>                               _original_score_bands;   ///< bands that were originally used to calculate score (from the configuration file)

// my information (from context)  &&& TODO
  std::string                                  _my_continent;        ///< my continent
  std::string                                  _my_country;          ///< canonical prefix for my country
  unsigned int                                 _my_cq_zone;          ///< CQ zone
  unsigned int                                 _my_itu_zone;         ///< ITU zone

  bool                                         _send_qtcs;           ///< whether to send QTCs

/*!     \brief              private function used to obtain all the understood values for a particular exchange field
        \param  field_name  name of the field for which the understood values are required
        \return             set of all the legal values for the field <i>field_name</i>

        Uses the variable <i>_exch_values</i> to obtain the returned value
*/
  const std::set<std::string> _all_exchange_values(const std::string& field_name) const;

/*!     \brief              parse all the "exchange [xx] = " lines from context
        \param  context     drlog context
        \return             name/mult/optional/choice status for exchange fields
*/
  const std::map<std::string, std::vector<exchange_field>> _parse_context_exchange(const drlog_context& context) const;

  const std::vector<exchange_field> _inner_parse(const std::vector<std::string>& exchange_fields , const std::vector<std::string>& exchange_mults_vec) const;

  void _parse_context_qthx(const drlog_context& context, location_database& location_db);
  void _init(const drlog_context& context, location_database& location_db);

public:
  
/// default constructor
  contest_rules(void);

/// constructor
  contest_rules(const drlog_context& context, location_database& location_db);
  
/// initialise for use (if created with default constructor)
  void prepare(const drlog_context& context, location_database& location_db);

/// is a particular mode permitted?
  inline const bool permitted_mode(const MODE mode) const
    { SAFELOCK(rules); return (_permitted_modes < mode); }
    
/// add a mode to the list of permitted modes
  inline void add_permitted_mode(const MODE mode)
    { SAFELOCK(rules); _permitted_modes.insert(mode); }
    
/// get the next mode
  const MODE next_mode(const MODE current_mode) const;

/// add a band to the list of permitted bands
  void add_permitted_band(const BAND b);
  
/// is a particular band permitted?
  const bool permitted_band(const BAND b) const;

/// get the next band up
  const BAND next_band_up(const BAND current_band) const;

/// get the next band down
  const BAND next_band_down(const BAND current_band) const;
  
  SAFEREAD(work_if_different_band, rules);               ///< whether it is OK to work the same station on different bands
  SAFEREAD(work_if_different_mode, rules);               ///< whether it is OK to work the same station on different modes
  SAFEREAD(permitted_bands, rules);                      ///< bands allowed in this contest

/*!     \brief                      Get the expected exchange fields for a particular canonical prefix
        \param  canonical_prefix    canonical prefix
        \return                     The exchange fields associated with <i>canonical_prefix</i>
*/
  const std::vector<exchange_field> exch(const std::string& canonical_prefix) const;

  const std::vector<exchange_field> expanded_exch(const std::string& canonical_prefix) const;

  SAFEREAD(country_mults, rules);       ///< collection of canonical prefixes of country multipliers
  SAFEREAD(callsign_mults, rules);      ///< collection of types of mults based on callsign (e.g., "WPXPX")

  SAFEREAD(per_band_country_mult_factor, rules);         ///< factor by which to multiply number of country mults, per band (see WAE rules)

  SAFEREAD(callsign_mults_per_band, rules);              ///< are callsign mults counted per-band?
  SAFEREAD(country_mults_per_band, rules);               ///< are country mults counted per-band?
  SAFEREAD(exchange_mults_per_band, rules);              ///< are exchange mults counted per-band?

  SAFEREAD(callsign_mults_used, rules);                  ///< are callsign mults used?
  SAFEREAD(country_mults_used, rules);                   ///< are country mults used?
  SAFEREAD(exchange_mults_used, rules);                  ///< are exchange mults used?

  SAFEREAD(score_bands, rules);               ///< bands currently used to calculate score
  SAFEREAD(original_score_bands, rules);      ///< bands that were originally used to calculate score (from the configuration file)
  SAFEREAD(exchange_mults, rules);  ///< the exchange multipliers, in the same order as in the configuration file

  SAFEREAD(send_qtcs, rules);               ///< Can QTCs be sent?

  SAFEREAD(exchange_field_eft, rules);

  const EFT exchange_field_eft(const std::string& field_name) const;

//  SAFEREAD(callsign_eft, rules);

/// Get all the known names of exchange fields
  const std::set<std::string> all_known_field_names(void) const;

/// restore the original set of bands to be scored (from the configuration file)
  inline void restore_original_score_bands(void)
    { _score_bands = _original_score_bands; }

/*! \brief  Define a new set of bands to be scored
    \param  new_bands   the set of bands to be scored
 */
  inline void score_bands(const std::set<BAND> new_bands)
    { _score_bands = new_bands; }

/*! \brief  Is a particular exchange field a multiplier
    \param  name   name of an exchange field (received)
*/
  const bool is_exchange_mult(const std::string& name) const;

/*! \brief  All the canonical values for a particular exchange field
    \param  field_name   name of an exchange field (received)

    Returns empty vector if no acceptable values are found (e.g., RST, RS, SERNO)
*/
  const std::vector<std::string> exch_canonical_values(const std::string& field_name) const;

/*! \brief  The permitted values for a particular exchange field
    \param  field_name   name of an exchange field (received)

    Returns empty set if the field can take any value
*/
  const std::set<std::string> exch_permitted_values(const std::string& field_name) const;

/*! \brief  Is a particular exchange field limited to only permitted values?
    \param  field_name   name of an exchange field (received)
*/
  inline const bool exch_has_permitted_values(const std::string& field_name) const
    { SAFELOCK(rules);

      return (_permitted_exchange_values.find(field_name) != _permitted_exchange_values.cend());
    }

/*! \brief  A canonical value
    \param  field_name   name of an exchange field (received)
    \param  acual_value  received value of the field

    Returns the received value if there are no canonical values
*/
  const std::string canonical_value(const std::string& field_name, const std::string& actual_value) const;

/*! \brief                      Add a canonical value for a particular exchange field
    \param  field_name          name of an exchange field (received)
    \param  new_canonical_value the canonical value to add

    Also adds <i>new_canonical_value</i> to the legal values for the field <i>field_name</i>. Does nothing
    if <i>new_canonical_value</i> is already a canonical value.
*/
  void add_exch_canonical_value(const std::string& field_name, const std::string& new_canonical_value);

/*! \brief                           Is a particular string the canonical value for a particular exchange field?
    \param  field_name               name of an exchange field (received)
    \param  putative_canonical_value the value to check

    Returns false if <i>field_name</i> is unrecognized
*/
  const bool is_canonical_value(const std::string& field_name, const std::string& putative_canonical_value) const;

/*! \brief                           Is a particular string a legal value for a particular exchange field?
    \param  field_name               name of an exchange field (received)
    \param  putative_value           the value to check

    Returns false if <i>field_name</i> is unrecognized
*/
    const bool is_legal_value(const std::string& field_name, const std::string& putative_canonical_value) const;

///< number of points if a particular exchange field has a particular value
  inline const std::map<std::string, unsigned int> exchange_value_points(void) const
    { SAFELOCK(rules); return _exchange_value_points; };

/// number of permitted bands
  inline const unsigned int n_bands(void) const
    { SAFELOCK(rules); return _permitted_bands.size(); }

/// number of permitted modes
  inline const unsigned int n_modes(void) const
    { SAFELOCK(rules); return _permitted_modes.size(); }

/// number of country mults
  inline const unsigned int n_country_mults(void) const
    { SAFELOCK(rules); return _country_mults.size(); }
    
/*! \brief  Points for a particular QSO
    \param  qso          QSO for which the points are to be calculated
    \param  location_db  location database

    Return the (location-based) points for <i>qso</i>
*/
  const unsigned int points(const QSO& qso, location_database& location_db) const;

/// human-readable string
  const std::string to_string(void) const;

// does the sent exchange include a particular field?
  inline const bool sent_exchange_includes(const std::string& str)
    { return (find(_sent_exchange.begin(), _sent_exchange.end(), str) != _sent_exchange.end()); }

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(rules);

     ar  & _permitted_modes
         & _permitted_bands
         & _exch
         & _expanded_exch
         & _sent_exchange
         & _exchange_mults
         & _exchange_mults_used
         & _work_if_different_band
         & _points
         & _exchange_present_points
         & _exchange_value_points
         & _countries
         & _country_mults
         & _country_mults_used
         & _callsign_mults
         & _callsign_mults_used
         & _callsign_mults_per_band
         & _exch_values
         & _permitted_exchange_values
         & _permitted_to_canonical
         & _score_bands
         & _original_score_bands
         & _my_continent
         & _my_country
         & _my_cq_zone
         & _my_itu_zone;
//         & _qthx_vector;              // &&& MORE HERE
    }
};

/*! \brief  The WPX prefix for a particular call
    \param  call         call for which the prefix is to be calculated

    Return the WPX prefix for <i>call</i>
*/
const std::string wpx_prefix(const std::string& call);

/*! \brief  The SAC prefix for a particular call
    \param  call         call for which the prefix is to be calculated

    Return the SAC prefix for <i>call</i>
*/
const std::string sac_prefix(const std::string& call);

/*! \brief  Given a received value of a particular multiplier field, what is the actual mult value?
    \param  field_name         name of the field
    \param  received_value     received value for field <i>field_name</i>
    \return                    The multiplier value for the field <i>field_name</i>

    For example, the mult value for a DOK field with the value A01 is A.
*/
const std::string MULT_VALUE(const std::string& field_name, const std::string& received_value);

#endif    // RULES_H

