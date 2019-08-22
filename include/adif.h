// $Id: adif.h 152 2019-08-21 20:23:38Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

#ifndef ADIF_H
#define ADIF_H

/*! \file   adif.h

    Objects and functions related to ADIF version 2.2.7 at http://www.adif.org/adif227.htm

*/

#include "macros.h"
#include "string_functions.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

using namespace std::literals::string_literals;

// enumerations

// ant path  -------------------------------------------------------

/// antenna path
#if 0
enum ANT_PATH_ENUM { ANT_PATH_GREYLINE,
                     ANT_PATH_OTHER,
                     ANT_PATH_SHORT_PATH,
                     ANT_PATH_LONG_PATH,
                     N_ANT_PATHS
                   };

using ANT_PATH_ENUMERATION_TYPE = std::array<std::string, N_ANT_PATHS>;     ///< type for antenna path enumeration
#endif

enum class ADIF_ANT_PATH { GREYLINE,
                           OTHER,
                           SHORT_PATH,
                           LONG_PATH,
                           N_ANT_PATHS
                         };

using ANT_PATH_ENUMERATION_TYPE = std::array<std::string, static_cast<unsigned int>(ADIF_ANT_PATH::N_ANT_PATHS)>;     ///< type for antenna path enumeration

/// legal values of ANT_PATH_ENUMERATION
static ANT_PATH_ENUMERATION_TYPE ANT_PATH_ENUMERATION { "G"s,    ///< greyline
                                                        "O"s,    ///< other
                                                        "S"s,    ///< short path
                                                        "L"s     ///< long path
                                                      };

// mode  -------------------------------------------------------

/// modes
enum class ADIF_MODE { AM,              // 0
                       AMTORFEC,
                       ASCI,
                       ATV,
                       CHIP64,
                       CHIP128,
                       CLO,
                       CONTESTI,
                       CW,
                       DSTAR,
                       DOMINO,          // 10
                       DOMINOF,
                       FAX,
                       FM,
                       FMHELL,
                       FSK31,
                       FSK441,
                       GTOR,
                       HELL,
                       HELL80,
                       HFSK,            // 20
                       JT44,
                       JT4A,
                       JT4B,
                       JT4C,
                       JT4D,
                       JT4E,
                       JT4F,
                       JT4G,
                       JT65,
                       JT65A,           // 30
                       JT65B,
                       JT65C,
                       JT6M,
                       MFSK8,
                       MFSK16,
                       MT63,
                       OLIVIA,
                       PAC,
                       PAC2,
                       PAC3,            // 40
                       PAX,
                       PAX2,
                       PCW,
                       PKT,
                       PSK10,
                       PSK31,
                       PSK63,
                       PSK63F,
                       PSK125,
                       PSKAM10,         // 50
                       PSKAM31,
                       PSKAM50,
                       PSKFEC31,
                       PSKHELL,
                       Q15,
                       QPSK31,
                       QPSK63,
                       QPSK125,
                       ROS,
                       RTTY,            // 60
                       RTTYM,
                       SSB,
                       SSTV,
                       THRB,
                       THOR,
                       THRBX,
                       TOR,
                       VOI,
                       WINMOR,
                       WSPR,            // 70
                       N_ADIF_MODES
                     };                                                       ///< enum for modes

using MODE_ENUMERATION_TYPE = std::array<std::string, static_cast<unsigned int>(ADIF_MODE::N_ADIF_MODES)>;    ///< type for mode enumeration

static MODE_ENUMERATION_TYPE MODE_ENUMERATION { "AM"s,               // 0
                                                "AMTORFEC"s,
                                                "ASCI"s,
                                                "ATV"s,
                                                "CHIP64"s,
                                                "CHIP128"s,
                                                "CLO"s,
                                                "CONTESTI"s,
                                                "CW"s,
                                                "DSTAR"s,
                                                "DOMINO"s,            // 10
                                                "DOMINOF"s,
                                                "FAX"s,
                                                "FM"s,
                                                "FMHELL"s,
                                                "FSK31"s,
                                                "FSK441"s,
                                                "GTOR"s,
                                                "HELL"s,
                                                "HELL80"s,
                                                "HFSK"s,             // 20
                                                "JT44"s,
                                                "JT4A"s,
                                                "JT4B"s,
                                                "JT4C"s,
                                                "JT4D"s,
                                                "JT4E"s,
                                                "JT4F"s,
                                                "JT4G"s,
                                                "JT65"s,
                                                "JT65A"s,            // 30
                                                "JT65B"s,
                                                "JT65C"s,
                                                "JT6M"s,
                                                "MFSK8"s,
                                                "MFSK16"s,
                                                "MT63"s,
                                                "OLIVIA"s,
                                                "PAC"s,
                                                "PAC2"s,
                                                "PAC3"s,             // 40
                                                "PAX"s,
                                                "PAX2"s,
                                                "PCW"s,
                                                "PKT"s,
                                                "PSK10"s,
                                                "PSK31"s,
                                                "PSK63"s,
                                                "PSK63F"s,
                                                "PSK125"s,
                                                "PSKAM10"s,          // 50
                                                "PSKAM31"s,
                                                "PSKAM50"s,
                                                "PSKFEC31"s,
                                                "PSKHELL"s,
                                                "Q15"s,
                                                "QPSK31"s,
                                                "QPSK63"s,
                                                "QPSK125"s,
                                                "ROS"s,
                                                "RTTY"s,             // 60
                                                "RTTYM"s,
                                                "SSB"s,
                                                "SSTV"s,
                                                "THRB"s,
                                                "THOR"s,
                                                "THRBX"s,
                                                "TOR"s,
                                                "VOI"s,
                                                "WINMOR"s,
                                                "WSPR"s              // 70
                                              };

// ARRL section  -------------------------------------------------------

/// sections
enum class ADIF_SECTION { AL,
                          AK,
                          AB,
                          AR,
                          AZ,
                          BC,
                          CO,
                          CT,
                          DE,
                          EB,
                          EMA,
                          ENY,
                          EPA,
                          EWA,
                          GA,
                          ID,
                          IL,
                          IN,
                          IA,
                          KS,
                          KY,
                          LAX,
                          LA,
                          ME,
                          MB,
                          MAR,
                          MDC,
                          MI,
                          MN,
                          MS,
                          MO,
                          MT,
                          NE,
                          NV,
                          NH,
                          NM,
                          NLI,
                          NL,
                          NC,
                          ND,
                          NTX,
                          NFL,
                          NNJ,
                          NNY,
                          NT,
                          OH,
                          OK,
                          ON,
                          ORG,
                          OR,
                          PAC,
                          PR,
                          QC,
                          RI,
                          SV,
                          SDG,
                          SF,
                          SJV,
                          SB,
                          SCV,
                          SK,
                          SC,
                          SD,
                          STX,
                          SFL,
                          SNJ,
                          TN,
                          VI,
                          UT,
                          VT,
                          VA,
                          WCF,
                          WTX,
                          WV,
                          WMA,
                          WNY,
                          WPA,
                          WWA,
                          WI,
                          WY,
                          N_SECTIONS
                        };                                                        ///< enum for sections

using SECTION_ENUMERATION_TYPE = std::array<std::string, static_cast<unsigned int>(ADIF_SECTION::N_SECTIONS)>;       ///< type for section enumeration

static SECTION_ENUMERATION_TYPE SECTION_ENUMERATION { "AL"s,
                                                      "AK"s,
                                                      "AB"s,
                                                      "AR"s,
                                                      "AZ"s,
                                                      "BC"s,
                                                      "CO"s,
                                                      "CT"s,
                                                      "DE"s,
                                                      "EB"s,
                                                      "EMA"s,
                                                      "ENY"s,
                                                      "EPA"s,
                                                      "EWA"s,
                                                      "GA"s,
                                                      "ID"s,
                                                      "IL"s,
                                                      "IN"s,
                                                      "IA"s,
                                                      "KS"s,
                                                      "KY"s,
                                                      "LAX"s,
                                                      "LA"s,
                                                      "ME"s,
                                                      "MB"s,
                                                      "MAR"s,
                                                      "MDC"s,
                                                      "MI"s,
                                                      "MN"s,
                                                      "MS"s,
                                                      "MO"s,
                                                      "MT"s,
                                                      "NE"s,
                                                      "NV"s,
                                                      "NH"s,
                                                      "NM"s,
                                                      "NLI"s,
                                                      "NL"s,
                                                      "NC"s,
                                                      "ND"s,
                                                      "NTX"s,
                                                      "NFL"s,
                                                      "NNJ"s,
                                                      "NNY"s,
                                                      "NT"s,
                                                      "OH"s,
                                                      "OK"s,
                                                      "ON"s,
                                                      "ORG"s,
                                                      "OR"s,
                                                      "PAC"s,
                                                      "PR"s,
                                                      "QC"s,
                                                      "RI"s,
                                                      "SV"s,
                                                      "SDG"s,
                                                      "SF"s,
                                                      "SJV"s,
                                                      "SB"s,
                                                      "SCV"s,
                                                      "SK"s,
                                                      "SC"s,
                                                      "SD"s,
                                                      "STX"s,
                                                      "SFL"s,
                                                      "SNJ"s,
                                                      "TN"s,
                                                      "VI"s,
                                                      "UT"s,
                                                      "VT"s,
                                                      "VA"s,
                                                      "WCF"s,
                                                      "WTX"s,
                                                      "WV"s,
                                                      "WMA"s,
                                                      "WNY"s,
                                                      "WPA"s,
                                                      "WWA"s,
                                                      "WI"s,
                                                      "WY"s
                                                    };

// awards  -------------------------------------------------------

/// awards
enum class ADIF_AWARD { AJA,
                        CQDX,
                        CQDXFIELD,
                        CQWAZ_MIXED,
                        CQWAZ_CW,
                        CQWAZ_PHONE,
                        CQWAZ_RTTY,
                        CQWAZ_160m,
                        CQWPX,
                        DARC_DOK,
                        DXCC,
                        DXCC_MIXED,
                        DXCC_CW,
                        DXCC_PHONE,
                        DXCC_RTTY,
                        IOTA,
                        JCC,
                        JCG,
                        MARATHON,
                        RDA,
                        WAB,
                        WAC,
                        WAE,
                        WAIP,
                        WAJA,
                        WAS,
                        WAZ,
                        USACA,
                        VUCC,
                        N_AWARDS
                      };                                                      ///< enum for awards

using AWARD_ENUMERATION_TYPE = std::array<std::string, static_cast<unsigned int>(ADIF_AWARD::N_AWARDS)>;       ///< type for award enumeration

static AWARD_ENUMERATION_TYPE AWARD_ENUMERATION { "AJA"s,
                                                  "CQDX"s,
                                                  "CQDXFIELD"s,
                                                  "CQWAZ_MIXED"s,
                                                  "CQWAZ_CW"s,
                                                  "CQWAZ_PHONE"s,
                                                  "CQWAZ_RTTY"s,
                                                  "CQWAZ_160m"s,
                                                  "CQWPX"s,
                                                  "DARC_DOK"s,
                                                  "DXCC"s,
                                                  "DXCC_MIXED"s,
                                                  "DXCC_CW"s,
                                                  "DXCC_PHONE"s,
                                                  "DXCC_RTTY"s,
                                                  "IOTA"s,
                                                  "JCC"s,
                                                  "JCG"s,
                                                  "MARATHON"s,
                                                  "RDA"s,
                                                  "WAB"s,
                                                  "WAC"s,
                                                  "WAE"s,
                                                  "WAIP"s,
                                                  "WAJA"s,
                                                  "WAS"s,
                                                  "WAZ"s,
                                                  "USACA"s,
                                                  "VUCC"s
                                                } ;

// band  -------------------------------------------------------

/// bands
enum class ADIF_BAND { BAND_2190m,  // keep the BAND_ prefix as names can't begin with a digit
                       BAND_560m,
                       BAND_160m,
                       BAND_80m,
                       BAND_60m,
                       BAND_40m,
                       BAND_30m,
                       BAND_20m,
                       BAND_17m,
                       BAND_15m,
                       BAND_12m,
                       BAND_10m,
                       BAND_6m,
                       BAND_4m,
                       BAND_2m,
                       BAND_1point25m,
                       BAND_70cm,
                       BAND_33cm,
                       BAND_23cm,
                       BAND_13cm,
                       BAND_9cm,
                       BAND_6cm,
                       BAND_3cm,
                       BAND_1point25cm,
                       BAND_6mm,
                       BAND_4mm,
                       BAND_2point5mm,
                       BAND_2mm,
                       BAND_1mm,
                       N_ADIF_BANDS
                     };                                                       ///< enum for bands

using BAND_ENUMERATION_TYPE = std::array<std::string, static_cast<unsigned int>(ADIF_BAND::N_ADIF_BANDS)>;     ///< type for band enumeration

static BAND_ENUMERATION_TYPE BAND_ENUMERATION { "2190m"s,
                                                "560m"s,
                                                "160m"s,
                                                "80m"s,
                                                "60m"s,
                                                "40m"s,
                                                "30m"s,
                                                "20m"s,
                                                "17m"s,
                                                "15m"s,
                                                "12m"s,
                                                "10m"s,
                                                "6m"s,
                                                "4m"s,
                                                "2m"s,
                                                "1.25m"s,
                                                "70cm"s,
                                                "33cm"s,
                                                "23cm"s,
                                                "13cm"s,
                                                "9cm"s,
                                                "6cm"s,
                                                "3cm"s,
                                                "1.25cm"s,
                                                "6mm"s,
                                                "4mm"s,
                                                "2.5mm"s,
                                                "2mm"s,
                                                "1mm"s
                                              };

// contest  -------------------------------------------------------

/// contests
enum class ADIF_CONTEST { SEVENQP,                //      7th-Area QSO Party
                          ANARTS_RTTY,        //      ANARTS WW RTTY
                          ANATOLIAN_RTTY,     //      Anatolian WW RTTY
                          AP_SPRINT,          //      Asia - Pacific Sprint
                          ARI_DX,             //      ARI DX Contest
                          ARRL_10,            //      ARRL 10 Meter Contest
                          ARRL_160,           //      ARRL 160 Meter Contest
                          ARRL_DX_CW,         //      ARRL International DX Contest (CW)
                          ARRL_DX_SSB,        //      ARRL International DX Contest (Phone)
                          ARRL_FIELD_DAY,     //      ARRL Field Day
                          ARRL_RTTY,          //      ARRL RTTY Round-Up
                          ARRL_SS_CW,         //      ARRL November Sweepstakes (CW)
                          ARRL_SS_SSB,        //      ARRL November Sweepstakes (Phone)
                          ARRL_UHF_AUG,       //      ARRL August UHF Contest
                          ARRL_VHF_JAN,       //      ARRL January VHF Sweepstakes
                          ARRL_VHF_JUN,       //      ARRL June VHF QSO Party
                          ARRL_VHF_SEP,       //      ARRL September VHF QSO Party
                          BARTG_RTTY,         //      BARTG Spring RTTY Contest
                          BARTG_SPRINT,       //      BARTG Sprint Contest
                          CA_QSO_PARTY,       //      California QSO Party
                          CQ_160_CW,          //      CQ WW 160 Meter DX Contest (CW)
                          CQ_160_SSB,         //      CQ WW 160 Meter DX Contest (SSB)
                          CQ_VHF,             //      CQ World_Wide VHF Contest
                          CQ_WPX_CW,          //      CQ WW WPX Contest (CW)
                          CQ_WPX_RTTY,        //      CQ/RJ WW RTTY WPX Contest
                          CQ_WPX_SSB,         //      CQ WW WPX Contest (SSB)
                          CQ_WW_CW,           //      CQ WW DX Contest (CW)
                          CQ_WW_RTTY,         //      CQ/RJ WW RTTY DX Contest
                          CQ_WW_SSB,          //      CQ WW DX Contest (SSB)
                          CWOPS_CWT,          //      CWops Mini-CWT Test
                          CIS_DX,             //      CIS DX Contest
                          DARC_WAEDC_CW,      //      WAE DX Contest (CW)
                          DARC_WAEDC_RTTY,    //      WAE DX Contest (RTTY)
                          DARC_WAEDC_SSB,     //      WAE DX Contest (SSB)
                          DL_DX_RTTY,         //      DL-DX RTTY Contest
                          EA_RTTY,            //      EA-WW-RTTY
                          EPC_PSK63,          //      PSK63 QSO Party
                          EU_SPRINT,          //      EU Sprint
                          EUCW160M,           //
                          EU_HF,              //      EU HF Championship
                          EU_PSK_DX,          //      EU PSK DX Contest
                          FALL_sprint,        //      FISTS Fall Sprint
                          FL_QSO_PARTY,       //      Florida QSO Party
                          GA_QSO_PARTY,       //      Georgia QSO Party
                          HELVETIA,           //      Helvetia Contest
                          IARU_HF,            //      IARU HF World Championship
                          IL_QSO_party,       //      Illinois QSO Party
                          JARTS_WW_RTTY,      //      JARTS WW RTTY
                          JIDX_CW,            //      Japan International DX Contest (CW)
                          JIDX_SSB,           //      Japan International DX Contest (SSB)
                          LZ_DX,              //      LZ DX Contest
                          MI_QSO_PARTY,       //      Michigan QSO Party
                          NAQP_CW,            //      North America QSO Party (CW)
                          NAQP_RTTY,          //      North America QSO Party (RTTY)
                          NAQP_SSB,           //      North America QSO Party (Phone)
                          NA_SPRINT_CW,       //      North America Sprint (CW)
                          NA_SPRINT_RTTY,     //      North America Sprint (RTTY)
                          NA_SPRINT_SSB,      //      North America Sprint (Phone)
                          NEQP,               //      New England QSO Party
                          NRAU_BALTIC_CW,     //      NRAU-Baltic Contest (CW)
                          NRAU_BALTIC_SSB,    //      NRAU-Baltic Contest (SSB)
                          OCEANIA_DX_CW,      //      Oceania DX Contest (CW)
                          OCEANIA_DX_SSB,     //      Oceania DX Contest (SSB)
                          OH_QSO_PARTY,       //      Ohio QSO Party
                          OK_DX_RTTY,         //
                          OK_OM_DX,           //      OK-OM DX Contest
                          ON_QSO_PARTY,       //      Ontario QSO Party
                          PACC,               //
                          QC_QSO_PARTY,       //      Quebec QSO Party
                          RAC,                //      Canada Day, RAC Winter contests
                          RDAC,               //      Russian District Award Contest
                          RDXC,               //      Russian DX Contest
                          REF_160M,           //
                          REF_CW,             //
                          REF_SSB,            //
                          RSGB_160,           //      1.8Mhz Contest
                          RSGB_21_28_CW,      //      21/28 MHz Contest (CW)
                          RSGB_21_28_SSB,     //      21/28 MHz Contest (SSB)
                          RSGB_80M_CC,        //      80m Club Championships
                          RSGB_AFS_CW,        //      Affiliated Societies Team Contest (CW)
                          RSGB_AFS_SSB,       //      Affiliated Societies Team Contest (SSB)
                          RSGB_CLUB_CALLS,    //      Club Calls
                          RSGB_COMMONWEALTH,  //      Commonwealth Contest
                          RSGB_IOTA,          //      IOTA Contest
                          RSGB_LOW_POWER,     //      Low Power Field Day
                          RSGB_NFD,           //      National Field Day
                          RSGB_ROPOCO,        //      RoPoCo
                          RSGB_SSB_FD,        //      SSB Field Day
                          RUSSIAN_RTTY,       //
                          SAC_CW,             //      Scandinavian Activity Contest (CW)
                          SAC_SSB,            //      Scandinavian Activity Contest (SSB)
                          SARTG_RTTY,         //      SARTG WW RTTY
                          SCC_RTTY,           //      SCC RTTY Championship
                          SMP_AUG,            //      SSA Portabeltest
                          SMP_MAY,            //      SSA Portabeltest
                          SPDXCCONTEST,       //       SP DX Contest
                          SPRING_SPRINT,      //      FISTS Spring Sprint
                          SR_MARATHON,        //      Scottish-Russian Marathon
                          STEW_PERRY,         //      Stew Perry Topband Distance Challenge
                          SUMMER_SPRINT,      //      FISTS Summer Sprint
                          TARA_RTTY,          //      TARA RTTY Mêlée
                          TMC_RTTY,           //      The Makrothen Contest
                          UBA_DX_CW,          //      UBA Contest (CW)
                          UBA_DX_SSB,         //      UBA Contest (SSB)
                          UK_DX_RTTY,         //      UK DX RTTY Contest
                          UKRAINIAN_DX,       //      Ukrainian DX
                          UKR_CHAMP_RTTY,     //      Open Ukraine RTTY Championship
                          URE_DX,             //
                          VIRGINIA_QSO_PARTY, //      Virginia QSO Party
                          VOLTA_RTTY,         //      Alessandro Volta RTTY DX Contest
                          WI_QSO_PARTY,       //      Wisconsin QSO Party
                          WINTER_SPRINT,      //      FISTS Winter Sprint
                          YUDXC,              //      YU DX Contest
                          N_CONTESTS
                        };

using CONTEST_ENUMERATION_TYPE = std::array<std::string, static_cast<unsigned int>(ADIF_CONTEST::N_CONTESTS)>;                           ///< type for contest enumeration

static CONTEST_ENUMERATION_TYPE CONTEST_ENUMERATION = { "7QP"s,                                //  7th-Area QSO Party
                                                        "ANARTS-RTTY"s,                        //  ANARTS WW RTTY
                                                        "ANATOLIAN-RTTY"s,                     //  Anatolian WW RTTY
                                                        "AP-SPRINT"s,                          //  Asia - Pacific Sprint
                                                        "ARI-DX"s,                             //  ARI DX Contest
                                                        "ARRL-10"s,                            //  ARRL 10 Meter Contest
                                                        "ARRL-160"s,                           //  ARRL 160 Meter Contest
                                                        "ARRL-DX-CW"s,                         //  ARRL International DX Contest (CW)
                                                        "ARRL-DX-SSB"s,                        //  ARRL International DX Contest (Phone)
                                                        "ARRL-FIELD-DAY"s,                     //  ARRL Field Day
                                                        "ARRL-RTTY"s,                          //  ARRL RTTY Round-Up
                                                        "ARRL-SS-CW"s,                         //  ARRL November Sweepstakes (CW)
                                                        "ARRL-SS-SSB"s,                        //  ARRL November Sweepstakes (Phone)
                                                        "ARRL-UHF-AUG"s,                       //  ARRL August UHF Contest
                                                        "ARRL-VHF-JAN"s,                       //  ARRL January VHF Sweepstakes
                                                        "ARRL-VHF-JUN"s,                       //  ARRL June VHF QSO Party
                                                        "ARRL-VHF-SEP"s,                       //  ARRL September VHF QSO Party
                                                        "BARTG-RTTY"s,                         //  BARTG Spring RTTY Contest
                                                        "BARTG-SPRINT"s,                       //  BARTG Sprint Contest
                                                        "CA-QSO-PARTY"s,                       //  California QSO Party
                                                        "CQ-160-CW"s,                          //  CQ WW 160 Meter DX Contest (CW)
                                                        "CQ-160-SSB"s,                         //  CQ WW 160 Meter DX Contest (SSB)
                                                        "CQ-VHF"s,                             //  CQ World-Wide VHF Contest
                                                        "CQ-WPX-CW"s,                          //  CQ WW WPX Contest (CW)
                                                        "CQ-WPX-RTTY"s,                        //  CQ/RJ WW RTTY WPX Contest
                                                        "CQ-WPX-SSB"s,                         //  CQ WW WPX Contest (SSB)
                                                        "CQ-WW-CW"s,                           //  CQ WW DX Contest (CW)
                                                        "CQ-WW-RTTY"s,                         //  CQ/RJ WW RTTY DX Contest
                                                        "CQ-WW-SSB"s,                          //  CQ WW DX Contest (SSB)
                                                        "CWOPS-CWT"s,                          //  CWops Mini-CWT Test
                                                        "CIS-DX"s,                             //  CIS DX Contest
                                                        "DARC-WAEDC-CW"s,                      //  WAE DX Contest (CW)
                                                        "DARC-WAEDC-RTTY"s,                    //  WAE DX Contest (RTTY)
                                                        "DARC-WAEDC-SSB"s,                     //  WAE DX Contest (SSB)
                                                        "DL-DX-RTTY"s,                         //  DL-DX RTTY Contest
                                                        "EA-RTTY"s,                            //  EA-WW-RTTY
                                                        "EPC-PSK63"s,                          //  PSK63 QSO Party
                                                        "EU Sprint"s,                          //  EU Sprint
                                                        "EUCW160M"s,                           //
                                                        "EU-HF"s,                              //  EU HF Championship
                                                        "EU-PSK-DX"s,                          //  EU PSK DX Contest
                                                        "Fall Sprint"s,                        //  FISTS Fall Sprint
                                                        "FL-QSO-PARTY"s,                       //  Florida QSO Party
                                                        "GA-QSO-PARTY"s,                       //  Georgia QSO Party
                                                        "HELVETIA"s,                           //  Helvetia Contest
                                                        "IARU-HF"s,                            //  IARU HF World Championship
                                                        "IL QSO Party"s,                       //  Illinois QSO Party
                                                        "JARTS-WW-RTTY"s,                      //  JARTS WW RTTY
                                                        "JIDX-CW"s,                            //  Japan International DX Contest (CW)
                                                        "JIDX-SSB"s,                           //  Japan International DX Contest (SSB)
                                                        "LZ DX"s,                              //  LZ DX Contest
                                                        "MI-QSO-PARTY"s,                       //  Michigan QSO Party
                                                        "NAQP-CW"s,                            //  North America QSO Party (CW)
                                                        "NAQP-RTTY"s,                          //  North America QSO Party (RTTY)
                                                        "NAQP-SSB"s,                           //  North America QSO Party (Phone)
                                                        "NA-SPRINT-CW"s,                       //  North America Sprint (CW)
                                                        "NA-SPRINT-RTTY"s,                     //  North America Sprint (RTTY)
                                                        "NA-SPRINT-SSB"s,                      //  North America Sprint (Phone)
                                                        "NEQP"s,                               //  New England QSO Party
                                                        "NRAU-BALTIC-CW"s,                     //  NRAU-Baltic Contest (CW)
                                                        "NRAU-BALTIC-SSB"s,                    //  NRAU-Baltic Contest (SSB)
                                                        "OCEANIA-DX-CW"s,                      //  Oceania DX Contest (CW)
                                                        "OCEANIA-DX-SSB"s,                     //  Oceania DX Contest (SSB)
                                                        "OH-QSO-PARTY"s,                       //  Ohio QSO Party
                                                        "OK-DX-RTTY"s,                         //
                                                        "OK-OM-DX"s,                           //  OK-OM DX Contest
                                                        "ON-QSO-PARTY"s,                       //  Ontario QSO Party
                                                        "PACC"s,                               //
                                                        "QC-QSO-PARTY"s,                       //  Quebec QSO Party
                                                        "RAC, CANADA DAY, CANADA WINTER"s,     //  Canada Day, RAC Winter contests
                                                        "RDAC"s,                               //  Russian District Award Contest
                                                        "RDXC"s,                               //  Russian DX Contest
                                                        "REF-160M"s,                           //
                                                        "REF-CW"s,                             //
                                                        "REF-SSB"s,                            //
                                                        "RSGB-160"s,                           //  1.8Mhz (sic) Contest
                                                        "RSGB-21/28-CW"s,                      //  21/28 MHz Contest (CW)
                                                        "RSGB-21/28-SSB"s,                     //  21/28 MHz Contest (SSB)
                                                        "RSGB-80M-CC"s,                        //  80m Club Championships
                                                        "RSGB-AFS-CW"s,                        //  Affiliated Societies Team Contest (CW)
                                                        "RSGB-AFS-SSB"s,                       //  Affiliated Societies Team Contest (SSB)
                                                        "RSGB-CLUB-CALLS"s,                    //  Club Calls
                                                        "RSGB-COMMONWEALTH"s,                  //  Commonwealth Contest
                                                        "RSGB-IOTA"s,                          //  IOTA Contest
                                                        "RSGB-LOW-POWER"s,                     //  Low Power Field Day
                                                        "RSGB-NFD"s,                           //  National Field Day
                                                        "RSGB-ROPOCO"s,                        //  RoPoCo
                                                        "RSGB-SSB-FD"s,                        //  SSB Field Day
                                                        "RUSSIAN-RTTY"s,                       //
                                                        "SAC-CW"s,                             //  Scandinavian Activity Contest (CW)
                                                        "SAC-SSB"s,                            //  Scandinavian Activity Contest (SSB)
                                                        "SARTG-RTTY"s,                         //  SARTG WW RTTY
                                                        "SCC-RTTY"s,                           //  SCC RTTY Championship
                                                        "SMP-AUG"s,                            //  SSA Portabeltest
                                                        "SMP-MAY"s,                            //  SSA Portabeltest
                                                        "SPDXContest"s,                        //  SP DX Contest
                                                        "Spring Sprint"s,                      //  FISTS Spring Sprint
                                                        "SR-MARATHON"s,                        //  Scottish-Russian Marathon
                                                        "STEW-PERRY"s,                         //  Stew Perry Topband Distance Challenge
                                                        "Summer Sprint"s,                      //  FISTS Summer Sprint
                                                        "TARA-RTTY"s,                          //  TARA RTTY Mêlée
                                                        "TMC-RTTY"s,                           //  The Makrothen Contest
                                                        "UBA-DX-CW"s,                          //  UBA Contest (CW)
                                                        "UBA-DX-SSB"s,                         //  UBA Contest (SSB)
                                                        "UK-DX-RTTY"s,                         //  UK DX RTTY Contest
                                                        "UKRAINIAN DX"s,                       //  Ukrainian DX
                                                        "UKR-CHAMP-RTTY"s,                     //  Open Ukraine RTTY Championship
                                                        "URE-DX"s,                             //
                                                        "Virginia QSO Party"s,                 //  Virginia QSO Party
                                                        "VOLTA-RTTY"s,                         //  Alessandro Volta RTTY DX Contest
                                                        "WI-QSO-PARTY"s,                       //  Wisconsin QSO Party
                                                        "Winter Sprint"s,                      //  FISTS Winter Sprint
                                                        "YUDXC"s,                              //  YU DX Contest
                                                      };                                          ///< values for contests

// propagation mode  -------------------------------------------------------

/// propagation modes
enum class ADIF_PROPAGATION_MODE { AUR,         //      Aurora
                                   AUE,         //      Aurora-E
                                   BS,          //      Back scatter
                                   ECH,         //      EchoLink
                                   EME,         //      Earth-Moon-Earth
                                   ES,          //      Sporadic E
                                   FAI,         //      Field Aligned Irregularities
                                   F2,          //      F2 Reflection
                                   INTERNET,    //      Internet-assisted
                                   ION,         //      Ionoscatter
                                   IRL,         //      IRLP
                                   MS,          //      Meteor scatter
                                   RPT,         //      Terrestrial or atmospheric repeater or transponder
                                   RS,          //      Rain scatter
                                   SAT,         //      Satellite
                                   TEP,         //      Trans-equatorial
                                   TR,          //      Tropospheric ducting
                                   N_PROP_MODES
                                 };

using PROPAGATION_MODE_ENUMERATION_TYPE = std::array<std::string, static_cast<unsigned int>(ADIF_PROPAGATION_MODE::N_PROP_MODES)>;    ///< type for propagation mode enumeration

static PROPAGATION_MODE_ENUMERATION_TYPE PROPAGATION_MODE_ENUMERATION = { "AUR"s,          //  Aurora
                                                                          "AUE"s,          //  Aurora-E
                                                                          "BS"s,           //  Back scatter
                                                                          "ECH"s,          //  EchoLink
                                                                          "EME"s,          //  Earth-Moon-Earth
                                                                          "ES"s,           //  Sporadic E
                                                                          "FAI"s,          //  Field Aligned Irregularities
                                                                          "F2"s,           //  F2 Reflection
                                                                          "INTERNET"s,     //  Internet-assisted
                                                                          "ION"s,          //  Ionoscatter
                                                                          "IRL"s,          //  IRLP
                                                                          "MS"s,           //  Meteor scatter
                                                                          "RPT"s,          //  Terrestrial or atmospheric repeater or transponder
                                                                          "RS"s,           //  Rain scatter
                                                                          "SAT"s,          //  Satellite
                                                                          "TEP"s,          //  Trans-equatorial
                                                                          "TR"s            //  Tropospheric ducting
                                                                        };                    ///< values for propagatiom mode

// primary administrative subdivisions  -------------------------------------------------------

/// Canada
enum class PRIMARY_ENUM_CANADA { CANADA_NS,                   // Nova Scotia
                                 CANADA_QC,                   // Québec
                                 CANADA_ON,                   // Ontario
                                 CANADA_MB,                   // Manitoba
                                 CANADA_SK,                   // Saskatchewan
                                 CANADA_AB,                   // ALberta
                                 CANADA_BC,                   // British Columbia
                                 CANADA_NT,                   // Northwest Territories
                                 CANADA_NB,                   // New Brunswick
                                 CANADA_NL,                   // Newfoundland and Labrador
                                 CANADA_YT,                   // Yukon
                                 CANADA_PE,                   // Prince Edward Island
                                 CANADA_NU,                   // Nunavut
                                 N_CANADA_PRIMARIES
                               };                             ///< enum for Canada

using PRIMARY_CANADA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_CANADA::N_CANADA_PRIMARIES)>; ///< type for Canada enumeration

static PRIMARY_CANADA_ENUMERATION_TYPE PRIMARY_CANADA_ENUMERATION = { "NS"s,
                                                                      "QC"s,
                                                                      "ON"s,
                                                                      "MB"s,
                                                                      "AB"s,
                                                                      "BC"s,
                                                                      "NT"s,
                                                                      "NB"s,
                                                                      "NL"s,
                                                                      "YT"s,
                                                                      "PE"s,
                                                                      "NU"s
                                                                    };            ///< values for Canada

/// Aland Is.
enum class PRIMARY_ENUM_ALAND { ALAND_001,    //     Brändö
                                ALAND_002,    //     Eckerö
                                ALAND_003,    //     Finström
                                ALAND_004,    //     Föglö
                                ALAND_005,    //     Geta
                                ALAND_006,    //     Hammarland
                                ALAND_007,    //     Jomala
                                ALAND_008,    //     Kumlinge
                                ALAND_009,    //     Kökar
                                ALAND_010,    //     Lemland
                                ALAND_011,    //     Lumparland
                                ALAND_012,    //     Maarianhamina
                                ALAND_013,    //     Saltvik
                                ALAND_014,    //     Sottunga
                                ALAND_015,    //     Sund
                                ALAND_016,    //     Vårdö
                                N_ALAND_PRIMARIES
                              };

//typedef std::array<std::string, static_class<int>(PRIMARY_ENUM_ALAND::N_ALAND_PRIMARIES)> PRIMARY_ALAND_ENUMERATION_TYPE;    ///< primaries for Aland Is.

using PRIMARY_ALAND_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_ALAND::N_ALAND_PRIMARIES)>;    ///< primaries for Aland Is.

