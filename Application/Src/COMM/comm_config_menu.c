/*
 * comm_config_menu.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
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

void setErrorDetection(void* argument);
void setPremableEcc(void* argument);
void setMessageEcc(void* argument);
void setModulationMethod(void* argument);
void setFskF0(void* argument);
void setFskF1(void* argument);
void setFhbfskFreqSpacing(void* argument);
void setFhbfskDwell(void* argument);
void setFhbfskTones(void* argument);
void setFhbfskHopper(void* argument);
void setBaudRate(void* argument);
void setCenterFrequency(void* argument);
void getBitPeriod(void* argument);
void getBandwidth(void* argument);
void toggleInterleaver(void* argument);
void setSynchronizer(void* argument);
void printConfigOptions(void* argument);
void importConfiOptions(void* argument);
void setDacTransitionDuration(void* argument);
void setModPowerControlMethod(void* argument);
void setModFixedOutput(void* argument);
void setMessageStartFunction(void* argument);
void setBitDecisionFunction(void* argument);
void setHistoricalComparisonThreshold(void* argument);
void toggleAgc(void* argument);
void setFixedPgaGain(void* argument);
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
void setID(void* argument);
void setStationaryFlag(void* argument);

/* Private variables ---------------------------------------------------------*/

/* Main menu starting point --------------------------------------------------*/

