// $Id: cabrillo.cpp 193 2021-10-03 20:05:48Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   cabrillo.cpp

    Classes and functions related to the Cabrillo format defined at:
    http://www.kkn.net/~trey/cabrillo/

    Just for the record, the "specification" at the above URL is grossly incomplete
    (even though the ARRL at http://www.arrl.org/contests/fileform.html
    explicitly claims that it is complete). As a result, there are several places
    where one has to guess what might be intended.

    The URL for the "specification" is now:
      http://wwrof.org/cabrillo/

    The version number at this site claims to be version 3; but it's different (but,
    in general, no less ambiguous) than the "version 3" that was at the original site.
*/

#include "cabrillo.h"
#include "macros.h"
#include "string_functions.h"

#include <fstream>
#include <iostream>

using namespace std;

extern ofstream            ost;                   ///< for debugging, info

// -----------  cabrillo_tag_template  ----------------

/*! \class  cabrillo_tag_template
    \brief  template for a cabrillo tag
*/

/*! \brief      Initialise the name from a string
    \param  nm  tag name
*/
void cabrillo_tag_template::_init_from_string(string_view str)
{ if (const auto posn { str.find(':') }; posn == string_view::npos)                             // no values included
    _name = str;
  else                                                          // one or more values are included
  { _name = str.substr(0, posn);
    
    string_view values { str.substr(posn + 1) };

    const vector<string_view> vec { remove_peripheral_spaces <std::string_view> (split_string <std::string_view> (values, ',')) };

    FOR_ALL(vec, [this] (auto str) { this->add_legal_value(string { str }); });
  }
}

#if 0
void cabrillo_tag_template::_init_from_string(const string& str)
{ if (const auto posn { str.find(':') }; posn == string::npos)                             // no values included
    _name = str;
  else                                                          // one or more values are included
  { _name = str.substr(0, posn);
    
    const string         values { str.substr(posn + 1) };
    const vector<string> vec    { remove_peripheral_spaces <std::string> (split_string <std::string> (values, ',')) };

    FOR_ALL(vec, [this] (const string& str) { this->add_legal_value(str); });
  }
}
#endif

// -----------  cabrillo_tag_templatess  ----------------

/*! \class  cabrillo_tag_templates
    \brief  all the cabrillo tag templates
*/

/// default constructor
cabrillo_tag_templates::cabrillo_tag_templates(void)
{ 
// START-OF-LOG
  _add("START-OF-LOG"s);

// ADDRESS (up to four of these are allowed)
  _add("ADDRESS"s);

// ADDRESS-CITY
  _add("ADDRESS-CITY"s);

// ADDRESS-COUNTRY
  _add("ADDRESS-COUNTRY"s);
  
// ADDRESS-POSTALCODE
  _add("ADDRESS-POSTALCODE"s);
  
// ADDRESS-STATE-PROVINCE
  _add("ADDRESS-STATE-PROVINCE"s);
  
// CALLSIGN
  _add("CALLSIGN"s);
  
// CATEGORY-ASSISTED
  _add("CATEGORY-ASSISTED:ASSISTED,NON-ASSISTED"s);
  
// CATEGORY-BAND
  _add("CATEGORY-BAND:ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light"s);
  
// CATEGORY-MODE; there appears to be a typo in the definition at http://www.kkn.net/~trey/cabrillo/tags.html,
// which has "MIX ED", rather than "MIXED"
  _add("CATEGORY-MODE:CW,MIXED,RTTY,SSB"s);
  
// CATEGORY-OPERATOR
  _add("CATEGORY-OPERATOR:CHECKLOG,MULTI-OP,SINGLE-OP"s);

// CATEGORY-OVERLAY
  _add("CATEGORY-OVERLAY:NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES"s);
  
// CATEGORY-POWER
  _add("CATEGORY-POWER:HIGH,LOW,QRP"s);

// CATEGORY-STATION
  _add("CATEGORY-STATION:EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL"s);

// CATEGORY-TIME
  _add("CATEGORY-TIME:6-HOURS,12-HOURS,24-HOURS"s);

// CATEGORY-TRANSMITTER
  _add("CATEGORY-TRANSMITTER:LIMITED,ONE,SWL,TWO,UNLIMITED"s);

// CERTIFICATE; the specification says: "The contest sponsor may or may not honor this tag, and if so may or may not use opt-in or opt-out
// as the default. YES is the default." Absent is any explanation of the bizarre sequence of words: "may or may not use opt-in or opt-out".
// There is no explanation of how a default value can have any meaning if the recipient can interpret the lack of an explicit
// entry as meaning either YES or NO.
  _add("CERTIFICATE:YES,NO"s);

// CLAIMED-SCORE
  _add("CLAIMED-SCORE"s);

// CLUB
  _add("CLUB"s);
  
// CONTEST
// the specification has been changed to say: "Note: Contest sponsors may create their own contest values".
// This chage was made WITHOUT CHANGING THE VERSION NUMBER OF THE SPECIFICATION
// And, of course, it makes a mockery of the values that are in the specification
  _add("CONTEST"s);

// CREATED-BY
  _add("CREATED-BY"s);

// EMAIL (sic)
  _add("EMAIL"s);
  
// END-OF-LOG
  _add("END-OF-LOG"s);

// LOCATION
  _add("LOCATION"s);

// NAME
  _add("NAME"s);

// OFFTIME
  _add("OFFTIME"s);
  
// OPERATORS
  _add("OPERATORS"s);

// QSO
  _add("QSO"s);
  
// SOAPBOX
  _add("SOAPBOX"s);
}

/// Instantiation of all the templates
cabrillo_tag_templates cabrillo_tags;
