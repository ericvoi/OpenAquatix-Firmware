/*
 * comm_config_menu.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_menu_registration.h"
#include "comm_menu_system.h"
#include "comm_main.h"
#include "cfg_parameters.h"
#include "comm_function_loops.h"
#include "cfg_import_export.h"
#include "main.h"
#include "mess_main.h"
#include "mess_modulate.h"
#include "check_inputs.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void setPreambleErrorDetection(void* argument);
void setCargoErrorDetection(void* argument);
void preambleErrorBehavior(void* argument);
void cargoErrorBehavior(void* argument);
void setPreambleEcc(void* argument);
void setMessageEcc(void* argument);
void setModulationMethod(void* argument);
void setFskF0(void* argument);
void setFskF1(void* argument);
void setFhbfskFreqSpacing(void* argument);
void setFhbfskDwell(void* argument);
void setFhbfskTones(void* argument);
void setFhbfskHopper(void* argument);
void toggleWakeupTones(void* argument);
void setWakeupTone1(void* argument);
void setWakeupTone2(void* argument);
void setWakeupTone3(void* argument);
void setBaudRate(void* argument);
void setCenterFrequency(void* argument);
void getBitPeriod(void* argument);
void getBandwidth(void* argument);
void toggleInterleaver(void* argument);
void setSynchronizer(void* argument);
void printConfigOptions(void* argument);
void importConfigOptions(void* argument);
void setDacTransitionDuration(void* argument);
void setModPowerControlMethod(void* argument);
void setModFixedOutput(void* argument);
void setMessageStartFunction(void* argument);
void setBitDecisionFunction(void* argument);
void setHistoricalComparisonThreshold(void* argument);
void toggleAgc(void* argument);
void setFixedPgaGain(void* argument);
void setWindowFunction(void* argument);
void configureSleep(void* argument);
void setLedBrightness(void* argument);
void toggleLed(void* argument);
void setModCalLowerFreq(void* argument);
void setModCalUpperFreq(void* argument);
void updateTvr(void* argument);
void modCalibration(void* argument);
void exportModCalibration(void* argument);
void tuneMatchingNetwork(void* argument);
void updateOcrr(void* argument);
void updateVmax(void* argument);
void toggleModFeedback(void* argument);
void setModFeedbackRatio(void* argument);
void setModOutputPower(void* argument);
void setTransducerR(void* argument);
void setTransducerC0(void* argument);
void setTransducerL0(void* argument);
void setTransducerC1(void* argument);
void setDemodCalRatio(void* argument);
void performDemodCal(void* argument);
void setDemodCalLowerFreq(void* argument);
void setDemodCalUpperFreq(void* argument);
void exportDemodCal(void* argument);
void setMacProtocol(void* argument);
void setID(void* argument);
void setStationaryFlag(void* argument);

/* Private variables ---------------------------------------------------------*/

/* Main menu starting point --------------------------------------------------*/

