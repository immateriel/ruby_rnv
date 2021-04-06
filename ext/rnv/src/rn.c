#include "type.h"

/* $Id: rn.c,v 1.62 2004/03/13 14:12:11 dvd Exp $ */

#include <string.h> /* strcmp,strlen,strcpy*/
#include "m.h"
#include "s.h" /* s_hval */
#include "ht.h"
#include "ll.h"
#include "rn.h"
#include "rnx.h"

#define LEN_P RN_LEN_P
#define PRIME_P RN_PRIME_P
#define LIM_P RN_LIM_P
#define LEN_NC RN_LEN_NC
#define PRIME_NC RN_PRIME_NC
#define LEN_S RN_LEN_S

#define P_SIZE 3
#define NC_SIZE 3
#define P_AVG_SIZE 2
#define NC_AVG_SIZE 2
#define S_AVG_SIZE 16

#define erased(i) (rnv->rn_pattern[i]&RN_P_FLG_ERS)
#define erase(i) (rnv->rn_pattern[i]|=RN_P_FLG_ERS)

static int p_size[]={1,1,1,1,3,3,3,2,2,3,3,3,3,3,2,3};
static int nc_size[]={1,3,2,1,3,3,3};

void rn_new_schema(rn_st_t *rn_st) {rn_st->base_p=rn_st->i_p; rn_st->i_ref=0;}

void rn_del_p(rn_st_t *rn_st, int i) {ht_deli(&rn_st->ht_p,i);}
void rn_add_p(rn_st_t *rn_st, int i) {if(ht_get(&rn_st->ht_p,i)==-1) ht_put(&rn_st->ht_p,i);}

int rn_contentType(rnv_t *rnv, int i) {return rnv->rn_pattern[i]&0x1C00;}
void rn_setContentType(rnv_t *rnv, int i,int t1,int t2) {rnv->rn_pattern[i]|=(t1>t2?t1:t2);}
int rn_groupable(rnv_t *rnv, int p1,int p2) {
  int ct1=rn_contentType(rnv, p1),ct2=rn_contentType(rnv, p2);
  return ((ct1&ct2&RN_P_FLG_CTC)||((ct1|ct2)&RN_P_FLG_CTE));
}

static int add_s(rnv_t *rnv, rn_st_t *rn_st, char *s) {
  int len=strlen(s)+1;
  if(rn_st->i_s+len>rn_st->len_s) rnv->rn_string=(char*)m_stretch(rnv->rn_string,
    rn_st->len_s=2*(rn_st->i_s+len),rn_st->i_s,sizeof(char));
  strcpy(rnv->rn_string+rn_st->i_s,s);
  return len;
}

/* the two functions below are structuraly identical;
 they used to be expanded from a macro using ##,
 but then I eliminated all occurences of ## --
 it was an obstacle to porting; sam script to turn
 the first into the second is
s/([^a-z])p([^a-z])/\1nc\2/g
s/([^A-Z])P([^A-Z])/\1NC\2/g
s/_pattern/_nameclass/g
 */

static int accept_p(rnv_t *rnv, rn_st_t *rn_st) {
  int j;
  if((j=ht_get(&rn_st->ht_p,rn_st->i_p))==-1) {
    ht_put(&rn_st->ht_p,j=rn_st->i_p);
    rn_st->i_p+=p_size[RN_P_TYP(rn_st->i_p)];
    if(rn_st->i_p+P_SIZE>rn_st->len_p) rnv->rn_pattern=(int *)m_stretch(rnv->rn_pattern,
      rn_st->len_p=2*(rn_st->i_p+P_SIZE),rn_st->i_p,sizeof(int));
  }
  return j;
}

static int accept_nc(rnv_t *rnv, rn_st_t *rn_st) {
  int j;
  if((j=ht_get(&rn_st->ht_nc,rn_st->i_nc))==-1) {
    ht_put(&rn_st->ht_nc,j=rn_st->i_nc);
    rn_st->i_nc+=nc_size[RN_NC_TYP(rn_st->i_nc)];
    if(rn_st->i_nc+NC_SIZE>rn_st->len_nc) rnv->rn_nameclass=(int *)m_stretch(rnv->rn_nameclass,
      rn_st->len_nc=2*(rn_st->i_nc+NC_SIZE),rn_st->i_nc,sizeof(int));
  }
  return j;
}

