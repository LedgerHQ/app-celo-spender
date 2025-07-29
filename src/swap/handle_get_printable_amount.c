#ifdef HAVE_SWAP
#include <string.h>  // memset, explicit_bzero
#include "swap.h"
#include "globals.h"
#include "swap_utils.h"
#include "ethUtils.h"


int allzeroes(const void *buf, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    for (size_t i = 0; i < n; ++i) {
        if (p[i]) {
            return 0;
        }
    }
    return 1;
}

bool uint256_to_decimal(const uint8_t *value,
                        size_t value_len,
                        char *out,
                        size_t out_len) {
    if (value_len > INT256_LENGTH) {
        // value len is bigger than INT256_LENGTH ?!
        return false;
    }

    uint16_t n[16] = {0};
    // Copy and right-align the number
    memcpy((uint8_t *) n + INT256_LENGTH - value_len, value, value_len);

    // Special case when value is 0
    if (allzeroes(n, INT256_LENGTH)) {
        if (out_len < 2) {
            // Not enough space to hold "0" and \0.
            return false;
        }
        strlcpy(out, "0", out_len);
        return true;
    }

    uint16_t *p = n;
    for (int i = 0; i < 16; i++) {
        n[i] = __builtin_bswap16(*p++);
    }
    int pos = out_len;
    while (!allzeroes(n, sizeof(n))) {
        if (pos == 0) {
            return false;
        }
        pos -= 1;
        unsigned int carry = 0;
        for (int i = 0; i < 16; i++) {
            int rem = ((carry << 16) | n[i]) % 10;
            n[i] = ((carry << 16) | n[i]) / 10;
            carry = rem;
        }
        out[pos] = '0' + carry;
    }
    memmove(out, out + pos, out_len - pos);
    out[out_len - pos] = 0;
    return true;
}

bool amountToString(const uint8_t *amount,
                    uint8_t amount_size,
                    uint8_t decimals,
                    const char *ticker,
                    char *out_buffer,
                    size_t out_buffer_size) {
    char tmp_buffer[100] = {0};

    if (uint256_to_decimal(amount,
                           amount_size,
                           tmp_buffer,
                           sizeof(tmp_buffer)) == false) {
        return false;
    }

    uint8_t amount_len = strnlen(tmp_buffer, sizeof(tmp_buffer));
    uint8_t ticker_len = strnlen(ticker, MAX_TICKER_LEN);

    if (ticker_len > 0) {
        if (out_buffer_size <= ticker_len + 1) {
            return false;
        }
        memcpy(out_buffer, ticker, ticker_len);
        out_buffer[ticker_len++] = ' ';
    }

    if (adjustDecimals(tmp_buffer,
                       amount_len,
                       out_buffer + ticker_len,
                       out_buffer_size - ticker_len - 1,
                       decimals) == false) {
        return false;
    }

    out_buffer[out_buffer_size - 1] = '\0';
    return true;
}

/* Set empty printable_amount on error, printable amount otherwise */
void swap_handle_get_printable_amount(get_printable_amount_parameters_t* params) {    
    uint8_t decimals;
    char ticker[MAX_TICKER_LEN] = {0};

    if (params->is_fee || params->coin_configuration == NULL) {
        memcpy(ticker, "CELO", sizeof("CELO"));
        decimals = CELO_PRECISION;
    } else {
        if (!swap_parse_config(params->coin_configuration,
                               params->coin_configuration_length,
                               ticker,
                               sizeof(ticker),
                               &decimals)) {
            PRINTF("Fail to parse coin_configuration\n");
            goto error;
        }
    }

    memset(params->printable_amount, 0, sizeof(params->printable_amount));
    if (params->amount_length > 32) {
        PRINTF("Amount is too big, 32 bytes max but buffer has %u bytes", params->amount_length);
        goto error;
    }
    if (!amountToString(params->amount,
                        params->amount_length,
                        decimals,
                        ticker,
                        params->printable_amount,
                        sizeof(params->printable_amount))) {
        memset(params->printable_amount, 0, sizeof(params->printable_amount));
        goto error;
    }

    PRINTF("Amount %s\n", params->printable_amount);
    return; 
error:
    memset(params->printable_amount, '\0', sizeof(params->printable_amount));    
}

#endif // HAVE_SWAP