static MenuID_t config_menu_children[] = {
  MENU_ID_CFG_UNIV, MENU_ID_CFG_MOD,    MENU_ID_CFG_DEMOD,      MENU_ID_CFG_MAC,
  MENU_ID_CFG_DAU,  MENU_ID_CFG_LED,    MENU_ID_CFG_SETID,      MENU_ID_CFG_STATIONARY
};
static const MenuNode_t config_menu = {
  .id = MENU_ID_CFG,
  .description = "Configuration Parameters Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = config_menu_children,
  .num_children = sizeof(config_menu_children) / sizeof(config_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

/* Sub menus -----------------------------------------------------------------*/

static MenuID_t univ_config_menu_children[] = {
  MENU_ID_CFG_UNIV_ERR,         MENU_ID_CFG_UNIV_ECCPREAMBLE, 
  MENU_ID_CFG_UNIV_ECCMESSAGE,  MENU_ID_CFG_UNIV_MOD,      
  MENU_ID_CFG_UNIV_FSK,         MENU_ID_CFG_UNIV_FHBFSK,  
  MENU_ID_CFG_UNIV_BAUD,        MENU_ID_CFG_UNIV_FC,    
  MENU_ID_CFG_UNIV_BP,          MENU_ID_CFG_UNIV_BANDWIDTH, 
  MENU_ID_CFG_UNIV_INTERLEAVER, MENU_ID_CFG_UNIV_SYNC,
  MENU_ID_CFG_UNIV_WAKEUP,      MENU_ID_CFG_UNIV_EXP,
  MENU_ID_CFG_UNIV_IMP
};
static const MenuNode_t univ_config_menu = {
  .id = MENU_ID_CFG_UNIV,
  .description = "Universal Waveform Processing Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = univ_config_menu_children,
  .num_children = sizeof(univ_config_menu_children) / sizeof(univ_config_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t mod_config_menu_children[] = {
  MENU_ID_CFG_MOD_TLEN,   MENU_ID_CFG_MOD_CAL, 
  MENU_ID_CFG_MOD_FB,     MENU_ID_CFG_MOD_METHOD,
  MENU_ID_CFG_MOD_FIXED,  MENU_ID_CFG_MOD_PWROPT
};
static const MenuNode_t mod_config_menu = {
  .id = MENU_ID_CFG_MOD,
  .description = "Modulation Waveform Processing Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = mod_config_menu_children,
  .num_children = sizeof(mod_config_menu_children) / sizeof(mod_config_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t demod_config_menu_children[] = {
  MENU_ID_CFG_DEMOD_CAL,       MENU_ID_CFG_DEMOD_START, 
  MENU_ID_CFG_DEMOD_DECISION,  MENU_ID_CFG_DEMOD_CMPTHRESH, 
  MENU_ID_CFG_DEMOD_AGCEN,     MENU_ID_CFG_DEMOD_GAIN,
  MENU_ID_CFG_DEMOD_WINDOWFCN
};
static const MenuNode_t demod_config_menu = {
  .id = MENU_ID_CFG_DEMOD,
  .description = "Demodulation Waveform Processing Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = demod_config_menu_children,
  .num_children = sizeof(demod_config_menu_children) / sizeof(demod_config_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t change_mac_params = {
  .param_id = MENU_ID_CFG_MAC,
  .state = PARAM_STATE_0
};
static const MenuNode_t change_mac = {
  .id = MENU_ID_CFG_MAC,
  .description = "Change MAC Protocol",
  .handler = setMacProtocol,
  .parent_id = MENU_ID_CFG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &change_mac_params
};

static MenuID_t dau_config_menu_children[] = {
  MENU_ID_CFG_DAU_SLEEP
};
static const MenuNode_t dau_config_menu = {
  .id = MENU_ID_CFG_DAU,
  .description = "Daughter Card Communication Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = dau_config_menu_children,
  .num_children = sizeof(dau_config_menu_children) / sizeof(dau_config_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t led_config_menu_children[] = {
  MENU_ID_CFG_LED_BRIGHTNESS, MENU_ID_CFG_LED_EN
};
static const MenuNode_t led_config_menu = {
  .id = MENU_ID_CFG_LED,
  .description = "LED Configuration Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = led_config_menu_children,
  .num_children = sizeof(led_config_menu_children) / sizeof(led_config_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t set_new_id_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_SETID
};
static const MenuNode_t set_new_id = {
  .id = MENU_ID_CFG_SETID,
  .description = "Set Modem ID",
  .handler = setID,
  .parent_id = MENU_ID_CFG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &set_new_id_param
};

static ParamContext_t set_stationary_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_STATIONARY
};
static const MenuNode_t set_stationary = {
  .id = MENU_ID_CFG_STATIONARY,
  .description = "Toggle Stationary Flag",
  .handler = setStationaryFlag,
  .parent_id = MENU_ID_CFG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &set_stationary_param
};

/* Sub sub menus -------------------------------------------------------------*/

static MenuID_t univ_config_err_children[] = {
  MENU_ID_CFG_UNIV_ERR_PREAMBLE, MENU_ID_CFG_UNIV_ERR_CARGO,
  MENU_ID_CFG_UNIV_ERR_PREERR,   MENU_ID_CFG_UNIV_ERR_CARGOERR
};
static const MenuNode_t univ_config_err_menu = {
  .id = MENU_ID_CFG_UNIV_ERR,
  .description = "Error Detection Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = univ_config_err_children,
  .num_children = sizeof(univ_config_err_children) / sizeof(univ_config_err_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t univ_config_ecc_preamble_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ECCPREAMBLE
};
static const MenuNode_t univ_config_ecc_preamble = {
  .id = MENU_ID_CFG_UNIV_ECCPREAMBLE,
  .description = "Preamble ECC",
  .handler = setPreambleEcc,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_ecc_preamble_param
};

static ParamContext_t univ_config_ecc_message_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ECCMESSAGE
};
static const MenuNode_t univ_config_ecc_message = {
  .id = MENU_ID_CFG_UNIV_ECCMESSAGE,
  .description = "Message ECC",
  .handler = setMessageEcc,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_ecc_message_param
};

static ParamContext_t univ_config_mod_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_MOD,
};
static const MenuNode_t univ_config_mod = {
  .id = MENU_ID_CFG_UNIV_MOD,
  .description = "Modulation Method",
  .handler = setModulationMethod,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_mod_param
};

static MenuID_t univ_config_fsk_children[] = {
  MENU_ID_CFG_UNIV_FSK_F0, MENU_ID_CFG_UNIV_FSK_F1
};
static const MenuNode_t univ_config_fsk_menu = {
  .id = MENU_ID_CFG_UNIV_FSK,
  .description = "FSK Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = univ_config_fsk_children,
  .num_children = sizeof(univ_config_fsk_children) / sizeof(univ_config_fsk_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t univ_config_fhbfsk_children[] = {
  MENU_ID_CFG_UNIV_FHBFSK_FSEP,  MENU_ID_CFG_UNIV_FHBFSK_DWELL,
  MENU_ID_CFG_UNIV_FHBFSK_TONES, MENU_ID_CFG_UNIV_FHBFSK_HOPP
};
static const MenuNode_t univ_config_fhbsk_menu = {
  .id = MENU_ID_CFG_UNIV_FHBFSK,
  .description = "FHBFSK Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = univ_config_fhbfsk_children,
  .num_children = sizeof(univ_config_fhbfsk_children) / sizeof(univ_config_fhbfsk_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t univ_config_baud_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_BAUD
};
static const MenuNode_t univ_config_baud = {
  .id = MENU_ID_CFG_UNIV_BAUD,
  .description = "Configure Baud Rate",
  .handler = setBaudRate,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_baud_param
};

static ParamContext_t univ_config_fc_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FC
};
static const MenuNode_t univ_config_fc = {
  .id = MENU_ID_CFG_UNIV_FC,
  .description = "Set Center Frequency",
  .handler = setCenterFrequency,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_fc_param
};

static ParamContext_t univ_config_bit_period_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_BP
};
static const MenuNode_t univ_config_bit_period = {
  .id = MENU_ID_CFG_UNIV_BP,
  .description = "Get Bit Period",
  .handler = getBitPeriod,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_bit_period_param
};

static ParamContext_t univ_config_bandwidth_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_BANDWIDTH
};
static const MenuNode_t univ_config_bandwidth = {
  .id = MENU_ID_CFG_UNIV_BANDWIDTH,
  .description = "Get Bandwidth",
  .handler = getBandwidth,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_bandwidth_param
};

static ParamContext_t univ_config_interleaver_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_INTERLEAVER
};
static const MenuNode_t univ_config_interleaver = {
  .id = MENU_ID_CFG_UNIV_INTERLEAVER,
  .description = "Toggle Interleaver",
  .handler = toggleInterleaver,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_interleaver_param
};

static ParamContext_t univ_config_sync_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_SYNC
};
static const MenuNode_t univ_config_sync = {
  .id = MENU_ID_CFG_UNIV_SYNC,
  .description = "Set Synchronization Method",
  .handler = setSynchronizer,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_sync_param
};

static MenuID_t univ_config_wakeup_children[] = {
  MENU_ID_CFG_UNIV_WAKEUP_EN, MENU_ID_CFG_UNIV_WAKEUP_F1,
  MENU_ID_CFG_UNIV_WAKEUP_F2, MENU_ID_CFG_UNIV_WAKEUP_F3
};
static const MenuNode_t univ_config_wakeup_menu = {
  .id = MENU_ID_CFG_UNIV_WAKEUP,
  .description = "Wakeup Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = univ_config_wakeup_children,
  .num_children = sizeof(univ_config_wakeup_children) / sizeof(univ_config_wakeup_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t univ_config_export_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_EXP  
};
static const MenuNode_t univ_config_export = {
  .id = MENU_ID_CFG_UNIV_EXP,
  .description = "Export Configuration Options Used",
  .handler = printConfigOptions,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_export_param
};

static ParamContext_t univ_config_import_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_IMP
};
static const MenuNode_t univ_config_import = {
  .id = MENU_ID_CFG_UNIV_IMP,
  .description = "Import Configuration options",
  .handler = importConfigOptions,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_config_import_param
};

static ParamContext_t mod_config_dac_transition_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_TLEN
};
static const MenuNode_t mod_config_dac_transition = {
  .id = MENU_ID_CFG_MOD_TLEN,
  .description = "Set DAC Transition Duration",
  .handler = setDacTransitionDuration,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_config_dac_transition_param
};

static MenuID_t mod_config_cal_children[] = {
  MENU_ID_CFG_MOD_CAL_LOWFREQ,  MENU_ID_CFG_MOD_CAL_HIFREQ, 
  MENU_ID_CFG_MOD_CAL_TVR,      MENU_ID_CFG_MOD_CAL_RECV, 
  MENU_ID_CFG_MOD_CAL_PERFORM,  MENU_ID_CFG_MOD_CAL_EXP, 
  MENU_ID_CFG_MOD_CAL_TUNE,     MENU_ID_CFG_MOD_CAL_VMAX
};
static const MenuNode_t mod_config_cal_menu = {
  .id = MENU_ID_CFG_MOD_CAL,
  .description = "Modulation Calibration Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = mod_config_cal_children,
  .num_children = sizeof(mod_config_cal_children) / sizeof(mod_config_cal_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t mod_config_feedback_children[] = {
  MENU_ID_CFG_MOD_FB_EN, MENU_ID_CFG_MOD_FB_RATIO
};
static const MenuNode_t mod_config_feedback_menu = {
  .id = MENU_ID_CFG_MOD_FB,
  .description = "Modulation Feedback Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = mod_config_feedback_children,
  .num_children = sizeof(mod_config_feedback_children) / sizeof(mod_config_feedback_children[0]),
  .access_level = 0,
  .parameters = NULL,
};

static ParamContext_t mod_config_method_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_METHOD
};
static const MenuNode_t mod_config_method = {
  .id = MENU_ID_CFG_MOD_METHOD,
  .description = "Set method to control output strength",
  .handler = setModPowerControlMethod,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_config_method_param
};

static ParamContext_t mod_config_fixed_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_FIXED
};
static const MenuNode_t mod_config_fixed = {
  .id = MENU_ID_CFG_MOD_FIXED,
  .description = "Fixed relative DAC output strength to use",
  .handler = setModFixedOutput,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_config_fixed_param
};

static MenuID_t mod_config_power_children[] = {
  MENU_ID_CFG_MOD_PWROPT_PWR, MENU_ID_CFG_MOD_PWROPT_R, 
  MENU_ID_CFG_MOD_PWROPT_C0,  MENU_ID_CFG_MOD_PWROPT_L0,
  MENU_ID_CFG_MOD_PWROPT_C1
};
static const MenuNode_t mod_config_power_menu = {
  .id = MENU_ID_CFG_MOD_PWROPT,
  .description = "Fixed output power level options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = mod_config_power_children,
  .num_children = sizeof(mod_config_power_children) / sizeof(mod_config_power_children[0]),
  .access_level = 0,
  .parameters = NULL,
};

static MenuID_t demod_config_cal_children[] = {
  MENU_ID_CFG_DEMOD_CAL_RATIO,     MENU_ID_CFG_DEMOD_CAL_PERFORM,
  MENU_ID_CFG_DEMOD_CAL_LOWFREQ,   MENU_ID_CFG_DEMOD_CAL_HIFREQ,
  MENU_ID_CFG_DEMOD_CAL_EXP
};
static const MenuNode_t demod_config_cal_menu = {
  .id = MENU_ID_CFG_DEMOD_CAL,
  .description = "Demodulation Cablibration Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = demod_config_cal_children,
  .num_children = sizeof(demod_config_cal_children) / sizeof(demod_config_cal_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t demod_config_start_fcn_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_START
};
static const MenuNode_t demod_config_start_fcn = {
  .id = MENU_ID_CFG_DEMOD_START,
  .description = "Select the message start function to use",
  .handler = setMessageStartFunction,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_config_start_fcn_param
};

static ParamContext_t demod_config_decision_fcn_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_DECISION
};
static const MenuNode_t demod_config_decision_fcn = {
  .id = MENU_ID_CFG_DEMOD_DECISION,
  .description = "Select the bit decision function to use",
  .handler = setBitDecisionFunction,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_config_decision_fcn_param
};

static ParamContext_t demod_config_sig_shift_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CMPTHRESH
};
static const MenuNode_t demod_config_sig_shift = {
  .id = MENU_ID_CFG_DEMOD_CMPTHRESH,
  .description = "Set the historical comparison threshold",
  .handler = setHistoricalComparisonThreshold,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_config_sig_shift_param
};

static ParamContext_t demod_config_use_agc_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_AGCEN
};
static const MenuNode_t demod_config_use_agc = {
  .id = MENU_ID_CFG_DEMOD_AGCEN,
  .description = "Enable/disable automatic gain control (AGC)",
  .handler = toggleAgc,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_config_use_agc_param
};

static ParamContext_t demod_config_fixed_gain_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_GAIN
};
static const MenuNode_t demod_config_fixed_gain = {
  .id = MENU_ID_CFG_DEMOD_GAIN,
  .description = "Set fixed PGA gain",
  .handler = setFixedPgaGain,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_config_fixed_gain_param
};

static ParamContext_t demod_config_window_fcn_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_WINDOWFCN
};
static const MenuNode_t demod_config_window_fcn = {
  .id = MENU_ID_CFG_DEMOD_WINDOWFCN,
  .description = "Set windowing function",
  .handler = setWindowFunction,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_config_window_fcn_param
};

static ParamContext_t dau_config_sleep_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DAU_SLEEP
};
static const MenuNode_t dau_config_sleep = {
  .id = MENU_ID_CFG_DAU_SLEEP,
  .description = "Enable or Disable Sleep Modes",
  .handler = configureSleep,
  .parent_id = MENU_ID_CFG_DAU,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &dau_config_sleep_param
};

static ParamContext_t led_config_brightness_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_LED_BRIGHTNESS
};
static const MenuNode_t led_config_brightness = {
  .id = MENU_ID_CFG_LED_BRIGHTNESS,
  .description = "Set LED Brightness",
  .handler = setLedBrightness,
  .parent_id = MENU_ID_CFG_LED,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &led_config_brightness_param
};

