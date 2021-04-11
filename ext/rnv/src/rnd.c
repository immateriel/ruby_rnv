/* $Id: rnd.c,v 1.33 2004/02/25 00:00:32 dvd Exp $ */
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "type.h"

#include "m.h"
#include "rn.h"
#include "rnx.h"
#include "ll.h"
#include "er.h"
#include "rnd.h"
#include "erbit.h"

#define LEN_F RND_LEN_F

#define err(msg) (*handler)(data,erno|ERBIT_RND,"error: "msg"\n",ap)
void rnd_default_verror_handler(void *data, int erno, int (*handler)(void *data, int erno,char *format, va_list ap), va_list ap) {
  switch(erno) {
  case RND_ER_LOOPST: err("loop in start pattern"); break;
  case RND_ER_LOOPEL: err("loop in pattern for element '%s'"); break;
  case RND_ER_CTYPE: err("content of element '%s' does not have a content-type"); break;
  case RND_ER_BADSTART: err("bad path in start pattern"); break;
  case RND_ER_BADMORE: err("bad path before '*' or '+' in element '%s'"); break;
  case RND_ER_BADEXPT: err("bad path after '-' in element '%s'"); break;
  case RND_ER_BADLIST: err("bad path after 'list' in element '%s'"); break;
  case RND_ER_BADATTR: err("bad path in attribute '%s' of element '%s'"); break;
  default: assert(0);
  }
}

void rnd_init(rnd_st_t *rnd_st) {
    rnd_st->verror_handler = &verror_default_handler;
}

void rnd_dispose(rnd_st_t *rnd_st) {
  if (rnd_st->flat)
    m_free(rnd_st->flat);
  m_free(rnd_st);
}

void rnd_clear(void) {}

static void error_handler(rnd_st_t *rnd_st, int erno,...) {
  va_list ap; va_start(ap,erno); 
  rnd_default_verror_handler(rnd_st->user_data, erno, rnd_st->verror_handler, ap); 
  va_end(ap);
  ++rnd_st->errors;
}

static int de(rnv_t *rnv, int p) {
  int p0=p,p1;
  RN_P_CHK(p,RN_P_REF);
  for(;;) {
    rn_Ref(p,p1);
    if(!RN_P_IS(p1,RN_P_REF)||p1==p0) break;
    p=p1;
  }
  return p1;
}

static void flatten(rnv_t *rnv, rnd_st_t *rnd_st, int p) { if(!rn_marked(p)) {rnd_st->flat[rnd_st->n_f++]=p; rn_mark(p);}}

static void deref(rnv_t *rnv, rnd_st_t *rnd_st, rn_st_t *rn_st, int start) {
  int p,p1,p2,nc,i,changed;

  rnd_st->flat=(int*)m_alloc(rnd_st->len_f=LEN_F,sizeof(int)); rnd_st->n_f=0;
  if(RN_P_IS(start,RN_P_REF)) start=de(rnv, start);
  flatten(rnv, rnd_st, start);

  i=0;
  do {
    p=rnd_st->flat[i++];
    switch(RN_P_TYP(p)) {
    case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT: case RN_P_DATA: case RN_P_VALUE:
      break;

    case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
    case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); goto BINARY;
    case RN_P_GROUP: rn_Group(p,p1,p2); goto BINARY;
    case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); goto BINARY;
    BINARY:
      changed=0;
      if(RN_P_IS(p1,RN_P_REF)) {p1=de(rnv, p1); changed=1;}
      if(RN_P_IS(p2,RN_P_REF)) {p2=de(rnv, p2); changed=1;}
      if(changed) {rn_del_p(rn_st, p); rnv->rn_pattern[p+1]=p1; rnv->rn_pattern[p+2]=p2; rn_add_p(rn_st, p);}
      if(rnd_st->n_f+2>rnd_st->len_f) rnd_st->flat=(int*)m_stretch(rnd_st->flat,rnd_st->len_f=2*(rnd_st->n_f+2),rnd_st->n_f,sizeof(int));
      flatten(rnv, rnd_st, p1); flatten(rnv, rnd_st, p2);
      break;

    case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); goto UNARY;
    case RN_P_LIST: rn_List(p,p1); goto UNARY;
    case RN_P_ATTRIBUTE: rn_Attribute(p,nc,p1); goto UNARY;
    case RN_P_ELEMENT: rn_Element(p,nc,p1); goto UNARY;
    UNARY:
      changed=0;
      if(RN_P_IS(p1,RN_P_REF)) {p1=de(rnv, p1); changed=1;}
      if(changed) {rn_del_p(rn_st, p); rnv->rn_pattern[p+1]=p1; rn_add_p(rn_st, p);}
      if(rnd_st->n_f+1>rnd_st->len_f) rnd_st->flat=(int*)m_stretch(rnd_st->flat,rnd_st->len_f=2*(rnd_st->n_f+1),rnd_st->n_f,sizeof(int));
      flatten(rnv, rnd_st, p1);
      break;

    case RN_P_REF: /* because of a loop, but will be handled in rnd_loops */
      break;

    default:
      assert(0);
    }
  } while(i!=rnd_st->n_f);
  for(i=0;i!=rnd_st->n_f;++i) rn_unmark(rnd_st->flat[i]);
}

