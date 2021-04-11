/* $Id: er.h,v 1.22 2004/01/20 00:20:57 dvd Exp $ */

#ifndef ER_H
#define ER_H 1

#include <stdarg.h>
#include "type.h"

extern int (*er_printf)(char *format,...);
extern int (*er_vprintf)(char *format,va_list ap);

extern int er_default_printf(char *format,...);
extern int er_default_vprintf(char *format,va_list ap);
extern int verror_default_handler(void *data, int erno, char *format,va_list ap);

#endif
