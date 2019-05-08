// $Id: adif.cpp 150 2019-04-05 16:09:55Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   adif.cpp

    Objects and functions related to ADIF version 2.2.7 at http://www.adif.org/adif227.htm

*/

#include "adif.h"
#include "string_functions.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------  adif_DATE -----------------------------------------

// set value
void adif_DATE::value(const std::string& v)
{ if (v.length() != 8)
    throw exception();

  if ((v.find_first_not_of(DIGITS)) != string::npos)
    throw exception();

  const string year_str  { v.substr(0, 4) };
  const string month_str { v.substr(4, 2) };
  const string day_str   { v.substr(6) };

  const unsigned int month { from_string<unsigned int>(month_str) };

  if (month < 1 or month > 12)
    throw exception();

  const unsigned int day { from_string<unsigned int>(day_str) };

  if (day < 1 or day > 31)
    throw exception();

  _value = v;
}

// ---------------------------------------------------  adif_STRING -----------------------------------------

// default constructor
adif_STRING::adif_STRING(void) :
    adif_type('S')
{ }

// construct with name and value
adif_STRING::adif_STRING(const string& nm, const string& v) :
    adif_type('S', nm, v)
{ }

// construct with name
adif_STRING::adif_STRING(const string& nm) :
    adif_type('S', nm, string())
{ }

// set value
// a sequence of Characters
// an ASCII character whose code lies in the range of 32 through 126, inclusive
void adif_STRING::value(const std::string& v)
{ FOR_ALL(v, [] (const char c) { if ( (c < 32) or (c > 126) ) throw exception(); } );

  _value = v;
}

// ---------------------------------------------------  adif_TIME -----------------------------------------

// default constructor
adif_TIME::adif_TIME(void) :
    adif_type('T')
{ }

// construct with name and value
adif_TIME::adif_TIME(const string& nm, const string& v) :
    adif_type('T', nm, v)
{ }

// construct with name
adif_TIME::adif_TIME(const string& nm) :
    adif_type('T', nm, string())
{ }

// set value
void adif_TIME::value(const std::string& v)
{ if ((v.find_first_not_of(DIGITS)) != string::npos)
    throw exception();

  if ((v.length() < 4) or (v.length() == 5) or (v.length() > 6))
    throw exception();

  const string       hour_str   { v.substr(0, 2) };
  const string       minute_str { v.substr(2, 2) };
  const unsigned int hour       { from_string<unsigned int>(hour_str) };

  if (hour > 23)
    throw exception();

  const unsigned int minute { from_string<unsigned int>(minute_str) };

  if (minute > 59)
    throw exception();

  if (v.length() == 6)
  { const string       second_str { v.substr(4, 2) };
    const unsigned int second     { from_string<unsigned int>(second_str) };

    if (second > 59)
      throw exception();
  }

  _value = v;
}

// ---------------------------------------------------  adif_record-----------------------------------------

/// constructor
adif_record::adif_record(void) :
    _linefeeds_after_field(1),
    _linefeeds_after_record(1),
    _address("address"s),
    _adif_ver("adif_ver"s),
    _age("age"s),
    _a_index("a_index"s),
    _ant_az("ant_az"s),
    _ant_el("ant_el"s),
    _ant_path("ant_path"s),
 //   _arrl_sect("arrl_sect"),
    _arrl_sect("arrl_sect"s, SECTION_ENUMERATION),
//    _band("band"),
    _band("band"s, BAND_ENUMERATION),
    _band_rx("band_rx"s),
    _call("call"),
    _check("check"),
    _class("class"),
    _cnty("cnty"),
    _comment("comment"),
    _cont("cont"),
    _contacted_op("contacted_op"),
    _contest_id("contest_id"),
    _country("country"),
    _cqz("cqz"),
    _credit_submitted("credit_submitted"),
    _credit_granted("credit_granted"),
    _distance("distance"),
    _dxcc("dxcc"),
    _email("email"),
    _eq_call("eq_call"),
    _eqsl_qslrdate("eqsl_qslrdate"),
    _eqsl_qslsdate("eqsl_qslsdate"),
    _eqsl_qsl_rcvd("eqsl_qsl_rcvd"),
    _eqsl_qsl_sent("eqsl_qsl_sent"),
    _force_init("force_init"),
    _freq("freq"),
    _freq_rx("freq_rx"),
    _gridsquare("gridsquare"),
    _iota("iota"),
    _iota_island_id("iota_island_id"),
    _ituz("ituz"),
    _k_index("k_index"),
    _lat("lat"),
    _lon("lon"),
    _lotw_qslrdate("lotw_qslrdate"),
    _lotw_qslsdate("lotw_qslsdate"),
    _lotw_qsl_rcvd("lotw_qsl_rcvd"),
    _lotw_qsl_sent("lotw_qsl_sent"),
    _max_bursts("max_bursts"),
//    _mode("mode"),
    _mode("mode", MODE_ENUMERATION),
    _ms_shower("ms_shower"),
    _my_city("my_city"),
    _my_cnty("my_cnty"),
    _my_country("my_country"),
    _my_cq_zone("my_cq_zone"),
    _my_gridsquare("my_gridsquare"),
    _my_iota("my_iota"),
    _my_iota_island_id("my_iota_island_id"),
    _my_itu_zone("my_itu_zone"),
    _my_lat("my_lat"),
    _my_lon("my_lon"),
    _my_name("my_name"),
    _my_postal_code("my_postal_code"),
    _my_rig("my_rig"),
    _my_sig("my_sig"),
    _my_sig_info("my_sig_info"),
    _my_state("my_state"),
    _my_street("my_street"),
    _name("name"),
    _notes("notes"),
    _nr_bursts("nr_bursts"),
    _nr_pings("nr_pings"),
    _operator("operator"),
    _owner_callsign("owner_callsign"),
    _pfx("pfx"),
    _precedence("precedence"),
    _programid("programid"),
    _programversion("programversion"),