int rn_newString(rnv_t *rnv, rn_st_t *rn_st, char *s) {
  int d_s,j;
  assert(!rn_st->adding_ps);
  d_s=add_s(rnv, rn_st, s);
  if((j=ht_get(&rn_st->ht_s,rn_st->i_s))==-1) {
    ht_put(&rn_st->ht_s,j=rn_st->i_s);
    rn_st->i_s+=d_s;
  }
  return j;
}

#define P_NEW(x) rnv->rn_pattern[rn_st->i_p]=x

int rn_newNotAllowed(rnv_t *rnv, rn_st_t *rn_st) { P_NEW(RN_P_NOT_ALLOWED);
  return accept_p(rnv, rn_st);
}

int rn_newEmpty(rnv_t *rnv, rn_st_t *rn_st) { P_NEW(RN_P_EMPTY);
  rn_setNullable(rn_st->i_p,1);
  return accept_p(rnv, rn_st);
}

int rn_newText(rnv_t *rnv, rn_st_t *rn_st) { P_NEW(RN_P_TEXT);
  rn_setNullable(rn_st->i_p,1);
  rn_setCdata(rn_st->i_p,1);
  return accept_p(rnv, rn_st);
}

int rn_newChoice(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) { P_NEW(RN_P_CHOICE);
  rnv->rn_pattern[rn_st->i_p+1]=p1; rnv->rn_pattern[rn_st->i_p+2]=p2;
  rn_setNullable(rn_st->i_p,rn_nullable(p1)||rn_nullable(p2));
  rn_setCdata(rn_st->i_p,rn_cdata(p1)||rn_cdata(p2));
  return accept_p(rnv, rn_st);
}

int rn_newInterleave(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) { P_NEW(RN_P_INTERLEAVE);
  rnv->rn_pattern[rn_st->i_p+1]=p1; rnv->rn_pattern[rn_st->i_p+2]=p2;
  rn_setNullable(rn_st->i_p,rn_nullable(p1)&&rn_nullable(p2));
  rn_setCdata(rn_st->i_p,rn_cdata(p1)||rn_cdata(p2));
  return accept_p(rnv, rn_st);
}

int rn_newGroup(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) { P_NEW(RN_P_GROUP);
  rnv->rn_pattern[rn_st->i_p+1]=p1; rnv->rn_pattern[rn_st->i_p+2]=p2;
  rn_setNullable(rn_st->i_p,rn_nullable(p1)&&rn_nullable(p2));
  rn_setCdata(rn_st->i_p,rn_cdata(p1)||rn_cdata(p2));
  return accept_p(rnv, rn_st);
}

int rn_newOneOrMore(rnv_t *rnv, rn_st_t *rn_st, int p1) { P_NEW(RN_P_ONE_OR_MORE);
  rnv->rn_pattern[rn_st->i_p+1]=p1;
  rn_setNullable(rn_st->i_p,rn_nullable(p1));
  rn_setCdata(rn_st->i_p,rn_cdata(p1));
  return accept_p(rnv, rn_st);
}

int rn_newList(rnv_t *rnv, rn_st_t *rn_st, int p1) { P_NEW(RN_P_LIST);
  rnv->rn_pattern[rn_st->i_p+1]=p1;
  rn_setCdata(rn_st->i_p,1);
  return accept_p(rnv, rn_st);
}

int rn_newData(rnv_t *rnv, rn_st_t *rn_st, int dt,int ps) { P_NEW(RN_P_DATA);
  rnv->rn_pattern[rn_st->i_p+1]=dt;
  rnv->rn_pattern[rn_st->i_p+2]=ps;
  rn_setCdata(rn_st->i_p,1);
  return accept_p(rnv, rn_st);
}

