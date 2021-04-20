/* $Id: drv.c,v 1.41 2004/03/13 13:28:02 dvd Exp $ */
#include "type.h"

#include "xmlc.h" /*xmlc_white_space*/
#include "m.h"
#include "s.h" /*s_tokcmpn*/
#include "ht.h"
#include "rn.h"
#include "xsd.h"
#include "ll.h"
#include "erbit.h"
#include "er.h"
#include "drv.h"

#define LEN_DTL DRV_LEN_DTL
#define LEN_M DRV_LEN_M
#define PRIME_M DRV_PRIME_M
#define LIM_M DRV_LIM_M

#define M_SIZE 5

#define M_STO 0
#define M_STC 1
#define M_ATT 2
#define M_TXT 3
#define M_END 4
#define M_SET(p) drv_st->memo[drv_st->i_m][M_SIZE-1]=p
#define M_RET(m) drv_st->memo[m][M_SIZE-1]

#define err(msg) (*handler)(data,erno|ERBIT_DRV,msg"\n",ap);
void drv_default_verror_handler(void *data, int erno, int (*handler)(void *data, int erno,char *format, va_list ap), va_list ap) {
  if(erno&ERBIT_XSD) {
    xsd_default_verror_handler(data, erno&~ERBIT_XSD,handler, ap);
  } else {
    switch(erno) {
    case DRV_ER_NODTL: err("no datatype library for URI '%s'"); break;
    default: assert(0);
    }
  }
}

static void error_handler(drv_st_t *drv_st,int erno,...) {
  va_list ap; va_start(ap,erno); drv_default_verror_handler(drv_st->user_data, erno, drv_st->verror_handler, ap); va_end(ap);
}

static void new_memo(drv_st_t *drv_st, int typ) {
  if(drv_st->drv_compact) ht_deli(&drv_st->ht_m,drv_st->i_m);
  drv_st->memo[drv_st->i_m][0]=typ;
}

static int equal_m(void *user, int m1,int m2) {
  drv_st_t *drv_st = (drv_st_t *)user;
  int *me1=drv_st->memo[m1],*me2=drv_st->memo[m2];
  return (me1[0]==me2[0])&&(me1[1]==me2[1])&&(me1[2]==me2[2])&&(me1[3]==me2[3]);
}
static int hash_m(void *user, int m) {
  drv_st_t *drv_st = (drv_st_t *)user;
  int *me=drv_st->memo[m];
  return ((me[0]&0x7)|((me[1]^me[2]^me[3])<<3))*PRIME_M;
}

static int newStartTagOpen(drv_st_t *drv_st, int p,int uri,int name) {
  int *me=drv_st->memo[drv_st->i_m];
  new_memo(drv_st, M_STO);
  me[1]=p; me[2]=uri; me[3]=name;
  return ht_get(&drv_st->ht_m,drv_st->i_m);
}

static int newAttributeOpen(drv_st_t *drv_st, int p,int uri,int name) {
  int *me=drv_st->memo[drv_st->i_m];
  new_memo(drv_st, M_ATT);
  me[1]=p; me[2]=uri; me[3]=name;
  return ht_get(&drv_st->ht_m,drv_st->i_m);
}

static int newStartTagClose(drv_st_t *drv_st, int p) {
  int *me=drv_st->memo[drv_st->i_m];
  new_memo(drv_st, M_STC);
  me[1]=p; me[2]=me[3]=0;
  return ht_get(&drv_st->ht_m,drv_st->i_m);
}

static int newMixedText(drv_st_t *drv_st, int p) {
  int *me=drv_st->memo[drv_st->i_m];
  new_memo(drv_st, M_TXT);
  me[1]=p; me[2]=me[3]=0;
  return ht_get(&drv_st->ht_m,drv_st->i_m);
}

static int newEndTag(drv_st_t *drv_st, int p) {
  int *me=drv_st->memo[drv_st->i_m];
  new_memo(drv_st, M_END);
  me[1]=p; me[2]=me[3]=0;
  return ht_get(&drv_st->ht_m,drv_st->i_m);
}

static void accept_m(drv_st_t *drv_st) {
  if(ht_get(&drv_st->ht_m,drv_st->i_m)!=-1) {
    if(drv_st->drv_compact) ht_del(&drv_st->ht_m,drv_st->i_m); else return;
  }
  ht_put(&drv_st->ht_m,drv_st->i_m++);
  if(drv_st->drv_compact&&drv_st->i_m==LIM_M) drv_st->i_m=0;
  if(drv_st->i_m==drv_st->len_m) drv_st->memo=(int(*)[M_SIZE])m_stretch(drv_st->memo,drv_st->len_m=2*drv_st->i_m,drv_st->i_m,sizeof(int[M_SIZE]));
}

