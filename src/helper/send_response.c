#include <stddef.h>
#include <stdint.h>
#include "constants.h"
#include "globals.h"
#include "sw.h"
#include "send_response.h"
#include "io.h"

int helper_send_response_pubkey() {
    uint8_t resp[1 + PUBKEY_LEN + 1 + CHAINCODE_LEN] = {0};
    size_t offset = 0;
    resp[offset++] = PUBKEY_LEN;
    memcpy(resp + offset, tmpCtx.publicKeyContext.publicKey.W, PUBKEY_LEN);
    offset += PUBKEY_LEN;
    resp[offset++] = CHAINCODE_LEN;
    memcpy(resp + offset, tmpCtx.publicKeyContext.address, CHAINCODE_LEN);
    offset += 40;
    if (tmpCtx.publicKeyContext.getChaincode) {
        memcpy(resp + offset, tmpCtx.publicKeyContext.chainCode, CHAINCODE_LEN);
        offset += CHAINCODE_LEN;
    }
    return io_send_response_pointer(resp, offset, SW_OK);
}

int helper_send_response_signature(const uint8_t *signature, unsigned int info, bool sign_message) {
    uint8_t out[SIGNATURE_LEN] = {0};
    // For EIP1559 and CIP64 transactions, the Ledger SDK expects v to be
    // the parity: 0 | 1
    uint8_t offset = 0;
    if (sign_message) {
        out[offset] = 27;
        if (info & CX_ECCINFO_xGTn) {
            out[offset] += 2;
        }
    } else {
        out[offset] = 0;
    }
    if (info & CX_ECCINFO_PARITY_ODD) {
        out[offset]++;
    }
    offset++;
    uint8_t xoffset = 4;  // point to r value
    // copy r
    uint8_t xlength = signature[xoffset - 1];
    if (xlength == 33) {
        xlength = 32;
        xoffset++;
    }
    memmove(out + offset + 32 - xlength, signature + xoffset, xlength);
    offset += 32;
    xoffset += xlength + 2;  // move over rvalue and TagLEn
    // copy s value
    xlength = signature[xoffset - 1];
    if (xlength == 33) {
        xlength = 32;
        xoffset++;
    }
    memmove(out + offset + 32 - xlength, signature + xoffset, xlength);
    return io_send_response_pointer(out, SIGNATURE_LEN, SW_OK);
}