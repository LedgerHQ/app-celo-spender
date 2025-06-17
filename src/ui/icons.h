#pragma once

#include <stdbool.h>  // bool

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define ICON_APP_CELO        C_celo_14px
#define ICON_APP_WARNING     C_icon_warning
#elif defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_CELO        C_celo_64px
#define ICON_APP_WARNING     C_Warning_64px
#endif