static int fallback_equal(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *val,char *s,int n) {return 1;}
static int fallback_allows(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *ps,char *s,int n) {return 1;}

static int builtin_equal(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *val,char *s,int n) {
  int dt=rn_newDatatype(rnv, rn_st, 0,typ-rnv->rn_string);
  if(dt==rnv->rn_dt_string) return s_cmpn(val,s,n)==0;
  else if(dt==rnv->rn_dt_token) return s_tokcmpn(val,s,n)==0;
  else assert(0);
  return 0;
}

static int builtin_allows(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *ps,char *s,int n) {return 1;}

static int emb_xsd_allows(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *ps,char *s,int n) {
  return xsd_allows(rx_st, typ,ps,s,n);
}

static int emb_xsd_equal(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *val,char *s,int n) {
  return xsd_equal(rx_st, typ,val,s,n);
}

void drv_init(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, rx_st_t *rx_st) {
    rx_st->verror_handler = &verror_default_handler;
    drv_st->verror_handler = &verror_default_handler;
    
    xsd_init(rx_st);
    
    if(rnv->verror_handler)
      rx_st->verror_handler = rnv->verror_handler;
    rx_st->user_data = rnv->user_data;

    drv_st->memo=(int (*)[M_SIZE])m_alloc(drv_st->len_m=LEN_M,sizeof(int[M_SIZE]));
    drv_st->dtl=(struct dtl*)m_alloc(drv_st->len_dtl=LEN_DTL,sizeof(struct dtl));
    drv_st->ht_m.user = drv_st;
    ht_init(&drv_st->ht_m,LEN_M,&hash_m,&equal_m);

    drv_st->i_m=0; drv_st->n_dtl=0;
    drv_add_dtl(rnv, drv_st, rn_st, rnv->rn_string+0,&fallback_equal,&fallback_allows); /* guard at 0 */
    drv_add_dtl(rnv, drv_st, rn_st, rnv->rn_string+0,&builtin_equal,&builtin_allows);
    drv_add_dtl(rnv, drv_st, rn_st, rnv->rn_string+rnv->rn_xsd_uri,&emb_xsd_equal,&emb_xsd_allows);

    if(rnv->verror_handler)
      drv_st->verror_handler = rnv->verror_handler;
    drv_st->user_data = rnv->user_data;
}

void drv_dispose(drv_st_t *drv_st) {
  if (drv_st->memo)
    m_free(drv_st->memo);
  if (drv_st->dtl)
    m_free(drv_st->dtl);
  ht_dispose(&drv_st->ht_m);
  m_free(drv_st);
}

void drv_add_dtl(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, char *suri,
    int (*equal)(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *val,char *s,int n),
    int (*allows)(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, int uri, char *typ,char *ps,char *s,int n)) {
  if(drv_st->n_dtl==drv_st->len_dtl) drv_st->dtl=(struct dtl *)m_stretch(drv_st->dtl,drv_st->len_dtl=drv_st->n_dtl*2,drv_st->n_dtl,sizeof(struct dtl));
  drv_st->dtl[drv_st->n_dtl].uri=rn_newString(rnv, rn_st, suri);
  drv_st->dtl[drv_st->n_dtl].equal=equal;
  drv_st->dtl[drv_st->n_dtl].allows=allows;
  ++drv_st->n_dtl;
}

static struct dtl *getdtl(rnv_t *rnv, drv_st_t *drv_st, int uri) {
  int i;
  drv_st->dtl[0].uri=uri; i=drv_st->n_dtl;
  while(drv_st->dtl[--i].uri!=uri);
  if(i==0) error_handler(drv_st, DRV_ER_NODTL,rnv->rn_string+uri);
  return drv_st->dtl+i;
}

