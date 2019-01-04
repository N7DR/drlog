// $Id: adif.h 149 2019-01-03 19:24:01Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

#ifndef ADIF_H
#define ADIF_H

/*! \file   adif.h

    Objects and functions related to ADIF version 2.2.7 at http://www.adif.org/adif227.htm

*/

#include "macros.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

// enumerations

// ant path  -------------------------------------------------------

/// antenna path
enum  ANT_PATH_ENUM { ANT_PATH_GREYLINE,
                      ANT_PATH_OTHER,
                      ANT_PATH_SHORT_PATH,
                      ANT_PATH_LONG_PATH,
                      N_ANT_PATHS
                    };

typedef std::array<std::string, N_ANT_PATHS> ANT_PATH_ENUMERATION_TYPE;     ///< type for antenna path enumeration

/// legal values of ANT_PATH_ENUMERATION
static ANT_PATH_ENUMERATION_TYPE ANT_PATH_ENUMERATION { "G",    ///< greyline
                                                        "O",    ///< other
                                                        "S",    ///< short path
                                                        "L"     ///< long path
                                                      };

// mode  -------------------------------------------------------

/// modes
enum MODE_ENUM { ADIF_MODE_AM,              // 0
                 ADIF_MODE_AMTORFEC,
                 ADIF_MODE_ASCI,
                 ADIF_MODE_ATV,
                 ADIF_MODE_CHIP64,
                 ADIF_MODE_CHIP128,
                 ADIF_MODE_CLO,
                 ADIF_MODE_CONTESTI,
                 ADIF_MODE_CW,
                 ADIF_MODE_DSTAR,
                 ADIF_MODE_DOMINO,          // 10
                 ADIF_MODE_DOMINOF,
                 ADIF_MODE_FAX,
                 ADIF_MODE_FM,
                 ADIF_MODE_FMHELL,
                 ADIF_MODE_FSK31,
                 ADIF_MODE_FSK441,
                 ADIF_MODE_GTOR,
                 ADIF_MODE_HELL,
                 ADIF_MODE_HELL80,
                 ADIF_MODE_HFSK,            // 20
                 ADIF_MODE_JT44,
                 ADIF_MODE_JT4A,
                 ADIF_MODE_JT4B,
                 ADIF_MODE_JT4C,
                 ADIF_MODE_JT4D,
                 ADIF_MODE_JT4E,
                 ADIF_MODE_JT4F,
                 ADIF_MODE_JT4G,
                 ADIF_MODE_JT65,
                 ADIF_MODE_JT65A,           // 30
                 ADIF_MODE_JT65B,
                 ADIF_MODE_JT65C,
                 ADIF_MODE_JT6M,
                 ADIF_MODE_MFSK8,
                 ADIF_MODE_MFSK16,
                 ADIF_MODE_MT63,
                 ADIF_MODE_OLIVIA,
                 ADIF_MODE_PAC,
                 ADIF_MODE_PAC2,
                 ADIF_MODE_PAC3,            // 40
                 ADIF_MODE_PAX,
                 ADIF_MODE_PAX2,
                 ADIF_MODE_PCW,
                 ADIF_MODE_PKT,
                 ADIF_MODE_PSK10,
                 ADIF_MODE_PSK31,
                 ADIF_MODE_PSK63,
                 ADIF_MODE_PSK63F,
                 ADIF_MODE_PSK125,
                 ADIF_MODE_PSKAM10,         // 50
                 ADIF_MODE_PSKAM31,
                 ADIF_MODE_PSKAM50,
                 ADIF_MODE_PSKFEC31,
                 ADIF_MODE_PSKHELL,
                 ADIF_MODE_Q15,
                 ADIF_MODE_QPSK31,
                 ADIF_MODE_QPSK63,
                 ADIF_MODE_QPSK125,
                 ADIF_MODE_ROS,
                 ADIF_MODE_RTTY,            // 60
                 ADIF_MODE_RTTYM,
                 ADIF_MODE_SSB,
                 ADIF_MODE_SSTV,
                 ADIF_MODE_THRB,
                 ADIF_MODE_THOR,
                 ADIF_MODE_THRBX,
                 ADIF_MODE_TOR,
                 ADIF_MODE_VOI,
                 ADIF_MODE_WINMOR,
                 ADIF_MODE_WSPR,            // 70
                 N_ADIF_MODES
               };                                                       ///< enum for modes

typedef std::array<std::string, N_ADIF_MODES> MODE_ENUMERATION_TYPE;    ///< type for mode enumeration

static MODE_ENUMERATION_TYPE MODE_ENUMERATION { "AM",               // 0
                                                "AMTORFEC",
                                                "ASCI",
                                                "ATV",
                                                "CHIP64",
                                                "CHIP128",
                                                "CLO",
                                                "CONTESTI",
                                                "CW",
                                                "DSTAR",
                                                "DOMINO",            // 10
                                                "DOMINOF",
                                                "FAX",
                                                "FM",
                                                "FMHELL",
                                                "FSK31",
                                                "FSK441",
                                                "GTOR",
                                                "HELL",
                                                "HELL80",
                                                "HFSK",             // 20
                                                "JT44",
                                                "JT4A",
                                                "JT4B",
                                                "JT4C",
                                                "JT4D",
                                                "JT4E",
                                                "JT4F",
                                                "JT4G",
                                                "JT65",
                                                "JT65A",            // 30
                                                "JT65B",
                                                "JT65C",
                                                "JT6M",
                                                "MFSK8",
                                                "MFSK16",
                                                "MT63",
                                                "OLIVIA",
                                                "PAC",
                                                "PAC2",
                                                "PAC3",             // 40
                                                "PAX",
                                                "PAX2",
                                                "PCW",
                                                "PKT",
                                                "PSK10",
                                                "PSK31",
                                                "PSK63",
                                                "PSK63F",
                                                "PSK125",
                                                "PSKAM10",          // 50
                                                "PSKAM31",
                                                "PSKAM50",
                                                "PSKFEC31",
                                                "PSKHELL",
                                                "Q15",
                                                "QPSK31",
                                                "QPSK63",
                                                "QPSK125",
                                                "ROS",
                                                "RTTY",             // 60
                                                "RTTYM",
                                                "SSB",
                                                "SSTV",
                                                "THRB",
                                                "THOR",
                                                "THRBX",
                                                "TOR",
                                                "VOI",
                                                "WINMOR",
                                                "WSPR"              // 70
                                              };

// ARRL section  -------------------------------------------------------

/// sections
enum SECTION_ENUM { SECTION_AL,
                    SECTION_AK,
                    SECTION_AB,
                    SECTION_AR,
                    SECTION_AZ,
                    SECTION_BC,
                    SECTION_CO,
                    SECTION_CT,
                    SECTION_DE,
                    SECTION_EB,
                    SECTION_EMA,
                    SECTION_ENY,
                    SECTION_EPA,
                    SECTION_EWA,
                    SECTION_GA,
                    SECTION_ID,
                    SECTION_IL,
                    SECTION_IN,
                    SECTION_IA,
                    SECTION_KS,
                    SECTION_KY,
                    SECTION_LAX,
                    SECTION_LA,
                    SECTION_ME,
                    SECTION_MB,
                    SECTION_MAR,
                    SECTION_MDC,
                    SECTION_MI,
                    SECTION_MN,
                    SECTION_MS,
                    SECTION_MO,
                    SECTION_MT,
                    SECTION_NE,
                    SECTION_NV,
                    SECTION_NH,
                    SECTION_NM,
                    SECTION_NLI,
                    SECTION_NL,
                    SECTION_NC,
                    SECTION_ND,
                    SECTION_NTX,
                    SECTION_NFL,
                    SECTION_NNJ,
                    SECTION_NNY,
                    SECTION_NT,
                    SECTION_OH,
                    SECTION_OK,
                    SECTION_ON,
                    SECTION_ORG,
                    SECTION_OR,
                    SECTION_PAC,
                    SECTION_PR,
                    SECTION_QC,
                    SECTION_RI,
                    SECTION_SV,
                    SECTION_SDG,
                    SECTION_SF,
                    SECTION_SJV,
                    SECTION_SB,
                    SECTION_SCV,
                    SECTION_SK,
                    SECTION_SC,
                    SECTION_SD,
                    SECTION_STX,
                    SECTION_SFL,
                    SECTION_SNJ,
                    SECTION_TN,
                    SECTION_VI,
                    SECTION_UT,
                    SECTION_VT,
                    SECTION_VA,
                    SECTION_WCF,
                    SECTION_WTX,
                    SECTION_WV,
                    SECTION_WMA,
                    SECTION_WNY,
                    SECTION_WPA,
                    SECTION_WWA,
                    SECTION_WI,
                    SECTION_WY,
                    N_SECTIONS
                  };                                                        ///< enum for sections

typedef std::array<std::string, N_SECTIONS> SECTION_ENUMERATION_TYPE;       ///< type for section enumeration

static SECTION_ENUMERATION_TYPE SECTION_ENUMERATION { "AL",
                                                      "AK",
                                                      "AB",
                                                      "AR",
                                                      "AZ",
                                                      "BC",
                                                      "CO",
                                                      "CT",
                                                      "DE",
                                                      "EB",
                                                      "EMA",
                                                      "ENY",
                                                      "EPA",
                                                      "EWA",
                                                      "GA",
                                                      "ID",
                                                      "IL",
                                                      "IN",
                                                      "IA",
                                                      "KS",
                                                      "KY",
                                                      "LAX",
                                                      "LA",
                                                      "ME",
                                                      "MB",
                                                      "MAR",
                                                      "MDC",
                                                      "MI",
                                                      "MN",
                                                      "MS",
                                                      "MO",
                                                      "MT",
                                                      "NE",
                                                      "NV",
                                                      "NH",
                                                      "NM",
                                                      "NLI",
                                                      "NL",
                                                      "NC",
                                                      "ND",
                                                      "NTX",
                                                      "NFL",
                                                      "NNJ",
                                                      "NNY",
                                                      "NT",
                                                      "OH",
                                                      "OK",
                                                      "ON",
                                                      "ORG",
                                                      "OR",
                                                      "PAC",
                                                      "PR",
                                                      "QC",
                                                      "RI",
                                                      "SV",
                                                      "SDG",
                                                      "SF",
                                                      "SJV",
                                                      "SB",
                                                      "SCV",
                                                      "SK",
                                                      "SC",
                                                      "SD",
                                                      "STX",
                                                      "SFL",
                                                      "SNJ",
                                                      "TN",
                                                      "VI",
                                                      "UT",
                                                      "VT",
                                                      "VA",
                                                      "WCF",
                                                      "WTX",
                                                      "WV",
                                                      "WMA",
                                                      "WNY",
                                                      "WPA",
                                                      "WWA",
                                                      "WI",
                                                      "WY"
                                                    };

// awards  -------------------------------------------------------

/// awards
enum AWARD_ENUM { AWARD_AJA,
                  AWARD_CQDX,
                  AWARD_CQDXFIELD,
                  AWARD_CQWAZ_MIXED,
                  AWARD_CQWAZ_CW,
                  AWARD_CQWAZ_PHONE,
                  AWARD_CQWAZ_RTTY,
                  AWARD_CQWAZ_160m,
                  AWARD_CQWPX,
                  AWARD_DARC_DOK,
                  AWARD_DXCC,
                  AWARD_DXCC_MIXED,
                  AWARD_DXCC_CW,
                  AWARD_DXCC_PHONE,
                  AWARD_DXCC_RTTY,
                  AWARD_IOTA,
                  AWARD_JCC,
                  AWARD_JCG,
                  AWARD_MARATHON,
                  AWARD_RDA,
                  AWARD_WAB,
                  AWARD_WAC,
                  AWARD_WAE,
                  AWARD_WAIP,
                  AWARD_WAJA,
                  AWARD_WAS,
                  AWARD_WAZ,
                  AWARD_USACA,
                  AWARD_VUCC,
                  N_AWARDS
                };                                                      ///< enum for awards

typedef std::array<std::string, N_AWARDS> AWARD_ENUMERATION_TYPE;       ///< type for award enumeration

static AWARD_ENUMERATION_TYPE AWARD_ENUMERATION { "AJA",
                                                  "CQDX",
                                                  "CQDXFIELD",
                                                  "CQWAZ_MIXED",
                                                  "CQWAZ_CW",
                                                  "CQWAZ_PHONE",
                                                  "CQWAZ_RTTY",
                                                  "CQWAZ_160m",
                                                  "CQWPX",
                                                  "DARC_DOK",
                                                  "DXCC",
                                                  "DXCC_MIXED",
                                                  "DXCC_CW",
                                                  "DXCC_PHONE",
                                                  "DXCC_RTTY",
                                                  "IOTA",
                                                  "JCC",
                                                  "JCG",
                                                  "MARATHON",
                                                  "RDA",
                                                  "WAB",
                                                  "WAC",
                                                  "WAE",
                                                  "WAIP",
                                                  "WAJA",
                                                  "WAS",
                                                  "WAZ",
                                                  "USACA",
                                                  "VUCC"
                                                } ;

// band  -------------------------------------------------------

/// bands
enum BAND_ENUM { ADIF_BAND_2190m,
                 ADIF_BAND_560m,
                 ADIF_BAND_160m,
                 ADIF_BAND_80m,
                 ADIF_BAND_60m,
                 ADIF_BAND_40m,
                 ADIF_BAND_30m,
                 ADIF_BAND_20m,
                 ADIF_BAND_17m,
                 ADIF_BAND_15m,
                 ADIF_BAND_12m,
                 ADIF_BAND_10m,
                 ADIF_BAND_6m,
                 ADIF_BAND_4m,
                 ADIF_BAND_2m,
                 ADIF_BAND_1point25m,
                 ADIF_BAND_70cm,
                 ADIF_BAND_33cm,
                 ADIF_BAND_23cm,
                 ADIF_BAND_13cm,
                 ADIF_BAND_9cm,
                 ADIF_BAND_6cm,
                 ADIF_BAND_3cm,
                 ADIF_BAND_1point25cm,
                 ADIF_BAND_6mm,
                 ADIF_BAND_4mm,
                 ADIF_BAND_2point5mm,
                 ADIF_BAND_2mm,
                 ADIF_BAND_1mm,
                 N_ADIF_BANDS
               };                                                       ///< enum for bands

typedef std::array<std::string, N_ADIF_BANDS> BAND_ENUMERATION_TYPE;    ///< type for band enumeration

static BAND_ENUMERATION_TYPE BAND_ENUMERATION { "2190m",
                                                "560m",
                                                "160m",
                                                "80m",
                                                "60m",
                                                "40m",
                                                "30m",
                                                "20m",
                                                "17m",
                                                "15m",
                                                "12m",
                                                "10m",
                                                "6m",
                                                "4m",
                                                "2m",
                                                "1.25m",
                                                "70cm",
                                                "33cm",
                                                "23cm",
                                                "13cm",
                                                "9cm",
                                                "6cm",
                                                "3cm",
                                                "1.25cm",
                                                "6mm",
                                                "4mm",
                                                "2.5mm",
                                                "2mm",
                                                "1mm"
                                              };

// contest  -------------------------------------------------------

/// contests
enum CONTEST_ENUM { CONTEST_7QP,                //      7th-Area QSO Party
                    CONTEST_ANARTS_RTTY,        //      ANARTS WW RTTY
                    CONTEST_ANATOLIAN_RTTY,     //      Anatolian WW RTTY
                    CONTEST_AP_SPRINT,          //      Asia - Pacific Sprint
                    CONTEST_ARI_DX,             //      ARI DX Contest
                    CONTEST_ARRL_10,            //      ARRL 10 Meter Contest
                    CONTEST_ARRL_160,           //      ARRL 160 Meter Contest
                    CONTEST_ARRL_DX_CW,         //      ARRL International DX Contest (CW)
                    CONTEST_ARRL_DX_SSB,        //      ARRL International DX Contest (Phone)
                    CONTEST_ARRL_FIELD_DAY,     //      ARRL Field Day
                    CONTEST_ARRL_RTTY,          //      ARRL RTTY Round-Up
                    CONTEST_ARRL_SS_CW,         //      ARRL November Sweepstakes (CW)
                    CONTEST_ARRL_SS_SSB,        //      ARRL November Sweepstakes (Phone)
                    CONTEST_ARRL_UHF_AUG,       //      ARRL August UHF Contest
                    CONTEST_ARRL_VHF_JAN,       //      ARRL January VHF Sweepstakes
                    CONTEST_ARRL_VHF_JUN,       //      ARRL June VHF QSO Party
                    CONTEST_ARRL_VHF_SEP,       //      ARRL September VHF QSO Party
                    CONTEST_BARTG_RTTY,         //      BARTG Spring RTTY Contest
                    CONTEST_BARTG_SPRINT,       //      BARTG Sprint Contest
                    CONTEST_CA_QSO_PARTY,       //      California QSO Party
                    CONTEST_CQ_160_CW,          //      CQ WW 160 Meter DX Contest (CW)
                    CONTEST_CQ_160_SSB,         //      CQ WW 160 Meter DX Contest (SSB)
                    CONTEST_CQ_VHF,             //      CQ World_Wide VHF Contest
                    CONTEST_CQ_WPX_CW,          //      CQ WW WPX Contest (CW)
                    CONTEST_CQ_WPX_RTTY,        //      CQ/RJ WW RTTY WPX Contest
                    CONTEST_CQ_WPX_SSB,         //      CQ WW WPX Contest (SSB)
                    CONTEST_CQ_WW_CW,           //      CQ WW DX Contest (CW)
                    CONTEST_CQ_WW_RTTY,         //      CQ/RJ WW RTTY DX Contest
                    CONTEST_CQ_WW_SSB,          //      CQ WW DX Contest (SSB)
                    CONTEST_CWOPS_CWT,          //      CWops Mini-CWT Test
                    CONTEST_CIS_DX,             //      CIS DX Contest
                    CONTEST_DARC_WAEDC_CW,      //      WAE DX Contest (CW)
                    CONTEST_DARC_WAEDC_RTTY,    //      WAE DX Contest (RTTY)
                    CONTEST_DARC_WAEDC_SSB,     //      WAE DX Contest (SSB)
                    CONTEST_DL_DX_RTTY,         //      DL-DX RTTY Contest
                    CONTEST_EA_RTTY,            //      EA-WW-RTTY
                    CONTEST_EPC_PSK63,          //      PSK63 QSO Party
                    CONTEST_EU_SPRINT,          //      EU Sprint
                    CONTEST_EUCW160M,           //
                    CONTEST_EU_HF,              //      EU HF Championship
                    CONTEST_EU_PSK_DX,          //      EU PSK DX Contest
                    CONTEST_FALL_sprint,        //      FISTS Fall Sprint
                    CONTEST_FL_QSO_PARTY,       //      Florida QSO Party
                    CONTEST_GA_QSO_PARTY,       //      Georgia QSO Party
                    CONTEST_HELVETIA,           //      Helvetia Contest
                    CONTEST_IARU_HF,            //      IARU HF World Championship
                    CONTEST_IL_QSO_party,       //      Illinois QSO Party
                    CONTEST_JARTS_WW_RTTY,      //      JARTS WW RTTY
                    CONTEST_JIDX_CW,            //      Japan International DX Contest (CW)
                    CONTEST_JIDX_SSB,           //      Japan International DX Contest (SSB)
                    CONTEST_LZ_DX,              //      LZ DX Contest
                    CONTEST_MI_QSO_PARTY,       //      Michigan QSO Party
                    CONTEST_NAQP_CW,            //      North America QSO Party (CW)
                    CONTEST_NAQP_RTTY,          //      North America QSO Party (RTTY)
                    CONTEST_NAQP_SSB,           //      North America QSO Party (Phone)
                    CONTEST_NA_SPRINT_CW,       //      North America Sprint (CW)
                    CONTEST_NA_SPRINT_RTTY,     //      North America Sprint (RTTY)
                    CONTEST_NA_SPRINT_SSB,      //      North America Sprint (Phone)
                    CONTEST_NEQP,               //      New England QSO Party
                    CONTEST_NRAU_BALTIC_CW,     //      NRAU-Baltic Contest (CW)
                    CONTEST_NRAU_BALTIC_SSB,    //      NRAU-Baltic Contest (SSB)
                    CONTEST_OCEANIA_DX_CW,      //      Oceania DX Contest (CW)
                    CONTEST_OCEANIA_DX_SSB,     //      Oceania DX Contest (SSB)
                    CONTEST_OH_QSO_PARTY,       //      Ohio QSO Party
                    CONTEST_OK_DX_RTTY,         //
                    CONTEST_OK_OM_DX,           //      OK-OM DX Contest
                    CONTEST_ON_QSO_PARTY,       //      Ontario QSO Party
                    CONTEST_PACC,               //
                    CONTEST_QC_QSO_PARTY,       //      Quebec QSO Party
                    CONTEST_RAC,                //      Canada Day, RAC Winter contests
                    CONTEST_RDAC,               //      Russian District Award Contest
                    CONTEST_RDXC,               //      Russian DX Contest
                    CONTEST_REF_160M,           //
                    CONTEST_REF_CW,             //
                    CONTEST_REF_SSB,            //
                    CONTEST_RSGB_160,           //      1.8Mhz Contest
                    CONTEST_RSGB_21_28_CW,      //      21/28 MHz Contest (CW)
                    CONTEST_RSGB_21_28_SSB,     //      21/28 MHz Contest (SSB)
                    CONTEST_RSGB_80M_CC,        //      80m Club Championships
                    CONTEST_RSGB_AFS_CW,        //      Affiliated Societies Team Contest (CW)
                    CONTEST_RSGB_AFS_SSB,       //      Affiliated Societies Team Contest (SSB)
                    CONTEST_RSGB_CLUB_CALLS,    //      Club Calls
                    CONTEST_RSGB_COMMONWEALTH,  //      Commonwealth Contest
                    CONTEST_RSGB_IOTA,          //      IOTA Contest
                    CONTEST_RSGB_LOW_POWER,     //      Low Power Field Day
                    CONTEST_RSGB_NFD,           //      National Field Day
                    CONTEST_RSGB_ROPOCO,        //      RoPoCo
                    CONTEST_RSGB_SSB_FD,        //      SSB Field Day
                    CONTEST_RUSSIAN_RTTY,       //
                    CONTEST_SAC_CW,             //      Scandinavian Activity Contest (CW)
                    CONTEST_SAC_SSB,            //      Scandinavian Activity Contest (SSB)
                    CONTEST_SARTG_RTTY,         //      SARTG WW RTTY
                    CONTEST_SCC_RTTY,           //      SCC RTTY Championship
                    CONTEST_SMP_AUG,            //      SSA Portabeltest
                    CONTEST_SMP_MAY,            //      SSA Portabeltest
                    CONTEST_SPDXCCONTEST,       //       SP DX Contest
                    CONTEST_SPRING_SPRINT,      //      FISTS Spring Sprint
                    CONTEST_SR_MARATHON,        //      Scottish-Russian Marathon
                    CONTEST_STEW_PERRY,         //      Stew Perry Topband Distance Challenge
                    CONTEST_SUMMER_SPRINT,      //      FISTS Summer Sprint
                    CONTEST_TARA_RTTY,          //      TARA RTTY Mêlée
                    CONTEST_TMC_RTTY,           //      The Makrothen Contest
                    CONTEST_UBA_DX_CW,          //      UBA Contest (CW)
                    CONTEST_UBA_DX_SSB,         //      UBA Contest (SSB)
                    CONTEST_UK_DX_RTTY,         //      UK DX RTTY Contest
                    CONTEST_UKRAINIAN_DX,       //      Ukrainian DX
                    CONTEST_UKR_CHAMP_RTTY,     //      Open Ukraine RTTY Championship
                    CONTEST_URE_DX,             //
                    CONTEST_VIRGINIA_QSO_PARTY, //      Virginia QSO Party
                    CONTEST_VOLTA_RTTY,         //      Alessandro Volta RTTY DX Contest
                    CONTEST_WI_QSO_PARTY,       //      Wisconsin QSO Party
                    CONTEST_WINTER_SPRINT,      //      FISTS Winter Sprint
                    CONTEST_YUDXC,              //      YU DX Contest
                    N_CONTESTS
                  };

