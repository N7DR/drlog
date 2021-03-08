// $Id: rules.h 179 2021-02-22 15:55:56Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef RULES_H
#define RULES_H

/*! \file   rules.h

    Classes and functions related to the contest rules
*/

#include "bands-modes.h"
#include "cty_data.h"
#include "drlog_context.h"
#include "exchange_field_template.h"
#include "grid.h"
#include "macros.h"
#include "pthread_support.h"
#include "serialization.h"

#include <set>
#include <tuple>
#include <utility>
#include <vector>

extern pt_mutex  rules_mutex;                           ///< mutex for rules

/// Some contests have unusual point structures
enum class POINTS { NORMAL,                       ///< points defined in configuration file
                    IARU,                         ///< IARU contest
                    STEW                          ///< Stew Perry contest
                  };

/// Syntactic sugar for read-only access
#define RULESREAD(y)          \
  inline decltype(_##y) y(void) const { SAFELOCK(rules); return _##y; }

// forward declarations
class QSO;

// -------------------------  choice_equivalents  ---------------------------

/*! \class  choice_equivalents
    \brief  Encapsulates the possibilities for a CHOICE received exchange

    Assumes that CHOICEs are in pairs
*/

class choice_equivalents
{
protected:

  std::map<std::string /* one possible field name */, std::string /* other possible field name */> _choices;    // the choices; could have inherited this class from this type

public:

/*! \brief              Add a pair of equivalent fields
    \param  ch1_ch2     pair: first element of choice, second element of choice
*/
  void operator+=(const std::pair<std::string, std::string>& ch1_ch2);

/*! \brief              Add a pair of equivalent fields in the form "FIELD1+FIELD2"
    \param  ch1_ch2     the two fields, separated by a plus sign

    Throws exception if <i>ch1_ch2<i> appears to be malformed
*/
  void operator+=(const std::string& ch1_ch2);

/*! \brief              Add a pair of equivalent fields only if the form is "FIELD1+FIELD2"
    \param  ch1_ch2     the two fields, separated by a plus sign

    If <i>ch1_ch2</i> appears to be malformed, does not attempt to add.
*/
  void add_if_choice(const std::string& ch1_ch2);

/*! \brief              Return the other choice of a pair
    \param  field_name  current field name
    \return             alternative choice for the field name

    Return empty string if <i>field_name</i> is not a choice
*/
  inline std::string other_choice(const std::string& field_name) const
    { return MUM_VALUE(_choices, field_name); }

/*! \brief              Is a field a choice?
    \param  field_name  current field name
    \return             whether there is an alternative field for <i>field_name</i>
*/
  inline bool is_choice(const std::string& field_name) const
    { return (_choices.find(field_name) != _choices.cend() ); }

/// return the inverse of whether there are any choices
  inline bool empty(void) const
    { return _choices.empty(); }

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _choices;
    }
};

// -------------------------  exchange_field_values  ---------------------------

/*! \class  exchange_field_values
    \brief  Encapsulates the name and legal values for an exchange field
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
  inline exchange_field_values(void) = default;

/*! \brief          Construct from name
    \param  nm      name of exchange field
*/
  explicit inline exchange_field_values(const std::string& nm) :
    _name(nm)
  { }

/*! \brief          Construct from useful values
    \param  nm      name of exchange field
    \param  mss     canonical field value, all equivalent values (including canonical value)
*/
  inline exchange_field_values(const std::string& nm, const std::map<std::string, std::set< std::string >> mss) :
    _name(nm),
    _values(mss)
  { }

  READ_AND_WRITE(name);             ///< name of the exchange field
  READ_AND_WRITE(values);           ///< associate legal values with a canonical value

/*! \brief      Add a canonical value
    \param  cv  canonical value to add

    Also adds <i>cv</i> as a possible value. Does nothing if <i>cv</i> is already
    present as a canonical value.
*/
  void add_canonical_value(const std::string& cv);

/*! \brief      Add a possible value
    \param  cv  canonical value to which <i>v</i> is to be added
    \param  v   value to be added

    Also adds <i>cv</i> as a canonical value if it does not already exist
*/
  void add_value(const std::string& cv, const std::string& v);

/*! \brief      Number of possible values for a particular canonical value
    \param  cv  canonical value
    \return     number of possible values for the canonical value <i>cv</i>

    Returns 0 if the canonical value does not exist
*/
  size_t n_values(const std::string& cv) const
    { return MUMF_VALUE(_values, cv, &std::set<std::string>::size); }