static int ncof(rnv_t *rnv, int nc,int uri,int name) {
  int uri2,name2,nc1,nc2;
  switch(RN_NC_TYP(nc)) {
  case RN_NC_QNAME: rn_QName(nc,uri2,name2); return uri2==uri&&name2==name;
  case RN_NC_NSNAME: rn_NsName(nc,uri2); return uri2==uri;
  case RN_NC_ANY_NAME: return 1;
  case RN_NC_EXCEPT: rn_NameClassExcept(nc,nc1,nc2); return ncof(rnv, nc1,uri,name)&&!ncof(rnv, nc2,uri,name);
  case RN_NC_CHOICE: rn_NameClassChoice(nc,nc1,nc2); return ncof(rnv, nc1,uri,name)||ncof(rnv, nc2,uri,name);
  default: assert(0);
  }
  return 0;
}

static int apply_after(rnv_t *rnv, rn_st_t *rn_st, int (*f)(rnv_t *rnv, rn_st_t *rn_st, int q1,int q2),int p1,int p0) {
  int p11,p12;
  switch(RN_P_TYP(p1)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_INTERLEAVE: case RN_P_GROUP: case RN_P_ONE_OR_MORE:
  case RN_P_LIST: case RN_P_DATA: case RN_P_DATA_EXCEPT: case RN_P_VALUE:
  case RN_P_ATTRIBUTE: case RN_P_ELEMENT:
    return rnv->rn_notAllowed;
  case RN_P_CHOICE: rn_Choice(p1,p11,p12); return rn_choice(rnv, rn_st, apply_after(rnv, rn_st, f,p11,p0),apply_after(rnv, rn_st, f,p12,p0));
  case RN_P_AFTER: rn_After(p1,p11,p12); return rn_after(rnv, rn_st, p11,(*f)(rnv, rn_st, p12,p0));
  default: assert(0);
  }
  return 0;
}

static int start_tag_open(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,int uri,int name,int recover) {
  int nc,p1,p2,m,ret=0;
  if(!recover) {
    m=newStartTagOpen(drv_st, p,uri,name);
    if(m!=-1) return M_RET(m);
  }
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_LIST: case RN_P_DATA: case RN_P_DATA_EXCEPT: case RN_P_VALUE:
  case RN_P_ATTRIBUTE:
    ret=rnv->rn_notAllowed;
    break;
  case RN_P_CHOICE: rn_Choice(p,p1,p2);
    ret=rn_choice(rnv, rn_st, start_tag_open(rnv, drv_st, rn_st, p1,uri,name,recover),start_tag_open(rnv, drv_st, rn_st, p2,uri,name,recover));
    break;
  case RN_P_ELEMENT: rn_Element(p,nc,p1);
    ret=ncof(rnv, nc,uri,name)?rn_after(rnv, rn_st, p1,rnv->rn_empty):rnv->rn_notAllowed;
    break;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2);
    ret=rn_choice(
      rnv, rn_st, apply_after(rnv, rn_st, &rn_ileave,start_tag_open(rnv, drv_st, rn_st, p1,uri,name,recover),p2),
      apply_after(rnv, rn_st, &rn_ileave,start_tag_open(rnv, drv_st, rn_st, p2,uri,name,recover),p1));
    break;
  case RN_P_GROUP: rn_Group(p,p1,p2);
    { int p11=apply_after(rnv, rn_st, &rn_group,start_tag_open(rnv, drv_st, rn_st, p1,uri,name,recover),p2);
      ret=(rn_nullable(p1)||recover)?rn_choice(rnv, rn_st, p11,start_tag_open(rnv, drv_st, rn_st, p2,uri,name,recover)):p11;
    } break;
  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1);
    ret=apply_after(rnv, rn_st, &rn_group,start_tag_open(rnv, drv_st, rn_st, p1,uri,name,recover),rn_choice(rnv, rn_st, p,rnv->rn_empty));
    break;
  case RN_P_AFTER: rn_After(p,p1,p2);
    ret=apply_after(rnv, rn_st, &rn_after,start_tag_open(rnv, drv_st, rn_st, p1,uri,name,recover),p2);
    break;
  default: assert(0);
  }
  if(!recover) {
    newStartTagOpen(drv_st, p,uri,name); M_SET(ret);
    accept_m(drv_st);
  }
  return ret;
}

int drv_start_tag_open(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,char *suri,char *sname) {return start_tag_open(rnv, drv_st, rn_st, p,rn_newString(rnv, rn_st, suri),rn_newString(rnv, rn_st, sname),0);}
int drv_start_tag_open_recover(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,char *suri,char *sname) {return start_tag_open(rnv, drv_st, rn_st, p,rn_newString(rnv, rn_st, suri),rn_newString(rnv, rn_st, sname),1);}