static MenuID_t configMenuChildren[] = {
  MENU_ID_CFG_UNIV, MENU_ID_CFG_MOD,    MENU_ID_CFG_DEMOD,      MENU_ID_CFG_DAU, 
  MENU_ID_CFG_LED,  MENU_ID_CFG_SETID,  MENU_ID_CFG_STATIONARY
};
static const MenuNode_t configMenu = {
  .id = MENU_ID_CFG,
  .description = "Configuration Parameters Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = configMenuChildren,
  .num_children = sizeof(configMenuChildren) / sizeof(configMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

/* Sub menus -----------------------------------------------------------------*/

static MenuID_t univConfigMenuChildren[] = {
  MENU_ID_CFG_UNIV_ERR,         MENU_ID_CFG_UNIV_ECCPREAMBLE, 
  MENU_ID_CFG_UNIV_ECCMESSAGE,  MENU_ID_CFG_UNIV_MOD,      
  MENU_ID_CFG_UNIV_FSK,         MENU_ID_CFG_UNIV_FHBFSK,  
  MENU_ID_CFG_UNIV_BAUD,        MENU_ID_CFG_UNIV_FC,    
  MENU_ID_CFG_UNIV_BP,          MENU_ID_CFG_UNIV_BANDWIDTH, 
  MENU_ID_CFG_UNIV_INTERLEAVER, MENU_ID_CFG_UNIV_SYNC,
  MENU_ID_CFG_UNIV_EXP,         MENU_ID_CFG_UNIV_IMP
};
static const MenuNode_t univConfigMenu = {
  .id = MENU_ID_CFG_UNIV,
  .description = "Universal Waveform Processing Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = univConfigMenuChildren,
  .num_children = sizeof(univConfigMenuChildren) / sizeof(univConfigMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t modConfigMenuChildren[] = {
  MENU_ID_CFG_MOD_TLEN,   MENU_ID_CFG_MOD_CAL, 
  MENU_ID_CFG_MOD_FB,     MENU_ID_CFG_MOD_METHOD,
  MENU_ID_CFG_MOD_FIXED,  MENU_ID_CFG_MOD_PWROPT
};
static const MenuNode_t modConfigMenu = {
  .id = MENU_ID_CFG_MOD,
  .description = "Modulation Waveform Processing Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = modConfigMenuChildren,
  .num_children = sizeof(modConfigMenuChildren) / sizeof(modConfigMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t demodConfigMenuChildren[] = {
  MENU_ID_CFG_DEMOD_CAL,       MENU_ID_CFG_DEMOD_START, 
  MENU_ID_CFG_DEMOD_DECISION,  MENU_ID_CFG_DEMOD_CMPTHRESH, 
  MENU_ID_CFG_DEMOD_AGCEN,     MENU_ID_CFG_DEMOD_GAIN
};
static const MenuNode_t demodConfigMenu = {
  .id = MENU_ID_CFG_DEMOD,
  .description = "Demodulation Waveform Processing Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = demodConfigMenuChildren,
  .num_children = sizeof(demodConfigMenuChildren) / sizeof(demodConfigMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t dauConfigMenuChildren[] = {
  MENU_ID_CFG_DAU_SLEEP
};
static const MenuNode_t dauConfigMenu = {
  .id = MENU_ID_CFG_DAU,
  .description = "Daughter Card Communication Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = dauConfigMenuChildren,
  .num_children = sizeof(dauConfigMenuChildren) / sizeof(dauConfigMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t ledConfigMenuChildren[] = {
  MENU_ID_CFG_LED_BRIGHTNESS, MENU_ID_CFG_LED_EN
};
static const MenuNode_t ledConfigMenu = {
  .id = MENU_ID_CFG_LED,
  .description = "LED Configuration Parameters",
  .handler = NULL,
  .parent_id = MENU_ID_CFG,
  .children_ids = ledConfigMenuChildren,
  .num_children = sizeof(ledConfigMenuChildren) / sizeof(ledConfigMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t setNewIdParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_SETID
};
static const MenuNode_t setNewId = {
  .id = MENU_ID_CFG_SETID,
  .description = "Set Modem ID",
  .handler = setID,
  .parent_id = MENU_ID_CFG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &setNewIdParam
};

static ParamContext_t setStationaryParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_STATIONARY
};
static const MenuNode_t setStationary = {
  .id = MENU_ID_CFG_STATIONARY,
  .description = "Toggle Stationary Flag",
  .handler = setStationaryFlag,
  .parent_id = MENU_ID_CFG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &setStationaryParam
};

/* Sub sub menus -------------------------------------------------------------*/

static ParamContext_t univConfigErrParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ERR
};
static const MenuNode_t univConfigErr = {
  .id = MENU_ID_CFG_UNIV_ERR,
  .description = "Error Detection Scheme",
  .handler = setErrorDetection,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigErrParam
};

static ParamContext_t univConfigEccPreambleParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ECCPREAMBLE
};
static const MenuNode_t univConfigEccPreamble = {
  .id = MENU_ID_CFG_UNIV_ECCPREAMBLE,
  .description = "Preamble ECC",
  .handler = setPremableEcc,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigEccPreambleParam
};

static ParamContext_t univConfigEccMessageParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_ECCMESSAGE
};
static const MenuNode_t univConfigEccMessage = {
  .id = MENU_ID_CFG_UNIV_ECCMESSAGE,
  .description = "Message ECC",
  .handler = setMessageEcc,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigEccMessageParam
};

static ParamContext_t univConfigModParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_MOD,
};
static const MenuNode_t univConfigMod = {
  .id = MENU_ID_CFG_UNIV_MOD,
  .description = "Modulation Method",
  .handler = setModulationMethod,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigModParam
};

static MenuID_t univConfigFskChildren[] = {
  MENU_ID_CFG_UNIV_FSK_F0, MENU_ID_CFG_UNIV_FSK_F1
};
static const MenuNode_t univConfigFskMenu = {
  .id = MENU_ID_CFG_UNIV_FSK,
  .description = "FSK Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = univConfigFskChildren,
  .num_children = sizeof(univConfigFskChildren) / sizeof(univConfigFskChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t univConfigFhbfskChildren[] = {
  MENU_ID_CFG_UNIV_FHBFSK_FSEP,  MENU_ID_CFG_UNIV_FHBFSK_DWELL,
  MENU_ID_CFG_UNIV_FHBFSK_TONES, MENU_ID_CFG_UNIV_FHBFSK_HOPP
};
static const MenuNode_t univConfigFhbskMenu = {
  .id = MENU_ID_CFG_UNIV_FHBFSK,
  .description = "FHBFSK Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = univConfigFhbfskChildren,
  .num_children = sizeof(univConfigFhbfskChildren) / sizeof(univConfigFhbfskChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t univConfigBaudParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_BAUD
};
static const MenuNode_t univConfigBaud = {
  .id = MENU_ID_CFG_UNIV_BAUD,
  .description = "Configure Baud Rate",
  .handler = setBaudRate,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigBaudParam
};

static ParamContext_t univConfigFcParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FC
};
static const MenuNode_t univConfigFc = {
  .id = MENU_ID_CFG_UNIV_FC,
  .description = "Set Center Frequency",
  .handler = setCenterFrequency,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigFcParam
};

static ParamContext_t univConfigBitPeriodParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_BP
};
static const MenuNode_t univConfigBitPeriod = {
  .id = MENU_ID_CFG_UNIV_BP,
  .description = "Get Bit Period",
  .handler = getBitPeriod,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigBitPeriodParam
};

static ParamContext_t univConfigBandwidthParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_BANDWIDTH
};
static const MenuNode_t univConfigBandwidth = {
  .id = MENU_ID_CFG_UNIV_BANDWIDTH,
  .description = "Get Bandwidth",
  .handler = getBandwidth,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigBandwidthParam
};

static ParamContext_t univConfigInterleaverParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_INTERLEAVER
};
static const MenuNode_t univConfigInterleaver = {
  .id = MENU_ID_CFG_UNIV_INTERLEAVER,
  .description = "Toggle Interleaver",
  .handler = toggleInterleaver,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigInterleaverParam
};

static ParamContext_t univConfigSyncParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_SYNC
};
static const MenuNode_t univConfigSync = {
  .id = MENU_ID_CFG_UNIV_SYNC,
  .description = "Set Synchronization Method",
  .handler = setSynchronizer,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigSyncParam
};

static ParamContext_t univConfigExportParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_EXP  
};
static const MenuNode_t univConfigExport = {
  .id = MENU_ID_CFG_UNIV_EXP,
  .description = "Export Configuration Options Used",
  .handler = printConfigOptions,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigExportParam
};

static ParamContext_t univConfigImportParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_IMP
};
static const MenuNode_t univConfigImport = {
  .id = MENU_ID_CFG_UNIV_IMP,
  .description = "Import Configuration options",
  .handler = importConfiOptions,
  .parent_id = MENU_ID_CFG_UNIV,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univConfigImportParam
};

static ParamContext_t modConfigDacTransitionParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_TLEN
};
static const MenuNode_t modConfigDacTransition = {
  .id = MENU_ID_CFG_MOD_TLEN,
  .description = "Set DAC Transition Duration",
  .handler = setDacTransitionDuration,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modConfigDacTransitionParam
};

static MenuID_t modConfigCalChildren[] = {
  MENU_ID_CFG_MOD_CAL_LOWFREQ,  MENU_ID_CFG_MOD_CAL_HIFREQ, 
  MENU_ID_CFG_MOD_CAL_TVR,      MENU_ID_CFG_MOD_CAL_RECV, 
  MENU_ID_CFG_MOD_CAL_PERFORM,  MENU_ID_CFG_MOD_CAL_EXP, 
  MENU_ID_CFG_MOD_CAL_TUNE,     MENU_ID_CFG_MOD_CAL_VMAX
};
static const MenuNode_t modConfigCalMenu = {
  .id = MENU_ID_CFG_MOD_CAL,
  .description = "Modulation Calibration Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = modConfigCalChildren,
  .num_children = sizeof(modConfigCalChildren) / sizeof(modConfigCalChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t modConfigFeedbackChildren[] = {
  MENU_ID_CFG_MOD_FB_EN, MENU_ID_CFG_MOD_FB_RATIO
};
static const MenuNode_t modConfigFeedbackMenu = {
  .id = MENU_ID_CFG_MOD_FB,
  .description = "Modulation Feedback Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = modConfigFeedbackChildren,
  .num_children = sizeof(modConfigFeedbackChildren) / sizeof(modConfigFeedbackChildren[0]),
  .access_level = 0,
  .parameters = NULL,
};

static ParamContext_t modConfigMethodParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_METHOD
};
static const MenuNode_t modConfigMethod = {
  .id = MENU_ID_CFG_MOD_METHOD,
  .description = "Set method to control output strength",
  .handler = setModPowerControlMethod,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modConfigMethodParam
};

static ParamContext_t modConfigFixedParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_FIXED
};
static const MenuNode_t modConfigFixed = {
  .id = MENU_ID_CFG_MOD_FIXED,
  .description = "Fixed relative DAC output strength to use",
  .handler = setModFixedOutput,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modConfigFixedParam
};

static MenuID_t modConfigPowerChildren[] = {
  MENU_ID_CFG_MOD_PWROPT_PWR, MENU_ID_CFG_MOD_PWROPT_R, 
  MENU_ID_CFG_MOD_PWROPT_C0,  MENU_ID_CFG_MOD_PWROPT_L0,
  MENU_ID_CFG_MOD_PWROPT_C1
};
static const MenuNode_t modConfigPowerMenu = {
  .id = MENU_ID_CFG_MOD_PWROPT,
  .description = "Fixed output power level options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_MOD,
  .children_ids = modConfigPowerChildren,
  .num_children = sizeof(modConfigPowerChildren) / sizeof(modConfigPowerChildren[0]),
  .access_level = 0,
  .parameters = NULL,
};

static MenuID_t demodConfigCalChildren[] = {
  MENU_ID_CFG_DEMOD_CAL_RATIO,     MENU_ID_CFG_DEMOD_CAL_PERFORM,
  MENU_ID_CFG_DEMOD_CAL_LOWFREQ,   MENU_ID_CFG_DEMOD_CAL_HIFREQ,
  MENU_ID_CFG_DEMOD_CAL_EXP
};
static const MenuNode_t demodConfigCalMenu = {
  .id = MENU_ID_CFG_DEMOD_CAL,
  .description = "Demodulation Cablibration Options",
  .handler = NULL,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = demodConfigCalChildren,
  .num_children = sizeof(demodConfigCalChildren) / sizeof(demodConfigCalChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t demodConfigStartFcnParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_START
};
static const MenuNode_t demodConfigStartFcn = {
  .id = MENU_ID_CFG_DEMOD_START,
  .description = "Select the message start function to use",
  .handler = setMessageStartFunction,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodConfigStartFcnParam
};

static ParamContext_t demodConfigDecisionFcnParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_DECISION
};
static const MenuNode_t demodConfigDecisionFcn = {
  .id = MENU_ID_CFG_DEMOD_DECISION,
  .description = "Select the bit decision function to use",
  .handler = setBitDecisionFunction,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodConfigDecisionFcnParam
};

static ParamContext_t demodConfigSigShiftParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CMPTHRESH
};
static const MenuNode_t demodConfigSigShift = {
  .id = MENU_ID_CFG_DEMOD_CMPTHRESH,
  .description = "Set the historical comparison threshold",
  .handler = setHistoricalComparisonThreshold,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodConfigSigShiftParam
};