/// Get the number of canonical values
  inline size_t n_canonical_values(void) const
    { return _values.size(); }

/*! \brief      Get all the canonical values
    \return     all the canonical values

    Returns empty set if there are no canonical values
*/
  std::set<std::string> canonical_values(void) const;

/*! \brief      Get all the legal values for a single canonical value
    \param  cv  canonical value
    \return     all the legal values corresponding to the canonical value <i>cv</i>

    Returns empty set if the canonical value does not exist
*/
  inline std::set<std::string> values(const std::string& cv) const
    { return MUM_VALUE(_values, cv); }

/*! \brief      Get all the legal values (for all canonical values)
    \return     all possible legal values for all canonical values

    Returns empty set if there are no canonical values
*/
  std::set<std::string> all_values(void) const;

/*! \brief                      Is a string a known canonical value?
    \param  putative_cv_value   string to test
    \return                     whether <i>putative_cv_value</i> is a canonical value
*/
  inline bool canonical_value_present(const std::string& putative_cv_value) const
    { return (_values.find(putative_cv_value) != _values.cend()); }

/*! \brief                      Is a string a known canonical value? Synonym for canonical_value_present()
    \param  putative_cv_value   string to test
    \return                     whether <i>putative_cv_value</i> is a canonical value
*/
  inline bool is_legal_canonical_value(const std::string& putative_cv_value) const
    { return canonical_value_present(putative_cv_value); }

/*! \brief          Is a string a legal value (for any canonical value)
    \param  value   value to be tested
    \return         whether <i>value</i> is a legal value of any canonical value
*/
  bool is_legal_value(const std::string& value) const;

/*! \brief                  Is a particular value legal for a given canonical value?
    \param  cv              canonical value
    \param  putative_value  value to test
    \return                 Whether <i>putative_value</i> is a legal value for the canonical value <i>cv</i>
*/
  bool is_legal_value(const std::string& cv, const std::string& putative_value) const;

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _name
         & _values;
    }
};

// -------------------------  exchange_field  ---------------------------

/*! \class  exchange_field
    \brief  Encapsulates the name for an exchange field, and whether it's a mult/optional/choice
*/

class exchange_field
{
protected:

  std::string                 _name;            ///< name of field
  bool                        _is_mult;         ///< is this field a multiplier?
  bool                        _is_optional;     ///< is this an optional field?
  std::vector<exchange_field> _choice;          ///< equivalents if this is a choice

public:

/*! \brief          Construct from name, multiplier and optional status
    \param  nm      name of field
    \param  mult    whether field is a mult
    \param  opt     whether field is optional

    Also the default constructor
*/
  inline exchange_field(const std::string& nm = std::string(), const bool mult = false, const bool opt = false) :
    _name(nm),
    _is_mult(mult),
    _is_optional(opt)
  { }

  READ_AND_WRITE(name);                ///< name of field
  READ(is_mult);                       ///< is this field a multiplier?
  READ(is_optional);                   ///< is this an optional field?
  READ_AND_WRITE(choice);              ///< equivalents if this is a choice

/// is this field a choice?
  inline bool is_choice(void) const
    { return !_choice.empty(); }

/*! \brief      Follow all trees to their leaves
    \return     the exchange field, expanded recursively into all possible choices
*/
 std::vector<exchange_field> expand(void) const;

/// exchange_field < exchange_field
  inline bool operator<(const exchange_field& ef) const   // needed for set<exchange_field> to work
    { return (_name < ef.name()); }

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _name
         & _is_mult
         & _is_optional
         & _choice;
    }
};

/*! \brief          Write an <i>exchange_field</i> object to an output stream
    \param  ost     output stream
    \param  exch_f  object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const exchange_field& exch_f);

// -------------------------  points_structure  ---------------------------

/*! \class  points_structure
    \brief  Encapsulate the vagaries of points-per-QSO rules
*/

class points_structure
{
protected:

  std::map<std::string, unsigned int> _continent_points;                    ///< per-continent points
  std::map<std::string, unsigned int> _country_points;                      ///< per-country points
  unsigned int                        _default_points;                      ///< default points
  POINTS                              _points_type { POINTS::NORMAL };      ///< is the points structure too complex for the configuration notation?

public:

/// default constructor
  inline points_structure(void) = default;