static int puorg_rn(rnv_t *rnv, rn_st_t *rn_st, int p2,int p1) {return rn_group(rnv, rn_st, p1,p2);}

static int attribute_open(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,int uri,int name) {
  int nc,p1,p2,m,ret=0;
  m=newAttributeOpen(drv_st, p,uri,name);
  if(m!=-1) return M_RET(m);
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_LIST: case RN_P_DATA: case RN_P_DATA_EXCEPT: case RN_P_VALUE:
  case RN_P_ELEMENT:
    ret=rnv->rn_notAllowed;
    break;
  case RN_P_CHOICE: rn_Choice(p,p1,p2);
    ret=rn_choice(rnv, rn_st, attribute_open(rnv, drv_st, rn_st, p1,uri,name),attribute_open(rnv, drv_st, rn_st, p2,uri,name));
    break;
  case RN_P_ATTRIBUTE: rn_Attribute(p,nc,p1);
    ret=ncof(rnv, nc,uri,name)?rn_after(rnv, rn_st, p1,rnv->rn_empty):rnv->rn_notAllowed;
    break;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2);
    ret=rn_choice(
      rnv, rn_st, apply_after(rnv, rn_st, &rn_ileave,attribute_open(rnv, drv_st, rn_st, p1,uri,name),p2),
      apply_after(rnv, rn_st, &rn_ileave,attribute_open(rnv, drv_st, rn_st, p2,uri,name),p1));
    break;
  case RN_P_GROUP: rn_Group(p,p1,p2);
    ret=rn_choice(
      rnv, rn_st, apply_after(rnv, rn_st, &rn_group,attribute_open(rnv, drv_st, rn_st, p1,uri,name),p2),
      apply_after(rnv, rn_st, &puorg_rn,attribute_open(rnv, drv_st, rn_st, p2,uri,name),p1));
    break;
  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1);
    ret=apply_after(rnv, rn_st, &rn_group,attribute_open(rnv, drv_st, rn_st, p1,uri,name),rn_choice(rnv, rn_st, p,rnv->rn_empty));
    break;
  case RN_P_AFTER: rn_After(p,p1,p2);
    ret=apply_after(rnv, rn_st, &rn_after,attribute_open(rnv, drv_st, rn_st, p1,uri,name),p2);
    break;
  default: assert(0);
  }
  newAttributeOpen(drv_st, p,uri,name); M_SET(ret);
  accept_m(drv_st);
  return ret;
}

int drv_attribute_open(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,char *suri,char *sname) {return attribute_open(rnv, drv_st, rn_st, p,rn_newString(rnv, rn_st, suri),rn_newString(rnv, rn_st, sname));}
int drv_attribute_open_recover(int p,char *suri,char *sname) {return p;}

extern int drv_attribute_close(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p) {return drv_end_tag(rnv, drv_st, rn_st, p);}
extern int drv_attribute_close_recover(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p) {return drv_end_tag_recover(rnv, drv_st, rn_st, p);}

static int start_tag_close(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,int recover) {
  int p1,p2,ret=0,m;
  if(!recover) {
    m=newStartTagClose(drv_st, p);
    if(m!=-1) return M_RET(m);
  }
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_LIST: case RN_P_DATA: case RN_P_DATA_EXCEPT: case RN_P_VALUE:
  case RN_P_ELEMENT:
    ret=p;
    break;
  case RN_P_CHOICE: rn_Choice(p,p1,p2);
    ret=rn_choice(rnv, rn_st, start_tag_close(rnv, drv_st, rn_st, p1,recover),start_tag_close(rnv, drv_st, rn_st, p2,recover));
    break;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2);
    ret=rn_ileave(rnv, rn_st, start_tag_close(rnv, drv_st, rn_st, p1,recover),start_tag_close(rnv, drv_st, rn_st, p2,recover));
    break;
  case RN_P_GROUP: rn_Group(p,p1,p2);
    ret=rn_group(rnv, rn_st, start_tag_close(rnv, drv_st, rn_st, p1,recover),start_tag_close(rnv, drv_st, rn_st, p2,recover));
    break;
  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1);
    ret=rn_one_or_more(rnv, rn_st, start_tag_close(rnv, drv_st, rn_st, p1,recover));
    break;
  case RN_P_ATTRIBUTE:
    ret=recover?rnv->rn_empty:rnv->rn_notAllowed;
    break;
  case RN_P_AFTER: rn_After(p,p1,p2);
    ret=rn_after(rnv, rn_st, start_tag_close(rnv, drv_st, rn_st, p1,recover),p2);
    break;
  default: assert(0);
  }
  if(!recover) {
    newStartTagClose(drv_st, p); M_SET(ret);
    accept_m(drv_st);
  }
  return ret;
}
int drv_start_tag_close(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p) {return start_tag_close(rnv, drv_st, rn_st, p,0);}
int drv_start_tag_close_recover(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p) {return start_tag_close(rnv, drv_st, rn_st, p,1);}