static ParamContext_t led_config_toggle_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_LED_EN
};
static const MenuNode_t led_config_toggle = {
  .id = MENU_ID_CFG_LED_EN,
  .description = "Toggle LED",
  .handler = toggleLed,
  .parent_id = MENU_ID_CFG_LED,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &led_config_toggle_param
};

/* Sub sub sub menus ---------------------------------------------------------*/

static ParamContext_t univ_err_config_preamble_validation_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ERR_PREAMBLE
};
static const MenuNode_t univ_err_config_preamble_validation = {
  .id = MENU_ID_CFG_UNIV_ERR_PREAMBLE,
  .description = "Preamble error detection method",
  .handler = setPreambleErrorDetection,
  .parent_id = MENU_ID_CFG_UNIV_ERR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_err_config_preamble_validation_param
};

static ParamContext_t univ_err_config_cargo_validation_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ERR_CARGO
};
static const MenuNode_t univ_err_config_cargo_validation = {
  .id = MENU_ID_CFG_UNIV_ERR_CARGO,
  .description = "Cargo error detection method",
  .handler = setCargoErrorDetection,
  .parent_id = MENU_ID_CFG_UNIV_ERR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_err_config_cargo_validation_param
};

static ParamContext_t univ_err_config_preamble_behavior_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ERR_PREERR
};
static const MenuNode_t univ_err_config_preamble_behavior = {
  .id = MENU_ID_CFG_UNIV_ERR_PREERR,
  .description = "Preamble error behavior",
  .handler = preambleErrorBehavior,
  .parent_id = MENU_ID_CFG_UNIV_ERR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_err_config_preamble_behavior_param
};