  READ_AND_WRITE(continent_points);    ///< per-continent points
  READ_AND_WRITE(country_points);      ///< per-country points
  READ_AND_WRITE(default_points);      ///< default points
  READ_AND_WRITE(points_type);         ///< is the points structure too complex for the configuration notation?

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _continent_points
       & _country_points
       & _default_points
       & _points_type;
  }
};

// -------------------------  contest_rules  ---------------------------

/*! \class  contest_rules
    \brief  A place to maintain all the rules
        
    This object should be created and initialized early, and from that
    point it should be treated as read-only. Being paranoid, though, there's still lots of
    internal locking, since there are ongoing debates about the true thread safety of
    strings in libstc++.
*/

class contest_rules
{
protected:
    
  std::set<std::string>               _bonus_countries;         ///< countries that are eligible for bonus points

  std::set<std::string>               _callsign_mults;           ///< collection of types of mults based on callsign (e.g., "WPXPX")
  bool                                _callsign_mults_per_band;  ///< are callsign mults counted per-band?
  bool                                _callsign_mults_per_mode;  ///< are callsign mults counted per-mode?
  bool                                _callsign_mults_used;      ///< are callsign mults used?

  std::unordered_set<std::string>     _countries;                     ///< collection of canonical prefixes for all the valid countries
  std::unordered_set<std::string>     _country_mults;                 ///< collection of canonical prefixes of all the valid country multipliers
  bool                                _country_mults_per_band;        ///< are country mults counted per-band?
  bool                                _country_mults_per_mode;        ///< are country mults counted per-mode?
  bool                                _mm_country_mults;              ///< can /MM QSOs be mults?
  std::map<BAND, int>                 _per_band_country_mult_factor;  ///< factor by which to multiply number of country mults, per band

  std::vector<std::string>            _exchange_mults;           ///< names of the exchange fields that are mults, in the same order as in the configuration file
  std::vector<std::string>            _expanded_exchange_mults;  ///< expanded names of the exchange fields that are mults, in the same order as in the configuration file

  bool                                _exchange_mults_per_band;  ///< are exchange mults counted per-band?
  bool                                _exchange_mults_per_mode;  ///< are exchange mults counted per-mode?
  bool                                _exchange_mults_used;      ///< are exchange mults used?

  std::map<std::string /* exchange field name */, unsigned int>  _exchange_present_points;                                  ///< number of points if a particular exchange field is received; only one value for all bands and modes
  std::map<MODE, std::map<std::string /* canonical prefix */, std::vector<exchange_field>>> _expanded_received_exchange;    ///< details of the received exchange fields; choices expanded; key = string() is default exchange

  std::vector<BAND> _permitted_bands;                               ///< bands allowed in this contest; use a vector container in order to keep the frequency order
  std::set<MODE>    _permitted_modes;                               ///< modes allowed in this contest
  std::array<std::map<BAND, points_structure>, N_MODES> _points;    ///< points structure for each band and mode
  
  std::map<MODE, std::map<std::string /* canonical prefix */, std::vector<exchange_field>>> _received_exchange;             ///< details of the received exchange fields; choices not expanded; key = string() is default exchange
  std::map<MODE, std::map<std::string /* canonical prefix */, choice_equivalents>>          _choice_exchange_equivalents;   ///< choices

  std::map<MODE, std::vector<std::string>>    _sent_exchange_names;    ///< names of fields in the sent exchange, per mode

  bool              _work_if_different_band;     ///< is it OK to work the same station on different bands?
  bool              _work_if_different_mode;     ///< is it OK to work the same station on different modes?
  
// structures to hold information about the possible values of exchange fields

/// all the equivalent values for all exchange fields; the enclosed map is empty if there are no canonical values
  std::vector<exchange_field_values>  _exch_values;

/// all the legal values for each exchange field that has defined legal values
  std::map
    <std::string,                                          /* exch field name */
      std::set<std::string> >                   _permitted_exchange_values;  ///< all the legal values for each exchange field that has defined legal values; does not include regex

/// mapping from permitted values to canonical values
  std::map
    <std::string,                                          /* exch field name */
      std::map
        <std::string,                                      /* permitted value */
          std::string                                      /* canonical value */
           > >                                  _permitted_to_canonical;    ///< mapping from a permitted value to the corresponding canonical value

  std::map<std::string /* field name */, EFT>   _exchange_field_eft;        ///< exchange field information THIS SHOULD POSSIBLY REPLACE _permitted_exchange_values everywhere, as this supports regex

