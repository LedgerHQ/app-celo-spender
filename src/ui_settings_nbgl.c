#ifdef HAVE_NBGL
#include "globals.h"
#include "ui_common.h"
#include "nbgl_use_case.h"
#include "bolos_target.h"

#define NB_INFO_FIELDS 2
#define PAGE_START 0
#define NB_PAGE_SETTING 2
#define IS_TOUCHABLE true

// Forward declaration
static void displaySettingsMenu(void);
static void settingsControlsCallback(int token, uint8_t index);
static bool settingsNavCallback(uint8_t page, nbgl_pageContent_t *content);

enum {
    SWITCH_CONTRACT_DATA_SET_TOKEN = FIRST_USER_TOKEN,
    SWITCH_DEBUG_DATA_SET_TOKEN,
    NB_SETTINGS_SWITCHES,
};

static nbgl_layoutSwitch_t switches[NB_SETTINGS_SWITCHES - FIRST_USER_TOKEN];

static const char* const infoTypes[] = {"Version", "Celo App"};
static const char* const infoContents[] = {APPVERSION, "(c) 2022 Ledger"};

static void onQuitCallback(void) {
    os_sched_exit(-1);
}

static bool settingsNavCallback(uint8_t page, nbgl_pageContent_t *content) {
    if (page == 0) {
        switches[0].text = "Contract data";
        switches[0].subText = "Allow contract data\nin transactions";
        switches[0].token = SWITCH_CONTRACT_DATA_SET_TOKEN;
        switches[0].tuneId = TUNE_TAP_CASUAL;
        switches[0].initState = N_storage.dataAllowed;

        switches[1].text = "Debug data";
        switches[1].subText = "Display contract data details";
        switches[1].token = SWITCH_DEBUG_DATA_SET_TOKEN;
        switches[1].tuneId = TUNE_TAP_CASUAL; 
        switches[1].initState = N_storage.contractDetails;

        content->type = SWITCHES_LIST;
        content->switchesList.nbSwitches = NB_SETTINGS_SWITCHES - FIRST_USER_TOKEN,
        content->switchesList.switches = (nbgl_layoutSwitch_t*) switches;
    }
    else if (page == 1) {
        content->type = INFOS_LIST;
        content->infosList.nbInfos = NB_INFO_FIELDS;
        content->infosList.infoTypes = (const char**) infoTypes;
        content->infosList.infoContents = (const char**) infoContents;
    }
    else {
        return false;
    }
    return true;
}

static void switch_settings_contract_data() {
  uint8_t value = (N_storage.dataAllowed ? 0 : 1);
  nvm_write(&N_storage.dataAllowed, (void*)&value, sizeof(uint8_t));
}

static void switch_settings_display_data() {
  uint8_t value = (N_storage.contractDetails ? 0 : 1);
  nvm_write(&N_storage.contractDetails, (void*)&value, sizeof(uint8_t));
}

static void settingsControlsCallback(int token, uint8_t index) {
    UNUSED(index);
    switch(token)
    {
        case SWITCH_CONTRACT_DATA_SET_TOKEN:
            switches[0].initState = N_storage.dataAllowed;
            switch_settings_contract_data();
            break;   

        case SWITCH_DEBUG_DATA_SET_TOKEN:
            switches[1].initState = N_storage.contractDetails;
            switch_settings_display_data();
            break;

        default:
            PRINTF("Should not happen !");
            break;
    }
    displaySettingsMenu();
}

static void displaySettingsMenu(void) {
    nbgl_useCaseSettings("Celo settings", PAGE_START, NB_PAGE_SETTING, IS_TOUCHABLE, ui_idle, 
            settingsNavCallback, settingsControlsCallback);
}

void ui_idle(void) {
    nbgl_useCaseHome("Celo", &C_celo_32px, 
            "Go to Ledger Live to create a\ntransaction. You will approve it\non Stax.", true, 
            displaySettingsMenu, onQuitCallback);
}

#endif // HAVE_NBGL