static ParamContext_t univ_err_config_cargo_behavior_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ERR_CARGOERR
};
static const MenuNode_t univ_err_config_cargo_behavior = {
  .id = MENU_ID_CFG_UNIV_ERR_CARGOERR,
  .description = "Cargo error behavior",
  .handler = cargoErrorBehavior,
  .parent_id = MENU_ID_CFG_UNIV_ERR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_err_config_cargo_behavior_param
};

static ParamContext_t univ_fsk_config_f0_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FSK_F0
};
static const MenuNode_t univ_fsk_config_f0 = {
  .id = MENU_ID_CFG_UNIV_FSK_F0,
  .description = "Set FSK Frequency '0'",
  .handler = setFskF0,
  .parent_id = MENU_ID_CFG_UNIV_FSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_fsk_config_f0_param
};

static ParamContext_t univ_fsk_config_f1_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FSK_F1
};
static const MenuNode_t univ_fsk_config_f1 = {
  .id = MENU_ID_CFG_UNIV_FSK_F1,
  .description = "Set FSK Frequency '1'",
  .handler = setFskF1,
  .parent_id = MENU_ID_CFG_UNIV_FSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_fsk_config_f1_param
};

static ParamContext_t univ_fhbfsk_config_freq_spacing_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FHBFSK_FSEP
};
static const MenuNode_t univ_fhbfsk_config_freq_spacing = {
  .id = MENU_ID_CFG_UNIV_FHBFSK_FSEP,
  .description = "Set Frequency Spacing",
  .handler = setFhbfskFreqSpacing,
  .parent_id = MENU_ID_CFG_UNIV_FHBFSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_fhbfsk_config_freq_spacing_param
};

static ParamContext_t univ_fhbfsk_config_dwell_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FHBFSK_DWELL
};
static const MenuNode_t univ_fhbfsk_config_dwell = {
  .id = MENU_ID_CFG_UNIV_FHBFSK_DWELL,
  .description = "Set Number of Bit Dwell Periods",
  .handler = setFhbfskDwell,
  .parent_id = MENU_ID_CFG_UNIV_FHBFSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_fhbfsk_config_dwell_param
};