int rn_newDataExcept(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) { P_NEW(RN_P_DATA_EXCEPT);
  rnv->rn_pattern[rn_st->i_p+1]=p1; rnv->rn_pattern[rn_st->i_p+2]=p2;
  rn_setCdata(rn_st->i_p,1);
  return accept_p(rnv, rn_st);
}

int rn_newValue(rnv_t *rnv, rn_st_t *rn_st, int dt,int s) { P_NEW(RN_P_VALUE);
  rnv->rn_pattern[rn_st->i_p+1]=dt; rnv->rn_pattern[rn_st->i_p+2]=s;
  rn_setCdata(rn_st->i_p,1);
  return accept_p(rnv, rn_st);
}

int rn_newAttribute(rnv_t *rnv, rn_st_t *rn_st, int nc,int p1) { P_NEW(RN_P_ATTRIBUTE);
  rnv->rn_pattern[rn_st->i_p+2]=nc; rnv->rn_pattern[rn_st->i_p+1]=p1;
  return accept_p(rnv, rn_st);
}

int rn_newElement(rnv_t *rnv, rn_st_t *rn_st, int nc,int p1) { P_NEW(RN_P_ELEMENT);
  rnv->rn_pattern[rn_st->i_p+2]=nc; rnv->rn_pattern[rn_st->i_p+1]=p1;
  return accept_p(rnv, rn_st);
}

int rn_newAfter(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) { P_NEW(RN_P_AFTER);
  rnv->rn_pattern[rn_st->i_p+1]=p1; rnv->rn_pattern[rn_st->i_p+2]=p2;
  rn_setCdata(rn_st->i_p,rn_cdata(p1));
  return accept_p(rnv, rn_st);
}

int rn_newRef(rnv_t *rnv, rn_st_t *rn_st) { P_NEW(RN_P_REF);
  rnv->rn_pattern[rn_st->i_p+1]=0;
  return ht_deli(&rn_st->ht_p,accept_p(rnv, rn_st));
}

int rn_one_or_more(rnv_t *rnv, rn_st_t *rn_st, int p) {
  if(RN_P_IS(p,RN_P_EMPTY)) return p;
  if(RN_P_IS(p,RN_P_NOT_ALLOWED)) return p;
  if(RN_P_IS(p,RN_P_TEXT)) return p;
  return rn_newOneOrMore(rnv, rn_st, p);
}

int rn_group(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) {
  if(RN_P_IS(p1,RN_P_NOT_ALLOWED)) return p1;
  if(RN_P_IS(p2,RN_P_NOT_ALLOWED)) return p2;
  if(RN_P_IS(p1,RN_P_EMPTY)) return p2;
  if(RN_P_IS(p2,RN_P_EMPTY)) return p1;
  return rn_newGroup(rnv, rn_st, p1,p2);
}

static int samechoice(rnv_t *rnv, int p1,int p2) {
  if(RN_P_IS(p1,RN_P_CHOICE)) {
    int p11,p12; rn_Choice(p1,p11,p12);
    return p12==p2||samechoice(rnv, p11,p2);
  } else return p1==p2;
}

int rn_choice(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) {
  if(RN_P_IS(p1,RN_P_NOT_ALLOWED)) return p2;
  if(RN_P_IS(p2,RN_P_NOT_ALLOWED)) return p1;
  if(RN_P_IS(p2,RN_P_CHOICE)) {
    int p21,p22; rn_Choice(p2,p21,p22);
    p1=rn_choice(rnv, rn_st, p1,p21); return rn_choice(rnv, rn_st, p1,p22);
  }
  if(samechoice(rnv, p1,p2)) return p1;
  if(rn_nullable(p1) && (RN_P_IS(p2,RN_P_EMPTY))) return p1;
  if(rn_nullable(p2) && (RN_P_IS(p1,RN_P_EMPTY))) return p2;
  return rn_newChoice(rnv, rn_st, p1,p2);
}

int rn_ileave(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) {
  if(RN_P_IS(p1,RN_P_NOT_ALLOWED)) return p1;
  if(RN_P_IS(p2,RN_P_NOT_ALLOWED)) return p2;
  if(RN_P_IS(p1,RN_P_EMPTY)) return p2;
  if(RN_P_IS(p2,RN_P_EMPTY)) return p1;
  return rn_newInterleave(rnv, rn_st, p1,p2);
}

