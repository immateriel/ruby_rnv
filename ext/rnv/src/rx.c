#include "type.h"

/* $Id: rx.c,v 1.33 2004/02/25 00:00:32 dvd Exp $ */

#include <string.h> /*strlen,strcpy,strcmp*/
#include <assert.h>
#include "u.h" /*u_get,u_strlen*/
#include "xmlc.h"
#include "m.h"
#include "s.h"
#include "ht.h"
#include "ll.h"
#include "er.h"
#include "rx.h"
#include "erbit.h"

#define LEN_P RX_LEN_P
#define PRIME_P RX_PRIME_P
#define LIM_P RX_LIM_P
#define LEN_2 RX_LEN_2
#define PRIME_2 RX_PRIME_2
#define LEN_R RX_LEN_R
#define PRIME_R RX_PRIME_R

#define R_AVG_SIZE 16

/* it is good to have few patterns when deltas are memoized */
#define P_ERROR 0
#define P_NOT_ALLOWED  1
#define P_EMPTY 2
#define P_CHOICE 3
#define P_GROUP 4
#define P_ONE_OR_MORE 5 /*+*/
#define P_EXCEPT 6 /*single-single*/
#define P_RANGE 7 /*lower,upper inclusive*/
#define P_CLASS 8 /*complement is .-*/
#define P_ANY 9
#define P_CHAR 10

#define P_SIZE 3
#define P_AVG_SIZE 2

static int p_size[]={1,1,1,3,3,2,3,3,2,1,2};

#define P_TYP(i) (rx_st->pattern[i]&0xF)
#define P_IS(i,x)  (x==P_TYP(i))
#define P_CHK(i,x)  assert(P_IS(i,x))

#define P_unop(TYP,p,p1) P_CHK(p,TYP); p1=rx_st->pattern[p+1]
#define P_binop(TYP,p,p1,p2) P_unop(TYP,p,p1); p2=rx_st->pattern[p+2]
#define NotAllowed(p) P_CHK(p,P_NotAllowed)
#define Empty(p) P_CHK(p,P_Empty)
#define Any(p) P_CHK(p,P_Any)
#define Choice(p,p1,p2) P_binop(P_CHOICE,p,p1,p2)
#define Group(p,p1,p2) P_binop(P_GROUP,p,p1,p2)
#define OneOrMore(p,p1) P_unop(P_ONE_OR_MORE,p,p1)
#define Except(p,p1,p2) P_binop(P_EXCEPT,p,p1,p2)
#define Range(p,cf,cl) P_binop(P_RANGE,p,cf,cl)
#define Class(p,cn) P_unop(P_CLASS,p,cn)
#define Char(p,c) P_unop(P_CHAR,p,c)

#define P_NUL 0x100

#define setNullable(x) if(x) rx_st->pattern[rx_st->i_p]|=P_NUL
#define nullable(p) (rx_st->pattern[p]&P_NUL)

/* 'compact' in drv and rx do different things.
 In drv, it limits the size of the table of memoized deltas. In rx, it limits the size
 of the buffer for cached regular expressions; memoized deltas are always limited by LIM_M,
 since the whole repertoire of unicode characters can blow up the buffer.
 */

static int accept_p(rx_st_t *rx_st) {
  int j;
  if((j=ht_get(&rx_st->ht_p,rx_st->i_p))==-1) {
    ht_put(&rx_st->ht_p,j=rx_st->i_p);
    rx_st->i_p+=p_size[P_TYP(rx_st->i_p)];
    if(rx_st->i_p+P_SIZE>rx_st->len_p) rx_st->pattern=(int*)m_stretch(rx_st->pattern,rx_st->len_p=2*(rx_st->i_p+P_SIZE),rx_st->i_p,sizeof(int));
  }
  return j;
}

#define P_NEW(x) (rx_st->pattern[rx_st->i_p]=x)

#define P_newunop(TYP,p1) P_NEW(TYP); rx_st->pattern[rx_st->i_p+1]=p1
#define P_newbinop(TYP,p1,p2) P_newunop(TYP,p1); rx_st->pattern[rx_st->i_p+2]=p2
static int newNotAllowed(rx_st_t *rx_st) {P_NEW(P_NOT_ALLOWED); return accept_p(rx_st);}
static int newEmpty(rx_st_t *rx_st) {P_NEW(P_EMPTY); setNullable(1); return accept_p(rx_st);}
static int newAny(rx_st_t *rx_st) {P_NEW(P_ANY); return accept_p(rx_st);}
static int newChoice(rx_st_t *rx_st, int p1,int p2) {P_newbinop(P_CHOICE,p1,p2); setNullable(nullable(p1)||nullable(p2)); return accept_p(rx_st);}
static int newGroup(rx_st_t *rx_st, int p1,int p2) {P_newbinop(P_GROUP,p1,p2); setNullable(nullable(p1)&&nullable(p2)); return accept_p(rx_st);}
static int newOneOrMore(rx_st_t *rx_st, int p1) {P_newunop(P_ONE_OR_MORE,p1); setNullable(nullable(p1)); return accept_p(rx_st);}
static int newExcept(rx_st_t *rx_st, int p1,int p2) {P_newbinop(P_EXCEPT,p1,p2); return accept_p(rx_st);}
static int newRange(rx_st_t *rx_st, int cf,int cl) {P_newbinop(P_RANGE,cf,cl); return accept_p(rx_st);}
static int newClass(rx_st_t *rx_st, int cn) {P_newunop(P_CLASS,cn); return accept_p(rx_st);}
static int newChar(rx_st_t *rx_st, int c) {P_newunop(P_CHAR,c); return accept_p(rx_st);}

static int one_or_more(rx_st_t *rx_st, int p) {
  if(P_IS(p,P_EMPTY)) return p;
  if(P_IS(p,P_NOT_ALLOWED)) return p;
  return newOneOrMore(rx_st, p);
}

static int group(rx_st_t *rx_st, int p1,int p2) {
  if(P_IS(p1,P_NOT_ALLOWED)) return p1;
  if(P_IS(p2,P_NOT_ALLOWED)) return p2;
  if(P_IS(p1,P_EMPTY)) return p2;
  if(P_IS(p2,P_EMPTY)) return p1;
  return newGroup(rx_st, p1,p2);
}

static int samechoice(rx_st_t *rx_st, int p1,int p2) {
  if(P_IS(p1,P_CHOICE)) {
    int p11,p12; Choice(p1,p11,p12);
    return p12==p2||samechoice(rx_st, p11,p2);
  } else return p1==p2;
}