static ParamContext_t demodConfigUseAgcParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_AGCEN
};
static const MenuNode_t demodConfigUseAgc = {
  .id = MENU_ID_CFG_DEMOD_AGCEN,
  .description = "Enable/disable automatic gain control (AGC)",
  .handler = toggleAgc,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodConfigUseAgcParam
};

static ParamContext_t demodConfigFixedGainParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_GAIN
};
static const MenuNode_t demodConfigFixedGain = {
  .id = MENU_ID_CFG_DEMOD_GAIN,
  .description = "Set fixed PGA gain",
  .handler = setFixedPgaGain,
  .parent_id = MENU_ID_CFG_DEMOD,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodConfigFixedGainParam
};

static ParamContext_t dauConfigSleepParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DAU_SLEEP
};
static const MenuNode_t dauConfigSleep = {
  .id = MENU_ID_CFG_DAU_SLEEP,
  .description = "Enable or Disable Sleep Modes",
  .handler = configureSleep,
  .parent_id = MENU_ID_CFG_DAU,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &dauConfigSleepParam
};

static ParamContext_t ledConfigBrightnessParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_LED_BRIGHTNESS
};
static const MenuNode_t ledConfigBrightness = {
  .id = MENU_ID_CFG_LED_BRIGHTNESS,
  .description = "Set LED Brightness",
  .handler = setLedBrightness,
  .parent_id = MENU_ID_CFG_LED,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &ledConfigBrightnessParam
};