int rn_after(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2) {
  if(RN_P_IS(p1,RN_P_NOT_ALLOWED)) return p1;
  if(RN_P_IS(p2,RN_P_NOT_ALLOWED)) return p2;
  return rn_newAfter(rnv, rn_st, p1,p2);
}

#define NC_NEW(x) rnv->rn_nameclass[rn_st->i_nc]=x

int rn_newQName(rnv_t *rnv, rn_st_t *rn_st, int uri,int name) { NC_NEW(RN_NC_QNAME);
  rnv->rn_nameclass[rn_st->i_nc+1]=uri; rnv->rn_nameclass[rn_st->i_nc+2]=name;
  return accept_nc(rnv, rn_st);
}

int rn_newNsName(rnv_t *rnv, rn_st_t *rn_st, int uri) { NC_NEW(RN_NC_NSNAME);
  rnv->rn_nameclass[rn_st->i_nc+1]=uri;
  return accept_nc(rnv, rn_st);
}

int rn_newAnyName(rnv_t *rnv, rn_st_t *rn_st) { NC_NEW(RN_NC_ANY_NAME);
  return accept_nc(rnv, rn_st);
}

int rn_newNameClassExcept(rnv_t *rnv, rn_st_t *rn_st, int nc1,int nc2) { NC_NEW(RN_NC_EXCEPT);
  rnv->rn_nameclass[rn_st->i_nc+1]=nc1; rnv->rn_nameclass[rn_st->i_nc+2]=nc2;
  return accept_nc(rnv, rn_st);
}

int rn_newNameClassChoice(rnv_t *rnv, rn_st_t *rn_st, int nc1,int nc2) { NC_NEW(RN_NC_CHOICE);
  rnv->rn_nameclass[rn_st->i_nc+1]=nc1; rnv->rn_nameclass[rn_st->i_nc+2]=nc2;
  return accept_nc(rnv, rn_st);
}

int rn_newDatatype(rnv_t *rnv, rn_st_t *rn_st, int lib,int typ) { NC_NEW(RN_NC_DATATYPE);
  rnv->rn_nameclass[rn_st->i_nc+1]=lib; rnv->rn_nameclass[rn_st->i_nc+2]=typ;
  return accept_nc(rnv, rn_st);
}

int rn_i_ps(rn_st_t *rn_st) {rn_st->adding_ps=1; return rn_st->i_s;}
void rn_add_pskey(rnv_t *rnv, rn_st_t *rn_st, char *s) {rn_st->i_s+=add_s(rnv, rn_st, s);}
void rn_add_psval(rnv_t *rnv, rn_st_t *rn_st, char *s) {rn_st->i_s+=add_s(rnv, rn_st, s);}
void rn_end_ps(rnv_t *rnv, rn_st_t *rn_st) {rn_st->i_s+=add_s(rnv, rn_st, ""); rn_st->adding_ps=0;}

static int hash_p(rnv_t *rnv, int i);
static int hash_nc(rnv_t *rnv, int i);
static int hash_s(rnv_t *rnv, int i);

static int equal_p(rnv_t *rnv, int p1,int p2);
static int equal_nc(rnv_t *rnv, int nc1,int nc2);
static int equal_s(rnv_t *rnv, int s1,int s2);

static void windup(rnv_t *rnv, rn_st_t *rn_st);

void rn_init(rnv_t *rnv, rn_st_t *rn_st) {
    rnv->rn_pattern=(int *)m_alloc(rn_st->len_p=P_AVG_SIZE*LEN_P,sizeof(int));
    rnv->rn_nameclass=(int *)m_alloc(rn_st->len_nc=NC_AVG_SIZE*LEN_NC,sizeof(int));
    rnv->rn_string=(char*)m_alloc(rn_st->len_s=S_AVG_SIZE*LEN_S,sizeof(char));
    rn_st->ht_p.user = rnv;
    rn_st->ht_nc.user = rnv;
    rn_st->ht_s.user = rnv;
    ht_init(&rn_st->ht_p,LEN_P,&hash_p,&equal_p);
    ht_init(&rn_st->ht_nc,LEN_NC,&hash_nc,&equal_nc);
    ht_init(&rn_st->ht_s,LEN_S,&hash_s,&equal_s);
    windup(rnv, rn_st);
}