static int choice(rx_st_t *rx_st, int p1,int p2) {
  if(P_IS(p1,P_NOT_ALLOWED)) return p2;
  if(P_IS(p2,P_NOT_ALLOWED)) return p1;
  if(P_IS(p2,P_CHOICE)) {
    int p21,p22; Choice(p2,p21,p22);
    p1=choice(rx_st, p1,p21); return choice(rx_st, p1,p22);
  }
  if(samechoice(rx_st, p1,p2)) return p1;
  if(nullable(p1) && (P_IS(p2,P_EMPTY))) return p1;
  if(nullable(p2) && (P_IS(p1,P_EMPTY))) return p2;
  return newChoice(rx_st, p1,p2);
}

static int cls(rx_st_t *rx_st, int cn) {
  if(cn<0) return newExcept(rx_st, rx_st->any,newClass(rx_st, -cn));
  if(cn==0) return rx_st->notAllowed;
  return newClass(rx_st, cn);
}

static int equal_r(void *user, int r1,int r2) {
  rx_st_t *rx_st = (rx_st_t *)user;
  return strcmp(rx_st->regex+r1,rx_st->regex+r2)==0;
}
static int hash_r(void *user, int r) {
  rx_st_t *rx_st = (rx_st_t *)user;
  return s_hval(rx_st->regex+r);
}

static int equal_p(void *user, int p1,int p2) {
  rx_st_t *rx_st = (rx_st_t *)user;
  int *pp1=rx_st->pattern+p1,*pp2=rx_st->pattern+p2;
  if(P_TYP(p1)!=P_TYP(p2)) return 0;
  switch(p_size[P_TYP(p1)]) {
  case 3: if(pp1[2]!=pp2[2]) return 0;
  case 2: if(pp1[1]!=pp2[1]) return 0;
  case 1: return 1;
  default: assert(0);
  }
  return 0;
}
static int hash_p(void *user, int p) {
  rx_st_t *rx_st = (rx_st_t *)user;
  int *pp=rx_st->pattern+p; int h=0;
  switch(p_size[P_TYP(p)]) {
  case 1: h=pp[0]&0xF; break;
  case 2: h=(pp[0]&0xF)|(pp[1]<<4); break;
  case 3: h=(pp[0]&0xF)|((pp[1]^pp[2])<<4); break;
  default: assert(0);
  }
  return h*PRIME_P;
}

static int equal_2(void *user, int x1,int x2) {
  rx_st_t *rx_st = (rx_st_t *)user;
  return rx_st->r2p[x1][0]==rx_st->r2p[x2][0];
}
static int hash_2(void *user, int x) {
  rx_st_t *rx_st = (rx_st_t *)user;
  return rx_st->r2p[x][0]*PRIME_2;
}

static int add_r(rx_st_t *rx_st, char *rx) {
  int len=strlen(rx)+1;
  if(rx_st->i_r+len>rx_st->len_r) rx_st->regex=(char*)m_stretch(rx_st->regex,rx_st->len_r=2*(rx_st->i_r+len),rx_st->i_r,sizeof(char));
  strcpy(rx_st->regex+rx_st->i_r,rx);
  return len;
}

#define ERRPOS

#define err(msg) (*rnv->verror_handler)(rnv,erno|ERBIT_RX,msg" in \"%s\" at offset %i\n",ap)
void rx_default_verror_handler(rnv_t *rnv, int erno,va_list ap) {
  (*er_printf)("regular expressions: ");
  switch(erno) {
  case RX_ER_BADCH: err("bad character"); break;
  case RX_ER_UNFIN: err("unfinished expression"); break;
  case RX_ER_NOLSQ: err("'[' expected"); break;
  case RX_ER_NORSQ: err("']' expected"); break;
  case RX_ER_NOLCU: err("'{' expected"); break;
  case RX_ER_NORCU: err("'}' expected"); break;
  case RX_ER_NOLPA: err("'(' expected"); break;
  case RX_ER_NORPA: err("')' expected"); break;
  case RX_ER_BADCL: err("unknown class"); break;
  case RX_ER_NODGT: err("digit expected"); break;
  case RX_ER_DNUOB: err("reversed bounds"); break;
  case RX_ER_NOTRC: err("range or class expected"); break;
  default: assert(0);
  }
}

//void (*rx_verror_handler)(int erno,va_list ap)=&rx_default_verror_handler;

static void error_handler(rx_st_t *rx_st,int erno,...) {
  va_list ap; va_start(ap,erno); (*rx_st->rnv->rx_verror_handler)(rx_st->rnv, erno,ap); va_end(ap);
}

#define LEN_M RX_LEN_M
#define PRIME_M RX_PRIME_M
#define LIM_M RX_LIM_M

#define M_SIZE 3

#define M_SET(p) rx_st->memo[rx_st->i_m][M_SIZE-1]=p
#define M_RET(m) rx_st->memo[m][M_SIZE-1]

static int new_memo(rx_st_t *rx_st, int p,int c) {
  int *me=rx_st->memo[rx_st->i_m];
  ht_deli(&rx_st->ht_m,rx_st->i_m);
  me[0]=p; me[1]=c;
  return ht_get(&rx_st->ht_m,rx_st->i_m);
}

static int equal_m(void *user,int m1,int m2) {
  rx_st_t *rx_st = (rx_st_t *)user;
  int *me1=rx_st->memo[m1],*me2=rx_st->memo[m2];
  return (me1[0]==me2[0])&&(me1[1]==me2[1]);
}
static int hash_m(void *user,int m) {
  rx_st_t *rx_st = (rx_st_t *)user;
  int *me=rx_st->memo[m];
  return (me[0]^me[1])*PRIME_M;
}

static void accept_m(rx_st_t *rx_st) {
  if(ht_get(&rx_st->ht_m,rx_st->i_m)!=-1) ht_del(&rx_st->ht_m,rx_st->i_m);
  ht_put(&rx_st->ht_m,rx_st->i_m++);
  if(rx_st->i_m>=LIM_M) rx_st->i_m=0;
  if(rx_st->i_m==rx_st->len_m) rx_st->memo=(int(*)[M_SIZE])m_stretch(rx_st->memo,rx_st->len_m=rx_st->i_m*2,rx_st->i_m,sizeof(int[M_SIZE]));
}