static PRIMARY_ALAND_ENUMERATION_TYPE PRIMARY_ALAND_ENUMERATION = { "001"s,    //     Brändö
                                                                    "002"s,    //     Eckerö
                                                                    "003"s,    //     Finström
                                                                    "004"s,    //     Föglö
                                                                    "005"s,    //     Geta
                                                                    "006"s,    //     Hammarland
                                                                    "007"s,    //     Jomala
                                                                    "008"s,    //     Kumlinge
                                                                    "009"s,    //     Kökar
                                                                    "010"s,    //     Lemland
                                                                    "011"s,    //     Lumparland
                                                                    "012"s,    //     Maarianhamina
                                                                    "013"s,    //     Saltvik
                                                                    "014"s,    //     Sottunga
                                                                    "015"s,    //     Sund
                                                                    "016"s     //     Vårdö
                                                                  };

/// Alaska
enum class PRIMARY_ENUM_ALASKA { AK,    //     ALASKA
                                 N_ALASKA_PRIMARIES
                               };

//typedef std::array<std::string, N_ALASKA_PRIMARIES> PRIMARY_ALASKA_ENUMERATION_TYPE;    ///< primaries for Alaska

using PRIMARY_ALASKA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_ALASKA::N_ALASKA_PRIMARIES)>;    ///< primaries for Alaska

static PRIMARY_ALASKA_ENUMERATION_TYPE PRIMARY_ALASKA_ENUMERATION = { "AK"s    //     Alaska
                                                                    };

/// Asiatic Russia
enum class PRIMARY_ENUM_ASIATIC_RUSSIA { UO,      // 174  Ust\u2019-Ordynsky Autonomous Okrug - for contacts made before 2008-01-01
                                         AB,      // 175  Aginsky Buryatsky Autonomous Okrug - for contacts made before 2008-03-01
                                         CB,      // 165  Chelyabinsk (Chelyabinskaya oblast)
                                         SV,      // 154  Sverdlovskaya oblast
                                         PM,      // 140  Perm` (Permskaya oblast) - for contacts made on or after 2005-12-01
//                                   PM,      // 140  Permskaya Kraj - for contacts made before 2005-12-01
                                         KP,      // 141  Komi-Permyatsky Autonomous Okrug - for contacts made before 2005-12-01
                                         TO,      // 158  Tomsk (Tomskaya oblast)
                                         HM,      // 162  Khanty-Mansyisky Autonomous Okrug
                                         YN,      // 163  Yamalo-Nenetsky Autonomous Okrug
                                         TN,      // 161  Tyumen' (Tyumenskaya oblast)
                                         OM,      // 146  Omsk (Omskaya oblast)
                                         NS,      // 145  Novosibirsk (Novosibirskaya oblast)
                                         KN,      // 134  Kurgan (Kurganskaya oblast)
                                         OB,      // 167  Orenburg (Orenburgskaya oblast)
                                         KE,      // 130  Kemerovo (Kemerovskaya oblast)
                                         BA,      // 84   Republic of Bashkortostan
                                         KO,      // 90   Republic of Komi
                                         AL,      // 99   Altaysky Kraj
                                         GA,      // 100  Republic Gorny Altay
                                         KK,      // 103  Krasnoyarsk (Krasnoyarsk Kraj)
//                                   KK,      // 103  Krasnoyarsk (Krasnoyarsk Kraj) - for contacts made on or after 2007-01-01
                                         TM,      // 105  Taymyr Autonomous Okrug - for contacts made before 2007-01-01
                                         HK,      // 110  Khabarovsk (Khabarovsky Kraj)
                                         EA,      // 111  Yevreyskaya Autonomous Oblast
                                         SL,      // 153  Sakhalin (Sakhalinskaya oblast)
                                         EV,      // 106  Evenkiysky Autonomous Okrug - for contacts made before 2007-01-01
                                         MG,      // 138  Magadan (Magadanskaya oblast)
                                         AM,      // 112  Amurskaya oblast
                                         CK,      // 139  Chukotka Autonomous Okrug
                                         PK,      // 107  Primorsky Kraj
                                         BU,      // 85   Republic of Buryatia
                                         YA,      // 98   Sakha (Yakut) Republic
                                         IR,      // 124  Irkutsk (Irkutskaya oblast)
                                         CT,      // 166  Zabaykalsky Kraj - referred to as Chita (Chitinskaya oblast) before 2008-03-01
                                         HA,      // 104  Republic of Khakassia
                                         KY,      // 129  Koryaksky Autonomous Okrug - for contacts made before 2007-01-01
                                         KT,      // 128  Kamchatka (Kamchatskaya oblast) - for contacts made on or after 2007-01-01
                                         TU,      // 159  Republic of Tuva
//                                   ASIATIC_RUSSIA_KT,      // 128  Kamchatka (Kamchatskaya oblast)
                                         N_ASIATIC_RUSSIA_PRIMARIES
                                       };

//typedef std::array<std::string, N_ASIATIC_RUSSIA_PRIMARIES> PRIMARY_ASIATIC_RUSSIA_ENUMERATION_TYPE;    ///< primaries for Asiatic Russia

using PRIMARY_ASIATIC_RUSSIA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_ASIATIC_RUSSIA::N_ASIATIC_RUSSIA_PRIMARIES)>;    ///< primaries for Asiatic Russia

static PRIMARY_ASIATIC_RUSSIA_ENUMERATION_TYPE PRIMARY_ASIATIC_RUSSIA_ENUMERATION = { "UO"s,      // 174  Ust\u2019-Ordynsky Autonomous Okrug - for contacts made before 2008-01-01
                                                                                      "AB"s,      // 175  Aginsky Buryatsky Autonomous Okrug - for contacts made before 2008-03-01
                                                                                      "CB"s,      // 165  Chelyabinsk (Chelyabinskaya oblast)
                                                                                      "SV"s,      // 154  Sverdlovskaya oblast
                                                                                      "PM"s,      // 140  Perm` (Permskaya oblast) - for contacts made on or after 2005-12-01
//                                   ASIATIC_RUSSIA_PM,      // 140  Permskaya Kraj - for contacts made before 2005-12-01
                                                                                      "KP"s,      // 141  Komi-Permyatsky Autonomous Okrug - for contacts made before 2005-12-01
                                                                                      "TO"s,      // 158  Tomsk (Tomskaya oblast)
                                                                                      "HM"s,      // 162  Khanty-Mansyisky Autonomous Okrug
                                                                                      "YN"s,      // 163  Yamalo-Nenetsky Autonomous Okrug
                                                                                      "TN"s,      // 161  Tyumen' (Tyumenskaya oblast)
                                                                                      "OM"s,      // 146  Omsk (Omskaya oblast)
                                                                                      "NS"s,      // 145  Novosibirsk (Novosibirskaya oblast)
                                                                                      "KN"s,      // 134  Kurgan (Kurganskaya oblast)
                                                                                      "OB"s,      // 167  Orenburg (Orenburgskaya oblast)
                                                                                      "KE"s,      // 130  Kemerovo (Kemerovskaya oblast)
                                                                                      "BA"s,      // 84   Republic of Bashkortostan
                                                                                      "KO"s,      // 90   Republic of Komi
                                                                                      "AL"s,      // 99   Altaysky Kraj
                                                                                      "GA"s,      // 100  Republic Gorny Altay
                                                                                      "KK"s,      // 103  Krasnoyarsk (Krasnoyarsk Kraj)
//                                   ASIATIC_RUSSIA_KK,      // 103  Krasnoyarsk (Krasnoyarsk Kraj) - for contacts made on or after 2007-01-01
                                                                                      "TM"s,      // 105  Taymyr Autonomous Okrug - for contacts made before 2007-01-01
                                                                                      "HK"s,      // 110  Khabarovsk (Khabarovsky Kraj)
                                                                                      "EA"s,      // 111  Yevreyskaya Autonomous Oblast
                                                                                      "SL"s,      // 153  Sakhalin (Sakhalinskaya oblast)
                                                                                      "EV"s,      // 106  Evenkiysky Autonomous Okrug - for contacts made before 2007-01-01
                                                                                      "MG"s,      // 138  Magadan (Magadanskaya oblast)
                                                                                      "AM"s,      // 112  Amurskaya oblast
                                                                                      "CK"s,      // 139  Chukotka Autonomous Okrug
                                                                                      "PK"s,      // 107  Primorsky Kraj
                                                                                      "BU"s,      // 85   Republic of Buryatia
                                                                                      "YA"s,      // 98   Sakha (Yakut) Republic
                                                                                      "IR"s,      // 124  Irkutsk (Irkutskaya oblast)
                                                                                      "CT"s,      // 166  Zabaykalsky Kraj - referred to as Chita (Chitinskaya oblast) before 2008-03-01
                                                                                      "HA"s,      // 104  Republic of Khakassia
                                                                                      "KY"s,      // 129  Koryaksky Autonomous Okrug - for contacts made before 2007-01-01
                                                                                      "KT"s,      // 128  Kamchatka (Kamchatskaya oblast) - for contacts made on or after 2007-01-01
                                                                                      "TU"s       // 159  Republic of Tuva
//                                   ASIATIC_RUSSIA_KT,      // 128  Kamchatka (Kamchatskaya oblast)
                                                                    };

/// Balearic Is. (the spec for some reason calls them "Baleric")
enum class PRIMARY_ENUM_BALEARICS { IB,
                                    N_BALEARICS_PRIMARIES
                                  };

//typedef std::array<std::string, N_BALEARICS_PRIMARIES> PRIMARY_BALEARICS_ENUMERATION_TYPE;    ///< primaries for Balearics

using PRIMARY_BALEARICS_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_BALEARICS::N_BALEARICS_PRIMARIES)>;    ///< primaries for Balearics

static PRIMARY_BALEARICS_ENUMERATION_TYPE PRIMARY_BALEARICS_ENUMERATION = { "IB"s
                                                                          };
/// Belarus
enum class PRIMARY_ENUM_BELARUS { MI,      // Minsk (Minskaya voblasts')
                                  BR,      // Brest (Brestskaya voblasts')
                                  HR,      // Grodno (Hrodzenskaya voblasts')
                                  VI,      // Vitebsk (Vitsyebskaya voblasts')
                                  MA,      // Mogilev (Mahilyowskaya voblasts')
                                  HO,      // Gomel (Homyel'skaya voblasts')
                                  HM,      // Horad Minsk
                                  N_BELARUS_PRIMARIES
                                };

//typedef std::array<std::string, N_BELARUS_PRIMARIES> PRIMARY_BELARUS_ENUMERATION_TYPE;    ///< primaries for Belarus

using PRIMARY_BELARUS_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_BELARUS::N_BELARUS_PRIMARIES)>;    ///< primaries for Belarus

static PRIMARY_BELARUS_ENUMERATION_TYPE PRIMARY_BELARUS_ENUMERATION = { "MI"s,      // Minsk (Minskaya voblasts')
                                                                        "BR"s,      // Brest (Brestskaya voblasts')
                                                                        "HR"s,      // Grodno (Hrodzenskaya voblasts')
                                                                        "VI"s,      // Vitebsk (Vitsyebskaya voblasts')
                                                                        "MA"s,      // Mogilev (Mahilyowskaya voblasts')
                                                                        "HO"s,      // Gomel (Homyel'skaya voblasts')
                                                                        "HM"s       // Horad Minsk
                                                                      };

/// Canary Is.
enum class PRIMARY_ENUM_CANARIES { GC,    // Las Palmas
                                   TF,    // Tenerife
                                   N_CANARIES_PRIMARIES
                                 };

//typedef std::array<std::string, N_CANARIES_PRIMARIES> PRIMARY_CANARIES_ENUMERATION_TYPE;    ///< primaries for Canaries

using PRIMARY_CANARIES_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_CANARIES::N_CANARIES_PRIMARIES)>;    ///< primaries for Canaries


static PRIMARY_CANARIES_ENUMERATION_TYPE PRIMARY_CANARIES_ENUMERATION = { "GC"s,    // Las Palmas
                                                                          "TF"s     // Tenerife
                                                                        };

/// Ceuta y Melilla. (which the standard calls "Cetua & Melilla". Sigh.)
enum class PRIMARY_ENUM_CEUTA { CU,    // Ceuta
                                ML,    // Melilla
                                N_CEUTA_PRIMARIES
                              };

//typedef std::array<std::string, N_CEUTA_PRIMARIES> PRIMARY_CEUTA_ENUMERATION_TYPE;    ///< primaries for Ceuta

using PRIMARY_CEUTA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_CEUTA::N_CEUTA_PRIMARIES)>;    ///< primaries for Ceuta

static PRIMARY_CEUTA_ENUMERATION_TYPE PRIMARY_CEUTA_ENUMERATION = { "CE"s,    // Ceuta
                                                                    "ML"s     // Melilla
                                                                  };

/// Mexico
enum class PRIMARY_ENUM_MEXICO { COL,      // Colima
                                 DF,       //   Distrito Federal
                                 EMX,      //  Estado de México
                                 GTO,      //  Guanajuato
                                 HGO,      //  Hidalgo
                                 JAL,      //  Jalisco
                                 MIC,      //  Michoacán de Ocampo
                                 MOR,      //  Morelos
                                 NAY,      //  Nayarit
                                 PUE,      //  Puebla
                                 QRO,      //  Querétaro de Arteaga
                                 TLX,      //  Tlaxcala
                                 VER,      //  Veracruz-Llave
                                 AGS,      //  Aguascalientes
                                 BC,       //   Baja California
                                 BCS,      //  Baja California Sur
                                 CHH,      //  Chihuahua
                                 COA,      //  Coahuila de Zaragoza
                                 DGO,      //  Durango
                                 NL,       //   Nuevo Leon
                                 SLP,      //  San Luis Potosí
                                 SIN,      //  Sinaloa
                                 SON,      //  Sonora
                                 TMS,      //  Tamaulipas
                                 ZAC,      //  Zacatecas
                                 CAM,      //  Campeche
                                 CHS,      //  Chiapas
                                 GRO,      //  Guerrero
                                 OAX,      //  Oaxaca
                                 QTR,      //  Quintana Roo
                                 TAB,      //  Tabasco
                                 YUC,      //  Yucatán
                                 N_MEXICO_PRIMARIES
                               };

//typedef std::array<std::string, N_MEXICO_PRIMARIES> PRIMARY_MEXICO_ENUMERATION_TYPE;    ///< primaries for Mexico

using PRIMARY_MEXICO_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_MEXICO::N_MEXICO_PRIMARIES)>;    ///< primaries for Mexico

static PRIMARY_MEXICO_ENUMERATION_TYPE PRIMARY_MEXICO_ENUMERATION = { "COL"s,      // Colima
                                                                      "DF"s,       //   Distrito Federal
                                                                      "EMX"s,      //  Estado de México
                                                                      "GTO"s,      //  Guanajuato
                                                                      "HGO"s,      //  Hidalgo
                                                                      "JAL"s,      //  Jalisco
                                                                      "MIC"s,      //  Michoacán de Ocampo
                                                                      "MOR"s,      //  Morelos
                                                                      "NAY"s,      //  Nayarit
                                                                      "PUE"s,      //  Puebla
                                                                      "QRO"s,      //  Querétaro de Arteaga
                                                                      "TLX"s,      //  Tlaxcala
                                                                      "VER"s,      //  Veracruz-Llave
                                                                      "AGS"s,      //  Aguascalientes
                                                                      "BC"s,       //   Baja California
                                                                      "BCS"s,      //  Baja California Sur
                                                                      "CHH"s,      //  Chihuahua
                                                                      "COA"s,      //  Coahuila de Zaragoza
                                                                      "DGO"s,      //  Durango
                                                                      "NL"s,       //   Nuevo Leon
                                                                      "SLP"s,      //  San Luis Potosí
                                                                      "SIN"s,      //  Sinaloa
                                                                      "SON"s,      //  Sonora
                                                                      "TMS"s,      //  Tamaulipas
                                                                      "ZAC"s,      //  Zacatecas
                                                                      "CAM"s,      //  Campeche
                                                                      "CHS"s,      //  Chiapas
                                                                      "GRO"s,      //  Guerrero
                                                                      "OAX"s,      //  Oaxaca
                                                                      "QTR"s,      //  Quintana Roo
                                                                      "TAB"s,      //  Tabasco
                                                                      "YUC"s       //  Yucatán
                                                                    };

/// European Russia
enum class PRIMARY_ENUM_EU_RUSSIA { SP,      // 169  City of St. Petersburg
                                    LO,      // 136  Leningradskaya oblast
                                    KL,      // 88   Republic of Karelia
                                    AR,      // 113  Arkhangelsk (Arkhangelskaya oblast)
                                    NO,      // 114  Nenetsky Autonomous Okrug
                                    VO,      // 120  Vologda (Vologodskaya oblast)
                                    NV,      // 144  Novgorodskaya oblast
                                    PS,      // 149  Pskov (Pskovskaya oblast)
                                    MU,      // 143  Murmansk (Murmanskaya oblast)
                                    MA,      // 170  City of Moscow
                                    MO,      // 142  Moscowskaya oblast
                                    OR,      // 147  Oryel (Orlovskaya oblast)
                                    LP,      // 137  Lipetsk (Lipetskaya oblast)
                                    TV,      // 126  Tver' (Tverskaya oblast)
                                    SM,      // 155  Smolensk (Smolenskaya oblast)
                                    YR,      // 168  Yaroslavl (Yaroslavskaya oblast)
                                    KS,      // 132  Kostroma (Kostromskaya oblast)
                                    TL,      // 160  Tula (Tul'skaya oblast)
                                    VR,      // 121  Voronezh (Voronezhskaya oblast)
                                    TB,      // 157  Tambov (Tambovskaya oblast)
                                    RA,      // 151  Ryazan' (Ryazanskaya oblast)
                                    NN,      // 122  Nizhni Novgorod (Nizhegorodskaya oblast)
                                    IV,      // 123  Ivanovo (Ivanovskaya oblast)
                                    VL,      // 119  Vladimir (Vladimirskaya oblast)
                                    KU,      // 135  Kursk (Kurskaya oblast)
                                    KG,      // 127  Kaluga (Kaluzhskaya oblast)
                                    BR,      // 118  Bryansk (Bryanskaya oblast)
                                    BO,      // 117  Belgorod (Belgorodskaya oblast)
                                    VG,      // 156  Volgograd (Volgogradskaya oblast)
                                    SA,      // 152  Saratov (Saratovskaya oblast)
                                    PE,      // 148  Penza (Penzenskaya oblast)
                                    SR,      // 133  Samara (Samarskaya oblast)
                                    UL,      // 164  Ulyanovsk (Ulyanovskaya oblast)
                                    KI,      // 131  Kirov (Kirovskaya oblast)
                                    TA,      // 94   Republic of Tataria
                                    MR,      // 91   Republic of Marij-El
                                    MD,      // 92   Republic of Mordovia
                                    UD,      // 95   Republic of Udmurtia
                                    CU,      // 97   Republic of Chuvashia
                                    KR,      // 101  Krasnodar (Krasnodarsky Kraj)
                                    KC,      // 109  Republic of Karachaevo-Cherkessia
                                    ST,      // 108  Stavropol' (Stavropolsky Kraj)
                                    KM,      // 89   Republic of Kalmykia
                                    SO,      // 93   Republic of Northern Ossetia
                                    RO,      // 150  Rostov-on-Don (Rostovskaya oblast)
                                    CN,      // 96   Republic Chechnya
                                    IN,      // 96   Republic of Ingushetia
                                    AO,      // 115  Astrakhan' (Astrakhanskaya oblast)
                                    DA,      // 86   Republic of Daghestan
                                    KB,      // 87   Republic of Kabardino-Balkaria
                                    AD,      // 102  Republic of Adygeya
                                    N_EU_RUSSIA_PRIMARIES
                                  };

//typedef std::array<std::string, N_EU_RUSSIA_PRIMARIES> PRIMARY_EU_RUSSIA_ENUMERATION_TYPE;    ///< primaries for European Russia

using PRIMARY_EU_RUSSIA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_EU_RUSSIA::N_EU_RUSSIA_PRIMARIES)>;    ///< primaries for European Russia

static PRIMARY_EU_RUSSIA_ENUMERATION_TYPE PRIMARY_EU_RUSSIA_ENUMERATION = { "SP"s,      // 169  City of St. Petersburg
                                                                            "LO"s,      // 136  Leningradskaya oblast
                                                                            "KL"s,      // 88   Republic of Karelia
                                                                            "AR"s,      // 113  Arkhangelsk (Arkhangelskaya oblast)
                                                                            "NO"s,      // 114  Nenetsky Autonomous Okrug
                                                                            "VO"s,      // 120  Vologda (Vologodskaya oblast)
                                                                            "NV"s,      // 144  Novgorodskaya oblast
                                                                            "PS"s,      // 149  Pskov (Pskovskaya oblast)
                                                                            "MU"s,      // 143  Murmansk (Murmanskaya oblast)
                                                                            "MA"s,      // 170  City of Moscow
                                                                            "MO"s,      // 142  Moscowskaya oblast
                                                                            "OR"s,      // 147  Oryel (Orlovskaya oblast)
                                                                            "LP"s,      // 137  Lipetsk (Lipetskaya oblast)
                                                                            "TV"s,      // 126  Tver' (Tverskaya oblast)
                                                                            "SM"s,      // 155  Smolensk (Smolenskaya oblast)
                                                                            "YR"s,      // 168  Yaroslavl (Yaroslavskaya oblast)
                                                                            "KS"s,      // 132  Kostroma (Kostromskaya oblast)
                                                                            "TL"s,      // 160  Tula (Tul'skaya oblast)
                                                                            "VR"s,      // 121  Voronezh (Voronezhskaya oblast)
                                                                            "TB"s,      // 157  Tambov (Tambovskaya oblast)
                                                                            "RA"s,      // 151  Ryazan' (Ryazanskaya oblast)
                                                                            "NN"s,      // 122  Nizhni Novgorod (Nizhegorodskaya oblast)
                                                                            "IV"s,      // 123  Ivanovo (Ivanovskaya oblast)
                                                                            "VL"s,      // 119  Vladimir (Vladimirskaya oblast)
                                                                            "KU"s,      // 135  Kursk (Kurskaya oblast)
                                                                            "KG"s,      // 127  Kaluga (Kaluzhskaya oblast)
                                                                            "BR"s,      // 118  Bryansk (Bryanskaya oblast)
                                                                            "BO"s,      // 117  Belgorod (Belgorodskaya oblast)
                                                                            "VG"s,      // 156  Volgograd (Volgogradskaya oblast)
                                                                            "SA"s,      // 152  Saratov (Saratovskaya oblast)
                                                                            "PE"s,      // 148  Penza (Penzenskaya oblast)
                                                                            "SR"s,      // 133  Samara (Samarskaya oblast)
                                                                            "UL"s,      // 164  Ulyanovsk (Ulyanovskaya oblast)
                                                                            "KI"s,      // 131  Kirov (Kirovskaya oblast)
                                                                            "TA"s,      // 94   Republic of Tataria
                                                                            "MR"s,      // 91   Republic of Marij-El
                                                                            "MD"s,      // 92   Republic of Mordovia
                                                                            "UD"s,      // 95   Republic of Udmurtia
                                                                            "CU"s,      // 97   Republic of Chuvashia
                                                                            "KR"s,      // 101  Krasnodar (Krasnodarsky Kraj)
                                                                            "KC"s,      // 109  Republic of Karachaevo-Cherkessia
                                                                            "ST"s,      // 108  Stavropol' (Stavropolsky Kraj)
                                                                            "KM"s,      // 89   Republic of Kalmykia
                                                                            "SO"s,      // 93   Republic of Northern Ossetia
                                                                            "RO"s,      // 150  Rostov-on-Don (Rostovskaya oblast)
                                                                            "CN"s,      // 96   Republic Chechnya
                                                                            "IN"s,      // 96   Republic of Ingushetia
                                                                            "AO"s,      // 115  Astrakhan' (Astrakhanskaya oblast)
                                                                            "DA"s,      // 86   Republic of Daghestan
                                                                            "KB"s,      // 87   Republic of Kabardino-Balkaria
                                                                            "AD"s      // 102  Republic of Adygeya
                                                                          };

/// Franz Josef Land
enum class PRIMARY_ENUM_FJL { FJL,
                              N_FJL_PRIMARIES
                            };

//typedef std::array<std::string, N_FJL_PRIMARIES> PRIMARY_FJL_ENUMERATION_TYPE;    ///< primaries for Franz Josef Land

using PRIMARY_FJL_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_FJL::N_FJL_PRIMARIES)>;    ///< primaries for Franz Josef Land

static PRIMARY_FJL_ENUMERATION_TYPE PRIMARY_FJL_ENUMERATION = { "FJL"
                                                              };

/// Argentina
enum class PRIMARY_ENUM_ARGENTINA { C,  // Capital federal (Buenos Aires City)
                                    B,  // Buenos Aires Province
                                    S,  // Santa Fe
                                    H,  // Chaco
                                    P,  // Formosa
                                    X,  // Cordoba
                                    N,  // Misiones
                                    E,  // Entre Rios
                                    T,  // Tucumán
                                    W,  // Corrientes
                                    M,  // Mendoza
                                    G,  // Santiago del Estero
                                    A,  // Salta
                                    J,  // San Juan
                                    D,  // San Luis
                                    K,  // Catamarca
                                    F,  // La Rioja
                                    Y,  // Jujuy
                                    L,  // La Pampa
                                    R,  // Rió Negro
                                    U,  // Chubut
                                    Z,  // Santa Cruz
                                    V,  // Tierra del Fuego
                                    Q,  // Neuquén
                                    N_ARGENTINA_PRIMARIES
                            };

//typedef std::array<std::string, N_ARGENTINA_PRIMARIES> PRIMARY_ARGENTINA_ENUMERATION_TYPE;    ///< primaries for Argentina

using PRIMARY_ARGENTINA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_ARGENTINA::N_ARGENTINA_PRIMARIES)>;    ///< primaries for Argentina

static PRIMARY_ARGENTINA_ENUMERATION_TYPE PRIMARY_ARGENTINA_ENUMERATION = { "C",  // Capital federal (Buenos Aires City)
                                                                            "B",  // Buenos Aires Province
                                                                            "S",  // Santa Fe
                                                                            "H",  // Chaco
                                                                            "P",  // Formosa
                                                                            "X",  // Cordoba
                                                                            "N",  // Misiones
                                                                            "E",  // Entre Rios
                                                                            "T",  // Tucumán
                                                                            "W",  // Corrientes
                                                                            "M",  // Mendoza
                                                                            "G",  // Santiago del Estero
                                                                            "A",  // Salta
                                                                            "J",  // San Juan
                                                                            "D",  // San Luis
                                                                            "K",  // Catamarca
                                                                            "F",  // La Rioja
                                                                            "Y",  // Jujuy
                                                                            "L",  // La Pampa
                                                                            "R",  // Rió Negro
                                                                            "U",  // Chubut
                                                                            "Z",  // Santa Cruz
                                                                            "V",  // Tierra del Fuego
                                                                            "Q"  // Neuquén
                                                                          };

/// Brazil
enum class PRIMARY_ENUM_BRAZIL { ES,      // Espírito Santo
                                 GO,      // Goiás
                                 SC,      // Santa Catarina
                                 SE,      // Sergipe
                                 AL,      // Alagoas
                                 AM,      // Amazonas
                                 TO,      // Tocantins
                                 AP,      // Amapã
                                 PB,      // Paraíba
                                 MA,      // Maranhao
                                 RN,      // Rio Grande do Norte
                                 PI,      // Piaui
                                 DF,      // Oietrito Federal (Brasila)
                                 CE,      // Ceará
                                 AC,      // Acre
                                 MS,      // Mato Grosso do Sul
                                 RR,      // Roraima
                                 RO,      // Rondônia
                                 RJ,      // Rio de Janeiro
                                 SP,      // Sao Paulo
                                 RS,      // Rio Grande do Sul
                                 MG,      // Minas Gerais
                                 PR,      // Paranã
                                 BA,      // Bahia
                                 PE,      // Pernambuco
                                 PA,      // Parã
                                 MT,      // Mato Grosso
                                 N_BRAZIL_PRIMARIES
                               };

//typedef std::array<std::string, N_BRAZIL_PRIMARIES> PRIMARY_BRAZIL_ENUMERATION_TYPE;    ///< primaries for Brazil

using PRIMARY_BRAZIL_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_BRAZIL::N_BRAZIL_PRIMARIES)>;    ///< primaries for Brazil

static PRIMARY_BRAZIL_ENUMERATION_TYPE PRIMARY_BRAZIL_ENUMERATION = { "ES",      // Espírito Santo
                                                                      "GO",      // Goiás
                                                                      "SC",      // Santa Catarina
                                                                      "SE",      // Sergipe
                                                                      "AL",      // Alagoas
                                                                      "AM",      // Amazonas
                                                                      "TO",      // Tocantins
                                                                      "AP",      // Amapã
                                                                      "PB",      // Paraíba
                                                                      "MA",      // Maranhao
                                                                      "RN",      // Rio Grande do Norte
                                                                      "PI",      // Piaui
                                                                      "DF",      // Oietrito Federal (Brasila)
                                                                      "CE",      // Ceará
                                                                      "AC",      // Acre
                                                                      "MS",      // Mato Grosso do Sul
                                                                      "RR",      // Roraima
                                                                      "RO",      // Rondônia
                                                                      "RJ",      // Rio de Janeiro
                                                                      "SP",      // Sao Paulo
                                                                      "RS",      // Rio Grande do Sul
                                                                      "MG",      // Minas Gerais
                                                                      "PR",      // Paranã
                                                                      "BA",      // Bahia
                                                                      "PE",      // Pernambuco
                                                                      "PA",      // Parã
                                                                      "MT"      // Mato Grosso
                                                                    };

/// Hawaii
enum PRIMARY_ENUM_HAWAII { HAWAII_HI,
                           N_HAWAII_PRIMARIES
                         };

//typedef std::array<std::string, N_HAWAII_PRIMARIES> PRIMARY_HAWAII_ENUMERATION_TYPE;    ///< primaries for Hawaii

using PRIMARY_HAWAII_ENUMERATION_TYPE = std::array<std::string, N_HAWAII_PRIMARIES>;    ///< primaries for Hawaii

static PRIMARY_HAWAII_ENUMERATION_TYPE PRIMARY_HAWAII_ENUMERATION = { { "HI"s
                                                                    } };

/// Chile
enum class PRIMARY_ENUM_CHILE { II,  // Antofagasta
                                III,  // Atacama
                                I,    // Tarapacá
                                IV,   // Coquimbo
                                V,    // Valparaíso
                                RM,   // Region Metropolitana de Santiago
                                VI,   // Libertador General Bernardo O'Higgins
                                VII,  // Maule
                                VIII, // Bío-Bío
                                IX,   // La Araucanía
                                X,    // Los Lagos
                                XI,   // Aisén del General Carlos Ibáñez del Campo
                                XII,  // Magallanes
                                N_CHILE_PRIMARIES
                              };

//typedef std::array<std::string, N_CHILE_PRIMARIES> PRIMARY_CHILE_ENUMERATION_TYPE;    ///< primaries for Chile

using PRIMARY_CHILE_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_CHILE::N_CHILE_PRIMARIES)>;    ///< primaries for Chile

static PRIMARY_CHILE_ENUMERATION_TYPE PRIMARY_CHILE_ENUMERATION = { "II"s,   // Antofagasta
                                                                    "III"s,  // Atacama
                                                                    "I"s,    // Tarapacá
                                                                    "IV"s,   // Coquimbo
                                                                    "V"s,    // Valparaíso
                                                                    "RM"s,   // Region Metropolitana de Santiago
                                                                    "VI"s,   // Libertador General Bernardo O'Higgins
                                                                    "VIIs",  // Maule
                                                                    "VIII"s, // Bío-Bío
                                                                    "IX"s,   // La Araucanía
                                                                    "X"s,    // Los Lagos
                                                                    "XI"s,   // Aisén del General Carlos Ibáñez del Campo
                                                                    "XIIs"   // Magallanes
                                                                  };

/// Kaliningrad
enum class PRIMARY_ENUM_KALININGRAD { KA,   // obl. 125 Kalingrad (Kaliningradskaya oblast)
                                      N_KALININGRAD_PRIMARIES
                                    };

//typedef std::array<std::string, N_KALININGRAD_PRIMARIES> PRIMARY_KALININGRAD_ENUMERATION_TYPE;    ///< primaries for Kaliningrad

using PRIMARY_KALININGRAD_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_KALININGRAD::N_KALININGRAD_PRIMARIES)>;    ///< primaries for Kaliningrad

static PRIMARY_KALININGRAD_ENUMERATION_TYPE PRIMARY_KALININGRAD_ENUMERATION = { "KA"s
                                                                              };

/// Paraguay
enum class PRIMARY_ENUM_PARAGUAY { PARAGUAY_16,   // Alto Paraguay
                                   PARAGUAY_19,   // Boquerón
                                   PARAGUAY_15,   // Presidente Hayes
                                   PARAGUAY_13,   // Amambay
                                   PARAGUAY_01,   // Concepción
                                   PARAGUAY_14,   // Canindeyú
                                   PARAGUAY_02,   // San Pedro
                                   PARAGUAY_ASU,  // Asunción
                                   PARAGUAY_11,   // Central
                                   PARAGUAY_03,   // Cordillera
                                   PARAGUAY_09,   // Paraguarí
                                   PARAGUAY_06,   // Caazapl
                                   PARAGUAY_05,   // Caeguazú
                                   PARAGUAY_04,   // Guairá
                                   PARAGUAY_08,   // Miaiones
                                   PARAGUAY_12,   // Ñeembucu
                                   PARAGUAY_10,   // Alto Paraná
                                   PARAGUAY_07,   // Itapua
                                   N_PARAGUAY_PRIMARIES
                                 };

//typedef std::array<std::string, N_PARAGUAY_PRIMARIES> PRIMARY_PARAGUAY_ENUMERATION_TYPE;    ///< primaries for Paraguay

using PRIMARY_PARAGUAY_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_PARAGUAY::N_PARAGUAY_PRIMARIES)>;    ///< primaries for Paraguay

static PRIMARY_PARAGUAY_ENUMERATION_TYPE PRIMARY_PARAGUAY_ENUMERATION = { "16"s,   // Alto Paraguay
                                                                          "19"s,   // Boquerón
                                                                          "15"s,   // Presidente Hayes
                                                                          "13"s,   // Amambay
                                                                          "01"s,   // Concepción
                                                                          "14"s,   // Canindeyú
                                                                          "02"s,   // San Pedro
                                                                          "ASU"s,  // Asunción
                                                                          "11"s,   // Central
                                                                          "03"s,   // Cordillera
                                                                          "09"s,   // Paraguarí
                                                                          "06"s,   // Caazapl
                                                                          "05"s,   // Caeguazú
                                                                          "04"s,   // Guairá
                                                                          "08"s,   // Miaiones
                                                                          "12"s,   // Ñeembucu
                                                                          "10"s,   // Alto Paraná
                                                                          "07"s    // Itapua
                                                                        };

/// ROK
enum class PRIMARY_ENUM_SOUTH_KOREA { A,  // Seoul (Seoul Teugbyeolsi)
                                      N,  // Inchon (Incheon Gwang'yeogsi)
                                      D,  // Kangwon-do (Gang 'weondo)
                                      C,  // Kyunggi-do (Gyeonggido)
                                      E,  // Choongchungbuk-do (Chungcheongbugdo)
                                      F,  // Choongchungnam-do (Chungcheongnamdo)
                                      R,  // Taejon (Daejeon Gwang'yeogsi)
                                      M,  // Cheju-do (Jejudo)
                                      G,  // Chollabuk-do (Jeonrabugdo)
                                      H,  // Chollanam-do (Jeonranamdo)
                                      Q,  // Kwangju (Gwangju Gwang'yeogsi)
                                      K,  // Kyungsangbuk-do (Gyeongsangbugdo)
                                      L,  // Kyungsangnam-do (Gyeongsangnamdo)
                                      B,  // Pusan (Busan Gwang'yeogsi)
                                      P,  // Taegu (Daegu Gwang'yeogsi)
                                      S,  // Ulsan (Ulsan Gwanq'yeogsi)
                                      N_SOUTH_KOREA_PRIMARIES
                                    };

