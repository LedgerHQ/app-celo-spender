/**
 * @file strcase.h
 * @brief Case-insensitive string comparison.
 *
 * Kept in its own translation unit, free of SDK dependencies, so that the code
 * comparing addresses and tickers can be compiled and tested on the host.
 */

#ifndef _STRCASE_H_
#define _STRCASE_H_

/**
 * @brief Compares two NUL-terminated strings ignoring case.
 *
 * @return 0 when equal, non-zero otherwise.
 */
int _strcasecmp(const char *s1, const char *s2);

#endif /* _STRCASE_H_ */