static ParamContext_t ledConfigToggleParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_LED_EN
};
static const MenuNode_t ledConfigToggle = {
  .id = MENU_ID_CFG_LED_EN,
  .description = "Toggle LED",
  .handler = toggleLed,
  .parent_id = MENU_ID_CFG_LED,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &ledConfigToggleParam
};

/* Sub sub sub menus ---------------------------------------------------------*/

static ParamContext_t univFskConfigF0Param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FSK_F0
};
static const MenuNode_t univFskConfigF0 = {
  .id = MENU_ID_CFG_UNIV_FSK_F0,
  .description = "Set FSK Frequency '0'",
  .handler = setFskF0,
  .parent_id = MENU_ID_CFG_UNIV_FSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univFskConfigF0Param
};

static ParamContext_t univFskConfigF1Param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FSK_F1
};
static const MenuNode_t univFskConfigF1 = {
  .id = MENU_ID_CFG_UNIV_FSK_F1,
  .description = "Set FSK Frequency '1'",
  .handler = setFskF1,
  .parent_id = MENU_ID_CFG_UNIV_FSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univFskConfigF1Param
};

static ParamContext_t univFhbfskConfigFreqSpacingParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FHBFSK_FSEP
};
static const MenuNode_t univFhbfskConfigFreqSpacing = {
  .id = MENU_ID_CFG_UNIV_FHBFSK_FSEP,
  .description = "Set Frequency Spacing",
  .handler = setFhbfskFreqSpacing,
  .parent_id = MENU_ID_CFG_UNIV_FHBFSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univFhbfskConfigFreqSpacingParam
};