//typedef std::array<std::string, N_SOUTH_KOREA_PRIMARIES> PRIMARY_SOUTH_KOREA_ENUMERATION_TYPE;    ///< primaries for South Korea

using PRIMARY_SOUTH_KOREA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_SOUTH_KOREA::N_SOUTH_KOREA_PRIMARIES)>;    ///< primaries for South Korea

static PRIMARY_SOUTH_KOREA_ENUMERATION_TYPE PRIMARY_SOUTH_KOREA_ENUMERATION = { "A"s,  // Seoul (Seoul Teugbyeolsi)
                                                                                "N"s,  // Inchon (Incheon Gwang'yeogsi)
                                                                                "D"s,  // Kangwon-do (Gang 'weondo)
                                                                                "C"s,  // Kyunggi-do (Gyeonggido)
                                                                                "E"s,  // Choongchungbuk-do (Chungcheongbugdo)
                                                                                "F"s,  // Choongchungnam-do (Chungcheongnamdo)
                                                                                "R"s,  // Taejon (Daejeon Gwang'yeogsi)
                                                                                "M"s,  // Cheju-do (Jejudo)
                                                                                "G"s,  // Chollabuk-do (Jeonrabugdo)
                                                                                "H"s,  // Chollanam-do (Jeonranamdo)
                                                                                "Q"s,  // Kwangju (Gwangju Gwang'yeogsi)
                                                                                "K"s,  // Kyungsangbuk-do (Gyeongsangbugdo)
                                                                                "L"s,  // Kyungsangnam-do (Gyeongsangnamdo)
                                                                                "B"s,  // Pusan (Busan Gwang'yeogsi)
                                                                                "P"s,  // Taegu (Daegu Gwang'yeogsi)
                                                                                "S"s   // Ulsan (Ulsan Gwanq'yeogsi)
                                                                              };

/// Kure
enum class PRIMARY_ENUM_KURE { KI,
                               N_KURE_PRIMARIES
                             };

//typedef std::array<std::string, N_KURE_PRIMARIES> PRIMARY_KURE_ENUMERATION_TYPE;    ///< primaries for Kure

using PRIMARY_KURE_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_KURE::N_KURE_PRIMARIES)>;    ///< primaries for Kure

static PRIMARY_KURE_ENUMERATION_TYPE PRIMARY_KURE_ENUMERATION = { "KI"s
                                                                };

/// Uruguay
enum class PRIMARY_ENUM_URUGUAY { MO, // Montevideo
                                  CA, // Canelones
                                  SJ, // San José
                                  CO, // Colonia
                                  SO, // Soriano
                                  RN, // Rio Negro
                                  PA, // Paysandu
                                  SA, // Salto
                                  AR, // Artigsa
                                  FD, // Florida
                                  FS, // Flores
                                  DU, // Durazno
                                  TA, // Tacuarembo
                                  RV, // Rivera
                                  MA, // Maldonado
                                  LA, // Lavalleja
                                  RO, // Rocha
                                  TT, // Treinta y Tres
                                  CL, // Cerro Largo
                                  N_URUGUAY_PRIMARIES
                                };

//typedef std::array<std::string, N_URUGUAY_PRIMARIES> PRIMARY_URUGUAY_ENUMERATION_TYPE;    ///< primaries for Uruguay

using PRIMARY_URUGUAY_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_URUGUAY::N_URUGUAY_PRIMARIES)>;    ///< primaries for Uruguay

static PRIMARY_URUGUAY_ENUMERATION_TYPE PRIMARY_URUGUAY_ENUMERATION = { "MO"s, // Montevideo
                                                                        "CA"s, // Canelones
                                                                        "SJ"s, // San José
                                                                        "CO"s, // Colonia
                                                                        "SO"s, // Soriano
                                                                        "RN"s, // Rio Negro
                                                                        "PA"s, // Paysandu
                                                                        "SA"s, // Salto
                                                                        "AR"s, // Artigsa
                                                                        "FD"s, // Florida
                                                                        "FS"s, // Flores
                                                                        "DU"s, // Durazno
                                                                        "TA"s, // Tacuarembo
                                                                        "RV"s, // Rivera
                                                                        "MA"s, // Maldonado
                                                                        "LA"s, // Lavalleja
                                                                        "RO"s, // Rocha
                                                                        "TT"s, // Treinta y Tres
                                                                        "CL"s // Cerro Largo
                                                                      };

/// Lord Howe Is.
enum class PRIMARY_ENUM_LORD_HOWE { LH,
                                    N_LORD_HOWE_PRIMARIES
                                  };

//typedef std::array<std::string, N_LORD_HOWE_PRIMARIES> PRIMARY_LORD_HOWE_ENUMERATION_TYPE;    ///< primaries for Lord Howe Is.

using PRIMARY_LORD_HOWE_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_LORD_HOWE::N_LORD_HOWE_PRIMARIES)>;    ///< primaries for Lord Howe Is.

static PRIMARY_LORD_HOWE_ENUMERATION_TYPE PRIMARY_LORD_HOWE_ENUMERATION = { "LH"s
                                                                          };
/// Venezuela
enum class PRIMARY_ENUM_VENEZUELA { AM,     // Amazonas
                                    AN,     // Anzoátegui
                                    AP,     // Apure
                                    AR,     // Aragua
                                    BA,     // Barinas
                                    BO,     // Bolívar
                                    CA,     // Carabobo
                                    CO,     // Cojedes
                                    DA,     // Delta Amacuro
                                    DC,     // Distrito Capital
                                    FA,     // Falcón
                                    GU,     // Guárico
                                    LA,     // Lara
                                    ME,     // Mérida
                                    MI,     // Miranda
                                    MO,     // Monagas
                                    NE,     // Nueva Esparta
                                    PO,     // Portuguesa
                                    SU,     // Sucre
                                    TA,     // Táchira
                                    TR,     // Trujillo
                                    VA,     // Vargas
                                    YA,     // Yaracuy
                                    ZU,     // Zulia
                                    N_VENEZUELA_PRIMARIES
                                  };

//typedef std::array<std::string, N_VENEZUELA_PRIMARIES> PRIMARY_VENEZUELA_ENUMERATION_TYPE;    ///< primaries for Venezuela

using PRIMARY_VENEZUELA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_VENEZUELA::N_VENEZUELA_PRIMARIES)>;    ///< primaries for Venezuela

static PRIMARY_VENEZUELA_ENUMERATION_TYPE PRIMARY_VENEZUELA_ENUMERATION = { "AM"s,     // Amazonas
                                                                            "AN"s,     // Anzoátegui
                                                                            "AP"s,     // Apure
                                                                            "AR"s,     // Aragua
                                                                            "BA"s,     // Barinas
                                                                            "BO"s,     // Bolívar
                                                                            "CA"s,     // Carabobo
                                                                            "CO"s,     // Cojedes
                                                                            "DA"s,     // Delta Amacuro
                                                                            "DC"s,     // Distrito Capital
                                                                            "FA"s,     // Falcón
                                                                            "GU"s,     // Guárico
                                                                            "LA"s,     // Lara
                                                                            "ME"s,     // Mérida
                                                                            "MI"s,     // Miranda
                                                                            "MO"s,     // Monagas
                                                                            "NE"s,     // Nueva Esparta
                                                                            "PO"s,     // Portuguesa
                                                                            "SU"s,     // Sucre
                                                                            "TA"s,     // Táchira
                                                                            "TR"s,     // Trujillo
                                                                            "VA"s,     // Vargas
                                                                            "YA"s,     // Yaracuy
                                                                            "ZU"s      // Zulia
                                                                          };

/// Azores
enum class PRIMARY_ENUM_AZORES { AC,
                                 N_AZORES_PRIMARIES
                               };

//typedef std::array<std::string, N_AZORES_PRIMARIES> PRIMARY_AZORES_ENUMERATION_TYPE;    ///< primaries for Azores

using PRIMARY_AZORES_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_AZORES::N_AZORES_PRIMARIES)>;    ///< primaries for Azores

static PRIMARY_AZORES_ENUMERATION_TYPE PRIMARY_AZORES_ENUMERATION = { "AC"s
                                                                    };

/// Australia
enum class PRIMARY_ENUM_AUSTRALIA { ACT,    // Australian Capital Territory
                                    NSW,    // New South Wales
                                    VIC,    // Victoria
                                    QLD,    // Queensland
                                    SA,     // South Australia
                                    WA,     // Western Australia
                                    TAS,    // Tasmania
                                    NT,     // Northern Territory
                                    N_AUSTRALIA_PRIMARIES
                                 };

//typedef std::array<std::string, N_AUSTRALIA_PRIMARIES> PRIMARY_AUSTRALIA_ENUMERATION_TYPE;    ///< primaries for Australia

using PRIMARY_AUSTRALIA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_AUSTRALIA::N_AUSTRALIA_PRIMARIES)>;    ///< primaries for Australia

static PRIMARY_AUSTRALIA_ENUMERATION_TYPE PRIMARY_AUSTRALIA_ENUMERATION = { "ACT"s,    // Australian Capital Territory
                                                                            "NSW"s,    // New South Wales
                                                                            "VIC"s,    // Victoria
                                                                            "QLD"s,    // Queensland
                                                                            "SA"s,     // South Australia
                                                                            "WA"s,     // Western Australia
                                                                            "TAS"s,    // Tasmania
                                                                            "NT"s,     // Northern Territory
                                                                          };

/// Malyj Vysotskij
enum class PRIMARY_ENUM_MV { MV,
                             N_MV_PRIMARIES
                           };

//typedef std::array<std::string, N_MV_PRIMARIES> PRIMARY_MV_ENUMERATION_TYPE;    ///< primaries for Malyj Vysotskij

using PRIMARY_MV_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_MV::N_MV_PRIMARIES)>;    ///< primaries for Malyj Vysotskij

static PRIMARY_MV_ENUMERATION_TYPE PRIMARY_MV_ENUMERATION = { "MV"s
                                                            };

/// Macquerie Is.
enum class PRIMARY_ENUM_MACQUERIE { MA,
                                    N_MACQUERIE_PRIMARIES
                                  };

//typedef std::array<std::string, N_MACQUERIE_PRIMARIES> PRIMARY_MACQUERIE_ENUMERATION_TYPE;    ///< primaries for Macquerie

using PRIMARY_MACQUERIE_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_MACQUERIE::N_MACQUERIE_PRIMARIES)>;    ///< primaries for Macquerie

static PRIMARY_MACQUERIE_ENUMERATION_TYPE PRIMARY_MACQUERIE_ENUMERATION = { "MA"s
                                                                          };

/// Papua New Guinea
enum class PRIMARY_ENUM_PAPUA_NEW_GUINEA { NCD,     // National Capital District (Port Moresby)
                                           CPM,     // Central
                                           CPK,     // Chimbu
                                           EHG,     // Eastern Highlands
                                           EBR,     // East New Britain
                                           ESW,     // East Sepik
                                           EPW,     // Enga
                                           GPK,     // Gulf
                                           MPM,     // Madang
                                           MRL,     // Manus
                                           MBA,     // Milne Bay
                                           MPL,     // Morobe
                                           NIK,     // New Ireland
                                           NPP,     // Northern
                                           NSA,     // North Solomons
                                           SAN,     // Santaun
                                           SHM,     // Southern Highlands
                                           WPD,     // Western
                                           WHM,     // Western Highlands
                                           WBR,     // West New Britain,
                                           N_PAPUA_NEW_GUINEA_PRIMARIES
                                         };

//typedef std::array<std::string, N_PAPUA_NEW_GUINEA_PRIMARIES> PRIMARY_PAPUA_NEW_GUINEA_ENUMERATION_TYPE;    ///< primaries for Papua New Guinea

using PRIMARY_PAPUA_NEW_GUINEA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_PAPUA_NEW_GUINEA::N_PAPUA_NEW_GUINEA_PRIMARIES)>;    ///< primaries for Papua New Guinea

static PRIMARY_PAPUA_NEW_GUINEA_ENUMERATION_TYPE PRIMARY_PAPUA_NEW_GUINEA_ENUMERATION = { "NCD"s,     // National Capital District (Port Moresby)
                                                                                          "CPM"s,     // Central
                                                                                          "CPK"s,     // Chimbu
                                                                                          "EHG"s,     // Eastern Highlands
                                                                                          "EBR"s,     // East New Britain
                                                                                          "ESW"s,     // East Sepik
                                                                                          "EPW"s,     // Enga
                                                                                          "GPK"s,     // Gulf
                                                                                          "MPM"s,     // Madang
                                                                                          "MRL"s,     // Manus
                                                                                          "MBA"s,     // Milne Bay
                                                                                          "MPL"s,     // Morobe
                                                                                          "NIK"s,     // New Ireland
                                                                                          "NPP"s,     // Northern
                                                                                          "NSA"s,     // North Solomons
                                                                                          "SAN"s,     // Santaun
                                                                                          "SHM"s,     // Southern Highlands
                                                                                          "WPD"s,     // Western
                                                                                          "WHM"s,     // Western Highlands
                                                                                          "WBR"s      // West New Britain,
                                                                                        };

/// New Zealand
enum class PRIMARY_ENUM_NEW_ZEALAND { NCD,    // National Capital District
                                      AUK,    // Auckland
                                      BOP,    // Bay of Plenty
                                      NTL,    // Northland
                                      WKO,    // Waikato
                                      GIS,    // Gisborne
                                      HKB,    // Hawkes Bay
                                      MWT,    // Manawatu-Wanganui
                                      TKI,    // Taranaki
                                      WGN,    // Wellington
                                      CAN,    // Canterbury
                                      MBH,    // Marlborough
                                      NSN,    // Nelson
                                      TAS,    // Tasman
                                      WTC,    // West Coast
                                      OTA,    // Otago
                                      STL,    // Southland
                                      N_NEW_ZEALAND_PRIMARIES
                                    };

//typedef std::array<std::string, N_NEW_ZEALAND_PRIMARIES> PRIMARY_NEW_ZEALAND_ENUMERATION_TYPE;    ///< primaries for New  Zealand

using PRIMARY_NEW_ZEALAND_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_NEW_ZEALAND::N_NEW_ZEALAND_PRIMARIES)>;    ///< primaries for New  Zealand

static PRIMARY_NEW_ZEALAND_ENUMERATION_TYPE PRIMARY_NEW_ZEALAND_ENUMERATION = { "NCD"s,    // National Capital District
                                                                                "AUK"s,    // Auckland
                                                                                "BOP"s,    // Bay of Plenty
                                                                                "NTL"s,    // Northland
                                                                                "WKO"s,    // Waikato
                                                                                "GIS"s,    // Gisborne
                                                                                "HKB"s,    // Hawkes Bay
                                                                                "MWT"s,    // Manawatu-Wanganui
                                                                                "TKI"s,    // Taranaki
                                                                                "WGN"s,    // Wellington
                                                                                "CAN"s,    // Canterbury
                                                                                "MBH"s,    // Marlborough
                                                                                "NSN"s,    // Nelson
                                                                                "TAS"s,    // Tasman
                                                                                "WTC"s,    // West Coast
                                                                                "OTA"s,    // Otago
                                                                                "STL"s     // Southland
                                                                              };

/// Austria
enum class PRIMARY_ENUM_AUSTRIA { WC,   // Wien
                                  HA,   // Hallein
                                  JO,   // St. Johann
                                  SC,   // Salzburg
                                  SL,   // Salzburg-Land
                                  TA,   // Tamsweg
                                  ZE,   // Zell Am See
                                  AM,   // Amstetten
                                  BL,   // Bruck/Leitha
                                  BN,   // Baden
                                  GD,   // Gmünd
                                  GF,   // Gänserndorf
                                  HL,   // Hollabrunn
                                  HO,   // Horn
                                  KO,   // Korneuburg
                                  KR,   // Krems-Region
                                  KS,   // Krems
                                  LF,   // Lilienfeld
                                  MD,   // Mödling
                                  ME,   // Melk
                                  MI,   // Mistelbach
                                  NK,   // Neunkirchen
                                  PC,   // St. Pölten
                                  PL,   // St. Pölten-Land
                                  SB,   // Scheibbs
                                  SW,   // Schwechat
                                  TU,   // Tulln
                                  WB,   // Wr.Neustadt-Bezirk
                                  WN,   // Wr.Neustadt
                                  WT,   // Waidhofen/Thaya
                                  WU,   // Wien-Umgebung
                                  WY,   // Waidhofen/Ybbs
                                  ZT,   // Zwettl
                                  EC,   // Eisenstadt
                                  EU,   // Eisenstadt-Umgebung
                                  GS,   // Güssing
                                  JE,   // Jennersdorf
                                  MA,   // Mattersburg
                                  ND,   // Neusiedl/See
                                  OP,   // Oberpullendorf
                                  OW,   // Oberwart
                                  BR,   // Braunau/Inn
                                  EF,   // Eferding
                                  FR,   // Freistadt
                                  GM,   // Gmunden
                                  GR,   // Grieskirchen
                                  KI,   // Kirchdorf
                                  LC,   // Linz
                                  LL,   // Linz-Land
                                  PE,   // Perg
                                  RI,   // Ried/Innkreis
                                  RO,   // Rohrbach
                                  SD,   // Schärding
                                  SE,   // Steyr-Land
                                  SR,   // Steyr
                                  UU,   // Urfahr
                                  VB,   // Vöcklabruck
                                  WE,   // Wels
                                  WL,   // Wels-Land
                                  BA,   // Bad Aussee
                                  BM,   // Bruck/Mur
                                  DL,   // Deutschlandsberg
                                  FB,   // Feldbach
                                  FF,   // Fürstenfeld
                                  GB,   // Gröbming
                                  GC,   // Graz
                                  GU,   // Graz-Umgebung
                                  HB,   // Hartberg
                                  JU,   // Judenburg
                                  KF,   // Knittelfeld
                                  LB,   // Leibnitz
                                  LE,   // Leoben
                                  LI,   // Liezen
                                  LN,   // Leoben-Land
                                  MU,   // Murau
                                  MZ,   // Mürzzuschlag
                                  RA,   // Radkersburg
                                  VO,   // Voitsberg
                                  WZ,   // Weiz
                                  IC,   // Innsbruck
                                  IL,   // Innsbruck-Land
                                  IM,   // Imst
                                  KB,   // Kitzbühel
                                  KU,   // Kufstein
                                  LA,   // Landeck
                                  LZ,   // Lienz
                                  RE,   // Reutte
                                  SZ,   // Schwaz
                                  FE,   // Feldkirchen
                                  HE,   // Hermagor
                                  KC,   // Klagenfurt
                                  KL,   // Klagenfurt-Land
                                  SP,   // Spittal/Drau
                                  SV,   // St.Veit/Glan
                                  VI,   // Villach
                                  VK,   // Völkermarkt
                                  VL,   // Villach-Land
                                  WO,   // Wolfsberg
                                  BC,   // Bregenz
                                  BZ,   // Bludenz
                                  DO,   // Dornbirn
                                  FK,   //Feldkirch
                                  N_AUSTRIA_PRIMARIES
                                };

//typedef std::array<std::string, N_AUSTRIA_PRIMARIES> PRIMARY_AUSTRIA_ENUMERATION_TYPE;    ///< primaries for Austria

using PRIMARY_AUSTRIA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_AUSTRIA::N_AUSTRIA_PRIMARIES)>;    ///< primaries for Austria

static PRIMARY_AUSTRIA_ENUMERATION_TYPE PRIMARY_AUSTRIA_ENUMERATION = { "WC"s,   // Wien
                                                                        "HA"s,   // Hallein
                                                                        "JO"s,   // St. Johann
                                                                        "SC"s,   // Salzburg
                                                                        "SL"s,   // Salzburg-Land
                                                                        "TA"s,   // Tamsweg
                                                                        "ZE"s,   // Zell Am See
                                                                        "AM"s,   // Amstetten
                                                                        "BL"s,   // Bruck/Leitha
                                                                        "BN"s,   // Baden
                                                                        "GD"s,   // Gmünd
                                                                        "GF"s,   // Gänserndorf
                                                                        "HL"s,   // Hollabrunn
                                                                        "HO"s,   // Horn
                                                                        "KO"s,   // Korneuburg
                                                                        "KR"s,   // Krems-Region
                                                                        "KS"s,   // Krems
                                                                        "LF"s,   // Lilienfeld
                                                                        "MD"s,   // Mödling
                                                                        "ME"s,   // Melk
                                                                        "MI"s,   // Mistelbach
                                                                        "NK"s,   // Neunkirchen
                                                                        "PC"s,   // St. Pölten
                                                                        "PL"s,   // St. Pölten-Land
                                                                        "SB"s,   // Scheibbs
                                                                        "SW"s,   // Schwechat
                                                                        "TU"s,   // Tulln
                                                                        "WB"s,   // Wr.Neustadt-Bezirk
                                                                        "WN"s,   // Wr.Neustadt
                                                                        "WT"s,   // Waidhofen/Thaya
                                                                        "WU"s,   // Wien-Umgebung
                                                                        "WY"s,   // Waidhofen/Ybbs
                                                                        "ZT"s,   // Zwettl
                                                                        "EC"s,   // Eisenstadt
                                                                        "EU"s,   // Eisenstadt-Umgebung
                                                                        "GS"s,   // Güssing
                                                                        "JE"s,   // Jennersdorf
                                                                        "MA"s,   // Mattersburg
                                                                        "ND"s,   // Neusiedl/See
                                                                        "OP"s,   // Oberpullendorf
                                                                        "OW"s,   // Oberwart
                                                                        "BR"s,   // Braunau/Inn
                                                                        "EF"s,   // Eferding
                                                                        "FR"s,   // Freistadt
                                                                        "GM"s,   // Gmunden
                                                                        "GR"s,   // Grieskirchen
                                                                        "KI"s,   // Kirchdorf
                                                                        "LC"s,   // Linz
                                                                        "LL"s,   // Linz-Land
                                                                        "PE"s,   // Perg
                                                                        "RI"s,   // Ried/Innkreis
                                                                        "RO"s,   // Rohrbach
                                                                        "SD"s,   // Schärding
                                                                        "SE"s,   // Steyr-Land
                                                                        "SR"s,   // Steyr
                                                                        "UU"s,   // Urfahr
                                                                        "VB"s,   // Vöcklabruck
                                                                        "WE"s,   // Wels
                                                                        "WL"s,   // Wels-Land
                                                                        "BA"s,   // Bad Aussee
                                                                        "BM"s,   // Bruck/Mur
                                                                        "DL"s,   // Deutschlandsberg
                                                                        "FB"s,   // Feldbach
                                                                        "FF"s,   // Fürstenfeld
                                                                        "GB"s,   // Gröbming
                                                                        "GC"s,   // Graz
                                                                        "GU"s,   // Graz-Umgebung
                                                                        "HB"s,   // Hartberg
                                                                        "JU"s,   // Judenburg
                                                                        "KF"s,   // Knittelfeld
                                                                        "LB"s,   // Leibnitz
                                                                        "LE"s,   // Leoben
                                                                        "LI"s,   // Liezen
                                                                        "LN"s,   // Leoben-Land
                                                                        "MU"s,   // Murau
                                                                        "MZ"s,   // Mürzzuschlag
                                                                        "RA"s,   // Radkersburg
                                                                        "VO"s,   // Voitsberg
                                                                        "WZ"s,   // Weiz
                                                                        "IC"s,   // Innsbruck
                                                                        "IL"s,   // Innsbruck-Land
                                                                        "IM"s,   // Imst
                                                                        "KB"s,   // Kitzbühel
                                                                        "KU"s,   // Kufstein
                                                                        "LA"s,   // Landeck
                                                                        "LZ"s,   // Lienz
                                                                        "RE"s,   // Reutte
                                                                        "SZ"s,   // Schwaz
                                                                        "FE"s,   // Feldkirchen
                                                                        "HE"s,   // Hermagor
                                                                        "KC"s,   // Klagenfurt
                                                                        "KL"s,   // Klagenfurt-Land
                                                                        "SP"s,   // Spittal/Drau
                                                                        "SV"s,   // St.Veit/Glan
                                                                        "VI"s,   // Villach
                                                                        "VK"s,   // Völkermarkt
                                                                        "VL"s,   // Villach-Land
                                                                        "WO"s,   // Wolfsberg
                                                                        "BC"s,   // Bregenz
                                                                        "BZ"s,   // Bludenz
                                                                        "DO"s,   // Dornbirn
                                                                        "FK"s    //Feldkirch
                                                                      };

/// Belgium
enum class PRIMARY_ENUM_BELGIUM { AN,     // Antwerpen
                                  BR,     // Brussels
                                  BW,     // Brabant Wallon
                                  HT,     // Hainaut
                                  LB,     // Limburg
                                  LG,     // Liêge
                                  NM,     // Namur
                                  LU,     // Luxembourg
                                  OV,     // Oost-Vlaanderen
                                  VB,     // Vlaams Brabant
                                  WZ,     // West-Vlaanderen
                                  N_BELGIUM_PRIMARIES
                               };

//typedef std::array<std::string, N_BELGIUM_PRIMARIES> PRIMARY_BELGIUM_ENUMERATION_TYPE;    ///< primaries for Belgium

using PRIMARY_BELGIUM_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_BELGIUM::N_BELGIUM_PRIMARIES)>;    ///< primaries for Belgium

static PRIMARY_BELGIUM_ENUMERATION_TYPE PRIMARY_BELGIUM_ENUMERATION = { "AN"s,     // Antwerpen
                                                                        "BR"s,     // Brussels
                                                                        "BW"s,     // Brabant Wallon
                                                                        "HT"s,     // Hainaut
                                                                        "LB"s,     // Limburg
                                                                        "LG"s,     // Liêge
                                                                        "NM"s,     // Namur
                                                                        "LU"s,     // Luxembourg
                                                                        "OV"s,     // Oost-Vlaanderen
                                                                        "VB"s,     // Vlaams Brabant
                                                                        "WZ"s     // West-Vlaanderen
                                                                      };

/// Bulgaria
enum class PRIMARY_ENUM_BULGARIA { BU,   // Burgas
                                   SL,   // Sliven
                                   YA,   // Yambol (Jambol)
                                   SO,   // Sofija Grad
                                   HA,   // Haskovo
                                   KA,   // Kărdžali
                                   SZ,   // Stara Zagora
                                   PA,   // Pazardžik
                                   PD,   // Plovdiv
                                   SM,   // Smoljan
                                   BL,   // Blagoevgrad
                                   KD,   // Kjustendil
                                   PK,   // Pernik
                                   SF,   // Sofija (Sofia)
                                   GA,   // Gabrovo
                                   LV,   // Loveč (Lovech)
                                   PL,   // Pleven
                                   VT,   // Veliko Tărnovo
                                   MN,   // Montana
                                   VD,   // Vidin
                                   VR,   // Vraca
                                   RZ,   // Razgrad
                                   RS,   // Ruse
                                   SS,   // Silistra
                                   TA,   // Tărgovište
                                   DO,   // Dobrič
                                   SN,   // Šumen
                                   VN,   //Varna
                                   N_BULGARIA_PRIMARIES
                                 };

//typedef std::array<std::string, N_BULGARIA_PRIMARIES> PRIMARY_BULGARIA_ENUMERATION_TYPE;    ///< primaries for Bulgaria

using PRIMARY_BULGARIA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_BULGARIA::N_BULGARIA_PRIMARIES)>;    ///< primaries for Bulgaria

static PRIMARY_BULGARIA_ENUMERATION_TYPE PRIMARY_BULGARIA_ENUMERATION = { "BU"s,   // Burgas
                                                                          "SL"s,   // Sliven
                                                                          "YA"s,   // Yambol (Jambol)
                                                                          "SO"s,   // Sofija Grad
                                                                          "HA"s,   // Haskovo
                                                                          "KA"s,   // Kărdžali
                                                                          "SZ"s,   // Stara Zagora
                                                                          "PA"s,   // Pazardžik
                                                                          "PD"s,   // Plovdiv
                                                                          "SM"s,   // Smoljan
                                                                          "BL"s,   // Blagoevgrad
                                                                          "KD"s,   // Kjustendil
                                                                          "PK"s,   // Pernik
                                                                          "SF"s,   // Sofija (Sofia)
                                                                          "GA"s,   // Gabrovo
                                                                          "LV"s,   // Loveč (Lovech)
                                                                          "PL"s,   // Pleven
                                                                          "VT"s,   // Veliko Tărnovo
                                                                          "MN"s,   // Montana
                                                                          "VD"s,   // Vidin
                                                                          "VR"s,   // Vraca
                                                                          "RZ"s,   // Razgrad
                                                                          "RS"s,   // Ruse
                                                                          "SS"s,   // Silistra
                                                                          "TA"s,   // Tărgovište
                                                                          "DO"s,   // Dobrič
                                                                          "SN"s,   // Šumen
                                                                          "VN"s    //Varna
                                                                        };

/// Corsica
enum class PRIMARY_ENUM_CORSICA { CORSICA_2A, // Corse-du-Sud
                                  CORSICA_2B, // Haute-Corse
                                  N_CORSICA_PRIMARIES
                                };

//typedef std::array<std::string, N_CORSICA_PRIMARIES> PRIMARY_CORSICA_ENUMERATION_TYPE;    ///< primaries for Corsica

using PRIMARY_CORSICA_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_CORSICA::N_CORSICA_PRIMARIES)>;    ///< primaries for Corsica

static PRIMARY_CORSICA_ENUMERATION_TYPE PRIMARY_CORSICA_ENUMERATION = { "2A"s, // Corse-du-Sud
                                                                        "2B"s  // Haute-Corse
                                                                      };

/// Denmark
enum class PRIMARY_ENUM_DENMARK { DENMARK_015,  // Koebenhavns amt
                                  DENMARK_020,  // Frederiksborg amt
                                  DENMARK_025,  // Roskilde amt
                                  DENMARK_030,  // Vestsjaellands amt
                                  DENMARK_035,  // Storstrøm amt (Storstroems)
                                  DENMARK_040,  // Bornholms amt
                                  DENMARK_042,  // Fyns amt
                                  DENMARK_050,  // Sínderjylland amt (Sydjyllands)
                                  DENMARK_055,  // Ribe amt
                                  DENMARK_060,  // Vejle amt
                                  DENMARK_065,  // Ringkøbing amt (Ringkoebing)
                                  DENMARK_070,  // Århus amt (Aarhus)
                                  DENMARK_076,  // Viborg amt
                                  DENMARK_080,  // Nordjyllands amt
                                  DENMARK_101,  // Copenhagen City
                                  DENMARK_147,  // Frederiksberg
                                  N_DENMARK_PRIMARIES
                               };

//typedef std::array<std::string, N_DENMARK_PRIMARIES> PRIMARY_DENMARK_ENUMERATION_TYPE;    ///< primaries for Denmark

using PRIMARY_DENMARK_ENUMERATION_TYPE = std::array<std::string, static_cast<int>(PRIMARY_ENUM_DENMARK::N_DENMARK_PRIMARIES)>;    ///< primaries for Denmark

static PRIMARY_DENMARK_ENUMERATION_TYPE PRIMARY_DENMARK_ENUMERATION = { "015"s,  // Koebenhavns amt
                                                                        "020"s,  // Frederiksborg amt
                                                                        "025"s,  // Roskilde amt
                                                                        "030"s,  // Vestsjaellands amt
                                                                        "035"s,  // Storstrøm amt (Storstroems)
                                                                        "040"s,  // Bornholms amt
                                                                        "042"s,  // Fyns amt
                                                                        "050"s,  // Sínderjylland amt (Sydjyllands)
                                                                        "055"s,  // Ribe amt
                                                                        "060"s,  // Vejle amt
                                                                        "065"s,  // Ringkøbing amt (Ringkoebing)
                                                                        "070"s,  // Århus amt (Aarhus)
                                                                        "076"s,  // Viborg amt
                                                                        "080"s,  // Nordjyllands amt
                                                                        "101"s,  // Copenhagen City
                                                                        "147"s   // Frederiksberg
                                                                      };

