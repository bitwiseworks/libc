/* emx/float.h (emx+gcc) */

#ifndef _EMX_FLOAT_H
#define _EMX_FLOAT_H

#if defined (__cplusplus)
extern "C" {
#endif

#define FX_P_NAN       1
#define FX_N_NAN       3
#define FX_P_NORMAL    4
#define FX_P_INFINITY  5
#define FX_N_NORMAL    6
#define FX_N_INFINITY  7
#define FX_P_ZERO      8
#define FX_P_EMPTY     9
#define FX_N_ZERO     10
#define FX_N_EMPTY    11
#define FX_P_DENORMAL 12
#define FX_N_DENORMAL 14

#define DTOA_PRINTF_E   0
#define DTOA_PRINTF_F   1
#define DTOA_PRINTF_G   2
#define DTOA_GCVT       3

const char * __legacy_atod (long double *p_result, const char *string,
                            int min_exp, int max_exp, int bias,
                            int mant_dig, int decimal_dig,
                            int max_10_exp, int min_den_10_exp);

char *__legacy_dtoa (char *buffer, int *p_exp, long double x, int ndigits,
                     int fmt, int dig);

void __remove_zeros (char *digits, int keep);

int _fxam (double);
int _fxaml (long double);


#if defined (__cplusplus)
}
#endif

#endif /* not _EMX_FLOAT_H */