static ParamContext_t univFhbfskConfigDwellParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FHBFSK_DWELL
};
static const MenuNode_t univFhbfskConfigDwell = {
  .id = MENU_ID_CFG_UNIV_FHBFSK_DWELL,
  .description = "Set Number of Bit Dwell Periods",
  .handler = setFhbfskDwell,
  .parent_id = MENU_ID_CFG_UNIV_FHBFSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univFhbfskConfigDwellParam
};

static ParamContext_t univFhbfskConfigTonesParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FHBFSK_TONES
};
static const MenuNode_t univFhbfskConfigTones = {
  .id = MENU_ID_CFG_UNIV_FHBFSK_TONES,
  .description = "Set Number of Tones",
  .handler = setFhbfskTones,
  .parent_id = MENU_ID_CFG_UNIV_FHBFSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univFhbfskConfigTonesParam
};

static ParamContext_t univFhbfskConfigHopperParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_UNIV_FHBFSK_HOPP
};
static const MenuNode_t univFhbfskConfigHopper = {
  .id = MENU_ID_CFG_UNIV_FHBFSK_HOPP,
  .description = "Set Frequency Hopper",
  .handler = setFhbfskHopper,
  .parent_id = MENU_ID_CFG_UNIV_FHBFSK,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &univFhbfskConfigHopperParam
};

static ParamContext_t modCalConfigLowFreqParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_LOWFREQ
};
static const MenuNode_t modCalConfigLowFreq = {
  .id = MENU_ID_CFG_MOD_CAL_LOWFREQ,
  .description = "Lower Frequency for Calibration",
  .handler = setModCalLowerFreq,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modCalConfigLowFreqParam
};

static ParamContext_t modCalConfigUpperFreqParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_HIFREQ
};
static const MenuNode_t modCalConfigUpperFreq = {
  .id = MENU_ID_CFG_MOD_CAL_HIFREQ,
  .description = "Upper Frequency for Calibration",
  .handler = setModCalUpperFreq,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modCalConfigUpperFreqParam
};

static ParamContext_t modCalConfigTvrParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_TVR
};
static const MenuNode_t modCalConfigTvr = {
  .id = MENU_ID_CFG_MOD_CAL_TVR,
  .description = "Import TVR",
  .handler = updateTvr,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modCalConfigTvrParam
};

static ParamContext_t modCalConfigPerformParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_PERFORM
};
static const MenuNode_t modCalConfigPerform = {
  .id = MENU_ID_CFG_MOD_CAL_PERFORM,
  .description = "Perform Calibration",
  .handler = modCalibration,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modCalConfigPerformParam
};

static ParamContext_t modCalConfigExportParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_EXP
};
static const MenuNode_t modCalConfigExport = {
  .id = MENU_ID_CFG_MOD_CAL_EXP,
  .description = "Export Calibration",
  .handler = exportModCalibration,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modCalConfigExportParam
};

static ParamContext_t modCalConfigTuneParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_TUNE
};
static const MenuNode_t modCalConfigTune = {
  .id = MENU_ID_CFG_MOD_CAL_TUNE,
  .description = "Tune Matching Network",
  .handler = tuneMatchingNetwork,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modCalConfigTuneParam
};

