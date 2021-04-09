#include "type.h"

/* $Id: drv.h,v 1.15 2004/01/01 00:57:14 dvd Exp $ */

#include <stdarg.h>

#ifndef DRV_H
#define DRV_H 1

#define DRV_ER_NODTL 0

struct dtl {
  int uri;
  int (*equal)(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *val,char *s,int n);
  int (*allows)(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *ps,char *s,int n);
};

extern void drv_default_verror_handler(rnv_t *rnv, int erno,va_list ap);

extern void drv_init(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st);
extern void drv_clear(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st);

/* Expat passes character data unterminated.  Hence functions that can deal with cdata expect the length of the data */
extern void drv_add_dtl(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, char *suri,
int (*equal)(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *val,char *s,int n),
int (*allows)(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *ps,char *s,int n));

extern int drv_start_tag_open(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,char *suri,char *sname);
extern int drv_start_tag_open_recover(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,char *suri,char *sname);
extern int drv_attribute_open(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,char *suri,char *s);
extern int drv_attribute_open_recover(int p,char *suri,char *s);
extern int drv_attribute_close(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p);
extern int drv_attribute_close_recover(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p);
extern int drv_start_tag_close(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p);
extern int drv_start_tag_close_recover(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p);
extern int drv_text(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, drv_st_t *drv_st, int p,char *s,int n);
extern int drv_text_recover(int p,char *s,int n);
extern int drv_mixed_text(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p);
extern int drv_mixed_text_recover(int p);
extern int drv_end_tag(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p);
extern int drv_end_tag_recover(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p);

#endif