typedef std::array<std::string, N_CONTESTS> CONTEST_ENUMERATION_TYPE;                           ///< type for contest enumeration

static CONTEST_ENUMERATION_TYPE CONTEST_ENUMERATION = { { "7QP",                                //  7th-Area QSO Party
                                                          "ANARTS-RTTY",                        //  ANARTS WW RTTY
                                                          "ANATOLIAN-RTTY",                     //  Anatolian WW RTTY
                                                          "AP-SPRINT",                          //  Asia - Pacific Sprint
                                                          "ARI-DX",                             //  ARI DX Contest
                                                          "ARRL-10",                            //  ARRL 10 Meter Contest
                                                          "ARRL-160",                           //  ARRL 160 Meter Contest
                                                          "ARRL-DX-CW",                         //  ARRL International DX Contest (CW)
                                                          "ARRL-DX-SSB",                        //  ARRL International DX Contest (Phone)
                                                          "ARRL-FIELD-DAY",                     //  ARRL Field Day
                                                          "ARRL-RTTY",                          //  ARRL RTTY Round-Up
                                                          "ARRL-SS-CW",                         //  ARRL November Sweepstakes (CW)
                                                          "ARRL-SS-SSB",                        //  ARRL November Sweepstakes (Phone)
                                                          "ARRL-UHF-AUG",                       //  ARRL August UHF Contest
                                                          "ARRL-VHF-JAN",                       //  ARRL January VHF Sweepstakes
                                                          "ARRL-VHF-JUN",                       //  ARRL June VHF QSO Party
                                                          "ARRL-VHF-SEP",                       //  ARRL September VHF QSO Party
                                                          "BARTG-RTTY",                         //  BARTG Spring RTTY Contest
                                                          "BARTG-SPRINT",                       //  BARTG Sprint Contest
                                                          "CA-QSO-PARTY",                       //  California QSO Party
                                                          "CQ-160-CW",                          //  CQ WW 160 Meter DX Contest (CW)
                                                          "CQ-160-SSB",                         //  CQ WW 160 Meter DX Contest (SSB)
                                                          "CQ-VHF",                             //  CQ World-Wide VHF Contest
                                                          "CQ-WPX-CW",                          //  CQ WW WPX Contest (CW)
                                                          "CQ-WPX-RTTY",                        //  CQ/RJ WW RTTY WPX Contest
                                                          "CQ-WPX-SSB",                         //  CQ WW WPX Contest (SSB)
                                                          "CQ-WW-CW",                           //  CQ WW DX Contest (CW)
                                                          "CQ-WW-RTTY",                         //  CQ/RJ WW RTTY DX Contest
                                                          "CQ-WW-SSB",                          //  CQ WW DX Contest (SSB)
                                                          "CWOPS-CWT",                          //  CWops Mini-CWT Test
                                                          "CIS-DX",                             //  CIS DX Contest
                                                          "DARC-WAEDC-CW",                      //  WAE DX Contest (CW)
                                                          "DARC-WAEDC-RTTY",                    //  WAE DX Contest (RTTY)
                                                          "DARC-WAEDC-SSB",                     //  WAE DX Contest (SSB)
                                                          "DL-DX-RTTY",                         //  DL-DX RTTY Contest
                                                          "EA-RTTY",                            //  EA-WW-RTTY
                                                          "EPC-PSK63",                          //  PSK63 QSO Party
                                                          "EU Sprint",                          //  EU Sprint
                                                          "EUCW160M",                           //
                                                          "EU-HF",                              //  EU HF Championship
                                                          "EU-PSK-DX",                          //  EU PSK DX Contest
                                                          "Fall Sprint",                        //  FISTS Fall Sprint
                                                          "FL-QSO-PARTY",                       //  Florida QSO Party
                                                          "GA-QSO-PARTY",                       //  Georgia QSO Party
                                                          "HELVETIA",                           //  Helvetia Contest
                                                          "IARU-HF",                            //  IARU HF World Championship
                                                          "IL QSO Party",                       //  Illinois QSO Party
                                                          "JARTS-WW-RTTY",                      //  JARTS WW RTTY
                                                          "JIDX-CW",                            //  Japan International DX Contest (CW)
                                                          "JIDX-SSB",                           //  Japan International DX Contest (SSB)
                                                          "LZ DX",                              //  LZ DX Contest
                                                          "MI-QSO-PARTY",                       //  Michigan QSO Party
                                                          "NAQP-CW",                            //  North America QSO Party (CW)
                                                          "NAQP-RTTY",                          //  North America QSO Party (RTTY)
                                                          "NAQP-SSB",                           //  North America QSO Party (Phone)
                                                          "NA-SPRINT-CW",                       //  North America Sprint (CW)
                                                          "NA-SPRINT-RTTY",                     //  North America Sprint (RTTY)
                                                          "NA-SPRINT-SSB",                      //  North America Sprint (Phone)
                                                          "NEQP",                               //  New England QSO Party
                                                          "NRAU-BALTIC-CW",                     //  NRAU-Baltic Contest (CW)
                                                          "NRAU-BALTIC-SSB",                    //  NRAU-Baltic Contest (SSB)
                                                          "OCEANIA-DX-CW",                      //  Oceania DX Contest (CW)
                                                          "OCEANIA-DX-SSB",                     //  Oceania DX Contest (SSB)
                                                          "OH-QSO-PARTY",                       //  Ohio QSO Party
                                                          "OK-DX-RTTY",                         //
                                                          "OK-OM-DX",                           //  OK-OM DX Contest
                                                          "ON-QSO-PARTY",                       //  Ontario QSO Party
                                                          "PACC",                               //
                                                          "QC-QSO-PARTY",                       //  Quebec QSO Party
                                                          "RAC, CANADA DAY, CANADA WINTER",     //  Canada Day, RAC Winter contests
                                                          "RDAC",                               //  Russian District Award Contest
                                                          "RDXC",                               //  Russian DX Contest
                                                          "REF-160M",                           //
                                                          "REF-CW",                             //
                                                          "REF-SSB",                            //
                                                          "RSGB-160",                           //  1.8Mhz (sic) Contest
                                                          "RSGB-21/28-CW",                      //  21/28 MHz Contest (CW)
                                                          "RSGB-21/28-SSB",                     //  21/28 MHz Contest (SSB)
                                                          "RSGB-80M-CC",                        //  80m Club Championships
                                                          "RSGB-AFS-CW",                        //  Affiliated Societies Team Contest (CW)
                                                          "RSGB-AFS-SSB",                       //  Affiliated Societies Team Contest (SSB)
                                                          "RSGB-CLUB-CALLS",                    //  Club Calls
                                                          "RSGB-COMMONWEALTH",                  //  Commonwealth Contest
                                                          "RSGB-IOTA",                          //  IOTA Contest
                                                          "RSGB-LOW-POWER",                     //  Low Power Field Day
                                                          "RSGB-NFD",                           //  National Field Day
                                                          "RSGB-ROPOCO",                        //  RoPoCo
                                                          "RSGB-SSB-FD",                        //  SSB Field Day
                                                          "RUSSIAN-RTTY",                       //
                                                          "SAC-CW",                             //  Scandinavian Activity Contest (CW)
                                                          "SAC-SSB",                            //  Scandinavian Activity Contest (SSB)
                                                          "SARTG-RTTY",                         //  SARTG WW RTTY
                                                          "SCC-RTTY",                           //  SCC RTTY Championship
                                                          "SMP-AUG",                            //  SSA Portabeltest
                                                          "SMP-MAY",                            //  SSA Portabeltest
                                                          "SPDXContest",                        //  SP DX Contest
                                                          "Spring Sprint",                      //  FISTS Spring Sprint
                                                          "SR-MARATHON",                        //  Scottish-Russian Marathon
                                                          "STEW-PERRY",                         //  Stew Perry Topband Distance Challenge
                                                          "Summer Sprint",                      //  FISTS Summer Sprint
                                                          "TARA-RTTY",                          //  TARA RTTY Mêlée
                                                          "TMC-RTTY",                           //  The Makrothen Contest
                                                          "UBA-DX-CW",                          //  UBA Contest (CW)
                                                          "UBA-DX-SSB",                         //  UBA Contest (SSB)
                                                          "UK-DX-RTTY",                         //  UK DX RTTY Contest
                                                          "UKRAINIAN DX",                       //  Ukrainian DX
                                                          "UKR-CHAMP-RTTY",                     //  Open Ukraine RTTY Championship
                                                          "URE-DX",                             //
                                                          "Virginia QSO Party",                 //  Virginia QSO Party
                                                          "VOLTA-RTTY",                         //  Alessandro Volta RTTY DX Contest
                                                          "WI-QSO-PARTY",                       //  Wisconsin QSO Party
                                                          "Winter Sprint",                      //  FISTS Winter Sprint
                                                          "YUDXC",                              //  YU DX Contest
                                                      } };                                          ///< values for contests

// propagation mode  -------------------------------------------------------

/// propagation modes
enum PROPAGATION_MODE_ENUM { PROP_MODE_AUR,         //      Aurora
                             PROP_MODE_AUE,         //      Aurora-E
                             PROP_MODE_BS,          //      Back scatter
                             PROP_MODE_ECH,         //      EchoLink
                             PROP_MODE_EME,         //      Earth-Moon-Earth
                             PROP_MODE_ES,          //      Sporadic E
                             PROP_MODE_FAI,         //      Field Aligned Irregularities
                             PROP_MODE_F2,          //      F2 Reflection
                             PROP_MODE_INTERNET,    //      Internet-assisted
                             PROP_MODE_ION,         //      Ionoscatter
                             PROP_MODE_IRL,         //      IRLP
                             PROP_MODE_MS,          //      Meteor scatter
                             PROP_MODE_RPT,         //      Terrestrial or atmospheric repeater or transponder
                             PROP_MODE_RS,          //      Rain scatter
                             PROP_MODE_SAT,         //      Satellite
                             PROP_MODE_TEP,         //      Trans-equatorial
                             PROP_MODE_TR,          //      Tropospheric ducting
                             N_PROP_MODES
};

typedef std::array<std::string, N_PROP_MODES> PROPAGATION_MODE_ENUMERATION_TYPE;    ///< type for propagation mode enumeration

static PROPAGATION_MODE_ENUMERATION_TYPE PROPAGATION_MODE_ENUMERATION = { { "AUR",          //  Aurora
                                                                            "AUE",          //  Aurora-E
                                                                            "BS",           //  Back scatter
                                                                            "ECH",          //  EchoLink
                                                                            "EME",          //  Earth-Moon-Earth
                                                                            "ES",           //  Sporadic E
                                                                            "FAI",          //  Field Aligned Irregularities
                                                                            "F2",           //  F2 Reflection
                                                                            "INTERNET",     //  Internet-assisted
                                                                            "ION",          //  Ionoscatter
                                                                            "IRL",          //  IRLP
                                                                            "MS",           //  Meteor scatter
                                                                            "RPT",          //  Terrestrial or atmospheric repeater or transponder
                                                                            "RS",           //  Rain scatter
                                                                            "SAT",          //  Satellite
                                                                            "TEP",          //  Trans-equatorial
                                                                            "TR"            //  Tropospheric ducting
                                                                        } };                    ///< values for propagatiom mode

// primary administrative subdivisions  -------------------------------------------------------

