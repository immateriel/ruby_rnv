/* $Id: rnl.c,v 1.2 2004/01/15 23:47:45 dvd Exp $ */
#include <stdarg.h>

#include "type.h"

#include "erbit.h"
#include "rn.h"
#include "rnc.h"
#include "rnd.h"
#include "rnl.h"

void rnl_default_verror_handler(void *data, int erno, int (*handler)(void *data, int erno,char *format, va_list ap), va_list ap) {
  if(erno&ERBIT_RNC) {
    rnc_default_verror_handler(data, erno&~ERBIT_RNC,handler, ap);
  } else if(erno&ERBIT_RND) {
    rnd_default_verror_handler(data, erno&~ERBIT_RND,handler, ap);
  }
}

void rnl_init(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st) {
    rn_init(rnv, rn_st);
    rnc_init(rnc_st);

    if(rnv->verror_handler)
      rnc_st->verror_handler = rnv->verror_handler;
    rnc_st->user_data = rnv->user_data;

    rnd_init(rnd_st); 

    if(rnv->verror_handler)
      rnd_st->verror_handler = rnv->verror_handler;
    rnd_st->user_data = rnv->user_data;
}

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
  src.verror_handler = rnv->verror_handler;
  src.user_data = rnv->user_data;
  rnc_open(&src,fn); 
  return load(rnv, rnc_st, rn_st, rnd_st, &src);
}

int rnl_fd(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, char *fn,int fd) {
  struct rnc_source src;
  src.verror_handler = rnv->verror_handler;
  src.user_data = rnv->user_data;
  rnc_bind(&src,fn,fd); 
  return load(rnv, rnc_st, rn_st, rnd_st, &src);
}

int rnl_s(rnv_t *rnv, rnc_st_t *rnc_st, rn_st_t *rn_st, rnd_st_t *rnd_st, char *fn,char *s,int len) {
  struct rnc_source src;
  src.verror_handler = rnv->verror_handler;
  src.user_data = rnv->user_data;
  rnc_stropen(&src,fn,s,len); 
  return load(rnv, rnc_st, rn_st, rnd_st, &src);
}
