/* $Id: rnv.h,v 1.7 2004/01/09 10:19:39 dvd Exp $ */

#include <stdarg.h>
#include "type.h"

#ifndef RNV_H
#define RNV_H 1

#define RNV_ER_ELEM 0
#define RNV_ER_AKEY 1
#define RNV_ER_AVAL 2
#define RNV_ER_EMIS 3
#define RNV_ER_AMIS 4
#define RNV_ER_UFIN 5
#define RNV_ER_TEXT 6
#define RNV_ER_NOTX 7

extern void rnv_default_verror_handler(rnv_t *rnv, int erno,va_list ap);

extern void rnv_init(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st);
extern void rnv_clear(void);
extern void rnv_dispose(rnv_t *rnv);

extern int rnv_text(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st, int *curp,int *prevp,char *text,int n_t,int mixed);
extern int rnv_start_tag(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st, int *curp,int *prevp,char *name,char **attrs);
extern int rnv_start_tag_open(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int *curp,int *prevp,char *name);
extern int rnv_attribute(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st, int *curp,int *prevp,char *name,char *val);
extern int rnv_start_tag_close(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int *curp,int *prevp,char *name);
extern int rnv_end_tag(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int *curp,int *prevp,char *name);

#endif