/// Finland
enum PRIMARY_ENUM_FINLAND { FINLAND_100,    // Somero
                            FINLAND_102,    // Alastaro
                            FINLAND_103,    // Askainen
                            FINLAND_104,    // Aura
                            FINLAND_105,    // Dragsfjärd
                            FINLAND_106,    // Eura
                            FINLAND_107,    // Eurajoki
                            FINLAND_108,    // Halikko
                            FINLAND_109,    // Harjavalta
                            FINLAND_110,    // Honkajoki
                            FINLAND_111,    // Houtskari
                            FINLAND_112,    // Huittinen
                            FINLAND_115,    // Iniö
                            FINLAND_116,    // Jämijärvi
                            FINLAND_117,    // Kaarina
                            FINLAND_119,    // Kankaanpää
                            FINLAND_120,    // Karinainen
                            FINLAND_122,    // Karvia
                            FINLAND_123,    // Äetsä
                            FINLAND_124,    // Kemiö
                            FINLAND_126,    // Kiikala
                            FINLAND_128,    // Kiikoinen
                            FINLAND_129,    // Kisko
                            FINLAND_130,    // Kiukainen
                            FINLAND_131,    // Kodisjoki
                            FINLAND_132,    // Kokemäki
                            FINLAND_133,    // Korppoo
                            FINLAND_134,    // Koski tl
                            FINLAND_135,    // Kullaa
                            FINLAND_136,    // Kustavi
                            FINLAND_137,    // Kuusjoki
                            FINLAND_138,    // Köyliö
                            FINLAND_139,    // Laitila
                            FINLAND_140,    // Lappi
                            FINLAND_141,    // Lavia
                            FINLAND_142,    // Lemu
                            FINLAND_143,    // Lieto
                            FINLAND_144,    // Loimaa
                            FINLAND_145,    // Loimaan kunta
                            FINLAND_147,    // Luvia
                            FINLAND_148,    // Marttila
                            FINLAND_149,    // Masku
                            FINLAND_150,    // Mellilä
                            FINLAND_151,    // Merikarvia
                            FINLAND_152,    // Merimasku
                            FINLAND_154,    // Mietoinen
                            FINLAND_156,    // Muurla
                            FINLAND_157,    // Mynämäki
                            FINLAND_158,    // Naantali
                            FINLAND_159,    // Nakkila
                            FINLAND_160,    // Nauvo
                            FINLAND_161,    // Noormarkku
                            FINLAND_162,    // Nousiainen
                            FINLAND_163,    // Oripää
                            FINLAND_164,    // Paimio
                            FINLAND_165,    // Parainen
                            FINLAND_167,    // Perniö
                            FINLAND_168,    // Pertteli
                            FINLAND_169,    // Piikkiö
                            FINLAND_170,    // Pomarkku
                            FINLAND_171,    // Pori
                            FINLAND_172,    // Punkalaidun
                            FINLAND_173,    // Pyhäranta
                            FINLAND_174,    // Pöytyä
                            FINLAND_175,    // Raisio
                            FINLAND_176,    // Rauma
                            FINLAND_178,    // Rusko
                            FINLAND_179,    // Rymättylä
                            FINLAND_180,    // Salo
                            FINLAND_181,    // Sauvo
                            FINLAND_182,    // Siikainen
                            FINLAND_183,    // Suodenniemi
                            FINLAND_184,    // Suomusjärvi
                            FINLAND_185,    // Säkylä
                            FINLAND_186,    // Särkisalo
                            FINLAND_187,    // Taivassalo
                            FINLAND_188,    // Tarvasjoki
                            FINLAND_189,    // Turku
                            FINLAND_190,    // Ulvila
                            FINLAND_191,    // Uusikaupunki
                            FINLAND_192,    // Vahto
                            FINLAND_193,    // Vammala
                            FINLAND_194,    // Vampula
                            FINLAND_195,    // Vehmaa
                            FINLAND_196,    // Velkua
                            FINLAND_198,    // Västanfjärd
                            FINLAND_199,    // Yläne
                            FINLAND_201,    // Artjärvi
                            FINLAND_202,    // Askola
                            FINLAND_204,    // Espoo
                            FINLAND_205,    // Hanko
                            FINLAND_206,    // Helsinki
                            FINLAND_207,    // Hyvinkää
                            FINLAND_208,    // Inkoo
                            FINLAND_209,    // Järvenpää
                            FINLAND_210,    // Karjaa
                            FINLAND_211,    // Karjalohja
                            FINLAND_212,    // Karkkila
                            FINLAND_213,    // Kauniainen
                            FINLAND_214,    // Kerava
                            FINLAND_215,    // Kirkkonummi
                            FINLAND_216,    // Lapinjärvi
                            FINLAND_217,    // Liljendal
                            FINLAND_218,    // Lohjan kaupunki
                            FINLAND_220,    // Loviisa
                            FINLAND_221,    // Myrskylä
                            FINLAND_222,    // Mäntsälä
                            FINLAND_223,    // Nummi-Pusula
                            FINLAND_224,    // Nurmijärvi
                            FINLAND_225,    // Orimattila
                            FINLAND_226,    // Pernaja
                            FINLAND_227,    // Pohja
                            FINLAND_228,    // Pornainen
                            FINLAND_229,    // Porvoo
                            FINLAND_231,    // Pukkila
                            FINLAND_233,    // Ruotsinpyhtää
                            FINLAND_234,    // Sammatti
                            FINLAND_235,    // Sipoo
                            FINLAND_236,    // Siuntio
                            FINLAND_238,    // Tammisaari
                            FINLAND_241,    // Tuusula
                            FINLAND_242,    // Vantaa
                            FINLAND_243,    // Vihti
                            FINLAND_301,    // Asikkala
                            FINLAND_303,    // Forssa
                            FINLAND_304,    // Hattula
                            FINLAND_305,    // Hauho
                            FINLAND_306,    // Hausjärvi
                            FINLAND_307,    // Hollola
                            FINLAND_308,    // Humppila
                            FINLAND_309,    // Hämeenlinna
                            FINLAND_310,    // Janakkala
                            FINLAND_311,    // Jokioinen
                            FINLAND_312,    // Juupajoki
                            FINLAND_313,    // Kalvola
                            FINLAND_314,    // Kangasala
                            FINLAND_315,    // Hämeenkoski
                            FINLAND_316,    // Kuhmalahti
                            FINLAND_318,    // Kuru
                            FINLAND_319,    // Kylmäkoski
                            FINLAND_320,    // Kärkölä
                            FINLAND_321,    // Lahti
                            FINLAND_322,    // Lammi
                            FINLAND_323,    // Lempäälä
                            FINLAND_324,    // Loppi
                            FINLAND_325,    // Luopioinen
                            FINLAND_326,    // Längelmäki
                            FINLAND_327,    // Mänttä
                            FINLAND_328,    // Nastola
                            FINLAND_329,    // Nokia
                            FINLAND_330,    // Orivesi
                            FINLAND_331,    // Padasjoki
                            FINLAND_332,    // Pirkkala
                            FINLAND_333,    // Pälkäne
                            FINLAND_334,    // Renko
                            FINLAND_335,    // Riihimäki
                            FINLAND_336,    // Ruovesi
                            FINLAND_337,    // Sahalahti
                            FINLAND_340,    // Tammela
                            FINLAND_341,    // Tampere
                            FINLAND_342,    // Toijala
                            FINLAND_344,    // Tuulos
                            FINLAND_345,    // Urjala
                            FINLAND_346,    // Valkeakoski
                            FINLAND_347,    // Vesilahti
                            FINLAND_348,    // Viiala
                            FINLAND_349,    // Vilppula
                            FINLAND_350,    // Virrat
                            FINLAND_351,    // Ylöjärvi
                            FINLAND_352,    // Ypäjä
                            FINLAND_353,    // Hämeenkyrö
                            FINLAND_354,    // Ikaalinen
                            FINLAND_355,    // Kihniö
                            FINLAND_356,    // Mouhijärvi
                            FINLAND_357,    // Parkano
                            FINLAND_358,    // Viljakkala
                            FINLAND_402,    // Enonkoski
                            FINLAND_403,    // Hartola
                            FINLAND_404,    // Haukivuori
                            FINLAND_405,    // Heinola
                            FINLAND_407,    // Heinävesi
                            FINLAND_408,    // Hirvensalmi
                            FINLAND_409,    // Joroinen
                            FINLAND_410,    // Juva
                            FINLAND_411,    // Jäppilä
                            FINLAND_412,    // Kangaslampi
                            FINLAND_413,    // Kangasniemi
                            FINLAND_414,    // Kerimäki
                            FINLAND_415,    // Mikkeli
                            FINLAND_417,    // Mäntyharju
                            FINLAND_418,    // Pertunmaa
                            FINLAND_419,    // Pieksämäki
                            FINLAND_420,    // Pieksänmaa
                            FINLAND_421,    // Punkaharju
                            FINLAND_422,    // Puumala
                            FINLAND_423,    // Rantasalmi
                            FINLAND_424,    // Ristiina
                            FINLAND_425,    // Savonlinna
                            FINLAND_426,    // Savonranta
                            FINLAND_427,    // Sulkava
                            FINLAND_428,    // Sysmä
                            FINLAND_502,    // Elimäki
                            FINLAND_503,    // Hamina
                            FINLAND_504,    // Iitti
                            FINLAND_505,    // Imatra
                            FINLAND_506,    // Jaala
                            FINLAND_507,    // Joutseno
                            FINLAND_509,    // Kotka
                            FINLAND_510,    // Kouvola
                            FINLAND_511,    // Kuusankoski
                            FINLAND_513,    // Lappeenranta
                            FINLAND_514,    // Lemi
                            FINLAND_515,    // Luumäki
                            FINLAND_516,    // Miehikkälä
                            FINLAND_518,    // Parikkala
                            FINLAND_519,    // Pyhtää
                            FINLAND_520,    // Rautjärvi
                            FINLAND_521,    // Ruokolahti
                            FINLAND_522,    // Saari
                            FINLAND_523,    // Savitaipale
                            FINLAND_525,    // Suomenniemi
                            FINLAND_526,    // Taipalsaari
                            FINLAND_527,    // Uukuniemi
                            FINLAND_528,    // Valkeala
                            FINLAND_530,    // Virolahti
                            FINLAND_531,    // Ylämaa
                            FINLAND_532,    // Anjalankoski
                            FINLAND_601,    // Alahärmä
                            FINLAND_602,    // Alajärvi
                            FINLAND_603,    // Alavus
                            FINLAND_604,    // Evijärvi
                            FINLAND_605,    // Halsua
                            FINLAND_606,    // Hankasalmi
                            FINLAND_607,    // Himanka
                            FINLAND_608,    // Ilmajoki
                            FINLAND_609,    // Isojoki
                            FINLAND_610,    // Isokyrö
                            FINLAND_611,    // Jalasjärvi
                            FINLAND_612,    // Joutsa
                            FINLAND_613,    // Jurva
                            FINLAND_614,    // Jyväskylä
                            FINLAND_615,    // Jyväskylän mlk
                            FINLAND_616,    // Jämsä
                            FINLAND_617,    // Jämsänkoski
                            FINLAND_619,    // Kannonkoski
                            FINLAND_620,    // Kannus
                            FINLAND_621,    // Karijoki
                            FINLAND_622,    // Karstula
                            FINLAND_623,    // Kaskinen
                            FINLAND_624,    // Kauhajoki
                            FINLAND_625,    // Kauhava
                            FINLAND_626,    // Kaustinen
                            FINLAND_627,    // Keuruu
                            FINLAND_628,    // Kinnula
                            FINLAND_629,    // Kivijärvi
                            FINLAND_630,    // Kokkola
                            FINLAND_632,    // Konnevesi
                            FINLAND_633,    // Korpilahti
                            FINLAND_634,    // Korsnäs
                            FINLAND_635,    // Kortesjärvi
                            FINLAND_636,    // Kristiinankaupunki
                            FINLAND_637,    // Kruunupyy
                            FINLAND_638,    // Kuhmoinen
                            FINLAND_639,    // Kuortane
                            FINLAND_640,    // Kurikka
                            FINLAND_641,    // Kyyjärvi
                            FINLAND_642,    // Kälviä
                            FINLAND_643,    // Laihia
                            FINLAND_644,    // Lappajärvi
                            FINLAND_645,    // Lapua
                            FINLAND_646,    // Laukaa
                            FINLAND_647,    // Lehtimäki
                            FINLAND_648,    // Leivonmäki
                            FINLAND_649,    // Lestijärvi
                            FINLAND_650,    // Lohtaja
                            FINLAND_651,    // Luhanka
                            FINLAND_652,    // Luoto
                            FINLAND_653,    // Maalahti
                            FINLAND_654,    // Maksamaa
                            FINLAND_655,    // Multia
                            FINLAND_656,    // Mustasaari
                            FINLAND_657,    // Muurame
                            FINLAND_658,    // Nurmo
                            FINLAND_659,    // Närpiö
                            FINLAND_660,    // Oravainen
                            FINLAND_661,    // Perho
                            FINLAND_662,    // Peräseinäjoki
                            FINLAND_663,    // Petäjävesi
                            FINLAND_664,    // Pietarsaari
                            FINLAND_665,    // Pedersöre
                            FINLAND_666,    // Pihtipudas
                            FINLAND_668,    // Pylkönmäki
                            FINLAND_669,    // Saarijärvi
                            FINLAND_670,    // Seinäjoki
                            FINLAND_671,    // Soini
                            FINLAND_672,    // Sumiainen
                            FINLAND_673,    // Suolahti
                            FINLAND_675,    // Teuva
                            FINLAND_676,    // Toholampi
                            FINLAND_677,    // Toivakka
                            FINLAND_678,    // Töysä
                            FINLAND_679,    // Ullava
                            FINLAND_680,    // Uurainen
                            FINLAND_681,    // Uusikaarlepyy
                            FINLAND_682,    // Vaasa
                            FINLAND_683,    // Veteli
                            FINLAND_684,    // Viitasaari
                            FINLAND_685,    // Vimpeli
                            FINLAND_686,    // Vähäkyrö
                            FINLAND_687,    // Vöyri
                            FINLAND_688,    // Ylihärmä
                            FINLAND_689,    // Ylistaro
                            FINLAND_690,    // Ähtäri
                            FINLAND_692,    // Äänekoski
                            FINLAND_701,    // Eno
                            FINLAND_702,    // Iisalmi
                            FINLAND_703,    // Ilomantsi
                            FINLAND_704,    // Joensuu
                            FINLAND_705,    // Juankoski
                            FINLAND_706,    // Juuka
                            FINLAND_707,    // Kaavi
                            FINLAND_708,    // Karttula
                            FINLAND_709,    // Keitele
                            FINLAND_710,    // Kesälahti
                            FINLAND_711,    // Kiihtelysvaara
                            FINLAND_712,    // Kitee
                            FINLAND_713,    // Kiuruvesi
                            FINLAND_714,    // Kontiolahti
                            FINLAND_715,    // Kuopio
                            FINLAND_716,    // Lapinlahti
                            FINLAND_717,    // Leppävirta
                            FINLAND_718,    // Lieksa
                            FINLAND_719,    // Liperi
                            FINLAND_720,    // Maaninka
                            FINLAND_721,    // Nilsiä
                            FINLAND_722,    // Nurmes
                            FINLAND_723,    // Outokumpu
                            FINLAND_724,    // Pielavesi
                            FINLAND_725,    // Polvijärvi
                            FINLAND_726,    // Pyhäselkä
                            FINLAND_727,    // Rautalampi
                            FINLAND_728,    // Rautavaara
                            FINLAND_729,    // Rääkkylä
                            FINLAND_730,    // Siilinjärvi
                            FINLAND_731,    // Sonkajärvi
                            FINLAND_732,    // Suonenjoki
                            FINLAND_733,    // Tervo
                            FINLAND_734,    // Tohmajärvi
                            FINLAND_735,    // Tuupovaara
                            FINLAND_736,    // Tuusniemi
                            FINLAND_737,    // Valtimo
                            FINLAND_738,    // Varkaus
                            FINLAND_739,    // Varpaisjärvi
                            FINLAND_740,    // Vehmersalmi
                            FINLAND_741,    // Vesanto
                            FINLAND_742,    // Vieremä
                            FINLAND_743,    // Värtsilä
                            FINLAND_801,    // Alavieska
                            FINLAND_802,    // Haapajärvi
                            FINLAND_803,    // Haapavesi
                            FINLAND_804,    // Hailuoto
                            FINLAND_805,    // Haukipudas
                            FINLAND_806,    // Hyrynsalmi
                            FINLAND_807,    // Ii
                            FINLAND_808,    // Kajaani
                            FINLAND_810,    // Kalajoki
                            FINLAND_811,    // Kempele
                            FINLAND_812,    // Kestilä
                            FINLAND_813,    // Kiiminki
                            FINLAND_814,    // Kuhmo
                            FINLAND_815,    // Kuivaniemi
                            FINLAND_816,    // Kuusamo
                            FINLAND_817,    // Kärsämäki
                            FINLAND_818,    // Liminka
                            FINLAND_819,    // Lumijoki
                            FINLAND_820,    // Merijärvi
                            FINLAND_821,    // Muhos
                            FINLAND_822,    // Nivala
                            FINLAND_823,    // Oulainen
                            FINLAND_824,    // Oulu
                            FINLAND_825,    // Oulunsalo
                            FINLAND_826,    // Paltamo
                            FINLAND_827,    // Pattijoki
                            FINLAND_828,    // Piippola
                            FINLAND_829,    // Pudasjärvi
                            FINLAND_830,    // Pulkkila
                            FINLAND_831,    // Puolanka
                            FINLAND_832,    // Pyhäjoki
                            FINLAND_833,    // Pyhäjärvi
                            FINLAND_834,    // Pyhäntä
                            FINLAND_835,    // Raahe
                            FINLAND_836,    // Rantsila
                            FINLAND_837,    // Reisjärvi
                            FINLAND_838,    // Ristijärvi
                            FINLAND_839,    // Ruukki
                            FINLAND_840,    // Sievi
                            FINLAND_841,    // Siikajoki
                            FINLAND_842,    // Sotkamo
                            FINLAND_843,    // Suomussalmi
                            FINLAND_844,    // Taivalkoski
                            FINLAND_846,    // Tyrnävä
                            FINLAND_847,    // Utajärvi
                            FINLAND_848,    // Vaala
                            FINLAND_849,    // Vihanti
                            FINLAND_850,    // Vuolijoki
                            FINLAND_851,    // Yli-Ii
                            FINLAND_852,    // Ylikiiminki
                            FINLAND_853,    // Ylivieska
                            FINLAND_901,    // Enontekiö
                            FINLAND_902,    // Inari
                            FINLAND_903,    // Kemi
                            FINLAND_904,    // Keminmaa
                            FINLAND_905,    // Kemijärvi
                            FINLAND_907,    // Kittilä
                            FINLAND_908,    // Kolari
                            FINLAND_909,    // Muonio
                            FINLAND_910,    // Pelkosenniemi
                            FINLAND_911,    // Pello
                            FINLAND_912,    // Posio
                            FINLAND_913,    // Ranua
                            FINLAND_914,    // Rovaniemi
                            FINLAND_915,    // Rovaniemen mlk
                            FINLAND_916,    // Salla
                            FINLAND_917,    // Savukoski
                            FINLAND_918,    // Simo
                            FINLAND_919,    // Sodankylä
                            FINLAND_920,    // Tervola
                            FINLAND_921,    // Tornio
                            FINLAND_922,    // Utsjoki
                            FINLAND_923,    // Ylitornio
                            N_FINLAND_PRIMARIES
                         };

//typedef std::array<std::string, N_FINLAND_PRIMARIES> PRIMARY_FINLAND_ENUMERATION_TYPE;    ///< primaries for Finland

using PRIMARY_FINLAND_ENUMERATION_TYPE = std::array<std::string, N_FINLAND_PRIMARIES>;    ///< primaries for Finland

static PRIMARY_FINLAND_ENUMERATION_TYPE PRIMARY_FINLAND_ENUMERATION = { "100"s,    // Somero
                                                                          "102"s,    // Alastaro
                                                                          "103"s,    // Askainen
                                                                          "104"s,    // Aura
                                                                          "105"s,    // Dragsfjärd
                                                                          "106"s,    // Eura
                                                                          "107"s,    // Eurajoki
                                                                          "108"s,    // Halikko
                                                                          "109"s,    // Harjavalta
                                                                          "110"s,    // Honkajoki
                                                                          "111"s,    // Houtskari
                                                                          "112"s,    // Huittinen
                                                                          "115"s,    // Iniö
                                                                          "116"s,    // Jämijärvi
                                                                          "117"s,    // Kaarina
                                                                          "119"s,    // Kankaanpää
                                                                          "120"s,    // Karinainen
                                                                          "122"s,    // Karvia
                                                                          "123"s,    // Äetsä
                                                                          "124"s,    // Kemiö
                                                                          "126"s,    // Kiikala
                                                                          "128"s,    // Kiikoinen
                                                                          "129"s,    // Kisko
                                                                          "130"s,    // Kiukainen
                                                                          "131"s,    // Kodisjoki
                                                                          "132"s,    // Kokemäki
                                                                          "133"s,    // Korppoo
                                                                          "134"s,    // Koski tl
                                                                          "135"s,    // Kullaa
                                                                          "136"s,    // Kustavi
                                                                          "137"s,    // Kuusjoki
                                                                          "138"s,    // Köyliö
                                                                          "139"s,    // Laitila
                                                                          "140"s,    // Lappi
                                                                          "141"s,    // Lavia
                                                                          "142"s,    // Lemu
                                                                          "143"s,    // Lieto
                                                                          "144"s,    // Loimaa
                                                                          "145"s,    // Loimaan kunta
                                                                          "147"s,    // Luvia
                                                                          "148"s,    // Marttila
                                                                          "149"s,    // Masku
                                                                          "150"s,    // Mellilä
                                                                          "151"s,    // Merikarvia
                                                                          "152"s,    // Merimasku
                                                                          "154"s,    // Mietoinen
                                                                          "156"s,    // Muurla
                                                                          "157"s,    // Mynämäki
                                                                          "158"s,    // Naantali
                                                                          "159"s,    // Nakkila
                                                                          "160"s,    // Nauvo
                                                                          "161"s,    // Noormarkku
                                                                          "162"s,    // Nousiainen
                                                                          "163"s,    // Oripää
                                                                          "164"s,    // Paimio
                                                                          "165"s,    // Parainen
                                                                          "167"s,    // Perniö
                                                                          "168"s,    // Pertteli
                                                                          "169"s,    // Piikkiö
                                                                          "170"s,    // Pomarkku
                                                                          "171"s,    // Pori
                                                                          "172"s,    // Punkalaidun
                                                                          "173"s,    // Pyhäranta
                                                                          "174"s,    // Pöytyä
                                                                          "175"s,    // Raisio
                                                                          "176"s,    // Rauma
                                                                          "178"s,    // Rusko
                                                                          "179"s,    // Rymättylä
                                                                          "180"s,    // Salo
                                                                          "181"s,    // Sauvo
                                                                          "182"s,    // Siikainen
                                                                          "183"s,    // Suodenniemi
                                                                          "184"s,    // Suomusjärvi
                                                                          "185"s,    // Säkylä
                                                                          "186"s,    // Särkisalo
                                                                          "187"s,    // Taivassalo
                                                                          "188"s,    // Tarvasjoki
                                                                          "189"s,    // Turku
                                                                          "190"s,    // Ulvila
                                                                          "191"s,    // Uusikaupunki
                                                                          "192"s,    // Vahto
                                                                          "193"s,    // Vammala
                                                                          "194"s,    // Vampula
                                                                          "195"s,    // Vehmaa
                                                                          "196"s,    // Velkua
                                                                          "198"s,    // Västanfjärd
                                                                          "199"s,    // Yläne
                                                                          "201"s,    // Artjärvi
                                                                          "202"s,    // Askola
                                                                          "204"s,    // Espoo
                                                                          "205"s,    // Hanko
                                                                          "206"s,    // Helsinki
                                                                          "207"s,    // Hyvinkää
                                                                          "208"s,    // Inkoo
                                                                          "209"s,    // Järvenpää
                                                                          "210"s,    // Karjaa
                                                                          "211"s,    // Karjalohja
                                                                          "212"s,    // Karkkila
                                                                          "213"s,    // Kauniainen
                                                                          "214"s,    // Kerava
                                                                          "215"s,    // Kirkkonummi
                                                                          "216"s,    // Lapinjärvi
                                                                          "217"s,    // Liljendal
                                                                          "218"s,    // Lohjan kaupunki
                                                                          "220"s,    // Loviisa
                                                                          "221"s,    // Myrskylä
                                                                          "222"s,    // Mäntsälä
                                                                          "223"s,    // Nummi-Pusula
                                                                          "224"s,    // Nurmijärvi
                                                                          "225"s,    // Orimattila
                                                                          "226"s,    // Pernaja
                                                                          "227"s,    // Pohja
                                                                          "228"s,    // Pornainen
                                                                          "229"s,    // Porvoo
                                                                          "231"s,    // Pukkila
                                                                          "233"s,    // Ruotsinpyhtää
                                                                          "234"s,    // Sammatti
                                                                          "235"s,    // Sipoo
                                                                          "236"s,    // Siuntio
                                                                          "238"s,    // Tammisaari
                                                                          "241"s,    // Tuusula
                                                                          "242"s,    // Vantaa
                                                                          "243"s,    // Vihti
                                                                          "301"s,    // Asikkala
                                                                          "303"s,    // Forssa
                                                                          "304"s,    // Hattula
                                                                          "305"s,    // Hauho
                                                                          "306"s,    // Hausjärvi
                                                                          "307"s,    // Hollola
                                                                          "308"s,    // Humppila
                                                                          "309"s,    // Hämeenlinna
                                                                          "310"s,    // Janakkala
                                                                          "311"s,    // Jokioinen
                                                                          "312"s,    // Juupajoki
                                                                          "313"s,    // Kalvola
                                                                          "314"s,    // Kangasala
                                                                          "315"s,    // Hämeenkoski
                                                                          "316"s,    // Kuhmalahti
                                                                          "318"s,    // Kuru
                                                                          "319"s,    // Kylmäkoski
                                                                          "320"s,    // Kärkölä
                                                                          "321"s,    // Lahti
                                                                          "322"s,    // Lammi
                                                                          "323"s,    // Lempäälä
                                                                          "324"s,    // Loppi
                                                                          "325"s,    // Luopioinen
                                                                          "326"s,    // Längelmäki
                                                                          "327"s,    // Mänttä
                                                                          "328"s,    // Nastola
                                                                          "329"s,    // Nokia
                                                                          "330"s,    // Orivesi
                                                                          "331"s,    // Padasjoki
                                                                          "332"s,    // Pirkkala
                                                                          "333"s,    // Pälkäne
                                                                          "334"s,    // Renko
                                                                          "335"s,    // Riihimäki
                                                                          "336"s,    // Ruovesi
                                                                          "337"s,    // Sahalahti
                                                                          "340"s,    // Tammela
                                                                          "341"s,    // Tampere
                                                                          "342"s,    // Toijala
                                                                          "344"s,    // Tuulos
                                                                          "345"s,    // Urjala
                                                                          "346"s,    // Valkeakoski
                                                                          "347"s,    // Vesilahti
                                                                          "348"s,    // Viiala
                                                                          "349"s,    // Vilppula
                                                                          "350"s,    // Virrat
                                                                          "351"s,    // Ylöjärvi
                                                                          "352"s,    // Ypäjä
                                                                          "353"s,    // Hämeenkyrö
                                                                          "354"s,    // Ikaalinen
                                                                          "355"s,    // Kihniö
                                                                          "356"s,    // Mouhijärvi
                                                                          "357"s,    // Parkano
                                                                          "358"s,    // Viljakkala
                                                                          "402"s,    // Enonkoski
                                                                          "403"s,    // Hartola
                                                                          "404"s,    // Haukivuori
                                                                          "405"s,    // Heinola
                                                                          "407"s,    // Heinävesi
                                                                          "408"s,    // Hirvensalmi
                                                                          "409"s,    // Joroinen
                                                                          "410"s,    // Juva
                                                                          "411"s,    // Jäppilä
                                                                          "412"s,    // Kangaslampi
                                                                          "413"s,    // Kangasniemi
                                                                          "414"s,    // Kerimäki
                                                                          "415"s,    // Mikkeli
                                                                          "417"s,    // Mäntyharju
                                                                          "418"s,    // Pertunmaa
                                                                          "419"s,    // Pieksämäki
                                                                          "420"s,    // Pieksänmaa
                                                                          "421"s,    // Punkaharju
                                                                          "422"s,    // Puumala
                                                                          "423"s,    // Rantasalmi
                                                                          "424"s,    // Ristiina
                                                                          "425"s,    // Savonlinna
                                                                          "426"s,    // Savonranta
                                                                          "427"s,    // Sulkava
                                                                          "428"s,    // Sysmä
                                                                          "502"s,    // Elimäki
                                                                          "503"s,    // Hamina
                                                                          "504"s,    // Iitti
                                                                          "505"s,    // Imatra
                                                                          "506"s,    // Jaala
                                                                          "507"s,    // Joutseno
                                                                          "509"s,    // Kotka
                                                                          "510"s,    // Kouvola
                                                                          "511"s,    // Kuusankoski
                                                                          "513"s,    // Lappeenranta
                                                                          "514"s,    // Lemi
                                                                          "515"s,    // Luumäki
                                                                          "516"s,    // Miehikkälä
                                                                          "518"s,    // Parikkala
                                                                          "519"s,    // Pyhtää
                                                                          "520"s,    // Rautjärvi
                                                                          "521"s,    // Ruokolahti
                                                                          "522"s,    // Saari
                                                                          "523"s,    // Savitaipale
                                                                          "525"s,    // Suomenniemi
                                                                          "526"s,    // Taipalsaari
                                                                          "527"s,    // Uukuniemi
                                                                          "528"s,    // Valkeala
                                                                          "530"s,    // Virolahti
                                                                          "531"s,    // Ylämaa
                                                                          "532"s,    // Anjalankoski
                                                                          "601"s,    // Alahärmä
                                                                          "602"s,    // Alajärvi
                                                                          "603"s,    // Alavus
                                                                          "604"s,    // Evijärvi
                                                                          "605"s,    // Halsua
                                                                          "606"s,    // Hankasalmi
                                                                          "607"s,    // Himanka
                                                                          "608"s,    // Ilmajoki
                                                                          "609"s,    // Isojoki
                                                                          "610"s,    // Isokyrö
                                                                          "611"s,    // Jalasjärvi
                                                                          "612"s,    // Joutsa
                                                                          "613"s,    // Jurva
                                                                          "614"s,    // Jyväskylä
                                                                          "615"s,    // Jyväskylän mlk
                                                                          "616"s,    // Jämsä
                                                                          "617"s,    // Jämsänkoski
                                                                          "619"s,    // Kannonkoski
                                                                          "620"s,    // Kannus
                                                                          "621"s,    // Karijoki
                                                                          "622"s,    // Karstula
                                                                          "623"s,    // Kaskinen
                                                                          "624"s,    // Kauhajoki
                                                                          "625"s,    // Kauhava
                                                                          "626"s,    // Kaustinen
                                                                          "627"s,    // Keuruu
                                                                          "628"s,    // Kinnula
                                                                          "629"s,    // Kivijärvi
                                                                          "630"s,    // Kokkola
                                                                          "632"s,    // Konnevesi
                                                                          "633"s,    // Korpilahti
                                                                          "634"s,    // Korsnäs
                                                                          "635"s,    // Kortesjärvi
                                                                          "636"s,    // Kristiinankaupunki
                                                                          "637"s,    // Kruunupyy
                                                                          "638"s,    // Kuhmoinen
                                                                          "639"s,    // Kuortane
                                                                          "640"s,    // Kurikka
                                                                          "641"s,    // Kyyjärvi
                                                                          "642"s,    // Kälviä
                                                                          "643"s,    // Laihia
                                                                          "644"s,    // Lappajärvi
                                                                          "645"s,    // Lapua
                                                                          "646"s,    // Laukaa
                                                                          "647"s,    // Lehtimäki
                                                                          "648"s,    // Leivonmäki
                                                                          "649"s,    // Lestijärvi
                                                                          "650"s,    // Lohtaja
                                                                          "651"s,    // Luhanka
                                                                          "652"s,    // Luoto
                                                                          "653"s,    // Maalahti
                                                                          "654"s,    // Maksamaa
                                                                          "655"s,    // Multia
                                                                          "656"s,    // Mustasaari
                                                                          "657"s,    // Muurame
                                                                          "658"s,    // Nurmo
                                                                          "659"s,    // Närpiö
                                                                          "660"s,    // Oravainen
                                                                          "661"s,    // Perho
                                                                          "662"s,    // Peräseinäjoki
                                                                          "663"s,    // Petäjävesi
                                                                          "664"s,    // Pietarsaari
                                                                          "665"s,    // Pedersöre
                                                                          "666"s,    // Pihtipudas
                                                                          "668"s,    // Pylkönmäki
                                                                          "669"s,    // Saarijärvi
                                                                          "670"s,    // Seinäjoki
                                                                          "671"s,    // Soini
                                                                          "672"s,    // Sumiainen
                                                                          "673"s,    // Suolahti
                                                                          "675"s,    // Teuva
                                                                          "676"s,    // Toholampi
                                                                          "677"s,    // Toivakka
                                                                          "678"s,    // Töysä
                                                                          "679"s,    // Ullava
                                                                          "680"s,    // Uurainen
                                                                          "681"s,    // Uusikaarlepyy
                                                                          "682"s,    // Vaasa
                                                                          "683"s,    // Veteli
                                                                          "684"s,    // Viitasaari
                                                                          "685"s,    // Vimpeli
                                                                          "686"s,    // Vähäkyrö
                                                                          "687"s,    // Vöyri
                                                                          "688"s,    // Ylihärmä
                                                                          "689"s,    // Ylistaro
                                                                          "690"s,    // Ähtäri
                                                                          "692"s,    // Äänekoski
                                                                          "701"s,    // Eno
                                                                          "702"s,    // Iisalmi
                                                                          "703"s,    // Ilomantsi
                                                                          "704"s,    // Joensuu
                                                                          "705"s,    // Juankoski
                                                                          "706"s,    // Juuka
                                                                          "707"s,    // Kaavi
                                                                          "708"s,    // Karttula
                                                                          "709"s,    // Keitele
                                                                          "710"s,    // Kesälahti
                                                                          "711"s,    // Kiihtelysvaara
                                                                          "712"s,    // Kitee
                                                                          "713"s,    // Kiuruvesi
                                                                          "714"s,    // Kontiolahti
                                                                          "715"s,    // Kuopio
                                                                          "716"s,    // Lapinlahti
                                                                          "717"s,    // Leppävirta
                                                                          "718"s,    // Lieksa
                                                                          "719"s,    // Liperi
                                                                          "720"s,    // Maaninka
                                                                          "721"s,    // Nilsiä
                                                                          "722"s,    // Nurmes
                                                                          "723"s,    // Outokumpu
                                                                          "724"s,    // Pielavesi
                                                                          "725"s,    // Polvijärvi
                                                                          "726"s,    // Pyhäselkä
                                                                          "727"s,    // Rautalampi
                                                                          "728"s,    // Rautavaara
                                                                          "729"s,    // Rääkkylä
                                                                          "730"s,    // Siilinjärvi
                                                                          "731"s,    // Sonkajärvi
                                                                          "732"s,    // Suonenjoki
                                                                          "733"s,    // Tervo
                                                                          "734"s,    // Tohmajärvi
                                                                          "735"s,    // Tuupovaara
                                                                          "736"s,    // Tuusniemi
                                                                          "737"s,    // Valtimo
                                                                          "738"s,    // Varkaus
                                                                          "739"s,    // Varpaisjärvi
                                                                          "740"s,    // Vehmersalmi
                                                                          "741"s,    // Vesanto
                                                                          "742"s,    // Vieremä
                                                                          "743"s,    // Värtsilä
                                                                          "801"s,    // Alavieska
                                                                          "802"s,    // Haapajärvi
                                                                          "803"s,    // Haapavesi
                                                                          "804"s,    // Hailuoto
                                                                          "805"s,    // Haukipudas
                                                                          "806"s,    // Hyrynsalmi
                                                                          "807"s,    // Ii
                                                                          "808"s,    // Kajaani
                                                                          "810"s,    // Kalajoki
                                                                          "811"s,    // Kempele
                                                                          "812"s,    // Kestilä
                                                                          "813"s,    // Kiiminki
                                                                          "814"s,    // Kuhmo
                                                                          "815"s,    // Kuivaniemi
                                                                          "816"s,    // Kuusamo
                                                                          "817"s,    // Kärsämäki
                                                                          "818"s,    // Liminka
                                                                          "819"s,    // Lumijoki
                                                                          "820"s,    // Merijärvi
                                                                          "821"s,    // Muhos
                                                                          "822"s,    // Nivala
                                                                          "823"s,    // Oulainen
                                                                          "824"s,    // Oulu
                                                                          "825"s,    // Oulunsalo
                                                                          "826"s,    // Paltamo
                                                                          "827"s,    // Pattijoki
                                                                          "828"s,    // Piippola
                                                                          "829"s,    // Pudasjärvi
                                                                          "830"s,    // Pulkkila
                                                                          "831"s,    // Puolanka
                                                                          "832"s,    // Pyhäjoki
                                                                          "833"s,    // Pyhäjärvi
                                                                          "834"s,    // Pyhäntä
                                                                          "835"s,    // Raahe
                                                                          "836"s,    // Rantsila
                                                                          "837"s,    // Reisjärvi
                                                                          "838"s,    // Ristijärvi
                                                                          "839"s,    // Ruukki
                                                                          "840"s,    // Sievi
                                                                          "841"s,    // Siikajoki
                                                                          "842"s,    // Sotkamo
                                                                          "843"s,    // Suomussalmi
                                                                          "844"s,    // Taivalkoski
                                                                          "846"s,    // Tyrnävä
                                                                          "847"s,    // Utajärvi
                                                                          "848"s,    // Vaala
                                                                          "849"s,    // Vihanti
                                                                          "850"s,    // Vuolijoki
                                                                          "851"s,    // Yli-Ii
                                                                          "852"s,    // Ylikiiminki
                                                                          "853"s,    // Ylivieska
                                                                          "901"s,    // Enontekiö
                                                                          "902"s,    // Inari
                                                                          "903"s,    // Kemi
                                                                          "904"s,    // Keminmaa
                                                                          "905"s,    // Kemijärvi
                                                                          "907"s,    // Kittilä
                                                                          "908"s,    // Kolari
                                                                          "909"s,    // Muonio
                                                                          "910"s,    // Pelkosenniemi
                                                                          "911"s,    // Pello
                                                                          "912"s,    // Posio
                                                                          "913"s,    // Ranua
                                                                          "914"s,    // Rovaniemi
                                                                          "915"s,    // Rovaniemen mlk
                                                                          "916"s,    // Salla
                                                                          "917"s,    // Savukoski
                                                                          "918"s,    // Simo
                                                                          "919"s,    // Sodankylä
                                                                          "920"s,    // Tervola
                                                                          "921"s,    // Tornio
                                                                          "922"s,    // Utsjoki
                                                                          "923"s     // Ylitornio
                                                                      };

