// $Id: cabrillo.cpp 88 2014-12-27 15:19:42Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file cabrillo.cpp

        Classes and functions related to the Cabrillo format defined at:
        http://www.kkn.net/~trey/cabrillo/
*/

#include "cabrillo.h"
#include "macros.h"
#include "string_functions.h"

#include <fstream>
#include <iostream>

using namespace std;

extern ofstream            ost;                   ///< for debugging, info

// -----------  cabrillo_tag_template  ----------------

/*!     \class cabrillo_tag_template
        \brief template for a cabrillo tag
*/

/*!     \brief  construct from name
        \param  nm      tag name
        
        Any tag value is legal for this tag
*/
cabrillo_tag_template::cabrillo_tag_template(const string& nm)
{ if (nm.find(":") == string::npos)                              // no values included
    _name = nm;
  else
  { _name = nm.substr(0, nm.find(":"));
    
// one or more values are included
    const string values = nm.substr(nm.find(":") + 1);
    const vector<string> vec = remove_peripheral_spaces(split_string(values, ","));

//    for_each(vec.cbegin(), vec.cend(), [&] (const string& str) { this->add_legal_value(str); });
    FOR_ALL(vec, [&] (const string& str) { this->add_legal_value(str); });
  }
}

/// cabrillo_tag_template = string
void cabrillo_tag_template::operator=(const string& nm)
{ if (nm.find(":") == string::npos)                              // no values included
    _name = nm;
  else
// one or more values are included
  { const string values = substring(nm, nm.find(":") + 1);
    const vector<string> vec = remove_peripheral_spaces(split_string(values, ","));

//    for_each(vec.cbegin(), vec.cend(), [&] (const string& str) { this->add_legal_value(str); });
    FOR_ALL(vec, [&] (const string& str) { this->add_legal_value(str); });
  }
}

// -----------  cabrillo_tag_templatess  ----------------

/*!     \class cabrillo_tag_templates
        \brief all the cabrillo tag templates
*/

