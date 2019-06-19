// $Id: cabrillo.h 146 2018-04-09 19:19:15Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef CABRILLO_H
#define CABRILLO_H

/*! \file   cabrillo.h

    Classes and functions related to the Cabrillo format "specified" at:
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

#include <string>
#include <vector>

class cabrillo_tag_templates;                   ///< forward declaration

extern cabrillo_tag_templates cabrillo_tags;    ///< instantiation of all the templates

// -----------  cabrillo_tag_template  ----------------

/*! \class  cabrillo_tag_template
    \brief  template for a cabrillo tag
*/

class cabrillo_tag_template
{
protected:

  std::string                 _name;            ///< name of the tag
  std::vector<std::string>    _legal_values;    ///< allowed values; if empty then any text is permitted

/*! \brief      Initialise the name from a string
    \param  nm  tag name
*/
  void _init_from_string(const std::string& str);

public:
  
/*! \brief      Construct from name
    \param  nm  tag name
        
    Any tag value is legal for this tag
*/
  inline explicit cabrillo_tag_template(const std::string& nm)
    { _init_from_string(nm); }
    
/// destructor
  inline virtual ~cabrillo_tag_template(void) = default;

/// cabrillo_tag_template = string
  inline void operator=(const std::string& nm)
    { _init_from_string(nm); }
  
/*! \brief          Add a legal value
    \param  val     value to add
*/
  inline void add_legal_value(const std::string& val)
    { _legal_values.push_back(val); }
};

// -----------  cabrillo_tag_templatess  ----------------

/*! \class  cabrillo_tag_templates
    \brief  all the cabrillo tag templates
*/

class cabrillo_tag_templates
{
protected:

  std::vector<cabrillo_tag_template>    _templates;    ///< container for the templates; there aren't many different kinds of tags, so a vector will suffice

/*! \brief          Add a tag name
    \param  str     name of tag to add
*/
  inline void _add(const std::string& str)
    { _templates.push_back(cabrillo_tag_template(str)); }
  
public:
  
/// default constructor
  cabrillo_tag_templates(void);
    
/// destructor
  inline virtual ~cabrillo_tag_templates(void) = default;
};

#endif    // CABRILLO_H
