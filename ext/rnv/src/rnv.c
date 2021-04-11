/* $Id: rnv.c,v 1.69 2004/01/28 23:21:24 dvd Exp $ */
#include <string.h> /*strncpy,strrchr*/
#include <assert.h>

#include "type.h"

#include "m.h"
#include "xmlc.h" /*xmlc_white_space*/
#include "erbit.h"
#include "drv.h"
#include "er.h"
#include "rnv.h"

#define err(msg) (*rnv->verror_handler)(rnv,erno|ERBIT_RNV,msg"\n",ap);
void rnv_default_verror_handler(rnv_t *rnv, int erno,va_list ap) {
  if(erno&ERBIT_DRV) {
    drv_default_verror_handler(rnv, erno&~ERBIT_DRV,ap);
  } else {
    switch(erno) {
    case RNV_ER_ELEM: err("element %s^%s not allowed"); break;
    case RNV_ER_AKEY: err("attribute %s^%s not allowed"); break;
    case RNV_ER_AVAL: err("attribute %s^%s with invalid value \"%s\""); break;
    case RNV_ER_EMIS: err("incomplete content"); break;
    case RNV_ER_AMIS: err("missing attributes of %s^%s"); break;
    case RNV_ER_UFIN: err("unfinished content of element %s^%s"); break;
    case RNV_ER_TEXT: err("invalid data or text not allowed"); break;
    case RNV_ER_NOTX: err("text not allowed"); break;
    default: assert(0);
    }
  }
}

static void error_handler(rnv_t *rnv, int erno,...) {
  va_list ap; va_start(ap,erno); (*rnv->rnv_verror_handler)(rnv,erno,ap); va_end(ap);
}

static void verror_handler_drv(rnv_t *rnv, int erno,va_list ap) {rnv_default_verror_handler(rnv,erno|ERBIT_DRV,ap);}

void rnv_init(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st) {
    rnv->rnv_verror_handler=&rnv_default_verror_handler;
    drv_init(rnv, drv_st, rn_st, rx_st);
    rnv->drv_verror_handler=&verror_handler_drv;
}

void rnv_dispose(rnv_t *rnv) {
  if (rnv->rn_pattern)
    m_free(rnv->rn_pattern);
  if (rnv->rn_nameclass)
    m_free(rnv->rn_nameclass);
  if (rnv->rn_string)
    m_free(rnv->rn_string);
  if (rnv->rnx_exp)
    m_free(rnv->rnx_exp);

  m_free(rnv);
}

static char *qname_open(char **surip,char **snamep,char *name) {
  char *sep;
  if((sep=strrchr(name,':'))) {
    *snamep=sep+1; *surip=name; *sep='\0';
  } else {
    *snamep=name; while(*name) ++name; *surip=name;
  }
  return sep; /* NULL if no namespace */
}

static void qname_close(char *sep) {if(sep) *sep=':';}

static int whitespace(char *text,int n_txt) {
  char *s=text,*end=text+n_txt;
  for(;;) {
    if(s==end) return 1;
    if(!xmlc_white_space(*(s++))) return 0;
  }
}

int rnv_text(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st, int *curp,int *prevp,char *text,int n_txt,int mixed) {
  int ok=1;
  if(mixed) {
    if(!whitespace(text,n_txt)) {
      *curp=drv_mixed_text(rnv, drv_st, rn_st, *prevp=*curp);
      if(*curp==rnv->rn_notAllowed) { ok=0;
	*curp=drv_mixed_text_recover(*prevp);
	error_handler(rnv, RNV_ER_NOTX);
      }
    }
  } else {
    *curp=drv_text(rnv, rn_st, rx_st, drv_st, *prevp=*curp,text,n_txt);
    if(*curp==rnv->rn_notAllowed) { ok=0;
      *curp=drv_text_recover(*prevp,text,n_txt);
      error_handler(rnv, RNV_ER_TEXT);
    }
  }
  return ok;
}

int rnv_start_tag_open(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int *curp,int *prevp,char *name) {
  int ok=1; char *suri,*sname,*sep;
  sep=qname_open(&suri,&sname,name);
  *curp=drv_start_tag_open(rnv, drv_st, rn_st, *prevp=*curp,suri,sname);
  if(*curp==rnv->rn_notAllowed) { ok=0;
    *curp=drv_start_tag_open_recover(rnv, drv_st, rn_st, *prevp,suri,sname);
    error_handler(rnv, *curp==rnv->rn_notAllowed?RNV_ER_ELEM:RNV_ER_EMIS,suri,sname);
  }
  qname_close(sep);
  return ok;
}

int rnv_attribute(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st, int *curp,int *prevp,char *name,char *val) {
  int ok=1; char *suri,*sname,*sep;
  sep=qname_open(&suri,&sname,name);
  *curp=drv_attribute_open(rnv, drv_st, rn_st, *prevp=*curp,suri,sname);
  if(*curp==rnv->rn_notAllowed) { ok=0;
    *curp=drv_attribute_open_recover(*prevp,suri,sname);
    error_handler(rnv, RNV_ER_AKEY,suri,sname);
  } else {
    *curp=drv_text(rnv, rn_st, rx_st, drv_st, *prevp=*curp,(char*)val,strlen(val));
    if(*curp==rnv->rn_notAllowed || (*curp=drv_attribute_close(rnv, drv_st, rn_st, *prevp=*curp))==rnv->rn_notAllowed) { ok=0;
      *curp=drv_attribute_close_recover(rnv, drv_st, rn_st, *prevp);
      error_handler(rnv, RNV_ER_AVAL,suri,sname,val);
    }
  }
  qname_close(sep);
  return ok;
}

int rnv_start_tag_close(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int *curp,int *prevp,char *name) {
  int ok=1; char *suri,*sname,*sep;
  *curp=drv_start_tag_close(rnv, drv_st, rn_st, *prevp=*curp);
  if(*curp==rnv->rn_notAllowed) { ok=0;
    *curp=drv_start_tag_close_recover(rnv, drv_st, rn_st, *prevp);
    sep=qname_open(&suri,&sname,name);
    error_handler(rnv, RNV_ER_AMIS,suri,sname);
    qname_close(sep);
  }
  return ok;
}

int rnv_start_tag(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st,  rx_st_t *rx_st, int *curp,int *prevp,char *name,char **attrs) {
  int ok=1;
  ok=rnv_start_tag_open(rnv, drv_st, rn_st, curp,prevp,name)&&ok;
  while(*curp!=rnv->rn_notAllowed) {
    if(!(*attrs)) break;
    ok = rnv_attribute(rnv, drv_st, rn_st, rx_st, curp,prevp,*attrs,*(attrs+1))&&ok;
    attrs+=2;
  }
  if(*curp!=rnv->rn_notAllowed) ok=rnv_start_tag_close(rnv, drv_st, rn_st, curp,prevp,name)&&ok;
  return ok;
}

int rnv_end_tag(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int *curp,int *prevp,char *name) {
  int ok=1; char *suri,*sname,*sep;
  *curp=drv_end_tag(rnv, drv_st, rn_st, *prevp=*curp);
  if(*curp==rnv->rn_notAllowed) { ok=0;
    sep=qname_open(&suri,&sname,name);
    error_handler(rnv, RNV_ER_UFIN,suri,sname);
    qname_close(sep);
    *curp=drv_end_tag_recover(rnv, drv_st, rn_st, *prevp);
  }
  return ok;
}