/// Sardinia
enum PRIMARY_ENUM_SARDINIA { SARDINIA_CA,   // Cagliari
                             SARDINIA_CI,   //  Carbonia Iglesias
                             SARDINIA_MD,   //  Medio Campidano (deprecated)
                             SARDINIA_NU,   // Nuoro
                             SARDINIA_OG,   // Ogliastra
                             SARDINIA_OR,   // Oristano
                             SARDINIA_OT,   // Olbia Tempio
                             SARDINIA_SS,   // Sassari
                             SARDINIA_VS,   // Medio Campidano
                             N_SARDINIA_PRIMARIES
                           };

//typedef std::array<std::string, N_SARDINIA_PRIMARIES> PRIMARY_SARDINIA_ENUMERATION_TYPE;    ///< primaries for Sardinia

using PRIMARY_SARDINIA_ENUMERATION_TYPE = std::array<std::string, N_SARDINIA_PRIMARIES>;    ///< primaries for Sardinia

static PRIMARY_SARDINIA_ENUMERATION_TYPE PRIMARY_SARDINIA_ENUMERATION = { "CA"s,   // Cagliari
                                                                            "CI"s,   //  Carbonia Iglesias
                                                                            "MD"s,   //  Medio Campidano (deprecated)
                                                                            "NU"s,   // Nuoro
                                                                            "OG"s,   // Ogliastra
                                                                            "OR"s,   // Oristano
                                                                            "OT"s,   // Olbia Tempio
                                                                            "SS"s,   // Sassari
                                                                            "VS"s,   // Medio Campidano
                                                                        };

/// France
enum PRIMARY_ENUM_FRANCE { FRANCE_01, // Ain
                           FRANCE_02, // Aisne
                           FRANCE_03, // Allier
                           FRANCE_04, // Alpes-de-Haute-Provence
                           FRANCE_05, // Hautes-Alpes
                           FRANCE_06, // Alpes-Maritimes
                           FRANCE_07, // Ardèche
                           FRANCE_08, // Ardennes
                           FRANCE_09, // Ariège
                           FRANCE_10, // Aube
                           FRANCE_11, // Aude
                           FRANCE_12, // Aveyron
                           FRANCE_13, // Bouches-du-Rhone
                           FRANCE_14, // Calvados
                           FRANCE_15, // Cantal
                           FRANCE_16, // Charente
                           FRANCE_17, // Charente-Maritime
                           FRANCE_18, // Cher
                           FRANCE_19, // Corrèze
                           FRANCE_21, // Cote-d'Or
                           FRANCE_22, // Cotes-d'Armor
                           FRANCE_23, // Creuse
                           FRANCE_24, // Dordogne
                           FRANCE_25, // Doubs
                           FRANCE_26, // Drôme
                           FRANCE_27, // Eure
                           FRANCE_28, // Eure-et-Loir
                           FRANCE_29, // Finistère
                           FRANCE_30, // Gard
                           FRANCE_31, // Haute-Garonne
                           FRANCE_32, // Gere
                           FRANCE_33, // Gironde
                           FRANCE_34, // Hérault
                           FRANCE_35, // Ille-et-Vilaine
                           FRANCE_36, // Indre
                           FRANCE_37, // Indre-et-Loire
                           FRANCE_38, // Isère
                           FRANCE_39, // Jura
                           FRANCE_40, // Landes
                           FRANCE_41, // Loir-et-Cher
                           FRANCE_42, // Loire
                           FRANCE_43, // Haute-Loire
                           FRANCE_44, // Loire-Atlantique
                           FRANCE_45, // Loiret
                           FRANCE_46, // Lot
                           FRANCE_47, // Lot-et-Garonne
                           FRANCE_48, // Lozère
                           FRANCE_49, // Maine-et-Loire
                           FRANCE_50, // Manche
                           FRANCE_51, // Marne
                           FRANCE_52, // Haute-Marne
                           FRANCE_53, // Mayenne
                           FRANCE_54, // Meurthe-et-Moselle
                           FRANCE_55, // Meuse
                           FRANCE_56, //Morbihan
                           FRANCE_57, // Moselle
                           FRANCE_58, // Niëvre
                           FRANCE_59, // Nord
                           FRANCE_60, // Oise
                           FRANCE_61, // Orne
                           FRANCE_62, // Pas-de-Calais
                           FRANCE_63, // Puy-de-Dôme
                           FRANCE_64, // Pyrénées-Atlantiques
                           FRANCE_65, // Hautea-Pyrénées
                           FRANCE_66, // Pyrénées-Orientales
                           FRANCE_67, // Bas-Rhin
                           FRANCE_68, // Haut-Rhin
                           FRANCE_69, // Rhône
                           FRANCE_70, // Haute-Saône
                           FRANCE_71, // Saône-et-Loire
                           FRANCE_72, // Sarthe
                           FRANCE_73, // Savoie
                           FRANCE_74, // Haute-Savoie
                           FRANCE_75, // Paris
                           FRANCE_76, // Seine-Maritime
                           FRANCE_77, // Seine-et-Marne
                           FRANCE_78, // Yvelines
                           FRANCE_79, // Deux-Sèvres
                           FRANCE_80, // Somme
                           FRANCE_81, // Tarn
                           FRANCE_82, // Tarn-et-Garonne
                           FRANCE_83, // Var
                           FRANCE_84, // Vaucluse
                           FRANCE_85, // Vendée
                           FRANCE_86, // Vienne
                           FRANCE_87, // Haute-Vienne
                           FRANCE_88, // Vosges
                           FRANCE_89, // Yonne
                           FRANCE_90, // Territoire de Belfort
                           FRANCE_91, // Essonne
                           FRANCE_92, // Hauts-de-Selne
                           FRANCE_93, // Seine-Saint-Denis
                           FRANCE_94, // Val-de-Marne
                           FRANCE_95, // Val-d'Oise
                           N_FRANCE_PRIMARIES
                         };

//typedef std::array<std::string, N_FRANCE_PRIMARIES> PRIMARY_FRANCE_ENUMERATION_TYPE;    ///< primaries for France

using PRIMARY_FRANCE_ENUMERATION_TYPE = std::array<std::string, N_FRANCE_PRIMARIES>;    ///< primaries for France

static PRIMARY_FRANCE_ENUMERATION_TYPE PRIMARY_FRANCE_ENUMERATION = { "01"s, // Ain
                                                                        "02"s, // Aisne
                                                                        "03"s, // Allier
                                                                        "04"s, // Alpes-de-Haute-Provence
                                                                        "05"s, // Hautes-Alpes
                                                                        "06"s, // Alpes-Maritimes
                                                                        "07"s, // Ardèche
                                                                        "08"s, // Ardennes
                                                                        "09"s, // Ariège
                                                                        "10"s, // Aube
                                                                        "11"s, // Aude
                                                                        "12"s, // Aveyron
                                                                        "13"s, // Bouches-du-Rhone
                                                                        "14"s, // Calvados
                                                                        "15"s, // Cantal
                                                                        "16"s, // Charente
                                                                        "17"s, // Charente-Maritime
                                                                        "18"s, // Cher
                                                                        "19"s, // Corrèze
                                                                        "21"s, // Cote-d'Or
                                                                        "22"s, // Cotes-d'Armor
                                                                        "23"s, // Creuse
                                                                        "24"s, // Dordogne
                                                                        "25"s, // Doubs
                                                                        "26"s, // Drôme
                                                                        "27"s, // Eure
                                                                        "28"s, // Eure-et-Loir
                                                                        "29"s, // Finistère
                                                                        "30"s, // Gard
                                                                        "31"s, // Haute-Garonne
                                                                        "32"s, // Gere
                                                                        "33"s, // Gironde
                                                                        "34"s, // Hérault
                                                                        "35"s, // Ille-et-Vilaine
                                                                        "36"s, // Indre
                                                                        "37"s, // Indre-et-Loire
                                                                        "38"s, // Isère
                                                                        "39"s, // Jura
                                                                        "40"s, // Landes
                                                                        "41"s, // Loir-et-Cher
                                                                        "42"s, // Loire
                                                                        "43"s, // Haute-Loire
                                                                        "44"s, // Loire-Atlantique
                                                                        "45"s, // Loiret
                                                                        "46"s, // Lot
                                                                        "47"s, // Lot-et-Garonne
                                                                        "48"s, // Lozère
                                                                        "49"s, // Maine-et-Loire
                                                                        "50"s, // Manche
                                                                        "51"s, // Marne
                                                                        "52"s, // Haute-Marne
                                                                        "53"s, // Mayenne
                                                                        "54"s, // Meurthe-et-Moselle
                                                                        "55"s, // Meuse
                                                                        "56"s, //Morbihan
                                                                        "57"s, // Moselle
                                                                        "58"s, // Niëvre
                                                                        "59"s, // Nord
                                                                        "60"s, // Oise
                                                                        "61"s, // Orne
                                                                        "62"s, // Pas-de-Calais
                                                                        "63"s, // Puy-de-Dôme
                                                                        "64"s, // Pyrénées-Atlantiques
                                                                        "65"s, // Hautea-Pyrénées
                                                                        "66"s, // Pyrénées-Orientales
                                                                        "67"s, // Bas-Rhin
                                                                        "68"s, // Haut-Rhin
                                                                        "69"s, // Rhône
                                                                        "70"s, // Haute-Saône
                                                                        "71"s, // Saône-et-Loire
                                                                        "72"s, // Sarthe
                                                                        "73"s, // Savoie
                                                                        "74"s, // Haute-Savoie
                                                                        "75"s, // Paris
                                                                        "76"s, // Seine-Maritime
                                                                        "77"s, // Seine-et-Marne
                                                                        "78"s, // Yvelines
                                                                        "79"s, // Deux-Sèvres
                                                                        "80"s, // Somme
                                                                        "81"s, // Tarn
                                                                        "82"s, // Tarn-et-Garonne
                                                                        "83"s, // Var
                                                                        "84"s, // Vaucluse
                                                                        "85"s, // Vendée
                                                                        "86"s, // Vienne
                                                                        "87"s, // Haute-Vienne
                                                                        "88"s, // Vosges
                                                                        "89"s, // Yonne
                                                                        "90"s, // Territoire de Belfort
                                                                        "91"s, // Essonne
                                                                        "92"s, // Hauts-de-Selne
                                                                        "93"s, // Seine-Saint-Denis
                                                                        "94"s, // Val-de-Marne
                                                                        "95"s  // Val-d'Oise
                                                                    };

/// Germany
enum PRIMARY_ENUM_GERMANY { GERMANY_BB,   // Brandenburg
                            GERMANY_BE,   // Berlin
                            GERMANY_BW,   // Baden-Württemberg
                            GERMANY_BY,   // Freistaat Bayern
                            GERMANY_HB,   // Freie Hansestadt Bremen
                            GERMANY_HE,   // Hessen
                            GERMANY_HH,   // Freie und Hansestadt Hamburg
                            GERMANY_MV,   // Mecklenburg-Vorpommern
                            GERMANY_NI,   // Niedersachsen
                            GERMANY_NW,   // Nordrhein-Westfalen
                            GERMANY_RP,   // Rheinland-Pfalz
                            GERMANY_SL,   // Saarland
                            GERMANY_SH,   // Schleswig-Holstein
                            GERMANY_SN,   // Freistaat Sachsen
                            GERMANY_ST,   // Sachsen-Anhalt
                            GERMANY_TH,   // Freistaat Thüringen
                            N_GERMANY_PRIMARIES
                          };

//typedef std::array<std::string, N_GERMANY_PRIMARIES> PRIMARY_GERMANY_ENUMERATION_TYPE;    ///< primaries for Germany

using PRIMARY_GERMANY_ENUMERATION_TYPE = std::array<std::string, N_GERMANY_PRIMARIES>;    ///< primaries for Germany

static PRIMARY_GERMANY_ENUMERATION_TYPE PRIMARY_GERMANY_ENUMERATION = { "BB"s,   // Brandenburg
                                                                          "BE"s,   // Berlin
                                                                          "BW"s,   // Baden-Württemberg
                                                                          "BY"s,   // Freistaat Bayern
                                                                          "HB"s,   // Freie Hansestadt Bremen
                                                                          "HE"s,   // Hessen
                                                                          "HH"s,   // Freie und Hansestadt Hamburg
                                                                          "MV"s,   // Mecklenburg-Vorpommern
                                                                          "NI"s,   // Niedersachsen
                                                                          "NW"s,   // Nordrhein-Westfalen
                                                                          "RP"s,   // Rheinland-Pfalz
                                                                          "SL"s,   // Saarland
                                                                          "SH"s,   // Schleswig-Holstein
                                                                          "SN"s,   // Freistaat Sachsen
                                                                          "ST"s,   // Sachsen-Anhalt
                                                                          "TH"s   // Freistaat Thüringen
                                                                      };

/// Hungary
enum PRIMARY_ENUM_HUNGARY { HUNGARY_GY,   // Gyõr (Gyõr-Moson-Sopron)
                            HUNGARY_VA,   // Vas
                            HUNGARY_ZA,   // Zala
                            HUNGARY_KO,   // Komárom (Komárom-Esztergom)
                            HUNGARY_VE,   // Veszprém
                            HUNGARY_BA,   // Baranya
                            HUNGARY_SO,   // Somogy
                            HUNGARY_TO,   // Tolna
                            HUNGARY_FE,   // Fejér
                            HUNGARY_BP,   // Budapest
                            HUNGARY_HE,   // Heves
                            HUNGARY_NG,   // Nógrád
                            HUNGARY_PE,   // Pest
                            HUNGARY_SZ,   // Szolnok (Jász-Nagykun-Szolnok)
                            HUNGARY_BE,   // Békés
                            HUNGARY_BN,   // Bács-Kiskun
                            HUNGARY_CS,   // Csongrád
                            HUNGARY_BO,   // Borsod (Borsod-Abaúj-Zemplén)
                            HUNGARY_HB,   // Hajdú-Bihar
                            HUNGARY_SA,   // Szabolcs (Szabolcs-Szatmár-Bereg)
                            N_HUNGARY_PRIMARIES
                          };

//typedef std::array<std::string, N_HUNGARY_PRIMARIES> PRIMARY_HUNGARY_ENUMERATION_TYPE;    ///< primaries for Hungary

using PRIMARY_HUNGARY_ENUMERATION_TYPE = std::array<std::string, N_HUNGARY_PRIMARIES>;    ///< primaries for Hungary

static PRIMARY_HUNGARY_ENUMERATION_TYPE PRIMARY_HUNGARY_ENUMERATION = { "GY"s,   // Gyõr (Gyõr-Moson-Sopron)
                                                                          "VA"s,   // Vas
                                                                          "ZA"s,   // Zala
                                                                          "KO"s,   // Komárom (Komárom-Esztergom)
                                                                          "VE"s,   // Veszprém
                                                                          "BA"s,   // Baranya
                                                                          "SO"s,   // Somogy
                                                                          "TO"s,   // Tolna
                                                                          "FE"s,   // Fejér
                                                                          "BP"s,   // Budapest
                                                                          "HE"s,   // Heves
                                                                          "NG"s,   // Nógrád
                                                                          "PE"s,   // Pest
                                                                          "SZ"s,   // Szolnok (Jász-Nagykun-Szolnok)
                                                                          "BE"s,   // Békés
                                                                          "BN"s,   // Bács-Kiskun
                                                                          "CS"s,   // Csongrád
                                                                          "BO"s,   // Borsod (Borsod-Abaúj-Zemplén)
                                                                          "HB"s,   // Hajdú-Bihar
                                                                          "SA"s   // Szabolcs (Szabolcs-Szatmár-Bereg)
                                                                      };

/// Ireland
enum PRIMARY_ENUM_IRELAND { IRELAND_CW,  // Carlow (Ceatharlach)
                            IRELAND_CN,  // Cavan (An Cabhán)
                            IRELAND_CE,  // Clare (An Clár)
                            IRELAND_C,   // Cork (Corcaigh)
                            IRELAND_DL,  // Donegal (Dún na nGall)
                            IRELAND_D,   // Dublin (Baile Áth Cliath)
                            IRELAND_G,   // Galway (Gaillimh)
                            IRELAND_KY,  // Kerry (Ciarraí)
                            IRELAND_KE,  // Kildare (Cill Dara)
                            IRELAND_KK,  // Kilkenny (Cill Chainnigh)
                            IRELAND_LS,  // Laois (Laois)
                            IRELAND_LM,  // Leitrim (Liatroim)
                            IRELAND_LK,  // Limerick (Luimneach)
                            IRELAND_LD,  // Longford (An Longfort)
                            IRELAND_LH,  // Louth (Lú)
                            IRELAND_MO,  // Mayo (Maigh Eo)
                            IRELAND_MH,  // Meath (An Mhí)
                            IRELAND_MN,  // Monaghan (Muineachán)
                            IRELAND_OY,  // Offaly (Uíbh Fhailí)
                            IRELAND_RN,  // Roscommon (Ros Comáin)
                            IRELAND_SO,  // Sligo (Sligeach)
                            IRELAND_TA,  // Tipperary (Tiobraid Árann)
                            IRELAND_WD,  // Waterford (Port Láirge)
                            IRELAND_WH,  // Westmeath (An Iarmhí)
                            IRELAND_WX,  // Wexford (Loch Garman)
                            IRELAND_WW,  // Wicklow (Cill Mhantáin)
                            N_IRELAND_PRIMARIES
                          };

//typedef std::array<std::string, N_IRELAND_PRIMARIES> PRIMARY_IRELAND_ENUMERATION_TYPE;    ///< primaries for Ireland

using PRIMARY_IRELAND_ENUMERATION_TYPE = std::array<std::string, N_IRELAND_PRIMARIES>;    ///< primaries for Ireland

static PRIMARY_IRELAND_ENUMERATION_TYPE PRIMARY_IRELAND_ENUMERATION = { "CW"s,  // Carlow (Ceatharlach)
                                                                          "CN"s,  // Cavan (An Cabhán)
                                                                          "CE"s,  // Clare (An Clár)
                                                                          "C"s,   // Cork (Corcaigh)
                                                                          "DL"s,  // Donegal (Dún na nGall)
                                                                          "D"s,   // Dublin (Baile Áth Cliath)
                                                                          "G"s,   // Galway (Gaillimh)
                                                                          "KY"s,  // Kerry (Ciarraí)
                                                                          "KE"s,  // Kildare (Cill Dara)
                                                                          "KK"s,  // Kilkenny (Cill Chainnigh)
                                                                          "LS"s,  // Laois (Laois)
                                                                          "LM"s,  // Leitrim (Liatroim)
                                                                          "LK"s,  // Limerick (Luimneach)
                                                                          "LD"s,  // Longford (An Longfort)
                                                                          "LH"s,  // Louth (Lú)
                                                                          "MO"s,  // Mayo (Maigh Eo)
                                                                          "MH"s,  // Meath (An Mhí)
                                                                          "MN"s,  // Monaghan (Muineachán)
                                                                          "OY"s,  // Offaly (Uíbh Fhailí)
                                                                          "RN"s,  // Roscommon (Ros Comáin)
                                                                          "SO"s,  // Sligo (Sligeach)
                                                                          "TA"s,  // Tipperary (Tiobraid Árann)
                                                                          "WD"s,  // Waterford (Port Láirge)
                                                                          "WH"s,  // Westmeath (An Iarmhí)
                                                                          "WX"s,  // Wexford (Loch Garman)
                                                                          "WW"s   // Wicklow (Cill Mhantáin)
                                                                      };

/// Italy
enum PRIMARY_ENUM_ITALY { ITALY_GE,   // Genova
                          ITALY_IM,   // Imperia
                          ITALY_SP,   // La Spezia
                          ITALY_SV,   // Savona
                          ITALY_AL,   // Alessandria
                          ITALY_AT,   // Asti
                          ITALY_BI,   // Biella
                          ITALY_CN,   // Cuneo
                          ITALY_NO,   // Novara
                          ITALY_TO,   // Torino
                          ITALY_VB,   // Verbano Cusio Ossola
                          ITALY_VC,   // Vercelli
                          ITALY_AO,   // Aosta
                          ITALY_BG,   // Bergamo
                          ITALY_BS,   // Brescia
                          ITALY_CO,   // Como
                          ITALY_CR,   // Cremona
                          ITALY_LC,   // Lecco
                          ITALY_LO,   // Lodi
                          ITALY_MB,   // Monza e Brianza
                          ITALY_MN,   // Mantova
                          ITALY_MI,   // Milano
                          ITALY_PV,   // Pavia
                          ITALY_SO,   // Sondrio
                          ITALY_VA,   // Varese
                          ITALY_BL,   // Belluno
                          ITALY_PD,   // Padova
                          ITALY_RO,   // Rovigo
                          ITALY_TV,   // Treviso
                          ITALY_VE,   // Venezia
                          ITALY_VR,   // Verona
                          ITALY_VI,   // Vicenza
                          ITALY_BZ,   // Bolzano
                          ITALY_TN,   // Trento
                          ITALY_GO,   // Gorizia
                          ITALY_PN,   // Pordenone
                          ITALY_TS,   // Trieste
                          ITALY_UD,   // Udine
                          ITALY_BO,   // Bologna
                          ITALY_FE,   // Ferrara
                          ITALY_FO,   // Forli (Deprecated)
                          ITALY_FC,   // Forli Cesena
                          ITALY_MO,   // Modena
                          ITALY_PR,   // Parma
                          ITALY_PC,   // Piacenza
                          ITALY_RA,   // Ravenna
                          ITALY_RE,   // Reggio Emilia
                          ITALY_RN,   // Rimini
                          ITALY_AR,   // Arezzo
                          ITALY_FI,   // Firenze
                          ITALY_GR,   // Grosseto
                          ITALY_LI,   // Livorno
                          ITALY_LU,   // Lucca
                          ITALY_MS,   // Massa Carrara
                          ITALY_PT,   // Pistoia
                          ITALY_PI,   // Pisa
                          ITALY_PO,   // Prato
                          ITALY_SI,   // Siena
                          ITALY_CH,   // Chieti
                          ITALY_AQ,   // L'Aquila
                          ITALY_PE,   // Pescara
                          ITALY_TE,   // Teramo
                          ITALY_AN,   // Ancona
                          ITALY_AP,   // Ascoli Piceno
                          ITALY_FM,   // Fermo
                          ITALY_MC,   // Macerata
                          ITALY_PS,   // Pesaro e Urbino (Deprecated)
                          ITALY_PU,   // Pesaro e Urbino
                          ITALY_MT,   // Matera
                          ITALY_BA,   // Bari
                          ITALY_BT,   // Barletta Andria Trani
                          ITALY_BR,   // Brindisi
                          ITALY_FG,   // Foggia
                          ITALY_LE,   // Lecce
                          ITALY_TA,   // Taranto
                          ITALY_PZ,   // Potenza
                          ITALY_CZ,   // Catanzaro
                          ITALY_CS,   // Cosenza
                          ITALY_KR,   // Crotone
                          ITALY_RC,   // Reggio Calabria
                          ITALY_VV,   // Vibo Valentia
                          ITALY_AV,   // Avellino
                          ITALY_BN,   // Benevento
                          ITALY_CE,   // Caserta
                          ITALY_NA,   // Napoli
                          ITALY_SA,   // Salerno
                          ITALY_IS,   // Isernia
                          ITALY_CB,   // Campobasso
                          ITALY_FR,   // Frosinone
                          ITALY_LT,   // Latina
                          ITALY_RI,   // Rieti
                          ITALY_RM,   // Roma
                          ITALY_VT,   // Viterbo
                          ITALY_PG,   // Perugia
                          ITALY_TR,   // Terni
                          ITALY_AG,   // Agrigento
                          ITALY_CL,   // Caltanissetta
                          ITALY_CT,   // Catania
                          ITALY_EN,   // Enna
                          ITALY_ME,   // Messina
                          ITALY_PA,   // Palermo
                          ITALY_RG,   // Ragusa
                          ITALY_SR,   // Siracusa
                          ITALY_TP,   // Trapani
                          N_ITALY_PRIMARIES
                        };

//typedef std::array<std::string, N_ITALY_PRIMARIES> PRIMARY_ITALY_ENUMERATION_TYPE;    ///< primaries for Italy

using PRIMARY_ITALY_ENUMERATION_TYPE = std::array<std::string, N_ITALY_PRIMARIES>;    ///< primaries for Italy

static PRIMARY_ITALY_ENUMERATION_TYPE PRIMARY_ITALY_ENUMERATION = { "GE"s,   // Genova
                                                                      "IM"s,   // Imperia
                                                                      "SP"s,   // La Spezia
                                                                      "SV"s,   // Savona
                                                                      "AL"s,   // Alessandria
                                                                      "AT"s,   // Asti
                                                                      "BI"s,   // Biella
                                                                      "CN"s,   // Cuneo
                                                                      "NO"s,   // Novara
                                                                      "TO"s,   // Torino
                                                                      "VB"s,   // Verbano Cusio Ossola
                                                                      "VC"s,   // Vercelli
                                                                      "AO"s,   // Aosta
                                                                      "BG"s,   // Bergamo
                                                                      "BS"s,   // Brescia
                                                                      "CO"s,   // Como
                                                                      "CR"s,   // Cremona
                                                                      "LC"s,   // Lecco
                                                                      "LO"s,   // Lodi
                                                                      "MB"s,   // Monza e Brianza
                                                                      "MN"s,   // Mantova
                                                                      "MI"s,   // Milano
                                                                      "PV"s,   // Pavia
                                                                      "SO"s,   // Sondrio
                                                                      "VA"s,   // Varese
                                                                      "BL"s,   // Belluno
                                                                      "PD"s,   // Padova
                                                                      "RO"s,   // Rovigo
                                                                      "TV"s,   // Treviso
                                                                      "VE"s,   // Venezia
                                                                      "VR"s,   // Verona
                                                                      "VI"s,   // Vicenza
                                                                      "BZ"s,   // Bolzano
                                                                      "TN"s,   // Trento
                                                                      "GO"s,   // Gorizia
                                                                      "PN"s,   // Pordenone
                                                                      "TS"s,   // Trieste
                                                                      "UD"s,   // Udine
                                                                      "BO"s,   // Bologna
                                                                      "FE"s,   // Ferrara
                                                                      "FO"s,   // Forli (Deprecated)
                                                                      "FC"s,   // Forli Cesena
                                                                      "MO"s,   // Modena
                                                                      "PR"s,   // Parma
                                                                      "PC"s,   // Piacenza
                                                                      "RA"s,   // Ravenna
                                                                      "RE"s,   // Reggio Emilia
                                                                      "RN"s,   // Rimini
                                                                      "AR"s,   // Arezzo
                                                                      "FI"s,   // Firenze
                                                                      "GR"s,   // Grosseto
                                                                      "LI"s,   // Livorno
                                                                      "LU"s,   // Lucca
                                                                      "MS"s,   // Massa Carrara
                                                                      "PT"s,   // Pistoia
                                                                      "PI"s,   // Pisa
                                                                      "PO"s,   // Prato
                                                                      "SI"s,   // Siena
                                                                      "CH"s,   // Chieti
                                                                      "AQ"s,   // L'Aquila
                                                                      "PE"s,   // Pescara
                                                                      "TE"s,   // Teramo
                                                                      "AN"s,   // Ancona
                                                                      "AP"s,   // Ascoli Piceno
                                                                      "FM"s,   // Fermo
                                                                      "MC"s,   // Macerata
                                                                      "PS"s,   // Pesaro e Urbino (Deprecated)
                                                                      "PU"s,   // Pesaro e Urbino
                                                                      "MT"s,   // Matera
                                                                      "BA"s,   // Bari
                                                                      "BT"s,   // Barletta Andria Trani
                                                                      "BR"s,   // Brindisi
                                                                      "FG"s,   // Foggia
                                                                      "LE"s,   // Lecce
                                                                      "TA"s,   // Taranto
                                                                      "PZ"s,   // Potenza
                                                                      "CZ"s,   // Catanzaro
                                                                      "CS"s,   // Cosenza
                                                                      "KR"s,   // Crotone
                                                                      "RC"s,   // Reggio Calabria
                                                                      "VV"s,   // Vibo Valentia
                                                                      "AV"s,   // Avellino
                                                                      "BN"s,   // Benevento
                                                                      "CE"s,   // Caserta
                                                                      "NA"s,   // Napoli
                                                                      "SA"s,   // Salerno
                                                                      "IS"s,   // Isernia
                                                                      "CB"s,   // Campobasso
                                                                      "FR"s,   // Frosinone
                                                                      "LT"s,   // Latina
                                                                      "RI"s,   // Rieti
                                                                      "RM"s,   // Roma
                                                                      "VT"s,   // Viterbo
                                                                      "PG"s,   // Perugia
                                                                      "TR"s,   // Terni
                                                                      "AG"s,   // Agrigento
                                                                      "CL"s,   // Caltanissetta
                                                                      "CT"s,   // Catania
                                                                      "EN"s,   // Enna
                                                                      "ME"s,   // Messina
                                                                      "PA"s,   // Palermo
                                                                      "RG"s,   // Ragusa
                                                                      "SR"s,   // Siracusa
                                                                      "TP"s   // Trapani
                                                                  };

/// Madeira
enum PRIMARY_ENUM_MADEIRA { MADEIRA_MD, // Madeira
                            N_MADEIRA_PRIMARIES
                          };

//typedef std::array<std::string, N_MADEIRA_PRIMARIES> PRIMARY_MADEIRA_ENUMERATION_TYPE;    ///< primaries for Madeira

using PRIMARY_MADEIRA_ENUMERATION_TYPE = std::array<std::string, N_MADEIRA_PRIMARIES>;    ///< primaries for Madeira

static PRIMARY_MADEIRA_ENUMERATION_TYPE PRIMARY_MADEIRA_ENUMERATION = { "MD"s // Madeira
                                                                      };

/// The Netherlands
enum PRIMARY_ENUM_NETHERLANDS { NETHERLANDS_DR,   // Drenthe
                                NETHERLANDS_FR,   // Friesland
                                NETHERLANDS_GR,   // Groningen
                                NETHERLANDS_NB,   // Noord-Brabant
                                NETHERLANDS_OV,   // Overijssel
                                NETHERLANDS_ZH,   // Zuid-Holland
                                NETHERLANDS_FL,   // Flevoland
                                NETHERLANDS_GD,   // Gelderland
                                NETHERLANDS_LB,   // Limburg
                                NETHERLANDS_NH,   // Noord-Holland
                                NETHERLANDS_UT,   // Utrecht
                                NETHERLANDS_ZL,   // Zeeland
                                N_NETHERLANDS_PRIMARIES
                              };

//typedef std::array<std::string, N_NETHERLANDS_PRIMARIES> PRIMARY_NETHERLANDS_ENUMERATION_TYPE;    ///< primaries for Netherlands

using PRIMARY_NETHERLANDS_ENUMERATION_TYPE = std::array<std::string, N_NETHERLANDS_PRIMARIES>;    ///< primaries for Netherlands

static PRIMARY_NETHERLANDS_ENUMERATION_TYPE PRIMARY_NETHERLANDS_ENUMERATION = { "DR"s,   // Drenthe
                                                                                  "FR"s,   // Friesland
                                                                                  "GR"s,   // Groningen
                                                                                  "NB"s,   // Noord-Brabant
                                                                                  "OV"s,   // Overijssel
                                                                                  "ZH"s,   // Zuid-Holland
                                                                                  "FL"s,   // Flevoland
                                                                                  "GD"s,   // Gelderland
                                                                                  "LB"s,   // Limburg
                                                                                  "NH"s,   // Noord-Holland
                                                                                  "UT"s,   // Utrecht
                                                                                  "ZL"s   // Zeeland
                                                                              };

/// Poland
enum PRIMARY_ENUM_POLAND { POLAND_Z,    // Zachodnio-Pomorskie
                           POLAND_F,    // Pomorskie
                           POLAND_P,    // Kujawsko-Pomorskie
                           POLAND_B,    // Lubuskie
                           POLAND_W,    // Wielkopolskie
                           POLAND_J,    // Warminsko-Mazurskie
                           POLAND_O,    // Podlaskie
                           POLAND_R,    // Mazowieckie
                           POLAND_D,    // Dolnoslaskie
                           POLAND_U,    // Opolskie
                           POLAND_C,    // Lodzkie
                           POLAND_S,    // Swietokrzyskie
                           POLAND_K,    // Podkarpackie
                           POLAND_L,    // Lubelskie
                           POLAND_G,    // Slaskie
                           POLAND_M,    // Malopolskie
                           N_POLAND_PRIMARIES
                         };

//typedef std::array<std::string, N_POLAND_PRIMARIES> PRIMARY_POLAND_ENUMERATION_TYPE;    ///< primaries for Poland

using PRIMARY_POLAND_ENUMERATION_TYPE = std::array<std::string, N_POLAND_PRIMARIES>;    ///< primaries for Poland

static PRIMARY_POLAND_ENUMERATION_TYPE PRIMARY_POLAND_ENUMERATION = { "Z"s,    // Zachodnio-Pomorskie
                                                                        "F"s,    // Pomorskie
                                                                        "P"s,    // Kujawsko-Pomorskie
                                                                        "B"s,    // Lubuskie
                                                                        "W"s,    // Wielkopolskie
                                                                        "J"s,    // Warminsko-Mazurskie
                                                                        "O"s,    // Podlaskie
                                                                        "R"s,    // Mazowieckie
                                                                        "D"s,    // Dolnoslaskie
                                                                        "U"s,    // Opolskie
                                                                        "C"s,    // Lodzkie
                                                                        "S"s,    // Swietokrzyskie
                                                                        "K"s,    // Podkarpackie
                                                                        "L"s,    // Lubelskie
                                                                        "G"s,    // Slaskie
                                                                        "M"s,    // Malopolskie
                                                                    };

/// Portugal
enum PRIMARY_ENUM_PORTUGAL { PORTUGAL_AV,   // Aveiro
                             PORTUGAL_BJ,   // Beja
                             PORTUGAL_BR,   // Braga
                             PORTUGAL_BG,   // Bragança
                             PORTUGAL_CB,   // Castelo Branco
                             PORTUGAL_CO,   // Coimbra
                             PORTUGAL_EV,   // Evora
                             PORTUGAL_FR,   // Faro
                             PORTUGAL_GD,   // Guarda
                             PORTUGAL_LR,   // Leiria
                             PORTUGAL_LX,   // Lisboa
                             PORTUGAL_PG,   // Portalegre
                             PORTUGAL_PT,   // Porto
                             PORTUGAL_SR,   // Santarem
                             PORTUGAL_ST,   // Setubal
                             PORTUGAL_VC,   // Viana do Castelo
                             PORTUGAL_VR,   // Vila Real
                             PORTUGAL_VS,   // Viseu
                             N_PORTUGAL_PRIMARIES
                           };

