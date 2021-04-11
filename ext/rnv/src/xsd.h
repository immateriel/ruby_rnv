/* $Id: xsd.h,v 1.14 2005/01/05 09:46:25 dvd Exp $ */

#include <stdarg.h>
#include "type.h"

#ifndef XSD_H
#define XSD_H 1

#define XSD_ER_TYP 0
#define XSD_ER_PAR 1
#define XSD_ER_PARVAL 2
#define XSD_ER_VAL 3
#define XSD_ER_NPAT 4
#define XSD_ER_WS 5
#define XSD_ER_ENUM 6

extern void xsd_default_verror_handler(void *data, int erno, int (*handler)(void *data, int erno,char *format, va_list ap), va_list ap);

extern void xsd_init(rx_st_t *rx_st);

extern int xsd_allows(rx_st_t *rx_st, char *typ,char *ps,char *s,int n);
extern int xsd_equal(rx_st_t *rx_st, char *typ,char *val,char *s,int n);

extern void xsd_test(void);

#endif