void rn_clear(rnv_t *rnv, rn_st_t *rn_st) {
  ht_clear(&rn_st->ht_p); ht_clear(&rn_st->ht_nc); ht_clear(&rn_st->ht_s);
  windup(rnv, rn_st);
}

static void windup(rnv_t *rnv, rn_st_t *rn_st) {
  rn_st->i_p=rn_st->i_nc=rn_st->i_s=0;
  rn_st->adding_ps=0;
  rnv->rn_pattern[0]=RN_P_ERROR;  accept_p(rnv, rn_st);
  rnv->rn_nameclass[0]=RN_NC_ERROR; accept_nc(rnv, rn_st);
  rn_newString(rnv, rn_st, "");
  rnv->rn_notAllowed=rn_newNotAllowed(rnv, rn_st); 
  rnv->rn_empty=rn_newEmpty(rnv, rn_st); 
  rnv->rn_text=rn_newText(rnv, rn_st); 
  rn_st->BASE_P=rn_st->i_p;
  rnv->rn_dt_string=rn_newDatatype(rnv, rn_st, 0,rn_newString(rnv, rn_st, "string")); 
  rnv->rn_dt_token=rn_newDatatype(rnv, rn_st, 0,rn_newString(rnv, rn_st, "token"));
  rnv->rn_xsd_uri=rn_newString(rnv, rn_st, "http://www.w3.org/2001/XMLSchema-datatypes");
}

static int hash_p(rnv_t *rnv, int p) {
  int *pp=rnv->rn_pattern+p; int h=0;
  switch(p_size[RN_P_TYP(p)]) {
  case 1: h=pp[0]&0xF; break;
  case 2: h=(pp[0]&0xF)|(pp[1]<<4); break;
  case 3: h=(pp[0]&0xF)|((pp[1]^pp[2])<<4); break;
  default: assert(0);
  }
  return h*PRIME_P;
}

static int hash_nc(rnv_t *rnv, int nc) {
  int *ncp=rnv->rn_nameclass+nc; int h=0;
  switch(nc_size[RN_NC_TYP(nc)]) {
  case 1: h=ncp[0]&0x7; break;
  case 2: h=(ncp[0]&0x7)|(ncp[1]<<3); break;
  case 3: h=(ncp[0]&0x7)|((ncp[1]^ncp[2])<<3); break;
  default: assert(0);
  }
  return h*PRIME_NC;
}

static int hash_s(rnv_t *rnv, int i) {return s_hval(rnv->rn_string+i);}

static int equal_p(rnv_t *rnv, int p1,int p2) {
  int *pp1=rnv->rn_pattern+p1,*pp2=rnv->rn_pattern+p2;
  if(RN_P_TYP(p1)!=RN_P_TYP(p2)) return 0;
  switch(p_size[RN_P_TYP(p1)]) {
  case 3: if(pp1[2]!=pp2[2]) return 0;
  case 2: if(pp1[1]!=pp2[1]) return 0;
  case 1: return 1;
  default: assert(0);
  }
  return 0;
}

static int equal_nc(rnv_t *rnv, int nc1,int nc2) {
  int *ncp1=rnv->rn_nameclass+nc1,*ncp2=rnv->rn_nameclass+nc2;
  if(RN_NC_TYP(nc1)!=RN_NC_TYP(nc2)) return 0;
  switch(nc_size[RN_NC_TYP(nc1)]) {
  case 3: if(ncp1[2]!=ncp2[2]) return 0;
  case 2: if(ncp1[1]!=ncp2[1]) return 0;
  case 1: return 1;
  default: assert(0);
  }
  return 0;
}