static int text(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, drv_st_t *drv_st, int p,char *s,int n);
static int list(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, drv_st_t *drv_st, int p,char *s,int n) {
  char *end=s+n,*sp;
  for(;;) {
    while(s!=end&&xmlc_white_space(*s)) ++s;
    sp=s;
    while(sp!=end&&!xmlc_white_space(*sp)) ++sp;
    if(s==end) break;
    p=text(rnv, rn_st, rx_st, drv_st, p,s,sp-s);
    s=sp;
  }
  return p;
}

static int text(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, drv_st_t *drv_st, int p,char *s,int n) { /* matches text, including whitespace */
  int p1,p2,dt,ps,lib,typ,val,ret=0;
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY:
  case RN_P_ATTRIBUTE: case RN_P_ELEMENT:
    ret=rnv->rn_notAllowed;
    break;
  case RN_P_TEXT:
    ret=p;
    break;
  case RN_P_AFTER: rn_After(p,p1,p2);
    ret=rn_after(rnv, rn_st, text(rnv, rn_st, rx_st, drv_st, p1,s,n),p2);
    break;
  case RN_P_CHOICE: rn_Choice(p,p1,p2);
    ret=rn_choice(rnv, rn_st, text(rnv, rn_st, rx_st, drv_st, p1,s,n),text(rnv, rn_st, rx_st, drv_st, p2,s,n));
    break;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2);
    ret=rn_choice(rnv, rn_st, rn_ileave(rnv, rn_st, text(rnv, rn_st, rx_st, drv_st, p1,s,n),p2),rn_ileave(rnv, rn_st, p1,text(rnv, rn_st, rx_st, drv_st, p2,s,n)));
    break;
  case RN_P_GROUP: rn_Group(p,p1,p2);
    { int p11=rn_group(rnv, rn_st, text(rnv, rn_st, rx_st, drv_st, p1,s,n),p2);
      ret=rn_nullable(p1)?rn_choice(rnv, rn_st, p11,text(rnv, rn_st, rx_st, drv_st, p2,s,n)):p11;
    } break;
  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1);
    ret=rn_group(rnv, rn_st, text(rnv, rn_st, rx_st, drv_st, p1,s,n),rn_choice(rnv, rn_st, p,rnv->rn_empty));
    break;
  case RN_P_LIST: rn_List(p,p1);
    ret=rn_nullable(list(rnv, rn_st, rx_st, drv_st, p1,s,n))?rnv->rn_empty:rnv->rn_notAllowed;
    break;
  case RN_P_DATA: rn_Data(p,dt,ps); rn_Datatype(dt,lib,typ);
    int i;
    // avoid dtl called multiple times for same data
    for(i = 0;i<drv_st->n_dtl_cb;i++) {
      if(drv_st->dtl_cb[i].p == p)
        ret = drv_st->dtl_cb[i].ret;
    }
    if(!ret) {
    ret=getdtl(rnv, drv_st, lib)->allows(rnv,rn_st,rx_st, lib, rnv->rn_string+typ,rnv->rn_string+ps,s,n)?rnv->rn_empty:rnv->rn_notAllowed;
    drv_st->dtl_cb[drv_st->n_dtl_cb].p = p;
    drv_st->dtl_cb[drv_st->n_dtl_cb].ret = ret;
    drv_st->n_dtl_cb++;
    }
    break;
  case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2);
    ret=text(rnv, rn_st, rx_st, drv_st, p1,s,n)==rnv->rn_empty&&!rn_nullable(text(rnv, rn_st, rx_st, drv_st, p2,s,n))?rnv->rn_empty:rnv->rn_notAllowed;
    break;
  case RN_P_VALUE: rn_Value(p,dt,val); rn_Datatype(dt,lib,typ);
    ret=getdtl(rnv, drv_st, lib)->equal(rnv,rn_st,rx_st, lib, rnv->rn_string+typ,rnv->rn_string+val,s,n)?rnv->rn_empty:rnv->rn_notAllowed;
    break;
  default: assert(0);
  }
  return ret;
}