static int loop(rnv_t *rnv, int p) {
  int nc,p1,p2,ret=1;
  if(rn_marked(p)) return 1;
  rn_mark(p);
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT: case RN_P_DATA: case RN_P_VALUE:
  case RN_P_ELEMENT:
    ret=0; break;

  case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); goto BINARY;
  case RN_P_GROUP: rn_Group(p,p1,p2); goto BINARY;
  case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); goto BINARY;
  BINARY:
    ret=loop(rnv, p1)||loop(rnv, p2); break;

  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); goto UNARY;
  case RN_P_LIST: rn_List(p,p1); goto UNARY;
  case RN_P_ATTRIBUTE:  rn_Attribute(p,nc,p1); goto UNARY;
  UNARY:
    ret=loop(rnv, p1); break;

  case RN_P_REF: ret=1; break;

  default: assert(0);
  }
  rn_unmark(p);
  return ret;
}

static void loops(rnv_t *rnv, rnd_st_t *rnd_st) {
  int i=0,p=rnd_st->flat[i],nc=-1,p1;
  for(;;) {
    if(loop(rnv, p)) {
      if(i==0) error_handler(rnd_st, RND_ER_LOOPST); else {
	char *s=rnx_nc2str(rnv, nc);
	error_handler(rnd_st,RND_ER_LOOPEL, s);
	m_free(s);
      }
    }
    for(;;) {++i;
      if(i==rnd_st->n_f) return;
      p=rnd_st->flat[i];
      if(RN_P_IS(p,RN_P_ELEMENT)) {
	rn_Element(p,nc,p1); p=p1;
	break;
      }
    }
  }
}