static void windup(rx_st_t *rx_st);
void rx_init(rx_st_t *rx_st) {
    // memset(rx_st, 0, sizeof(rx_st_t));

    rx_st->rnv->rx_verror_handler=&rx_default_verror_handler;

    rx_st->pattern=(int *)m_alloc(rx_st->len_p=P_AVG_SIZE*LEN_P,sizeof(int));
    rx_st->r2p=(int (*)[2])m_alloc(rx_st->len_2=LEN_2,sizeof(int[2]));
    rx_st->regex=(char*)m_alloc(rx_st->len_r=R_AVG_SIZE*LEN_R,sizeof(char));
    rx_st->memo=(int (*)[M_SIZE])m_alloc(rx_st->len_m=LEN_M,sizeof(int[M_SIZE]));

    rx_st->ht_p.user = rx_st;
    rx_st->ht_2.user = rx_st;
    rx_st->ht_r.user = rx_st;
    rx_st->ht_m.user = rx_st;

    ht_init(&rx_st->ht_p,LEN_P,&hash_p,&equal_p);
    ht_init(&rx_st->ht_2,LEN_2,&hash_2,&equal_2);
    ht_init(&rx_st->ht_r,LEN_R,&hash_r,&equal_r);
    ht_init(&rx_st->ht_m,LEN_M,&hash_m,&equal_m);

    windup(rx_st);
}

void rx_clear(rx_st_t *rx_st) {
  ht_clear(&rx_st->ht_p); ht_clear(&rx_st->ht_2); ht_clear(&rx_st->ht_r); ht_clear(&rx_st->ht_m);
  windup(rx_st);
}

static void windup(rx_st_t *rx_st) {
  rx_st->i_p=rx_st->i_r=rx_st->i_2=rx_st->i_m=0;
  rx_st->pattern[0]=P_ERROR;  accept_p(rx_st);
  rx_st->empty=newEmpty(rx_st); rx_st->notAllowed=newNotAllowed(rx_st); rx_st->any=newAny(rx_st);
}

#define SYM_END 0
#define SYM_CLS 1
#define SYM_ESC 2
#define SYM_CHR 3

static void error(rx_st_t *rx_st, int erno) {
  if(!rx_st->errors) error_handler(rx_st, erno,rx_st->regex+rx_st->r0,u_strlen(rx_st->regex+rx_st->r0)-u_strlen(rx_st->regex+rx_st->ri));
  ++rx_st->errors;
}

#include "rx_cls_u.c"

static int chclass(rx_st_t *rx_st) {
  int u,cl,rj;
  rx_st->ri+=u_get(&u,rx_st->regex+rx_st->ri);
  if(u=='\0') {--rx_st->ri; error(rx_st, RX_ER_NOLCU); return 0;}
  if(u!='{') {error(rx_st, RX_ER_NOLCU); return 0;}
  rj=rx_st->ri;
  for(;;) {
    if(rx_st->regex[rj]=='\0') {rx_st->ri=rj; error(rx_st, RX_ER_NORCU); return 0;}
    if(rx_st->regex[rj]=='}') {
      if((cl=s_ntab(rx_st->regex+rx_st->ri,rj-rx_st->ri,clstab,NUM_CLS_U))==NUM_CLS_U) {error(rx_st, RX_ER_BADCL); cl=0;}
      rx_st->ri=rj+1;
      return cl;
    }
    ++rj;
  }
}

#define CLS_NL (NUM_CLS_U+1)
#define CLS_S (NUM_CLS_U+2)
#define CLS_I (NUM_CLS_U+3)
#define CLS_C (NUM_CLS_U+4)
#define CLS_W (NUM_CLS_U+5)
#define NUM_CLS (NUM_CLS_U+6)

static void getsym(rx_st_t *rx_st) {
  int u;
  if(rx_st->regex[rx_st->ri]=='\0') rx_st->sym=SYM_END; else {
    rx_st->ri+=u_get(&u,rx_st->regex+rx_st->ri);
    if(u=='\\') {
      rx_st->ri+=u_get(&u,rx_st->regex+rx_st->ri);
      switch(u) {
      case '\0': --rx_st->ri; error(rx_st, RX_ER_UNFIN); rx_st->sym=SYM_END; break;
      case 'p': rx_st->sym=SYM_CLS; rx_st->val=chclass(rx_st); break;
      case 'P': rx_st->sym=SYM_CLS; rx_st->val=-chclass(rx_st); break;
      case 's': rx_st->sym=SYM_CLS; rx_st->val=CLS_S; break;
      case 'S': rx_st->sym=SYM_CLS; rx_st->val=-CLS_S; break;
      case 'i': rx_st->sym=SYM_CLS; rx_st->val=CLS_I; break;
      case 'I': rx_st->sym=SYM_CLS; rx_st->val=-CLS_I; break;
      case 'c': rx_st->sym=SYM_CLS; rx_st->val=CLS_C; break;
      case 'C': rx_st->sym=SYM_CLS; rx_st->val=-CLS_C; break;
      case 'd': rx_st->sym=SYM_CLS; rx_st->val=CLS_U_Nd; break;
      case 'D': rx_st->sym=SYM_CLS; rx_st->val=-CLS_U_Nd; break;
      case 'w': rx_st->sym=SYM_CLS; rx_st->val=CLS_W; break;
      case 'W': rx_st->sym=SYM_CLS; rx_st->val=-CLS_W; break;
      case 'n': rx_st->sym=SYM_ESC; rx_st->val=0xA; break;
      case 'r': rx_st->sym=SYM_ESC; rx_st->val=0xD; break;
      case 't': rx_st->sym=SYM_ESC; rx_st->val=0x9; break;
      case '\\': case '|': case '.': case '-': case '^': case '?': case '*': case '+':
      case '{': case '}': case '[': case ']': case '(': case ')':
	rx_st->sym=SYM_ESC; rx_st->val=u; break;
      default: error(rx_st, RX_ER_BADCH); rx_st->sym=SYM_ESC; rx_st->val=u; break;
      }
    } else {
      switch(u) {
      case '.': rx_st->sym=SYM_CLS; rx_st->val=-CLS_NL; break;
      default: rx_st->sym=SYM_CHR; rx_st->val=u; break;
      }
    }
  }
}

static void chk_get(rx_st_t *rx_st, int v,int erno) {if(rx_st->sym!=SYM_CHR||rx_st->val!=v) error(rx_st, erno); getsym(rx_st);}


#define chkrch(val) if((val)=='['||(val)==']'||(val)=='-') error(rx_st, RX_ER_NOTRC)

static int chgroup(rx_st_t *rx_st) {
  int p=rx_st->notAllowed,c;
  for(;;) {
    switch(rx_st->sym) {
    case SYM_CHR: chkrch(rx_st->val);
    case SYM_ESC: c=rx_st->val; getsym(rx_st);
      if(rx_st->sym==SYM_CHR&&rx_st->val=='-') {
	if(rx_st->regex[rx_st->ri]=='[') {
	  p=choice(rx_st, p,newChar(rx_st, c));
	  goto END_OF_GROUP;
	} else {
	  getsym(rx_st);
	  switch(rx_st->sym) {
	  case SYM_CHR: chkrch(rx_st->val);
	  case SYM_ESC: p=choice(rx_st, p,newRange(rx_st, c,rx_st->val)); getsym(rx_st); break;
	  default: error(rx_st, RX_ER_BADCH); getsym(rx_st); break;
	  }
	}
      } else {
	p=choice(rx_st, p,newChar(rx_st, c));
      }
      break;
    case SYM_CLS: p=choice(rx_st, p,cls(rx_st, rx_st->val)); getsym(rx_st); break;
    case SYM_END: error(rx_st, RX_ER_NORSQ); goto END_OF_GROUP;
    default: assert(0);
    }
    if(rx_st->sym==SYM_CHR&&(rx_st->val==']'||rx_st->val=='-')) goto END_OF_GROUP;
  }
  END_OF_GROUP:;
  return p;
}