static ParamContext_t modCalConfigRecvParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_RECV
};
static const MenuNode_t modCalConfigRecv = {
  .id = MENU_ID_CFG_MOD_CAL_RECV,
  .description = "Import OCRR",
  .handler = updateOcrr,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modCalConfigRecvParam
};

static ParamContext_t modCalConfigVmaxParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_CAL_VMAX
};
static const MenuNode_t modCalConfigVmax = {
  .id = MENU_ID_CFG_MOD_CAL_VMAX,
  .description = "Set Maximum Output Voltage",
  .handler = updateVmax,
  .parent_id = MENU_ID_CFG_MOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modCalConfigVmaxParam
};

static ParamContext_t modFbConfigToggleParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_FB_EN
};
static const MenuNode_t modFbConfigToggle = {
  .id = MENU_ID_CFG_MOD_FB_EN,
  .description = "Toggle Feedback Network",
  .handler = toggleModFeedback,
  .parent_id = MENU_ID_CFG_MOD_FB,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modFbConfigToggleParam
};

static ParamContext_t modFbConfigRatioParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_FB_RATIO
};
static const MenuNode_t modFbConfigRatio = {
  .id = MENU_ID_CFG_MOD_FB_RATIO,
  .description = "Set Feedback Division Ratio",
  .handler = setModFeedbackRatio,
  .parent_id = MENU_ID_CFG_MOD_FB,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modFbConfigRatioParam
};

static ParamContext_t modPwrConfigTargetParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_PWR
};
static const MenuNode_t modPwrConfigTarget = {
  .id = MENU_ID_CFG_MOD_PWROPT_PWR,
  .description = "Target output power (W)",
  .handler = setModOutputPower,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modPwrConfigTargetParam
};

static ParamContext_t modPwrConfigRParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_R
};
static const MenuNode_t modPwrConfigR = {
  .id = MENU_ID_CFG_MOD_PWROPT_R,
  .description = "Series resistance of the motional branch (R) [ohms]",
  .handler = setTransducerR,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modPwrConfigRParam
};

static ParamContext_t modPwrConfigC0Param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_C0
};
static const MenuNode_t modPwrConfigC0 = {
  .id = MENU_ID_CFG_MOD_PWROPT_C0,
  .description = "Series capacitance of the motional branch (C0) [nF]",
  .handler = setTransducerC0,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modPwrConfigC0Param
};

static ParamContext_t modPwrConfigL0Param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_L0
};
static const MenuNode_t modPwrConfigL0 = {
  .id = MENU_ID_CFG_MOD_PWROPT_L0,
  .description = "Series inductance of the motional branch (L0) [mH]",
  .handler = setTransducerL0,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modPwrConfigL0Param
};

static ParamContext_t modPwrConfigC1Param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_MOD_PWROPT_C1
};
static const MenuNode_t modPwrConfigC1 = {
  .id = MENU_ID_CFG_MOD_PWROPT_C1,
  .description = "Parallel capacitance with the motional branch (C1) [nF]",
  .handler = setTransducerC1,
  .parent_id = MENU_ID_CFG_MOD_PWROPT,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &modPwrConfigC1Param
};

static ParamContext_t demodCalConfigRatioParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_RATIO
};
static const MenuNode_t demodCalConfigRatio = {
  .id = MENU_ID_CFG_DEMOD_CAL_RATIO,
  .description = "Set Feedback Network Voltage Division Ratio",
  .handler = setDemodCalRatio,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodCalConfigRatioParam
};

static ParamContext_t demodCalConfigPerformParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_PERFORM
};
static const MenuNode_t demodCalConfigPerform = {
  .id = MENU_ID_CFG_DEMOD_CAL_PERFORM,
  .description = "Perform Calibration",
  .handler = performDemodCal,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodCalConfigPerformParam
};

static ParamContext_t demodCalConfigLowFreqParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_LOWFREQ
};
static const MenuNode_t demodCalConfigLowFreq = {
  .id = MENU_ID_CFG_DEMOD_CAL_LOWFREQ,
  .description = "Lower frequency used for demodulation calibration",
  .handler = setDemodCalLowerFreq,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodCalConfigLowFreqParam
};