/// Canada
enum PRIMARY_ENUM_CANADA { CANADA_NS,                   // Nova Scotia
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

typedef std::array<std::string, N_CANADA_PRIMARIES> PRIMARY_CANADA_ENUMERATION_TYPE; ///< type for Canada enumeration

static PRIMARY_CANADA_ENUMERATION_TYPE PRIMARY_CANADA_ENUMERATION = { { "NS",
                                                                        "QC",
                                                                        "ON",
                                                                        "MB",
                                                                        "AB",
                                                                        "BC",
                                                                        "NT",
                                                                        "NB",
                                                                        "NL",
                                                                        "YT",
                                                                        "PE",
                                                                        "NU"
                                                                    } };            ///< values for Canada

/// Aland Is.
enum PRIMARY_ENUM_ALAND { ALAND_001,    //     Brändö
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

typedef std::array<std::string, N_ALAND_PRIMARIES> PRIMARY_ALAND_ENUMERATION_TYPE;    ///< primaries for Aland Is.

static PRIMARY_ALAND_ENUMERATION_TYPE PRIMARY_ALAND_ENUMERATION = { { "001",    //     Brändö
                                                                      "002",    //     Eckerö
                                                                      "003",    //     Finström
                                                                      "004",    //     Föglö
                                                                      "005",    //     Geta
                                                                      "006",    //     Hammarland
                                                                      "007",    //     Jomala
                                                                      "008",    //     Kumlinge
                                                                      "009",    //     Kökar
                                                                      "010",    //     Lemland
                                                                      "011",    //     Lumparland
                                                                      "012",    //     Maarianhamina
                                                                      "013",    //     Saltvik
                                                                      "014",    //     Sottunga
                                                                      "015",    //     Sund
                                                                      "016"     //     Vårdö
                                                                    } };

/// Alaska
enum PRIMARY_ENUM_ALASKA { ALASKA_AK,    //     ALASKA
                           N_ALASKA_PRIMARIES
                         };

typedef std::array<std::string, N_ALASKA_PRIMARIES> PRIMARY_ALASKA_ENUMERATION_TYPE;    ///< primaries for Alaska

static PRIMARY_ALASKA_ENUMERATION_TYPE PRIMARY_ALASKA_ENUMERATION = { { "AK"    //     Alaska
                                                                    } };

/// Asiatic Russia
enum PRIMARY_ENUM_ASIATIC_RUSSIA { ASIATIC_RUSSIA_UO,      // 174  Ust\u2019-Ordynsky Autonomous Okrug - for contacts made before 2008-01-01
                                   ASIATIC_RUSSIA_AB,      // 175  Aginsky Buryatsky Autonomous Okrug - for contacts made before 2008-03-01
                                   ASIATIC_RUSSIA_CB,      // 165  Chelyabinsk (Chelyabinskaya oblast)
                                   ASIATIC_RUSSIA_SV,      // 154  Sverdlovskaya oblast
                                   ASIATIC_RUSSIA_PM,      // 140  Perm` (Permskaya oblast) - for contacts made on or after 2005-12-01
//                                   ASIATIC_RUSSIA_PM,      // 140  Permskaya Kraj - for contacts made before 2005-12-01
                                   ASIATIC_RUSSIA_KP,      // 141  Komi-Permyatsky Autonomous Okrug - for contacts made before 2005-12-01
                                   ASIATIC_RUSSIA_TO,      // 158  Tomsk (Tomskaya oblast)
                                   ASIATIC_RUSSIA_HM,      // 162  Khanty-Mansyisky Autonomous Okrug
                                   ASIATIC_RUSSIA_YN,      // 163  Yamalo-Nenetsky Autonomous Okrug
                                   ASIATIC_RUSSIA_TN,      // 161  Tyumen' (Tyumenskaya oblast)
                                   ASIATIC_RUSSIA_OM,      // 146  Omsk (Omskaya oblast)
                                   ASIATIC_RUSSIA_NS,      // 145  Novosibirsk (Novosibirskaya oblast)
                                   ASIATIC_RUSSIA_KN,      // 134  Kurgan (Kurganskaya oblast)
                                   ASIATIC_RUSSIA_OB,      // 167  Orenburg (Orenburgskaya oblast)
                                   ASIATIC_RUSSIA_KE,      // 130  Kemerovo (Kemerovskaya oblast)
                                   ASIATIC_RUSSIA_BA,      // 84   Republic of Bashkortostan
                                   ASIATIC_RUSSIA_KO,      // 90   Republic of Komi
                                   ASIATIC_RUSSIA_AL,      // 99   Altaysky Kraj
                                   ASIATIC_RUSSIA_GA,      // 100  Republic Gorny Altay
                                   ASIATIC_RUSSIA_KK,      // 103  Krasnoyarsk (Krasnoyarsk Kraj)
//                                   ASIATIC_RUSSIA_KK,      // 103  Krasnoyarsk (Krasnoyarsk Kraj) - for contacts made on or after 2007-01-01
                                   ASIATIC_RUSSIA_TM,      // 105  Taymyr Autonomous Okrug - for contacts made before 2007-01-01
                                   ASIATIC_RUSSIA_HK,      // 110  Khabarovsk (Khabarovsky Kraj)
                                   ASIATIC_RUSSIA_EA,      // 111  Yevreyskaya Autonomous Oblast
                                   ASIATIC_RUSSIA_SL,      // 153  Sakhalin (Sakhalinskaya oblast)
                                   ASIATIC_RUSSIA_EV,      // 106  Evenkiysky Autonomous Okrug - for contacts made before 2007-01-01
                                   ASIATIC_RUSSIA_MG,      // 138  Magadan (Magadanskaya oblast)
                                   ASIATIC_RUSSIA_AM,      // 112  Amurskaya oblast
                                   ASIATIC_RUSSIA_CK,      // 139  Chukotka Autonomous Okrug
                                   ASIATIC_RUSSIA_PK,      // 107  Primorsky Kraj
                                   ASIATIC_RUSSIA_BU,      // 85   Republic of Buryatia
                                   ASIATIC_RUSSIA_YA,      // 98   Sakha (Yakut) Republic
                                   ASIATIC_RUSSIA_IR,      // 124  Irkutsk (Irkutskaya oblast)
                                   ASIATIC_RUSSIA_CT,      // 166  Zabaykalsky Kraj - referred to as Chita (Chitinskaya oblast) before 2008-03-01
                                   ASIATIC_RUSSIA_HA,      // 104  Republic of Khakassia
                                   ASIATIC_RUSSIA_KY,      // 129  Koryaksky Autonomous Okrug - for contacts made before 2007-01-01
                                   ASIATIC_RUSSIA_KT,      // 128  Kamchatka (Kamchatskaya oblast) - for contacts made on or after 2007-01-01
                                   ASIATIC_RUSSIA_TU,      // 159  Republic of Tuva
//                                   ASIATIC_RUSSIA_KT,      // 128  Kamchatka (Kamchatskaya oblast)
                                   N_ASIATIC_RUSSIA_PRIMARIES
                                 };

typedef std::array<std::string, N_ASIATIC_RUSSIA_PRIMARIES> PRIMARY_ASIATIC_RUSSIA_ENUMERATION_TYPE;    ///< primaries for Asiatic Russia

static PRIMARY_ASIATIC_RUSSIA_ENUMERATION_TYPE PRIMARY_ASIATIC_RUSSIA_ENUMERATION = { { "UO",      // 174  Ust\u2019-Ordynsky Autonomous Okrug - for contacts made before 2008-01-01
                                                                                        "AB",      // 175  Aginsky Buryatsky Autonomous Okrug - for contacts made before 2008-03-01
                                                                                        "CB",      // 165  Chelyabinsk (Chelyabinskaya oblast)
                                                                                        "SV",      // 154  Sverdlovskaya oblast
                                                                                        "PM",      // 140  Perm` (Permskaya oblast) - for contacts made on or after 2005-12-01
//                                   ASIATIC_RUSSIA_PM,      // 140  Permskaya Kraj - for contacts made before 2005-12-01
                                                                                        "KP",      // 141  Komi-Permyatsky Autonomous Okrug - for contacts made before 2005-12-01
                                                                                        "TO",      // 158  Tomsk (Tomskaya oblast)
                                                                                        "HM",      // 162  Khanty-Mansyisky Autonomous Okrug
                                                                                        "YN",      // 163  Yamalo-Nenetsky Autonomous Okrug
                                                                                        "TN",      // 161  Tyumen' (Tyumenskaya oblast)
                                                                                        "OM",      // 146  Omsk (Omskaya oblast)
                                                                                        "NS",      // 145  Novosibirsk (Novosibirskaya oblast)
                                                                                        "KN",      // 134  Kurgan (Kurganskaya oblast)
                                                                                        "OB",      // 167  Orenburg (Orenburgskaya oblast)
                                                                                        "KE",      // 130  Kemerovo (Kemerovskaya oblast)
                                                                                        "BA",      // 84   Republic of Bashkortostan
                                                                                        "KO",      // 90   Republic of Komi
                                                                                        "AL",      // 99   Altaysky Kraj
                                                                                        "GA",      // 100  Republic Gorny Altay
                                                                                        "KK",      // 103  Krasnoyarsk (Krasnoyarsk Kraj)
//                                   ASIATIC_RUSSIA_KK,      // 103  Krasnoyarsk (Krasnoyarsk Kraj) - for contacts made on or after 2007-01-01
                                                                                        "TM",      // 105  Taymyr Autonomous Okrug - for contacts made before 2007-01-01
                                                                                        "HK",      // 110  Khabarovsk (Khabarovsky Kraj)
                                                                                        "EA",      // 111  Yevreyskaya Autonomous Oblast
                                                                                        "SL",      // 153  Sakhalin (Sakhalinskaya oblast)
                                                                                        "EV",      // 106  Evenkiysky Autonomous Okrug - for contacts made before 2007-01-01
                                                                                        "MG",      // 138  Magadan (Magadanskaya oblast)
                                                                                        "AM",      // 112  Amurskaya oblast
                                                                                        "CK",      // 139  Chukotka Autonomous Okrug
                                                                                        "PK",      // 107  Primorsky Kraj
                                                                                        "BU",      // 85   Republic of Buryatia
                                                                                        "YA",      // 98   Sakha (Yakut) Republic
                                                                                        "IR",      // 124  Irkutsk (Irkutskaya oblast)
                                                                                        "CT",      // 166  Zabaykalsky Kraj - referred to as Chita (Chitinskaya oblast) before 2008-03-01
                                                                                        "HA",      // 104  Republic of Khakassia
                                                                                        "KY",      // 129  Koryaksky Autonomous Okrug - for contacts made before 2007-01-01
                                                                                        "KT",      // 128  Kamchatka (Kamchatskaya oblast) - for contacts made on or after 2007-01-01
                                                                                        "TU"       // 159  Republic of Tuva
//                                   ASIATIC_RUSSIA_KT,      // 128  Kamchatka (Kamchatskaya oblast)
                                                                    } };

/// Balearic Is. (the spec for some reason calls them "Baleric")
enum PRIMARY_ENUM_BALEARICS { BALEARICS_IB,
                              N_BALEARICS_PRIMARIES
                            };

typedef std::array<std::string, N_BALEARICS_PRIMARIES> PRIMARY_BALEARICS_ENUMERATION_TYPE;    ///< primaries for Balearics

static PRIMARY_BALEARICS_ENUMERATION_TYPE PRIMARY_BALEARICS_ENUMERATION = { { "IB"
                                                                          } };
/// Belarus
enum PRIMARY_ENUM_BELARUS { BELARUS_MI,      // Minsk (Minskaya voblasts')
                            BELARUS_BR,      // Brest (Brestskaya voblasts')
                            BELARUS_HR,      // Grodno (Hrodzenskaya voblasts')
                            BELARUS_VI,      // Vitebsk (Vitsyebskaya voblasts')
                            BELARUS_MA,      // Mogilev (Mahilyowskaya voblasts')
                            BELARUS_HO,      // Gomel (Homyel'skaya voblasts')
                            BELARUS_HM,      // Horad Minsk
                            N_BELARUS_PRIMARIES
                          };

typedef std::array<std::string, N_BELARUS_PRIMARIES> PRIMARY_BELARUS_ENUMERATION_TYPE;    ///< primaries for Belarus

static PRIMARY_BELARUS_ENUMERATION_TYPE PRIMARY_BELARUS_ENUMERATION = { { "MI",      // Minsk (Minskaya voblasts')
                                                                          "BR",      // Brest (Brestskaya voblasts')
                                                                          "HR",      // Grodno (Hrodzenskaya voblasts')
                                                                          "VI",      // Vitebsk (Vitsyebskaya voblasts')
                                                                          "MA",      // Mogilev (Mahilyowskaya voblasts')
                                                                          "HO",      // Gomel (Homyel'skaya voblasts')
                                                                          "HM"       // Horad Minsk
                                                                      } };

/// Canary Is.
enum PRIMARY_ENUM_CANARIES { CANARIES_GC,    // Las Palmas
                             CANARIES_TF,    // Tenerife
                             N_CANARIES_PRIMARIES
                           };

typedef std::array<std::string, N_CANARIES_PRIMARIES> PRIMARY_CANARIES_ENUMERATION_TYPE;    ///< primaries for Canaries


static PRIMARY_CANARIES_ENUMERATION_TYPE PRIMARY_CANARIES_ENUMERATION = { { "GC",    // Las Palmas
                                                                            "TF"     // Tenerife
                                                                        } };

/// Ceuta y Melilla. (which the standard calls "Cetua & Melilla". Sigh.)
enum PRIMARY_ENUM_CEUTA { CEUTA_CU,    // Ceuta
                          CEUTA_ML,    // Melilla
                          N_CEUTA_PRIMARIES
                        };

typedef std::array<std::string, N_CEUTA_PRIMARIES> PRIMARY_CEUTA_ENUMERATION_TYPE;    ///< primaries for Ceuta


static PRIMARY_CEUTA_ENUMERATION_TYPE PRIMARY_CEUTA_ENUMERATION = { { "CE",    // Ceuta
                                                                      "ML"     // Melilla
                                                                  } };

/// Mexico
enum PRIMARY_ENUM_MEXICO { MEXICO_COL,      // Colima
                           MEXICO_DF,       //   Distrito Federal
                           MEXICO_EMX,      //  Estado de México
                           MEXICO_GTO,      //  Guanajuato
                           MEXICO_HGO,      //  Hidalgo
                           MEXICO_JAL,      //  Jalisco
                           MEXICO_MIC,      //  Michoacán de Ocampo
                           MEXICO_MOR,      //  Morelos
                           MEXICO_NAY,      //  Nayarit
                           MEXICO_PUE,      //  Puebla
                           MEXICO_QRO,      //  Querétaro de Arteaga
                           MEXICO_TLX,      //  Tlaxcala
                           MEXICO_VER,      //  Veracruz-Llave
                           MEXICO_AGS,      //  Aguascalientes
                           MEXICO_BC,       //   Baja California
                           MEXICO_BCS,      //  Baja California Sur
                           MEXICO_CHH,      //  Chihuahua
                           MEXICO_COA,      //  Coahuila de Zaragoza
                           MEXICO_DGO,      //  Durango
                           MEXICO_NL,       //   Nuevo Leon
                           MEXICO_SLP,      //  San Luis Potosí
                           MEXICO_SIN,      //  Sinaloa
                           MEXICO_SON,      //  Sonora
                           MEXICO_TMS,      //  Tamaulipas
                           MEXICO_ZAC,      //  Zacatecas
                           MEXICO_CAM,      //  Campeche
                           MEXICO_CHS,      //  Chiapas
                           MEXICO_GRO,      //  Guerrero
                           MEXICO_OAX,      //  Oaxaca
                           MEXICO_QTR,      //  Quintana Roo
                           MEXICO_TAB,      //  Tabasco
                           MEXICO_YUC,      //  Yucatán
                           N_MEXICO_PRIMARIES
                         };

typedef std::array<std::string, N_MEXICO_PRIMARIES> PRIMARY_MEXICO_ENUMERATION_TYPE;    ///< primaries for Mexico

static PRIMARY_MEXICO_ENUMERATION_TYPE PRIMARY_MEXICO_ENUMERATION = { { "COL",      // Colima
                                                                        "DF",       //   Distrito Federal
                                                                        "EMX",      //  Estado de México
                                                                        "GTO",      //  Guanajuato
                                                                        "HGO",      //  Hidalgo
                                                                        "JAL",      //  Jalisco
                                                                        "MIC",      //  Michoacán de Ocampo
                                                                        "MOR",      //  Morelos
                                                                        "NAY",      //  Nayarit
                                                                        "PUE",      //  Puebla
                                                                        "QRO",      //  Querétaro de Arteaga
                                                                        "TLX",      //  Tlaxcala
                                                                        "VER",      //  Veracruz-Llave
                                                                        "AGS",      //  Aguascalientes
                                                                        "BC",       //   Baja California
                                                                        "BCS",      //  Baja California Sur
                                                                        "CHH",      //  Chihuahua
                                                                        "COA",      //  Coahuila de Zaragoza
                                                                        "DGO",      //  Durango
                                                                        "NL",       //   Nuevo Leon
                                                                        "SLP",      //  San Luis Potosí
                                                                        "SIN",      //  Sinaloa
                                                                        "SON",      //  Sonora
                                                                        "TMS",      //  Tamaulipas
                                                                        "ZAC",      //  Zacatecas
                                                                        "CAM",      //  Campeche
                                                                        "CHS",      //  Chiapas
                                                                        "GRO",      //  Guerrero
                                                                        "OAX",      //  Oaxaca
                                                                        "QTR",      //  Quintana Roo
                                                                        "TAB",      //  Tabasco
                                                                        "YUC"       //  Yucatán
                                                                    } };

/// European Russia
enum PRIMARY_ENUM_EU_RUSSIA { EU_RUSSIA_SP,      // 169  City of St. Petersburg
                              EU_RUSSIA_LO,      // 136  Leningradskaya oblast
                              EU_RUSSIA_KL,      // 88   Republic of Karelia
                              EU_RUSSIA_AR,      // 113  Arkhangelsk (Arkhangelskaya oblast)
                              EU_RUSSIA_NO,      // 114  Nenetsky Autonomous Okrug
                              EU_RUSSIA_VO,      // 120  Vologda (Vologodskaya oblast)
                              EU_RUSSIA_NV,      // 144  Novgorodskaya oblast
                              EU_RUSSIA_PS,      // 149  Pskov (Pskovskaya oblast)
                              EU_RUSSIA_MU,      // 143  Murmansk (Murmanskaya oblast)
                              EU_RUSSIA_MA,      // 170  City of Moscow
                              EU_RUSSIA_MO,      // 142  Moscowskaya oblast
                              EU_RUSSIA_OR,      // 147  Oryel (Orlovskaya oblast)
                              EU_RUSSIA_LP,      // 137  Lipetsk (Lipetskaya oblast)
                              EU_RUSSIA_TV,      // 126  Tver' (Tverskaya oblast)
                              EU_RUSSIA_SM,      // 155  Smolensk (Smolenskaya oblast)
                              EU_RUSSIA_YR,      // 168  Yaroslavl (Yaroslavskaya oblast)
                              EU_RUSSIA_KS,      // 132  Kostroma (Kostromskaya oblast)
                              EU_RUSSIA_TL,      // 160  Tula (Tul'skaya oblast)
                              EU_RUSSIA_VR,      // 121  Voronezh (Voronezhskaya oblast)
                              EU_RUSSIA_TB,      // 157  Tambov (Tambovskaya oblast)
                              EU_RUSSIA_RA,      // 151  Ryazan' (Ryazanskaya oblast)
                              EU_RUSSIA_NN,      // 122  Nizhni Novgorod (Nizhegorodskaya oblast)
                              EU_RUSSIA_IV,      // 123  Ivanovo (Ivanovskaya oblast)
                              EU_RUSSIA_VL,      // 119  Vladimir (Vladimirskaya oblast)
                              EU_RUSSIA_KU,      // 135  Kursk (Kurskaya oblast)
                              EU_RUSSIA_KG,      // 127  Kaluga (Kaluzhskaya oblast)
                              EU_RUSSIA_BR,      // 118  Bryansk (Bryanskaya oblast)
                              EU_RUSSIA_BO,      // 117  Belgorod (Belgorodskaya oblast)
                              EU_RUSSIA_VG,      // 156  Volgograd (Volgogradskaya oblast)
                              EU_RUSSIA_SA,      // 152  Saratov (Saratovskaya oblast)
                              EU_RUSSIA_PE,      // 148  Penza (Penzenskaya oblast)
                              EU_RUSSIA_SR,      // 133  Samara (Samarskaya oblast)
                              EU_RUSSIA_UL,      // 164  Ulyanovsk (Ulyanovskaya oblast)
                              EU_RUSSIA_KI,      // 131  Kirov (Kirovskaya oblast)
                              EU_RUSSIA_TA,      // 94   Republic of Tataria
                              EU_RUSSIA_MR,      // 91   Republic of Marij-El
                              EU_RUSSIA_MD,      // 92   Republic of Mordovia
                              EU_RUSSIA_UD,      // 95   Republic of Udmurtia
                              EU_RUSSIA_CU,      // 97   Republic of Chuvashia
                              EU_RUSSIA_KR,      // 101  Krasnodar (Krasnodarsky Kraj)
                              EU_RUSSIA_KC,      // 109  Republic of Karachaevo-Cherkessia
                              EU_RUSSIA_ST,      // 108  Stavropol' (Stavropolsky Kraj)
                              EU_RUSSIA_KM,      // 89   Republic of Kalmykia
                              EU_RUSSIA_SO,      // 93   Republic of Northern Ossetia
                              EU_RUSSIA_RO,      // 150  Rostov-on-Don (Rostovskaya oblast)
                              EU_RUSSIA_CN,      // 96   Republic Chechnya
                              EU_RUSSIA_IN,      // 96   Republic of Ingushetia
                              EU_RUSSIA_AO,      // 115  Astrakhan' (Astrakhanskaya oblast)
                              EU_RUSSIA_DA,      // 86   Republic of Daghestan
                              EU_RUSSIA_KB,      // 87   Republic of Kabardino-Balkaria
                              EU_RUSSIA_AD,      // 102  Republic of Adygeya
                              N_EU_RUSSIA_PRIMARIES
                            };

typedef std::array<std::string, N_EU_RUSSIA_PRIMARIES> PRIMARY_EU_RUSSIA_ENUMERATION_TYPE;    ///< primaries for European Russia

static PRIMARY_EU_RUSSIA_ENUMERATION_TYPE PRIMARY_EU_RUSSIA_ENUMERATION = { { "SP",      // 169  City of St. Petersburg
                                                                              "LO",      // 136  Leningradskaya oblast
                                                                              "KL",      // 88   Republic of Karelia
                                                                              "AR",      // 113  Arkhangelsk (Arkhangelskaya oblast)
                                                                              "NO",      // 114  Nenetsky Autonomous Okrug
                                                                              "VO",      // 120  Vologda (Vologodskaya oblast)
                                                                              "NV",      // 144  Novgorodskaya oblast
                                                                              "PS",      // 149  Pskov (Pskovskaya oblast)
                                                                              "MU",      // 143  Murmansk (Murmanskaya oblast)
                                                                              "MA",      // 170  City of Moscow
                                                                              "MO",      // 142  Moscowskaya oblast
                                                                              "OR",      // 147  Oryel (Orlovskaya oblast)
                                                                              "LP",      // 137  Lipetsk (Lipetskaya oblast)
                                                                              "TV",      // 126  Tver' (Tverskaya oblast)
                                                                              "SM",      // 155  Smolensk (Smolenskaya oblast)
                                                                              "YR",      // 168  Yaroslavl (Yaroslavskaya oblast)
                                                                              "KS",      // 132  Kostroma (Kostromskaya oblast)
                                                                              "TL",      // 160  Tula (Tul'skaya oblast)
                                                                              "VR",      // 121  Voronezh (Voronezhskaya oblast)
                                                                              "TB",      // 157  Tambov (Tambovskaya oblast)
                                                                              "RA",      // 151  Ryazan' (Ryazanskaya oblast)
                                                                              "NN",      // 122  Nizhni Novgorod (Nizhegorodskaya oblast)
                                                                              "IV",      // 123  Ivanovo (Ivanovskaya oblast)
                                                                              "VL",      // 119  Vladimir (Vladimirskaya oblast)
                                                                              "KU",      // 135  Kursk (Kurskaya oblast)
                                                                              "KG",      // 127  Kaluga (Kaluzhskaya oblast)
                                                                              "BR",      // 118  Bryansk (Bryanskaya oblast)
                                                                              "BO",      // 117  Belgorod (Belgorodskaya oblast)
                                                                              "VG",      // 156  Volgograd (Volgogradskaya oblast)
                                                                              "SA",      // 152  Saratov (Saratovskaya oblast)
                                                                              "PE",      // 148  Penza (Penzenskaya oblast)
                                                                              "SR",      // 133  Samara (Samarskaya oblast)
                                                                              "UL",      // 164  Ulyanovsk (Ulyanovskaya oblast)
                                                                              "KI",      // 131  Kirov (Kirovskaya oblast)
                                                                              "TA",      // 94   Republic of Tataria
                                                                              "MR",      // 91   Republic of Marij-El
                                                                              "MD",      // 92   Republic of Mordovia
                                                                              "UD",      // 95   Republic of Udmurtia
                                                                              "CU",      // 97   Republic of Chuvashia
                                                                              "KR",      // 101  Krasnodar (Krasnodarsky Kraj)
                                                                              "KC",      // 109  Republic of Karachaevo-Cherkessia
                                                                              "ST",      // 108  Stavropol' (Stavropolsky Kraj)
                                                                              "KM",      // 89   Republic of Kalmykia
                                                                              "SO",      // 93   Republic of Northern Ossetia
                                                                              "RO",      // 150  Rostov-on-Don (Rostovskaya oblast)
                                                                              "CN",      // 96   Republic Chechnya
                                                                              "IN",      // 96   Republic of Ingushetia
                                                                              "AO",      // 115  Astrakhan' (Astrakhanskaya oblast)
                                                                              "DA",      // 86   Republic of Daghestan
                                                                              "KB",      // 87   Republic of Kabardino-Balkaria
                                                                              "AD"      // 102  Republic of Adygeya
                                                                          } };

/// Franz Josef Land
enum PRIMARY_ENUM_FJL { FJL_FJL,
                        N_FJL_PRIMARIES
                      };

typedef std::array<std::string, N_FJL_PRIMARIES> PRIMARY_FJL_ENUMERATION_TYPE;    ///< primaries for Franz Josef Land

static PRIMARY_FJL_ENUMERATION_TYPE PRIMARY_FJL_ENUMERATION = { { "FJL"
                                                              } };

/// Argentina
enum PRIMARY_ENUM_ARGENTINA { ARGENTINA_C,  // Capital federal (Buenos Aires City)
                              ARGENTINA_B,  // Buenos Aires Province
                              ARGENTINA_S,  // Santa Fe
                              ARGENTINA_H,  // Chaco
                              ARGENTINA_P,  // Formosa
                              ARGENTINA_X,  // Cordoba
                              ARGENTINA_N,  // Misiones
                              ARGENTINA_E,  // Entre Rios
                              ARGENTINA_T,  // Tucumán
                              ARGENTINA_W,  // Corrientes
                              ARGENTINA_M,  // Mendoza
                              ARGENTINA_G,  // Santiago del Estero
                              ARGENTINA_A,  // Salta
                              ARGENTINA_J,  // San Juan
                              ARGENTINA_D,  // San Luis
                              ARGENTINA_K,  // Catamarca
                              ARGENTINA_F,  // La Rioja
                              ARGENTINA_Y,  // Jujuy
                              ARGENTINA_L,  // La Pampa
                              ARGENTINA_R,  // Rió Negro
                              ARGENTINA_U,  // Chubut
                              ARGENTINA_Z,  // Santa Cruz
                              ARGENTINA_V,  // Tierra del Fuego
                              ARGENTINA_Q,  // Neuquén
                              N_ARGENTINA_PRIMARIES
                      };

typedef std::array<std::string, N_ARGENTINA_PRIMARIES> PRIMARY_ARGENTINA_ENUMERATION_TYPE;    ///< primaries for Argentina

static PRIMARY_ARGENTINA_ENUMERATION_TYPE PRIMARY_ARGENTINA_ENUMERATION = { { "C",  // Capital federal (Buenos Aires City)
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
                                                                          } };

/// Brazil
enum PRIMARY_ENUM_BRAZIL { BRAZIL_ES,      // Espírito Santo
                           BRAZIL_GO,      // Goiás
                           BRAZIL_SC,      // Santa Catarina
                           BRAZIL_SE,      // Sergipe
                           BRAZIL_AL,      // Alagoas
                           BRAZIL_AM,      // Amazonas
                           BRAZIL_TO,      // Tocantins
                           BRAZIL_AP,      // Amapã
                           BRAZIL_PB,      // Paraíba
                           BRAZIL_MA,      // Maranhao
                           BRAZIL_RN,      // Rio Grande do Norte
                           BRAZIL_PI,      // Piaui
                           BRAZIL_DF,      // Oietrito Federal (Brasila)
                           BRAZIL_CE,      // Ceará
                           BRAZIL_AC,      // Acre
                           BRAZIL_MS,      // Mato Grosso do Sul
                           BRAZIL_RR,      // Roraima
                           BRAZIL_RO,      // Rondônia
                           BRAZIL_RJ,      // Rio de Janeiro
                           BRAZIL_SP,      // Sao Paulo
                           BRAZIL_RS,      // Rio Grande do Sul
                           BRAZIL_MG,      // Minas Gerais
                           BRAZIL_PR,      // Paranã
                           BRAZIL_BA,      // Bahia
                           BRAZIL_PE,      // Pernambuco
                           BRAZIL_PA,      // Parã
                           BRAZIL_MT,      // Mato Grosso
                           N_BRAZIL_PRIMARIES
                         };

typedef std::array<std::string, N_BRAZIL_PRIMARIES> PRIMARY_BRAZIL_ENUMERATION_TYPE;    ///< primaries for Brazil

static PRIMARY_BRAZIL_ENUMERATION_TYPE PRIMARY_BRAZIL_ENUMERATION = { { "ES",      // Espírito Santo
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
                                                                    } };

/// Hawaii
enum PRIMARY_ENUM_HAWAII { HAWAII_HI,
                           N_HAWAII_PRIMARIES
                         };

typedef std::array<std::string, N_HAWAII_PRIMARIES> PRIMARY_HAWAII_ENUMERATION_TYPE;    ///< primaries for Hawaii

static PRIMARY_HAWAII_ENUMERATION_TYPE PRIMARY_HAWAII_ENUMERATION = { { "HI"
                                                                    } };

/// Chile
enum PRIMARY_ENUM_CHILE { CHILE_II,  // Antofagasta
                          CHILE_III,  // Atacama
                          CHILE_I,    // Tarapacá
                          CHILE_IV,   // Coquimbo
                          CHILE_V,    // Valparaíso
                          CHILE_RM,   // Region Metropolitana de Santiago
                          CHILE_VI,   // Libertador General Bernardo O'Higgins
                          CHILE_VII,  // Maule
                          CHILE_VIII, // Bío-Bío
                          CHILE_IX,   // La Araucanía
                          CHILE_X,    // Los Lagos
                          CHILE_XI,   // Aisén del General Carlos Ibáñez del Campo
                          CHILE_XII,  // Magallanes
                          N_CHILE_PRIMARIES
                        };

typedef std::array<std::string, N_CHILE_PRIMARIES> PRIMARY_CHILE_ENUMERATION_TYPE;    ///< primaries for Chile

static PRIMARY_CHILE_ENUMERATION_TYPE PRIMARY_CHILE_ENUMERATION = { { "II",   // Antofagasta
                                                                      "III",  // Atacama
                                                                      "I",    // Tarapacá
                                                                      "IV",   // Coquimbo
                                                                      "V",    // Valparaíso
                                                                      "RM",   // Region Metropolitana de Santiago
                                                                      "VI",   // Libertador General Bernardo O'Higgins
                                                                      "VII",  // Maule
                                                                      "VIII", // Bío-Bío
                                                                      "IX",   // La Araucanía
                                                                      "X",    // Los Lagos
                                                                      "XI",   // Aisén del General Carlos Ibáñez del Campo
                                                                      "XII"   // Magallanes
                                                                  } };

/// Kaliningrad
enum PRIMARY_ENUM_KALININGRAD { KALININGRAD_KA, // obl. 125 Kalingrad (Kaliningradskaya oblast)
                                N_KALININGRAD_PRIMARIES
                              };

typedef std::array<std::string, N_KALININGRAD_PRIMARIES> PRIMARY_KALININGRAD_ENUMERATION_TYPE;    ///< primaries for Kaliningrad

static PRIMARY_KALININGRAD_ENUMERATION_TYPE PRIMARY_KALININGRAD_ENUMERATION = { { "KA"
                                                                              } };

/// Paraguay
enum PRIMARY_ENUM_PARAGUAY { PARAGUAY_16,   // Alto Paraguay
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

typedef std::array<std::string, N_PARAGUAY_PRIMARIES> PRIMARY_PARAGUAY_ENUMERATION_TYPE;    ///< primaries for Paraguay

static PRIMARY_PARAGUAY_ENUMERATION_TYPE PRIMARY_PARAGUAY_ENUMERATION = { { "16",   // Alto Paraguay
                                                                            "19",   // Boquerón
                                                                            "15",   // Presidente Hayes
                                                                            "13",   // Amambay
                                                                            "01",   // Concepción
                                                                            "14",   // Canindeyú
                                                                            "02",   // San Pedro
                                                                            "ASU",  // Asunción
                                                                            "11",   // Central
                                                                            "03",   // Cordillera
                                                                            "09",   // Paraguarí
                                                                            "06",   // Caazapl
                                                                            "05",   // Caeguazú
                                                                            "04",   // Guairá
                                                                            "08",   // Miaiones
                                                                            "12",   // Ñeembucu
                                                                            "10",   // Alto Paraná
                                                                            "07"    // Itapua
                                                                        } };

/// ROK
enum PRIMARY_ENUM_SOUTH_KOREA { SOUTH_KOREA_A,  // Seoul (Seoul Teugbyeolsi)
                                SOUTH_KOREA_N,  // Inchon (Incheon Gwang'yeogsi)
                                SOUTH_KOREA_D,  // Kangwon-do (Gang 'weondo)
                                SOUTH_KOREA_C,  // Kyunggi-do (Gyeonggido)
                                SOUTH_KOREA_E,  // Choongchungbuk-do (Chungcheongbugdo)
                                SOUTH_KOREA_F,  // Choongchungnam-do (Chungcheongnamdo)
                                SOUTH_KOREA_R,  // Taejon (Daejeon Gwang'yeogsi)
                                SOUTH_KOREA_M,  // Cheju-do (Jejudo)
                                SOUTH_KOREA_G,  // Chollabuk-do (Jeonrabugdo)
                                SOUTH_KOREA_H,  // Chollanam-do (Jeonranamdo)
                                SOUTH_KOREA_Q,  // Kwangju (Gwangju Gwang'yeogsi)
                                SOUTH_KOREA_K,  // Kyungsangbuk-do (Gyeongsangbugdo)
                                SOUTH_KOREA_L,  // Kyungsangnam-do (Gyeongsangnamdo)
                                SOUTH_KOREA_B,  // Pusan (Busan Gwang'yeogsi)
                                SOUTH_KOREA_P,  // Taegu (Daegu Gwang'yeogsi)
                                SOUTH_KOREA_S,  // Ulsan (Ulsan Gwanq'yeogsi)
                                N_SOUTH_KOREA_PRIMARIES
                              };

typedef std::array<std::string, N_SOUTH_KOREA_PRIMARIES> PRIMARY_SOUTH_KOREA_ENUMERATION_TYPE;    ///< primaries for South Korea

static PRIMARY_SOUTH_KOREA_ENUMERATION_TYPE PRIMARY_SOUTH_KOREA_ENUMERATION = { { "A",  // Seoul (Seoul Teugbyeolsi)
                                                                                  "N",  // Inchon (Incheon Gwang'yeogsi)
                                                                                  "D",  // Kangwon-do (Gang 'weondo)
                                                                                  "C",  // Kyunggi-do (Gyeonggido)
                                                                                  "E",  // Choongchungbuk-do (Chungcheongbugdo)
                                                                                  "F",  // Choongchungnam-do (Chungcheongnamdo)
                                                                                  "R",  // Taejon (Daejeon Gwang'yeogsi)
                                                                                  "M",  // Cheju-do (Jejudo)
                                                                                  "G",  // Chollabuk-do (Jeonrabugdo)
                                                                                  "H",  // Chollanam-do (Jeonranamdo)
                                                                                  "Q",  // Kwangju (Gwangju Gwang'yeogsi)
                                                                                  "K",  // Kyungsangbuk-do (Gyeongsangbugdo)
                                                                                  "L",  // Kyungsangnam-do (Gyeongsangnamdo)
                                                                                  "B",  // Pusan (Busan Gwang'yeogsi)
                                                                                  "P",  // Taegu (Daegu Gwang'yeogsi)
                                                                                  "S"   // Ulsan (Ulsan Gwanq'yeogsi)
                                                                              } };

/// Kure
enum PRIMARY_ENUM_KURE { KURE_KI,
                         N_KURE_PRIMARIES
                       };

typedef std::array<std::string, N_KURE_PRIMARIES> PRIMARY_KURE_ENUMERATION_TYPE;    ///< primaries for Kure

static PRIMARY_KURE_ENUMERATION_TYPE PRIMARY_KURE_ENUMERATION = { { "KI"
                                                                } };

/// Uruguay
enum PRIMARY_ENUM_URUGUAY { URUGUAY_MO, // Montevideo
                            URUGUAY_CA, // Canelones
                            URUGUAY_SJ, // San José
                            URUGUAY_CO, // Colonia
                            URUGUAY_SO, // Soriano
                            URUGUAY_RN, // Rio Negro
                            URUGUAY_PA, // Paysandu
                            URUGUAY_SA, // Salto
                            URUGUAY_AR, // Artigsa
                            URUGUAY_FD, // Florida
                            URUGUAY_FS, // Flores
                            URUGUAY_DU, // Durazno
                            URUGUAY_TA, // Tacuarembo
                            URUGUAY_RV, // Rivera
                            URUGUAY_MA, // Maldonado
                            URUGUAY_LA, // Lavalleja
                            URUGUAY_RO, // Rocha
                            URUGUAY_TT, // Treinta y Tres
                            URUGUAY_CL, // Cerro Largo
                            N_URUGUAY_PRIMARIES
                          };

typedef std::array<std::string, N_URUGUAY_PRIMARIES> PRIMARY_URUGUAY_ENUMERATION_TYPE;    ///< primaries for Uruguay

static PRIMARY_URUGUAY_ENUMERATION_TYPE PRIMARY_URUGUAY_ENUMERATION = { { "MO", // Montevideo
                                                                          "CA", // Canelones
                                                                          "SJ", // San José
                                                                          "CO", // Colonia
                                                                          "SO", // Soriano
                                                                          "RN", // Rio Negro
                                                                          "PA", // Paysandu
                                                                          "SA", // Salto
                                                                          "AR", // Artigsa
                                                                          "FD", // Florida
                                                                          "FS", // Flores
                                                                          "DU", // Durazno
                                                                          "TA", // Tacuarembo
                                                                          "RV", // Rivera
                                                                          "MA", // Maldonado
                                                                          "LA", // Lavalleja
                                                                          "RO", // Rocha
                                                                          "TT", // Treinta y Tres
                                                                          "CL" // Cerro Largo
                                                                      } };

/// Lord Howe Is.
enum PRIMARY_ENUM_LORD_HOWE { LORD_HOWE_LH,
                              N_LORD_HOWE_PRIMARIES
                            };

typedef std::array<std::string, N_LORD_HOWE_PRIMARIES> PRIMARY_LORD_HOWE_ENUMERATION_TYPE;    ///< primaries for Lord Howe Is.

static PRIMARY_LORD_HOWE_ENUMERATION_TYPE PRIMARY_LORD_HOWE_ENUMERATION = { { "LH"
                                                                          } };

/// Venezuela
enum PRIMARY_ENUM_VENEZUELA { VENEZUELA_AM,     // Amazonas
                              VENEZUELA_AN,     // Anzoátegui
                              VENEZUELA_AP,     // Apure
                              VENEZUELA_AR,     // Aragua
                              VENEZUELA_BA,     // Barinas
                              VENEZUELA_BO,     // Bolívar
                              VENEZUELA_CA,     // Carabobo
                              VENEZUELA_CO,     // Cojedes
                              VENEZUELA_DA,     // Delta Amacuro
                              VENEZUELA_DC,     // Distrito Capital
                              VENEZUELA_FA,     // Falcón
                              VENEZUELA_GU,     // Guárico
                              VENEZUELA_LA,     // Lara
                              VENEZUELA_ME,     // Mérida
                              VENEZUELA_MI,     // Miranda
                              VENEZUELA_MO,     // Monagas
                              VENEZUELA_NE,     // Nueva Esparta
                              VENEZUELA_PO,     // Portuguesa
                              VENEZUELA_SU,     // Sucre
                              VENEZUELA_TA,     // Táchira
                              VENEZUELA_TR,     // Trujillo
                              VENEZUELA_VA,     // Vargas
                              VENEZUELA_YA,     // Yaracuy
                              VENEZUELA_ZU,     // Zulia
                              N_VENEZUELA_PRIMARIES
                            };

typedef std::array<std::string, N_VENEZUELA_PRIMARIES> PRIMARY_VENEZUELA_ENUMERATION_TYPE;    ///< primaries for Venezuela

static PRIMARY_VENEZUELA_ENUMERATION_TYPE PRIMARY_VENEZUELA_ENUMERATION = { { "AM",     // Amazonas
                                                                              "AN",     // Anzoátegui
                                                                              "AP",     // Apure
                                                                              "AR",     // Aragua
                                                                              "BA",     // Barinas
                                                                              "BO",     // Bolívar
                                                                              "CA",     // Carabobo
                                                                              "CO",     // Cojedes
                                                                              "DA",     // Delta Amacuro
                                                                              "DC",     // Distrito Capital
                                                                              "FA",     // Falcón
                                                                              "GU",     // Guárico
                                                                              "LA",     // Lara
                                                                              "ME",     // Mérida
                                                                              "MI",     // Miranda
                                                                              "MO",     // Monagas
                                                                              "NE",     // Nueva Esparta
                                                                              "PO",     // Portuguesa
                                                                              "SU",     // Sucre
                                                                              "TA",     // Táchira
                                                                              "TR",     // Trujillo
                                                                              "VA",     // Vargas
                                                                              "YA",     // Yaracuy
                                                                              "ZU"      // Zulia
                                                                          } };

/// Azores
enum PRIMARY_ENUM_AZORES { AZORES_AC,
                           N_AZORES_PRIMARIES
                         };

typedef std::array<std::string, N_AZORES_PRIMARIES> PRIMARY_AZORES_ENUMERATION_TYPE;    ///< primaries for Azores

static PRIMARY_AZORES_ENUMERATION_TYPE PRIMARY_AZORES_ENUMERATION = { { "AC"
                                                                    } };

/// Australia
enum PRIMARY_ENUM_AUSTRALIA { AUSTRALIA_ACT,    // Australian Capital Territory
                              AUSTRALIA_NSW,    // New South Wales
                              AUSTRALIA_VIC,    // Victoria
                              AUSTRALIA_QLD,    // Queensland
                              AUSTRALIA_SA,     // South Australia
                              AUSTRALIA_WA,     // Western Australia
                              AUSTRALIA_TAS,    // Tasmania
                              AUSTRALIA_NT,     // Northern Territory
                              N_AUSTRALIA_PRIMARIES
                           };

typedef std::array<std::string, N_AUSTRALIA_PRIMARIES> PRIMARY_AUSTRALIA_ENUMERATION_TYPE;    ///< primaries for Australia

static PRIMARY_AUSTRALIA_ENUMERATION_TYPE PRIMARY_AUSTRALIA_ENUMERATION = { { "ACT",    // Australian Capital Territory
                                                                              "NSW",    // New South Wales
                                                                              "VIC",    // Victoria
                                                                              "QLD",    // Queensland
                                                                              "SA",     // South Australia
                                                                              "WA",     // Western Australia
                                                                              "TAS",    // Tasmania
                                                                              "NT",     // Northern Territory
                                                                          } };

/// Malyj Vysotskij
enum PRIMARY_ENUM_MV { MV_MV,
                       N_MV_PRIMARIES
                     };

typedef std::array<std::string, N_MV_PRIMARIES> PRIMARY_MV_ENUMERATION_TYPE;    ///< primaries for Malyj Vysotskij

static PRIMARY_MV_ENUMERATION_TYPE PRIMARY_MV_ENUMERATION = { { "MV"
                                                            } };

/// Macquerie Is.
enum PRIMARY_ENUM_MACQUERIE { MACQUERIE_MA,
                              N_MACQUERIE_PRIMARIES
                            };

typedef std::array<std::string, N_MACQUERIE_PRIMARIES> PRIMARY_MACQUERIE_ENUMERATION_TYPE;    ///< primaries for Macquerie

static PRIMARY_MACQUERIE_ENUMERATION_TYPE PRIMARY_MACQUERIE_ENUMERATION = { { "MA"
                                                                        } };

/// Papua New Guinea
enum PRIMARY_ENUM_PAPUA_NEW_GUINEA { PAPUA_NEW_GUINEA_NCD,     // National Capital District (Port Moresby)
                                     PAPUA_NEW_GUINEA_CPM,     // Central
                                     PAPUA_NEW_GUINEA_CPK,     // Chimbu
                                     PAPUA_NEW_GUINEA_EHG,     // Eastern Highlands
                                     PAPUA_NEW_GUINEA_EBR,     // East New Britain
                                     PAPUA_NEW_GUINEA_ESW,     // East Sepik
                                     PAPUA_NEW_GUINEA_EPW,     // Enga
                                     PAPUA_NEW_GUINEA_GPK,     // Gulf
                                     PAPUA_NEW_GUINEA_MPM,     // Madang
                                     PAPUA_NEW_GUINEA_MRL,     // Manus
                                     PAPUA_NEW_GUINEA_MBA,     // Milne Bay
                                     PAPUA_NEW_GUINEA_MPL,     // Morobe
                                     PAPUA_NEW_GUINEA_NIK,     // New Ireland
                                     PAPUA_NEW_GUINEA_NPP,     // Northern
                                     PAPUA_NEW_GUINEA_NSA,     // North Solomons
                                     PAPUA_NEW_GUINEA_SAN,     // Santaun
                                     PAPUA_NEW_GUINEA_SHM,     // Southern Highlands
                                     PAPUA_NEW_GUINEA_WPD,     // Western
                                     PAPUA_NEW_GUINEA_WHM,     // Western Highlands
                                     PAPUA_NEW_GUINEA_WBR,     // West New Britain,
                                     N_PAPUA_NEW_GUINEA_PRIMARIES
                                   };

typedef std::array<std::string, N_PAPUA_NEW_GUINEA_PRIMARIES> PRIMARY_PAPUA_NEW_GUINEA_ENUMERATION_TYPE;    ///< primaries for Papua New Guinea

static PRIMARY_PAPUA_NEW_GUINEA_ENUMERATION_TYPE PRIMARY_PAPUA_NEW_GUINEA_ENUMERATION = { { "NCD",     // National Capital District (Port Moresby)
                                                                                            "CPM",     // Central
                                                                                            "CPK",     // Chimbu
                                                                                            "EHG",     // Eastern Highlands
                                                                                            "EBR",     // East New Britain
                                                                                            "ESW",     // East Sepik
                                                                                            "EPW",     // Enga
                                                                                            "GPK",     // Gulf
                                                                                            "MPM",     // Madang
                                                                                            "MRL",     // Manus
                                                                                            "MBA",     // Milne Bay
                                                                                            "MPL",     // Morobe
                                                                                            "NIK",     // New Ireland
                                                                                            "NPP",     // Northern
                                                                                            "NSA",     // North Solomons
                                                                                            "SAN",     // Santaun
                                                                                            "SHM",     // Southern Highlands
                                                                                            "WPD",     // Western
                                                                                            "WHM",     // Western Highlands
                                                                                            "WBR"      // West New Britain,
                                                                                        } };

/// New Zealand
enum PRIMARY_ENUM_NEW_ZEALAND { NEW_ZEALAND_NCD,    // National Capital District
                                NEW_ZEALAND_AUK,    // Auckland
                                NEW_ZEALAND_BOP,    // Bay of Plenty
                                NEW_ZEALAND_NTL,    // Northland
                                NEW_ZEALAND_WKO,    // Waikato
                                NEW_ZEALAND_GIS,    // Gisborne
                                NEW_ZEALAND_HKB,    // Hawkes Bay
                                NEW_ZEALAND_MWT,    // Manawatu-Wanganui
                                NEW_ZEALAND_TKI,    // Taranaki
                                NEW_ZEALAND_WGN,    // Wellington
                                NEW_ZEALAND_CAN,    // Canterbury
                                NEW_ZEALAND_MBH,    // Marlborough
                                NEW_ZEALAND_NSN,    // Nelson
                                NEW_ZEALAND_TAS,    // Tasman
                                NEW_ZEALAND_WTC,    // West Coast
                                NEW_ZEALAND_OTA,    // Otago
                                NEW_ZEALAND_STL,    // Southland
                                N_NEW_ZEALAND_PRIMARIES
                              };

typedef std::array<std::string, N_NEW_ZEALAND_PRIMARIES> PRIMARY_NEW_ZEALAND_ENUMERATION_TYPE;    ///< primaries for New  Zealand

static PRIMARY_NEW_ZEALAND_ENUMERATION_TYPE PRIMARY_NEW_ZEALAND_ENUMERATION = { { "NCD",    // National Capital District
                                                                                  "AUK",    // Auckland
                                                                                  "BOP",    // Bay of Plenty
                                                                                  "NTL",    // Northland
                                                                                  "WKO",    // Waikato
                                                                                  "GIS",    // Gisborne
                                                                                  "HKB",    // Hawkes Bay
                                                                                  "MWT",    // Manawatu-Wanganui
                                                                                  "TKI",    // Taranaki
                                                                                  "WGN",    // Wellington
                                                                                  "CAN",    // Canterbury
                                                                                  "MBH",    // Marlborough
                                                                                  "NSN",    // Nelson
                                                                                  "TAS",    // Tasman
                                                                                  "WTC",    // West Coast
                                                                                  "OTA",    // Otago
                                                                                  "STL"     // Southland
                                                                              } };

/// Austria
enum PRIMARY_ENUM_AUSTRIA { AUSTRIA_WC,   // Wien
                            AUSTRIA_HA,   // Hallein
                            AUSTRIA_JO,   // St. Johann
                            AUSTRIA_SC,   // Salzburg
                            AUSTRIA_SL,   // Salzburg-Land
                            AUSTRIA_TA,   // Tamsweg
                            AUSTRIA_ZE,   // Zell Am See
                            AUSTRIA_AM,   // Amstetten
                            AUSTRIA_BL,   // Bruck/Leitha
                            AUSTRIA_BN,   // Baden
                            AUSTRIA_GD,   // Gmünd
                            AUSTRIA_GF,   // Gänserndorf
                            AUSTRIA_HL,   // Hollabrunn
                            AUSTRIA_HO,   // Horn
                            AUSTRIA_KO,   // Korneuburg
                            AUSTRIA_KR,   // Krems-Region
                            AUSTRIA_KS,   // Krems
                            AUSTRIA_LF,   // Lilienfeld
                            AUSTRIA_MD,   // Mödling
                            AUSTRIA_ME,   // Melk
                            AUSTRIA_MI,   // Mistelbach
                            AUSTRIA_NK,   // Neunkirchen
                            AUSTRIA_PC,   // St. Pölten
                            AUSTRIA_PL,   // St. Pölten-Land
                            AUSTRIA_SB,   // Scheibbs
                            AUSTRIA_SW,   // Schwechat
                            AUSTRIA_TU,   // Tulln
                            AUSTRIA_WB,   // Wr.Neustadt-Bezirk
                            AUSTRIA_WN,   // Wr.Neustadt
                            AUSTRIA_WT,   // Waidhofen/Thaya
                            AUSTRIA_WU,   // Wien-Umgebung
                            AUSTRIA_WY,   // Waidhofen/Ybbs
                            AUSTRIA_ZT,   // Zwettl
                            AUSTRIA_EC,   // Eisenstadt
                            AUSTRIA_EU,   // Eisenstadt-Umgebung
                            AUSTRIA_GS,   // Güssing
                            AUSTRIA_JE,   // Jennersdorf
                            AUSTRIA_MA,   // Mattersburg
                            AUSTRIA_ND,   // Neusiedl/See
                            AUSTRIA_OP,   // Oberpullendorf
                            AUSTRIA_OW,   // Oberwart
                            AUSTRIA_BR,   // Braunau/Inn
                            AUSTRIA_EF,   // Eferding
                            AUSTRIA_FR,   // Freistadt
                            AUSTRIA_GM,   // Gmunden
                            AUSTRIA_GR,   // Grieskirchen
                            AUSTRIA_KI,   // Kirchdorf
                            AUSTRIA_LC,   // Linz
                            AUSTRIA_LL,   // Linz-Land
                            AUSTRIA_PE,   // Perg
                            AUSTRIA_RI,   // Ried/Innkreis
                            AUSTRIA_RO,   // Rohrbach
                            AUSTRIA_SD,   // Schärding
                            AUSTRIA_SE,   // Steyr-Land
                            AUSTRIA_SR,   // Steyr
                            AUSTRIA_UU,   // Urfahr
                            AUSTRIA_VB,   // Vöcklabruck
                            AUSTRIA_WE,   // Wels
                            AUSTRIA_WL,   // Wels-Land
                            AUSTRIA_BA,   // Bad Aussee
                            AUSTRIA_BM,   // Bruck/Mur
                            AUSTRIA_DL,   // Deutschlandsberg
                            AUSTRIA_FB,   // Feldbach
                            AUSTRIA_FF,   // Fürstenfeld
                            AUSTRIA_GB,   // Gröbming
                            AUSTRIA_GC,   // Graz
                            AUSTRIA_GU,   // Graz-Umgebung
                            AUSTRIA_HB,   // Hartberg
                            AUSTRIA_JU,   // Judenburg
                            AUSTRIA_KF,   // Knittelfeld
                            AUSTRIA_LB,   // Leibnitz
                            AUSTRIA_LE,   // Leoben
                            AUSTRIA_LI,   // Liezen
                            AUSTRIA_LN,   // Leoben-Land
                            AUSTRIA_MU,   // Murau
                            AUSTRIA_MZ,   // Mürzzuschlag
                            AUSTRIA_RA,   // Radkersburg
                            AUSTRIA_VO,   // Voitsberg
                            AUSTRIA_WZ,   // Weiz
                            AUSTRIA_IC,   // Innsbruck
                            AUSTRIA_IL,   // Innsbruck-Land
                            AUSTRIA_IM,   // Imst
                            AUSTRIA_KB,   // Kitzbühel
                            AUSTRIA_KU,   // Kufstein
                            AUSTRIA_LA,   // Landeck
                            AUSTRIA_LZ,   // Lienz
                            AUSTRIA_RE,   // Reutte
                            AUSTRIA_SZ,   // Schwaz
                            AUSTRIA_FE,   // Feldkirchen
                            AUSTRIA_HE,   // Hermagor
                            AUSTRIA_KC,   // Klagenfurt
                            AUSTRIA_KL,   // Klagenfurt-Land
                            AUSTRIA_SP,   // Spittal/Drau
                            AUSTRIA_SV,   // St.Veit/Glan
                            AUSTRIA_VI,   // Villach
                            AUSTRIA_VK,   // Völkermarkt
                            AUSTRIA_VL,   // Villach-Land
                            AUSTRIA_WO,   // Wolfsberg
                            AUSTRIA_BC,   // Bregenz
                            AUSTRIA_BZ,   // Bludenz
                            AUSTRIA_DO,   // Dornbirn
                            AUSTRIA_FK,   //Feldkirch
                            N_AUSTRIA_PRIMARIES
                          };

typedef std::array<std::string, N_AUSTRIA_PRIMARIES> PRIMARY_AUSTRIA_ENUMERATION_TYPE;    ///< primaries for Austria

static PRIMARY_AUSTRIA_ENUMERATION_TYPE PRIMARY_AUSTRIA_ENUMERATION = { { "WC",   // Wien
                                                                          "HA",   // Hallein
                                                                          "JO",   // St. Johann
                                                                          "SC",   // Salzburg
                                                                          "SL",   // Salzburg-Land
                                                                          "TA",   // Tamsweg
                                                                          "ZE",   // Zell Am See
                                                                          "AM",   // Amstetten
                                                                          "BL",   // Bruck/Leitha
                                                                          "BN",   // Baden
                                                                          "GD",   // Gmünd
                                                                          "GF",   // Gänserndorf
                                                                          "HL",   // Hollabrunn
                                                                          "HO",   // Horn
                                                                          "KO",   // Korneuburg
                                                                          "KR",   // Krems-Region
                                                                          "KS",   // Krems
                                                                          "LF",   // Lilienfeld
                                                                          "MD",   // Mödling
                                                                          "ME",   // Melk
                                                                          "MI",   // Mistelbach
                                                                          "NK",   // Neunkirchen
                                                                          "PC",   // St. Pölten
                                                                          "PL",   // St. Pölten-Land
                                                                          "SB",   // Scheibbs
                                                                          "SW",   // Schwechat
                                                                          "TU",   // Tulln
                                                                          "WB",   // Wr.Neustadt-Bezirk
                                                                          "WN",   // Wr.Neustadt
                                                                          "WT",   // Waidhofen/Thaya
                                                                          "WU",   // Wien-Umgebung
                                                                          "WY",   // Waidhofen/Ybbs
                                                                          "ZT",   // Zwettl
                                                                          "EC",   // Eisenstadt
                                                                          "EU",   // Eisenstadt-Umgebung
                                                                          "GS",   // Güssing
                                                                          "JE",   // Jennersdorf
                                                                          "MA",   // Mattersburg
                                                                          "ND",   // Neusiedl/See
                                                                          "OP",   // Oberpullendorf
                                                                          "OW",   // Oberwart
                                                                          "BR",   // Braunau/Inn
                                                                          "EF",   // Eferding
                                                                          "FR",   // Freistadt
                                                                          "GM",   // Gmunden
                                                                          "GR",   // Grieskirchen
                                                                          "KI",   // Kirchdorf
                                                                          "LC",   // Linz
                                                                          "LL",   // Linz-Land
                                                                          "PE",   // Perg
                                                                          "RI",   // Ried/Innkreis
                                                                          "RO",   // Rohrbach
                                                                          "SD",   // Schärding
                                                                          "SE",   // Steyr-Land
                                                                          "SR",   // Steyr
                                                                          "UU",   // Urfahr
                                                                          "VB",   // Vöcklabruck
                                                                          "WE",   // Wels
                                                                          "WL",   // Wels-Land
                                                                          "BA",   // Bad Aussee
                                                                          "BM",   // Bruck/Mur
                                                                          "DL",   // Deutschlandsberg
                                                                          "FB",   // Feldbach
                                                                          "FF",   // Fürstenfeld
                                                                          "GB",   // Gröbming
                                                                          "GC",   // Graz
                                                                          "GU",   // Graz-Umgebung
                                                                          "HB",   // Hartberg
                                                                          "JU",   // Judenburg
                                                                          "KF",   // Knittelfeld
                                                                          "LB",   // Leibnitz
                                                                          "LE",   // Leoben
                                                                          "LI",   // Liezen
                                                                          "LN",   // Leoben-Land
                                                                          "MU",   // Murau
                                                                          "MZ",   // Mürzzuschlag
                                                                          "RA",   // Radkersburg
                                                                          "VO",   // Voitsberg
                                                                          "WZ",   // Weiz
                                                                          "IC",   // Innsbruck
                                                                          "IL",   // Innsbruck-Land
                                                                          "IM",   // Imst
                                                                          "KB",   // Kitzbühel
                                                                          "KU",   // Kufstein
                                                                          "LA",   // Landeck
                                                                          "LZ",   // Lienz
                                                                          "RE",   // Reutte
                                                                          "SZ",   // Schwaz
                                                                          "FE",   // Feldkirchen
                                                                          "HE",   // Hermagor
                                                                          "KC",   // Klagenfurt
                                                                          "KL",   // Klagenfurt-Land
                                                                          "SP",   // Spittal/Drau
                                                                          "SV",   // St.Veit/Glan
                                                                          "VI",   // Villach
                                                                          "VK",   // Völkermarkt
                                                                          "VL",   // Villach-Land
                                                                          "WO",   // Wolfsberg
                                                                          "BC",   // Bregenz
                                                                          "BZ",   // Bludenz
                                                                          "DO",   // Dornbirn
                                                                          "FK"    //Feldkirch
                                                                      } };

/// Belgium
enum PRIMARY_ENUM_BELGIUM { BELGIUM_AN,     // Antwerpen
                            BELGIUM_BR,     // Brussels
                            BELGIUM_BW,     // Brabant Wallon
                            BELGIUM_HT,     // Hainaut
                            BELGIUM_LB,     // Limburg
                            BELGIUM_LG,     // Liêge
                            BELGIUM_NM,     // Namur
                            BELGIUM_LU,     // Luxembourg
                            BELGIUM_OV,     // Oost-Vlaanderen
                            BELGIUM_VB,     // Vlaams Brabant
                            BELGIUM_WZ,     // West-Vlaanderen
                            N_BELGIUM_PRIMARIES
                         };

typedef std::array<std::string, N_BELGIUM_PRIMARIES> PRIMARY_BELGIUM_ENUMERATION_TYPE;    ///< primaries for Belgium

static PRIMARY_BELGIUM_ENUMERATION_TYPE PRIMARY_BELGIUM_ENUMERATION = { { "AN",     // Antwerpen
                                                                          "BR",     // Brussels
                                                                          "BW",     // Brabant Wallon
                                                                          "HT",     // Hainaut
                                                                          "LB",     // Limburg
                                                                          "LG",     // Liêge
                                                                          "NM",     // Namur
                                                                          "LU",     // Luxembourg
                                                                          "OV",     // Oost-Vlaanderen
                                                                          "VB",     // Vlaams Brabant
                                                                          "WZ"     // West-Vlaanderen
                                                                      } };

/// Bulgaria
enum PRIMARY_ENUM_BULGARIA { BULGARIA_BU,   // Burgas
                             BULGARIA_SL,   // Sliven
                             BULGARIA_YA,   // Yambol (Jambol)
                             BULGARIA_SO,   // Sofija Grad
                             BULGARIA_HA,   // Haskovo
                             BULGARIA_KA,   // Kărdžali
                             BULGARIA_SZ,   // Stara Zagora
                             BULGARIA_PA,   // Pazardžik
                             BULGARIA_PD,   // Plovdiv
                             BULGARIA_SM,   // Smoljan
                             BULGARIA_BL,   // Blagoevgrad
                             BULGARIA_KD,   // Kjustendil
                             BULGARIA_PK,   // Pernik
                             BULGARIA_SF,   // Sofija (Sofia)
                             BULGARIA_GA,   // Gabrovo
                             BULGARIA_LV,   // Loveč (Lovech)
                             BULGARIA_PL,   // Pleven
                             BULGARIA_VT,   // Veliko Tărnovo
                             BULGARIA_MN,   // Montana
                             BULGARIA_VD,   // Vidin
                             BULGARIA_VR,   // Vraca
                             BULGARIA_RZ,   // Razgrad
                             BULGARIA_RS,   // Ruse
                             BULGARIA_SS,   // Silistra
                             BULGARIA_TA,   // Tărgovište
                             BULGARIA_DO,   // Dobrič
                             BULGARIA_SN,   // Šumen
                             BULGARIA_VN,   //Varna
                             N_BULGARIA_PRIMARIES
                           };

typedef std::array<std::string, N_BULGARIA_PRIMARIES> PRIMARY_BULGARIA_ENUMERATION_TYPE;    ///< primaries for Bulgaria

static PRIMARY_BULGARIA_ENUMERATION_TYPE PRIMARY_BULGARIA_ENUMERATION = { { "BU",   // Burgas
                                                                            "SL",   // Sliven
                                                                            "YA",   // Yambol (Jambol)
                                                                            "SO",   // Sofija Grad
                                                                            "HA",   // Haskovo
                                                                            "KA",   // Kărdžali
                                                                            "SZ",   // Stara Zagora
                                                                            "PA",   // Pazardžik
                                                                            "PD",   // Plovdiv
                                                                            "SM",   // Smoljan
                                                                            "BL",   // Blagoevgrad
                                                                            "KD",   // Kjustendil
                                                                            "PK",   // Pernik
                                                                            "SF",   // Sofija (Sofia)
                                                                            "GA",   // Gabrovo
                                                                            "LV",   // Loveč (Lovech)
                                                                            "PL",   // Pleven
                                                                            "VT",   // Veliko Tărnovo
                                                                            "MN",   // Montana
                                                                            "VD",   // Vidin
                                                                            "VR",   // Vraca
                                                                            "RZ",   // Razgrad
                                                                            "RS",   // Ruse
                                                                            "SS",   // Silistra
                                                                            "TA",   // Tărgovište
                                                                            "DO",   // Dobrič
                                                                            "SN",   // Šumen
                                                                            "VN"    //Varna
                                                                        } };

/// Corsica
enum PRIMARY_ENUM_CORSICA { CORSICA_2A, // Corse-du-Sud
                            CORSICA_2B, // Haute-Corse
                            N_CORSICA_PRIMARIES
                          };

typedef std::array<std::string, N_CORSICA_PRIMARIES> PRIMARY_CORSICA_ENUMERATION_TYPE;    ///< primaries for Corsica

static PRIMARY_CORSICA_ENUMERATION_TYPE PRIMARY_CORSICA_ENUMERATION = { { "2A", // Corse-du-Sud
                                                                          "2B"  // Haute-Corse
                                                                      } };

/// Denmark
enum PRIMARY_ENUM_DENMARK { DENMARK_015,  // Koebenhavns amt
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

typedef std::array<std::string, N_DENMARK_PRIMARIES> PRIMARY_DENMARK_ENUMERATION_TYPE;    ///< primaries for Denmark

static PRIMARY_DENMARK_ENUMERATION_TYPE PRIMARY_DENMARK_ENUMERATION = { { "015",  // Koebenhavns amt
                                                                          "020",  // Frederiksborg amt
                                                                          "025",  // Roskilde amt
                                                                          "030",  // Vestsjaellands amt
                                                                          "035",  // Storstrøm amt (Storstroems)
                                                                          "040",  // Bornholms amt
                                                                          "042",  // Fyns amt
                                                                          "050",  // Sínderjylland amt (Sydjyllands)
                                                                          "055",  // Ribe amt
                                                                          "060",  // Vejle amt
                                                                          "065",  // Ringkøbing amt (Ringkoebing)
                                                                          "070",  // Århus amt (Aarhus)
                                                                          "076",  // Viborg amt
                                                                          "080",  // Nordjyllands amt
                                                                          "101",  // Copenhagen City
                                                                          "147"   // Frederiksberg
                                                                      } };

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

typedef std::array<std::string, N_FINLAND_PRIMARIES> PRIMARY_FINLAND_ENUMERATION_TYPE;    ///< primaries for Finland

static PRIMARY_FINLAND_ENUMERATION_TYPE PRIMARY_FINLAND_ENUMERATION = { { "100",    // Somero
                                                                          "102",    // Alastaro
                                                                          "103",    // Askainen
                                                                          "104",    // Aura
                                                                          "105",    // Dragsfjärd
                                                                          "106",    // Eura
                                                                          "107",    // Eurajoki
                                                                          "108",    // Halikko
                                                                          "109",    // Harjavalta
                                                                          "110",    // Honkajoki
                                                                          "111",    // Houtskari
                                                                          "112",    // Huittinen
                                                                          "115",    // Iniö
                                                                          "116",    // Jämijärvi
                                                                          "117",    // Kaarina
                                                                          "119",    // Kankaanpää
                                                                          "120",    // Karinainen
                                                                          "122",    // Karvia
                                                                          "123",    // Äetsä
                                                                          "124",    // Kemiö
                                                                          "126",    // Kiikala
                                                                          "128",    // Kiikoinen
                                                                          "129",    // Kisko
                                                                          "130",    // Kiukainen
                                                                          "131",    // Kodisjoki
                                                                          "132",    // Kokemäki
                                                                          "133",    // Korppoo
                                                                          "134",    // Koski tl
                                                                          "135",    // Kullaa
                                                                          "136",    // Kustavi
                                                                          "137",    // Kuusjoki
                                                                          "138",    // Köyliö
                                                                          "139",    // Laitila
                                                                          "140",    // Lappi
                                                                          "141",    // Lavia
                                                                          "142",    // Lemu
                                                                          "143",    // Lieto
                                                                          "144",    // Loimaa
                                                                          "145",    // Loimaan kunta
                                                                          "147",    // Luvia
                                                                          "148",    // Marttila
                                                                          "149",    // Masku
                                                                          "150",    // Mellilä
                                                                          "151",    // Merikarvia
                                                                          "152",    // Merimasku
                                                                          "154",    // Mietoinen
                                                                          "156",    // Muurla
                                                                          "157",    // Mynämäki
                                                                          "158",    // Naantali
                                                                          "159",    // Nakkila
                                                                          "160",    // Nauvo
                                                                          "161",    // Noormarkku
                                                                          "162",    // Nousiainen
                                                                          "163",    // Oripää
                                                                          "164",    // Paimio
                                                                          "165",    // Parainen
                                                                          "167",    // Perniö
                                                                          "168",    // Pertteli
                                                                          "169",    // Piikkiö
                                                                          "170",    // Pomarkku
                                                                          "171",    // Pori
                                                                          "172",    // Punkalaidun
                                                                          "173",    // Pyhäranta
                                                                          "174",    // Pöytyä
                                                                          "175",    // Raisio
                                                                          "176",    // Rauma
                                                                          "178",    // Rusko
                                                                          "179",    // Rymättylä
                                                                          "180",    // Salo
                                                                          "181",    // Sauvo
                                                                          "182",    // Siikainen
                                                                          "183",    // Suodenniemi
                                                                          "184",    // Suomusjärvi
                                                                          "185",    // Säkylä
                                                                          "186",    // Särkisalo
                                                                          "187",    // Taivassalo
                                                                          "188",    // Tarvasjoki
                                                                          "189",    // Turku
                                                                          "190",    // Ulvila
                                                                          "191",    // Uusikaupunki
                                                                          "192",    // Vahto
                                                                          "193",    // Vammala
                                                                          "194",    // Vampula
                                                                          "195",    // Vehmaa
                                                                          "196",    // Velkua
                                                                          "198",    // Västanfjärd
                                                                          "199",    // Yläne
                                                                          "201",    // Artjärvi
                                                                          "202",    // Askola
                                                                          "204",    // Espoo
                                                                          "205",    // Hanko
                                                                          "206",    // Helsinki
                                                                          "207",    // Hyvinkää
                                                                          "208",    // Inkoo
                                                                          "209",    // Järvenpää
                                                                          "210",    // Karjaa
                                                                          "211",    // Karjalohja
                                                                          "212",    // Karkkila
                                                                          "213",    // Kauniainen
                                                                          "214",    // Kerava
                                                                          "215",    // Kirkkonummi
                                                                          "216",    // Lapinjärvi
                                                                          "217",    // Liljendal
                                                                          "218",    // Lohjan kaupunki
                                                                          "220",    // Loviisa
                                                                          "221",    // Myrskylä
                                                                          "222",    // Mäntsälä
                                                                          "223",    // Nummi-Pusula
                                                                          "224",    // Nurmijärvi
                                                                          "225",    // Orimattila
                                                                          "226",    // Pernaja
                                                                          "227",    // Pohja
                                                                          "228",    // Pornainen
                                                                          "229",    // Porvoo
                                                                          "231",    // Pukkila
                                                                          "233",    // Ruotsinpyhtää
                                                                          "234",    // Sammatti
                                                                          "235",    // Sipoo
                                                                          "236",    // Siuntio
                                                                          "238",    // Tammisaari
                                                                          "241",    // Tuusula
                                                                          "242",    // Vantaa
                                                                          "243",    // Vihti
                                                                          "301",    // Asikkala
                                                                          "303",    // Forssa
                                                                          "304",    // Hattula
                                                                          "305",    // Hauho
                                                                          "306",    // Hausjärvi
                                                                          "307",    // Hollola
                                                                          "308",    // Humppila
                                                                          "309",    // Hämeenlinna
                                                                          "310",    // Janakkala
                                                                          "311",    // Jokioinen
                                                                          "312",    // Juupajoki
                                                                          "313",    // Kalvola
                                                                          "314",    // Kangasala
                                                                          "315",    // Hämeenkoski
                                                                          "316",    // Kuhmalahti
                                                                          "318",    // Kuru
                                                                          "319",    // Kylmäkoski
                                                                          "320",    // Kärkölä
                                                                          "321",    // Lahti
                                                                          "322",    // Lammi
                                                                          "323",    // Lempäälä
                                                                          "324",    // Loppi
                                                                          "325",    // Luopioinen
                                                                          "326",    // Längelmäki
                                                                          "327",    // Mänttä
                                                                          "328",    // Nastola
                                                                          "329",    // Nokia
                                                                          "330",    // Orivesi
                                                                          "331",    // Padasjoki
                                                                          "332",    // Pirkkala
                                                                          "333",    // Pälkäne
                                                                          "334",    // Renko
                                                                          "335",    // Riihimäki
                                                                          "336",    // Ruovesi
                                                                          "337",    // Sahalahti
                                                                          "340",    // Tammela
                                                                          "341",    // Tampere
                                                                          "342",    // Toijala
                                                                          "344",    // Tuulos
                                                                          "345",    // Urjala
                                                                          "346",    // Valkeakoski
                                                                          "347",    // Vesilahti
                                                                          "348",    // Viiala
                                                                          "349",    // Vilppula
                                                                          "350",    // Virrat
                                                                          "351",    // Ylöjärvi
                                                                          "352",    // Ypäjä
                                                                          "353",    // Hämeenkyrö
                                                                          "354",    // Ikaalinen
                                                                          "355",    // Kihniö
                                                                          "356",    // Mouhijärvi
                                                                          "357",    // Parkano
                                                                          "358",    // Viljakkala
                                                                          "402",    // Enonkoski
                                                                          "403",    // Hartola
                                                                          "404",    // Haukivuori
                                                                          "405",    // Heinola
                                                                          "407",    // Heinävesi
                                                                          "408",    // Hirvensalmi
                                                                          "409",    // Joroinen
                                                                          "410",    // Juva
                                                                          "411",    // Jäppilä
                                                                          "412",    // Kangaslampi
                                                                          "413",    // Kangasniemi
                                                                          "414",    // Kerimäki
                                                                          "415",    // Mikkeli
                                                                          "417",    // Mäntyharju
                                                                          "418",    // Pertunmaa
                                                                          "419",    // Pieksämäki
                                                                          "420",    // Pieksänmaa
                                                                          "421",    // Punkaharju
                                                                          "422",    // Puumala
                                                                          "423",    // Rantasalmi
                                                                          "424",    // Ristiina
                                                                          "425",    // Savonlinna
                                                                          "426",    // Savonranta
                                                                          "427",    // Sulkava
                                                                          "428",    // Sysmä
                                                                          "502",    // Elimäki
                                                                          "503",    // Hamina
                                                                          "504",    // Iitti
                                                                          "505",    // Imatra
                                                                          "506",    // Jaala
                                                                          "507",    // Joutseno
                                                                          "509",    // Kotka
                                                                          "510",    // Kouvola
                                                                          "511",    // Kuusankoski
                                                                          "513",    // Lappeenranta
                                                                          "514",    // Lemi
                                                                          "515",    // Luumäki
                                                                          "516",    // Miehikkälä
                                                                          "518",    // Parikkala
                                                                          "519",    // Pyhtää
                                                                          "520",    // Rautjärvi
                                                                          "521",    // Ruokolahti
                                                                          "522",    // Saari
                                                                          "523",    // Savitaipale
                                                                          "525",    // Suomenniemi
                                                                          "526",    // Taipalsaari
                                                                          "527",    // Uukuniemi
                                                                          "528",    // Valkeala
                                                                          "530",    // Virolahti
                                                                          "531",    // Ylämaa
                                                                          "532",    // Anjalankoski
                                                                          "601",    // Alahärmä
                                                                          "602",    // Alajärvi
                                                                          "603",    // Alavus
                                                                          "604",    // Evijärvi
                                                                          "605",    // Halsua
                                                                          "606",    // Hankasalmi
                                                                          "607",    // Himanka
                                                                          "608",    // Ilmajoki
                                                                          "609",    // Isojoki
                                                                          "610",    // Isokyrö
                                                                          "611",    // Jalasjärvi
                                                                          "612",    // Joutsa
                                                                          "613",    // Jurva
                                                                          "614",    // Jyväskylä
                                                                          "615",    // Jyväskylän mlk
                                                                          "616",    // Jämsä
                                                                          "617",    // Jämsänkoski
                                                                          "619",    // Kannonkoski
                                                                          "620",    // Kannus
                                                                          "621",    // Karijoki
                                                                          "622",    // Karstula
                                                                          "623",    // Kaskinen
                                                                          "624",    // Kauhajoki
                                                                          "625",    // Kauhava
                                                                          "626",    // Kaustinen
                                                                          "627",    // Keuruu
                                                                          "628",    // Kinnula
                                                                          "629",    // Kivijärvi
                                                                          "630",    // Kokkola
                                                                          "632",    // Konnevesi
                                                                          "633",    // Korpilahti
                                                                          "634",    // Korsnäs
                                                                          "635",    // Kortesjärvi
                                                                          "636",    // Kristiinankaupunki
                                                                          "637",    // Kruunupyy
                                                                          "638",    // Kuhmoinen
                                                                          "639",    // Kuortane
                                                                          "640",    // Kurikka
                                                                          "641",    // Kyyjärvi
                                                                          "642",    // Kälviä
                                                                          "643",    // Laihia
                                                                          "644",    // Lappajärvi
                                                                          "645",    // Lapua
                                                                          "646",    // Laukaa
                                                                          "647",    // Lehtimäki
                                                                          "648",    // Leivonmäki
                                                                          "649",    // Lestijärvi
                                                                          "650",    // Lohtaja
                                                                          "651",    // Luhanka
                                                                          "652",    // Luoto
                                                                          "653",    // Maalahti
                                                                          "654",    // Maksamaa
                                                                          "655",    // Multia
                                                                          "656",    // Mustasaari
                                                                          "657",    // Muurame
                                                                          "658",    // Nurmo
                                                                          "659",    // Närpiö
                                                                          "660",    // Oravainen
                                                                          "661",    // Perho
                                                                          "662",    // Peräseinäjoki
                                                                          "663",    // Petäjävesi
                                                                          "664",    // Pietarsaari
                                                                          "665",    // Pedersöre
                                                                          "666",    // Pihtipudas
                                                                          "668",    // Pylkönmäki
                                                                          "669",    // Saarijärvi
                                                                          "670",    // Seinäjoki
                                                                          "671",    // Soini
                                                                          "672",    // Sumiainen
                                                                          "673",    // Suolahti
                                                                          "675",    // Teuva
                                                                          "676",    // Toholampi
                                                                          "677",    // Toivakka
                                                                          "678",    // Töysä
                                                                          "679",    // Ullava
                                                                          "680",    // Uurainen
                                                                          "681",    // Uusikaarlepyy
                                                                          "682",    // Vaasa
                                                                          "683",    // Veteli
                                                                          "684",    // Viitasaari
                                                                          "685",    // Vimpeli
                                                                          "686",    // Vähäkyrö
                                                                          "687",    // Vöyri
                                                                          "688",    // Ylihärmä
                                                                          "689",    // Ylistaro
                                                                          "690",    // Ähtäri
                                                                          "692",    // Äänekoski
                                                                          "701",    // Eno
                                                                          "702",    // Iisalmi
                                                                          "703",    // Ilomantsi
                                                                          "704",    // Joensuu
                                                                          "705",    // Juankoski
                                                                          "706",    // Juuka
                                                                          "707",    // Kaavi
                                                                          "708",    // Karttula
                                                                          "709",    // Keitele
                                                                          "710",    // Kesälahti
                                                                          "711",    // Kiihtelysvaara
                                                                          "712",    // Kitee
                                                                          "713",    // Kiuruvesi
                                                                          "714",    // Kontiolahti
                                                                          "715",    // Kuopio
                                                                          "716",    // Lapinlahti
                                                                          "717",    // Leppävirta
                                                                          "718",    // Lieksa
                                                                          "719",    // Liperi
                                                                          "720",    // Maaninka
                                                                          "721",    // Nilsiä
                                                                          "722",    // Nurmes
                                                                          "723",    // Outokumpu
                                                                          "724",    // Pielavesi
                                                                          "725",    // Polvijärvi
                                                                          "726",    // Pyhäselkä
                                                                          "727",    // Rautalampi
                                                                          "728",    // Rautavaara
                                                                          "729",    // Rääkkylä
                                                                          "730",    // Siilinjärvi
                                                                          "731",    // Sonkajärvi
                                                                          "732",    // Suonenjoki
                                                                          "733",    // Tervo
                                                                          "734",    // Tohmajärvi
                                                                          "735",    // Tuupovaara
                                                                          "736",    // Tuusniemi
                                                                          "737",    // Valtimo
                                                                          "738",    // Varkaus
                                                                          "739",    // Varpaisjärvi
                                                                          "740",    // Vehmersalmi
                                                                          "741",    // Vesanto
                                                                          "742",    // Vieremä
                                                                          "743",    // Värtsilä
                                                                          "801",    // Alavieska
                                                                          "802",    // Haapajärvi
                                                                          "803",    // Haapavesi
                                                                          "804",    // Hailuoto
                                                                          "805",    // Haukipudas
                                                                          "806",    // Hyrynsalmi
                                                                          "807",    // Ii
                                                                          "808",    // Kajaani
                                                                          "810",    // Kalajoki
                                                                          "811",    // Kempele
                                                                          "812",    // Kestilä
                                                                          "813",    // Kiiminki
                                                                          "814",    // Kuhmo
                                                                          "815",    // Kuivaniemi
                                                                          "816",    // Kuusamo
                                                                          "817",    // Kärsämäki
                                                                          "818",    // Liminka
                                                                          "819",    // Lumijoki
                                                                          "820",    // Merijärvi
                                                                          "821",    // Muhos
                                                                          "822",    // Nivala
                                                                          "823",    // Oulainen
                                                                          "824",    // Oulu
                                                                          "825",    // Oulunsalo
                                                                          "826",    // Paltamo
                                                                          "827",    // Pattijoki
                                                                          "828",    // Piippola
                                                                          "829",    // Pudasjärvi
                                                                          "830",    // Pulkkila
                                                                          "831",    // Puolanka
                                                                          "832",    // Pyhäjoki
                                                                          "833",    // Pyhäjärvi
                                                                          "834",    // Pyhäntä
                                                                          "835",    // Raahe
                                                                          "836",    // Rantsila
                                                                          "837",    // Reisjärvi
                                                                          "838",    // Ristijärvi
                                                                          "839",    // Ruukki
                                                                          "840",    // Sievi
                                                                          "841",    // Siikajoki
                                                                          "842",    // Sotkamo
                                                                          "843",    // Suomussalmi
                                                                          "844",    // Taivalkoski
                                                                          "846",    // Tyrnävä
                                                                          "847",    // Utajärvi
                                                                          "848",    // Vaala
                                                                          "849",    // Vihanti
                                                                          "850",    // Vuolijoki
                                                                          "851",    // Yli-Ii
                                                                          "852",    // Ylikiiminki
                                                                          "853",    // Ylivieska
                                                                          "901",    // Enontekiö
                                                                          "902",    // Inari
                                                                          "903",    // Kemi
                                                                          "904",    // Keminmaa
                                                                          "905",    // Kemijärvi
                                                                          "907",    // Kittilä
                                                                          "908",    // Kolari
                                                                          "909",    // Muonio
                                                                          "910",    // Pelkosenniemi
                                                                          "911",    // Pello
                                                                          "912",    // Posio
                                                                          "913",    // Ranua
                                                                          "914",    // Rovaniemi
                                                                          "915",    // Rovaniemen mlk
                                                                          "916",    // Salla
                                                                          "917",    // Savukoski
                                                                          "918",    // Simo
                                                                          "919",    // Sodankylä
                                                                          "920",    // Tervola
                                                                          "921",    // Tornio
                                                                          "922",    // Utsjoki
                                                                          "923"     // Ylitornio
                                                                      } };

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

typedef std::array<std::string, N_SARDINIA_PRIMARIES> PRIMARY_SARDINIA_ENUMERATION_TYPE;    ///< primaries for Sardinia

static PRIMARY_SARDINIA_ENUMERATION_TYPE PRIMARY_SARDINIA_ENUMERATION = { { "CA",   // Cagliari
                                                                            "CI",   //  Carbonia Iglesias
                                                                            "MD",   //  Medio Campidano (deprecated)
                                                                            "NU",   // Nuoro
                                                                            "OG",   // Ogliastra
                                                                            "OR",   // Oristano
                                                                            "OT",   // Olbia Tempio
                                                                            "SS",   // Sassari
                                                                            "VS",   // Medio Campidano
                                                                        } };

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

typedef std::array<std::string, N_FRANCE_PRIMARIES> PRIMARY_FRANCE_ENUMERATION_TYPE;    ///< primaries for France

static PRIMARY_FRANCE_ENUMERATION_TYPE PRIMARY_FRANCE_ENUMERATION = { { "01", // Ain
                                                                        "02", // Aisne
                                                                        "03", // Allier
                                                                        "04", // Alpes-de-Haute-Provence
                                                                        "05", // Hautes-Alpes
                                                                        "06", // Alpes-Maritimes
                                                                        "07", // Ardèche
                                                                        "08", // Ardennes
                                                                        "09", // Ariège
                                                                        "10", // Aube
                                                                        "11", // Aude
                                                                        "12", // Aveyron
                                                                        "13", // Bouches-du-Rhone
                                                                        "14", // Calvados
                                                                        "15", // Cantal
                                                                        "16", // Charente
                                                                        "17", // Charente-Maritime
                                                                        "18", // Cher
                                                                        "19", // Corrèze
                                                                        "21", // Cote-d'Or
                                                                        "22", // Cotes-d'Armor
                                                                        "23", // Creuse
                                                                        "24", // Dordogne
                                                                        "25", // Doubs
                                                                        "26", // Drôme
                                                                        "27", // Eure
                                                                        "28", // Eure-et-Loir
                                                                        "29", // Finistère
                                                                        "30", // Gard
                                                                        "31", // Haute-Garonne
                                                                        "32", // Gere
                                                                        "33", // Gironde
                                                                        "34", // Hérault
                                                                        "35", // Ille-et-Vilaine
                                                                        "36", // Indre
                                                                        "37", // Indre-et-Loire
                                                                        "38", // Isère
                                                                        "39", // Jura
                                                                        "40", // Landes
                                                                        "41", // Loir-et-Cher
                                                                        "42", // Loire
                                                                        "43", // Haute-Loire
                                                                        "44", // Loire-Atlantique
                                                                        "45", // Loiret
                                                                        "46", // Lot
                                                                        "47", // Lot-et-Garonne
                                                                        "48", // Lozère
                                                                        "49", // Maine-et-Loire
                                                                        "50", // Manche
                                                                        "51", // Marne
                                                                        "52", // Haute-Marne
                                                                        "53", // Mayenne
                                                                        "54", // Meurthe-et-Moselle
                                                                        "55", // Meuse
                                                                        "56", //Morbihan
                                                                        "57", // Moselle
                                                                        "58", // Niëvre
                                                                        "59", // Nord
                                                                        "60", // Oise
                                                                        "61", // Orne
                                                                        "62", // Pas-de-Calais
                                                                        "63", // Puy-de-Dôme
                                                                        "64", // Pyrénées-Atlantiques
                                                                        "65", // Hautea-Pyrénées
                                                                        "66", // Pyrénées-Orientales
                                                                        "67", // Bas-Rhin
                                                                        "68", // Haut-Rhin
                                                                        "69", // Rhône
                                                                        "70", // Haute-Saône
                                                                        "71", // Saône-et-Loire
                                                                        "72", // Sarthe
                                                                        "73", // Savoie
                                                                        "74", // Haute-Savoie
                                                                        "75", // Paris
                                                                        "76", // Seine-Maritime
                                                                        "77", // Seine-et-Marne
                                                                        "78", // Yvelines
                                                                        "79", // Deux-Sèvres
                                                                        "80", // Somme
                                                                        "81", // Tarn
                                                                        "82", // Tarn-et-Garonne
                                                                        "83", // Var
                                                                        "84", // Vaucluse
                                                                        "85", // Vendée
                                                                        "86", // Vienne
                                                                        "87", // Haute-Vienne
                                                                        "88", // Vosges
                                                                        "89", // Yonne
                                                                        "90", // Territoire de Belfort
                                                                        "91", // Essonne
                                                                        "92", // Hauts-de-Selne
                                                                        "93", // Seine-Saint-Denis
                                                                        "94", // Val-de-Marne
                                                                        "95"  // Val-d'Oise
                                                                    } };

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

typedef std::array<std::string, N_GERMANY_PRIMARIES> PRIMARY_GERMANY_ENUMERATION_TYPE;    ///< primaries for Germany

static PRIMARY_GERMANY_ENUMERATION_TYPE PRIMARY_GERMANY_ENUMERATION = { { "BB",   // Brandenburg
                                                                          "BE",   // Berlin
                                                                          "BW",   // Baden-Württemberg
                                                                          "BY",   // Freistaat Bayern
                                                                          "HB",   // Freie Hansestadt Bremen
                                                                          "HE",   // Hessen
                                                                          "HH",   // Freie und Hansestadt Hamburg
                                                                          "MV",   // Mecklenburg-Vorpommern
                                                                          "NI",   // Niedersachsen
                                                                          "NW",   // Nordrhein-Westfalen
                                                                          "RP",   // Rheinland-Pfalz
                                                                          "SL",   // Saarland
                                                                          "SH",   // Schleswig-Holstein
                                                                          "SN",   // Freistaat Sachsen
                                                                          "ST",   // Sachsen-Anhalt
                                                                          "TH"   // Freistaat Thüringen
                                                                      } };

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

typedef std::array<std::string, N_HUNGARY_PRIMARIES> PRIMARY_HUNGARY_ENUMERATION_TYPE;    ///< primaries for Hungary

static PRIMARY_HUNGARY_ENUMERATION_TYPE PRIMARY_HUNGARY_ENUMERATION = { { "GY",   // Gyõr (Gyõr-Moson-Sopron)
                                                                          "VA",   // Vas
                                                                          "ZA",   // Zala
                                                                          "KO",   // Komárom (Komárom-Esztergom)
                                                                          "VE",   // Veszprém
                                                                          "BA",   // Baranya
                                                                          "SO",   // Somogy
                                                                          "TO",   // Tolna
                                                                          "FE",   // Fejér
                                                                          "BP",   // Budapest
                                                                          "HE",   // Heves
                                                                          "NG",   // Nógrád
                                                                          "PE",   // Pest
                                                                          "SZ",   // Szolnok (Jász-Nagykun-Szolnok)
                                                                          "BE",   // Békés
                                                                          "BN",   // Bács-Kiskun
                                                                          "CS",   // Csongrád
                                                                          "BO",   // Borsod (Borsod-Abaúj-Zemplén)
                                                                          "HB",   // Hajdú-Bihar
                                                                          "SA"   // Szabolcs (Szabolcs-Szatmár-Bereg)
                                                                      } };

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

typedef std::array<std::string, N_IRELAND_PRIMARIES> PRIMARY_IRELAND_ENUMERATION_TYPE;    ///< primaries for Ireland

static PRIMARY_IRELAND_ENUMERATION_TYPE PRIMARY_IRELAND_ENUMERATION = { { "CW",  // Carlow (Ceatharlach)
                                                                          "CN",  // Cavan (An Cabhán)
                                                                          "CE",  // Clare (An Clár)
                                                                          "C",   // Cork (Corcaigh)
                                                                          "DL",  // Donegal (Dún na nGall)
                                                                          "D",   // Dublin (Baile Áth Cliath)
                                                                          "G",   // Galway (Gaillimh)
                                                                          "KY",  // Kerry (Ciarraí)
                                                                          "KE",  // Kildare (Cill Dara)
                                                                          "KK",  // Kilkenny (Cill Chainnigh)
                                                                          "LS",  // Laois (Laois)
                                                                          "LM",  // Leitrim (Liatroim)
                                                                          "LK",  // Limerick (Luimneach)
                                                                          "LD",  // Longford (An Longfort)
                                                                          "LH",  // Louth (Lú)
                                                                          "MO",  // Mayo (Maigh Eo)
                                                                          "MH",  // Meath (An Mhí)
                                                                          "MN",  // Monaghan (Muineachán)
                                                                          "OY",  // Offaly (Uíbh Fhailí)
                                                                          "RN",  // Roscommon (Ros Comáin)
                                                                          "SO",  // Sligo (Sligeach)
                                                                          "TA",  // Tipperary (Tiobraid Árann)
                                                                          "WD",  // Waterford (Port Láirge)
                                                                          "WH",  // Westmeath (An Iarmhí)
                                                                          "WX",  // Wexford (Loch Garman)
                                                                          "WW"   // Wicklow (Cill Mhantáin)
                                                                      } };

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

typedef std::array<std::string, N_ITALY_PRIMARIES> PRIMARY_ITALY_ENUMERATION_TYPE;    ///< primaries for Italy

static PRIMARY_ITALY_ENUMERATION_TYPE PRIMARY_ITALY_ENUMERATION = { { "GE",   // Genova
                                                                      "IM",   // Imperia
                                                                      "SP",   // La Spezia
                                                                      "SV",   // Savona
                                                                      "AL",   // Alessandria
                                                                      "AT",   // Asti
                                                                      "BI",   // Biella
                                                                      "CN",   // Cuneo
                                                                      "NO",   // Novara
                                                                      "TO",   // Torino
                                                                      "VB",   // Verbano Cusio Ossola
                                                                      "VC",   // Vercelli
                                                                      "AO",   // Aosta
                                                                      "BG",   // Bergamo
                                                                      "BS",   // Brescia
                                                                      "CO",   // Como
                                                                      "CR",   // Cremona
                                                                      "LC",   // Lecco
                                                                      "LO",   // Lodi
                                                                      "MB",   // Monza e Brianza
                                                                      "MN",   // Mantova
                                                                      "MI",   // Milano
                                                                      "PV",   // Pavia
                                                                      "SO",   // Sondrio
                                                                      "VA",   // Varese
                                                                      "BL",   // Belluno
                                                                      "PD",   // Padova
                                                                      "RO",   // Rovigo
                                                                      "TV",   // Treviso
                                                                      "VE",   // Venezia
                                                                      "VR",   // Verona
                                                                      "VI",   // Vicenza
                                                                      "BZ",   // Bolzano
                                                                      "TN",   // Trento
                                                                      "GO",   // Gorizia
                                                                      "PN",   // Pordenone
                                                                      "TS",   // Trieste
                                                                      "UD",   // Udine
                                                                      "BO",   // Bologna
                                                                      "FE",   // Ferrara
                                                                      "FO",   // Forli (Deprecated)
                                                                      "FC",   // Forli Cesena
                                                                      "MO",   // Modena
                                                                      "PR",   // Parma
                                                                      "PC",   // Piacenza
                                                                      "RA",   // Ravenna
                                                                      "RE",   // Reggio Emilia
                                                                      "RN",   // Rimini
                                                                      "AR",   // Arezzo
                                                                      "FI",   // Firenze
                                                                      "GR",   // Grosseto
                                                                      "LI",   // Livorno
                                                                      "LU",   // Lucca
                                                                      "MS",   // Massa Carrara
                                                                      "PT",   // Pistoia
                                                                      "PI",   // Pisa
                                                                      "PO",   // Prato
                                                                      "SI",   // Siena
                                                                      "CH",   // Chieti
                                                                      "AQ",   // L'Aquila
                                                                      "PE",   // Pescara
                                                                      "TE",   // Teramo
                                                                      "AN",   // Ancona
                                                                      "AP",   // Ascoli Piceno
                                                                      "FM",   // Fermo
                                                                      "MC",   // Macerata
                                                                      "PS",   // Pesaro e Urbino (Deprecated)
                                                                      "PU",   // Pesaro e Urbino
                                                                      "MT",   // Matera
                                                                      "BA",   // Bari
                                                                      "BT",   // Barletta Andria Trani
                                                                      "BR",   // Brindisi
                                                                      "FG",   // Foggia
                                                                      "LE",   // Lecce
                                                                      "TA",   // Taranto
                                                                      "PZ",   // Potenza
                                                                      "CZ",   // Catanzaro
                                                                      "CS",   // Cosenza
                                                                      "KR",   // Crotone
                                                                      "RC",   // Reggio Calabria
                                                                      "VV",   // Vibo Valentia
                                                                      "AV",   // Avellino
                                                                      "BN",   // Benevento
                                                                      "CE",   // Caserta
                                                                      "NA",   // Napoli
                                                                      "SA",   // Salerno
                                                                      "IS",   // Isernia
                                                                      "CB",   // Campobasso
                                                                      "FR",   // Frosinone
                                                                      "LT",   // Latina
                                                                      "RI",   // Rieti
                                                                      "RM",   // Roma
                                                                      "VT",   // Viterbo
                                                                      "PG",   // Perugia
                                                                      "TR",   // Terni
                                                                      "AG",   // Agrigento
                                                                      "CL",   // Caltanissetta
                                                                      "CT",   // Catania
                                                                      "EN",   // Enna
                                                                      "ME",   // Messina
                                                                      "PA",   // Palermo
                                                                      "RG",   // Ragusa
                                                                      "SR",   // Siracusa
                                                                      "TP"   // Trapani
                                                                  } };

/// Madeira
enum PRIMARY_ENUM_MADEIRA { MADEIRA_MD, // Madeira
                            N_MADEIRA_PRIMARIES
                          };

typedef std::array<std::string, N_MADEIRA_PRIMARIES> PRIMARY_MADEIRA_ENUMERATION_TYPE;    ///< primaries for Madeira

static PRIMARY_MADEIRA_ENUMERATION_TYPE PRIMARY_MADEIRA_ENUMERATION = { { "MD" // Madeira
                                                                      } };

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

typedef std::array<std::string, N_NETHERLANDS_PRIMARIES> PRIMARY_NETHERLANDS_ENUMERATION_TYPE;    ///< primaries for Netherlands

static PRIMARY_NETHERLANDS_ENUMERATION_TYPE PRIMARY_NETHERLANDS_ENUMERATION = { { "DR",   // Drenthe
                                                                                  "FR",   // Friesland
                                                                                  "GR",   // Groningen
                                                                                  "NB",   // Noord-Brabant
                                                                                  "OV",   // Overijssel
                                                                                  "ZH",   // Zuid-Holland
                                                                                  "FL",   // Flevoland
                                                                                  "GD",   // Gelderland
                                                                                  "LB",   // Limburg
                                                                                  "NH",   // Noord-Holland
                                                                                  "UT",   // Utrecht
                                                                                  "ZL"   // Zeeland
                                                                              } };

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

typedef std::array<std::string, N_POLAND_PRIMARIES> PRIMARY_POLAND_ENUMERATION_TYPE;    ///< primaries for Poland

static PRIMARY_POLAND_ENUMERATION_TYPE PRIMARY_POLAND_ENUMERATION = { { "Z",    // Zachodnio-Pomorskie
                                                                        "F",    // Pomorskie
                                                                        "P",    // Kujawsko-Pomorskie
                                                                        "B",    // Lubuskie
                                                                        "W",    // Wielkopolskie
                                                                        "J",    // Warminsko-Mazurskie
                                                                        "O",    // Podlaskie
                                                                        "R",    // Mazowieckie
                                                                        "D",    // Dolnoslaskie
                                                                        "U",    // Opolskie
                                                                        "C",    // Lodzkie
                                                                        "S",    // Swietokrzyskie
                                                                        "K",    // Podkarpackie
                                                                        "L",    // Lubelskie
                                                                        "G",    // Slaskie
                                                                        "M",    // Malopolskie
                                                                    } };

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

typedef std::array<std::string, N_PORTUGAL_PRIMARIES> PRIMARY_PORTUGAL_ENUMERATION_TYPE;    ///< primaries for Portugal

static PRIMARY_PORTUGAL_ENUMERATION_TYPE PRIMARY_PORTUGAL_ENUMERATION = { { "AV",   // Aveiro
                                                                            "BJ",   // Beja
                                                                            "BR",   // Braga
                                                                            "BG",   // Bragança
                                                                            "CB",   // Castelo Branco
                                                                            "CO",   // Coimbra
                                                                            "EV",   // Evora
                                                                            "FR",   // Faro
                                                                            "GD",   // Guarda
                                                                            "LR",   // Leiria
                                                                            "LX",   // Lisboa
                                                                            "PG",   // Portalegre
                                                                            "PT",   // Porto
                                                                            "SR",   // Santarem
                                                                            "ST",   // Setubal
                                                                            "VC",   // Viana do Castelo
                                                                            "VR",   // Vila Real
                                                                            "VS",   // Viseu
                                                                        } };

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

typedef std::array<std::string, N_ROMANIA_PRIMARIES> PRIMARY_ROMANIA_ENUMERATION_TYPE;    ///< primaries for Romania

static PRIMARY_ROMANIA_ENUMERATION_TYPE PRIMARY_ROMANIA_ENUMERATION = { { "AR", //  Arad
                                                                          "CS",   // Cara'-Severin
                                                                          "HD",   // Hunedoara
                                                                          "TM",   // Timiş (Timis)
                                                                          "BU",   // Bucureşti (Bucure'ti)
                                                                          "IF",   // Ilfov
                                                                          "BR",   // Brăila (Braila)
                                                                          "CT",   // Conatarta
                                                                          "GL",   // Galati
                                                                          "TL",   // Tulcea
                                                                          "VN",   // Vrancea
                                                                          "AB",   // Alba
                                                                          "BH",   // Bihor
                                                                          "BN",   // Bistrita-Nasaud
                                                                          "CJ",   // Cluj
                                                                          "MM",   // Maramureş (Maramures)
                                                                          "SJ",   // Sălaj (Salaj)
                                                                          "SM",   // Satu Mare
                                                                          "BV",   // Braşov (Bra'ov)
                                                                          "CV",   // Covasna
                                                                          "HR",   // Harghita
                                                                          "MS",   // Mureş (Mures)
                                                                          "SB",   // Sibiu
                                                                          "AG",   // Arge'
                                                                          "DJ",   // Dolj
                                                                          "GJ",   // Gorj
                                                                          "MH",   // Mehedinţi (Mehedinti)
                                                                          "OT",   // Olt
                                                                          "VL",   // Vâlcea
                                                                          "BC",   // Bacau
                                                                          "BT",   // Boto'ani
                                                                          "IS",   // Iaşi (Iasi)
                                                                          "NT",   // Neamţ (Neamt)
                                                                          "SV",   // Suceava
                                                                          "VS",   // Vaslui
                                                                          "BZ",   // Buzău (Buzau)
                                                                          "CL",   // Călăraşi (Calarasi)
                                                                          "DB",   // Dâmboviţa (Dambovita)
                                                                          "GR",   // Giurqiu
                                                                          "IL",   // Ialomita
                                                                          "PH",   // Prahova
                                                                          "TR"    // Teleorman
                                                                      } };

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

typedef std::array<std::string, N_SPAIN_PRIMARIES> PRIMARY_SPAIN_ENUMERATION_TYPE;    ///< primaries for Spain

static PRIMARY_SPAIN_ENUMERATION_TYPE PRIMARY_SPAIN_ENUMERATION = { { "AV",   //  Avila
                                                                      "BU",   //  Burgos
                                                                      "C",    //  A Coruña
                                                                      "LE",   //  Leon
                                                                      "LO",   //  La Rioja
                                                                      "LU",   //  Lugo
                                                                      "O",    //  Asturias
                                                                      "OU",   //  Ourense
                                                                      "P",    //  Palencia
                                                                      "PO",   //  Pontevedra
                                                                      "S",    //  Cantabria
                                                                      "SA",   //  Salamanca
                                                                      "SG",   //  Segovia
                                                                      "SO",   //  Soria
                                                                      "VA",   //  Valladolid
                                                                      "ZA",   //  Zamora
                                                                      "BI",   //  Vizcaya
                                                                      "HU",   //  Huesca
                                                                      "NA",   //  Navarra
                                                                      "SS",   //  Guipuzcoa
                                                                      "TE",   //  Teruel
                                                                      "VI",   //  Alava
                                                                      "Z",    //  Zaragoza
                                                                      "B",    //  Barcelona
                                                                      "GI",   //  Girona
                                                                      "L",    //  Lleida
                                                                      "T",    //  Tarragona
                                                                      "BA",   //  Badajoz
                                                                      "CC",   //  Caceres
                                                                      "CR",   //  Ciudad Real
                                                                      "CU",   //  Cuenca
                                                                      "GU",   //  Guadalajara
                                                                      "M",    //  Madrid
                                                                      "TO",   //  Toledo
                                                                      "A",    //  Alicante
                                                                      "AB",   //  Albacete
                                                                      "CS",   //  Castellon
                                                                      "MU",   //  Murcia
                                                                      "V",    //  Valencia
                                                                      "AL",   //  Almeria
                                                                      "CA",   //  Cadiz
                                                                      "CO",   //  Cordoba
                                                                      "GR",   //  Granada
                                                                      "H",    //  Huelva
                                                                      "J",    //  Jaen
                                                                      "MA",   //  Malaga
                                                                      "SE"   //  Sevilla
                                                                  } };

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

typedef std::array<std::string, N_SWEDEN_PRIMARIES> PRIMARY_SWEDEN_ENUMERATION_TYPE;    ///< primaries for Sweden

static PRIMARY_SWEDEN_ENUMERATION_TYPE PRIMARY_SWEDEN_ENUMERATION = { { "AB",   //  Stockholm län
                                                                        "I",    //  Gotlands län
                                                                        "BD",   //  Norrbottens län
                                                                        "AC",   //  Västerbottens län
                                                                        "X",    //  Gävleborgs län
                                                                        "Z",    //  Jämtlands län
                                                                        "Y",    //  Västernorrlands län
                                                                        "W",    //  Dalarna län
                                                                        "S",    //  Värmlands län
                                                                        "O",    //  Västra Götalands län
                                                                        "T",    //  Örebro län
                                                                        "E",    //  Östergötlands län
                                                                        "D",    //  Södermanlands län
                                                                        "C",    //  Uppsala län
                                                                        "U",    //  Västmanlands län
                                                                        "N",    //  Hallands län
                                                                        "K",    //  Blekinge län
                                                                        "F",    //  Jönköpings län
                                                                        "H",    //  Kalmar län
                                                                        "G",    //  Kronobergs län
                                                                        "L"     //   Skåne län
                                                                    } };

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

typedef std::array<std::string, N_SWITZERLAND_PRIMARIES> PRIMARY_SWITZERLAND_ENUMERATION_TYPE;    ///< primaries for Switzerland

static PRIMARY_SWITZERLAND_ENUMERATION_TYPE PRIMARY_SWITZERLAND_ENUMERATION = { { "AG",   //  Aargau
                                                                                  "AR",   //  Appenzell Ausserrhoden
                                                                                  "AI",   //  Appenzell Innerrhoden
                                                                                  "BL",   //  Basel Landschaft
                                                                                  "BS",   //  Basel Stadt
                                                                                  "BE",   //  Bern
                                                                                  "FR",   //  Freiburg / Fribourg
                                                                                  "GE",   //  Genf / Genève
                                                                                  "GL",   //  Glarus
                                                                                  "GR",   //  Graubuenden / Grisons
                                                                                  "JU",   //  Jura
                                                                                  "LU",   //  Luzern
                                                                                  "NE",   //  Neuenburg / Neuchâtel
                                                                                  "NW",   //  Nidwalden
                                                                                  "OW",   //  Obwalden
                                                                                  "SH",   //  Schaffhausen
                                                                                  "SZ",   //  Schwyz
                                                                                  "SO",   //  Solothurn
                                                                                  "SG",   //  St. Gallen
                                                                                  "TI",   //  Tessin / Ticino
                                                                                  "TG",   //  Thurgau
                                                                                  "UR",   //  Uri
                                                                                  "VD",   //  Waadt / Vaud
                                                                                  "VS",   //  Wallis / Valais
                                                                                  "ZH",   //  Zuerich
                                                                                  "ZG"    //  Zug
                                                                              } };

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

typedef std::array<std::string, N_UKRAINE_PRIMARIES> PRIMARY_UKRAINE_ENUMERATION_TYPE;    ///< primaries for Ukraine

static PRIMARY_UKRAINE_ENUMERATION_TYPE PRIMARY_UKRAINE_ENUMERATION = { { "SU", //  Sums'ka Oblast'
                                                                          "TE",   //  Ternopil's'ka Oblast'
                                                                          "CH",   //  Cherkas'ka Oblast'
                                                                          "ZA",   //  Zakarpats'ka Oblast'
                                                                          "DN",   //  Dnipropetrovs'ka Oblast'
                                                                          "OD",   //  Odes'ka Oblast'
                                                                          "HE",   //  Khersons'ka Oblast'
                                                                          "PO",   //  Poltavs'ka Oblast'
                                                                          "DO",   //  Donets'ka Oblast'
                                                                          "RI",   //  Rivnens'ka Oblast'
                                                                          "HA",   //  Kharkivs'ka Oblast'
                                                                          "LU",   //  Luhans'ka Oblast'
                                                                          "VI",   //  Vinnyts'ka Oblast'
                                                                          "VO",   //  Volyos'ka Oblast'
                                                                          "ZP",   //  Zaporiz'ka Oblast'
                                                                          "CR",   //  Chernihivs'ka Oblast'
                                                                          "IF",   //  Ivano-Frankivs'ka Oblast'
                                                                          "HM",   //  Khmel'nyts'ka Oblast'
                                                                          "KV",   //  Kyïv
                                                                          "KO",   //  Kyivs'ka Oblast'
                                                                          "KI",   //  Kirovohrads'ka Oblast'
                                                                          "LV",   //  L'vivs'ka Oblast'
                                                                          "ZH",   //  Zhytomyrs'ka Oblast'
                                                                          "CN",   //  Chernivets'ka Oblast'
                                                                          "NI",   //  Mykolaivs'ka Oblast'
                                                                          "KR",   //  Respublika Krym
                                                                          "SL"    //  Sevastopol'
                                                                      } };

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

typedef std::array<std::string, N_UNITED_STATES_PRIMARIES> PRIMARY_UNITED_STATES_ENUMERATION_TYPE;    ///< primaries for United States

static PRIMARY_UNITED_STATES_ENUMERATION_TYPE PRIMARY_UNITED_STATES_ENUMERATION = { { "CT",   //  Connecticut
                                                                                      "ME",   //  Maine
                                                                                      "MA",   //  Massachusetts
                                                                                      "NH",   //  New Hampshire
                                                                                      "RI",   //  Rhode Island
                                                                                      "VT",   //  Vermont
                                                                                      "NJ",   //  New Jersey
                                                                                      "NY",   //  New York
                                                                                      "DE",   //  Delaware
                                                                                      "DC",   //  District of Columbia
                                                                                      "MD",   //  Maryland
                                                                                      "PA",   //  Pennsylvania
                                                                                      "AL",   //  Alabama
                                                                                      "FL",   //  Florida
                                                                                      "GA",   //  Georgia
                                                                                      "KY",   //  Kentucky
                                                                                      "NC",   //  North Carolina
                                                                                      "SC",   //  South Carolina
                                                                                      "TN",   //  Tennessee
                                                                                      "VA",   //  Virginia
                                                                                      "AR",   //  Arkansas
                                                                                      "LA",   //  Louisiana
                                                                                      "MS",   //  Mississippi
                                                                                      "NM",   //  New Mexico
                                                                                      "OK",   //  Oklahoma
                                                                                      "TX",   //  Texas
                                                                                      "CA",   //  California
                                                                                      "AZ",   //  Arizona
                                                                                      "ID",   //  Idaho
                                                                                      "MT",   //  Montana
                                                                                      "NV",   //  Nevada
                                                                                      "OR",   //  Oregon
                                                                                      "UT",   //  Utah
                                                                                      "WA",   //  Washington
                                                                                      "WY",   //  Wyoming
                                                                                      "MI",   //  Michigan
                                                                                      "OH",   //  Ohio
                                                                                      "WV",   //  West Virginia
                                                                                      "IL",   //  Illinois
                                                                                      "IN",   //  Indiana
                                                                                      "WI",   //  Wisconsin
                                                                                      "CO",   //  Colorado
                                                                                      "IA",   //  Iowa
                                                                                      "KS",   //  Kansas
                                                                                      "MN",   //  Minnesota
                                                                                      "MO",   //  Missouri
                                                                                      "NE",   //  Nebraska
                                                                                      "ND",   //  North Dakota
                                                                                      "SD"    //  South Dakota
                                                                                  } };

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

typedef std::array<std::string, N_JAPAN_PRIMARIES> PRIMARY_JAPAN_ENUMERATION_TYPE;    ///< primaries for Japan

static PRIMARY_JAPAN_ENUMERATION_TYPE PRIMARY_JAPAN_ENUMERATION = { { "12",   //  Chiba
                                                                      "16",   //  Gunma
                                                                      "14",   //  Ibaraki
                                                                      "11",   //  Kanagawa
                                                                      "13",   //  Saitama
                                                                      "15",   //  Tochigi
                                                                      "10",   //  Tokyo
                                                                      "17",   //  Yamanashi
                                                                      "20",   //  Aichi
                                                                      "19",   //  Gifu
                                                                      "21",   //  Mie
                                                                      "18",   //  Shizuoka
                                                                      "27",   //  Hyogo
                                                                      "22",   //  Kyoto
                                                                      "24",   //  Nara
                                                                      "25",   //  Osaka
                                                                      "23",   //  Shiga
                                                                      "26",   //  Wakayama
                                                                      "35",   //  Hiroshima
                                                                      "31",   //  Okayama
                                                                      "32",   //  Shimane
                                                                      "34",   //  Tottori
                                                                      "33",   //  Yamaguchi
                                                                      "38",   //  Ehime
                                                                      "36",   //  Kagawa
                                                                      "39",   //  Kochi
                                                                      "37",   //  Tokushima
                                                                      "40",   //  Fukuoka
                                                                      "46",   //  Kagoshima
                                                                      "43",   //  Kumamoto
                                                                      "45",   //  Miyazaki
                                                                      "42",   //  Nagasaki
                                                                      "44",   //  Oita
                                                                      "47",   //  Okinawa
                                                                      "41",   //  Saga
                                                                      "04",   //  Akita
                                                                      "02",   //  Aomori
                                                                      "07",   //  Fukushima
                                                                      "03",   //  Iwate
                                                                      "06",   //  Miyagi
                                                                      "05",   //  Yamagata
                                                                      "01",   //  Hokkaido
                                                                      "29",   //  Fukui
                                                                      "30",   //  Ishikawa
                                                                      "28",   //  Toyama
                                                                      "09",   //  Nagano
                                                                      "08"    //  Niigata
                                                                  } };

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

typedef std::array<std::string, N_PHILIPPINES_PRIMARIES> PRIMARY_PHILIPPINES_ENUMERATION_TYPE;    ///< primaries for Philippines

static PRIMARY_PHILIPPINES_ENUMERATION_TYPE PRIMARY_PHILIPPINES_ENUMERATION = { { "AUR",  //  Aurora
                                                                                  "BTG",  //  Batangas
                                                                                  "CAV",  //  Cavite
                                                                                  "LAG",  //  Laguna
                                                                                  "MAD",  //  Marinduque
                                                                                  "MDC",  //  Mindoro Occidental
                                                                                  "MDR",  //  Mindoro Oriental
                                                                                  "PLW",  //  Palawan
                                                                                  "QUE",  //  Quezon
                                                                                  "RIZ",  //  Rizal
                                                                                  "ROM",  //  Romblon
                                                                                  "ILN",  //  Ilocos Norte
                                                                                  "ILS",  //  Ilocos Sur
                                                                                  "LUN",  //  La Union
                                                                                  "PAN",  //  Pangasinan
                                                                                  "BTN",  //  Batanes
                                                                                  "CAG",  //  Cagayan
                                                                                  "ISA",  //  Isabela
                                                                                  "NUV",  //  Nueva Vizcaya
                                                                                  "QUI",  //  Quirino
                                                                                  "ABR",  //  Abra
                                                                                  "APA",  //  Apayao
                                                                                  "BEN",  //  Benguet
                                                                                  "IFU",  //  Ifugao
                                                                                  "KAL",  //  Kalinga-Apayso
                                                                                  "MOU",  //  Mountain Province
                                                                                  "BAN",  //  Batasn
                                                                                  "BUL",  //  Bulacan
                                                                                  "NUE",  //  Nueva Ecija
                                                                                  "PAM",  //  Pampanga
                                                                                  "TAR",  //  Tarlac
                                                                                  "ZMB",  //  Zambales
                                                                                  "ALB",  //  Albay
                                                                                  "CAN",  //  Camarines Norte
                                                                                  "CAS",  //  Camarines Sur
                                                                                  "CAT",  //  Catanduanes
                                                                                  "MAS",  //  Masbate
                                                                                  "SOR",  //  Sorsogon
                                                                                  "BIL",  //  Biliran
                                                                                  "EAS",  //  Eastern Samar
                                                                                  "LEY",  //  Leyte
                                                                                  "NSA",  //  Northern Samar
                                                                                  "SLE",  //  Southern Leyte
                                                                                  "WSA",  //  Western Samar
                                                                                  "AKL",  //  Aklan
                                                                                  "ANT",  //  Antique
                                                                                  "CAP",  //  Capiz
                                                                                  "GUI",  //  Guimaras
                                                                                  "ILI",  //  Iloilo
                                                                                  "NEC",  //  Negroe Occidental
                                                                                  "BOH",  //  Bohol
                                                                                  "CEB",  //  Cebu
                                                                                  "NER",  //  Negros Oriental
                                                                                  "SIG",  //  Siquijor
                                                                                  "ZAN",  //  Zamboanga del Norte
                                                                                  "ZAS",  //  Zamboanga del Sur
                                                                                  "ZSI",  //  Zamboanga Sibugay
                                                                                  "NCO",  //  North Cotabato
                                                                                  "SUK",  //  Sultan Kudarat
                                                                                  "SAR",  //  Sarangani
                                                                                  "SCO",  //  South Cotabato
                                                                                  "BAS",  //  Basilan
                                                                                  "LAS",  //  Lanao del Sur
                                                                                  "MAG",  //  Maguindanao
                                                                                  "SLU",  //  Sulu
                                                                                  "TAW",  //  Tawi-Tawi
                                                                                  "LAN",  //  Lanao del Norte
                                                                                  "BUK",  //  Bukidnon
                                                                                  "CAM",  //  Camiguin
                                                                                  "MSC",  //  Misamis Occidental
                                                                                  "MSR",  //  Misamis Oriental
                                                                                  "COM",  //  Compostela Valley
                                                                                  "DAV",  //  Davao del Norte
                                                                                  "DAS",  //  Davao del Sur
                                                                                  "DAO",  //  Davao Oriental
                                                                                  "AGN",  //  Agusan del Norte
                                                                                  "AGS",  //  Agusan del Sur
                                                                                  "SUN",  //  Surigao del Norte
                                                                                  "SUR"   //  Surigao del Sur
                                                                              } };

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

typedef std::array<std::string, N_CROATIA_PRIMARIES> PRIMARY_CROATIA_ENUMERATION_TYPE;    ///< primaries for Croatia

static PRIMARY_CROATIA_ENUMERATION_TYPE PRIMARY_CROATIA_ENUMERATION = { { "01",  //  Zagrebačka županija
                                                                          "02",  //  Krapinsko-Zagorska županija
                                                                          "03",  //  Sisačko-Moslavačka županija
                                                                          "04",  //  Karlovačka županija
                                                                          "05",  //  Varaždinska županija
                                                                          "06",  //  Koprivničko-Križevačka županija
                                                                          "07",  //  Bjelovarsko-Bilogorska županija
                                                                          "08",  //  Primorsko-Goranska županija
                                                                          "09",  //  Ličko-Senjska županija
                                                                          "10",  //  Virovitičko-Podravska županija
                                                                          "11",  //  Požeško-Slavonska županija
                                                                          "12",  //  Brodsko-Posavska županija
                                                                          "13",  //  Zadarska županija
                                                                          "14",  //  Osječko-Baranjska županija
                                                                          "15",  //  Šibensko-Kninska županija
                                                                          "16",  //  Vukovarsko-Srijemska županija
                                                                          "17",  //  Splitsko-Dalmatinska županija
                                                                          "18",  //  Istarska županija
                                                                          "19",  //  Dubrovačko-Neretvanska županija
                                                                          "20",  //  Međimurska županija
                                                                          "21"   //  Grad Zagreb
                                                                      } };

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

typedef std::array<std::string, N_CZECH_PRIMARIES> PRIMARY_CZECH_ENUMERATION_TYPE;    ///< primaries for Czech Republic

static PRIMARY_CZECH_ENUMERATION_TYPE PRIMARY_CZECH_ENUMERATION = { { "APA",  //  Praha 1
                                                                      "APB",  //  Praha 2
                                                                      "APC",  //  Praha 3
                                                                      "APD",  //  Praha 4
                                                                      "APE",  //  Praha 5
                                                                      "APF",  //  Praha 6
                                                                      "APG",  //  Praha 7
                                                                      "APH",  //  Praha 8
                                                                      "API",  //  Praha 9
                                                                      "APJ",  //  Praha 10
                                                                      "BBN",  //  Benesov
                                                                      "BBE",  //  Beroun
                                                                      "BKD",  //  Kladno
                                                                      "BKO",  //  Kolin
                                                                      "BKH",  //  Kutna Hora
                                                                      "BME",  //  Melnik
                                                                      "BMB",  //  Mlada Boleslav
                                                                      "BNY",  //  Nymburk
                                                                      "BPZ",  //  Praha zapad
                                                                      "BPV",  //  Praha vychod
                                                                      "BPB",  //  Pribram
                                                                      "BRA",  //  Rakovnik
                                                                      "CBU",  //  Ceske Budejovice
                                                                      "CCK",  //  Cesky Krumlov
                                                                      "CJH",  //  Jindrichuv Hradec
                                                                      "CPE",  //  Pelhrimov
                                                                      "CPI",  //  Pisek
                                                                      "CPR",  //  Prachatice
                                                                      "CST",  //  Strakonice
                                                                      "CTA",  //  Tabor
                                                                      "DDO",  //  Domazlice
                                                                      "DCH",  //  Cheb
                                                                      "DKV",  //  Karlovy Vary
                                                                      "DKL",  //  Klatovy
                                                                      "DPM",  //  Plzen mesto
                                                                      "DPJ",  //  Plzen jih
                                                                      "DPS",  //  Plzen sever
                                                                      "DRO",  //  Rokycany
                                                                      "DSO",  //  Sokolov
                                                                      "DTA",  //  Tachov
                                                                      "ECL",  //  Ceska Lipa
                                                                      "EDE",  //  Decin
                                                                      "ECH",  //  Chomutov
                                                                      "EJA",  //  Jablonec n. Nisou
                                                                      "ELI",  //  Liberec
                                                                      "ELT",  //  Litomerice
                                                                      "ELO",  //  Louny
                                                                      "EMO",  //  Most
                                                                      "ETE",  //  Teplice
                                                                      "EUL",  //  Usti nad Labem
                                                                      "FHB",  //  Havlickuv Brod
                                                                      "FHK",  //  Hradec Kralove
                                                                      "FCR",  //  Chrudim
                                                                      "FJI",  //  Jicin
                                                                      "FNA",  //  Nachod
                                                                      "FPA",  //  Pardubice
                                                                      "FRK",  //  Rychn n. Kneznou
                                                                      "FSE",  //  Semily
                                                                      "FSV",  //  Svitavy
                                                                      "FTR",  //  Trutnov
                                                                      "FUO",  //  Usti nad Orlici
                                                                      "GBL",  //  Blansko
                                                                      "GBM",  //  Brno mesto
                                                                      "GBV",  //  Brno venkov
                                                                      "GBR",  //  Breclav
                                                                      "GHO",  //  Hodonin
                                                                      "GJI",  //  Jihlava
                                                                      "GKR",  //  Kromeriz
                                                                      "GPR",  //  Prostejov
                                                                      "GTR",  //  Trebic
                                                                      "GUH",  //  Uherske Hradiste
                                                                      "GVY",  //  Vyskov
                                                                      "GZL",  //  Zlin
                                                                      "GZN",  //  Znojmo
                                                                      "GZS",  //  Zdar nad Sazavou
                                                                      "HBR",  //  Bruntal
                                                                      "HFM",  //  Frydek-Mistek
                                                                      "HJE",  //  Jesenik
                                                                      "HKA",  //  Karvina
                                                                      "HNJ",  //  Novy Jicin
                                                                      "HOL",  //  Olomouc
                                                                      "HOP",  //  Opava
                                                                      "HOS",  //  Ostrava
                                                                      "HPR",  //  Prerov
                                                                      "HSU",  //  Sumperk
                                                                      "HVS"   //  Vsetin
                                                                  } };

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

typedef std::array<std::string, N_SLOVAKIA_PRIMARIES> PRIMARY_SLOVAKIA_ENUMERATION_TYPE;    ///< primaries for Slovakia

static PRIMARY_SLOVAKIA_ENUMERATION_TYPE PRIMARY_SLOVAKIA_ENUMERATION = { { "BAA",  //  Bratislava 1
                                                                            "BAB",  //  Bratislava 2
                                                                            "BAC",  //  Bratislava 3
                                                                            "BAD",  //  Bratislava 4
                                                                            "BAE",  //  Bratislava 5
                                                                            "MAL",  //  Malacky
                                                                            "PEZ",  //  Pezinok
                                                                            "SEN",  //  Senec
                                                                            "DST",  //  Dunajska Streda
                                                                            "GAL",  //  Galanta
                                                                            "HLO",  //  Hlohovec
                                                                            "PIE",  //  Piestany
                                                                            "SEA",  //  Senica
                                                                            "SKA",  //  Skalica
                                                                            "TRN",  //  Trnava
                                                                            "BAN",  //  Banovce n. Bebr.
                                                                            "ILA",  //  Ilava
                                                                            "MYJ",  //  Myjava
                                                                            "NMV",  //  Nove Mesto n. Vah
                                                                            "PAR",  //  Partizanske
                                                                            "PBY",  //  Povazska Bystrica
                                                                            "PRI",  //  Prievidza
                                                                            "PUC",  //  Puchov
                                                                            "TNC",  //  Trencin
                                                                            "KOM",  //  Komarno
                                                                            "LVC",  //  Levice
                                                                            "NIT",  //  Nitra
                                                                            "NZA",  //  Nove Zamky
                                                                            "SAL",  //  Sala
                                                                            "TOP",  //  Topolcany
                                                                            "ZMO",  //  Zlate Moravce
                                                                            "BYT",  //  Bytca
                                                                            "CAD",  //  Cadca
                                                                            "DKU",  //  Dolny Kubin
                                                                            "KNM",  //  Kysucke N. Mesto
                                                                            "LMI",  //  Liptovsky Mikulas
                                                                            "MAR",  //  Martin
                                                                            "NAM",  //  Namestovo
                                                                            "RUZ",  //  Ruzomberok
                                                                            "TTE",  //  Turcianske Teplice
                                                                            "TVR",  //  Tvrdosin
                                                                            "ZIL",  //  Zilina
                                                                            "BBY",  //  Banska Bystrica
                                                                            "BST",  //  Banska Stiavnica
                                                                            "BRE",  //  Brezno
                                                                            "DET",  //  Detva
                                                                            "KRU",  //  Krupina
                                                                            "LUC",  //  Lucenec
                                                                            "POL",  //  Poltar
                                                                            "REV",  //  Revuca
                                                                            "RSO",  //  Rimavska Sobota
                                                                            "VKR",  //  Velky Krtis
                                                                            "ZAR",  //  Zarnovica
                                                                            "ZIH",  //  Ziar nad Hronom
                                                                            "ZVO",  //  Zvolen
                                                                            "GEL",  //  Gelnica
                                                                            "KEA",  //  Kosice 1
                                                                            "KEB",  //  Kosice 2
                                                                            "KEC",  //  Kosice 3
                                                                            "KED",  //  Kosice 4
                                                                            "KEO",  //  Kosice-okolie
                                                                            "MIC",  //  Michalovce
                                                                            "ROZ",  //  Roznava
                                                                            "SOB",  //  Sobrance
                                                                            "SNV",  //  Spisska Nova Ves
                                                                            "TRE",  //  Trebisov
                                                                            "BAR",  //  Bardejov
                                                                            "HUM",  //  Humenne
                                                                            "KEZ",  //  Kezmarok
                                                                            "LEV",  //  Levoca
                                                                            "MED",  //  Medzilaborce
                                                                            "POP",  //  Poprad
                                                                            "PRE",  //  Presov
                                                                            "SAB",  //  Sabinov
                                                                            "SNI",  //  Snina
                                                                            "SLU",  //  Stara Lubovna
                                                                            "STR",  //  Stropkov
                                                                            "SVI",  //  Svidnik
                                                                            "VRT"   //  Vranov nad Toplou
                                                                        } };

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
  adif_country(const std::string& nm, const std::string& pfx = "", bool del = false);
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
  void _add_country(const std::string& nm, const unsigned int index, const std::string& pfx = "", const bool deleted = false);

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
  adif_type(const char ty, const std::string& nm = "", const std::string& v = "");

  READ_AND_WRITE(name);                        ///< name of the type
  READ_AND_WRITE(type_indicator);              ///< letter that identifies the types
  READ_AND_WRITE(value);                       ///< value of the type

/// convert to printable string
  const std::string to_string(void) const;
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
  adif_DATE(void);

/*! \brief      Constructor
    \param  nm  name
    \param  v   value
*/
  adif_DATE(const std::string& nm, const std::string& v);

/// construct with name
  explicit adif_DATE(const std::string& nm);

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
