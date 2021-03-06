/* $Id: rnl.h,v 1.1 2004/01/10 14:25:05 dvd Exp $ */

#ifndef RNL_H
#define RNL_H 1

#include <stdarg.h>
#include "type.h"

extern void rnl_default_verror_handler(void *data, int erno, int (*handler)(void *data, int erno,char *format, va_list ap), va_list ap);

extern void rnl_init(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st);

extern int rnl_fn(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, char *fn);
extern int rnl_fd(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, char *fn,int fd);
extern int rnl_s(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, char *fn,char *s,int len);

#endif
