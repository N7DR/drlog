// $Id: cabrillo.h 41 2013-11-09 13:23:18Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef CABRILLO_H
#define CABRILLO_H

/*!     \file cabrillo.h

        Classes and functions related to the Cabrillo format "specified" at:
        http://www.kkn.net/~trey/cabrillo/
        
        Just for the record, the "specification" at the above URL is grossly incomplete
        (even though the ARRL at http://www.arrl.org/contests/fileform.html
        explicitly claims that they are complete. As a result, there are several places 
        where I have to guess what is intended. 
*/

#include <string>
#include <vector>

// -----------  cabrillo_tag_template  ----------------

/*!     \class cabrillo_tag_template
        \brief template for a cabrillo tag
*/

class cabrillo_tag_template
{
protected:
  std::string                 _name;            ///< name of the tag
  std::vector<std::string>    _legal_values;    ///< allowed values; if empty then any text is permitted
  
public:
  
/*!     \brief  construct from name
        \param  nm      tag name
        
        Any tag value is legal for this tag
*/
  explicit cabrillo_tag_template(const std::string& nm);
    
/// destructor
  virtual ~cabrillo_tag_template(void)
  { }

/// cabrillo_tag_template = string
  void operator=(const std::string& nm);
  
/// add a legal value
  inline void add_legal_value(const std::string& val)
    { _legal_values.push_back(val); }
};

// -----------  cabrillo_tag_templatess  ----------------

/*!     \class cabrillo_tag_templates
        \brief all the cabrillo tag templates
*/

class cabrillo_tag_templates
{
protected:
  
// there aren't many different kinds of tags, so a vector will suffice  
  std::vector<cabrillo_tag_template>        _templates;    ///< container for the templates
  
public:
  
/// default constructor
  cabrillo_tag_templates(void);
    
/// destructor
  virtual ~cabrillo_tag_templates(void)
  { }  
};

/// Instantiation of all the templates
extern cabrillo_tag_templates cabrillo_tags;

#endif    // CABRILLO_H