static ParamContext_t univ_fhbfsk_config_tones_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FHBFSK_TONES
};
static const MenuNode_t univ_fhbfsk_config_tones = {
  .id = MENU_ID_CFG_UNIV_FHBFSK_TONES,
  .description = "Set Number of Tones",
  .handler = setFhbfskTones,
  .parent_id = MENU_ID_CFG_UNIV_FHBFSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_fhbfsk_config_tones_param
};

static ParamContext_t univ_fhbfsk_config_hopper_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FHBFSK_HOPP
};
static const MenuNode_t univ_fhbfsk_config_hopper = {
  .id = MENU_ID_CFG_UNIV_FHBFSK_HOPP,
  .description = "Set Frequency Hopper",
  .handler = setFhbfskHopper,
  .parent_id = MENU_ID_CFG_UNIV_FHBFSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_fhbfsk_config_hopper_param
};

static ParamContext_t univ_wakeup_config_en_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_WAKEUP_EN
};
static const MenuNode_t univ_wakeup_config_en = {
  .id = MENU_ID_CFG_UNIV_WAKEUP_EN,
  .description = "Toggle sending wakeup tones",
  .handler = toggleWakeupTones,
  .parent_id = MENU_ID_CFG_UNIV_WAKEUP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_wakeup_config_en_param
};

static ParamContext_t univ_wakeup_config_tone1_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_WAKEUP_F1
};
static const MenuNode_t univ_wakeup_config_tone1 = {
  .id = MENU_ID_CFG_UNIV_WAKEUP_F1,
  .description = "First wakeup tone frequency",
  .handler = setWakeupTone1,
  .parent_id = MENU_ID_CFG_UNIV_WAKEUP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_wakeup_config_tone1_param
};

static ParamContext_t univ_wakeup_config_tone2_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_WAKEUP_F2
};
static const MenuNode_t univ_wakeup_config_tone2 = {
  .id = MENU_ID_CFG_UNIV_WAKEUP_F2,
  .description = "Second wakeup tone frequency",
  .handler = setWakeupTone2,
  .parent_id = MENU_ID_CFG_UNIV_WAKEUP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_wakeup_config_tone2_param
};

static ParamContext_t univ_wakeup_config_tone3_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_WAKEUP_F3
};
static const MenuNode_t univ_wakeup_config_tone3 = {
  .id = MENU_ID_CFG_UNIV_WAKEUP_F3,
  .description = "Third wakeup tone frequency",
  .handler = setWakeupTone3,
  .parent_id = MENU_ID_CFG_UNIV_WAKEUP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univ_wakeup_config_tone3_param
};

static ParamContext_t mod_cal_config_low_freq_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_LOWFREQ
};
static const MenuNode_t mod_cal_config_low_freq = {
  .id = MENU_ID_CFG_MOD_CAL_LOWFREQ,
  .description = "Lower Frequency for Calibration",
  .handler = setModCalLowerFreq,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_cal_config_low_freq_param
};

static ParamContext_t mod_cal_config_upper_freq_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_HIFREQ
};
static const MenuNode_t mod_cal_config_upper_freq = {
  .id = MENU_ID_CFG_MOD_CAL_HIFREQ,
  .description = "Upper Frequency for Calibration",
  .handler = setModCalUpperFreq,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_cal_config_upper_freq_param
};

static ParamContext_t mod_cal_config_tvr_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_TVR
};
static const MenuNode_t mod_cal_config_tvr = {
  .id = MENU_ID_CFG_MOD_CAL_TVR,
  .description = "Import TVR",
  .handler = updateTvr,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_cal_config_tvr_param
};

static ParamContext_t mod_cal_config_perform_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_PERFORM
};
static const MenuNode_t mod_cal_config_perform = {
  .id = MENU_ID_CFG_MOD_CAL_PERFORM,
  .description = "Perform Calibration",
  .handler = modCalibration,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_cal_config_perform_param
};

static ParamContext_t mod_cal_config_export_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_EXP
};
static const MenuNode_t mod_cal_config_export = {
  .id = MENU_ID_CFG_MOD_CAL_EXP,
  .description = "Export Calibration",
  .handler = exportModCalibration,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_cal_config_export_param
};

static ParamContext_t mod_cal_config_tune_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_TUNE
};
static const MenuNode_t mod_cal_config_tune = {
  .id = MENU_ID_CFG_MOD_CAL_TUNE,
  .description = "Tune Matching Network",
  .handler = tuneMatchingNetwork,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_cal_config_tune_param
};

static ParamContext_t mod_cal_config_recv_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_RECV
};
static const MenuNode_t mod_cal_config_recv = {
  .id = MENU_ID_CFG_MOD_CAL_RECV,
  .description = "Import OCRR",
  .handler = updateOcrr,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_cal_config_recv_param
};

static ParamContext_t mod_cal_config_vmax_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_VMAX
};
static const MenuNode_t mod_cal_config_vmax = {
  .id = MENU_ID_CFG_MOD_CAL_VMAX,
  .description = "Set Maximum Output Voltage",
  .handler = updateVmax,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_cal_config_vmax_param
};

static ParamContext_t mod_fb_config_toggle_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_FB_EN
};
static const MenuNode_t mod_fb_config_toggle = {
  .id = MENU_ID_CFG_MOD_FB_EN,
  .description = "Toggle Feedback Network",
  .handler = toggleModFeedback,
  .parent_id = MENU_ID_CFG_MOD_FB,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_fb_config_toggle_param
};