  std::map<std::string /* canonical prefix */, std::set<std::string> /* exchange field names */>  _per_country_exchange_fields;     ///< exchange fields associated with a country

// copied from context, so that we can score correctly without loading context
  std::set<BAND>                               _score_bands;            ///< bands currently used to calculate score
  std::set<BAND>                               _original_score_bands;   ///< bands that were originally used to calculate score (from the configuration file)

  std::set<MODE>                               _score_modes;            ///< modes currently used to calculate score
  std::set<MODE>                               _original_score_modes;   ///< modes that were originally used to calculate score (from the configuration file)

// my information (from context)
  std::string                                  _my_continent;        ///< my continent
  std::string                                  _my_country;          ///< canonical prefix for my country
  unsigned int                                 _my_cq_zone;          ///< CQ zone
  grid_square                                  _my_grid;             ///< Maidenhead locator
  unsigned int                                 _my_itu_zone;         ///< ITU zone

  bool                                         _send_qtcs { false };           ///< whether to send QTCs
  bool                                         _uba_bonus { false };           ///< the UBA contests have weird bonus points

/*! \brief              Private function used to obtain all the understood values for a particular exchange field
    \param  field_name  name of the field for which the understood values are required
    \return             set of all the legal values for the field <i>field_name</i>

    Uses the variable <i>_exch_values</i> to obtain the returned value
*/
  std::set<std::string> _all_exchange_values(const std::string& field_name) const;

/*! \brief                  Initialize an object that was created from the default constructor
    \param  context         context for this contest
    \param  location_db     location database

    After calling this function, the object is ready for use
*/
  void _init(const drlog_context& context, location_database& location_db);

/*! \brief                      Parse exchange line from context
    \param  exchange_fields     container of fields taken from line in configuration file
    \param  exchange_mults_vec  container of fields that are mults
    \return                     container of detailed information about each exchange field in <i>exchange_fields</i>
*/
  std::vector<exchange_field> _inner_parse(const std::vector<std::string>& exchange_fields , const std::vector<std::string>& exchange_mults_vec) const;

/*! \brief              Parse all the "exchange [xx] = " lines from context
    \param  context     drlog context
    \return             name/mult/optional/choice status for exchange fields

    Puts correct values in _received_exchange
*/
  void _parse_context_exchange(const drlog_context& context);

/*! \brief                  Parse and incorporate the "QTHX[xx] = " lines from context
    \param  context         context for this contest
    \param  location_db     location database

    Incorporates the parsed information into _exch
*/
  void _parse_context_qthx(const drlog_context& context, location_database& location_db);

/*! \brief                      Get the expected exchange fields for a particular canonical prefix
    \param  canonical_prefix    canonical prefix
    \param  m                   mode
    \param  expand_choices      whether to expand CHOICE fields
    \return                     the exchange fields associated with <i>canonical_prefix</i>
*/
  std::vector<exchange_field> _exchange_fields(const std::string& canonical_prefix, const MODE m, const bool expand_choices) const;

public:
  
/// default constructor
  contest_rules(void);

/*! \brief              Construct an object ready for use
    \param  context     context for this contest
    \param  location_db location database
*/
  contest_rules(const drlog_context& context, location_database& location_db);
  
/*! \brief                  Prepare for use an object that was created from the default constructor
    \param  context         context for this contest
    \param  location_db     location database
*/
  void prepare(const drlog_context& context, location_database& location_db);
    
/*! \brief          Add a mode to those permitted in the contest
    \param  mode    mode to add

    Does nothing if <i>mode</i> is already permitted
*/
  inline void add_permitted_mode(const MODE mode)
    { SAFELOCK(rules); _permitted_modes.insert(mode); }
    
/*! \brief                  Get the next mode in sequence
    \param  current_mode    the current mode
    \return                 the mode after <i>current_mode</i>

    Cycles through the available modes.
    Currently supports only MODE_CW and MODE_SSB
*/
  MODE next_mode(const MODE current_mode) const;

/*! \brief      Add a band to those permitted in the contest
    \param  b   band to add

    Does nothing if <i>b</i> is already permitted
*/
  void add_permitted_band(const BAND b);

/// get the next band that is higher in frequency than a given band
  BAND next_band_up(const BAND current_band) const;

/// get the next band that is lower in frequency than a given band
  BAND next_band_down(const BAND current_band) const;
  