//typedef std::array<std::string, N_PORTUGAL_PRIMARIES> PRIMARY_PORTUGAL_ENUMERATION_TYPE;    ///< primaries for Portugal

using PRIMARY_PORTUGAL_ENUMERATION_TYPE = std::array<std::string, N_PORTUGAL_PRIMARIES>;    ///< primaries for Portugal

static PRIMARY_PORTUGAL_ENUMERATION_TYPE PRIMARY_PORTUGAL_ENUMERATION = { "AV"s,   // Aveiro
                                                                            "BJ"s,   // Beja
                                                                            "BR"s,   // Braga
                                                                            "BG"s,   // Bragança
                                                                            "CB"s,   // Castelo Branco
                                                                            "CO"s,   // Coimbra
                                                                            "EV"s,   // Evora
                                                                            "FR"s,   // Faro
                                                                            "GD"s,   // Guarda
                                                                            "LR"s,   // Leiria
                                                                            "LX"s,   // Lisboa
                                                                            "PG"s,   // Portalegre
                                                                            "PT"s,   // Porto
                                                                            "SR"s,   // Santarem
                                                                            "ST"s,   // Setubal
                                                                            "VC"s,   // Viana do Castelo
                                                                            "VR"s,   // Vila Real
                                                                            "VS"s,   // Viseu
                                                                        };

/// Romania
enum PRIMARY_ENUM_ROMANIA { ROMANIA_AR,   //  Arad
                            ROMANIA_CS,   // Cara'-Severin
                            ROMANIA_HD,   // Hunedoara
                            ROMANIA_TM,   // Timiş (Timis)
                            ROMANIA_BU,   // Bucureşti (Bucure'ti)
                            ROMANIA_IF,   // Ilfov
                            ROMANIA_BR,   // Brăila (Braila)
                            ROMANIA_CT,   // Conatarta
                            ROMANIA_GL,   // Galati
                            ROMANIA_TL,   // Tulcea
                            ROMANIA_VN,   // Vrancea
                            ROMANIA_AB,   // Alba
                            ROMANIA_BH,   // Bihor
                            ROMANIA_BN,   // Bistrita-Nasaud
                            ROMANIA_CJ,   // Cluj
                            ROMANIA_MM,   // Maramureş (Maramures)
                            ROMANIA_SJ,   // Sălaj (Salaj)
                            ROMANIA_SM,   // Satu Mare
                            ROMANIA_BV,   // Braşov (Bra'ov)
                            ROMANIA_CV,   // Covasna
                            ROMANIA_HR,   // Harghita
                            ROMANIA_MS,   // Mureş (Mures)
                            ROMANIA_SB,   // Sibiu
                            ROMANIA_AG,   // Arge'
                            ROMANIA_DJ,   // Dolj
                            ROMANIA_GJ,   // Gorj
                            ROMANIA_MH,   // Mehedinţi (Mehedinti)
                            ROMANIA_OT,   // Olt
                            ROMANIA_VL,   // Vâlcea
                            ROMANIA_BC,   // Bacau
                            ROMANIA_BT,   // Boto'ani
                            ROMANIA_IS,   // Iaşi (Iasi)
                            ROMANIA_NT,   // Neamţ (Neamt)
                            ROMANIA_SV,   // Suceava
                            ROMANIA_VS,   // Vaslui
                            ROMANIA_BZ,   // Buzău (Buzau)
                            ROMANIA_CL,   // Călăraşi (Calarasi)
                            ROMANIA_DB,   // Dâmboviţa (Dambovita)
                            ROMANIA_GR,   // Giurqiu
                            ROMANIA_IL,   // Ialomita
                            ROMANIA_PH,   // Prahova
                            ROMANIA_TR,   // Teleorman
                            N_ROMANIA_PRIMARIES
                          };

//typedef std::array<std::string, N_ROMANIA_PRIMARIES> PRIMARY_ROMANIA_ENUMERATION_TYPE;    ///< primaries for Romania

using PRIMARY_ROMANIA_ENUMERATION_TYPE = std::array<std::string, N_ROMANIA_PRIMARIES>;    ///< primaries for Romania

static PRIMARY_ROMANIA_ENUMERATION_TYPE PRIMARY_ROMANIA_ENUMERATION = { "AR"s, //  Arad
                                                                          "CS"s,   // Cara'-Severin
                                                                          "HD"s,   // Hunedoara
                                                                          "TM"s,   // Timiş (Timis)
                                                                          "BU"s,   // Bucureşti (Bucure'ti)
                                                                          "IF"s,   // Ilfov
                                                                          "BR"s,   // Brăila (Braila)
                                                                          "CT"s,   // Conatarta
                                                                          "GL"s,   // Galati
                                                                          "TL"s,   // Tulcea
                                                                          "VN"s,   // Vrancea
                                                                          "AB"s,   // Alba
                                                                          "BH"s,   // Bihor
                                                                          "BN"s,   // Bistrita-Nasaud
                                                                          "CJ"s,   // Cluj
                                                                          "MM"s,   // Maramureş (Maramures)
                                                                          "SJ"s,   // Sălaj (Salaj)
                                                                          "SM"s,   // Satu Mare
                                                                          "BV"s,   // Braşov (Bra'ov)
                                                                          "CV"s,   // Covasna
                                                                          "HR"s,   // Harghita
                                                                          "MS"s,   // Mureş (Mures)
                                                                          "SB"s,   // Sibiu
                                                                          "AG"s,   // Arge'
                                                                          "DJ"s,   // Dolj
                                                                          "GJ"s,   // Gorj
                                                                          "MH"s,   // Mehedinţi (Mehedinti)
                                                                          "OT"s,   // Olt
                                                                          "VL"s,   // Vâlcea
                                                                          "BC"s,   // Bacau
                                                                          "BT"s,   // Boto'ani
                                                                          "IS"s,   // Iaşi (Iasi)
                                                                          "NT"s,   // Neamţ (Neamt)
                                                                          "SV"s,   // Suceava
                                                                          "VS"s,   // Vaslui
                                                                          "BZ"s,   // Buzău (Buzau)
                                                                          "CL"s,   // Călăraşi (Calarasi)
                                                                          "DB"s,   // Dâmboviţa (Dambovita)
                                                                          "GR"s,   // Giurqiu
                                                                          "IL"s,   // Ialomita
                                                                          "PH"s,   // Prahova
                                                                          "TR"s    // Teleorman
                                                                      };

/// Spain
enum PRIMARY_ENUM_SPAIN { SPAIN_AV,   //  Avila
                          SPAIN_BU,   //  Burgos
                          SPAIN_C,    //  A Coruña
                          SPAIN_LE,   //  Leon
                          SPAIN_LO,   //  La Rioja
                          SPAIN_LU,   //  Lugo
                          SPAIN_O,    //  Asturias
                          SPAIN_OU,   //  Ourense
                          SPAIN_P,    //  Palencia
                          SPAIN_PO,   //  Pontevedra
                          SPAIN_S,    //  Cantabria
                          SPAIN_SA,   //  Salamanca
                          SPAIN_SG,   //  Segovia
                          SPAIN_SO,   //  Soria
                          SPAIN_VA,   //  Valladolid
                          SPAIN_ZA,   //  Zamora
                          SPAIN_BI,   //  Vizcaya
                          SPAIN_HU,   //  Huesca
                          SPAIN_NA,   //  Navarra
                          SPAIN_SS,   //  Guipuzcoa
                          SPAIN_TE,   //  Teruel
                          SPAIN_VI,   //  Alava
                          SPAIN_Z,    //  Zaragoza
                          SPAIN_B,    //  Barcelona
                          SPAIN_GI,   //  Girona
                          SPAIN_L,    //  Lleida
                          SPAIN_T,    //  Tarragona
                          SPAIN_BA,   //  Badajoz
                          SPAIN_CC,   //  Caceres
                          SPAIN_CR,   //  Ciudad Real
                          SPAIN_CU,   //  Cuenca
                          SPAIN_GU,   //  Guadalajara
                          SPAIN_M,    //  Madrid
                          SPAIN_TO,   //  Toledo
                          SPAIN_A,    //  Alicante
                          SPAIN_AB,   //  Albacete
                          SPAIN_CS,   //  Castellon
                          SPAIN_MU,   //  Murcia
                          SPAIN_V,    //  Valencia
                          SPAIN_AL,   //  Almeria
                          SPAIN_CA,   //  Cadiz
                          SPAIN_CO,   //  Cordoba
                          SPAIN_GR,   //  Granada
                          SPAIN_H,    //  Huelva
                          SPAIN_J,    //  Jaen
                          SPAIN_MA,   //  Malaga
                          SPAIN_SE,   //  Sevilla
                          N_SPAIN_PRIMARIES
                       };

//typedef std::array<std::string, N_SPAIN_PRIMARIES> PRIMARY_SPAIN_ENUMERATION_TYPE;    ///< primaries for Spain

using PRIMARY_SPAIN_ENUMERATION_TYPE = std::array<std::string, N_SPAIN_PRIMARIES>;    ///< primaries for Spain

static PRIMARY_SPAIN_ENUMERATION_TYPE PRIMARY_SPAIN_ENUMERATION = { "AV"s,   //  Avila
                                                                      "BU"s,   //  Burgos
                                                                      "C"s,    //  A Coruña
                                                                      "LE"s,   //  Leon
                                                                      "LO"s,   //  La Rioja
                                                                      "LU"s,   //  Lugo
                                                                      "O"s,    //  Asturias
                                                                      "OU"s,   //  Ourense
                                                                      "P"s,    //  Palencia
                                                                      "PO"s,   //  Pontevedra
                                                                      "S"s,    //  Cantabria
                                                                      "SA"s,   //  Salamanca
                                                                      "SG"s,   //  Segovia
                                                                      "SO"s,   //  Soria
                                                                      "VA"s,   //  Valladolid
                                                                      "ZA"s,   //  Zamora
                                                                      "BI"s,   //  Vizcaya
                                                                      "HU"s,   //  Huesca
                                                                      "NA"s,   //  Navarra
                                                                      "SS"s,   //  Guipuzcoa
                                                                      "TE"s,   //  Teruel
                                                                      "VI"s,   //  Alava
                                                                      "Z"s,    //  Zaragoza
                                                                      "B"s,    //  Barcelona
                                                                      "GI"s,   //  Girona
                                                                      "L"s,    //  Lleida
                                                                      "T"s,    //  Tarragona
                                                                      "BA"s,   //  Badajoz
                                                                      "CC"s,   //  Caceres
                                                                      "CR"s,   //  Ciudad Real
                                                                      "CU"s,   //  Cuenca
                                                                      "GU"s,   //  Guadalajara
                                                                      "M"s,    //  Madrid
                                                                      "TO"s,   //  Toledo
                                                                      "A"s,    //  Alicante
                                                                      "AB"s,   //  Albacete
                                                                      "CS"s,   //  Castellon
                                                                      "MU"s,   //  Murcia
                                                                      "V"s,    //  Valencia
                                                                      "AL"s,   //  Almeria
                                                                      "CA"s,   //  Cadiz
                                                                      "CO"s,   //  Cordoba
                                                                      "GR"s,   //  Granada
                                                                      "H"s,    //  Huelva
                                                                      "J"s,    //  Jaen
                                                                      "MA",   //  Malaga
                                                                      "SE"s   //  Sevilla
                                                                  };

/// Sweden
enum PRIMARY_ENUM_SWEDEN { SWEDEN_AB,   //  Stockholm län
                           SWEDEN_I,    //  Gotlands län
                           SWEDEN_BD,   //  Norrbottens län
                           SWEDEN_AC,   //  Västerbottens län
                           SWEDEN_X,    //  Gävleborgs län
                           SWEDEN_Z,    //  Jämtlands län
                           SWEDEN_Y,    //  Västernorrlands län
                           SWEDEN_W,    //  Dalarna län
                           SWEDEN_S,    //  Värmlands län
                           SWEDEN_O,    //  Västra Götalands län
                           SWEDEN_T,    //  Örebro län
                           SWEDEN_E,    //  Östergötlands län
                           SWEDEN_D,    //  Södermanlands län
                           SWEDEN_C,    //  Uppsala län
                           SWEDEN_U,    //  Västmanlands län
                           SWEDEN_N,    //  Hallands län
                           SWEDEN_K,    //  Blekinge län
                           SWEDEN_F,    //  Jönköpings län
                           SWEDEN_H,    //  Kalmar län
                           SWEDEN_G,    //  Kronobergs län
                           SWEDEN_L,    //   Skåne län
                           N_SWEDEN_PRIMARIES
                         };

//typedef std::array<std::string, N_SWEDEN_PRIMARIES> PRIMARY_SWEDEN_ENUMERATION_TYPE;    ///< primaries for Sweden

using PRIMARY_SWEDEN_ENUMERATION_TYPE = std::array<std::string, N_SWEDEN_PRIMARIES>;    ///< primaries for Sweden

static PRIMARY_SWEDEN_ENUMERATION_TYPE PRIMARY_SWEDEN_ENUMERATION = { "AB"s,   //  Stockholm län
                                                                        "I"s,    //  Gotlands län
                                                                        "BD"s,   //  Norrbottens län
                                                                        "AC"s,   //  Västerbottens län
                                                                        "X"s,    //  Gävleborgs län
                                                                        "Z"s,    //  Jämtlands län
                                                                        "Y"s,    //  Västernorrlands län
                                                                        "W"s,    //  Dalarna län
                                                                        "S"s,    //  Värmlands län
                                                                        "O"s,    //  Västra Götalands län
                                                                        "T"s,    //  Örebro län
                                                                        "E"s,    //  Östergötlands län
                                                                        "D"s,    //  Södermanlands län
                                                                        "C"s,    //  Uppsala län
                                                                        "U"s,    //  Västmanlands län
                                                                        "N"s,    //  Hallands län
                                                                        "K"s,    //  Blekinge län
                                                                        "F"s,    //  Jönköpings län
                                                                        "H"s,    //  Kalmar län
                                                                        "G"s,    //  Kronobergs län
                                                                        "L"s     //   Skåne län
                                                                    };

/// Switzerland
enum PRIMARY_ENUM_SWITZERLAND { SWITZERLAND_AG,   //  Aargau
                                SWITZERLAND_AR,   //  Appenzell Ausserrhoden
                                SWITZERLAND_AI,   //  Appenzell Innerrhoden
                                SWITZERLAND_BL,   //  Basel Landschaft
                                SWITZERLAND_BS,   //  Basel Stadt
                                SWITZERLAND_BE,   //  Bern
                                SWITZERLAND_FR,   //  Freiburg / Fribourg
                                SWITZERLAND_GE,   //  Genf / Genève
                                SWITZERLAND_GL,   //  Glarus
                                SWITZERLAND_GR,   //  Graubuenden / Grisons
                                SWITZERLAND_JU,   //  Jura
                                SWITZERLAND_LU,   //  Luzern
                                SWITZERLAND_NE,   //  Neuenburg / Neuchâtel
                                SWITZERLAND_NW,   //  Nidwalden
                                SWITZERLAND_OW,   //  Obwalden
                                SWITZERLAND_SH,   //  Schaffhausen
                                SWITZERLAND_SZ,   //  Schwyz
                                SWITZERLAND_SO,   //  Solothurn
                                SWITZERLAND_SG,   //  St. Gallen
                                SWITZERLAND_TI,   //  Tessin / Ticino
                                SWITZERLAND_TG,   //  Thurgau
                                SWITZERLAND_UR,   //  Uri
                                SWITZERLAND_VD,   //  Waadt / Vaud
                                SWITZERLAND_VS,   //  Wallis / Valais
                                SWITZERLAND_ZH,   //  Zuerich
                                SWITZERLAND_ZG,   //  Zug
                                N_SWITZERLAND_PRIMARIES
                              };

//typedef std::array<std::string, N_SWITZERLAND_PRIMARIES> PRIMARY_SWITZERLAND_ENUMERATION_TYPE;    ///< primaries for Switzerland

using PRIMARY_SWITZERLAND_ENUMERATION_TYPE = std::array<std::string, N_SWITZERLAND_PRIMARIES>;    ///< primaries for Switzerland

static PRIMARY_SWITZERLAND_ENUMERATION_TYPE PRIMARY_SWITZERLAND_ENUMERATION = { "AG"s,   //  Aargau
                                                                                  "AR"s,   //  Appenzell Ausserrhoden
                                                                                  "AI"s,   //  Appenzell Innerrhoden
                                                                                  "BL"s,   //  Basel Landschaft
                                                                                  "BS"s,   //  Basel Stadt
                                                                                  "BE"s,   //  Bern
                                                                                  "FR"s,   //  Freiburg / Fribourg
                                                                                  "GE"s,   //  Genf / Genève
                                                                                  "GL"s,   //  Glarus
                                                                                  "GR"s,   //  Graubuenden / Grisons
                                                                                  "JU"s,   //  Jura
                                                                                  "LU"s,   //  Luzern
                                                                                  "NE"s,   //  Neuenburg / Neuchâtel
                                                                                  "NW"s,   //  Nidwalden
                                                                                  "OW"s,   //  Obwalden
                                                                                  "SH"s,   //  Schaffhausen
                                                                                  "SZ"s,   //  Schwyz
                                                                                  "SO"s,   //  Solothurn
                                                                                  "SG"s,   //  St. Gallen
                                                                                  "TI"s,   //  Tessin / Ticino
                                                                                  "TG"s,   //  Thurgau
                                                                                  "UR"s,   //  Uri
                                                                                  "VD"s,   //  Waadt / Vaud
                                                                                  "VS"s,   //  Wallis / Valais
                                                                                  "ZH"s,   //  Zuerich
                                                                                  "ZG"s    //  Zug
                                                                              };

/// Ukraine
enum PRIMARY_ENUM_UKRAINE { UKRAINE_SU, //  Sums'ka Oblast'
                            UKRAINE_TE,   //  Ternopil's'ka Oblast'
                            UKRAINE_CH,   //  Cherkas'ka Oblast'
                            UKRAINE_ZA,   //  Zakarpats'ka Oblast'
                            UKRAINE_DN,   //  Dnipropetrovs'ka Oblast'
                            UKRAINE_OD,   //  Odes'ka Oblast'
                            UKRAINE_HE,   //  Khersons'ka Oblast'
                            UKRAINE_PO,   //  Poltavs'ka Oblast'
                            UKRAINE_DO,   //  Donets'ka Oblast'
                            UKRAINE_RI,   //  Rivnens'ka Oblast'
                            UKRAINE_HA,   //  Kharkivs'ka Oblast'
                            UKRAINE_LU,   //  Luhans'ka Oblast'
                            UKRAINE_VI,   //  Vinnyts'ka Oblast'
                            UKRAINE_VO,   //  Volyos'ka Oblast'
                            UKRAINE_ZP,   //  Zaporiz'ka Oblast'
                            UKRAINE_CR,   //  Chernihivs'ka Oblast'
                            UKRAINE_IF,   //  Ivano-Frankivs'ka Oblast'
                            UKRAINE_HM,   //  Khmel'nyts'ka Oblast'
                            UKRAINE_KV,   //  Kyïv
                            UKRAINE_KO,   //  Kyivs'ka Oblast'
                            UKRAINE_KI,   //  Kirovohrads'ka Oblast'
                            UKRAINE_LV,   //  L'vivs'ka Oblast'
                            UKRAINE_ZH,   //  Zhytomyrs'ka Oblast'
                            UKRAINE_CN,   //  Chernivets'ka Oblast'
                            UKRAINE_NI,   //  Mykolaivs'ka Oblast'
                            UKRAINE_KR,   //  Respublika Krym
                            UKRAINE_SL,   //  Sevastopol'
                            N_UKRAINE_PRIMARIES
                          };

//typedef std::array<std::string, N_UKRAINE_PRIMARIES> PRIMARY_UKRAINE_ENUMERATION_TYPE;    ///< primaries for Ukraine

using PRIMARY_UKRAINE_ENUMERATION_TYPE = std::array<std::string, N_UKRAINE_PRIMARIES>;    ///< primaries for Ukraine

static PRIMARY_UKRAINE_ENUMERATION_TYPE PRIMARY_UKRAINE_ENUMERATION = { "SU"s, //  Sums'ka Oblast'
                                                                          "TE"s,   //  Ternopil's'ka Oblast'
                                                                          "CH"s,   //  Cherkas'ka Oblast'
                                                                          "ZA"s,   //  Zakarpats'ka Oblast'
                                                                          "DN"s,   //  Dnipropetrovs'ka Oblast'
                                                                          "OD"s,   //  Odes'ka Oblast'
                                                                          "HE"s,   //  Khersons'ka Oblast'
                                                                          "PO"s,   //  Poltavs'ka Oblast'
                                                                          "DO"s,   //  Donets'ka Oblast'
                                                                          "RI"s,   //  Rivnens'ka Oblast'
                                                                          "HA"s,   //  Kharkivs'ka Oblast'
                                                                          "LU"s,   //  Luhans'ka Oblast'
                                                                          "VI"s,   //  Vinnyts'ka Oblast'
                                                                          "VO"s,   //  Volyos'ka Oblast'
                                                                          "ZP"s,   //  Zaporiz'ka Oblast'
                                                                          "CR"s,   //  Chernihivs'ka Oblast'
                                                                          "IF"s,   //  Ivano-Frankivs'ka Oblast'
                                                                          "HM"s,   //  Khmel'nyts'ka Oblast'
                                                                          "KV"s,   //  Kyïv
                                                                          "KO"s,   //  Kyivs'ka Oblast'
                                                                          "KI"s,   //  Kirovohrads'ka Oblast'
                                                                          "LV"s,   //  L'vivs'ka Oblast'
                                                                          "ZH"s,   //  Zhytomyrs'ka Oblast'
                                                                          "CN"s,   //  Chernivets'ka Oblast'
                                                                          "NI"s,   //  Mykolaivs'ka Oblast'
                                                                          "KR"s,   //  Respublika Krym
                                                                          "SL"s    //  Sevastopol'
                                                                      };

/// United States
enum PRIMARY_ENUM_UNITED_STATES { UNITED_STATES_CT,   //  Connecticut
                                  UNITED_STATES_ME,   //  Maine
                                  UNITED_STATES_MA,   //  Massachusetts
                                  UNITED_STATES_NH,   //  New Hampshire
                                  UNITED_STATES_RI,   //  Rhode Island
                                  UNITED_STATES_VT,   //  Vermont
                                  UNITED_STATES_NJ,   //  New Jersey
                                  UNITED_STATES_NY,   //  New York
                                  UNITED_STATES_DE,   //  Delaware
                                  UNITED_STATES_DC,   //  District of Columbia
                                  UNITED_STATES_MD,   //  Maryland
                                  UNITED_STATES_PA,   //  Pennsylvania
                                  UNITED_STATES_AL,   //  Alabama
                                  UNITED_STATES_FL,   //  Florida
                                  UNITED_STATES_GA,   //  Georgia
                                  UNITED_STATES_KY,   //  Kentucky
                                  UNITED_STATES_NC,   //  North Carolina
                                  UNITED_STATES_SC,   //  South Carolina
                                  UNITED_STATES_TN,   //  Tennessee
                                  UNITED_STATES_VA,   //  Virginia
                                  UNITED_STATES_AR,   //  Arkansas
                                  UNITED_STATES_LA,   //  Louisiana
                                  UNITED_STATES_MS,   //  Mississippi
                                  UNITED_STATES_NM,   //  New Mexico
                                  UNITED_STATES_OK,   //  Oklahoma
                                  UNITED_STATES_TX,   //  Texas
                                  UNITED_STATES_CA,   //  California
                                  UNITED_STATES_AZ,   //  Arizona
                                  UNITED_STATES_ID,   //  Idaho
                                  UNITED_STATES_MT,   //  Montana
                                  UNITED_STATES_NV,   //  Nevada
                                  UNITED_STATES_OR,   //  Oregon
                                  UNITED_STATES_UT,   //  Utah
                                  UNITED_STATES_WA,   //  Washington
                                  UNITED_STATES_WY,   //  Wyoming
                                  UNITED_STATES_MI,   //  Michigan
                                  UNITED_STATES_OH,   //  Ohio
                                  UNITED_STATES_WV,   //  West Virginia
                                  UNITED_STATES_IL,   //  Illinois
                                  UNITED_STATES_IN,   //  Indiana
                                  UNITED_STATES_WI,   //  Wisconsin
                                  UNITED_STATES_CO,   //  Colorado
                                  UNITED_STATES_IA,   //  Iowa
                                  UNITED_STATES_KS,   //  Kansas
                                  UNITED_STATES_MN,   //  Minnesota
                                  UNITED_STATES_MO,   //  Missouri
                                  UNITED_STATES_NE,   //  Nebraska
                                  UNITED_STATES_ND,   //  North Dakota
                                  UNITED_STATES_SD,   //  South Dakota
                                  N_UNITED_STATES_PRIMARIES
                                };

//typedef std::array<std::string, N_UNITED_STATES_PRIMARIES> PRIMARY_UNITED_STATES_ENUMERATION_TYPE;    ///< primaries for United States

using PRIMARY_UNITED_STATES_ENUMERATION_TYPE = std::array<std::string, N_UNITED_STATES_PRIMARIES>;    ///< primaries for United States

static PRIMARY_UNITED_STATES_ENUMERATION_TYPE PRIMARY_UNITED_STATES_ENUMERATION = { "CT"s,   //  Connecticut
                                                                                      "ME"s,   //  Maine
                                                                                      "MA"s,   //  Massachusetts
                                                                                      "NH"s,   //  New Hampshire
                                                                                      "RI"s,   //  Rhode Island
                                                                                      "VT"s,   //  Vermont
                                                                                      "NJ"s,   //  New Jersey
                                                                                      "NY"s,   //  New York
                                                                                      "DE"s,   //  Delaware
                                                                                      "DC"s,   //  District of Columbia
                                                                                      "MD"s,   //  Maryland
                                                                                      "PA"s,   //  Pennsylvania
                                                                                      "AL"s,   //  Alabama
                                                                                      "FL"s,   //  Florida
                                                                                      "GA"s,   //  Georgia
                                                                                      "KY"s,   //  Kentucky
                                                                                      "NC"s,   //  North Carolina
                                                                                      "SC"s,   //  South Carolina
                                                                                      "TN"s,   //  Tennessee
                                                                                      "VA"s,   //  Virginia
                                                                                      "AR"s,   //  Arkansas
                                                                                      "LA"s,   //  Louisiana
                                                                                      "MS"s,   //  Mississippi
                                                                                      "NM"s,   //  New Mexico
                                                                                      "OK"s,   //  Oklahoma
                                                                                      "TX"s,   //  Texas
                                                                                      "CA"s,   //  California
                                                                                      "AZ"s,   //  Arizona
                                                                                      "ID"s,   //  Idaho
                                                                                      "MT"s,   //  Montana
                                                                                      "NV"s,   //  Nevada
                                                                                      "OR"s,   //  Oregon
                                                                                      "UT"s,   //  Utah
                                                                                      "WA"s,   //  Washington
                                                                                      "WY"s,   //  Wyoming
                                                                                      "MI"s,   //  Michigan
                                                                                      "OH"s,   //  Ohio
                                                                                      "WV"s,   //  West Virginia
                                                                                      "IL"s,   //  Illinois
                                                                                      "IN"s,   //  Indiana
                                                                                      "WI"s,   //  Wisconsin
                                                                                      "CO"s,   //  Colorado
                                                                                      "IA"s,   //  Iowa
                                                                                      "KS"s,   //  Kansas
                                                                                      "MN"s,   //  Minnesota
                                                                                      "MO"s,   //  Missouri
                                                                                      "NE"s,   //  Nebraska
                                                                                      "ND"s,   //  North Dakota
                                                                                      "SD"s    //  South Dakota
                                                                                  };

/// Japan
enum PRIMARY_ENUM_JAPAN { JAPAN_12,   //  Chiba
                          JAPAN_16,   //  Gunma
                          JAPAN_14,   //  Ibaraki
                          JAPAN_11,   //  Kanagawa
                          JAPAN_13,   //  Saitama
                          JAPAN_15,   //  Tochigi
                          JAPAN_10,   //  Tokyo
                          JAPAN_17,   //  Yamanashi
                          JAPAN_20,   //  Aichi
                          JAPAN_19,   //  Gifu
                          JAPAN_21,   //  Mie
                          JAPAN_18,   //  Shizuoka
                          JAPAN_27,   //  Hyogo
                          JAPAN_22,   //  Kyoto
                          JAPAN_24,   //  Nara
                          JAPAN_25,   //  Osaka
                          JAPAN_23,   //  Shiga
                          JAPAN_26,   //  Wakayama
                          JAPAN_35,   //  Hiroshima
                          JAPAN_31,   //  Okayama
                          JAPAN_32,   //  Shimane
                          JAPAN_34,   //  Tottori
                          JAPAN_33,   //  Yamaguchi
                          JAPAN_38,   //  Ehime
                          JAPAN_36,   //  Kagawa
                          JAPAN_39,   //  Kochi
                          JAPAN_37,   //  Tokushima
                          JAPAN_40,   //  Fukuoka
                          JAPAN_46,   //  Kagoshima
                          JAPAN_43,   //  Kumamoto
                          JAPAN_45,   //  Miyazaki
                          JAPAN_42,   //  Nagasaki
                          JAPAN_44,   //  Oita
                          JAPAN_47,   //  Okinawa
                          JAPAN_41,   //  Saga
                          JAPAN_04,   //  Akita
                          JAPAN_02,   //  Aomori
                          JAPAN_07,   //  Fukushima
                          JAPAN_03,   //  Iwate
                          JAPAN_06,   //  Miyagi
                          JAPAN_05,   //  Yamagata
                          JAPAN_01,   //  Hokkaido
                          JAPAN_29,   //  Fukui
                          JAPAN_30,   //  Ishikawa
                          JAPAN_28,   //  Toyama
                          JAPAN_09,   //  Nagano
                          JAPAN_08,   //  Niigata
                          N_JAPAN_PRIMARIES
                        };

//typedef std::array<std::string, N_JAPAN_PRIMARIES> PRIMARY_JAPAN_ENUMERATION_TYPE;    ///< primaries for Japan

using PRIMARY_JAPAN_ENUMERATION_TYPE = std::array<std::string, N_JAPAN_PRIMARIES>;    ///< primaries for Japan

static PRIMARY_JAPAN_ENUMERATION_TYPE PRIMARY_JAPAN_ENUMERATION = { "12"s,   //  Chiba
                                                                      "16"s,   //  Gunma
                                                                      "14"s,   //  Ibaraki
                                                                      "11"s,   //  Kanagawa
                                                                      "13"s,   //  Saitama
                                                                      "15"s,   //  Tochigi
                                                                      "10"s,   //  Tokyo
                                                                      "17"s,   //  Yamanashi
                                                                      "20"s,   //  Aichi
                                                                      "19"s,   //  Gifu
                                                                      "21"s,   //  Mie
                                                                      "18"s,   //  Shizuoka
                                                                      "27"s,   //  Hyogo
                                                                      "22"s,   //  Kyoto
                                                                      "24"s,   //  Nara
                                                                      "25"s,   //  Osaka
                                                                      "23"s,   //  Shiga
                                                                      "26"s,   //  Wakayama
                                                                      "35"s,   //  Hiroshima
                                                                      "31"s,   //  Okayama
                                                                      "32"s,   //  Shimane
                                                                      "34"s,   //  Tottori
                                                                      "33"s,   //  Yamaguchi
                                                                      "38"s,   //  Ehime
                                                                      "36"s,   //  Kagawa
                                                                      "39"s,   //  Kochi
                                                                      "37"s,   //  Tokushima
                                                                      "40"s,   //  Fukuoka
                                                                      "46"s,   //  Kagoshima
                                                                      "43"s,   //  Kumamoto
                                                                      "45"s,   //  Miyazaki
                                                                      "42"s,   //  Nagasaki
                                                                      "44"s,   //  Oita
                                                                      "47"s,   //  Okinawa
                                                                      "41"s,   //  Saga
                                                                      "04"s,   //  Akita
                                                                      "02"s,   //  Aomori
                                                                      "07"s,   //  Fukushima
                                                                      "03"s,   //  Iwate
                                                                      "06"s,   //  Miyagi
                                                                      "05"s,   //  Yamagata
                                                                      "01"s,   //  Hokkaido
                                                                      "29"s,   //  Fukui
                                                                      "30"s,   //  Ishikawa
                                                                      "28"s,   //  Toyama
                                                                      "09"s,   //  Nagano
                                                                      "08"s    //  Niigata
                                                                  };

/// Philippines
enum PRIMARY_ENUM_PHILIPPINES { PHILIPPINES_AUR,  //  Aurora
                                PHILIPPINES_BTG,  //  Batangas
                                PHILIPPINES_CAV,  //  Cavite
                                PHILIPPINES_LAG,  //  Laguna
                                PHILIPPINES_MAD,  //  Marinduque
                                PHILIPPINES_MDC,  //  Mindoro Occidental
                                PHILIPPINES_MDR,  //  Mindoro Oriental
                                PHILIPPINES_PLW,  //  Palawan
                                PHILIPPINES_QUE,  //  Quezon
                                PHILIPPINES_RIZ,  //  Rizal
                                PHILIPPINES_ROM,  //  Romblon
                                PHILIPPINES_ILN,  //  Ilocos Norte
                                PHILIPPINES_ILS,  //  Ilocos Sur
                                PHILIPPINES_LUN,  //  La Union
                                PHILIPPINES_PAN,  //  Pangasinan
                                PHILIPPINES_BTN,  //  Batanes
                                PHILIPPINES_CAG,  //  Cagayan
                                PHILIPPINES_ISA,  //  Isabela
                                PHILIPPINES_NUV,  //  Nueva Vizcaya
                                PHILIPPINES_QUI,  //  Quirino
                                PHILIPPINES_ABR,  //  Abra
                                PHILIPPINES_APA,  //  Apayao
                                PHILIPPINES_BEN,  //  Benguet
                                PHILIPPINES_IFU,  //  Ifugao
                                PHILIPPINES_KAL,  //  Kalinga-Apayso
                                PHILIPPINES_MOU,  //  Mountain Province
                                PHILIPPINES_BAN,  //  Batasn
                                PHILIPPINES_BUL,  //  Bulacan
                                PHILIPPINES_NUE,  //  Nueva Ecija
                                PHILIPPINES_PAM,  //  Pampanga
                                PHILIPPINES_TAR,  //  Tarlac
                                PHILIPPINES_ZMB,  //  Zambales
                                PHILIPPINES_ALB,  //  Albay
                                PHILIPPINES_CAN,  //  Camarines Norte
                                PHILIPPINES_CAS,  //  Camarines Sur
                                PHILIPPINES_CAT,  //  Catanduanes
                                PHILIPPINES_MAS,  //  Masbate
                                PHILIPPINES_SOR,  //  Sorsogon
                                PHILIPPINES_BIL,  //  Biliran
                                PHILIPPINES_EAS,  //  Eastern Samar
                                PHILIPPINES_LEY,  //  Leyte
                                PHILIPPINES_NSA,  //  Northern Samar
                                PHILIPPINES_SLE,  //  Southern Leyte
                                PHILIPPINES_WSA,  //  Western Samar
                                PHILIPPINES_AKL,  //  Aklan
                                PHILIPPINES_ANT,  //  Antique
                                PHILIPPINES_CAP,  //  Capiz
                                PHILIPPINES_GUI,  //  Guimaras
                                PHILIPPINES_ILI,  //  Iloilo
                                PHILIPPINES_NEC,  //  Negroe Occidental
                                PHILIPPINES_BOH,  //  Bohol
                                PHILIPPINES_CEB,  //  Cebu
                                PHILIPPINES_NER,  //  Negros Oriental
                                PHILIPPINES_SIG,  //  Siquijor
                                PHILIPPINES_ZAN,  //  Zamboanga del Norte
                                PHILIPPINES_ZAS,  //  Zamboanga del Sur
                                PHILIPPINES_ZSI,  //  Zamboanga Sibugay
                                PHILIPPINES_NCO,  //  North Cotabato
                                PHILIPPINES_SUK,  //  Sultan Kudarat
                                PHILIPPINES_SAR,  //  Sarangani
                                PHILIPPINES_SCO,  //  South Cotabato
                                PHILIPPINES_BAS,  //  Basilan
                                PHILIPPINES_LAS,  //  Lanao del Sur
                                PHILIPPINES_MAG,  //  Maguindanao
                                PHILIPPINES_SLU,  //  Sulu
                                PHILIPPINES_TAW,  //  Tawi-Tawi
                                PHILIPPINES_LAN,  //  Lanao del Norte
                                PHILIPPINES_BUK,  //  Bukidnon
                                PHILIPPINES_CAM,  //  Camiguin
                                PHILIPPINES_MSC,  //  Misamis Occidental
                                PHILIPPINES_MSR,  //  Misamis Oriental
                                PHILIPPINES_COM,  //  Compostela Valley
                                PHILIPPINES_DAV,  //  Davao del Norte
                                PHILIPPINES_DAS,  //  Davao del Sur
                                PHILIPPINES_DAO,  //  Davao Oriental
                                PHILIPPINES_AGN,  //  Agusan del Norte
                                PHILIPPINES_AGS,  //  Agusan del Sur
                                PHILIPPINES_SUN,  //  Surigao del Norte
                                PHILIPPINES_SUR,  //  Surigao del Sur
                                N_PHILIPPINES_PRIMARIES
                              };