static void ctype(rnv_t *rnv, int p) {
  int p1,p2,nc;
  if(!rn_contentType(rnv, p)) {
    switch(RN_P_TYP(p)) {
    case RN_P_NOT_ALLOWED: rn_setContentType(rnv, p,RN_P_FLG_CTE,0); break;
    case RN_P_EMPTY: rn_setContentType(rnv, p,RN_P_FLG_CTE,0); break;
    case RN_P_TEXT: rn_setContentType(rnv, p,RN_P_FLG_CTC,0); break;
    case RN_P_CHOICE: rn_Choice(p,p1,p2); ctype(rnv, p1); ctype(rnv, p2);
      rn_setContentType(rnv, p,rn_contentType(rnv, p1),rn_contentType(rnv, p2)); break;
    case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); ctype(rnv, p1); ctype(rnv, p2);
      if(rn_groupable(rnv, p1,p2)) rn_setContentType(rnv, p,rn_contentType(rnv, p1),rn_contentType(rnv, p2)); break;
    case RN_P_GROUP: rn_Group(p,p1,p2); ctype(rnv, p1); ctype(rnv, p2);
      if(rn_groupable(rnv, p1,p2)) rn_setContentType(rnv, p,rn_contentType(rnv, p1),rn_contentType(rnv, p2)); break;
    case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); ctype(rnv, p1);
      if(rn_groupable(rnv, p1,p1)) rn_setContentType(rnv, p,rn_contentType(rnv, p1),0); break;
    case RN_P_LIST: rn_setContentType(rnv, p,RN_P_FLG_CTS,0); break;
    case RN_P_DATA: rn_setContentType(rnv, p,RN_P_FLG_CTS,0); break;
    case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); ctype(rnv, p1); ctype(rnv, p2);
      if(rn_contentType(rnv, p2)) rn_setContentType(rnv, p,RN_P_FLG_CTS,0); break;
    case RN_P_VALUE: rn_setContentType(rnv, p,RN_P_FLG_CTS,0); break;
    case RN_P_ATTRIBUTE: rn_Attribute(p,nc,p1); ctype(rnv, p1);
      if(rn_contentType(rnv, p1)) rn_setContentType(rnv, p,RN_P_FLG_CTE,0); break;
    case RN_P_ELEMENT: rn_setContentType(rnv, p,RN_P_FLG_CTC,0); break;
    default: assert(0);
    }
  }
}

static void ctypes(rnv_t *rnv, rnd_st_t *rnd_st) {
  int i,p,p1,nc;
  for(i=0;i!=rnd_st->n_f;++i) {
    p=rnd_st->flat[i];
    if(RN_P_IS(p,RN_P_ELEMENT)) {
      rn_Element(p,nc,p1);
      ctype(rnv, p1);
      if(!rn_contentType(rnv, p1)) {
	char *s=rnx_nc2str(rnv, nc);
	error_handler(rnd_st,RND_ER_CTYPE, s);
	m_free(s);
      }
    }
  }
}

static int bad_start(rnv_t *rnv, int p) {
  int p1,p2;
  switch(RN_P_TYP(p)) {
  case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_INTERLEAVE: case RN_P_GROUP: case RN_P_ONE_OR_MORE:
  case RN_P_LIST: case RN_P_DATA: case RN_P_DATA_EXCEPT: case RN_P_VALUE:
  case RN_P_ATTRIBUTE:
    return 1;
  case RN_P_NOT_ALLOWED:
  case RN_P_ELEMENT:
    return 0;
  case RN_P_CHOICE: rn_Choice(p,p1,p2);
    return bad_start(rnv, p1)||bad_start(rnv, p2);
  default: assert(0);
  }
  return 1;
}

static int bad_data_except(rnv_t *rnv, int p) {
  int p1,p2;
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED:
  case RN_P_VALUE: case RN_P_DATA:
    return 0;

  case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
  case RN_P_DATA_EXCEPT: rn_Choice(p,p1,p2); goto BINARY;
  BINARY: return bad_data_except(rnv, p1)||bad_data_except(rnv, p2);

  case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_INTERLEAVE: case RN_P_GROUP: case RN_P_ONE_OR_MORE:
  case RN_P_LIST:
  case RN_P_ATTRIBUTE: case RN_P_ELEMENT:
    return 1;
  default: assert(0);
  }
  return 1;
}

static int bad_one_or_more(rnv_t *rnv, int p,int in_group) {
  int nc,p1,p2;
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_DATA: case RN_P_VALUE:
  case RN_P_ELEMENT:
    return 0;

  case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); in_group=1; goto BINARY;
  case RN_P_GROUP: rn_Group(p,p1,p2); in_group=1; goto BINARY;
  case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); goto BINARY;
  BINARY: return  bad_one_or_more(rnv, p1,in_group)||bad_one_or_more(rnv, p2,in_group);

  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); goto UNARY;
  case RN_P_LIST: rn_List(p,p1); goto UNARY;
  case RN_P_ATTRIBUTE: if(in_group) return 1;
    rn_Attribute(p,nc,p1); goto UNARY;
  UNARY: return  bad_one_or_more(rnv, p1,in_group);
  default: assert(0);
  }
  return 1;
}