  RULESREAD(bonus_countries);                     ///< countries that are eligible for bonus points

  RULESREAD(permitted_bands);                     ///< bands allowed in this contest
  RULESREAD(permitted_modes);                     ///< modes allowed in this contest

  RULESREAD(work_if_different_band);              ///< whether it is OK to work the same station on different bands
  RULESREAD(work_if_different_mode);              ///< whether it is OK to work the same station on different modes

/*! \brief                      Get the expected exchange fields for a particular canonical prefix
    \param  canonical_prefix    canonical prefix
    \param  m                   mode
    \return                     the exchange fields associated with <i>canonical_prefix</i>

    CHOICE fields ARE NOT expanded
*/
  inline std::vector<exchange_field> unexpanded_exch(const std::string& canonical_prefix, const MODE m) const
    { return _exchange_fields(canonical_prefix, m, false); }

/*! \brief                      Get the expected exchange fields for a particular canonical prefix
    \param  canonical_prefix    canonical prefix
    \param  m                   mode
    \return                     the exchange fields associated with <i>canonical_prefix</i>

    CHOICE fields ARE expanded
*/
  inline std::vector<exchange_field> expanded_exch(const std::string& canonical_prefix, const MODE m) const
    { return _exchange_fields(canonical_prefix, m, true); }

  RULESREAD(callsign_mults);                      ///< collection of types of mults based on callsign (e.g., "WPXPX")
  RULESREAD(callsign_mults_per_band);             ///< are callsign mults counted per-band?
  RULESREAD(callsign_mults_per_mode);             ///< are callsign mults counted per-mode?
  RULESREAD(callsign_mults_used);                 ///< are callsign mults used?

  RULESREAD(country_mults);                       ///< collection of canonical prefixes of country multipliers
  RULESREAD(country_mults_per_band);              ///< are country mults counted per-band?
  RULESREAD(country_mults_per_mode);              ///< are country mults counted per-mode?
  RULESREAD(mm_country_mults);                    ///< can /MM stations be country mults?

  RULESREAD(exchange_mults);                      ///< the exchange multipliers, in the same order as in the configuration file
  RULESREAD(exchange_mults_per_band);             ///< are exchange mults counted per-band?
  RULESREAD(exchange_mults_per_mode);             ///< are exchange mults counted per-mode?
  RULESREAD(exchange_mults_used);                 ///< are exchange mults used?
  RULESREAD(expanded_exchange_mults);             ///< expanded exchange multipliers

  RULESREAD(original_score_bands);                ///< bands that were originally used to calculate score (from the configuration file)
  RULESREAD(original_score_modes);                ///< modes that were originally used to calculate score (from the configuration file)

  RULESREAD(per_band_country_mult_factor);        ///< factor by which to multiply number of country mults, per band (see WAE rules)

  RULESREAD(score_bands);                         ///< bands currently used to calculate score
  RULESREAD(score_modes);                         ///< modes currently used to calculate score
  RULESREAD(send_qtcs);                           ///< can QTCs be sent?

  RULESREAD(uba_bonus);                           ///< do we have bonus points for ON stations?