static int chexpr(rx_st_t *rx_st) {
  int p;
  if(rx_st->sym==SYM_CHR&&rx_st->val=='^') { getsym(rx_st);
    p=newExcept(rx_st, rx_st->any,chgroup(rx_st));
  } else {
    p=chgroup(rx_st);
  }
  if(rx_st->sym==SYM_CHR&&rx_st->val=='-') { getsym(rx_st);
    chk_get(rx_st, '[',RX_ER_NOLSQ); p=newExcept(rx_st, p,chexpr(rx_st)); chk_get(rx_st, ']',RX_ER_NORSQ);
  }
  return p;
}

static int expression(rx_st_t *rx_st);
static int atom(rx_st_t *rx_st) {
  int p=0;
  switch(rx_st->sym) {
  case SYM_CHR:
    switch(rx_st->val) {
    case '[': getsym(rx_st); p=chexpr(rx_st); chk_get(rx_st, ']',RX_ER_NORSQ); break;
    case '(': getsym(rx_st); p=expression(rx_st); chk_get(rx_st, ')',RX_ER_NORPA); break;
    case '{': case '?': case '*': case '+': case '|':
    case ')': case ']': case '}': error(rx_st, RX_ER_BADCH); getsym(rx_st); break;
    default: p=newChar(rx_st, rx_st->val); getsym(rx_st); break;
    }
    break;
  case SYM_ESC: p=newChar(rx_st, rx_st->val); getsym(rx_st); break;
  case SYM_CLS: p=cls(rx_st, rx_st->val); getsym(rx_st); break;
  default: error(rx_st, RX_ER_BADCH); getsym(rx_st); break;
  }
  return p;
}

static int number(rx_st_t *rx_st) {
  int n=0,m;
  for(;;) {
    if(rx_st->sym!=SYM_CHR) goto END_OF_DIGITS;
    switch(rx_st->val) {
    case '0': m=0; break;
    case '1': m=1; break;
    case '2': m=2; break;
    case '3': m=3; break;
    case '4': m=4; break;
    case '5': m=5; break;
    case '6': m=6; break;
    case '7': m=7; break;
    case '8': m=8; break;
    case '9': m=9; break;
    default: goto END_OF_DIGITS;
    }
    n=n*10+m;
    getsym(rx_st);
  }
  END_OF_DIGITS:;
  return n;
}

static int quantifier(rx_st_t *rx_st, int p0) {
  int p=rx_st->empty,n,n0;
  n=n0=number(rx_st);
  while(n--) p=group(rx_st, p,p0);
  if(rx_st->sym==SYM_CHR) {
    if(rx_st->val==',') {
      getsym(rx_st);
      if(rx_st->sym==SYM_CHR && rx_st->val=='}') {
	p=group(rx_st, p,choice(rx_st, rx_st->empty,one_or_more(rx_st, p0)));
      } else {
	n=number(rx_st)-n0; if(n<0) {error(rx_st, RX_ER_DNUOB); n=0;}
	while(n--) p=group(rx_st, p,choice(rx_st, rx_st->empty,p0));
      }
    }
  } else error(rx_st, RX_ER_NODGT);
  return p;
}

static int piece(rx_st_t *rx_st) {
  int p;
  p=atom(rx_st);
  if(rx_st->sym==SYM_CHR) {
    switch(rx_st->val) {
    case '{': getsym(rx_st); p=quantifier(rx_st, p); chk_get(rx_st, '}',RX_ER_NOLCU); break;
    case '?': getsym(rx_st); p=choice(rx_st, rx_st->empty,p); break;
    case '*': getsym(rx_st); p=choice(rx_st, rx_st->empty,one_or_more(rx_st, p)); break;
    case '+': getsym(rx_st); p=one_or_more(rx_st, p); break;
    default: break;
    }
  }
  return p;
}

static int branch(rx_st_t *rx_st) {
  int p;
  p=rx_st->empty;
  while(!(rx_st->sym==SYM_END||(rx_st->sym==SYM_CHR&&(rx_st->val=='|'||rx_st->val==')')))) p=group(rx_st, p,piece(rx_st));
  return p;
}

static int expression(rx_st_t *rx_st) {
  int p;
  p=branch(rx_st);
  while(rx_st->sym==SYM_CHR&&rx_st->val=='|') {
    getsym(rx_st);
    p=choice(rx_st, p,branch(rx_st));
  }
  return p;
}

static void bind(rx_st_t *rx_st, int r) {
  rx_st->r0=rx_st->ri=r; rx_st->sym=-1; rx_st->errors=0;
  getsym(rx_st);
}

static int compile(rnv_t *rnv, rx_st_t *rx_st, char *rx) {
  int r=0,p=0,d_r;
  d_r=add_r(rx_st, rx);
  if((r=ht_get(&rx_st->ht_r,rx_st->i_r))==-1) {
    if(rnv->rx_compact&&rx_st->i_p>=P_AVG_SIZE*LIM_P) {rx_clear(rx_st); d_r=add_r(rx_st, rx);}
    ht_put(&rx_st->ht_r,r=rx_st->i_r);
    rx_st->i_r+=d_r;
    bind(rx_st, r); p=expression(rx_st); if(rx_st->sym!=SYM_END) error(rx_st, RX_ER_BADCH);
    rx_st->r2p[rx_st->i_2][0]=r; rx_st->r2p[rx_st->i_2][1]=p;
    ht_put(&rx_st->ht_2,rx_st->i_2++);
    if(rx_st->i_2==rx_st->len_2) rx_st->r2p=(int(*)[2])m_stretch(rx_st->r2p,rx_st->len_2=2*rx_st->i_2,rx_st->i_2,sizeof(int[2]));
  } else {
    rx_st->r2p[rx_st->i_2][0]=r;
    p=rx_st->r2p[ht_get(&rx_st->ht_2,rx_st->i_2)][1];
  }
  return p;
}

#include "rx_cls_ranges.c"

