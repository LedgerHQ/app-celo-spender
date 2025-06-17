#pragma once

#include "ethUstream.h"
#include "types.h"
#include "cx.h"
#include "sw.h"

#ifdef HAVE_NBGL
#include "nbgl_types.h"

// is this isefull ? 
// #ifndef FIRST_USER_TOKEN
// #define FIRST_USER_TOKEN 0
// #endif 

#endif // HAVE_NBGL

#define CHAINID_UPCASE "CELO"
#define CHAINID_COINNAME "CELO"
#define CHAIN_ID 0

extern tmpContent_t tmpContent;

extern txContext_t txContext;

extern tmpCtx_t tmpCtx;

extern strings_t strings;

extern volatile bool dataPresent;
extern volatile uint8_t dataAllowed;
extern volatile uint8_t contractDetails;

extern const internalStorage_t N_storage_real;
#define N_storage (*(internalStorage_t*) PIC(&N_storage_real))

extern char addressSummary[32];
extern cx_sha3_t sha3;

extern volatile provision_type_t provisionType;

extern dataContext_t dataContext;