  RULESREAD(exchange_field_eft);                  ///< exchange field information

/*! \brief              The exchange field template corresponding to a particular field
    \param  field_name  name of the field
    \return             The exchange field information associated with <i>field_name</i>

    Returns EFT("none") if <i>field_name</i> is unknown.
*/
  EFT exchange_field_eft(const std::string& field_name) const;

/*! \brief                      Get the expanded names of the exchange fields for a particular canonical prefix and mode
    \param  canonical_prefix    canonical prefix
    \param  m                   mode
    \return                     the exchange field names associated with <i>canonical_prefix</i> and <i>m</i>
*/
  std::vector<std::string> expanded_exchange_field_names(const std::string& canonical_prefix, const MODE m) const;

/*! \brief                      Get the unexpanded names of the exchange fields for a particular canonical prefix and mode
    \param  canonical_prefix    canonical prefix
    \param  m                   mode
    \return                     the exchange field names associated with <i>canonical_prefix</i> and <i>m</i>
*/
  std::vector<std::string> unexpanded_exchange_field_names(const std::string& canonical_prefix, const MODE m) const;

/// Return all the known names of exchange fields
  std::set<std::string> all_known_field_names(void) const;

/// Restore the original set of bands to be scored (from the configuration file)
  inline void restore_original_score_bands(void)
    { SAFELOCK(rules);

      _score_bands = _original_score_bands;
    }

/// Restore the original set of modes to be scored (from the configuration file)
  inline void restore_original_score_modes(void)
    { SAFELOCK(rules);

      _score_modes = _original_score_modes;
    }

/*! \brief              Define a new set of bands to be scored
    \param  new_bands   the set of bands to be scored

    Does nothing if <i>new_bands</i> is empty
*/
  void score_bands(const std::set<BAND>& new_bands);

/*! \brief              Define a new set of modes to be scored
    \param  new_modes   the set of modes to be scored

    Does nothing if <i>new_modes</i> is empty
*/
  void score_modes(const std::set<MODE>& new_modes);

/*! \brief      Do the country mults (if any) include a particular country?
    \param  cp  canonical prefix of country to test
    \return     whether cp is a country mult

    If <i>cp</i> is empty, then tests whether any countries are mults.
*/
  bool country_mults_used(const std::string& cp = std::string()) const;

/*! \brief      Do the country mults (if any) include a particular country?
    \param  cp  canonical prefix of country to test
    \return     whether cp is a country mult
*/
  inline bool is_country_mult(const std::string& cp) const
    { return country_mults_used(cp); }

/*! \brief          Is an exchange field a mult?
    \param  name    name of exchange field
    \return         whether <i>name</i> is an exchange mult

    Returns <i>false</i> if <i>name</i> is unrecognised
*/
  bool is_exchange_mult(const std::string& name) const;

/*! \brief              All the canonical values for a particular exchange field
    \param  field_name  name of an exchange field (received)
    \return             all the canonical values for the exchange field <i>field_name</i>

    Returns empty vector if no acceptable values are found (e.g., RST, RS, SERNO)
*/
  std::vector<std::string> exch_canonical_values(const std::string& field_name) const;

/*! \brief              The permitted values for a particular exchange field
    \param  field_name  name of an exchange field (received)
    \return             all the permitted values for the exchange field <i>field_name</i>

    Returns empty set if the field can take any value, or if it's a regex.
*/
  std::set<std::string> exch_permitted_values(const std::string& field_name) const;

/*! \brief              Is a particular exchange field limited to only permitted values?
    \param  field_name  name of an exchange field (received)
    \return             whether field <i>field_name</i> has permitted values.

    Generally (perhaps always) this should be the opposite of <i>exchange_field_is_regex(field_name)</i>
*/
  inline bool exch_has_permitted_values(const std::string& field_name) const
    { SAFELOCK(rules);

      return ( _permitted_exchange_values.find(field_name) != _permitted_exchange_values.cend() );
    }

/*! \brief              Is a particular exchange field a regex?
    \param  field_name  name of an exchange field (received)
    \return             whether field <i>field_name</i> is a regex field

    Returns <i>false</i> if <i>field_name</i> is unknown.
*/
  bool exchange_field_is_regex(const std::string& field_name) const;

/*! \brief                  A canonical value
    \param  field_name      name of an exchange field (received)
    \param  actual_value    received value of the field
    \return                 the canonical value of the field <i>field_name</i> associated with the value <i>actual_value</i>

    Returns the received value if there are no canonical values
*/
  std::string canonical_value(const std::string& field_name, const std::string& actual_value) const;

/*! \brief                          Add a canonical value for a particular exchange field
    \param  field_name              name of an exchange field (received)
    \param  new_canonical_value     the canonical value to add

    Also adds <i>new_canonical_value</i> to the legal values for the field <i>field_name</i>. Does nothing
    if <i>new_canonical_value</i> is already a canonical value.
*/
  void add_exch_canonical_value(const std::string& field_name, const std::string& new_canonical_value);

/*! \brief                              Is a particular string the canonical value for a particular exchange field?
    \param  field_name                  name of an exchange field (received)
    \param  putative_canonical_value    the value to check
    \return                             whether <i>putative_canonical_value</i> is an actual canonical value for the field <i>field_name</i>

    Returns false if <i>field_name</i> is unrecognized
*/
  bool is_canonical_value(const std::string& field_name, const std::string& putative_canonical_value) const;

/*! \brief                  Is a particular string a legal value for a particular exchange field?
    \param  field_name      name of an exchange field (received)
    \param  putative_value  the value to check
    \return                 whether <i>putative_value</i> is a legal value for the field <i>field_name</i>

    Returns false if <i>field_name</i> is unrecognized. Supports regex exchanges.
*/
  bool is_legal_value(const std::string& field_name, const std::string& putative_value) const;

/// number of permitted bands
  inline unsigned int n_bands(void) const
    { SAFELOCK(rules); return _permitted_bands.size(); }

/// number of permitted modes
  inline unsigned int n_modes(void) const
    { SAFELOCK(rules); return _permitted_modes.size(); }

/// do we allow multiple bands?
  inline bool multiple_bands(void) const
    { return (n_bands() != 1); }

/// do we allow multiple modes?
  inline bool multiple_modes(void) const
    { return (n_modes() != 1); }

/// number of country mults
  inline unsigned int n_country_mults(void) const
    { SAFELOCK(rules); return _country_mults.size(); }
    
/*! \brief                  Points for a particular QSO
    \param  qso             QSO for which the points are to be calculated
    \param  location_db     location database
    \return                 the (location-based) points for <i>qso</i>
*/
  unsigned int points(const QSO& qso, location_database& location_db) const;

/// human-readable string
  std::string to_string(void) const;

/*! \brief          Does the sent exchange include a particular field?
    \param  str     name of field to test
    \param  m       mode
    \return         whether the sent exchange for mode <i>m</i> includes a field with the name <i>str</i>
*/
  bool sent_exchange_includes(const std::string& str, const MODE m) const;

/// get the permitted bands as a set
  inline std::set<BAND> permitted_bands_set(void) const
    { return std::set<BAND>(_permitted_bands.cbegin(), _permitted_bands.cend() ); }

/*! \brief                      Is a particular field used for QSOs with a particular country?
    \param  field_name          name of exchange field to test
    \param  canonical_prefix    country to test
    \return                     whether the field <i>field_name</i> is used when the country's canonical prefix is <i>canonical_prefix</i>
*/
  bool is_exchange_field_used_for_country(const std::string& field_name, const std::string& canonical_prefix) const;

/// the names of all the possible exchange fields
  std::set<std::string> exchange_field_names(void) const;

/*! \brief      The equivalent choices of exchange fields for a given mode and country?
    \param  m   mode
    \param  cp  canonical prefix of country
    \return     all the equivalent firlds for mode <i>m</i> and country <i>cp</i>
*/
  choice_equivalents equivalents(const MODE m, const std::string& cp) const;

/// read from and write to disk
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(rules);