static int in_class(int c,int cn) {
  switch(cn) {
  case 0: return 0;
  case CLS_U_C: return in_class(c,CLS_U_Cc)||in_class(c,CLS_U_Cf)||in_class(c,CLS_U_Co);
  case CLS_U_Cc: return u_in_ranges(c,CcRanges,sizeof(CcRanges)/sizeof(int[2]));
  case CLS_U_Cf: return u_in_ranges(c,CfRanges,sizeof(CfRanges)/sizeof(int[2]));
  case CLS_U_Co: return u_in_ranges(c,CoRanges,sizeof(CoRanges)/sizeof(int[2]));
  case CLS_U_IsAlphabeticPresentationForms: return u_in_ranges(c,IsAlphabeticPresentationFormsRanges,sizeof(IsAlphabeticPresentationFormsRanges)/sizeof(int[2]));
  case CLS_U_IsArabic: return u_in_ranges(c,IsArabicRanges,sizeof(IsArabicRanges)/sizeof(int[2]));
  case CLS_U_IsArabicPresentationForms_A: return u_in_ranges(c,IsArabicPresentationForms_ARanges,sizeof(IsArabicPresentationForms_ARanges)/sizeof(int[2]));
  case CLS_U_IsArabicPresentationForms_B: return u_in_ranges(c,IsArabicPresentationForms_BRanges,sizeof(IsArabicPresentationForms_BRanges)/sizeof(int[2]));
  case CLS_U_IsArmenian: return u_in_ranges(c,IsArmenianRanges,sizeof(IsArmenianRanges)/sizeof(int[2]));
  case CLS_U_IsArrows: return u_in_ranges(c,IsArrowsRanges,sizeof(IsArrowsRanges)/sizeof(int[2]));
  case CLS_U_IsBasicLatin: return u_in_ranges(c,IsBasicLatinRanges,sizeof(IsBasicLatinRanges)/sizeof(int[2]));
  case CLS_U_IsBengali: return u_in_ranges(c,IsBengaliRanges,sizeof(IsBengaliRanges)/sizeof(int[2]));
  case CLS_U_IsBlockElements: return u_in_ranges(c,IsBlockElementsRanges,sizeof(IsBlockElementsRanges)/sizeof(int[2]));
  case CLS_U_IsBopomofo: return u_in_ranges(c,IsBopomofoRanges,sizeof(IsBopomofoRanges)/sizeof(int[2]));
  case CLS_U_IsBopomofoExtended: return u_in_ranges(c,IsBopomofoExtendedRanges,sizeof(IsBopomofoExtendedRanges)/sizeof(int[2]));
  case CLS_U_IsBoxDrawing: return u_in_ranges(c,IsBoxDrawingRanges,sizeof(IsBoxDrawingRanges)/sizeof(int[2]));
  case CLS_U_IsBraillePatterns: return u_in_ranges(c,IsBraillePatternsRanges,sizeof(IsBraillePatternsRanges)/sizeof(int[2]));
  case CLS_U_IsByzantineMusicalSymbols: return u_in_ranges(c,IsByzantineMusicalSymbolsRanges,sizeof(IsByzantineMusicalSymbolsRanges)/sizeof(int[2]));
  case CLS_U_IsCJKCompatibility: return u_in_ranges(c,IsCJKCompatibilityRanges,sizeof(IsCJKCompatibilityRanges)/sizeof(int[2]));
  case CLS_U_IsCJKCompatibilityForms: return u_in_ranges(c,IsCJKCompatibilityFormsRanges,sizeof(IsCJKCompatibilityFormsRanges)/sizeof(int[2]));
  case CLS_U_IsCJKCompatibilityIdeographs: return u_in_ranges(c,IsCJKCompatibilityIdeographsRanges,sizeof(IsCJKCompatibilityIdeographsRanges)/sizeof(int[2]));
  case CLS_U_IsCJKCompatibilityIdeographsSupplement: return u_in_ranges(c,IsCJKCompatibilityIdeographsSupplementRanges,sizeof(IsCJKCompatibilityIdeographsSupplementRanges)/sizeof(int[2]));
  case CLS_U_IsCJKRadicalsSupplement: return u_in_ranges(c,IsCJKRadicalsSupplementRanges,sizeof(IsCJKRadicalsSupplementRanges)/sizeof(int[2]));
  case CLS_U_IsCJKSymbolsandPunctuation: return u_in_ranges(c,IsCJKSymbolsandPunctuationRanges,sizeof(IsCJKSymbolsandPunctuationRanges)/sizeof(int[2]));
  case CLS_U_IsCJKUnifiedIdeographs: return u_in_ranges(c,IsCJKUnifiedIdeographsRanges,sizeof(IsCJKUnifiedIdeographsRanges)/sizeof(int[2]));
  case CLS_U_IsCJKUnifiedIdeographsExtensionA: return u_in_ranges(c,IsCJKUnifiedIdeographsExtensionARanges,sizeof(IsCJKUnifiedIdeographsExtensionARanges)/sizeof(int[2]));
  case CLS_U_IsCJKUnifiedIdeographsExtensionB: return u_in_ranges(c,IsCJKUnifiedIdeographsExtensionBRanges,sizeof(IsCJKUnifiedIdeographsExtensionBRanges)/sizeof(int[2]));
  case CLS_U_IsCherokee: return u_in_ranges(c,IsCherokeeRanges,sizeof(IsCherokeeRanges)/sizeof(int[2]));
  case CLS_U_IsCombiningDiacriticalMarks: return u_in_ranges(c,IsCombiningDiacriticalMarksRanges,sizeof(IsCombiningDiacriticalMarksRanges)/sizeof(int[2]));
  case CLS_U_IsCombiningHalfMarks: return u_in_ranges(c,IsCombiningHalfMarksRanges,sizeof(IsCombiningHalfMarksRanges)/sizeof(int[2]));
  case CLS_U_IsCombiningMarksforSymbols: return u_in_ranges(c,IsCombiningMarksforSymbolsRanges,sizeof(IsCombiningMarksforSymbolsRanges)/sizeof(int[2]));
  case CLS_U_IsControlPictures: return u_in_ranges(c,IsControlPicturesRanges,sizeof(IsControlPicturesRanges)/sizeof(int[2]));
  case CLS_U_IsCurrencySymbols: return u_in_ranges(c,IsCurrencySymbolsRanges,sizeof(IsCurrencySymbolsRanges)/sizeof(int[2]));
  case CLS_U_IsCyrillic: return u_in_ranges(c,IsCyrillicRanges,sizeof(IsCyrillicRanges)/sizeof(int[2]));
  case CLS_U_IsDeseret: return u_in_ranges(c,IsDeseretRanges,sizeof(IsDeseretRanges)/sizeof(int[2]));
  case CLS_U_IsDevanagari: return u_in_ranges(c,IsDevanagariRanges,sizeof(IsDevanagariRanges)/sizeof(int[2]));
  case CLS_U_IsDingbats: return u_in_ranges(c,IsDingbatsRanges,sizeof(IsDingbatsRanges)/sizeof(int[2]));
  case CLS_U_IsEnclosedAlphanumerics: return u_in_ranges(c,IsEnclosedAlphanumericsRanges,sizeof(IsEnclosedAlphanumericsRanges)/sizeof(int[2]));
  case CLS_U_IsEnclosedCJKLettersandMonths: return u_in_ranges(c,IsEnclosedCJKLettersandMonthsRanges,sizeof(IsEnclosedCJKLettersandMonthsRanges)/sizeof(int[2]));
  case CLS_U_IsEthiopic: return u_in_ranges(c,IsEthiopicRanges,sizeof(IsEthiopicRanges)/sizeof(int[2]));
  case CLS_U_IsGeneralPunctuation: return u_in_ranges(c,IsGeneralPunctuationRanges,sizeof(IsGeneralPunctuationRanges)/sizeof(int[2]));
  case CLS_U_IsGeometricShapes: return u_in_ranges(c,IsGeometricShapesRanges,sizeof(IsGeometricShapesRanges)/sizeof(int[2]));
  case CLS_U_IsGeorgian: return u_in_ranges(c,IsGeorgianRanges,sizeof(IsGeorgianRanges)/sizeof(int[2]));
  case CLS_U_IsGothic: return u_in_ranges(c,IsGothicRanges,sizeof(IsGothicRanges)/sizeof(int[2]));
  case CLS_U_IsGreek: return u_in_ranges(c,IsGreekRanges,sizeof(IsGreekRanges)/sizeof(int[2]));
  case CLS_U_IsGreekExtended: return u_in_ranges(c,IsGreekExtendedRanges,sizeof(IsGreekExtendedRanges)/sizeof(int[2]));
  case CLS_U_IsGujarati: return u_in_ranges(c,IsGujaratiRanges,sizeof(IsGujaratiRanges)/sizeof(int[2]));
  case CLS_U_IsGurmukhi: return u_in_ranges(c,IsGurmukhiRanges,sizeof(IsGurmukhiRanges)/sizeof(int[2]));
  case CLS_U_IsHalfwidthandFullwidthForms: return u_in_ranges(c,IsHalfwidthandFullwidthFormsRanges,sizeof(IsHalfwidthandFullwidthFormsRanges)/sizeof(int[2]));
  case CLS_U_IsHangulCompatibilityJamo: return u_in_ranges(c,IsHangulCompatibilityJamoRanges,sizeof(IsHangulCompatibilityJamoRanges)/sizeof(int[2]));
  case CLS_U_IsHangulJamo: return u_in_ranges(c,IsHangulJamoRanges,sizeof(IsHangulJamoRanges)/sizeof(int[2]));
  case CLS_U_IsHangulSyllables: return u_in_ranges(c,IsHangulSyllablesRanges,sizeof(IsHangulSyllablesRanges)/sizeof(int[2]));
  case CLS_U_IsHebrew: return u_in_ranges(c,IsHebrewRanges,sizeof(IsHebrewRanges)/sizeof(int[2]));
  case CLS_U_IsHiragana: return u_in_ranges(c,IsHiraganaRanges,sizeof(IsHiraganaRanges)/sizeof(int[2]));
  case CLS_U_IsIPAExtensions: return u_in_ranges(c,IsIPAExtensionsRanges,sizeof(IsIPAExtensionsRanges)/sizeof(int[2]));
  case CLS_U_IsIdeographicDescriptionCharacters: return u_in_ranges(c,IsIdeographicDescriptionCharactersRanges,sizeof(IsIdeographicDescriptionCharactersRanges)/sizeof(int[2]));
  case CLS_U_IsKanbun: return u_in_ranges(c,IsKanbunRanges,sizeof(IsKanbunRanges)/sizeof(int[2]));
  case CLS_U_IsKangxiRadicals: return u_in_ranges(c,IsKangxiRadicalsRanges,sizeof(IsKangxiRadicalsRanges)/sizeof(int[2]));
  case CLS_U_IsKannada: return u_in_ranges(c,IsKannadaRanges,sizeof(IsKannadaRanges)/sizeof(int[2]));
  case CLS_U_IsKatakana: return u_in_ranges(c,IsKatakanaRanges,sizeof(IsKatakanaRanges)/sizeof(int[2]));
  case CLS_U_IsKhmer: return u_in_ranges(c,IsKhmerRanges,sizeof(IsKhmerRanges)/sizeof(int[2]));
  case CLS_U_IsLao: return u_in_ranges(c,IsLaoRanges,sizeof(IsLaoRanges)/sizeof(int[2]));
  case CLS_U_IsLatin_1Supplement: return u_in_ranges(c,IsLatin_1SupplementRanges,sizeof(IsLatin_1SupplementRanges)/sizeof(int[2]));
  case CLS_U_IsLatinExtended_A: return u_in_ranges(c,IsLatinExtended_ARanges,sizeof(IsLatinExtended_ARanges)/sizeof(int[2]));
  case CLS_U_IsLatinExtended_B: return u_in_ranges(c,IsLatinExtended_BRanges,sizeof(IsLatinExtended_BRanges)/sizeof(int[2]));
  case CLS_U_IsLatinExtendedAdditional: return u_in_ranges(c,IsLatinExtendedAdditionalRanges,sizeof(IsLatinExtendedAdditionalRanges)/sizeof(int[2]));
  case CLS_U_IsLetterlikeSymbols: return u_in_ranges(c,IsLetterlikeSymbolsRanges,sizeof(IsLetterlikeSymbolsRanges)/sizeof(int[2]));
  case CLS_U_IsMalayalam: return u_in_ranges(c,IsMalayalamRanges,sizeof(IsMalayalamRanges)/sizeof(int[2]));
  case CLS_U_IsMathematicalAlphanumericSymbols: return u_in_ranges(c,IsMathematicalAlphanumericSymbolsRanges,sizeof(IsMathematicalAlphanumericSymbolsRanges)/sizeof(int[2]));
  case CLS_U_IsMathematicalOperators: return u_in_ranges(c,IsMathematicalOperatorsRanges,sizeof(IsMathematicalOperatorsRanges)/sizeof(int[2]));
  case CLS_U_IsMiscellaneousSymbols: return u_in_ranges(c,IsMiscellaneousSymbolsRanges,sizeof(IsMiscellaneousSymbolsRanges)/sizeof(int[2]));
  case CLS_U_IsMiscellaneousTechnical: return u_in_ranges(c,IsMiscellaneousTechnicalRanges,sizeof(IsMiscellaneousTechnicalRanges)/sizeof(int[2]));
  case CLS_U_IsMongolian: return u_in_ranges(c,IsMongolianRanges,sizeof(IsMongolianRanges)/sizeof(int[2]));
  case CLS_U_IsMusicalSymbols: return u_in_ranges(c,IsMusicalSymbolsRanges,sizeof(IsMusicalSymbolsRanges)/sizeof(int[2]));
  case CLS_U_IsMyanmar: return u_in_ranges(c,IsMyanmarRanges,sizeof(IsMyanmarRanges)/sizeof(int[2]));
  case CLS_U_IsNumberForms: return u_in_ranges(c,IsNumberFormsRanges,sizeof(IsNumberFormsRanges)/sizeof(int[2]));
  case CLS_U_IsOgham: return u_in_ranges(c,IsOghamRanges,sizeof(IsOghamRanges)/sizeof(int[2]));
  case CLS_U_IsOldItalic: return u_in_ranges(c,IsOldItalicRanges,sizeof(IsOldItalicRanges)/sizeof(int[2]));
  case CLS_U_IsOpticalCharacterRecognition: return u_in_ranges(c,IsOpticalCharacterRecognitionRanges,sizeof(IsOpticalCharacterRecognitionRanges)/sizeof(int[2]));
  case CLS_U_IsOriya: return u_in_ranges(c,IsOriyaRanges,sizeof(IsOriyaRanges)/sizeof(int[2]));
  case CLS_U_IsPrivateUse: return u_in_ranges(c,IsPrivateUseRanges,sizeof(IsPrivateUseRanges)/sizeof(int[2]));
  case CLS_U_IsRunic: return u_in_ranges(c,IsRunicRanges,sizeof(IsRunicRanges)/sizeof(int[2]));
  case CLS_U_IsSinhala: return u_in_ranges(c,IsSinhalaRanges,sizeof(IsSinhalaRanges)/sizeof(int[2]));
  case CLS_U_IsSmallFormVariants: return u_in_ranges(c,IsSmallFormVariantsRanges,sizeof(IsSmallFormVariantsRanges)/sizeof(int[2]));
  case CLS_U_IsSpacingModifierLetters: return u_in_ranges(c,IsSpacingModifierLettersRanges,sizeof(IsSpacingModifierLettersRanges)/sizeof(int[2]));
  case CLS_U_IsSpecials: return u_in_ranges(c,IsSpecialsRanges,sizeof(IsSpecialsRanges)/sizeof(int[2]));
  case CLS_U_IsSuperscriptsandSubscripts: return u_in_ranges(c,IsSuperscriptsandSubscriptsRanges,sizeof(IsSuperscriptsandSubscriptsRanges)/sizeof(int[2]));
  case CLS_U_IsSyriac: return u_in_ranges(c,IsSyriacRanges,sizeof(IsSyriacRanges)/sizeof(int[2]));
  case CLS_U_IsTags: return u_in_ranges(c,IsTagsRanges,sizeof(IsTagsRanges)/sizeof(int[2]));
  case CLS_U_IsTamil: return u_in_ranges(c,IsTamilRanges,sizeof(IsTamilRanges)/sizeof(int[2]));
  case CLS_U_IsTelugu: return u_in_ranges(c,IsTeluguRanges,sizeof(IsTeluguRanges)/sizeof(int[2]));
  case CLS_U_IsThaana: return u_in_ranges(c,IsThaanaRanges,sizeof(IsThaanaRanges)/sizeof(int[2]));
  case CLS_U_IsThai: return u_in_ranges(c,IsThaiRanges,sizeof(IsThaiRanges)/sizeof(int[2]));
  case CLS_U_IsTibetan: return u_in_ranges(c,IsTibetanRanges,sizeof(IsTibetanRanges)/sizeof(int[2]));
  case CLS_U_IsUnifiedCanadianAboriginalSyllabics: return u_in_ranges(c,IsUnifiedCanadianAboriginalSyllabicsRanges,sizeof(IsUnifiedCanadianAboriginalSyllabicsRanges)/sizeof(int[2]));
  case CLS_U_IsYiRadicals: return u_in_ranges(c,IsYiRadicalsRanges,sizeof(IsYiRadicalsRanges)/sizeof(int[2]));
  case CLS_U_IsYiSyllables: return u_in_ranges(c,IsYiSyllablesRanges,sizeof(IsYiSyllablesRanges)/sizeof(int[2]));
  case CLS_U_L: return in_class(c,CLS_U_Ll)||in_class(c,CLS_U_Lm)||in_class(c,CLS_U_Lo)||in_class(c,CLS_U_Lt)||in_class(c,CLS_U_Lu);
  case CLS_U_Ll: return u_in_ranges(c,LlRanges,sizeof(LlRanges)/sizeof(int[2]));
  case CLS_U_Lm: return u_in_ranges(c,LmRanges,sizeof(LmRanges)/sizeof(int[2]));
  case CLS_U_Lo: return u_in_ranges(c,LoRanges,sizeof(LoRanges)/sizeof(int[2]));
  case CLS_U_Lt: return u_in_ranges(c,LtRanges,sizeof(LtRanges)/sizeof(int[2]));
  case CLS_U_Lu: return u_in_ranges(c,LuRanges,sizeof(LuRanges)/sizeof(int[2]));
  case CLS_U_M: return in_class(c,CLS_U_Mc)||in_class(c,CLS_U_Me)||in_class(c,CLS_U_Mn);
  case CLS_U_Mc: return u_in_ranges(c,McRanges,sizeof(McRanges)/sizeof(int[2]));
  case CLS_U_Me: return u_in_ranges(c,MeRanges,sizeof(MeRanges)/sizeof(int[2]));
  case CLS_U_Mn: return u_in_ranges(c,MnRanges,sizeof(MnRanges)/sizeof(int[2]));
  case CLS_U_N: return in_class(c,CLS_U_Nd)||in_class(c,CLS_U_Nl)||in_class(c,CLS_U_No);
  case CLS_U_Nd: return u_in_ranges(c,NdRanges,sizeof(NdRanges)/sizeof(int[2]));
  case CLS_U_Nl: return u_in_ranges(c,NlRanges,sizeof(NlRanges)/sizeof(int[2]));
  case CLS_U_No: return u_in_ranges(c,NoRanges,sizeof(NoRanges)/sizeof(int[2]));
  case CLS_U_P: return in_class(c,CLS_U_Pc)||in_class(c,CLS_U_Pd)||in_class(c,CLS_U_Pe)||in_class(c,CLS_U_Pf)||in_class(c,CLS_U_Pi)||in_class(c,CLS_U_Po)||in_class(c,CLS_U_Ps);
  case CLS_U_Pc: return u_in_ranges(c,PcRanges,sizeof(PcRanges)/sizeof(int[2]));
  case CLS_U_Pd: return u_in_ranges(c,PdRanges,sizeof(PdRanges)/sizeof(int[2]));
  case CLS_U_Pe: return u_in_ranges(c,PeRanges,sizeof(PeRanges)/sizeof(int[2]));
  case CLS_U_Pf: return u_in_ranges(c,PfRanges,sizeof(PfRanges)/sizeof(int[2]));
  case CLS_U_Pi: return u_in_ranges(c,PiRanges,sizeof(PiRanges)/sizeof(int[2]));
  case CLS_U_Po: return u_in_ranges(c,PoRanges,sizeof(PoRanges)/sizeof(int[2]));
  case CLS_U_Ps: return u_in_ranges(c,PsRanges,sizeof(PsRanges)/sizeof(int[2]));
  case CLS_U_S: return in_class(c,CLS_U_Sc)||in_class(c,CLS_U_Sk)||in_class(c,CLS_U_Sm)||in_class(c,CLS_U_So);
  case CLS_U_Sc: return u_in_ranges(c,ScRanges,sizeof(ScRanges)/sizeof(int[2]));
  case CLS_U_Sk: return u_in_ranges(c,SkRanges,sizeof(SkRanges)/sizeof(int[2]));
  case CLS_U_Sm: return u_in_ranges(c,SmRanges,sizeof(SmRanges)/sizeof(int[2]));
  case CLS_U_So: return u_in_ranges(c,SoRanges,sizeof(SoRanges)/sizeof(int[2]));
  case CLS_U_Z: return in_class(c,CLS_U_Zl)||in_class(c,CLS_U_Zp)||in_class(c,CLS_U_Zs);
  case CLS_U_Zl: return u_in_ranges(c,ZlRanges,sizeof(ZlRanges)/sizeof(int[2]));
  case CLS_U_Zp: return u_in_ranges(c,ZpRanges,sizeof(ZpRanges)/sizeof(int[2]));
  case CLS_U_Zs: return u_in_ranges(c,ZsRanges,sizeof(ZsRanges)/sizeof(int[2]));
  case CLS_NL: return c=='\n'||c=='\r';
  case CLS_S: return xmlc_white_space(c);
  case CLS_I: return xmlc_base_char(c)||xmlc_ideographic(c)||c=='_'||c==':';
  case CLS_C: return in_class(c,CLS_I)||xmlc_digit(c)||xmlc_combining_char(c)||xmlc_extender(c)||c=='.'||c=='-';
  case CLS_W: return !(in_class(c,CLS_U_P)||in_class(c,CLS_U_Z)||in_class(c,CLS_U_C));
  default: assert(0);
  }
  return 0;
}