static ParamContext_t mod_fb_config_ratio_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_FB_RATIO
};
static const MenuNode_t mod_fb_config_ratio = {
  .id = MENU_ID_CFG_MOD_FB_RATIO,
  .description = "Set Feedback Division Ratio",
  .handler = setModFeedbackRatio,
  .parent_id = MENU_ID_CFG_MOD_FB,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_fb_config_ratio_param
};

static ParamContext_t mod_pwr_config_target_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_PWR
};
static const MenuNode_t mod_pwr_config_target = {
  .id = MENU_ID_CFG_MOD_PWROPT_PWR,
  .description = "Target output power (W)",
  .handler = setModOutputPower,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_pwr_config_target_param
};

static ParamContext_t mod_pwr_config_r_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_R
};
static const MenuNode_t mod_pwr_config_r = {
  .id = MENU_ID_CFG_MOD_PWROPT_R,
  .description = "Series resistance of the motional branch (R) [ohms]",
  .handler = setTransducerR,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_pwr_config_r_param
};

static ParamContext_t mod_pwr_config_c0_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_C0
};
static const MenuNode_t mod_pwr_config_c0 = {
  .id = MENU_ID_CFG_MOD_PWROPT_C0,
  .description = "Series capacitance of the motional branch (C0) [nF]",
  .handler = setTransducerC0,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_pwr_config_c0_param
};

static ParamContext_t mod_pwr_config_l0_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_L0
};
static const MenuNode_t mod_pwr_config_l0 = {
  .id = MENU_ID_CFG_MOD_PWROPT_L0,
  .description = "Series inductance of the motional branch (L0) [mH]",
  .handler = setTransducerL0,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_pwr_config_l0_param
};

static ParamContext_t mod_pwr_config_c1_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_C1
};
static const MenuNode_t mod_pwr_config_c1 = {
  .id = MENU_ID_CFG_MOD_PWROPT_C1,
  .description = "Parallel capacitance with the motional branch (C1) [nF]",
  .handler = setTransducerC1,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &mod_pwr_config_c1_param
};

static ParamContext_t demod_cal_config_ratio_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_RATIO
};
static const MenuNode_t demod_cal_config_ratio = {
  .id = MENU_ID_CFG_DEMOD_CAL_RATIO,
  .description = "Set Feedback Network Voltage Division Ratio",
  .handler = setDemodCalRatio,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_cal_config_ratio_param
};

static ParamContext_t demod_cal_config_perform_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_PERFORM
};
static const MenuNode_t demod_cal_config_perform = {
  .id = MENU_ID_CFG_DEMOD_CAL_PERFORM,
  .description = "Perform Calibration",
  .handler = performDemodCal,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_cal_config_perform_param
};

static ParamContext_t demod_cal_config_low_freq_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_LOWFREQ
};
static const MenuNode_t demod_cal_config_low_freq = {
  .id = MENU_ID_CFG_DEMOD_CAL_LOWFREQ,
  .description = "Lower frequency used for demodulation calibration",
  .handler = setDemodCalLowerFreq,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_cal_config_low_freq_param
};

static ParamContext_t demod_cal_config_upper_freq_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_HIFREQ,
};
static const MenuNode_t demod_cal_config_upper_freq = {
  .id = MENU_ID_CFG_DEMOD_CAL_HIFREQ,
  .description = "Upper frequency used for demodulation calibration",
  .handler = setDemodCalUpperFreq,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_cal_config_upper_freq_param
};

static ParamContext_t demod_cal_config_export_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_EXP
};
static const MenuNode_t demod_cal_config_export = {
  .id = MENU_ID_CFG_DEMOD_CAL_EXP,
  .description = "Export Calibration Results",
  .handler = exportDemodCal,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demod_cal_config_export_param
};

/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterConfigurationMenu()
{
  bool ret = registerMenu(&config_menu) && registerMenu(&univ_config_menu) &&
             registerMenu(&mod_config_menu) && registerMenu(&demod_config_menu) &&
             registerMenu(&dau_config_menu) && registerMenu(&led_config_menu) && 
             registerMenu(&univ_config_err_menu) && registerMenu(&demod_config_decision_fcn) &&
             registerMenu(&univ_config_mod) && registerMenu(&univ_config_fsk_menu) && 
             registerMenu(&univ_config_fhbsk_menu) && registerMenu(&univ_config_baud) && 
             registerMenu(&univ_config_fc) && registerMenu(&univ_config_bit_period) && 
             registerMenu(&univ_config_export) && registerMenu(&univ_config_import) && 
             registerMenu(&set_stationary) && registerMenu(&mod_config_dac_transition) &&
             registerMenu(&mod_config_cal_menu) && registerMenu(&mod_config_feedback_menu) && 
             registerMenu(&mod_config_method) && registerMenu(&univ_config_interleaver) &&
             registerMenu(&demod_config_cal_menu) && registerMenu(&univ_fhbfsk_config_hopper) &&
             registerMenu(&dau_config_sleep) && registerMenu(&led_config_brightness) &&
             registerMenu(&led_config_toggle) && registerMenu(&mod_cal_config_low_freq) &&
             registerMenu(&mod_cal_config_upper_freq) && registerMenu(&mod_cal_config_tvr) && 
             registerMenu(&mod_cal_config_perform) && registerMenu(&mod_cal_config_export) &&
             registerMenu(&mod_cal_config_tune) && registerMenu(&mod_cal_config_recv) && 
             registerMenu(&mod_cal_config_vmax) && registerMenu(&mod_fb_config_toggle) &&
             registerMenu(&mod_fb_config_ratio) && registerMenu(&demod_config_sig_shift) &&
             registerMenu(&demod_cal_config_ratio) && registerMenu(&demod_cal_config_perform) && 
             registerMenu(&demod_cal_config_low_freq) && registerMenu(&demod_cal_config_upper_freq) && 
             registerMenu(&demod_cal_config_export) && registerMenu(&univ_config_sync) &&
             registerMenu(&demod_config_start_fcn) && registerMenu(&univ_fsk_config_f0) &&
             registerMenu(&univ_fsk_config_f1) && registerMenu(&univ_fhbfsk_config_freq_spacing) &&
             registerMenu(&univ_fhbfsk_config_dwell) && registerMenu(&univ_config_bandwidth) &&
             registerMenu(&univ_fhbfsk_config_tones) && registerMenu(&set_new_id) &&
             registerMenu(&mod_config_fixed) && registerMenu(&mod_config_power_menu) &&
             registerMenu(&mod_pwr_config_target) && registerMenu(&mod_pwr_config_r) &&
             registerMenu(&mod_pwr_config_c0) && registerMenu(&mod_pwr_config_l0) &&
             registerMenu(&mod_pwr_config_c1) && registerMenu(&demod_config_use_agc) &&
             registerMenu(&demod_config_fixed_gain) && registerMenu(&univ_config_ecc_preamble) &&
             registerMenu(&univ_config_ecc_message) && registerMenu(&univ_err_config_preamble_validation) &&
             registerMenu(&univ_err_config_cargo_validation) && registerMenu(&univ_err_config_preamble_behavior) &&
             registerMenu(&univ_err_config_cargo_behavior) && registerMenu(&demod_config_window_fcn) &&
             registerMenu(&univ_config_wakeup_menu) && registerMenu(&univ_wakeup_config_tone1) &&
             registerMenu(&univ_wakeup_config_en) && registerMenu(&univ_wakeup_config_tone2) &&
             registerMenu(&univ_wakeup_config_tone3) && registerMenu(&change_mac);

  return ret;
}