//    _prop_mode("prop_mode"),
    _prop_mode("prop_mode", PROPAGATION_MODE_ENUMERATION),
    _public_key("public_key"),
    _qslmsg("qslmsg"),
    _qslrdate("qslrdate"),
    _qslsdate("qslsdate"),
    _qsl_rcvd("qsl_rcvd"),
    _qsl_rcvd_via("qsl_rcvd_via"),
    _qsl_sent("qsl_sent"),
    _qsl_sent_via("qsl_sent_via"),
    _qsl_via("qsl_via"),
    _qso_complete("qso_complete"),
    _qso_date("qso_date"),
    _qso_date_off("qso_date_off"),
    _qso_random("qso_random"),
    _qth("qth"),
    _rig("rig"),
    _rst_rcvd("rst_rcvd"),
    _rst_sent("rst_sent"),
    _rx_pwr("rx_pwr"),
    _sat_mode("sat_mode"),
    _sat_name("sat_name"),
    _sfi("sfi"),
    _sig("sig"),
    _sig_info("sig_info"),
    _srx("srx"),
    _srx_string("srx_string"),
    _state("state"),
    _station_callsign("station_callsign"),
    _stx("stx"),
    _stx_string("stx_string"),
    _swl("swl"),
    _ten_ten("ten_ten"),
    _time_off("time_off"),
    _time_on("time_on"),
    _tx_pwr("tx_pwr"),
    _web("web")
//    _test("burble", ANT_PATH_ENUMERATION)
{ }

template <class T>
inline const string field_string(const T& mbr, const string& post)
  { return mbr.to_string() + (mbr.value().empty() ? string() : post); }

/// convert record to the printable string format
const string adif_record::to_string(void) const
{ const string post_field_string(_linefeeds_after_record, '\n');
  const string post_record_string('\n', _linefeeds_after_record);

  string rv;

  rv += field_string(_address, post_field_string);
  rv += field_string(_adif_ver, post_field_string);
  rv += field_string(_age, post_field_string);
  rv += field_string(_a_index, post_field_string);
  rv += field_string(_ant_az, post_field_string);
  rv += field_string(_ant_el, post_field_string);
  rv += field_string(_ant_path, post_field_string);
  rv += field_string(_arrl_sect, post_field_string);

  rv += field_string(_band, post_field_string);
  rv += field_string(_band_rx, post_field_string);

  rv += field_string(_call, post_field_string);
  rv += field_string(_check, post_field_string);
  rv += field_string(_class, post_field_string);
  rv += field_string(_cnty, post_field_string);
  rv += field_string(_comment, post_field_string);
  rv += field_string(_cont, post_field_string);
  rv += field_string(_contacted_op, post_field_string);
  rv += field_string(_contest_id, post_field_string);
  rv += field_string(_country, post_field_string);
  rv += field_string(_cqz, post_field_string);
  rv += field_string(_credit_submitted, post_field_string);
  rv += field_string(_credit_granted, post_field_string);

  rv += field_string(_distance, post_field_string);
  rv += field_string(_dxcc, post_field_string);

  rv += field_string(_email, post_field_string);
  rv += field_string(_eq_call, post_field_string);
  rv += field_string(_eqsl_qslrdate, post_field_string);
  rv += field_string(_eqsl_qslsdate, post_field_string);
  rv += field_string(_eqsl_qsl_rcvd, post_field_string);
  rv += field_string(_eqsl_qsl_sent, post_field_string);

  rv += field_string(_force_init, post_field_string);
  rv += field_string(_freq, post_field_string);
  rv += field_string(_freq_rx, post_field_string);

  rv += field_string(_gridsquare, post_field_string);

  rv += field_string(_iota, post_field_string);
  rv += field_string(_iota_island_id, post_field_string);
  rv += field_string(_ituz, post_field_string);

  rv += field_string(_k_index, post_field_string);

  rv += field_string(_lat, post_field_string);
  rv += field_string(_lon, post_field_string);
  rv += field_string(_lotw_qslrdate, post_field_string);
  rv += field_string(_lotw_qslsdate, post_field_string);
  rv += field_string(_lotw_qsl_rcvd, post_field_string);
  rv += field_string(_lotw_qsl_sent, post_field_string);

  rv += field_string(_max_bursts, post_field_string);
  rv += field_string(_mode, post_field_string);
  rv += field_string(_ms_shower, post_field_string);
  rv += field_string(_my_city, post_field_string);
  rv += field_string(_my_cnty, post_field_string);
  rv += field_string(_my_country, post_field_string);
  rv += field_string(_my_cq_zone, post_field_string);
  rv += field_string(_my_gridsquare, post_field_string);
  rv += field_string(_my_iota, post_field_string);
  rv += field_string(_my_iota_island_id, post_field_string);
  rv += field_string(_my_itu_zone, post_field_string);
  rv += field_string(_my_lat, post_field_string);
  rv += field_string(_my_lon, post_field_string);
  rv += field_string(_my_name, post_field_string);
  rv += field_string(_my_postal_code, post_field_string);
  rv += field_string(_my_rig, post_field_string);
  rv += field_string(_my_sig, post_field_string);
  rv += field_string(_my_sig_info, post_field_string);
  rv += field_string(_my_state, post_field_string);
  rv += field_string(_my_street, post_field_string);

  rv += field_string(_name, post_field_string);
  rv += field_string(_notes, post_field_string);
  rv += field_string(_nr_bursts, post_field_string);
  rv += field_string(_nr_pings, post_field_string);

  rv += field_string(_operator, post_field_string);
  rv += field_string(_owner_callsign, post_field_string);

  rv += field_string(_pfx, post_field_string);
  rv += field_string(_precedence, post_field_string);
  rv += field_string(_programid, post_field_string);
  rv += field_string(_programversion, post_field_string);
  rv += field_string(_prop_mode, post_field_string);
  rv += field_string(_public_key, post_field_string);

  rv += field_string(_qslmsg, post_field_string);
  rv += field_string(_qslrdate, post_field_string);
  rv += field_string(_qslsdate, post_field_string);
  rv += field_string(_qsl_rcvd, post_field_string);
  rv += field_string(_qsl_rcvd_via, post_field_string);
  rv += field_string(_qsl_sent, post_field_string);
  rv += field_string(_qsl_sent_via, post_field_string);
  rv += field_string(_qsl_via, post_field_string);
  rv += field_string(_qso_complete, post_field_string);
  rv += field_string(_qso_date, post_field_string);
  rv += field_string(_qso_date_off, post_field_string);
  rv += field_string(_qso_random, post_field_string);
  rv += field_string(_qth, post_field_string);

  rv += field_string(_rig, post_field_string);
  rv += field_string(_rst_rcvd, post_field_string);
  rv += field_string(_rst_sent, post_field_string);
  rv += field_string(_rx_pwr, post_field_string);

  rv += field_string(_sat_mode, post_field_string);
  rv += field_string(_sat_name, post_field_string);
  rv += field_string(_sfi, post_field_string);
  rv += field_string(_sig, post_field_string);
  rv += field_string(_sig_info, post_field_string);
  rv += field_string(_srx, post_field_string);
  rv += field_string(_srx_string, post_field_string);
  rv += field_string(_state, post_field_string);
  rv += field_string(_station_callsign, post_field_string);
  rv += field_string(_stx, post_field_string);
  rv += field_string(_stx_string, post_field_string);
  rv += field_string(_swl, post_field_string);

  rv += field_string(_ten_ten, post_field_string);
  rv += field_string(_time_off, post_field_string);
  rv += field_string(_time_on, post_field_string);
  rv += field_string(_tx_pwr, post_field_string);

  rv += field_string(_web, post_field_string);
  rv += ("<eor>"s + post_field_string);


  return rv;
}