static ParamContext_t demodCalConfigUpperFreqParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_HIFREQ,
};
static const MenuNode_t demodCalConfigUpperFreq = {
  .id = MENU_ID_CFG_DEMOD_CAL_HIFREQ,
  .description = "Upper frequency used for demodulation calibration",
  .handler = setDemodCalUpperFreq,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodCalConfigUpperFreqParam
};

static ParamContext_t demodCalConfigExportParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_CFG_DEMOD_CAL_EXP
};
static const MenuNode_t demodCalConfigExport = {
  .id = MENU_ID_CFG_DEMOD_CAL_EXP,
  .description = "Export Calibration Results",
  .handler = exportDemodCal,
  .parent_id = MENU_ID_CFG_DEMOD_CAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &demodCalConfigExportParam
};

/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterConfigurationMenu()
{
  bool ret = registerMenu(&configMenu) && registerMenu(&univConfigMenu) &&
             registerMenu(&modConfigMenu) && registerMenu(&demodConfigMenu) &&
             registerMenu(&dauConfigMenu) && registerMenu(&ledConfigMenu) && 
             registerMenu(&univConfigErr) && registerMenu(&demodConfigDecisionFcn) &&
             registerMenu(&univConfigMod) && registerMenu(&univConfigFskMenu) && 
             registerMenu(&univConfigFhbskMenu) && registerMenu(&univConfigBaud) && 
             registerMenu(&univConfigFc) && registerMenu(&univConfigBitPeriod) && 
             registerMenu(&univConfigExport) && registerMenu(&univConfigImport) && 
             registerMenu(&setStationary) && registerMenu(&modConfigDacTransition) &&
             registerMenu(&modConfigCalMenu) && registerMenu(&modConfigFeedbackMenu) && 
             registerMenu(&modConfigMethod) && registerMenu(&univConfigInterleaver) &&
             registerMenu(&demodConfigCalMenu) && registerMenu(&univFhbfskConfigHopper) &&
             registerMenu(&dauConfigSleep) && registerMenu(&ledConfigBrightness) &&
             registerMenu(&ledConfigToggle) && registerMenu(&modCalConfigLowFreq) &&
             registerMenu(&modCalConfigUpperFreq) && registerMenu(&modCalConfigTvr) && 
             registerMenu(&modCalConfigPerform) && registerMenu(&modCalConfigExport) &&
             registerMenu(&modCalConfigTune) && registerMenu(&modCalConfigRecv) && 
             registerMenu(&modCalConfigVmax) && registerMenu(&modFbConfigToggle) &&
             registerMenu(&modFbConfigRatio) && registerMenu(&demodConfigSigShift) &&
             registerMenu(&demodCalConfigRatio) && registerMenu(&demodCalConfigPerform) && 
             registerMenu(&demodCalConfigLowFreq) && registerMenu(&demodCalConfigUpperFreq) && 
             registerMenu(&demodCalConfigExport) && registerMenu(&univConfigSync) &&
             registerMenu(&demodConfigStartFcn) && registerMenu(&univFskConfigF0) &&
             registerMenu(&univFskConfigF1) && registerMenu(&univFhbfskConfigFreqSpacing) &&
             registerMenu(&univFhbfskConfigDwell) && registerMenu(&univConfigBandwidth) &&
             registerMenu(&univFhbfskConfigTones) && registerMenu(&setNewId) &&
             registerMenu(&modConfigFixed) && registerMenu(&modConfigPowerMenu) &&
             registerMenu(&modPwrConfigTarget) && registerMenu(&modPwrConfigR) &&
             registerMenu(&modPwrConfigC0) && registerMenu(&modPwrConfigL0) &&
             registerMenu(&modPwrConfigC1) && registerMenu(&demodConfigUseAgc) &&
             registerMenu(&demodConfigFixedGain) && registerMenu(&univConfigEccPreamble) &&
             registerMenu(&univConfigEccMessage);

  return ret;
}

/* Private function definitions ----------------------------------------------*/

void setErrorDetection(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  char* descriptors[] = {"None",
                         "CRC-8",      "CRC-16",      "CRC-32", 
                         "Checksum-8", "Checksum-16", "Checksum-32"};

  COMMLoops_LoopEnum(context, PARAM_ERROR_DETECTION, descriptors, 
    sizeof(descriptors) / sizeof(descriptors[0]));
}

void setPremableEcc(void* argument)
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

void importConfiOptions(void* argument)
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