      ar & _bonus_countries
         & _callsign_mults
         & _callsign_mults_per_band
         & _callsign_mults_per_mode
         & _callsign_mults_used
         & _choice_exchange_equivalents
         & _countries
         & _country_mults
         & _country_mults_per_band
         & _country_mults_per_mode
         & _exchange_field_eft
         & _exchange_mults
         & _exchange_mults_per_band
         & _exchange_mults_per_mode
         & _exchange_mults_used
         & _exchange_present_points
         & _exch_values
         & _expanded_received_exchange
         & _my_continent
         & _my_country
         & _my_cq_zone
         & _my_grid
         & _my_itu_zone
         & _original_score_bands
         & _original_score_modes
         & _per_band_country_mult_factor
         & _permitted_bands
         & _permitted_exchange_values
         & _permitted_modes
         & _permitted_to_canonical
         & _per_country_exchange_fields
         & _points
         & _received_exchange
         & _score_bands
         & _score_modes
         & _send_qtcs
         & _sent_exchange_names
         & _uba_bonus
         & _work_if_different_band
         & _work_if_different_mode;
    }
};

/*! \brief          Return the WPX prefix of a call
    \param  call    callsign for which the WPX prefix is desired
    \return         the WPX prefix corresponding to <i>call</i>
*/
std::string wpx_prefix(const std::string& call);

/*! \brief          The SAC prefix for a particular call
    \param  call    call for which the prefix is to be calculated
    \return         the SAC prefix corresponding to <i>call</i>

    The SAC rules as written do not allow for weird prefixes such as LA100, etc.
*/
std::string sac_prefix(const std::string& call);

/*! \brief                  Given a received value of a particular multiplier field, what is the actual mult value?
    \param  field_name      name of the field
    \param  received_value  received value for field <i>field_name</i>
    \return                 the multiplier value for the field <i>field_name</i>

    For example, the mult value in WAG for a DOK field with the value A01 is A.
*/
std::string MULT_VALUE(const std::string& field_name, const std::string& received_value);

#endif    // RULES_H