static int bad_list(rnv_t *rnv, int p) {
  int p1,p2;
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY:
  case RN_P_DATA: case RN_P_VALUE:
    return 0;

  case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
  case RN_P_GROUP: rn_Group(p,p1,p2); goto BINARY;
  case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); goto BINARY;
  BINARY: return bad_list(rnv, p1)||bad_list(rnv, p2);

  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); goto UNARY;
  case RN_P_LIST: rn_List(p,p1); goto UNARY;
  UNARY: return bad_list(rnv, p1);

  case RN_P_TEXT:
  case RN_P_INTERLEAVE:
  case RN_P_ATTRIBUTE:
  case RN_P_ELEMENT:
    return 1;
  default: assert(0);
  }
  return 1;
}

static int bad_attribute(rnv_t *rnv, int p) {
  int p1,p2;
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_DATA: case RN_P_VALUE:
    return 0;

  case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); goto BINARY;
  case RN_P_GROUP: rn_Group(p,p1,p2); goto BINARY;
  case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2); goto BINARY;
  BINARY: return bad_attribute(rnv, p1)||bad_attribute(rnv, p2);


  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); goto UNARY;
  case RN_P_LIST: rn_List(p,p1); goto UNARY;
  UNARY: return bad_attribute(rnv, p1);

  case RN_P_ATTRIBUTE: case RN_P_ELEMENT:
    return 1;
  default: assert(0);
  }
  return 1;
}

static void path(rnv_t *rnv, rnd_st_t *rnd_st, int p,int nc) {
  int p1,p2,nc1;
  switch(RN_P_TYP(p)) {
  case RN_P_NOT_ALLOWED: case RN_P_EMPTY: case RN_P_TEXT:
  case RN_P_DATA: case RN_P_VALUE:
  case RN_P_ELEMENT:
    break;

  case RN_P_CHOICE: rn_Choice(p,p1,p2); goto BINARY;
  case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); goto BINARY;
  case RN_P_GROUP: rn_Group(p,p1,p2); goto BINARY;
  case RN_P_DATA_EXCEPT: rn_DataExcept(p,p1,p2);
    if(bad_data_except(rnv, p2)) {char *s=rnx_nc2str(rnv, nc); error_handler(rnd_st,RND_ER_BADEXPT, s); m_free(s);}
    goto BINARY;
  BINARY: path(rnv, rnd_st, p1,nc); path(rnv, rnd_st, p2,nc); break;

  case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1);
    if(bad_one_or_more(rnv, p1,0)) {char *s=rnx_nc2str(rnv, nc); error_handler(rnd_st,RND_ER_BADMORE, s); m_free(s);}
    goto UNARY;
  case RN_P_LIST: rn_List(p,p1);
    if(bad_list(rnv, p1)) {char *s=rnx_nc2str(rnv, nc); error_handler(rnd_st,RND_ER_BADLIST, s); m_free(s);}
    goto UNARY;
  case RN_P_ATTRIBUTE: rn_Attribute(p,nc1,p1);
    if(bad_attribute(rnv, p1)) {char *s=rnx_nc2str(rnv, nc),*s1=rnx_nc2str(rnv, nc1); error_handler(rnd_st,RND_ER_BADATTR, s1,s); m_free(s1); m_free(s);}
    goto UNARY;
  UNARY: path(rnv, rnd_st, p1,nc); break;

  default: assert(0);
  }
}