static int drv(rx_st_t *rx_st, int p,int c) {
  int p1,p2,cf,cl,cn,ret,m;
  assert(!P_IS(p,P_ERROR));
  m=new_memo(rx_st, p,c);
  if(m!=-1) return M_RET(m);
  switch(P_TYP(p)) {
  case P_NOT_ALLOWED: case P_EMPTY: ret=rx_st->notAllowed; break;
  case P_CHOICE: Choice(p,p1,p2); ret=choice(rx_st, drv(rx_st, p1,c),drv(rx_st, p2,c)); break;
  case P_GROUP: Group(p,p1,p2); {int p11=group(rx_st, drv(rx_st, p1,c),p2); ret=nullable(p1)?choice(rx_st, p11,drv(rx_st, p2,c)):p11;} break;
  case P_ONE_OR_MORE: OneOrMore(p,p1); ret=group(rx_st, drv(rx_st, p1,c),choice(rx_st, rx_st->empty,p)); break;
  case P_EXCEPT: Except(p,p1,p2); ret=nullable(drv(rx_st, p1,c))&&!nullable(drv(rx_st, p2,c))?rx_st->empty:rx_st->notAllowed; break;
  case P_RANGE: Range(p,cf,cl); ret=cf<=c&&c<=cl?rx_st->empty:rx_st->notAllowed; break;
  case P_CLASS: Class(p,cn); ret=in_class(c,cn)?rx_st->empty:rx_st->notAllowed; break;
  case P_ANY: ret=rx_st->empty; break;
  case P_CHAR: Char(p,cf); ret=c==cf?rx_st->empty:rx_st->notAllowed; break;
  default: ret=0; assert(0);
  }
  new_memo(rx_st, p,c); M_SET(ret);
  accept_m(rx_st);
  return ret;
}

