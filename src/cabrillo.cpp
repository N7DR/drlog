// $Id: cabrillo.cpp 52 2014-03-01 16:17:18Z  $

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
#include "string_functions.h"

#include <fstream>
#include <iostream>

using namespace std;

// used only for debugging
extern ofstream            ost;

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

    for_each(vec.cbegin(), vec.cend(), [&] (const string& str) { this->add_legal_value(str); });
  }
}

/// cabrillo_tag_template = string
void cabrillo_tag_template::operator=(const string& nm)
{ if (nm.find(":") == string::npos)                              // no values included
    _name = nm;
  else
// one or more values are included
  { const string values = nm.substr(nm.find(":") + 1);
    const vector<string> vec = remove_peripheral_spaces(split_string(values, ","));

    for_each(vec.cbegin(), vec.cend(), [&] (const string& str) { this->add_legal_value(str); });
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
  cabrillo_tag_template tag("START-OF-LOG");
  
  _templates.push_back(tag);

// ADDRESS (up to four of these are allowed)
  tag = "ADDRESS";
  _templates.push_back(tag);

// ADDRESS-CITY
  tag = "ADDRESS-CITY";
  _templates.push_back(tag);  

// ADDRESS-COUNTRY
  tag = "ADDRESS-COUNTRY";
  _templates.push_back(tag);  
  
// ADDRESS-POSTALCODE
  tag = "ADDRESS-POSTALCODE";
  _templates.push_back(tag);  
  
// ADDRESS-STATE-PROVINCE
  tag = "ADDRESS-STATE-PROVINCE";
  _templates.push_back(tag);  
  
// CALLSIGN
  tag = "CALLSIGN";
  _templates.push_back(tag);  
  
// CATEGORY-ASSISTED
  tag = "CATEGORY-ASSISTED:ASSISTED,NON-ASSISTED";
  _templates.push_back(tag);
  
// CATEGORY-BAND
  tag = "CATEGORY-BAND:ALL,160M,80M,40M,20M,15M,10M,6M,2M,222,432,902,1.2G,2.3G,3.4G,5.7G,10G,24G,47G,75G,119G,142G,241G,Light"; 
  _templates.push_back(tag);
  
// CATEGORY-MODE; there appears to be a typo in the definition at http://www.kkn.net/~trey/cabrillo/tags.html,
// which has "MIX ED", rather than "MIXED"
  tag = "CATEGORY-MODE:CW,MIXED,RTTY,SSB"; 
  _templates.push_back(tag);
  
// CATEGORY-OPERATOR
  tag = "CATEGORY-OPERATOR:CHECKLOG,MULTI-OP,SINGLE-OP"; 
  _templates.push_back(tag);  

// CATEGORY-OVERLAY
  tag = "CATEGORY-OVERLAY:NOVICE-TECH,OVER-50,ROOKIE,TB-WIRES"; 
  _templates.push_back(tag);  
  
// CATEGORY-POWER
  tag = "CATEGORY-POWER:HIGH,LOW,QRP"; 
  _templates.push_back(tag);    

// CATEGORY-STATION
  tag = "CATEGORY-STATION:EXPEDITION,FIXED,HQ,MOBILE,PORTABLE,ROVER,SCHOOL"; 
  _templates.push_back(tag);   

// CATEGORY-TIME
  tag = "CATEGORY-TIME:6-HOURS,12-HOURS,24-HOURS"; 
  _templates.push_back(tag);    

// CATEGORY-TRANSMITTER
  tag = "CATEGORY-TRANSMITTER:LIMITED,ONE,SWL,TWO,UNLIMITED";   
  _templates.push_back(tag);

// CERTIFICATE
  tag = "CERTIFICATE:YES,NO";
  _templates.push_back(tag);

// CLAIMED-SCORE
  tag = "CLAIMED-SCORE";
  _templates.push_back(tag);  

// CLUB
  tag = "CLUB";
  _templates.push_back(tag); 
  
// CONTEST
  tag = "CONTEST:AP-SPRINT,ARRL-10,ARRL-160,ARRL-DX-CW,ARRL-DX-SSB,ARRL-SS-CW,ARRL-SS-SSB,ARRL-UHF-AUG,ARRL-VHF-JAN,ARRL-VHF-JUN,ARRL-VHF-SEP,ARRL-RTTY,BARTG-RTTY,CQ-160-CW,CQ-160-SSB,CQ-WPX-CW,CQ-WPX-RTTY,CQ-WPX-SSB,CQ-VHF,CQ-WW-CW,CQ-WW-RTTY,CQ-WW-SSB,DARC-WAEDC-CW,DARC-WAEDC-RTTY,DARC-WAEDC-SSB,FCG-FQP,IARU-HF,JIDX-CW,JIDX-SSB,NAQP-CW,NAQP-RTTY,NAQP-SSB,NA-SPRINT-CW,NA-SPRINT-SSB,NCCC-CQP,NEQP,OCEANIA-DX-CW,OCEANIA-DX-SSB,RDXC,RSGB-IOTA,SAC-CW,SAC-SSB,STEW-PERRY,TARA-RTTY";
  _templates.push_back(tag);  

// CREATED-BY
  tag = "CREATED-BY";
  _templates.push_back(tag);  

// EMAIL (sic)
  tag = "EMAIL";
  _templates.push_back(tag);    
  
// END-OF-LOG
  tag = "END-OF-LOG";
  _templates.push_back(tag);

// LOCATION
  tag = "LOCATION";
  _templates.push_back(tag);  

// NAME
  tag = "NAME";
  _templates.push_back(tag);

// OFFTIME
  tag = "OFFTIME";
  _templates.push_back(tag);  
  
// OPERATORS
  tag = "OPERATORS";
  _templates.push_back(tag);

// QSO
  tag = "QSO";
  _templates.push_back(tag);  
  
// SOAPBOX
  tag = "SOAPBOX";
  _templates.push_back(tag);  
}

/// Instantiation of all the templates
cabrillo_tag_templates cabrillo_tags;
