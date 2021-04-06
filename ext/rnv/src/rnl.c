#include "type.h"

/* $Id: rnl.c,v 1.2 2004/01/15 23:47:45 dvd Exp $ */

#include <stdarg.h>
#include "erbit.h"
#include "rn.h"
#include "rnc.h"
#include "rnd.h"
#include "rnl.h"

void rnl_default_verror_handler(rnv_t *rnv, int erno,va_list ap) {
  if(erno&ERBIT_RNC) {
    rnc_default_verror_handler(rnv, erno&~ERBIT_RNC,ap);
  } else if(erno&ERBIT_RND) {
    rnd_default_verror_handler(rnv, erno&~ERBIT_RND,ap);
  }
}

static void verror_handler_rnc(rnv_t *rnv, int erno,va_list ap) {rnl_default_verror_handler(rnv, erno|ERBIT_RNC,ap);}
static void verror_handler_rnd(rnv_t *rnv, int erno,va_list ap) {rnl_default_verror_handler(rnv, erno|ERBIT_RND,ap);}

void rnl_init(rnv_t *rnv, rn_st_t *rn_st, rnc_st_t *rnc_st) {
    rnv->rnl_verror_handler=&rnl_default_verror_handler;
    rn_init(rnv, rn_st);
    rnc_init(rnv, rnc_st, rn_st);
    rnv->rnc_verror_handler=&verror_handler_rnc;
    rnd_init(rnv, rn_st); 
    rnv->rnd_verror_handler=&verror_handler_rnd;
}

void rnl_clear(void) {}

static int load(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, struct rnc_source *sp) {
  int start=-1;
  if(!rnc_errors(sp)) start=rnc_parse(rnv, rnc_st, rn_st, sp); rnc_close(sp);
  if(!rnc_errors(sp)&&(start=rnd_fixup(rnv, rnd_st, rn_st, start))) {
    start=rn_compress_last(rnv, rn_st, start);
  } else start=0;
  return start;
}

int rnl_fn(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, char *fn) {
  struct rnc_source src;
  src.rnv = rnv;
  rnc_open(&src,fn); 
  return load(rnv, rnc_st, rn_st, rnd_st, &src);
}

int rnl_fd(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, char *fn,int fd) {
  struct rnc_source src;
  src.rnv = rnv;
  rnc_bind(&src,fn,fd); 
  return load(rnv, rnc_st, rn_st, rnd_st, &src);
}

int rnl_s(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, char *fn,char *s,int len) {
  struct rnc_source src;
  src.rnv = rnv;
  rnc_stropen(&src,fn,s,len); 
  return load(rnv, rnc_st, rn_st, rnd_st, &src);
}
