#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "nbgl_use_case.h"
#include "bolos_target.h"
#include "icons.h"

#ifndef FIRST_USER_TOKEN
#define FIRST_USER_TOKEN 0
#endif

#define NB_INFO_FIELDS 2
#define PAGE_START 0
#define NB_PAGE_SETTING 2
#define IS_TOUCHABLE false

// Forward declaration
static uint8_t initSettingPage;
static void controls_callback(int token, uint8_t index, int page);

enum {
    SWITCH_CONTRACT_DATA_SET_TOKEN = FIRST_USER_TOKEN,
    SWITCH_DEBUG_DATA_SET_TOKEN,
    NB_SETTINGS_SWITCHES,
};

static nbgl_contentSwitch_t switches[NB_SETTINGS_SWITCHES - FIRST_USER_TOKEN];

static const char* const infoTypes[NB_INFO_FIELDS] = {"Version", "Celo App"};
static const char* const infoContents[NB_INFO_FIELDS] = {APPVERSION, "(c) 2022 Ledger"};

static void onQuitCallback(void) {
    os_sched_exit(-1);
}

static const nbgl_contentInfoList_t infoList = {
    .nbInfos = NB_INFO_FIELDS,
    .infoTypes = (const char**) infoTypes,
    .infoContents = (const char**) infoContents,
};


// settings menu definition
#define SETTING_CONTENTS_NB 1
static const nbgl_content_t contents[SETTING_CONTENTS_NB] = {
    {.type = SWITCHES_LIST,
     .content.switchesList.nbSwitches = NB_SETTINGS_SWITCHES - FIRST_USER_TOKEN,
     .content.switchesList.switches = (nbgl_layoutSwitch_t*) switches,
     .contentActionCallback = controls_callback}};

static const nbgl_genericContents_t settingContents = {.callbackCallNeeded = false,
                                                       .contentsList = contents,
                                                       .nbContents = SETTING_CONTENTS_NB};

static void switch_settings_contract_data() {
  uint8_t value = (N_storage.dataAllowed ? 0 : 1);
  nvm_write(&N_storage.dataAllowed, (void*)&value, sizeof(uint8_t));
}

static void switch_settings_display_data() {
  uint8_t value = (N_storage.contractDetails ? 0 : 1);
  nvm_write(&N_storage.contractDetails, (void*)&value, sizeof(uint8_t));
}
static void controls_callback(int token, uint8_t index, int page) {
    UNUSED(index);
    initSettingPage = page;
    switch(token)
    {
        case SWITCH_CONTRACT_DATA_SET_TOKEN:
            switch_settings_contract_data();
            break;

        case SWITCH_DEBUG_DATA_SET_TOKEN:
            switch_settings_display_data();
            break;

        default:
            PRINTF("Should not happen !");
            break;
    }

    switches[0].initState = N_storage.dataAllowed;
    switches[1].initState = N_storage.contractDetails;
}

void ui_idle(void) {
        switches[0].initState = N_storage.dataAllowed;
        switches[0].text = "Contract data";
        switches[0].subText = "Allow contract data\nin transactions";
        switches[0].token = SWITCH_CONTRACT_DATA_SET_TOKEN;
#ifdef HAVE_PIEZO_SOUND
        switches[0].tuneId = TUNE_TAP_CASUAL;
#endif
        switches[1].initState = N_storage.contractDetails;
        switches[1].text = "Debug data";
        switches[1].subText = "Display contract data details";
        switches[1].token = SWITCH_DEBUG_DATA_SET_TOKEN;
#ifdef HAVE_PIEZO_SOUND
        switches[1].tuneId = TUNE_TAP_CASUAL;
#endif

    nbgl_useCaseHomeAndSettings(
        "Celo",
        &ICON_APP_CELO,
        NULL,
        INIT_HOME_PAGE,
        &settingContents,
        &infoList,
        NULL,
        onQuitCallback
    );
}

#endif // HAVE_NBGL