static int equal_s(rnv_t *rnv, int s1,int s2) {return strcmp(rnv->rn_string+s1,rnv->rn_string+s2)==0;}

/* marks patterns reachable from start, assumes that the references are resolved */
#define pick_p(p) do { \
  if(p>=since && !rn_marked(p)) {flat[n_f++]=p; rn_mark(p);}  \
} while(0)
static void mark_p(rnv_t *rnv, rn_st_t *rn_st, int start,int since) {
  int p,p1,p2,nc,i,n_f;
  int *flat=(int*)m_alloc(rn_st->i_p-since,sizeof(int));

  n_f=0; pick_p(start);
  for(i=0;i!=n_f;++i) {
    p=flat[i];
    switch(RN_P_TYP(p)) {
    case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
    case RN_P_DATA: case RN_P_VALUE: break;

    case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
    case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); goto BINARY;
    case RN_P_GROUP: rn_Group(p,p1,p2); goto BINARY;
    case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); goto BINARY;
    BINARY: pick_p(p2); goto UNARY;

    case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); goto UNARY;
    case RN_P_LIST: rn_List(p,p1); goto UNARY;
    case RN_P_ATTRIBUTE: rn_Attribute(p,nc,p1); goto UNARY;
    case RN_P_ELEMENT: rn_Element(p,nc,p1); goto UNARY;
    UNARY: pick_p(p1); break;

    default:
      assert(0);
    }
  }
  m_free(flat);
}

/* assumes that used patterns are marked */
#define redir_p() do { \
  if(q<since || xlat[q-since]!=-1) { \
    rn_unmark(p); xlat[p-since]=q; \
    changed=1; \
  } else { \
    ht_deli(&rn_st->ht_p,q); ht_put(&rn_st->ht_p,p); \
  } \
} while(0)
static void sweep_p(rnv_t *rnv, rn_st_t *rn_st, int *starts,int n_st,int since) {
  int p,p1,p2,nc,q,changed,touched;
  int *xlat;
  xlat=(int*)m_alloc(rn_st->i_p-since,sizeof(int));
  changed=0;
  for(p=since;p!=rn_st->i_p;p+=p_size[RN_P_TYP(p)]) {
    if(rn_marked(p)) xlat[p-since]=p; else xlat[p-since]=-1;
  }
  for(p=since;p!=rn_st->i_p;p+=p_size[RN_P_TYP(p)]) {
    if(xlat[p-since]==p && (q=ht_get(&rn_st->ht_p,p))!=p) redir_p();
  }
  while(changed) {
    changed=0;
    for(p=since;p!=rn_st->i_p;p+=p_size[RN_P_TYP(p)]) {
      if(xlat[p-since]==p) {
	touched=0;
	switch(RN_P_TYP(p)) {
	case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
	case RN_P_DATA: case RN_P_VALUE:
	  break;

	case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
	case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); goto BINARY;
	case RN_P_GROUP: rn_Group(p,p1,p2); goto BINARY;
	case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); goto BINARY;
	BINARY:
	  if(p2>=since && (q=xlat[p2-since])!=p2) {
	    ht_deli(&rn_st->ht_p,p);
	    touched=1;
	    rnv->rn_pattern[p+2]=q;
	  }
	  goto UNARY;

	case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); goto UNARY;
	case RN_P_LIST: rn_List(p,p1); goto UNARY;
	case RN_P_ATTRIBUTE: rn_Attribute(p,nc,p1); goto UNARY;
	case RN_P_ELEMENT: rn_Element(p,nc,p1); goto UNARY;
	UNARY:
	  if(p1>=since && (q=xlat[p1-since])!=p1) {
	    if(!touched) ht_deli(&rn_st->ht_p,p);
	    touched=1;
	    rnv->rn_pattern[p+1]=q;
	  }
	  break;

	default:
	  assert(0);
	}
	if(touched) {
	  changed=1; /* recursion through redirection */
	  if((q=ht_get(&rn_st->ht_p,p))==-1) {
	    ht_put(&rn_st->ht_p,p);
	  } else {
	    redir_p();
	  }
	}
      }
    }
  }
  while(n_st--!=0) {
    if(*starts>=since) *starts=xlat[*starts-since];
    ++starts;
  }
  m_free(xlat);
}