//typedef std::array<std::string, N_PHILIPPINES_PRIMARIES> PRIMARY_PHILIPPINES_ENUMERATION_TYPE;    ///< primaries for Philippines

using PRIMARY_PHILIPPINES_ENUMERATION_TYPE = std::array<std::string, N_PHILIPPINES_PRIMARIES>;    ///< primaries for Philippines

static PRIMARY_PHILIPPINES_ENUMERATION_TYPE PRIMARY_PHILIPPINES_ENUMERATION = { "AUR"s,  //  Aurora
                                                                                  "BTG"s,  //  Batangas
                                                                                  "CAV"s,  //  Cavite
                                                                                  "LAG"s,  //  Laguna
                                                                                  "MAD"s,  //  Marinduque
                                                                                  "MDC"s,  //  Mindoro Occidental
                                                                                  "MDR"s,  //  Mindoro Oriental
                                                                                  "PLW"s,  //  Palawan
                                                                                  "QUE"s,  //  Quezon
                                                                                  "RIZ"s,  //  Rizal
                                                                                  "ROM"s,  //  Romblon
                                                                                  "ILN"s,  //  Ilocos Norte
                                                                                  "ILS"s,  //  Ilocos Sur
                                                                                  "LUN"s,  //  La Union
                                                                                  "PAN"s,  //  Pangasinan
                                                                                  "BTN"s,  //  Batanes
                                                                                  "CAG"s,  //  Cagayan
                                                                                  "ISA"s,  //  Isabela
                                                                                  "NUV"s,  //  Nueva Vizcaya
                                                                                  "QUI"s,  //  Quirino
                                                                                  "ABR"s,  //  Abra
                                                                                  "APA"s,  //  Apayao
                                                                                  "BEN"s,  //  Benguet
                                                                                  "IFU"s,  //  Ifugao
                                                                                  "KAL"s,  //  Kalinga-Apayso
                                                                                  "MOU"s,  //  Mountain Province
                                                                                  "BAN"s,  //  Batasn
                                                                                  "BUL"s,  //  Bulacan
                                                                                  "NUE"s,  //  Nueva Ecija
                                                                                  "PAM"s,  //  Pampanga
                                                                                  "TAR"s,  //  Tarlac
                                                                                  "ZMB"s,  //  Zambales
                                                                                  "ALB"s,  //  Albay
                                                                                  "CAN"s,  //  Camarines Norte
                                                                                  "CAS"s,  //  Camarines Sur
                                                                                  "CAT"s,  //  Catanduanes
                                                                                  "MAS"s,  //  Masbate
                                                                                  "SOR"s,  //  Sorsogon
                                                                                  "BIL"s,  //  Biliran
                                                                                  "EAS"s,  //  Eastern Samar
                                                                                  "LEY"s,  //  Leyte
                                                                                  "NSA"s,  //  Northern Samar
                                                                                  "SLE"s,  //  Southern Leyte
                                                                                  "WSA"s,  //  Western Samar
                                                                                  "AKL"s,  //  Aklan
                                                                                  "ANT"s,  //  Antique
                                                                                  "CAP"s,  //  Capiz
                                                                                  "GUI"s,  //  Guimaras
                                                                                  "ILI"s,  //  Iloilo
                                                                                  "NEC"s,  //  Negroe Occidental
                                                                                  "BOH"s,  //  Bohol
                                                                                  "CEB"s,  //  Cebu
                                                                                  "NER"s,  //  Negros Oriental
                                                                                  "SIG"s,  //  Siquijor
                                                                                  "ZAN"s,  //  Zamboanga del Norte
                                                                                  "ZAS"s,  //  Zamboanga del Sur
                                                                                  "ZSI"s,  //  Zamboanga Sibugay
                                                                                  "NCO"s,  //  North Cotabato
                                                                                  "SUK"s,  //  Sultan Kudarat
                                                                                  "SAR"s,  //  Sarangani
                                                                                  "SCO"s,  //  South Cotabato
                                                                                  "BAS"s,  //  Basilan
                                                                                  "LAS"s,  //  Lanao del Sur
                                                                                  "MAG"s,  //  Maguindanao
                                                                                  "SLU"s,  //  Sulu
                                                                                  "TAW"s,  //  Tawi-Tawi
                                                                                  "LAN"s,  //  Lanao del Norte
                                                                                  "BUK"s,  //  Bukidnon
                                                                                  "CAM"s,  //  Camiguin
                                                                                  "MSC"s,  //  Misamis Occidental
                                                                                  "MSR"s,  //  Misamis Oriental
                                                                                  "COM"s,  //  Compostela Valley
                                                                                  "DAV"s,  //  Davao del Norte
                                                                                  "DAS"s,  //  Davao del Sur
                                                                                  "DAO"s,  //  Davao Oriental
                                                                                  "AGN"s,  //  Agusan del Norte
                                                                                  "AGS"s,  //  Agusan del Sur
                                                                                  "SUN"s,  //  Surigao del Norte
                                                                                  "SUR"s   //  Surigao del Sur
                                                                              };

/// Croatia
enum PRIMARY_ENUM_CROATIA { CROATIA_01,  //  Zagrebačka županija
                            CROATIA_02,  //  Krapinsko-Zagorska županija
                            CROATIA_03,  //  Sisačko-Moslavačka županija
                            CROATIA_04,  //  Karlovačka županija
                            CROATIA_05,  //  Varaždinska županija
                            CROATIA_06,  //  Koprivničko-Križevačka županija
                            CROATIA_07,  //  Bjelovarsko-Bilogorska županija
                            CROATIA_08,  //  Primorsko-Goranska županija
                            CROATIA_09,  //  Ličko-Senjska županija
                            CROATIA_10,  //  Virovitičko-Podravska županija
                            CROATIA_11,  //  Požeško-Slavonska županija
                            CROATIA_12,  //  Brodsko-Posavska županija
                            CROATIA_13,  //  Zadarska županija
                            CROATIA_14,  //  Osječko-Baranjska županija
                            CROATIA_15,  //  Šibensko-Kninska županija
                            CROATIA_16,  //  Vukovarsko-Srijemska županija
                            CROATIA_17,  //  Splitsko-Dalmatinska županija
                            CROATIA_18,  //  Istarska županija
                            CROATIA_19,  //  Dubrovačko-Neretvanska županija
                            CROATIA_20,  //  Međimurska županija
                            CROATIA_21,  //  Grad Zagreb
                            N_CROATIA_PRIMARIES
                          };

//typedef std::array<std::string, N_CROATIA_PRIMARIES> PRIMARY_CROATIA_ENUMERATION_TYPE;    ///< primaries for Croatia

using PRIMARY_CROATIA_ENUMERATION_TYPE = std::array<std::string, N_CROATIA_PRIMARIES>;    ///< primaries for Croatia

static PRIMARY_CROATIA_ENUMERATION_TYPE PRIMARY_CROATIA_ENUMERATION = { "01"s,  //  Zagrebačka županija
                                                                          "02"s,  //  Krapinsko-Zagorska županija
                                                                          "03"s,  //  Sisačko-Moslavačka županija
                                                                          "04"s,  //  Karlovačka županija
                                                                          "05"s,  //  Varaždinska županija
                                                                          "06"s,  //  Koprivničko-Križevačka županija
                                                                          "07"s,  //  Bjelovarsko-Bilogorska županija
                                                                          "08"s,  //  Primorsko-Goranska županija
                                                                          "09"s,  //  Ličko-Senjska županija
                                                                          "10"s,  //  Virovitičko-Podravska županija
                                                                          "11"s,  //  Požeško-Slavonska županija
                                                                          "12"s,  //  Brodsko-Posavska županija
                                                                          "13"s,  //  Zadarska županija
                                                                          "14"s,  //  Osječko-Baranjska županija
                                                                          "15"s,  //  Šibensko-Kninska županija
                                                                          "16"s,  //  Vukovarsko-Srijemska županija
                                                                          "17"s,  //  Splitsko-Dalmatinska županija
                                                                          "18"s,  //  Istarska županija
                                                                          "19"s,  //  Dubrovačko-Neretvanska županija
                                                                          "20"s,  //  Međimurska županija
                                                                          "21"s   //  Grad Zagreb
                                                                      };

/// Czech Republic
enum PRIMARY_ENUM_CZECH { CZECH_APA,  //  Praha 1
                          CZECH_APB,  //  Praha 2
                          CZECH_APC,  //  Praha 3
                          CZECH_APD,  //  Praha 4
                          CZECH_APE,  //  Praha 5
                          CZECH_APF,  //  Praha 6
                          CZECH_APG,  //  Praha 7
                          CZECH_APH,  //  Praha 8
                          CZECH_API,  //  Praha 9
                          CZECH_APJ,  //  Praha 10
                          CZECH_BBN,  //  Benesov
                          CZECH_BBE,  //  Beroun
                          CZECH_BKD,  //  Kladno
                          CZECH_BKO,  //  Kolin
                          CZECH_BKH,  //  Kutna Hora
                          CZECH_BME,  //  Melnik
                          CZECH_BMB,  //  Mlada Boleslav
                          CZECH_BNY,  //  Nymburk
                          CZECH_BPZ,  //  Praha zapad
                          CZECH_BPV,  //  Praha vychod
                          CZECH_BPB,  //  Pribram
                          CZECH_BRA,  //  Rakovnik
                          CZECH_CBU,  //  Ceske Budejovice
                          CZECH_CCK,  //  Cesky Krumlov
                          CZECH_CJH,  //  Jindrichuv Hradec
                          CZECH_CPE,  //  Pelhrimov
                          CZECH_CPI,  //  Pisek
                          CZECH_CPR,  //  Prachatice
                          CZECH_CST,  //  Strakonice
                          CZECH_CTA,  //  Tabor
                          CZECH_DDO,  //  Domazlice
                          CZECH_DCH,  //  Cheb
                          CZECH_DKV,  //  Karlovy Vary
                          CZECH_DKL,  //  Klatovy
                          CZECH_DPM,  //  Plzen mesto
                          CZECH_DPJ,  //  Plzen jih
                          CZECH_DPS,  //  Plzen sever
                          CZECH_DRO,  //  Rokycany
                          CZECH_DSO,  //  Sokolov
                          CZECH_DTA,  //  Tachov
                          CZECH_ECL,  //  Ceska Lipa
                          CZECH_EDE,  //  Decin
                          CZECH_ECH,  //  Chomutov
                          CZECH_EJA,  //  Jablonec n. Nisou
                          CZECH_ELI,  //  Liberec
                          CZECH_ELT,  //  Litomerice
                          CZECH_ELO,  //  Louny
                          CZECH_EMO,  //  Most
                          CZECH_ETE,  //  Teplice
                          CZECH_EUL,  //  Usti nad Labem
                          CZECH_FHB,  //  Havlickuv Brod
                          CZECH_FHK,  //  Hradec Kralove
                          CZECH_FCR,  //  Chrudim
                          CZECH_FJI,  //  Jicin
                          CZECH_FNA,  //  Nachod
                          CZECH_FPA,  //  Pardubice
                          CZECH_FRK,  //  Rychn n. Kneznou
                          CZECH_FSE,  //  Semily
                          CZECH_FSV,  //  Svitavy
                          CZECH_FTR,  //  Trutnov
                          CZECH_FUO,  //  Usti nad Orlici
                          CZECH_GBL,  //  Blansko
                          CZECH_GBM,  //  Brno mesto
                          CZECH_GBV,  //  Brno venkov
                          CZECH_GBR,  //  Breclav
                          CZECH_GHO,  //  Hodonin
                          CZECH_GJI,  //  Jihlava
                          CZECH_GKR,  //  Kromeriz
                          CZECH_GPR,  //  Prostejov
                          CZECH_GTR,  //  Trebic
                          CZECH_GUH,  //  Uherske Hradiste
                          CZECH_GVY,  //  Vyskov
                          CZECH_GZL,  //  Zlin
                          CZECH_GZN,  //  Znojmo
                          CZECH_GZS,  //  Zdar nad Sazavou
                          CZECH_HBR,  //  Bruntal
                          CZECH_HFM,  //  Frydek-Mistek
                          CZECH_HJE,  //  Jesenik
                          CZECH_HKA,  //  Karvina
                          CZECH_HNJ,  //  Novy Jicin
                          CZECH_HOL,  //  Olomouc
                          CZECH_HOP,  //  Opava
                          CZECH_HOS,  //  Ostrava
                          CZECH_HPR,  //  Prerov
                          CZECH_HSU,  //  Sumperk
                          CZECH_HVS,  //  Vsetin
                          N_CZECH_PRIMARIES
                        };

//typedef std::array<std::string, N_CZECH_PRIMARIES> PRIMARY_CZECH_ENUMERATION_TYPE;    ///< primaries for Czech Republic

using PRIMARY_CZECH_ENUMERATION_TYPE = std::array<std::string, N_CZECH_PRIMARIES>;    ///< primaries for Czech Republic

static PRIMARY_CZECH_ENUMERATION_TYPE PRIMARY_CZECH_ENUMERATION = { "APA"s,  //  Praha 1
                                                                      "APB"s,  //  Praha 2
                                                                      "APC"s,  //  Praha 3
                                                                      "APD"s,  //  Praha 4
                                                                      "APE"s,  //  Praha 5
                                                                      "APF"s,  //  Praha 6
                                                                      "APG"s,  //  Praha 7
                                                                      "APH"s,  //  Praha 8
                                                                      "API"s,  //  Praha 9
                                                                      "APJ"s,  //  Praha 10
                                                                      "BBN"s,  //  Benesov
                                                                      "BBE"s,  //  Beroun
                                                                      "BKD"s,  //  Kladno
                                                                      "BKO"s,  //  Kolin
                                                                      "BKH"s,  //  Kutna Hora
                                                                      "BME"s,  //  Melnik
                                                                      "BMB"s,  //  Mlada Boleslav
                                                                      "BNY"s,  //  Nymburk
                                                                      "BPZ"s,  //  Praha zapad
                                                                      "BPV"s,  //  Praha vychod
                                                                      "BPB"s,  //  Pribram
                                                                      "BRA"s,  //  Rakovnik
                                                                      "CBU"s,  //  Ceske Budejovice
                                                                      "CCK"s,  //  Cesky Krumlov
                                                                      "CJH"s,  //  Jindrichuv Hradec
                                                                      "CPE"s,  //  Pelhrimov
                                                                      "CPI"s,  //  Pisek
                                                                      "CPR"s,  //  Prachatice
                                                                      "CST"s,  //  Strakonice
                                                                      "CTA"s,  //  Tabor
                                                                      "DDO"s,  //  Domazlice
                                                                      "DCH"s,  //  Cheb
                                                                      "DKV"s,  //  Karlovy Vary
                                                                      "DKL"s,  //  Klatovy
                                                                      "DPM"s,  //  Plzen mesto
                                                                      "DPJ"s,  //  Plzen jih
                                                                      "DPS"s,  //  Plzen sever
                                                                      "DRO"s,  //  Rokycany
                                                                      "DSO"s,  //  Sokolov
                                                                      "DTA"s,  //  Tachov
                                                                      "ECL"s,  //  Ceska Lipa
                                                                      "EDE"s,  //  Decin
                                                                      "ECH"s,  //  Chomutov
                                                                      "EJA"s,  //  Jablonec n. Nisou
                                                                      "ELI"s,  //  Liberec
                                                                      "ELT"s,  //  Litomerice
                                                                      "ELO"s,  //  Louny
                                                                      "EMO"s,  //  Most
                                                                      "ETE"s,  //  Teplice
                                                                      "EUL"s,  //  Usti nad Labem
                                                                      "FHB"s,  //  Havlickuv Brod
                                                                      "FHK"s,  //  Hradec Kralove
                                                                      "FCR"s,  //  Chrudim
                                                                      "FJI"s,  //  Jicin
                                                                      "FNA"s,  //  Nachod
                                                                      "FPA"s,  //  Pardubice
                                                                      "FRK"s,  //  Rychn n. Kneznou
                                                                      "FSE"s,  //  Semily
                                                                      "FSV"s,  //  Svitavy
                                                                      "FTR"s,  //  Trutnov
                                                                      "FUO"s,  //  Usti nad Orlici
                                                                      "GBL"s,  //  Blansko
                                                                      "GBM"s,  //  Brno mesto
                                                                      "GBV"s,  //  Brno venkov
                                                                      "GBR"s,  //  Breclav
                                                                      "GHO"s,  //  Hodonin
                                                                      "GJI"s,  //  Jihlava
                                                                      "GKR"s,  //  Kromeriz
                                                                      "GPR"s,  //  Prostejov
                                                                      "GTR"s,  //  Trebic
                                                                      "GUH"s,  //  Uherske Hradiste
                                                                      "GVY"s,  //  Vyskov
                                                                      "GZL"s,  //  Zlin
                                                                      "GZN"s,  //  Znojmo
                                                                      "GZS"s,  //  Zdar nad Sazavou
                                                                      "HBR"s,  //  Bruntal
                                                                      "HFM"s,  //  Frydek-Mistek
                                                                      "HJE"s,  //  Jesenik
                                                                      "HKA"s,  //  Karvina
                                                                      "HNJ"s,  //  Novy Jicin
                                                                      "HOL"s,  //  Olomouc
                                                                      "HOP"s,  //  Opava
                                                                      "HOS"s,  //  Ostrava
                                                                      "HPR"s,  //  Prerov
                                                                      "HSU"s,  //  Sumperk
                                                                      "HVS"s   //  Vsetin
                                                                  };

/// Slovakia
enum PRIMARY_ENUM_SLOVAKIA { SLOVAKIA_BAA,  //  Bratislava 1
                             SLOVAKIA_BAB,  //  Bratislava 2
                             SLOVAKIA_BAC,  //  Bratislava 3
                             SLOVAKIA_BAD,  //  Bratislava 4
                             SLOVAKIA_BAE,  //  Bratislava 5
                             SLOVAKIA_MAL,  //  Malacky
                             SLOVAKIA_PEZ,  //  Pezinok
                             SLOVAKIA_SEN,  //  Senec
                             SLOVAKIA_DST,  //  Dunajska Streda
                             SLOVAKIA_GAL,  //  Galanta
                             SLOVAKIA_HLO,  //  Hlohovec
                             SLOVAKIA_PIE,  //  Piestany
                             SLOVAKIA_SEA,  //  Senica
                             SLOVAKIA_SKA,  //  Skalica
                             SLOVAKIA_TRN,  //  Trnava
                             SLOVAKIA_BAN,  //  Banovce n. Bebr.
                             SLOVAKIA_ILA,  //  Ilava
                             SLOVAKIA_MYJ,  //  Myjava
                             SLOVAKIA_NMV,  //  Nove Mesto n. Vah
                             SLOVAKIA_PAR,  //  Partizanske
                             SLOVAKIA_PBY,  //  Povazska Bystrica
                             SLOVAKIA_PRI,  //  Prievidza
                             SLOVAKIA_PUC,  //  Puchov
                             SLOVAKIA_TNC,  //  Trencin
                             SLOVAKIA_KOM,  //  Komarno
                             SLOVAKIA_LVC,  //  Levice
                             SLOVAKIA_NIT,  //  Nitra
                             SLOVAKIA_NZA,  //  Nove Zamky
                             SLOVAKIA_SAL,  //  Sala
                             SLOVAKIA_TOP,  //  Topolcany
                             SLOVAKIA_ZMO,  //  Zlate Moravce
                             SLOVAKIA_BYT,  //  Bytca
                             SLOVAKIA_CAD,  //  Cadca
                             SLOVAKIA_DKU,  //  Dolny Kubin
                             SLOVAKIA_KNM,  //  Kysucke N. Mesto
                             SLOVAKIA_LMI,  //  Liptovsky Mikulas
                             SLOVAKIA_MAR,  //  Martin
                             SLOVAKIA_NAM,  //  Namestovo
                             SLOVAKIA_RUZ,  //  Ruzomberok
                             SLOVAKIA_TTE,  //  Turcianske Teplice
                             SLOVAKIA_TVR,  //  Tvrdosin
                             SLOVAKIA_ZIL,  //  Zilina
                             SLOVAKIA_BBY,  //  Banska Bystrica
                             SLOVAKIA_BST,  //  Banska Stiavnica
                             SLOVAKIA_BRE,  //  Brezno
                             SLOVAKIA_DET,  //  Detva
                             SLOVAKIA_KRU,  //  Krupina
                             SLOVAKIA_LUC,  //  Lucenec
                             SLOVAKIA_POL,  //  Poltar
                             SLOVAKIA_REV,  //  Revuca
                             SLOVAKIA_RSO,  //  Rimavska Sobota
                             SLOVAKIA_VKR,  //  Velky Krtis
                             SLOVAKIA_ZAR,  //  Zarnovica
                             SLOVAKIA_ZIH,  //  Ziar nad Hronom
                             SLOVAKIA_ZVO,  //  Zvolen
                             SLOVAKIA_GEL,  //  Gelnica
                             SLOVAKIA_KEA,  //  Kosice 1
                             SLOVAKIA_KEB,  //  Kosice 2
                             SLOVAKIA_KEC,  //  Kosice 3
                             SLOVAKIA_KED,  //  Kosice 4
                             SLOVAKIA_KEO,  //  Kosice-okolie
                             SLOVAKIA_MIC,  //  Michalovce
                             SLOVAKIA_ROZ,  //  Roznava
                             SLOVAKIA_SOB,  //  Sobrance
                             SLOVAKIA_SNV,  //  Spisska Nova Ves
                             SLOVAKIA_TRE,  //  Trebisov
                             SLOVAKIA_BAR,  //  Bardejov
                             SLOVAKIA_HUM,  //  Humenne
                             SLOVAKIA_KEZ,  //  Kezmarok
                             SLOVAKIA_LEV,  //  Levoca
                             SLOVAKIA_MED,  //  Medzilaborce
                             SLOVAKIA_POP,  //  Poprad
                             SLOVAKIA_PRE,  //  Presov
                             SLOVAKIA_SAB,  //  Sabinov
                             SLOVAKIA_SNI,  //  Snina
                             SLOVAKIA_SLU,  //  Stara Lubovna
                             SLOVAKIA_STR,  //  Stropkov
                             SLOVAKIA_SVI,  //  Svidnik
                             SLOVAKIA_VRT,  //  Vranov nad Toplou
                             N_SLOVAKIA_PRIMARIES
                           };

//typedef std::array<std::string, N_SLOVAKIA_PRIMARIES> PRIMARY_SLOVAKIA_ENUMERATION_TYPE;    ///< primaries for Slovakia

using PRIMARY_SLOVAKIA_ENUMERATION_TYPE = std::array<std::string, N_SLOVAKIA_PRIMARIES>;    ///< primaries for Slovakia

static PRIMARY_SLOVAKIA_ENUMERATION_TYPE PRIMARY_SLOVAKIA_ENUMERATION = { "BAA"s,  //  Bratislava 1
                                                                            "BAB"s,  //  Bratislava 2
                                                                            "BAC"s,  //  Bratislava 3
                                                                            "BAD"s,  //  Bratislava 4
                                                                            "BAE"s,  //  Bratislava 5
                                                                            "MAL"s,  //  Malacky
                                                                            "PEZ"s,  //  Pezinok
                                                                            "SEN"s,  //  Senec
                                                                            "DST"s,  //  Dunajska Streda
                                                                            "GAL"s,  //  Galanta
                                                                            "HLO"s,  //  Hlohovec
                                                                            "PIE"s,  //  Piestany
                                                                            "SEA"s,  //  Senica
                                                                            "SKA"s,  //  Skalica
                                                                            "TRN"s,  //  Trnava
                                                                            "BAN"s,  //  Banovce n. Bebr.
                                                                            "ILA"s,  //  Ilava
                                                                            "MYJ"s,  //  Myjava
                                                                            "NMV"s,  //  Nove Mesto n. Vah
                                                                            "PAR"s,  //  Partizanske
                                                                            "PBY"s,  //  Povazska Bystrica
                                                                            "PRI"s,  //  Prievidza
                                                                            "PUC"s,  //  Puchov
                                                                            "TNC"s,  //  Trencin
                                                                            "KOM"s,  //  Komarno
                                                                            "LVC"s,  //  Levice
                                                                            "NIT"s,  //  Nitra
                                                                            "NZA"s,  //  Nove Zamky
                                                                            "SAL"s,  //  Sala
                                                                            "TOP"s,  //  Topolcany
                                                                            "ZMO"s,  //  Zlate Moravce
                                                                            "BYT"s,  //  Bytca
                                                                            "CAD"s,  //  Cadca
                                                                            "DKU"s,  //  Dolny Kubin
                                                                            "KNM"s,  //  Kysucke N. Mesto
                                                                            "LMI"s,  //  Liptovsky Mikulas
                                                                            "MAR"s,  //  Martin
                                                                            "NAM"s,  //  Namestovo
                                                                            "RUZ"s,  //  Ruzomberok
                                                                            "TTE"s,  //  Turcianske Teplice
                                                                            "TVR"s,  //  Tvrdosin
                                                                            "ZIL"s,  //  Zilina
                                                                            "BBY"s,  //  Banska Bystrica
                                                                            "BST"s,  //  Banska Stiavnica
                                                                            "BRE"s,  //  Brezno
                                                                            "DET"s,  //  Detva
                                                                            "KRU"s,  //  Krupina
                                                                            "LUC"s,  //  Lucenec
                                                                            "POL"s,  //  Poltar
                                                                            "REV"s,  //  Revuca
                                                                            "RSO"s,  //  Rimavska Sobota
                                                                            "VKR"s,  //  Velky Krtis
                                                                            "ZAR"s,  //  Zarnovica
                                                                            "ZIH"s,  //  Ziar nad Hronom
                                                                            "ZVO"s,  //  Zvolen
                                                                            "GEL"s,  //  Gelnica
                                                                            "KEA"s,  //  Kosice 1
                                                                            "KEB"s,  //  Kosice 2
                                                                            "KEC"s,  //  Kosice 3
                                                                            "KED"s,  //  Kosice 4
                                                                            "KEO"s,  //  Kosice-okolie
                                                                            "MIC"s,  //  Michalovce
                                                                            "ROZ"s,  //  Roznava
                                                                            "SOB"s,  //  Sobrance
                                                                            "SNV"s,  //  Spisska Nova Ves
                                                                            "TRE"s,  //  Trebisov
                                                                            "BAR"s,  //  Bardejov
                                                                            "HUM"s,  //  Humenne
                                                                            "KEZ"s,  //  Kezmarok
                                                                            "LEV"s,  //  Levoca
                                                                            "MED"s,  //  Medzilaborce
                                                                            "POP"s,  //  Poprad
                                                                            "PRE"s,  //  Presov
                                                                            "SAB"s,  //  Sabinov
                                                                            "SNI"s,  //  Snina
                                                                            "SLU"s,  //  Stara Lubovna
                                                                            "STR"s,  //  Stropkov
                                                                            "SVI",  //  Svidnik
                                                                            "VRTs"   //  Vranov nad Toplou
                                                                        };

// ---------------------------------------------------  adif_country  -----------------------------------------

/*! \class  adif_country
    \brief  Encapsulate ADIF country
*/

class adif_country
{
protected:

  unsigned int  _code;                  ///< ID code for the country
  std::string   _name;                  ///< country name
  bool          _deleted;               ///< whether the country is deleted

  std::string   _canonical_prefix;      ///< canonical prefix; taked from cty.dat

  static unsigned int next_code;        ///< next free code

public:

/*! \brief          Constructor
    \param  nm      country name
    \param  pfx     canonical prefix
    \param  del     whether country has been deleted
*/
  adif_country(const std::string& nm, const std::string& pfx = ""s, bool del = false);
};

// ---------------------------------------------------  adif_countries  -----------------------------------------

/*! \class  adif_countries
    \brief  All ADIF countries
*/

class adif_countries
{
protected:

  std::vector<adif_country> _countries;     ///< container of countries

/*! \brief              Add a country at a particular index number
    \param  nm          country name
    \param  index       index at which the country is to be added
    \param  pfx         canonical prefix
    \param  deleted     whether country has been deleted
*/
  void _add_country(const std::string& nm, const unsigned int index, const std::string& pfx = ""s, const bool deleted = false);

/*! \brief              Add a deleted country at a particular index number
    \param  nm          country name
    \param  index       index at which the country is to be added
*/
  inline void _add_deleted_country(const std::string& nm, const unsigned int index)
    { _add_country(nm, index, std::string(), true); }

public:

/// default constructor
  adif_countries(void);
};

// ---------------------------------------------------  adif_type  -----------------------------------------

/*! \class adif_type
    \brief Base class for all the ADIF types
*/

class adif_type
{
protected:

  std::string _name;                        ///< name of the type
  char        _type_indicator;              ///< letter that identifies the types
  std::string _value;                       ///< value of the type

public:

/*! \brief      Constructor
    \param  ty  letter used to identify the type
    \param  nm  name of the instance of the type
    \param  v   value of the instance of the type
*/
  inline adif_type(const char ty, const std::string& nm = ""s, const std::string& v = ""s) :
    _name(nm),
    _type_indicator(ty),
    _value(v)
    { }

  READ_AND_WRITE(name);                        ///< name of the type
  READ_AND_WRITE(type_indicator);              ///< letter that identifies the types
  READ_AND_WRITE(value);                       ///< value of the type

/// convert to printable string
  inline const std::string to_string(void) const
    { return ( (_name.empty() or _value.empty()) ? std::string() : ( "<"s + _name + ":"s + ::to_string(_value.length()) + ">"s + _value ) ); }
};

// it's a pity that there is no way I can think of to create classes like this using templates instead of macros

/// macro to create some simple ADIF types; x = class name; y = 'char'
#define ADIF_CLASS(x, y) \
class x : public adif_type \
{ \
protected: \
\
public: \
\
/*! \brief default constructor \
*/ \
  inline x(void): \
    adif_type(y) \
    { } \
\
/*! \brief      Constructor \
    \param  nm  name \
    \param  v   value \
*/ \
  inline x(const std::string& nm, const std::string& val): \
            adif_type(y, nm, val) \
    { } \
\
/*! \brief      Constructor \
    \param  nm  name \
\
    Sets <i>_value</i> to the empty string. \
*/\
  explicit x(const std::string& nm) : \
  adif_type(y, nm, std::string()) \
  { }\
}

// ---------------------------------------------------  adif_AWARD_LIST -----------------------------------------

ADIF_CLASS(adif_AWARD_LIST, 'A');   ///< Encapsulate ADIF AwardList

// ---------------------------------------------------  adif_BOOLEAN -----------------------------------------

ADIF_CLASS(adif_BOOLEAN, 'B');      ///< Encapsulate ADIF Boolean

// ---------------------------------------------------  adif_DATE -----------------------------------------

/*! \class  adif_DATE
    \brief  Encapsulate ADIF Date
*/

class adif_DATE : public adif_type
{
protected:

public:

/// default constructor
  inline adif_DATE(void) :
    adif_type('D')
    { }

/*! \brief      Constructor
    \param  nm  name
    \param  v   value
*/
  inline adif_DATE(const std::string& nm, const std::string& v) :
    adif_type('D', nm, v)
    { }

/// construct with name
  explicit inline adif_DATE(const std::string& nm) :
    adif_type('D', nm, std::string())
    { }

/// set value
  void value(const std::string& v);

/// get value
  inline const std::string value(void) const
    { return adif_type::value(); }
};

// ---------------------------------------------------  adif_ENUMERATION -----------------------------------------

ADIF_CLASS(adif_ENUMERATION, ' ');   ///< Encapsulate ADIF Enumeration

// ---------------------------------------------------  adif_ENUM -----------------------------------------

/*! \class  adif_ENUM
    \brief  Encapsulate ADIF Enumeration with explicit types, names and values
*/

template <class T>
class adif_ENUM : public adif_type
{
protected:

  T _legal_values;          ///< legal values for the type

public:

/*! \brief          Construct from legal values
    \param  vals    container of legal values
*/
  adif_ENUM(const T& vals)  :
    adif_type(' '),
    _legal_values(vals)
    { }

/*! \brief          Construct with name, legal values and initial value
    \param  nm      name
    \param  vals    container of legal values
    \param  v       initial value
*/
  adif_ENUM(const std::string& nm, const T& vals, const std::string& v) :
    adif_type(' ', nm, v),
    _legal_values(vals)
    { }

/*! \brief          Construct with name and legal values
    \param  nm      name
    \param  vals    container of legal values
*/
  adif_ENUM(const std::string& nm, const T& vals)  :
        adif_type(' ', nm, std::string()),
        _legal_values(vals)
    { }

/*! \brief      Set the value
    \param  N   index into the legal values
*/
  inline void value(const unsigned int N)
    { _value = _legal_values[N]; }

/// get the value
  inline const std::string value(void) const
    { return adif_type::value(); }
};

// ---------------------------------------------------  adif_LOCATION -----------------------------------------

ADIF_CLASS(adif_LOCATION, 'L');  ///< Encapsulate ADIF Location

// ---------------------------------------------------  adif_MULTILINE_STRING -----------------------------------------

ADIF_CLASS(adif_MULTILINE_STRING, 'M');  ///< Encapsulate ADIF MultilineString; defined as: "a sequence of Characters and line-breaks, where a line break is an ASCII CR (code 13) followed immediately by an ASCII LF (code 10)"

// ---------------------------------------------------  adif_NUMBER -----------------------------------------

ADIF_CLASS(adif_NUMBER, 'N');    ///< Encapsulate ADIF Number; defined as: "a sequence of Digits optionally preceded by a minus sign (ASCII code 45) and optionally including a single decimal point (ASCII code 46)"

