// $Id: cabrillo.cpp 115 2015-08-29 15:44:57Z  $

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

/*! \brief      Construct from name
    \param  nm  tag name
        
    Any tag value is legal for this tag
*/
cabrillo_tag_template::cabrillo_tag_template(const string& nm)
{ if (nm.find(":") == string::npos)                             // no values included
    _name = nm;
  else                                                          // one or more values are included
  { _name = nm.substr(0, nm.find(":"));
    
    const string values = nm.substr(nm.find(":") + 1);
    const vector<string> vec = remove_peripheral_spaces(split_string(values, ","));

    FOR_ALL(vec, [&] (const string& str) { this->add_legal_value(str); });
  }
}

/// cabrillo_tag_template = string
void cabrillo_tag_template::operator=(const string& nm)
{ if (nm.find(":") == string::npos)                             // no values included
    _name = nm;
  else                                                          // one or more values are included
  { const string values = substring(nm, nm.find(":") + 1);
    const vector<string> vec = remove_peripheral_spaces(split_string(values, ","));

    FOR_ALL(vec, [&] (const string& str) { this->add_legal_value(str); });
  }
}

// -----------  cabrillo_tag_templatess  ----------------

/*! \class  cabrillo_tag_templates
    \brief  all the cabrillo tag templates
*/

/// default constructor
cabrillo_tag_templates::cabrillo_tag_templates(void)
{ 
// START-OF-LOG
  _add("START-OF-LOG");

// ADDRESS (up to four of these are allowed)
  _add("ADDRESS");

// ADDRESS-CITY
  _add("ADDRESS-CITY");

// ADDRESS-COUNTRY
  _add("ADDRESS-COUNTRY");
  
// ADDRESS-POSTALCODE
  _add("ADDRESS-POSTALCODE");
  
// ADDRESS-STATE-PROVINCE
  _add("ADDRESS-STATE-PROVINCE");
  
// CALLSIGN
  _add("CALLSIGN");
  
// CATEGORY-ASSISTED
  _add("CATEGORY-ASSISTED:ASSISTED,NON-ASSISTED");
  
// CATEGORY-BAND
  _add("CATEGORY-BAND:ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light");
  
// CATEGORY-MODE; there appears to be a typo in the definition at http://www.kkn.net/~trey/cabrillo/tags.html,
// which has "MIX ED", rather than "MIXED"
  _add("CATEGORY-MODE:CW,MIXED,RTTY,SSB");
  
// CATEGORY-OPERATOR
  _add("CATEGORY-OPERATOR:CHECKLOG,MULTI-OP,SINGLE-OP");

// CATEGORY-OVERLAY
  _add("CATEGORY-OVERLAY:NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES");
  
// CATEGORY-POWER
  _add("CATEGORY-POWER:HIGH,LOW,QRP");

// CATEGORY-STATION
  _add("CATEGORY-STATION:EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL");

// CATEGORY-TIME
  _add("CATEGORY-TIME:6-HOURS,12-HOURS,24-HOURS");

// CATEGORY-TRANSMITTER
  _add("CATEGORY-TRANSMITTER:LIMITED,ONE,SWL,TWO,UNLIMITED");

// CERTIFICATE; the specification says: "The contest sponsor may or may not honor this tag, and if so may or may not use opt-in or opt-out
// as the default. YES is the default." Absent is any explanation of the bizarre sequence of words: "may or may not use opt-in or opt-out".
// There is no explanation of how a default value can have any meaning if the recipient can interpret the lack of an explicit
// entry as meaning either YES or NO.
  _add("CERTIFICATE:YES,NO");

// CLAIMED-SCORE
  _add("CLAIMED-SCORE");

// CLUB
  _add("CLUB");
  
// CONTEST
// the specification has been changed to say: "Note: Contest sponsors may create their own contest values".
// This chage was made WITHOUT CHANGING THE VERSION NUMBER OF THE SPECIFICATION
// And, of course, it makes a mockery of the values that are in the specification
  _add("CONTEST");

// CREATED-BY
  _add("CREATED-BY");

// EMAIL (sic)
  _add("EMAIL");
  
// END-OF-LOG
  _add("END-OF-LOG");

// LOCATION
  _add("LOCATION");

// NAME
  _add("NAME");

// OFFTIME
  _add("OFFTIME");
  
// OPERATORS
  _add("OPERATORS");

// QSO
  _add("QSO");
  
// SOAPBOX
  _add("SOAPBOX");
}

/// Instantiation of all the templates
cabrillo_tag_templates cabrillo_tags;