static void unmark_p(rnv_t *rnv, rn_st_t *rn_st, int since) {
  int p;
  for(p=since;p!=rn_st->i_p;p+=p_size[RN_P_TYP(p)]) {
    if(rn_marked(p)) rn_unmark(p); else {ht_deli(&rn_st->ht_p,p); erase(p);}
  }
}

static void compress_p(rnv_t *rnv, rn_st_t *rn_st, int *starts,int n_st,int since) {
  int p,psiz, p1,p2,nc, q,i_q, newlen_p;
  int *xlat=(int*)m_alloc(rn_st->i_p-since,sizeof(int));
  p=q=since;
  while(p!=rn_st->i_p) { psiz=p_size[RN_P_TYP(p)];
    if(erased(p)) {
      xlat[p-since]=-1;
    } else {
      ht_deli(&rn_st->ht_p,p);
      xlat[p-since]=q;
      q+=psiz;
    }
    p+=psiz;
  }
  i_q=q; p=since;
  while(p!=rn_st->i_p) { psiz=p_size[RN_P_TYP(p)]; /* rn_pattern[p] changes */
    if(xlat[p-since]!=-1) {
      switch(RN_P_TYP(p)) {
      case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
      case RN_P_DATA: case RN_P_VALUE:
	break;

      case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
      case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); goto BINARY;
      case RN_P_GROUP: rn_Group(p,p1,p2); goto BINARY;
      case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); goto BINARY;
      BINARY:
	if(p2>=since && (q=xlat[p2-since])!=p2) rnv->rn_pattern[p+2]=q;
	goto UNARY;

      case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); goto UNARY;
      case RN_P_LIST: rn_List(p,p1); goto UNARY;
      case RN_P_ATTRIBUTE: rn_Attribute(p,nc,p1); goto UNARY;
      case RN_P_ELEMENT: rn_Element(p,nc,p1); goto UNARY;
      UNARY:
	if(p1>=since && (q=xlat[p1-since])!=p1) rnv->rn_pattern[p+1]=q;
	break;

      default:
	assert(0);
      }
      if((q=xlat[p-since])!=p) { int i;
	for(i=0;i!=psiz;++i) rnv->rn_pattern[q+i]=rnv->rn_pattern[p+i];
	assert(q+psiz<rn_st->i_p);
      }
      ht_put(&rn_st->ht_p,q);
    }
    p+=psiz;
  }
  while(n_st--!=0) {
    if(*starts>=since) *starts=xlat[*starts-since];
    ++starts;
  }
  m_free(xlat);
  
  if(i_q!=rn_st->i_p) { rn_st->i_p=i_q; newlen_p=rn_st->i_p*2;
    if(rn_st->len_p>P_AVG_SIZE*LIM_P&&newlen_p<rn_st->len_p) {
      rnv->rn_pattern=(int*)m_stretch(rnv->rn_pattern,
	rn_st->len_p=newlen_p>P_AVG_SIZE*LEN_P?newlen_p:P_AVG_SIZE*LEN_P,
	rn_st->i_p,sizeof(int));
    }
  }
}

void rn_compress(rnv_t *rnv, rn_st_t *rn_st, int *starts,int n_st) {
  int i;
  for(i=0;i!=n_st;++i) mark_p(rnv, rn_st, starts[i],rn_st->BASE_P);
  sweep_p(rnv, rn_st, starts,n_st,rn_st->BASE_P);
  unmark_p(rnv, rn_st, rn_st->BASE_P);
  compress_p(rnv, rn_st, starts,n_st,rn_st->BASE_P);
}

int rn_compress_last(rnv_t *rnv, rn_st_t *rn_st, int start) {
  mark_p(rnv, rn_st, start,rn_st->base_p);
  sweep_p(rnv, rn_st, &start,1,rn_st->base_p);
  unmark_p(rnv, rn_st, rn_st->base_p);
  compress_p(rnv, rn_st, &start,1,rn_st->base_p);
  return start;
}