// ---------------------------------------------------  adif_STRING -----------------------------------------

/*! \class  adif_STRING
    \brief  Encapsulate ADIF String

    defined as: "a sequence of Characters"
*/

class adif_STRING : public adif_type
{
protected:

public:

/// default constructor
  adif_STRING(void);

/// construct with name and value
  adif_STRING(const std::string& nm, const std::string& val);

/// construct with name
  explicit adif_STRING(const std::string& nm);

/// set value
  void value(const std::string& v);

/// get value
  inline const std::string value(void) const
    { return adif_type::value(); }
};

// ---------------------------------------------------  adif_TIME -----------------------------------------

/*! \class  adif_TIME
    \brief  Encapsulate ADIF Time
*/

class adif_TIME : public adif_type
{
protected:

public:

/// default constructor
  adif_TIME(void);

/// construct with name and value
  adif_TIME(const std::string& nm, const std::string& val);

/// construct with name
  explicit adif_TIME(const std::string& nm);

/// set value
  void value(const std::string& v);

/// get value
  inline const std::string value(void) const
      { return adif_type::value(); }
};

// ---------------------------------------------------  adif_record-----------------------------------------

/*! \class  adif_record
    \brief  a single ADIF record
*/

class adif_record
{
protected:

/*  http://www.adif.org/adif227.htm

  ADDRESS           MultilineString                                                     the contacted station's mailing address
  ADIF_VER          String                                              yes             identifies the version of the ADIF used in this file
  AGE               Number                                                              the contacted station's operator's age in years
  A_INDEX           Number                                                              the geomagnetic A index at the time of the QSO
  ANT_AZ            Number                                                               the logging station's antenna azimuth, in degrees
  ANT_EL            Number                                                              the logging station's antenna elevation, in degrees
  ANT_PATH          Enumeration         Ant Path                                        the signal path
  ARRL_SECT         Enumeration         ARRL Section                                    the contacted station's ARRL section

  BAND              Enumeration         Band                                            QSO Band
  BAND_RX           Enumeration         Band                                            in a split frequency QSO, the logging station's receiving band

  CALL              String                                                              the contacted station's Callsign
  CHECK             String                                                              contest check (e.g. for ARRL Sweepstakes)
  CLASS             String                                                              contest class (e.g. for ARRL Field Day)
  CNTY              Enumeration         function of STATE                               the contacted station's Secondary Administrative Subdivision of contacted station (e.g. US county, JA Gun), in the specified format
  COMMENT           String                                                              comment field for QSO
  CONT              Enumeration         {NA, SA, EU , AF, OC, AS, AN}                   the contacted station's Continent
  CONTACTED_OP      String                                                              the callsign of the individual operating the contacted station
  CONTEST_ID        String              Contest ID                                      QSO Contest Identifier -- use enumeration values for interoperability
  COUNTRY           String                                                              the contacted station's DXCC entity name
  CQZ               Number                                                              the contacted station's CQ Zone
  CREDIT_SUBMITTED  AwardList           Award                                           the list of awards for which credit has been submitted
  CREDIT_GRANTED    AwardList           Award                                           the list of awards for which credit has been granted

  DISTANCE          Number                                                              the distance between the logging station and the contacted station in kilometers
  DXCC              Enumeration         DXCC                                            the contacted station's Country Code

  EMAIL             String                                                              the contacted station's e-mail address
  EQ_CALL           String                                                              the contacted station's owner's callsign
  EQSL_QSLRDATE     Date                                                                date QSL received from eQSL.cc (only valid if EQSL_RCVD is Y, I, or V)
  EQSL_QSLSDATE     Date                                                                date QSL sent to eQSL.cc    (only valid if EQSL_SENT is Y, Q, or I)
  EQSL_QSL_RCVD     Enumeration     {Y, N, R, I, V}                                     eQSL.cc QSL received status: Y - yes (confirmed); N - no; R - requested; I - ignore or invalid; V - validated
  EQSL_QSL_SENT     Enumeration     {Y, N, R, Q, I}                                     eQSL.cc QSL sent status: Y - yes; N - no; R - requested; Q - queued; I - ignore or invalid

  FORCE_INIT        Boolean                                                             new EME "initial"
  FREQ              Number                                                              QSO frequency in Megahertz
  FREQ_RX           Number                                                              in a split frequency QSO, the logging station's receiving frequency in megahertz

  GRIDSQUARE        String                                                              the contacted station's Maidenhead Grid Square
  GUEST_OP          String                                                              deprecated: use OPERATOR instead

  IOTA              String                                                              the contacted station's IOTA designator, in format CC-XXX, where CC is the continent designator {NA, SA, EU , AF, OC, AS, AN}, XXX is the island designator, where 0 <= XXX ,<= 999 [use leading zeroes]
  IOTA_ISLAND_ID    String                                                              the contacted station's IOTA Island Identifier
  ITUZ              Number                                                              the contacted station's ITU zone

  K_INDEX           Number                                                              the geomagnetic K index at the time of the QSO

  LAT               Location                                                            the contacted station's latitude
  LON               Location                                                            the contacted station's longitude
  LOTW_QSLRDATE     Date                                                                date QSL received from ARRL Logbook of the World (only valid if LOTW_RCVD is Y, I, or V)
  LOTW_QSLSDATE     Date                                                                date QSL sent to ARRL Logbook of the World (only valid if LOTW_SENT is Y, Q, or I)
  LOTW_QSL_RCVD     Enumeration     {Y, N, R, I, V}                                     ARRL Logbook of the World QSL received status: Y - yes (confirmed); N - no; R - requested; I - ignore or invalid V - validated
  LOTW_QSL_SENT     Enumeration     {Y, N, R, Q, I}                                     ARRL Logbook of the World QSL sent status: Y - yes; N - no; R - requested; Q - queued; I - ignore or invalid

  MAX_BURSTS        Number                                                              maximum length of meteor scatter bursts heard by the logging station, in seconds
  MODE              Enumeration     Mode                                                QSO Mode
  MS_SHOWER         String                                                              For Meteor Scatter QSOs, the name of the meteor shower in progress
  MY_CITY           String                                                              the logging station's city
  MY_CNTY           Enumeration     function of MY_STATE                                the logging station's Secondary Administrative Subdivision  (e.g. US county, JA Gun) , in the specified format
  MY_COUNTRY        Enumeration     Country                                             the logging station's DXCC entity name
  MY_CQ_ZONE        Number                                                              the logging station's CQ Zone
  MY_GRIDSQUARE     String                                                              the logging station's Maidenhead Grid Square
  MY_IOTA           String                                                              the logging station's IOTA designator, in format CC-XXX, where CC is the continent designator {NA, SA, EU , AF, OC, AS, AN}, XXX is the island designator, where 0 <= XXX ,<= 999  [use leading zeroes]
  MY_IOTA_ISLAND_ID String                                                              the logging station's IOTA Island Identifier
  MY_ITU_ZONE       Number                                                              the logging station's ITU zone
  MY_LAT            Location                                                            the logging station's latitude
  MY_LON            Location                                                            the logging station's longitude
  MY_NAME           String                                                              the logging operator's name
  MY_POSTAL_CODE    String                                                              the logging station's postal code
  MY_RIG            String                                                              description of the logging station's equipment
  MY_SIG            String                                                              special interest activity or event
  MY_SIG_INFO       String                                                              special interest activity or event information
  MY_STATE          Enumeration     function of MY_COUNTRY                              the code for the logging station's Primary Administrative Subdivision (e.g. US State, JA Island, VE Province)
  MY_STREET         String                                                              the logging station's street

  NAME              String                                                              the contacted station's operator's name
  NOTES             MultilineString                                                     QSO notes
  NR_BURSTS         Number                                                              the number of meteor scatter bursts heard by the logging station
  NR_PINGS          Number                                                              the number of meteor scatter pings heard by the logging station

  OPERATOR          String                                                              the logging operator's callsign; if STATION_CALLSIGN is absent, OPERATOR shall be treated as both the logging station's callsign and the logging operator's callsign
  OWNER_CALLSIGN    String                                                              the callsign of the owner of the station used to log the contact (the callsign of the OPERATOR's host); if OWNER_CALLSIGN is absent, STATION_CALLSIGN shall be treated as both the logging station's callsign and the callsign of the owner of the station

  PFX               String                                                              the contacted station's WPX prefix
  PRECEDENCE        String                                                              contest precedence (e.g. for ARRL Sweepstakes)
  PROGRAMID         String                                              yes             identifies the name of the logger, converter, or utility that created or processed this ADIF file
  PROGRAMVERSION    String                                              yes             identifies the version of the logger, converter, or utility that created or processed this ADIF file
  PROP_MODE         Enumeration     Propagation                                         QSO propagation mode
  PUBLIC_KEY        String                                                              public encryption key

  QSLMSG            MultilineString                                                     QSL card message
  QSLRDATE          Date                                                                QSL received date (only valid if QSL_RCVD is Y, I, or V)
  QSLSDATE          Date                                                                QSL sent date (only valid if QSL_SENT is Y, Q, or I)
  QSL_RCVD          Enumeration     {Y, N, R, I, V}                                     QSL received status: Y - yes (confirmed); N - no; R - requested; I - ignore or invalid; V - verified
  QSL_RCVD_VIA      Enumeration     {B, D, E, M}                                        means by which the QSL was received by the logging station: B - bureau; D - direct; E - electronic; M - manager
  QSL_SENT          Enumeration     {Y, N, R, Q, I}                                     QSL sent status: Y - yes; N - no; R - requested; Q - queued; I - ignore or invalid
  QSL_SENT_VIA      Enumeration     {B, D, E, M}                                        means by which the QSL was sent by the logging station: B - bureau; D - direct; E - electronic; M - manager
  QSL_VIA           String                                                              the contacted station's QSL route
  QSO_COMPLETE      Enumeration     {Y, N, NIL, ?}                                      indicates whether the QSO was complete from the perspective of the logging station: Y - yes; N - no; NIL - not heard; ? - uncertain
  QSO_DATE          Date                                                                date on which the QSO started
  QSO_DATE_OFF      Date                                                                date on which the QSO ended
  QSO_RANDOM        Boolean                                                             indicates whether the QSO was random or scheduled
  QTH               String                                                              the contacted station's city

  RIG               MultilineString                                                     description of the contacted station's equipment
  RST_RCVD          String                                                              signal report from the contacted station
  RST_SENT          String                                                              signal report sent to the contacted station
  RX_PWR            Number                                                              the contacted station's transmitter power in watts

  SAT_MODE          String                                                              satellite mode
  SAT_NAME          String                                                              name of satellite
  SFI               Number                                                              the solar flux at the time of the QSO
  SIG               String                                                              the name of the contacted station's special activity or interest group
  SIG_INFO          String                                                              information associated with the contacted station's activity or interest group
  SRX               Number                                                              contest QSO received serial number
  SRX_STRING        String                                                              contest QSO received information -- use Cabrillo format to convey contest information for which ADIF fields are not specified; in the event of a conflict between information in a dedicated contest field and this field, information in the dedicated contest field shall prevail
  STATE             Enumeration     function of Country Code                            the code for the contacted station's Primary Administrative Subdivision (e.g. US State, JA Island, VE Province)
  STATION_CALLSIGN  String                                                              the logging station's callsign (the callsign used over the air); if STATION_CALLSIGN is absent, OPERATOR shall be treated as both the logging station's callsign and the logging operator's callsign
  STX               Number                                                              contest QSO transmitted serial number
  STX_STRING        String                                                              contest QSO transmitted information;  use Cabrillo format to convey contest information for which ADIF fields are not specified;  in the event of a conflict between information in a dedicated contest field and this field, information in the dedicated contest field shall prevail
  SWL               Boolean                                                             indicates that the QSO information pertains to an SWL report

  TEN_TEN           Number                                                              Ten-Ten number
  TIME_OFF          Time                                                                HHMM or HHMMSS in UTC; in the absence of <QSO_DATE_OFF>, the QSO duration is less than 24 hours
  TIME_ON           Time                                                                HHMM or HHMMSS in UTC
  TX_PWR            Number                                                              the logging station's power in watts

  USERDEFn          String                                                  yes         specifies the name of the nth user-defined field, where n is a positive integer,
                                                                                        e.g., <USERDEF1:8>QRP_ARCI
                                                                                        optionally specifies an enumeration or range,
                                                                                        e.g., <USERDEF2:19>SweaterSize,{S,M,L}
                                                                                        <USERDEF3:15>ShoeSize,{5:20}
                                                                                        Constraints:
                                                                                           * The name of a user-defined field may not be an ADIF field name; e.g., <USERDEF1:4>CALL is invalid.
                                                                                           * The name of a user-defined field may not contain a > or a : character; e.g., <USERDEF1:4>oh:4 is invalid
                                                                                           * The name of a user-defined field may not begin or end with a space character; e.g., <USERDEF1:5> shoe is invalid
                                                                                           * The first number in a range may not be greater than the second number; e.g., {3:1} is invalid.

  VE_PROV           String                                                              deprecated; use State instead
  WEB               String                                                              the contacted station's URL
*/

  adif_MULTILINE_STRING _address;               ///< the contacted station's mailing address
  adif_STRING           _adif_ver;              ///< identifies the version of the ADIF used in this file
  adif_NUMBER           _age;                   ///< the contacted station's operator's age in years
  adif_NUMBER           _a_index;               ///< the geomagnetic A index at the time of the QSO
  adif_NUMBER           _ant_az;                ///< the logging station's antenna azimuth, in degrees
  adif_NUMBER           _ant_el;                ///< the logging station's antenna elevation, in degrees
  adif_ENUMERATION      _ant_path;              ///< the signal path
  adif_ENUM<SECTION_ENUMERATION_TYPE> _arrl_sect;   ///< the contacted station's ARRL section

//  static ANT_PATH_ENUMERATION_TYPE ANT_PATH_ENUMERATION = { { "G" , "O", "S", "L" } };

//  adif_ENUM<ANT_PATH_ENUMERATION_TYPE> _test;

  adif_ENUM<BAND_ENUMERATION_TYPE> _band;       ///< QSO Band
  adif_ENUMERATION      _band_rx;               ///< in a split frequency QSO, the logging station's receiving band

  adif_STRING           _call;                  ///< the contacted station's Callsign
  adif_STRING           _check;                 ///< contest check (e.g., for ARRL Sweepstakes)
  adif_STRING           _class;                 ///< contest class (e.g., for ARRL Field Day)
  adif_ENUMERATION      _cnty;                  ///< the contacted station's Secondary Administrative Subdivision of contacted station
  adif_STRING           _comment;               ///< comment field for QSO
  adif_ENUMERATION      _cont;                  ///< the contacted station's Continent
  adif_STRING           _contacted_op;          ///< the callsign of the individual operating the contacted station
  adif_STRING           _contest_id;            ///< QSO Contest Identifier
  adif_STRING           _country;               ///< the contacted station's DXCC entity name
  adif_NUMBER           _cqz;                   ///< the contacted station's CQ Zone
  adif_AWARD_LIST       _credit_submitted;      ///< the list of awards for which credit has been submitted
  adif_AWARD_LIST       _credit_granted;        ///< the list of awards for which credit has been granted

  adif_NUMBER           _distance;              ///< the distance between the logging station and the contacted station in kilometers
  adif_ENUMERATION      _dxcc;                  ///< the contacted station's Country Code

  adif_STRING           _email;                 ///< the contacted station's e-mail address
  adif_STRING           _eq_call;               ///< the contacted station's owner's callsign
  adif_DATE             _eqsl_qslrdate;         ///< date QSL received from eQSL.cc
  adif_DATE             _eqsl_qslsdate;         ///< date QSL sent to eQSL.cc
  adif_ENUMERATION      _eqsl_qsl_rcvd;         ///< eQSL.cc QSL received status
  adif_ENUMERATION      _eqsl_qsl_sent;         ///< eQSL.cc QSL sent status

  adif_BOOLEAN          _force_init;            ///< new EME initial
  adif_NUMBER           _freq;                  ///< QSO frequency in megahertz
  adif_NUMBER           _freq_rx;               ///< in a split frequency QSO, the logging station's receiving frequency in megahertz

  adif_STRING           _gridsquare;            ///< the contacted station's Maidenhead grid square

  adif_STRING           _iota;                  ///< the contacted station's IOTA designator, in format CC-XXX
  adif_STRING           _iota_island_id;        ///< the contacted station's IOTA Island Identifier
  adif_NUMBER           _ituz;                  ///< the contacted station's ITU zone

  adif_NUMBER           _k_index;               ///< the geomagnetic K index at the time of the QSO

  adif_LOCATION         _lat;                   ///< the contacted station's latitude
  adif_LOCATION         _lon;                   ///< the contacted station's longitude
  adif_DATE             _lotw_qslrdate;         ///< date QSL received from ARRL soi-disant Logbook of the World
  adif_DATE             _lotw_qslsdate;         ///< date QSL sent to ARRL soi-disant Logbook of the World
  adif_ENUMERATION      _lotw_qsl_rcvd;         ///< ARRL soi-disant Logbook of the World QSL received status
  adif_ENUMERATION      _lotw_qsl_sent;         ///< ARRL soi-disant Logbook of the World QSL sent status

  adif_NUMBER           _max_bursts;            ///< maximum length of meteor scatter bursts heard by the logging station, in seconds
  adif_ENUM<MODE_ENUMERATION_TYPE> _mode;       ///< QSO Mode
  adif_STRING           _ms_shower;             ///< For meteor scatter QSOs, the name of the meteor shower in progress
  adif_STRING           _my_city;               ///< the logging station's city/town/village/hamlet
  adif_ENUMERATION      _my_cnty;               ///< the logging station's Secondary Administrative Subdivision
  adif_ENUMERATION      _my_country;            ///< the logging station's DXCC entity name
  adif_NUMBER           _my_cq_zone;            ///< the logging station's CQ zone
  adif_STRING           _my_gridsquare;         ///< the logging station's Maidenhead grid qquare
  adif_STRING           _my_iota;               ///< the logging station's IOTA designator
  adif_STRING           _my_iota_island_id;     ///< the logging station's IOTA Island Identifier
  adif_NUMBER           _my_itu_zone;           ///< the logging station's ITU zone
  adif_LOCATION         _my_lat;                ///< the logging station's latitude
  adif_LOCATION         _my_lon;                ///< the logging station's longitude
  adif_STRING           _my_name;               ///< the logging operator's name
  adif_STRING           _my_postal_code;        ///< the logging station's postal code
  adif_STRING           _my_rig;                ///< description of the logging station's equipment
  adif_STRING           _my_sig;                ///< special interest activity or event
  adif_STRING           _my_sig_info;           ///< special interest activity or event information
  adif_ENUMERATION      _my_state;              ///< the code for the logging station's Primary Administrative Subdivision
  adif_STRING           _my_street;             ///< the logging station's street

  adif_STRING           _name;                  ///< the contacted station's operator's name
  adif_MULTILINE_STRING _notes;                 ///< QSO notes
  adif_NUMBER           _nr_bursts;             ///< the number of meteor scatter bursts heard by the logging station
  adif_NUMBER           _nr_pings;              ///< the number of meteor scatter pings heard by the logging station

  adif_STRING           _operator;              ///< the logging operator's callsign
  adif_STRING           _owner_callsign;        ///< the callsign of the owner of the station used to log the contact

  adif_STRING           _pfx;                   ///< the contacted station's WPX prefix
  adif_STRING           _precedence;            ///< contest precedence (e.g. for ARRL Sweepstakes)
  adif_STRING           _programid;             ///< identifies the name of the logger, converter, or utility that created or processed this ADIF file
  adif_STRING           _programversion;        ///< identifies the version of the logger, converter, or utility that created or processed this ADIF file
  adif_ENUM<PROPAGATION_MODE_ENUMERATION_TYPE> _prop_mode;  ///< QSO propagation mode
  adif_STRING           _public_key;            ///< public encryption key

  adif_MULTILINE_STRING _qslmsg;                ///< QSL card message
  adif_DATE             _qslrdate;              ///< QSL received date
  adif_DATE             _qslsdate;              ///< QSL sent date
  adif_ENUMERATION      _qsl_rcvd;              ///< QSL received status
  adif_ENUMERATION      _qsl_rcvd_via;          ///< means by which the QSL was received by the logging station
  adif_ENUMERATION      _qsl_sent;              ///< QSL sent status
  adif_ENUMERATION      _qsl_sent_via;          ///< means by which the QSL was sent by the logging station
  adif_STRING           _qsl_via;               ///< the contacted station's QSL route
  adif_ENUMERATION      _qso_complete;          ///< indicates whether the QSO was complete from the perspective of the logging station
  adif_DATE             _qso_date;              ///< date on which the QSO started
  adif_DATE             _qso_date_off;          ///< date on which the QSO ended
  adif_BOOLEAN          _qso_random;            ///< indicates whether the QSO was random or scheduled
  adif_STRING           _qth;                   ///< the contacted station's conurbation

  adif_MULTILINE_STRING _rig;                   ///< description of the contacted station's equipment
  adif_STRING           _rst_rcvd;              ///< signal report from the contacted station
  adif_STRING           _rst_sent;              ///< signal report sent to the contacted station
  adif_NUMBER           _rx_pwr;                ///< the contacted station's transmitter power in watts (bizarre! since "rx" means "receiver")

  adif_STRING           _sat_mode;              ///< satellite mode
  adif_STRING           _sat_name;              ///< name of satellite
  adif_NUMBER           _sfi;                   ///< the solar flux at the time of the QSO [sic]
  adif_STRING           _sig;                   ///< the name of the contacted station's special activity or interest group
  adif_STRING           _sig_info;              ///< information associated with the contacted station's activity or interest group
  adif_NUMBER           _srx;                   ///< contest QSO received serial number
  adif_STRING           _srx_string;            ///< contest QSO received information
  adif_ENUMERATION      _state;                 ///< the code for the contacted station's Primary Administrative Subdivision
  adif_STRING           _station_callsign;      ///< the logging station's callsign
  adif_NUMBER           _stx;                   ///< contest QSO transmitted serial number
  adif_STRING           _stx_string;            ///< contest QSO transmitted information
  adif_BOOLEAN          _swl;                   ///< indicates whether the QSO information pertains to an SWL report

  adif_NUMBER           _ten_ten;               ///< Ten-Ten number
  adif_TIME             _time_off;              ///< HHMM or HHMMSS in UTC
  adif_TIME             _time_on;               ///< HHMM or HHMMSS in UTC
  adif_NUMBER           _tx_pwr;                ///< the logging station's power in watts

  std::vector<adif_STRING>  _USERDEF;           ///< the names of user-defined fields

  adif_STRING           _web;                   ///< the contacted station's URL [sic]

  unsigned int          _linefeeds_after_field; ///< number of linefeeds to insert after each field (typically 0 or 1)
  unsigned int          _linefeeds_after_record;///< number of *additional* linefeeds to insert after the record (typically 0, 1 or 2)

public:

/// constructor
  adif_record(void);

// Read/Write access to the members
  READ_AND_WRITE(address);               ///< the contacted station's mailing address
  READ_AND_WRITE(adif_ver);              ///< identifies the version of the ADIF used in this file
  READ_AND_WRITE(age);                   ///< the contacted station's operator's age in years
  READ_AND_WRITE(a_index);               ///< the geomagnetic A index at the time of the QSO
  READ_AND_WRITE(ant_az);                ///< the logging station's antenna azimuth, in degrees
  READ_AND_WRITE(ant_el);                ///< the logging station's antenna elevation, in degrees
  READ_AND_WRITE(ant_path);              ///< the signal path
//  RW(adif_ENUMERATION,      arrl_sect);             ///< the contacted station's ARRL section
  READ_AND_WRITE(arrl_sect);             ///< the contacted station's ARRL section

//  RW(adif_ENUMERATION,      band);                  ///< QSO Band
  READ_AND_WRITE(band);       ///< QSO band
  READ_AND_WRITE(band_rx);               ///< in a split frequency QSO, the logging station's receiving band

  READ_AND_WRITE(call);                  ///< the contacted station's callsign
  READ_AND_WRITE(check);                 ///< contest check (e.g., for ARRL Sweepstakes)
//  RW(adif_STRING,           class);                 ///< contest class (e.g., for ARRL Field Day)  -- see below
  READ_AND_WRITE(cnty);                  ///< the contacted station's Secondary Administrative Subdivision of contacted station
  READ_AND_WRITE(comment);               ///< comment field for QSO
  READ_AND_WRITE(cont);                  ///< the contacted station's Continent
  READ_AND_WRITE(contacted_op);          ///< the callsign of the individual operating the contacted station
  READ_AND_WRITE(contest_id);            ///< QSO Contest Identifier
  READ_AND_WRITE(country);               ///< the contacted station's DXCC entity name
  READ_AND_WRITE(cqz);                   ///< the contacted station's CQ Zone
  READ_AND_WRITE(credit_submitted);      ///< the list of awards for which credit has been submitted
  READ_AND_WRITE(credit_granted);        ///< the list of awards for which credit has been granted

  READ_AND_WRITE(distance);              ///< the distance between the logging station and the contacted station in kilometers
  READ_AND_WRITE(dxcc);                  ///< the contacted station's Country Code

  READ_AND_WRITE(email);                 ///< the contacted station's e-mail address
  READ_AND_WRITE(eq_call);               ///< the contacted station's owner's callsign
  READ_AND_WRITE(eqsl_qslrdate);         ///< date QSL received from eQSL.cc
  READ_AND_WRITE(eqsl_qslsdate);         ///< date QSL sent to eQSL.cc
  READ_AND_WRITE(eqsl_qsl_rcvd);         ///< eQSL.cc QSL received status
  READ_AND_WRITE(eqsl_qsl_sent);         ///< eQSL.cc QSL sent status

  READ_AND_WRITE(force_init);            ///< new EME initial
  READ_AND_WRITE(freq);                  ///< QSO frequency in megahertz

/*! \brief      Set the frequency (in MHz) from a string
    \param  v   string representing teh frequency in MHz
*/
  inline void freq(const std::string& v)
    { _freq.value(v); }

  READ_AND_WRITE(freq_rx);               ///< in a split frequency QSO, the logging station's receiving frequency in megahertz

  READ_AND_WRITE(gridsquare);            ///< the contacted station's Maidenhead grid square

  READ_AND_WRITE(iota);                  ///< the contacted station's IOTA designator, in format CC-XXX
  READ_AND_WRITE(iota_island_id);        ///< the contacted station's IOTA Island Identifier
  READ_AND_WRITE(ituz);                  ///< the contacted station's ITU zone

  READ_AND_WRITE(k_index);               ///< the geomagnetic K index at the time of the QSO

  READ_AND_WRITE(lat);                   ///< the contacted station's latitude
  READ_AND_WRITE(lon);                   ///< the contacted station's longitude
  READ_AND_WRITE(lotw_qslrdate);         ///< date QSL received from ARRL soi-disant Logbook of the World
  READ_AND_WRITE(lotw_qslsdate);         ///< date QSL sent to ARRL soi-disant Logbook of the World
  READ_AND_WRITE(lotw_qsl_rcvd);         ///< ARRL soi-disant Logbook of the World QSL received status
  READ_AND_WRITE(lotw_qsl_sent);         ///< ARRL soi-disant Logbook of the World QSL sent status

  READ_AND_WRITE(max_bursts);            ///< maximum length of meteor scatter bursts heard by the logging station, in seconds
//  RW(adif_ENUMERATION,      mode);                  ///< QSO Mode
  READ_AND_WRITE(mode);       ///< QSO mode
  READ_AND_WRITE(ms_shower);             ///< For meteor scatter QSOs, the name of the meteor shower in progress
  READ_AND_WRITE(my_city);               ///< the logging station's city/town/village/hamlet
  READ_AND_WRITE(my_cnty);               ///< the logging station's Secondary Administrative Subdivision
  READ_AND_WRITE(my_country);            ///< the logging station's DXCC entity name
  READ_AND_WRITE(my_cq_zone);            ///< the logging station's CQ zone
  READ_AND_WRITE(my_gridsquare);         ///< the logging station's Maidenhead grid qquare
  READ_AND_WRITE(my_iota);               ///< the logging station's IOTA designator
  READ_AND_WRITE(my_iota_island_id);     ///< the logging station's IOTA Island Identifier
  READ_AND_WRITE(my_itu_zone);           ///< the logging station's ITU zone
  READ_AND_WRITE(my_lat);                ///< the logging station's latitude
  READ_AND_WRITE(my_lon);                ///< the logging station's longitude
  READ_AND_WRITE(my_name);               ///< the logging operator's name
  READ_AND_WRITE(my_postal_code);        ///< the logging station's postal code
  READ_AND_WRITE(my_rig);                ///< description of the logging station's equipment
  READ_AND_WRITE(my_sig);                ///< special interest activity or event
  READ_AND_WRITE(my_sig_info);           ///< special interest activity or event information
  READ_AND_WRITE(my_state);              ///< the code for the logging station's Primary Administrative Subdivision
  READ_AND_WRITE(my_street);             ///< the logging station's street

  READ_AND_WRITE(name);                  ///< the contacted station's operator's name
  READ_AND_WRITE(notes);                 ///< QSO notes
  READ_AND_WRITE(nr_bursts);             ///< the number of meteor scatter bursts heard by the logging station
  READ_AND_WRITE(nr_pings);              ///< the number of meteor scatter pings heard by the logging station

//  RW(adif_STRING,           operator);              ///< the logging operator's callsign
  READ_AND_WRITE(owner_callsign);        ///< the callsign of the owner of the station used to log the contact

  READ_AND_WRITE(pfx);                   ///< the contacted station's WPX prefix
  READ_AND_WRITE(precedence);            ///< contest precedence (e.g. for ARRL Sweepstakes)
  READ_AND_WRITE(programid);             ///< identifies the name of the logger, converter, or utility that created or processed this ADIF file
  READ_AND_WRITE(programversion);        ///< identifies the version of the logger, converter, or utility that created or processed this ADIF file
//  RW(adif_ENUMERATION,      prop_mode);             ///< QSO propagation mode
  READ_AND_WRITE(prop_mode);  ///< QSO propagation mode
  READ_AND_WRITE(public_key);            ///< public encryption key

  READ_AND_WRITE(qslmsg);                ///< QSL card message
  READ_AND_WRITE(qslrdate);              ///< QSL received date
  READ_AND_WRITE(qslsdate);              ///< QSL sent date
  READ_AND_WRITE(qsl_rcvd);              ///< QSL received status
  READ_AND_WRITE(qsl_rcvd_via);          ///< means by which the QSL was received by the logging station
  READ_AND_WRITE(qsl_sent);              ///< QSL sent status
  READ_AND_WRITE(qsl_sent_via);          ///< means by which the QSL was sent by the logging station
  READ_AND_WRITE(qsl_via);               ///< the contacted station's QSL route
  READ_AND_WRITE(qso_complete);          ///< indicates whether the QSO was complete from the perspective of the logging station
  READ_AND_WRITE(qso_date);              ///< date on which the QSO started
  READ_AND_WRITE(qso_date_off);          ///< date on which the QSO ended
  READ_AND_WRITE(qso_random);            ///< indicates whether the QSO was random or scheduled
  READ_AND_WRITE(qth);                   ///< the contacted station's conurbation

  READ_AND_WRITE(rig);                   ///< description of the contacted station's equipment
  READ_AND_WRITE(rst_rcvd);              ///< signal report from the contacted station
  READ_AND_WRITE(rst_sent);              ///< signal report sent to the contacted station
  READ_AND_WRITE(rx_pwr);                ///< the contacted station's transmitter power in watts (bizarre! since "rx" means "receiver")

  READ_AND_WRITE(sat_mode);              ///< satellite mode
  READ_AND_WRITE(sat_name);              ///< name of satellite
  READ_AND_WRITE(sfi);                   ///< the solar flux at the time of the QSO [sic]
  READ_AND_WRITE(sig);                   ///< the name of the contacted station's special activity or interest group
  READ_AND_WRITE(sig_info);              ///< information associated with the contacted station's activity or interest group
  READ_AND_WRITE(srx);                   ///< contest QSO received serial number
  READ_AND_WRITE(srx_string);            ///< contest QSO received information
  READ_AND_WRITE(state);                 ///< the code for the contacted station's Primary Administrative Subdivision
  READ_AND_WRITE(station_callsign);      ///< the logging station's callsign
  READ_AND_WRITE(stx);                   ///< contest QSO transmitted serial number
  READ_AND_WRITE(stx_string);            ///< contest QSO transmitted information
  READ_AND_WRITE(swl);                   ///< indicates whether the QSO information pertains to an SWL report

  READ_AND_WRITE(ten_ten);               ///< Ten-Ten number
  READ_AND_WRITE(time_off);              ///< HHMM or HHMMSS in UTC
  READ_AND_WRITE(time_on);               ///< HHMM or HHMMSS in UTC
  READ_AND_WRITE(tx_pwr);                ///< the logging station's power in watts

//  std::vector<adif_STRING>  _USERDEF;           ///< the names of user-defined fields

  READ_AND_WRITE(web);                   ///< the contacted station's URL [sic]

  READ_AND_WRITE(linefeeds_after_field); ///< number of linefeeds to insert after each field (typically 0 or 1)
  READ_AND_WRITE(linefeeds_after_record);///< number of *additional* linefeeds to insert after the record (typically 0, 1 or 2)

/// access to _class is different because "class" is a reserved word in C++
  inline const adif_STRING clss(void) const
    { return _class; }

/*! \brief      Define the class
    \param  n   the class
*/
  inline void clss(const adif_STRING& n)
    { _class = n; }

/// access to _operator is different because "operator" is a reserved word in C++
  inline const adif_STRING op(void) const
    { return _operator; }

/*! \brief      Define the operator
    \param  n   the operator
*/
  inline void op(const adif_STRING& n)
    { _operator = n; }

/// direct write access to member values using ordinary non-ADIF types
#define DIRECT_WRITE(x) \
  template <class T> \
  inline void x(const T& v) \
    { _##x.value(v); }

  DIRECT_WRITE(address);            ///< the contacted station's mailing address
  DIRECT_WRITE(arrl_sect);          ///< the contacted station's ARRL section
  DIRECT_WRITE(band);               ///< QSO band
  DIRECT_WRITE(call);               ///< the contacted station's callsign
  DIRECT_WRITE(comment);            ///< comment field for QSO
  DIRECT_WRITE(mode);               ///< QSO mode
  DIRECT_WRITE(notes);              ///< QSO notes
  DIRECT_WRITE(qsl_rcvd);           ///< QSL received status
  DIRECT_WRITE(qsl_via);            ///< the contacted station's QSL route
  DIRECT_WRITE(qso_date);           ///< date on which the QSO started
  DIRECT_WRITE(rst_sent);           ///< signal report sent to the contacted station
  DIRECT_WRITE(rst_rcvd);           ///< signal report from the contacted station
  DIRECT_WRITE(station_callsign);   ///< the logging station's callsign
  DIRECT_WRITE(time_on);            ///< HHMM or HHMMSS in UTC

/// convert record to the printable string format
  const std::string to_string(void) const;
};

/*! \brief              Extract the value from an ADIF line, ignoring the last <i>offeset</i> characters
    \param  this_line   line from an ADIF file
    \param  offset      number of characters to ignore at the end of the line
    \return             value extracted from <i>this_line</i>
*/
const std::string adif_value(const std::string& this_line, const unsigned int offset = 0);

#endif /* ADIF_H_ */