int rx_check(rnv_t *rnv, rx_st_t *rx_st, char *rx) {(void)compile(rnv, rx_st, rx); return !rx_st->errors;}

int rx_match(rnv_t *rnv, rx_st_t *rx_st, char *rx,char *s,int n) {
  int p=compile(rnv, rx_st, rx);
  if(!rx_st->errors) {
    char *end=s+n;
    int u;
    for(;;) {
      if(p==rx_st->notAllowed) return 0;
      if(s==end) return nullable(p);
      s+=u_get(&u,s);
      p=drv(rx_st, p,u);
    }
  } else return 0;
}

int rx_rmatch(rnv_t *rnv, rx_st_t *rx_st, char *rx,char *s,int n) {
  int p=compile(rnv, rx_st, rx);
  if(!rx_st->errors) {
    char *end=s+n;
    int u;
    for(;;) {
      if(p==rx_st->notAllowed) return 0;
      if(s==end) return nullable(p);
      s+=u_get(&u,s);
      if(xmlc_white_space(u)) u=' ';
      p=drv(rx_st, p,u);
    }
  } else return 0;
}

int rx_cmatch(rnv_t *rnv, rx_st_t *rx_st, char *rx,char *s,int n) {
  int p=compile(rnv, rx_st, rx);
  if(!rx_st->errors) {
    char *end=s+n;
    int u;
    SKIP_SPACE: for(;;) {
      if(s==end) return nullable(p);
      s+=u_get(&u,s);
      if(!xmlc_white_space(u)) break;
    }
    for(;;) {
      if(p==rx_st->notAllowed) return 0;
      if(xmlc_white_space(u)) { u=' ';
	p=drv(rx_st, p,u);
	if(p==rx_st->notAllowed) {
	  for(;;) {
	    if(s==end) return 1;
	    s+=u_get(&u,s);
	    if(!xmlc_white_space(u)) return 0;
	  }
	} else goto SKIP_SPACE;
      }
      p=drv(rx_st, p,u);
      if(s==end) goto SKIP_SPACE;
      s+=u_get(&u,s);
    }
  } else return 0;
}