/// default constructor
cabrillo_tag_templates::cabrillo_tag_templates(void)
{ 
// START-OF-LOG
//  cabrillo_tag_template tag("START-OF-LOG");
  
//  _templates.push_back(tag);
  _add("START-OF-LOG");

// ADDRESS (up to four of these are allowed)
//  tag = "ADDRESS";
  _add("ADDRESS");

// ADDRESS-CITY
//  tag = "ADDRESS-CITY";
  _add("ADDRESS-CITY");

// ADDRESS-COUNTRY
//  tag = "ADDRESS-COUNTRY";
  _add("ADDRESS-COUNTRY");
  
// ADDRESS-POSTALCODE
//  tag = "ADDRESS-POSTALCODE";
  _add("ADDRESS-POSTALCODE");
  
// ADDRESS-STATE-PROVINCE
//  tag = "ADDRESS-STATE-PROVINCE";
  _add("ADDRESS-STATE-PROVINCE");
  
// CALLSIGN
//  tag = "CALLSIGN";
  _add("CALLSIGN");
  
// CATEGORY-ASSISTED
//  tag = "CATEGORY-ASSISTED:ASSISTED,NON-ASSISTED";
  _add("CATEGORY-ASSISTED:ASSISTED,NON-ASSISTED");
  
// CATEGORY-BAND
//  tag = "CATEGORY-BAND:ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light";
  _add("CATEGORY-BAND:ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light");
  
// CATEGORY-MODE; there appears to be a typo in the definition at http://www.kkn.net/~trey/cabrillo/tags.html,
// which has "MIX ED", rather than "MIXED"
//  tag = "CATEGORY-MODE:CW,MIXED,RTTY,SSB";
  _add("CATEGORY-MODE:CW,MIXED,RTTY,SSB");
  
// CATEGORY-OPERATOR
//  tag = "CATEGORY-OPERATOR:CHECKLOG,MULTI-OP,SINGLE-OP";
  _add("CATEGORY-OPERATOR:CHECKLOG,MULTI-OP,SINGLE-OP");

// CATEGORY-OVERLAY
//  tag = "CATEGORY-OVERLAY:NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES";
  _add("CATEGORY-OVERLAY:NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES");
  
// CATEGORY-POWER
//  tag = "CATEGORY-POWER:HIGH,LOW,QRP";
  _add("CATEGORY-POWER:HIGH,LOW,QRP");

// CATEGORY-STATION
//  tag = "CATEGORY-STATION:EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL";
  _add("CATEGORY-STATION:EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL");

// CATEGORY-TIME
//  tag = "CATEGORY-TIME:6-HOURS,12-HOURS,24-HOURS";
  _add("CATEGORY-TIME:6-HOURS,12-HOURS,24-HOURS");

// CATEGORY-TRANSMITTER
//  tag = "CATEGORY-TRANSMITTER:LIMITED,ONE,SWL,TWO,UNLIMITED";
  _add("CATEGORY-TRANSMITTER:LIMITED,ONE,SWL,TWO,UNLIMITED");

// CERTIFICATE
//  tag = "CERTIFICATE:YES,NO";
  _add("CERTIFICATE:YES,NO");

// CLAIMED-SCORE
//  tag = "CLAIMED-SCORE";
  _add("CLAIMED-SCORE");

// CLUB
//  tag = "CLUB";
  _add("CLUB");
  
// CONTEST
//  tag = "CONTEST:AP-SPRINT,ARRL-10,ARRL-160,ARRL-DX-CW,ARRL-DX-SSB,ARRL-SS-CW,ARRL-SS-SSB,ARRL-UHF-AUG,ARRL-VHF-JAN,ARRL-VHF-JUN,ARRL-VHF-SEP,ARRL-RTTY,BARTG-RTTY,CQ-160-CW,CQ-160-SSB,CQ-WPX-CW,CQ-WPX-RTTY,CQ-WPX-SSB,CQ-VHF,CQ-WW-CW,CQ-WW-RTTY,CQ-WW-SSB,DARC-WAEDC-CW,DARC-WAEDC-RTTY,DARC-WAEDC-SSB,FCG-FQP,IARU-HF,JIDX-CW,JIDX-SSB,NAQP-CW,NAQP-RTTY,NAQP-SSB,NA-SPRINT-CW,NA-SPRINT-SSB,NCCC-CQP,NEQP,OCEANIA-DX-CW,OCEANIA-DX-SSB,RDXC,RSGB-IOTA,SAC-CW,SAC-SSB,STEW-PERRY,TARA-RTTY";
  _add("CONTEST:AP-SPRINT,ARRL-10,ARRL-160,ARRL-DX-CW,ARRL-DX-SSB,ARRL-SS-CW,ARRL-SS-SSB,ARRL-UHF-AUG,ARRL-VHF-JAN,ARRL-VHF-JUN,ARRL-VHF-SEP,ARRL-RTTY,BARTG-RTTY,CQ-160-CW,CQ-160-SSB,CQ-WPX-CW,CQ-WPX-RTTY,CQ-WPX-SSB,CQ-VHF,CQ-WW-CW,CQ-WW-RTTY,CQ-WW-SSB,DARC-WAEDC-CW,DARC-WAEDC-RTTY,DARC-WAEDC-SSB,FCG-FQP,IARU-HF,JIDX-CW,JIDX-SSB,NAQP-CW,NAQP-RTTY,NAQP-SSB,NA-SPRINT-CW,NA-SPRINT-SSB,NCCC-CQP,NEQP,OCEANIA-DX-CW,OCEANIA-DX-SSB,RDXC,RSGB-IOTA,SAC-CW,SAC-SSB,STEW-PERRY,TARA-RTTY");

// CREATED-BY
//  tag = "CREATED-BY";
  _add("CREATED-BY");

// EMAIL (sic)
//  tag = "EMAIL";
  _add("EMAIL");
  
// END-OF-LOG
//  tag = "END-OF-LOG";
  _add("END-OF-LOG");

// LOCATION
//  tag = "LOCATION";
  _add("LOCATION");

// NAME
//  tag = "NAME";
  _add("NAME");

// OFFTIME
//  tag = "OFFTIME";
  _add("OFFTIME");
  
// OPERATORS
//  tag = "OPERATORS";
  _add("OPERATORS");

// QSO
//  tag = "QSO";
  _add("QSO");
  
// SOAPBOX
//  tag = "SOAPBOX";
  _add("SOAPBOX");
}

/// Instantiation of all the templates
cabrillo_tag_templates cabrillo_tags;
