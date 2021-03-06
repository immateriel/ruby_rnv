/* $Id: rx.h,v 1.12 2004/01/24 18:42:41 dvd Exp $ */

#include <stdarg.h>
#include "type.h"

#ifndef RX_H
#define RX_H

#define RX_ER_BADCH 0
#define RX_ER_UNFIN 1
#define RX_ER_NOLSQ 2
#define RX_ER_NORSQ 3
#define RX_ER_NOLCU 4
#define RX_ER_NORCU 5
#define RX_ER_NOLPA 6
#define RX_ER_NORPA 7
#define RX_ER_BADCL 8
#define RX_ER_NODGT 9
#define RX_ER_DNUOB 10
#define RX_ER_NOTRC 11

extern void rx_default_verror_handler(void *data, int erno, int (*handler)(void *data, int erno,char *format, va_list ap), va_list ap);

extern void rx_init(rx_st_t *rx_st);
extern void rx_clear(rx_st_t *rx_st);

extern void rx_dispose(rx_st_t *rx_st);

/* just compiles the expression to check the syntax */
extern int rx_check(rx_st_t *rx_st, char *rx);

/*
 returns positive value if the s[0..n] ~= rx, 0 if not, -1 on regex error;
 rx and s are in utf-8, rx is 0-terminated, s is n bytes long;
 rmatch replaces white space in s with 0x20,
 cmatch collapses white space.
 */
extern int rx_match(rx_st_t *rx_st, char *rx,char *s,int n);
extern int rx_rmatch(rx_st_t *rx_st, char *rx,char *s,int n);
extern int rx_cmatch(rx_st_t *rx_st, char *rx,char *s,int n);

#endif
