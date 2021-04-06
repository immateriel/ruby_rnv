/* $Id: rnx.h,v 1.7 2004/02/18 12:53:42 dvd Exp $ */

#include "type.h"

#ifndef RNX_H
#define RNX_H 1

extern void rnx_init(rnv_t *rnv, rnx_st_t *rnx_st);
extern void rnx_clear(void);

extern void rnx_expected(rnv_t *rnv, rnx_st_t *rnx_st, int p,int req);

extern char *rnx_p2str(rnv_t *rnv, int p);
extern char *rnx_nc2str(rnv_t *rnv, int nc);

#endif
