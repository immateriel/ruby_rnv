/* $Id: rnc.h,v 1.13 2004/01/02 20:27:23 dvd Exp $ */

#include <stdarg.h>
#include "type.h"

#ifndef RNC_H
#define RNC_H 1

#define RNC_ER_IO 0
#define RNC_ER_UTF 10
#define RNC_ER_XESC 20
#define RNC_ER_LEXP 30
#define RNC_ER_LLIT 31
#define RNC_ER_LILL 32
#define RNC_ER_SEXP 40
#define RNC_ER_SILL 41
#define RNC_ER_NOTGR 42
#define RNC_ER_EXT 50
#define RNC_ER_DUPNS 51
#define RNC_ER_DUPDT 52
#define RNC_ER_DFLTNS 53
#define RNC_ER_DFLTDT 54
#define RNC_ER_NONS 55
#define RNC_ER_NODT 56
#define RNC_ER_NCEX 57
#define RNC_ER_2HEADS 58
#define RNC_ER_COMBINE 59
#define RNC_ER_OVRIDE 60
#define RNC_ER_EXPT 61
#define RNC_ER_INCONT 62
#define RNC_ER_NOSTART 70
#define RNC_ER_UNDEF 71

extern void rnc_default_verror_handler(void *data, int erno, int (*handler)(void *data, int erno,char *format, va_list ap), va_list ap);

extern void rnc_init(rnc_st_t *rnc_st);
extern void rnc_dispose(rnc_st_t *rnc_st);

extern int rnc_open(struct rnc_source *sp,char *fn);
extern int rnc_stropen(struct rnc_source *sp,char *fn,char *s,int len);
extern int rnc_bind(struct rnc_source *sp,char *fn,int fd);
extern int rnc_close(struct rnc_source *sp);

extern int rnc_parse(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, struct rnc_source *sp);

extern int rnc_errors(struct rnc_source *sp);

#endif