// constructor
adif_country::adif_country(const string& nm, const string& pfx, bool del) :
    _code(++next_code),
    _name(nm),
    _canonical_prefix(pfx),
    _deleted(del)
{ }

unsigned int adif_country::next_code { 1 };

void adif_countries::_add_country(const string& nm, const unsigned int index, const string& pfx, const bool deleted)
{ while ((_countries.size() + 1)< index)
    { _countries.push_back(adif_country("")); }

  _countries.push_back(adif_country(nm, pfx, deleted));
}

// default constructor
// The names ADIF defines are, let's just say, idiosyncratic.
// No, let's say inconsistent and sometimes downright wrong. For an international standard,
// it gives every appearance of being simply irresponsible.
// There are multiple holes in the array, I have no idea why
adif_countries::adif_countries(void)
{ _countries.push_back(adif_country("CANADA"s, "VE"s));
  _countries.push_back(adif_country("ABU AIL IS"s, ""s, true));
  _countries.push_back(adif_country("AFGHANISTAN"s, "YA"s));
  _countries.push_back(adif_country("AGALEGA & ST BRANDON"s, "3B6"s));
  _countries.push_back(adif_country("ALAND IS"s, "OH0"s));
  _countries.push_back(adif_country("ALASKA"s, "KL"s));
  _countries.push_back(adif_country("ALBANIA"s, "ZA"s));
  _countries.push_back(adif_country("ALDABRA"s, ""s, true));
  _countries.push_back(adif_country("AMERICAN SAMOA"s, "KH8"s));
  _countries.push_back(adif_country("AMSTERDAM & ST PAUL"s, "FT5Z"s));
  _countries.push_back(adif_country("ANDAMAN & NICOBAR IS"s, "VU4"s));
  _countries.push_back(adif_country("ANGUILLA"s, "VP2E"s));
  _countries.push_back(adif_country("ANTARCTICA"s, "CE9"s));
  _countries.push_back(adif_country("ARMENIA"s, "EK"s));
  _countries.push_back(adif_country("ASIATIC RUSSIA"s, "UA9"s));
  _countries.push_back(adif_country("AUCKLAND & CAMPBELL"s, "ZL9"s));
  _countries.push_back(adif_country("AVES ISLAND"s, "YV0"s));
  _countries.push_back(adif_country("AZERBAIJAN"s, "4J"s));
  _countries.push_back(adif_country("BAJO NUEVO"s, ""s, true));
  _countries.push_back(adif_country("BAKER, HOWLAND IS"s, "KH1"s));
  _countries.push_back(adif_country("BALEARIC IS"s, "EA6"s));
  _countries.push_back(adif_country("PALAU"s, "T8"s));
  _countries.push_back(adif_country("BLENHEIM REEF"s, ""s, true));
  _countries.push_back(adif_country("BOUVET"s, "3Y/b"s));
  _countries.push_back(adif_country("BRITISH N. BORNEO"s, ""s, true));
  _countries.push_back(adif_country("BRITISH SOMALI"s, ""s, true));
  _countries.push_back(adif_country("BELARUS"s, "EU"s));
  _countries.push_back(adif_country("CANAL ZONE"s, ""s, true));
  _countries.push_back(adif_country("CANARY IS"s, "EA8"s));
  _countries.push_back(adif_country("CELEBE/MOLUCCA IS"s, ""s, true));
  _countries.push_back(adif_country("CEUTA & MELILLA"s, "EA9"s));
  _countries.push_back(adif_country("C KIRIBATI"s, "T31"s));                     // my vote for the most ludicrous ADIF name
  _countries.push_back(adif_country("CHAGOS"s, "VQ9"s));
  _countries.push_back(adif_country("CHATHAM IS"s, "ZL7"s));
  _countries.push_back(adif_country("CHRISTMAS IS"s, "VK9X"s));
  _countries.push_back(adif_country("CLIPPERTON IS"s, "FO/c"s));
  _countries.push_back(adif_country("COCOS ISLAND"s, "TI9"s));
  _countries.push_back(adif_country("COCOS-KEELING IS"s, "VK9C"s));
  _countries.push_back(adif_country("COMOROS"s, ""s, true));
  _countries.push_back(adif_country("CRETE"s, "SV9"s));
  _countries.push_back(adif_country("CROZET"s, "FT5W"s));
  _countries.push_back(adif_country("DAMAO, DUI"s, ""s, true));
  _countries.push_back(adif_country("DESECHEO IS"s, "KP5"s));
  _countries.push_back(adif_country("DESROCHES"s, ""s, true));
  _countries.push_back(adif_country("DODECANESE"s, "SV5"s));
  _countries.push_back(adif_country("EAST MALAYSIA"s, "9M6"s));
  _countries.push_back(adif_country("EASTER IS"s, "CE0Y"s));
  _countries.push_back(adif_country("EASTERN KIRIBATI"s, "T32"s));
  _countries.push_back(adif_country("EQUATORIAL GUINEA"s, "3C"s));
  _countries.push_back(adif_country("MEXICO"s, "XE"s));
  _countries.push_back(adif_country("ERITREA"s, "E3"s));
  _countries.push_back(adif_country("ESTONIA"s, "ES"s));
  _countries.push_back(adif_country("ETHIOPIA"s "ET"s));
  _countries.push_back(adif_country("EUROPEAN RUSSIA"s, "UA"s));
  _countries.push_back(adif_country("FARQUHAR"s, ""s, true));
  _countries.push_back(adif_country("FERNANDO DE NORONHA"s, "PY0F"s));
  _countries.push_back(adif_country("FRENCH EQ. AFRICA"s, ""s, true));
  _countries.push_back(adif_country("FRENCH INDO-CHINA"s, ""s, true));
  _countries.push_back(adif_country("FRENCH WEST AFRICA"s, ""s, true));
  _countries.push_back(adif_country("BAHAMAS"s, "C6"s));
  _countries.push_back(adif_country("FRANZ JOSEF LAND"s, "R1FJ"s));
  _countries.push_back(adif_country("BARBADOS"s, "8P"s));
  _countries.push_back(adif_country("FRENCH GUIANA"s, "FY"s));
  _countries.push_back(adif_country("BERMUDA"s, "VP9"s));
  _countries.push_back(adif_country("BRITISH VIRGIN IS"s, "VP2V"s));
  _countries.push_back(adif_country("BELIZE"s, "V3"s));
  _countries.push_back(adif_country("FRENCH INDIA"s, ""s, true));
  _countries.push_back(adif_country("SAUDI/KUWAIT N.Z."s, ""s, true));
  _countries.push_back(adif_country("CAYMAN ISLANDS"s, "ZF"s));
  _countries.push_back(adif_country("CUBA"s, "CM"s));
  _countries.push_back(adif_country("GALAPAGOS"s, "HC8"s));
  _countries.push_back(adif_country("DOMINICAN REPUBLIC"s, "HI"s));
  _countries.push_back(adif_country("EL SALVADOR"s, "YS"s));
  _countries.push_back(adif_country(""s));                   // don't ask me... there really is an unused country code at value 73. The first of several.
  _countries.push_back(adif_country("GEORGIA"s, "4L"s));
  _countries.push_back(adif_country("GUATEMALA"s, "TG"s));
  _countries.push_back(adif_country("GRENADA"s, "J3"s));
  _countries.push_back(adif_country("HAITI"s, "HH"s));
  _countries.push_back(adif_country("GUADELOUPE"s, "FG"s));
  _countries.push_back(adif_country("HONDURAS"s, "HR"s));
  _countries.push_back(adif_country("GERMANY"s, ""s, true));      /// yep, ADIF really does define a deleted country called "GERMANY"
  _countries.push_back(adif_country("JAMAICA"s, "6Y"s));
  _countries.push_back(adif_country(""s));                   // 83
  _countries.push_back(adif_country("MARTINIQUE"s, "FM"s));
  _countries.push_back(adif_country("BONAIRE,CURACAO"s, ""s, true));    // there really is no space after the comma
  _countries.push_back(adif_country("NICARAGUA"s, "YN"s));
  _countries.push_back(adif_country(""s));                   // 87
  _countries.push_back(adif_country("PANAMA"s, "HP"s));
  _countries.push_back(adif_country("TURKS & CAICOS IS"s, "VP5"s));
  _countries.push_back(adif_country("TRINIDAD & TOBAGO"s, "9Y"s));
  _countries.push_back(adif_country("ARUBA"s, "P4"s));
  _countries.push_back(adif_country(""));                   // 92
  _countries.push_back(adif_country("GEYSER REEF", "", true));
  _countries.push_back(adif_country("ANTIGUA & BARBUDA", "V2"));
  _countries.push_back(adif_country("DOMINICA", "J7"));
  _countries.push_back(adif_country("MONTSERRAT", "VP2M"));
  _countries.push_back(adif_country("ST LUCIA", "J6"));
  _countries.push_back(adif_country("ST VINCENT", "J8"));
  _countries.push_back(adif_country("GLORIOSO IS", "FR/g"));
  _countries.push_back(adif_country("ARGENTINA", "LU"));
  _countries.push_back(adif_country("GOA", "", true));
  _countries.push_back(adif_country("GOLD COAST, TOGOLAND", "", true));     // !!
  _countries.push_back(adif_country("GUAM", "KH2"));
  _countries.push_back(adif_country("BOLIVIA", "CP"));
  _countries.push_back(adif_country("GUANTANAMO BAY", "KG4"));
  _countries.push_back(adif_country("GUERNSEY", "GU"));
  _countries.push_back(adif_country("GUINEA", "3X"));
  _countries.push_back(adif_country("BRAZIL", "PY"));
  _countries.push_back(adif_country("GUINEA-BISSAU", "J5"));
  _countries.push_back(adif_country("HAWAII", "KH6"));
  _countries.push_back(adif_country("HEARD IS", "VK0H"));
  _countries.push_back(adif_country("CHILE", "CE"));
  _countries.push_back(adif_country("IFNI", "", true));
  _countries.push_back(adif_country("ISLE OF MAN", "GD"));
  _countries.push_back(adif_country("ITALIAN SOMALI", "", true));
  _countries.push_back(adif_country("COLOMBIA", "HK"));
  _countries.push_back(adif_country("ITU HQ", "4U1I"));
  _countries.push_back(adif_country("JAN MAYEN", "JX"));
  _countries.push_back(adif_country("JAVA", "", true));
  _countries.push_back(adif_country("ECUADOR", "HC"));
  _countries.push_back(adif_country(""));                   // 121
  _countries.push_back(adif_country("JERSEY", "GJ"));
  _countries.push_back(adif_country("JOHNSTON IS", "KH3"));
  _countries.push_back(adif_country("JUAN DE NOVA", "FR/j"));
  _countries.push_back(adif_country("JUAN FERNANDEZ", "CE0Z"));
  _countries.push_back(adif_country("KALININGRAD", "UA2"));
  _countries.push_back(adif_country("KAMARAN IS", "", true));
  _countries.push_back(adif_country("KARELO-FINN REP", "", true));
  _countries.push_back(adif_country("GUYANA", "8R"));
  _countries.push_back(adif_country("KAZAKHSTAN", "UN"));
  _countries.push_back(adif_country("KERGUELEN", "FT5X"));
  _countries.push_back(adif_country("PARAGUAY", "ZP"));
  _countries.push_back(adif_country("KERMADEC", "ZL8"));
  _countries.push_back(adif_country("KINGMAN REEF", "KH5K"));
  _countries.push_back(adif_country("KYRGYZSTAN", "EX"));
  _countries.push_back(adif_country("PERU", "OA"));
  _countries.push_back(adif_country("REPUBLIC OF KOREA", "HK"));
  _countries.push_back(adif_country("KURE ISLAND", "KH7K"));
  _countries.push_back(adif_country("KURIA MURIA IS", "", true));
  _countries.push_back(adif_country("SURINAME", "PZ"));
  _countries.push_back(adif_country("FALKLAND IS", "VP8"));
  _countries.push_back(adif_country("LAKSHADWEEP ISLANDS", "VU7"));
  _countries.push_back(adif_country("LAOS", "XW"));
  _countries.push_back(adif_country("URUGUAY", "CX"));
  _countries.push_back(adif_country("LATVIA", "YL"));
  _countries.push_back(adif_country("LITHUANIA", "LY"));
  _countries.push_back(adif_country("LORD HOWE IS", "VK9L"));
  _countries.push_back(adif_country("VENEZUELA", "YV"));
  _countries.push_back(adif_country("AZORES", "CU"));
  _countries.push_back(adif_country("AUSTRALIA", "VK"));
  _countries.push_back(adif_country("MALYJ VYSOTSKI IS", "R1MV"));
  _countries.push_back(adif_country("MACAO", "XX9"));
  _countries.push_back(adif_country("MACQUARIE IS", "VK0M"));
  _countries.push_back(adif_country("YEMEN ARAB REP", "", true));
  _countries.push_back(adif_country("MALAYA", "", true));
  _countries.push_back(adif_country(""));                   // 156
  _countries.push_back(adif_country("NAURU", "C2"));
  _countries.push_back(adif_country("VANUATU", "YJ"));
  _countries.push_back(adif_country("MALDIVES", "8Q"));
  _countries.push_back(adif_country("TONGA", "A3"));
  _countries.push_back(adif_country("MALPELO IS", "HK0/m"));
  _countries.push_back(adif_country("NEW CALEDONIA", "FK"));
  _countries.push_back(adif_country("PAPUA NEW GUINEA", "P2"));
  _countries.push_back(adif_country("MANCHURIA", "", true));
  _countries.push_back(adif_country("MAURITIUS IS", "3B8"));
  _countries.push_back(adif_country("MARIANA IS", "KH0"));
  _countries.push_back(adif_country("MARKET REEF", "OJ0"));
  _countries.push_back(adif_country("MARSHALL IS", "V7"));
  _countries.push_back(adif_country("MAYOTTE", "FH"));
  _countries.push_back(adif_country("NEW ZEALAND", "ZL"));
  _countries.push_back(adif_country("MELLISH REEF", "VK9M"));
  _countries.push_back(adif_country("PITCAIRN IS", "VP6"));
  _countries.push_back(adif_country("MICRONESIA", "V6"));
  _countries.push_back(adif_country("MIDWAY IS", "KH4"));
  _countries.push_back(adif_country("FRENCH POLYNESIA", "FO"));
  _countries.push_back(adif_country("FIJI", "3D"));
  _countries.push_back(adif_country("MINAMI TORISHIMA", "JD/m"));
  _countries.push_back(adif_country("MINERVA REEF", "", true));
  _countries.push_back(adif_country("MOLDOVA", "ER"));
  _countries.push_back(adif_country("MOUNT ATHOS", "SV/a"));
  _countries.push_back(adif_country("MOZAMBIQUE", "C9"));
  _countries.push_back(adif_country("NAVASSA IS", "KP1"));
  _countries.push_back(adif_country("NETHERLANDS BORNEO", "", true));
  _countries.push_back(adif_country("NETHERLANDS N GUINEA", "", true));
  _countries.push_back(adif_country("SOLOMON ISLANDS", "H4"));
  _countries.push_back(adif_country("NEWFOUNDLAND, LABRADOR", "", true));
  _countries.push_back(adif_country("NIGER", "5U"));
  _countries.push_back(adif_country("NIUE", "ZK2"));
  _countries.push_back(adif_country("NORFOLK IS", "VK9N"));
  _countries.push_back(adif_country("SAMOA", "5W"));
  _countries.push_back(adif_country("N COOK IS", "E5/n"));
  _countries.push_back(adif_country("OGASAWARA", "JD/o"));
  _countries.push_back(adif_country("OKINAWA", "", true));
  _countries.push_back(adif_country("OKINO TORI-SHIMA", "", true));
  _countries.push_back(adif_country("ANNOBON I.", "3C0"));             // don't ask me why it isn't "IS"
  _countries.push_back(adif_country("PALESTINE", "", true));
  _countries.push_back(adif_country("PALMYRA & JARVIS IS", "KH5"));
  _countries.push_back(adif_country("PAPUA TERR", "", true));
  _countries.push_back(adif_country("PETER I IS", "3Y/p"));
  _countries.push_back(adif_country("PORTUGUESE TIMOR", "", true));
  _countries.push_back(adif_country("PRINCE EDWARD & MARION", "ZS8"));
  _countries.push_back(adif_country("PUERTO RICO", "KP4"));
  _countries.push_back(adif_country("ANDORRA", "C3"));
  _countries.push_back(adif_country("REVILLAGIGEDO", "XF4"));
  _countries.push_back(adif_country("ASCENSION ISLAND", "ZD8"));
  _countries.push_back(adif_country("AUSTRIA", "OE"));
  _countries.push_back(adif_country("RODRIGUEZ IS", "3B9"));
  _countries.push_back(adif_country("RUANDA-URUNDI", "", true));
  _countries.push_back(adif_country("BELGIUM", "ON"));
  _countries.push_back(adif_country("SAAR", "", true));
  _countries.push_back(adif_country("SABLE ISLAND", "CY0"));
  _countries.push_back(adif_country("BULGARIA", "LZ"));
  _countries.push_back(adif_country("SAINT MARTIN", "FS"));
  _countries.push_back(adif_country("CORSICA", "TK"));
  _countries.push_back(adif_country("CYPRUS", "5B"));
  _countries.push_back(adif_country("SAN ANDRES & PROVIDENCIA", "HK0/a"));
  _countries.push_back(adif_country("SAN FELIX", "CE0X"));
  _countries.push_back(adif_country("CZECHOSLOVAKIA", "", true));
  _countries.push_back(adif_country("SAO TOME & PRINCIPE", "S9"));
  _countries.push_back(adif_country("SARAWAK", "", true));
  _add_country("DENMARK", 221, "OZ");
  _add_country("FAROE IS", 222, "OY");
  _add_country("ENGLAND", 223, "G");
  _add_country("FINLAND", 224, "OH");
  _add_country("SARDINIA", 225, "IS");
  _add_deleted_country("SAUDI/IRAQ N.Z.", 226);
  _add_country("FRANCE", 227, "F");
  _add_deleted_country("SERRANA BANK & RONCADOR CAY", 228);  // Yes I stole the name of Roncador Cay to use in one of my books
  _add_deleted_country("GERMAN DEM. REP.", 229);
  _add_country("FED REP OF GERMANY", 230, "DL");
  _add_deleted_country("SIKKIM", 231);
  _add_country("SOMALIA", 232, "T5");
  _add_country("GIBRALTAR", 233, "ZB");
  _add_country("S COOK IS", 234, "E5/s");
  _add_country("SOUTH GEORGIA IS", 235, "VP8/g");
  _add_country("GREECE", 236, "SV");
  _add_country("GREENLAND", 237, "OX");
  _add_country("SOUTH ORKNEY IS", 238, "VP8/o");
  _add_country("HUNGARY", 239, "HA");
  _add_country("SOUTH SANDWICH ISLANDS", 240, "VP8/s");
  _add_country("SOUTH SHETLAND ISLANDS", 241, "VP8/h");
  _add_country("ICELAND", 242, "TF");
  _add_deleted_country("DEM REP OF YEMEN", 243);
  _add_deleted_country("SOUTHERN SUDAN", 244);
  _add_country("IRELAND", 245, "EI");
  _add_country("SOV MILITARY ORDER OF MALTA", 246, "1A");
  _add_country("SPRATLY IS", 247, "1S");
  _add_country("ITALY", 248, "I");
  _add_country("ST KITTS & NEVIS", 249, "V4");
  _add_country("ST HELENA IS", 250, "ZD7");
  _add_country("LIECHTENSTEIN", 251, "HB0");
  _add_country("ST PAUL ISLAND", 252, "CY9");
  _add_country("ST. PETER & ST. PAUL ROCKS", 253, "PY0S");                 // cf ST[.] prior entry(!)
  _add_country("LUXEMBOURG", 254, "LX");
  _add_deleted_country("SINT MAARTEN, SABA, ST EUSTATIUS", 255);
  _add_country("MADEIRA IS", 256, "CT3");
  _add_country("MALTA", 257, "9H");
  _add_deleted_country("SUMATRA", 258);
  _add_country("SVALBARD IS", 259, "JW");
  _add_country("MONACO", 260, "3A");
  _add_deleted_country("SWAN ISLAND", 261);
  _add_country("TAJIKISTAN", 262, "EY");
  _add_country("NETHERLANDS", 263, "PA");
  _add_deleted_country("TANGIER", 264);
  _add_country("NORTHERN IRELAND", 265, "GI");
  _add_country("NORWAY", 266, "LA");
  _add_deleted_country("TERR NEW GUINEA", 267);
  _add_deleted_country("TIBET", 268);
  _add_country("POLAND", 269, "SP");
  _add_country("TOKELAU IS", 270, "ZK3");
  _add_deleted_country("TRIESTE"s, 271);
  _add_country("PORTUGAL", 272, "CT");
  _add_country("TRINDADE & MARTIN VAZ ISLANDS", 273, "PY0T");      // MARTIN??? -- Where did that come from?
  _add_country("TRISTAN DA CUNHA & GOUGH IS", 274, "ZD9");
  _add_country("ROMANIA", 275, "YO");
  _add_country("TROMELIN", 276, "FR/t");
  _add_country("ST PIERRE & MIQUELON", 277, "FP");
  _add_country("SAN MARINO", 278, "T7");
  _add_country("SCOTLAND", 279, "GM");
  _add_country("TURKMENISTAN", 280, "EZ");
  _add_country("SPAIN", 281, "EA");
  _add_country("TUVALU", 282, "T2");
  _add_country("UK BASES ON CYPRUS", 283, "ZC4");
  _add_country("SWEDEN", 284, "SM");
  _add_country("US VIRGIN ISLANDS", 285, "KP2");
  _add_country("UGANDA", 286, "5X");
  _add_country("SWITZERLAND", 287, "HB");
  _add_country("UKRAINE", 288, "UR");
  _add_country("UNITED NATIONS HQ", 289, "4U1U");
  _add_country("UNITED STATES", 291, "K");
  _add_country("UZBEKISTAN", 292, "UK");
  _add_country("VIETNAM", 293, "3W");
  _add_country("WALES", 294, "GW");
  _add_country("VATICAN", 295, "HV");
  _add_country("SERBIA", 296, "YU");
  _add_country("WAKE IS", 297, "KH9");
  _add_country("WALLIS & FUTUNA", 298, "FW");
  _add_country("WEST MALAYSIA", 299, "9M2");
  _add_country("W KIRIBATI", 301, "T30");
  _add_country("WESTERN SAHARA", 302, "S0");
  _add_country("WILLIS IS", 303, "VK9W");
  _add_country("BAHRAIN", 304, "A9");
  _add_country("BANGLADESH", 305, "S2");
  _add_country("BHUTAN", 306, "A5");
  _add_country("ZANZIBAR", 307, "", true);
  _add_country("COSTA RICA", 308, "TI");
  _add_country("MYANMAR", 309, "XZ");
  _add_country("CAMBODIA", 312, "XU");     // Presumably this is supposed to be Kampuchea
  _add_country("SRI LANKA", 315, "4S");
  _add_country("CHINA", 318, "BY");
  _add_country("HONG KONG", 321, "VR");
  _add_country("INDIA", 324, "VU");
  _add_country("INDONESIA", 327, "YB");
  _add_country("IRAN", 330, "EP");
  _add_country("IRAQ", 333, "YI");
  _add_country("ISRAEL", 336, "4X");
  _add_country("JAPAN", 339, "JA");
  _add_country("JORDAN"s,                                342, "JY"s);
  _add_country("DEMOCRATIC PEOPLE'S REPUBLIC OF KOREA"s, 344, "HM"s);
  _add_country("BRUNEI"s,                                345, "V8"s);
  _add_country("KUWAIT"s,                                348, "9K"s);
  _add_country("LEBANON"s,                               354, "OD"s);
  _add_country("MONGOLIA"s,                              363, "JT"s);
  _add_country("NEPAL"s,                                 369, "9N"s);
  _add_country("OMAN"s,                                  370, "A4"s);
  _add_country("PAKISTAN"s,                              372, "AP"s);
  _add_country("PHILIPPINES"s,                           375, "DU"s);
  _add_country("QATAR"s,                                 376, "A7"s);
  _add_country("SAUDI ARABIA"s,                          378, "HZ"s);
  _add_country("SEYCHELLES"s,                            379, "S7"s);
  _add_country("SINGAPORE"s,                             381, "9V"s);
  _add_country("DJIBOUTI"s,                              382, "J2"s);
  _add_country("SYRIA"s,                                 384, "YK"s);
  _add_country("TAIWAN"s,                                386, "BV"s);
  _add_country("THAILAND"s,                              387, "HS"s);
  _add_country("TURKEY"s,                                390, "TA"s);
  _add_country("UNITED ARAB EMIRATES"s,                  391, "A6"s);
  _add_country("ALGERIA"s,                               400, "7X"s);
  _add_country("ANGOLA"s,                     401, "D2"s);
  _add_country("BOTSWANA"s,                   402, "A2"s);
  _add_country("BURUNDI"s,                    404, "9U"s);
  _add_country("CAMEROON"s,                   406, "TJ"s);
  _add_country("CENTRAL AFRICAN REPUBLIC"s,   408, "TL"s);
  _add_country("CAPE VERDE"s,                 409, "D4"s);
  _add_country("CHAD"s,                       410, "TT"s);
  _add_country("COMOROS"s,                    411, "D6"s);
  _add_country("REPUBLIC OF THE CONGO"s,      412, "9Q"s);
  _add_country("DEM. REPUBLIC OF THE CONGO"s, 414, "TN"s);
  _add_country("BENIN"s,                      416, "TY"s);
  _add_country("GABON"s,                      420, "TR"s);
  _add_country("THE GAMBIA"s,                 422, "C5"s);
  _add_country("GHANA"s,                      424, "9G"s);
  _add_country("COTE D'IVOIRE"s,              428, "TU"s);
  _add_country("KENYA"s,                      430, "5Z"s);
  _add_country("LESOTHO"s,                    432, "7P"s);
  _add_country("LIBERIA"s,                    434, "EL"s);
  _add_country("LIBYA"s,                      436, "5A"s);
  _add_country("MADAGASCAR"s,                 438, "5R"s);
  _add_country("MALAWI"s,                     440, "7Q"s);
  _add_country("MALI"s,                       442, "TZ"s);
  _add_country("MAURITANIA"s,                 444, "5T"s);
  _add_country("MOROCCO"s,                    446, "CN"s);
  _add_country("NIGERIA"s,                    450, "5N"s);
  _add_country("ZIMBABWE"s,                   452, "Z2"s);
  _add_country("REUNION"s,                    453, "FR"s);
  _add_country("RWANDA"s,                     454, "9X"s);
  _add_country("SENEGAL"s,                    456, "6W"s);
  _add_country("SIERRA LEONE"s,               458, "9L"s);
  _add_country("ROTUMA IS"s,                  460, "3D2/r"s);
  _add_country("REPUBLIC OF SOUTH AFRICA"s,   462, "ZS"s);
  _add_country("NAMIBIA"s,                    464, "V5"s);
  _add_country("SUDAN"s,                      466, "ST"s);
  _add_country("SWAZILAND"s,                  468, "3DA"s);
  _add_country("TANZANIA"s,                   470, "5H"s);
  _add_country("TUNISIA"s,                    474, "3V"s);
  _add_country("EGYPT"s,                      478, "SU"s);
  _add_country("BURKINA-FASO"s,               480, "XT"s);
  _add_country("ZAMBIA"s,                     482, "9J"s);
  _add_country("TOGO"s,                       483, "5V"s);
  _add_deleted_country("WALVIS BAY"s,         488);
  _add_country("CONWAY REEF"s,                489, "3D2/c"s);
  _add_country("BANABA ISLAND"s,              490, "T33"s);
  _add_country("YEMEN"s,                      492, "7O"s);
  _add_deleted_country("PENGUIN ISLANDS"s,    493);
  _add_country("CROATIA"s,                    497, "9A"s);
  _add_country("SLOVENIA"s,                   499, "S5"s);
  _add_country("BOSNIA-HERZEGOVINA"s,         501, "E7"s);
  _add_country("MACEDONIA"s,                  502, "Z3"s);
  _add_country("CZECH REPUBLIC"s,             503, "OK"s);
  _add_country("SLOVAK REPUBLIC"s,            504, "OM"s);
  _add_country("PRATAS IS"s,                  505, "BV9P"s);
  _add_country("SCARBOROUGH REEF"s,           506, "BS7"s);
  _add_country("TEMOTU PROVINCE"s,            507, "H40"s);
  _add_country("AUSTRAL IS"s,                 508, "FO/a"s);
  _add_country("MARQUESAS IS"s,               509, "FO/m"s);
  _add_country("PALESTINE"s,                  510, "E4"s);
  _add_country("TIMOR-LESTE"s,                511, "4W"s);
  _add_country("CHESTERFIELD IS"s,            512, "FK/c"s);
  _add_country("DUCIE IS"s,                   513, "VP6/d"s);
  _add_country("MONTENEGRO"s,                 514, "4O"s);
  _add_country("SWAINS ISLAND"s,              515, "KH8/s"s);
  _add_country("ST. BARTHELEMY"s,             516, "FJ"s);
  _add_country("CURACAO"s,                    517, "PJ2"s);
  _add_country("SINT MAARTEN"s,               518, "PJ7"s);
  _add_country("ST EUSTATIUS AND SABA"s,      519, "PJ5"s);
  _add_country("BONAIRE"s,                    520, "PJ4"s);
}

/*! \brief              Extract the value from an ADIF line, ignoring the last <i>offeset</i> characters
    \param  this_line   line from an ADIF file
    \param  offset      number of characters to ignore at the end of the line
    \return             value extracted from <i>this_line</i>
*/
const string adif_value(const string& this_line, const unsigned int offset)
{ const string         tag { delimited_substring(this_line, '<', '>') };
  const vector<string> vs  { split_string(tag, ":"s) };

  if (vs.size() != 2)
  { cerr << "ERROR parsing old log line: " << this_line << endl;
    return string();
  }
  else
  { const size_t n_chars { from_string<size_t>(vs[1]) };
    const size_t posn    { this_line.find('>') };

    return substring(this_line, posn + 1, n_chars - offset);
  }
};