static void paths(rnv_t *rnv, rnd_st_t *rnd_st) {
  int i,p,p1,nc;
  if(bad_start(rnv, rnd_st->flat[0])) error_handler(rnd_st, RND_ER_BADSTART);
  for(i=0;i!=rnd_st->n_f;++i) {
    p=rnd_st->flat[i];
    if(RN_P_IS(p,RN_P_ELEMENT)) {
      rn_Element(p,nc,p1);
      path(rnv, rnd_st, p1,nc);
    }
  }
}

static void restrictions(rnv_t *rnv, rnd_st_t *rnd_st) {
  loops(rnv, rnd_st); if(rnd_st->errors) return; /* loops can cause endless loops in subsequent calls */
  ctypes(rnv, rnd_st);
  paths(rnv, rnd_st);
}

static void nullables(rnv_t *rnv, rnd_st_t *rnd_st) {
  int i,p,p1,p2,changed;
  do {
    changed=0;
    for(i=0;i!=rnd_st->n_f;++i) {
      p=rnd_st->flat[i];
      if(!rn_nullable(p)) {
	switch(RN_P_TYP(p)) {
	case RN_P_NOT_ALLOWED:
	case RN_P_DATA: case RN_P_DATA_EXCEPT: case RN_P_VALUE: case RN_P_LIST:
	case RN_P_ATTRIBUTE: case RN_P_ELEMENT:
	  break;

	case RN_P_CHOICE: rn_Choice(p,p1,p2); rn_setNullable(p,rn_nullable(p1)||rn_nullable(p2)); break;
	case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); rn_setNullable(p,rn_nullable(p1)&&rn_nullable(p2)); break;
	case RN_P_GROUP: rn_Group(p,p1,p2);  rn_setNullable(p,rn_nullable(p1)&&rn_nullable(p2)); break;

	case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); rn_setNullable(p,rn_nullable(p1)); break;

	default: assert(0);
	}
	changed=changed||rn_nullable(p);
      }
    }
  } while(changed);
}

static void cdatas(rnv_t *rnv, rnd_st_t *rnd_st) {
  int i,p,p1,p2,changed;
  do {
    changed=0;
    for(i=0;i!=rnd_st->n_f;++i) {
      p=rnd_st->flat[i];
      if(!rn_cdata(p)) {
	switch(RN_P_TYP(p)) {
	case RN_P_NOT_ALLOWED: case RN_P_EMPTY:
	case RN_P_ATTRIBUTE: case RN_P_ELEMENT:
	  break;

	case RN_P_CHOICE: rn_Choice(p,p1,p2); rn_setCdata(p,rn_cdata(p1)||rn_cdata(p2)); break;
	case RN_P_INTERLEAVE: rn_Interleave(p,p1,p2); rn_setCdata(p,rn_cdata(p1)||rn_cdata(p2)); break;
	case RN_P_GROUP: rn_Group(p,p1,p2);  rn_setCdata(p,rn_cdata(p1)||rn_cdata(p2)); break;

	case RN_P_ONE_OR_MORE: rn_OneOrMore(p,p1); rn_setCdata(p,rn_cdata(p1)); break;

	default: assert(0);
	}
	changed=changed||rn_cdata(p);
      }
    }
  } while(changed);
}

static void traits(rnv_t *rnv, rnd_st_t *rnd_st) {
  nullables(rnv, rnd_st);
  cdatas(rnv, rnd_st);
}

static int release(rnd_st_t *rnd_st) {
  int start=rnd_st->flat[0];
  m_free(rnd_st->flat); rnd_st->flat=NULL;
  return start;
}

int rnd_fixup(rnv_t *rnv, rnd_st_t *rnd_st, rn_st_t *rn_st, int start) {
  rnd_st->errors=0; deref(rnv, rnd_st, rn_st, start);
  if(!rnd_st->errors) {restrictions(rnv, rnd_st); if(!rnd_st->errors) traits(rnv, rnd_st);}
  start=release(rnd_st); return rnd_st->errors?0:start;
}