/* Private function definitions ----------------------------------------------*/

void setPreambleErrorDetection(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  char* descriptors[] = {"None",
                         "CRC-8",      "CRC-16",      "CRC-32", 
                         "Checksum-8", "Checksum-16", "Checksum-32"};

  COMMLoops_LoopEnum(context, PARAM_PREAMBLE_ERROR_DETECTION, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setCargoErrorDetection(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  char* descriptors[] = {"None",
                         "CRC-8",      "CRC-16",      "CRC-32", 
                         "Checksum-8", "Checksum-16", "Checksum-32"};

  COMMLoops_LoopEnum(context, PARAM_CARGO_ERROR_DETECTION, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void preambleErrorBehavior(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void cargoErrorBehavior(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void setPreambleEcc(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  char* descriptors[] = {"None", "1-bit Hamming Code", "1:2 Convolutional Code (JANUS)"};

  COMMLoops_LoopEnum(context, PARAM_ECC_PREAMBLE, descriptors,
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setMessageEcc(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  char* descriptors[] = {"None", "1-bit Hamming Code", "1:2 Convolutional Code (JANUS)"};

  COMMLoops_LoopEnum(context, PARAM_ECC_MESSAGE, descriptors,
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setModulationMethod(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"FSK", "FHBFSK"};
  
  COMMLoops_LoopEnum(context, PARAM_MOD_DEMOD_METHOD, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setFskF0(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint32(context, PARAM_FSK_F0);
}

void setFskF1(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint32(context, PARAM_FSK_F1);
}

void setFhbfskFreqSpacing(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint8(context, PARAM_FHBFSK_FREQ_SPACING);
}

void setFhbfskDwell(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint8(context, PARAM_FHBFSK_DWELL_TIME);
}

void setFhbfskTones(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint8(context, PARAM_FHBFSK_NUM_TONES);
}

void setFhbfskHopper(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char *descriptors[] = {"Increment by 1", "Galois Field arithmetic (JANUS)", 
                         "Prime selector"};

  COMMLoops_LoopEnum(context, PARAM_FHBFSK_HOPPER, descriptors, 
      sizeof(descriptors) / sizeof(descriptors[0]));
}

void toggleWakeupTones(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopToggle(context, PARAM_WAKEUP_TONES_STATE);
}

void setWakeupTone1(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint32(context, PARAM_WAKEUP_TONE1);
}

void setWakeupTone2(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint32(context, PARAM_WAKEUP_TONE2);
}

void setWakeupTone3(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint32(context, PARAM_WAKEUP_TONE3);
}

void setBaudRate(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  ParamIds_t param_id = PARAM_BAUD;

  ParamState_t old_state = context->state->state;

  static float new_baud;

  char* parameter_name = Param_GetName(param_id);

  if (parameter_name == NULL) {
    COMM_TransmitData(uninitialized_parameter_message, CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  float min, max;
  if (Param_GetFloatLimits(param_id, &min, &max) == false) {
    COMM_TransmitData(error_limits_message, CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        float current_value;
        if (Param_GetFloat(param_id, &current_value) == false) {
          sprintf((char*) context->output_buffer, "\r\nError obtaining current "
                  "value for %s\r\n", parameter_name);
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
        }
        else {
          sprintf((char*) context->output_buffer, "\r\n\r\nCurrent value of %s:"
                  " %.2f\r\n", parameter_name, current_value);
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          sprintf((char*) context->output_buffer, "Please enter a new value from"
                  " %.2f to %.2f:\r\n", min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_1;
        }
        break;
      case PARAM_STATE_1:
        if (checkFloat(context->input, &new_baud, min, max) == true) {
          MESS_RoundBaud(&new_baud);
          sprintf((char*) context->output_buffer, "\r\nThe closest allowable "
                  "baud rate is %.2f. Is this ok? (y/n)\r\n", new_baud);
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_2;
          break;
        } else {
          sprintf((char*) context->output_buffer, "\r\nValue %.2f is outside "
                  "the range of %.2f and %.2f\r\n", new_baud, min, max);
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        break;
      case PARAM_STATE_2:
        bool confirmed = false;
        if (checkYesNo(*context->input, &confirmed) == true) {
          if (confirmed == true) {
            if (Param_SetFloat(param_id, &new_baud) == true) {
              sprintf((char*) context->output_buffer, "\r\n%s successfully set"
              " to new value of %.2f\r\n", parameter_name, new_baud);
              COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
            }
            else {
              COMM_TransmitData(error_updating_message, CALC_LEN, context->comm_interface);
            }
            context->state->state = PARAM_STATE_COMPLETE;
          }
          else {
            context->state->state = PARAM_STATE_COMPLETE;
          }
        }
        else {
          sprintf((char*) context->output_buffer, "\r\nInvalid Input!\r\n");
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_1;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void setCenterFrequency(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint32(context, PARAM_FC);
}

void getBitPeriod(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  float bit_period_ms;

  if (MESS_GetBitPeriod(&bit_period_ms) == false) {
    COMM_TransmitData("\r\nInternal Error!\r\n", CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  sprintf((char*) context->output_buffer, "\r\nBit period: %.2fms\r\n", bit_period_ms);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  context->state->state = PARAM_STATE_COMPLETE;
}

void getBandwidth(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  uint32_t bandwidth, lower_freq, upper_freq;

  if (MESS_GetBandwidth(&bandwidth, &lower_freq, &upper_freq) == false) {
    COMM_TransmitData("\r\nInternal Error!\r\n", CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  sprintf((char*) context->output_buffer, "\r\nLower frequency: %luHz\r\n", lower_freq);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  sprintf((char*) context->output_buffer, "Upper frequency: %luHz\r\n", upper_freq);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  sprintf((char*) context->output_buffer, "Bandwidth: %luHz\r\n", bandwidth);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  context->state->state = PARAM_STATE_COMPLETE;
}

void toggleInterleaver(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopToggle(context, PARAM_USE_INTERLEAVER);
}

void setSynchronizer(void* argument) 
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"None", "JANUS 32-chips"};

  COMMLoops_LoopEnum(context, PARAM_SYNC_METHOD, descriptors,
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void printConfigOptions(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  if (ImportExport_ExportConfiguration(context) == false) {
    COMM_TransmitData("\r\nInternal Error!\r\n", CALC_LEN, context->comm_interface);
  }
}

void importConfigOptions(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  ImportExport_ImportConfiguration(context);
}

void setDacTransitionDuration(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint16(context, PARAM_DAC_TRANSITION_LEN);
}

void setModPowerControlMethod(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"Static DAC output", "Static Output Power"};

  COMMLoops_LoopEnum(context, PARAM_MODULATION_OUTPUT_METHOD, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setModFixedOutput(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopFloat(context, PARAM_OUTPUT_AMPLITUDE);
}

void setMessageStartFunction(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"Use amplitude threshold", "Use overlapping FFTs"};

  COMMLoops_LoopEnum(context, PARAM_MSG_START_FCN, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setBitDecisionFunction(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"Use energy comparison", "Use historical comparison"};

  COMMLoops_LoopEnum(context, PARAM_DEMODULATION_DECISION, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setHistoricalComparisonThreshold(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopFloat(context, PARAM_HISTORICAL_COMPARISON_THRESHOLD);
}

void toggleAgc(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopToggle(context, PARAM_AGC_ENABLE);
}

void setFixedPgaGain(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"1", "2", "5", "10", "20", "50", "100", "200"};

  COMMLoops_LoopEnum(context, PARAM_FIXED_PGA_GAIN, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setWindowFunction(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"Rectangular", "Hann", "Hamming"};

  COMMLoops_LoopEnum(context, PARAM_WINDOW_FUNCTION, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

// TODO: implement
void configureSleep(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void setLedBrightness(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopUint16(context, PARAM_LED_BRIGHTNESS);
}

void toggleLed(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopToggle(context, PARAM_LED_ENABLE);
}

void setModCalLowerFreq(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint32(context, PARAM_MOD_CAL_LOWER_FREQ);
}

void setModCalUpperFreq(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint32(context, PARAM_MOD_CAL_UPPER_FREQ);
}

// TODO: implement
void updateTvr(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void modCalibration(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  if (print_event_handle == NULL) return;

  osEventFlagsSet(print_event_handle, MESS_FREQ_RESP);

  context->state->state = PARAM_STATE_COMPLETE;
}

// TODO: implement
void exportModCalibration(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void tuneMatchingNetwork(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void updateOcrr(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void updateVmax(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopFloat(context, PARAM_MAX_TRANSDUCER_VOLTAGE);
}

// TODO: implement
void toggleModFeedback(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void setModFeedbackRatio(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void setModOutputPower(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopFloat(context, PARAM_MODULATION_TARGET_POWER);
}

void setTransducerR(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopFloat(context, PARAM_R);
}

void setTransducerC0(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopFloat(context, PARAM_C0);
}

void setTransducerL0(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopFloat(context, PARAM_L0);
}

void setTransducerC1(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopFloat(context, PARAM_C1);
}

// TODO: implement
void setDemodCalRatio(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void performDemodCal(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void setDemodCalLowerFreq(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopUint32(context, PARAM_DEMOD_CAL_LOWER_FREQ);
}

void setDemodCalUpperFreq(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_LoopUint32(context, PARAM_DEMOD_CAL_UPPER_FREQ);
}

// TODO: implement
void exportDemodCal(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void setMacProtocol(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"No MAC", "GA CSMA/CA with BEB (JANUS)"};

  COMMLoops_LoopEnum(context, PARAM_MAC, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setID(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint8(context, PARAM_ID);
}

void setStationaryFlag(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopToggle(context, PARAM_STATIONARY_FLAG);
}