static int textws(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, drv_st_t *drv_st, int p,char *s,int n) {
  drv_st->n_dtl_cb = 0;
  int p1=text(rnv, rn_st, rx_st, drv_st, p,s,n),ws=1;
  char *end=s+n;
  while(s!=end) {if(!xmlc_white_space(*s)) {ws=0; break;} ++s;}
  return ws?rn_choice(rnv, rn_st, p,p1):p1;
}
int drv_text(rnv_t *rnv, rn_st_t *rn_st, rx_st_t *rx_st, drv_st_t *drv_st, int p,char *s,int n) {return textws(rnv, rn_st, rx_st, drv_st, p,s,n);}
int drv_text_recover(int p,char *s,int n) {return p;}

static int mixed_text(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p) { /* matches text in mixed context */
  int p1,p2,ret=0,m;
  m=newMixedText(drv_st, p);
  if(m!=-1) return M_RET(m);
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY:
  case RN_P_ATTRIBUTE: case RN_P_ELEMENT:
  case RN_P_LIST: case RN_P_DATA: case RN_P_DATA_EXCEPT: case RN_P_VALUE:
    ret=rnv->rn_notAllowed;
    break;
  case RN_P_TEXT:
    ret=p;
    break;
  case RN_P_AFTER: rn_After(p,p1,p2);
    ret=rn_after(rnv, rn_st, mixed_text(rnv, drv_st, rn_st, p1),p2);
    break;
  case RN_P_CHOICE: rn_Choice(p,p1,p2);
    ret=rn_choice(rnv, rn_st, mixed_text(rnv, drv_st, rn_st, p1),mixed_text(rnv, drv_st, rn_st, p2));
    break;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2);
    ret=rn_choice(rnv, rn_st, rn_ileave(rnv, rn_st, mixed_text(rnv, drv_st, rn_st, p1),p2),rn_ileave(rnv, rn_st, p1,mixed_text(rnv, drv_st, rn_st, p2)));
    break;
  case RN_P_GROUP: rn_Group(p,p1,p2);
    { int p11=rn_group(rnv, rn_st, mixed_text(rnv, drv_st, rn_st, p1),p2);
      ret=rn_nullable(p1)?rn_choice(rnv, rn_st, p11,mixed_text(rnv, drv_st, rn_st, p2)):p11;
    } break;
  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1);
    ret=rn_group(rnv, rn_st, mixed_text(rnv, drv_st, rn_st, p1),rn_choice(rnv, rn_st, p,rnv->rn_empty));
    break;
  default: assert(0);
  }
  newMixedText(drv_st, p); M_SET(ret);
  accept_m(drv_st);
  return ret;
}
int drv_mixed_text(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p) {return mixed_text(rnv, drv_st, rn_st, p);}
int drv_mixed_text_recover(int p) {return p;}

static int end_tag(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p,int recover) {
  int p1,p2,ret=0,m;
  if(!recover) {
    m=newEndTag(drv_st, p);
    if(m!=-1) return M_RET(m);
  }
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_INTERLEAVE: case RN_P_GROUP: case RN_P_ONE_OR_MORE:
  case RN_P_LIST: case RN_P_DATA: case RN_P_DATA_EXCEPT: case RN_P_VALUE:
  case RN_P_ATTRIBUTE: case RN_P_ELEMENT:
    ret=rnv->rn_notAllowed;
    break;
  case RN_P_CHOICE: rn_Choice(p,p1,p2);
    ret=rn_choice(rnv, rn_st, end_tag(rnv, drv_st, rn_st, p1,recover),end_tag(rnv, drv_st, rn_st, p2,recover));
    break;
  case RN_P_AFTER: rn_After(p,p1,p2);
    ret=(rn_nullable(p1)||recover)?p2:rnv->rn_notAllowed;
    break;
  default: assert(0);
  }
  if(!recover) {
    newEndTag(drv_st, p); M_SET(ret);
    accept_m(drv_st);
  }
  return ret;
}
int drv_end_tag(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p) {return end_tag(rnv, drv_st, rn_st, p,0);}
int drv_end_tag_recover(rnv_t *rnv, drv_st_t *drv_st, rn_st_t *rn_st, int p) {return end_tag(rnv, drv_st, rn_st, p,1);}
