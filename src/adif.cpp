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
  _countries.push_back(adif_country(""s));                   // 92
  _countries.push_back(adif_country("GEYSER REEF"s, ""s, true));
  _countries.push_back(adif_country("ANTIGUA & BARBUDA"s, "V2"s));
  _countries.push_back(adif_country("DOMINICA"s, "J7"s));
  _countries.push_back(adif_country("MONTSERRAT"s, "VP2M"s));
  _countries.push_back(adif_country("ST LUCIA"s, "J6"s));
  _countries.push_back(adif_country("ST VINCENT"s, "J8"s));
  _countries.push_back(adif_country("GLORIOSO IS"s, "FR/g"s));
  _countries.push_back(adif_country("ARGENTINA"s, "LU"s));
  _countries.push_back(adif_country("GOA"s, ""s, true));
  _countries.push_back(adif_country("GOLD COAST, TOGOLAND"s, ""s, true));     // !!
  _countries.push_back(adif_country("GUAM"s, "KH2"s));
  _countries.push_back(adif_country("BOLIVIA"s, "CP"s));
  _countries.push_back(adif_country("GUANTANAMO BAY"s, "KG4"s));
  _countries.push_back(adif_country("GUERNSEY"s, "GU"s));
  _countries.push_back(adif_country("GUINEA"s, "3X"s));
  _countries.push_back(adif_country("BRAZIL"s, "PY"s));
  _countries.push_back(adif_country("GUINEA-BISSAU"s, "J5"s));
  _countries.push_back(adif_country("HAWAII"s, "KH6"s));
  _countries.push_back(adif_country("HEARD IS"s, "VK0H"s));
  _countries.push_back(adif_country("CHILE"s, "CE"s));
  _countries.push_back(adif_country("IFNI"s, ""s, true));
  _countries.push_back(adif_country("ISLE OF MAN"s, "GD"s));
  _countries.push_back(adif_country("ITALIAN SOMALI"s, ""s, true));
  _countries.push_back(adif_country("COLOMBIA"s, "HK"s));
  _countries.push_back(adif_country("ITU HQ"s, "4U1I"s));
  _countries.push_back(adif_country("JAN MAYEN"s, "JX"s));
  _countries.push_back(adif_country("JAVA"s, ""s, true));
  _countries.push_back(adif_country("ECUADOR"s, "HC"s));
  _countries.push_back(adif_country(""s));                   // 121
  _countries.push_back(adif_country("JERSEY"s, "GJ"s));
  _countries.push_back(adif_country("JOHNSTON IS"s, "KH3"s));
  _countries.push_back(adif_country("JUAN DE NOVA"s, "FR/j"s));
  _countries.push_back(adif_country("JUAN FERNANDEZ"s, "CE0Z"s));
  _countries.push_back(adif_country("KALININGRAD"s, "UA2"s));
  _countries.push_back(adif_country("KAMARAN IS"s, ""s, true));
  _countries.push_back(adif_country("KARELO-FINN REP"s, ""s, true));
  _countries.push_back(adif_country("GUYANA"s, "8R"s));
  _countries.push_back(adif_country("KAZAKHSTAN"s, "UN"s));
  _countries.push_back(adif_country("KERGUELEN"s, "FT5X"s));
  _countries.push_back(adif_country("PARAGUAY"s, "ZP"s));
  _countries.push_back(adif_country("KERMADEC"s, "ZL8"s));
  _countries.push_back(adif_country("KINGMAN REEF"s, "KH5K"s));
  _countries.push_back(adif_country("KYRGYZSTAN"s, "EX"s));
  _countries.push_back(adif_country("PERU"s, "OA"s));
  _countries.push_back(adif_country("REPUBLIC OF KOREA"s, "HK"s));
  _countries.push_back(adif_country("KURE ISLAND"s, "KH7K"s));
  _countries.push_back(adif_country("KURIA MURIA IS"s, ""s, true));
  _countries.push_back(adif_country("SURINAME"s, "PZ"s));
  _add_country("FALKLAND IS"s, 141, "VP8"s);
  _add_country("LAKSHADWEEP ISLANDS"s, 142, "VU7"s);
  _add_country("LAOS"s, 143, "XW"s);
  _add_country("URUGUAY"s, 144, "CX"s);
  _add_country("LATVIA"s, 145, "YL"s);
  _add_country("LITHUANIA"s, 146, "LY"s);
  _add_country("LORD HOWE IS"s, 147, "VK9L"s);
  _add_country("VENEZUELA"s, 148, "YV"s);
  _add_country("AZORES"s, 149, "CU"s);
  _add_country("AUSTRALIA"s, 150, "VK"s);
  _add_country("MALYJ VYSOTSKI IS"s, 151, "R1MV"s);
  _add_country("MACAO", 152, "XX9");
  _add_country("MACQUARIE IS", 153, "VK0M");
  _add_deleted_country("YEMEN ARAB REP", 154);
  _add_deleted_country("MALAYA", 155);
  _add_country("NAURU", 157, "C2");
  _add_country("VANUATU", 158, "YJ");
  _add_country("MALDIVES", 159, "8Q");
  _add_country("TONGA", 160, "A3");
  _add_country("MALPELO IS", 161, "HK0/m");
  _add_country("NEW CALEDONIA", 162, "FK");
  _add_country("PAPUA NEW GUINEA", 163, "P2");
  _add_deleted_country("MANCHURIA", 164);
  _add_country("MAURITIUS IS", 165, "3B8");
  _add_country("MARIANA IS", 166, "KH0");
  _add_country("MARKET REEF", 167, "OJ0");
  _add_country("MARSHALL IS", 168, "V7");
  _add_country("MAYOTTE", 169, "FH");
  _add_country("NEW ZEALAND", 170, "ZL");
  _add_country("MELLISH REEF", 171, "VK9M");
  _add_country("PITCAIRN IS", 172, "VP6");
  _add_country("MICRONESIA", 173, "V6");
  _add_country("MIDWAY IS", 174, "KH4");
  _add_country("FRENCH POLYNESIA", 175, "FO");
  _add_country("FIJI", 176, "3D");
  _add_country("MINAMI TORISHIMA", 177, "JD/m");
  _add_deleted_country("MINERVA REEF", 178);
  _add_country("MOLDOVA", 179, "ER");
  _add_country("MOUNT ATHOS", 180, "SV/a");
  _add_country("MOZAMBIQUE", 181, "C9");
  _add_country("NAVASSA IS", 182, "KP1");
  _add_deleted_country("NETHERLANDS BORNEO", 183);
  _add_deleted_country("NETHERLANDS N GUINEA", 184);
  _add_country("SOLOMON ISLANDS", 185, "H4");
  _add_deleted_country("NEWFOUNDLAND, LABRADOR", 186);
  _add_country("NIGER", 187, "5U");
  _add_country("NIUE", 188, "ZK2");
  _add_country("NORFOLK IS", 189, "VK9N");
  _add_country("SAMOA", 190, "E5/n");
  _add_country("N COOK IS", 191, "E5/n");
  _add_country("OGASAWARA", 192, "JD/o");
  _add_deleted_country("OKINAWA", 193);
  _add_deleted_country("OKINO TORI-SHIMA", 194);
  _add_country("ANNOBON I.", 195, "3C0");             // don't ask me why it isn't "IS"
  _add_deleted_country("PALESTINE", 196);
  _add_country("PALMYRA & JARVIS IS", 197, "KH5");
  _add_deleted_country("PAPUA TERR", 198);
  _add_country("PETER I IS", 199, "3Y/p");
  _add_deleted_country("PORTUGUESE TIMOR", 200);
  _add_country("PRINCE EDWARD & MARION"s,                   201, "ZS8"s);
  _add_country("PUERTO RICO"s,                              202, "KP4"s);
  _add_country("ANDORRA"s,                                  203, "C3"s);
  _add_country("REVILLAGIGEDO"s,                            204, "XF4"s);
  _add_country("ASCENSION ISLAND"s,                         205, "ZD8"s);
  _add_country("AUSTRIA"s,                                  206, "OE"s);
  _add_country("RODRIGUEZ IS"s,                             207, "3B9"s);
  _add_deleted_country("RUANDA-URUNDI"s,                    208);
  _add_country("BELGIUM"s,                                  209, "ON"s);
  _add_deleted_country("SAAR"s,                             210);
  _add_country("SABLE ISLAND"s,                             211, "CY0"s);
  _add_country("BULGARIA"s,                                 212, "LZ"s);
  _add_country("SAINT MARTIN"s,                             213, "FS"s);
  _add_country("CORSICA"s,                                  214, "TK"s);
  _add_country("CYPRUS"s,                                   215, "5B"s);
  _add_country("SAN ANDRES & PROVIDENCIA"s,                 216, "HK0/a"s);
  _add_country("SAN FELIX"s,                                217, "CE0X"s);
  _add_deleted_country("CZECHOSLOVAKIA"s,                   218);
  _add_country("SAO TOME & PRINCIPE"s,                      219, "S9"s);
  _add_deleted_country("SARAWAK"s,                          220);
  _add_country("DENMARK"s,                                  221, "OZ"s);
  _add_country("FAROE IS"s,                                 222, "OY"s);
  _add_country("ENGLAND"s,                                  223, "G"s);
  _add_country("FINLAND"s,                                  224, "OH"s);
  _add_country("SARDINIA"s,                                 225, "IS"s);
  _add_deleted_country("SAUDI/IRAQ N.Z."s,                  226);
  _add_country("FRANCE"s,                                   227, "F"s);
  _add_deleted_country("SERRANA BANK & RONCADOR CAY"s,      228);  // Yes I stole the name of Roncador Cay to use in one of my books
  _add_deleted_country("GERMAN DEM. REP."s,                 229);
  _add_country("FED REP OF GERMANY"s,                       230, "DL"s);
  _add_deleted_country("SIKKIM"s,                           231);
  _add_country("SOMALIA"s,                                  232, "T5"s);
  _add_country("GIBRALTAR"s,                                233, "ZB"s);
  _add_country("S COOK IS"s,                                234, "E5/s"s);
  _add_country("SOUTH GEORGIA IS"s,                         235, "VP8/g"s);
  _add_country("GREECE"s,                                   236, "SV"s);
  _add_country("GREENLAND"s,                                237, "OX"s);
  _add_country("SOUTH ORKNEY IS"s,                          238, "VP8/o"s);
  _add_country("HUNGARY"s,                                  239, "HA"s);
  _add_country("SOUTH SANDWICH ISLANDS"s,                   240, "VP8/s"s);
  _add_country("SOUTH SHETLAND ISLANDS"s,                   241, "VP8/h"s);
  _add_country("ICELAND"s,                                  242, "TF"s);
  _add_deleted_country("DEM REP OF YEMEN"s,                 243);
  _add_deleted_country("SOUTHERN SUDAN"s,                   244);
  _add_country("IRELAND"s,                                  245, "EI"s);
  _add_country("SOV MILITARY ORDER OF MALTA"s,              246, "1A"s);
  _add_country("SPRATLY IS"s,                               247, "1S"s);
  _add_country("ITALY"s,                                    248, "I"s);
  _add_country("ST KITTS & NEVIS"s,                         249, "V4"s);
  _add_country("ST HELENA IS"s,                             250, "ZD7"s);
  _add_country("LIECHTENSTEIN"s,                            251, "HB0"s);
  _add_country("ST PAUL ISLAND"s,                           252, "CY9"s);
  _add_country("ST. PETER & ST. PAUL ROCKS"s,               253, "PY0S"s);                 // cf ST[.] prior entry(!)
  _add_country("LUXEMBOURG"s,                               254, "LX"s);
  _add_deleted_country("SINT MAARTEN, SABA, ST EUSTATIUS"s, 255);
  _add_country("MADEIRA IS"s,                               256, "CT3"s);
  _add_country("MALTA"s,                                    257, "9H"s);
  _add_deleted_country("SUMATRA"s,                          258);
  _add_country("SVALBARD IS"s,                              259, "JW"s);
  _add_country("MONACO"s,                                   260, "3A"s);
  _add_deleted_country("SWAN ISLAND"s,                      261);
  _add_country("TAJIKISTAN"s,                               262, "EY"s);
  _add_country("NETHERLANDS"s,                              263, "PA"s);
  _add_deleted_country("TANGIER"s,                          264);
  _add_country("NORTHERN IRELAND"s,                         265, "GI"s);
  _add_country("NORWAY"s,                                   266, "LA"s);
  _add_deleted_country("TERR NEW GUINEA"s,                  267);
  _add_deleted_country("TIBET"s,                            268);
  _add_country("POLAND"s,                                   269, "SP"s);
  _add_country("TOKELAU ISs",                               270, "ZK3"s);
  _add_deleted_country("TRIESTE"s,                          271);
  _add_country("PORTUGAL"s,                                 272, "CT"s);
  _add_country("TRINDADE & MARTIN VAZ ISLANDS"s,            273, "PY0T"s);      // MARTIN??? -- Where did that come from?
  _add_country("TRISTAN DA CUNHA & GOUGH IS"s,              274, "ZD9"s);
  _add_country("ROMANIA"s,                                  275, "YO"s);
  _add_country("TROMELIN"s,                                 276, "FR/t"s);
  _add_country("ST PIERRE & MIQUELON"s,                     277, "FP"s);
  _add_country("SAN MARINO"s,                               278, "T7"s);
  _add_country("SCOTLAND"s,                                 279, "GM"s);
  _add_country("TURKMENISTAN"s,                             280, "EZ"s);
  _add_country("SPAIN"s,                                    281, "EA"s);
  _add_country("TUVALU"s,                                   282, "T2"s);
  _add_country("UK BASES ON CYPRUS"s,                       283, "ZC4"s);
  _add_country("SWEDEN"s,                                   284, "SM"s);
  _add_country("US VIRGIN ISLANDS"s,                        285, "KP2"s);
  _add_country("UGANDA"s,                                   286, "5X"s);
  _add_country("SWITZERLAND"s,                              287, "HB"s);
  _add_country("UKRAINE"s,                                  288, "UR"s);
  _add_country("UNITED NATIONS HQ"s,                        289, "4U1U"s);
  _add_country("UNITED STATES"s,                            291, "K"s);
  _add_country("UZBEKISTAN"s,                               292, "UK"s);
  _add_country("VIETNAM"s,                                  293, "3W"s);
  _add_country("WALES"s,                                    294, "GW"s);
  _add_country("VATICAN"s,                                  295, "HV"s);
  _add_country("SERBIA"s,                                   296, "YU"s);
  _add_country("WAKE IS"s,                                  297, "KH9"s);
  _add_country("WALLIS & FUTUNA"s,                          298, "FW"s);
  _add_country("WEST MALAYSIA"s,                            299, "9M2"s);
  _add_country("W KIRIBATI"s,                               301, "T30"s);
  _add_country("WESTERN SAHARA"s,                           302, "S0"s);
  _add_country("WILLIS IS"s,                                303, "VK9W"s);
  _add_country("BAHRAIN"s,                                  304, "A9"s);
  _add_country("BANGLADESH"s,                               305, "S2"s);
  _add_country("BHUTAN"s,                                   306, "A5"s);
  _add_deleted_country("ZANZIBAR"s,                         307);
  _add_country("COSTA RICA"s,                               308, "TI"s);
  _add_country("MYANMAR"s,                                  309, "XZ"s);
  _add_country("CAMBODIA"s,                                 312, "XU"s);     // Presumably this is supposed to be Kampuchea
  _add_country("SRI LANKA"s,                                315, "4S"s);
  _add_country("CHINA"s,                                    318, "BY"s);
  _add_country("HONG KONG"s,                                321, "VR"s);
  _add_country("INDIA"s,                                    324, "VU"s);
  _add_country("INDONESIA"s,                                327, "YB"s);
  _add_country("IRAN"s,                                     330, "EP"s);
  _add_country("IRAQ"s,                                     333, "YI"s);
  _add_country("ISRAEL"s,                                   336, "4X"s);
  _add_country("JAPAN"s,                                    339, "JA"s);
  _add_country("JORDAN"s,                                   342, "JY"s);
  _add_country("DEMOCRATIC PEOPLE'S REPUBLIC OF KOREA"s,    344, "HM"s);
  _add_country("BRUNEI"s,                                   345, "V8"s);
  _add_country("KUWAIT"s,                                   348, "9K"s);
  _add_country("LEBANON"s,                                  354, "OD"s);
  _add_country("MONGOLIA"s,                                 363, "JT"s);
  _add_country("NEPAL"s,                                    369, "9N"s);
  _add_country("OMAN"s,                                     370, "A4"s);
  _add_country("PAKISTAN"s,                                 372, "AP"s);
  _add_country("PHILIPPINES"s,                              375, "DU"s);
  _add_country("QATAR"s,                                    376, "A7"s);
  _add_country("SAUDI ARABIA"s,                             378, "HZ"s);
  _add_country("SEYCHELLES"s,                               379, "S7"s);
  _add_country("SINGAPORE"s,                                381, "9V"s);
  _add_country("DJIBOUTI"s,                                 382, "J2"s);
  _add_country("SYRIA"s,                                    384, "YK"s);
  _add_country("TAIWAN"s,                                   386, "BV"s);
  _add_country("THAILAND"s,                                 387, "HS"s);
  _add_country("TURKEY"s,                                   390, "TA"s);
  _add_country("UNITED ARAB EMIRATES"s,                     391, "A6"s);
  _add_country("ALGERIA"s,                                  400, "7X"s);
  _add_country("ANGOLA"s,                                401, "D2"s);
  _add_country("BOTSWANA"s,                              402, "A2"s);
  _add_country("BURUNDI"s,                               404, "9U"s);
  _add_country("CAMEROON"s,                              406, "TJ"s);
  _add_country("CENTRAL AFRICAN REPUBLIC"s,              408, "TL"s);
  _add_country("CAPE VERDE"s,                            409, "D4"s);
  _add_country("CHAD"s,                                  410, "TT"s);
  _add_country("COMOROS"s,                               411, "D6"s);
  _add_country("REPUBLIC OF THE CONGO"s,                 412, "9Q"s);
  _add_country("DEM. REPUBLIC OF THE CONGO"s,            414, "TN"s);
  _add_country("BENIN"s,                                 416, "TY"s);
  _add_country("GABON"s,                                 420, "TR"s);
  _add_country("THE GAMBIA"s,                            422, "C5"s);
  _add_country("GHANA"s,                                 424, "9G"s);
  _add_country("COTE D'IVOIRE"s,                         428, "TU"s);
  _add_country("KENYA"s,                                 430, "5Z"s);
  _add_country("LESOTHO"s,                               432, "7P"s);
  _add_country("LIBERIA"s,                               434, "EL"s);
  _add_country("LIBYA"s,                                 436, "5A"s);
  _add_country("MADAGASCAR"s,                            438, "5R"s);
  _add_country("MALAWI"s,                                440, "7Q"s);
  _add_country("MALI"s,                                  442, "TZ"s);
  _add_country("MAURITANIA"s,                            444, "5T"s);
  _add_country("MOROCCO"s,                               446, "CN"s);
  _add_country("NIGERIA"s,                               450, "5N"s);
  _add_country("ZIMBABWE"s,                              452, "Z2"s);
  _add_country("REUNION"s,                               453, "FR"s);
  _add_country("RWANDA"s,                                454, "9X"s);
  _add_country("SENEGAL"s,                               456, "6W"s);
  _add_country("SIERRA LEONE"s,                          458, "9L"s);
  _add_country("ROTUMA IS"s,                             460, "3D2/r"s);
  _add_country("REPUBLIC OF SOUTH AFRICA"s,              462, "ZS"s);
  _add_country("NAMIBIA"s,                               464, "V5"s);
  _add_country("SUDAN"s,                                 466, "ST"s);
  _add_country("SWAZILAND"s,                             468, "3DA"s);
  _add_country("TANZANIA"s,                              470, "5H"s);
  _add_country("TUNISIA"s,                               474, "3V"s);
  _add_country("EGYPT"s,                                 478, "SU"s);
  _add_country("BURKINA-FASO"s,                          480, "XT"s);
  _add_country("ZAMBIA"s,                                482, "9J"s);
  _add_country("TOGO"s,                                  483, "5V"s);
  _add_deleted_country("WALVIS BAY"s,                    488);
  _add_country("CONWAY REEF"s,                           489, "3D2/c"s);
  _add_country("BANABA ISLAND"s,                         490, "T33"s);
  _add_country("YEMEN"s,                                 492, "7O"s);
  _add_deleted_country("PENGUIN ISLANDS"s,               493);
  _add_country("CROATIA"s,                               497, "9A"s);
  _add_country("SLOVENIA"s,                              499, "S5"s);
  _add_country("BOSNIA-HERZEGOVINA"s,                    501, "E7"s);
  _add_country("MACEDONIA"s,                             502, "Z3"s);
  _add_country("CZECH REPUBLIC"s,                        503, "OK"s);
  _add_country("SLOVAK REPUBLIC"s,                       504, "OM"s);
  _add_country("PRATAS IS"s,                             505, "BV9P"s);
  _add_country("SCARBOROUGH REEF"s,                      506, "BS7"s);
  _add_country("TEMOTU PROVINCE"s,                       507, "H40"s);
  _add_country("AUSTRAL IS"s,                            508, "FO/a"s);
  _add_country("MARQUESAS IS"s,                          509, "FO/m"s);
  _add_country("PALESTINE"s,                             510, "E4"s);
  _add_country("TIMOR-LESTE"s,                           511, "4W"s);
  _add_country("CHESTERFIELD IS"s,                       512, "FK/c"s);
  _add_country("DUCIE IS"s,                              513, "VP6/d"s);
  _add_country("MONTENEGRO"s,                            514, "4O"s);
  _add_country("SWAINS ISLAND"s,                         515, "KH8/s"s);
  _add_country("ST. BARTHELEMY"s,                        516, "FJ"s);
  _add_country("CURACAO"s,                               517, "PJ2"s);
  _add_country("SINT MAARTEN"s,                          518, "PJ7"s);
  _add_country("ST EUSTATIUS AND SABA"s,                 519, "PJ5"s);
  _add_country("BONAIRE"s,                               520, "PJ4"s);
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
