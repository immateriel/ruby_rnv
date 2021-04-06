/* $Id: rn.h,v 1.35 2004/02/25 00:00:32 dvd Exp $ */

#ifndef RN_H
#define RN_H 1

#include <assert.h>
#include "type.h"

/* Patterns */
#define RN_P_ERROR 0
#define RN_P_NOT_ALLOWED 1
#define RN_P_EMPTY 2
#define RN_P_TEXT 3
#define RN_P_CHOICE 4
#define RN_P_INTERLEAVE 5
#define RN_P_GROUP 6
#define RN_P_ONE_OR_MORE 7
#define RN_P_LIST 8
#define RN_P_DATA 9
#define RN_P_DATA_EXCEPT 10
#define RN_P_VALUE 11
#define RN_P_ATTRIBUTE 12
#define RN_P_ELEMENT 13
#define RN_P_REF 14
#define RN_P_AFTER 15

/*
Patterns and nameclasses are stored in arrays of integers.
an integer is either an index in the same or another array,
or a value that denotes record type etc.

Each record has a macro that accesses its fields by assigning
them to variables in the local scope, and a creator.
*/

/* Pattern Bindings */
#define RN_P_TYP(i) (rnv->rn_pattern[i]&0xFF)
#define RN_P_IS(i,x)  (x==RN_P_TYP(i))
#define RN_P_CHK(i,x)  assert(RN_P_IS(i,x))

#define RN_P_FLG_NUL 0x00000100
#define RN_P_FLG_TXT 0x00000200
#define RN_P_FLG_CTE 0x00000400
#define RN_P_FLG_CTC 0x00000800
#define RN_P_FLG_CTS 0x00001000
#define RN_P_FLG_ERS 0x40000000
#define RN_P_FLG_MRK 0x80000000

#define rn_marked(i) (rnv->rn_pattern[i]&RN_P_FLG_MRK)
#define rn_mark(i) (rnv->rn_pattern[i]|=RN_P_FLG_MRK)
#define rn_unmark(i) (rnv->rn_pattern[i]&=~RN_P_FLG_MRK)

#define rn_nullable(i) (rnv->rn_pattern[i]&RN_P_FLG_NUL)
#define rn_setNullable(i,x) if(x) rnv->rn_pattern[i]|=RN_P_FLG_NUL

#define rn_cdata(i) rnv->rn_pattern[i]&RN_P_FLG_TXT
#define rn_setCdata(i,x) if(x) rnv->rn_pattern[i]|=RN_P_FLG_TXT

/* assert: p1 at 1, p2 at 2 */

#define rn_NotAllowed(i) RN_P_CHK(i,RN_P_NOT_ALLOWED)
#define rn_Empty(i) RN_P_CHK(i,RN_P_EMPTY)
#define rn_Text(i) RN_P_CHK(i,RN_P_TEXT)
#define rn_Choice(i,p1,p2) RN_P_CHK(i,RN_P_CHOICE); p1=rnv->rn_pattern[i+1]; p2=rnv->rn_pattern[i+2]
#define rn_Interleave(i,p1,p2) RN_P_CHK(i,RN_P_INTERLEAVE); p1=rnv->rn_pattern[i+1]; p2=rnv->rn_pattern[i+2]
#define rn_Group(i,p1,p2) RN_P_CHK(i,RN_P_GROUP); p1=rnv->rn_pattern[i+1]; p2=rnv->rn_pattern[i+2]
#define rn_OneOrMore(i,p1) RN_P_CHK(i,RN_P_ONE_OR_MORE); p1=rnv->rn_pattern[i+1]
#define rn_List(i,p1) RN_P_CHK(i,RN_P_LIST); p1=rnv->rn_pattern[i+1]
#define rn_Data(i,dt,ps) RN_P_CHK(i,RN_P_DATA); dt=rnv->rn_pattern[i+1]; ps=rnv->rn_pattern[i+2]
#define rn_DataExcept(i,p1,p2) RN_P_CHK(i,RN_P_DATA_EXCEPT); p1=rnv->rn_pattern[i+1]; p2=rnv->rn_pattern[i+2]
#define rn_Value(i,dt,s) RN_P_CHK(i,RN_P_VALUE); dt=rnv->rn_pattern[i+1]; s=rnv->rn_pattern[i+2]
#define rn_Attribute(i,nc,p1) RN_P_CHK(i,RN_P_ATTRIBUTE);  p1=rnv->rn_pattern[i+1]; nc=rnv->rn_pattern[i+2]
#define rn_Element(i,nc,p1) RN_P_CHK(i,RN_P_ELEMENT); p1=rnv->rn_pattern[i+1]; nc=rnv->rn_pattern[i+2]
#define rn_After(i,p1,p2) RN_P_CHK(i,RN_P_AFTER); p1=rnv->rn_pattern[i+1]; p2=rnv->rn_pattern[i+2]
#define rn_Ref(i,p) RN_P_CHK(i,RN_P_REF); p=rnv->rn_pattern[i+1]

/* Name Classes */
#define RN_NC_ERROR 0
#define RN_NC_QNAME 1
#define RN_NC_NSNAME 2
#define RN_NC_ANY_NAME 3
#define RN_NC_EXCEPT 4
#define RN_NC_CHOICE 5
#define RN_NC_DATATYPE 6

/* Name Class Bindings  */
#define RN_NC_TYP(i) (rnv->rn_nameclass[i]&0xFF)
#define RN_NC_IS(i,x) (x==RN_NC_TYP(i))
#define RN_NC_CHK(i,x) assert(RN_NC_IS(i,x))

#define rn_QName(i,uri,name) RN_NC_CHK(i,RN_NC_QNAME); uri=rnv->rn_nameclass[i+1]; name=rnv->rn_nameclass[i+2]
#define rn_NsName(i,uri) RN_NC_CHK(i,RN_NC_NSNAME); uri=rnv->rn_nameclass[i+1]
#define rn_AnyName(i) RN_NC_CHK(i,RN_NC_ANY_NAME)
#define rn_NameClassExcept(i,nc1,nc2) RN_NC_CHK(i,RN_NC_EXCEPT); nc1=rnv->rn_nameclass[i+1]; nc2=rnv->rn_nameclass[i+2]
#define rn_NameClassChoice(i,nc1,nc2) RN_NC_CHK(i,RN_NC_CHOICE); nc1=rnv->rn_nameclass[i+1]; nc2=rnv->rn_nameclass[i+2]
#define rn_Datatype(i,lib,typ) RN_NC_CHK(i,RN_NC_DATATYPE); lib=rnv->rn_nameclass[i+1]; typ=rnv->rn_nameclass[i+2]

extern void rn_new_schema(rn_st_t *rn_st);

extern int rn_contentType(rnv_t *rnv, int i);
extern void rn_setContentType(rnv_t *rnv, int i,int t1,int t2);
extern int rn_groupable(rnv_t *rnv, int p1,int p2);

extern void rn_del_p(rn_st_t *rn_st, int i);
extern void rn_add_p(rn_st_t *rn_st, int i);

extern int rn_newString(rnv_t *rnv, rn_st_t *rn_st, char *s);

extern int rn_newNotAllowed(rnv_t *rnv, rn_st_t *rn_st);
extern int rn_newEmpty(rnv_t *rnv, rn_st_t *rn_st);
extern int rn_newText(rnv_t *rnv, rn_st_t *rn_st);
extern int rn_newChoice(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);
extern int rn_newInterleave(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);
extern int rn_newGroup(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);
extern int rn_newOneOrMore(rnv_t *rnv, rn_st_t *rn_st, int p1);
extern int rn_newList(rnv_t *rnv, rn_st_t *rn_st, int p1);
extern int rn_newData(rnv_t *rnv, rn_st_t *rn_st, int dt,int ps);
extern int rn_newDataExcept(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);
extern int rn_newValue(rnv_t *rnv, rn_st_t *rn_st, int dt,int s);
extern int rn_newAttribute(rnv_t *rnv, rn_st_t *rn_st, int nc,int p1);
extern int rn_newElement(rnv_t *rnv, rn_st_t *rn_st, int nc,int p1);
extern int rn_newAfter(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);
extern int rn_newRef(rnv_t *rnv, rn_st_t *rn_st);

extern int rn_one_or_more(rnv_t *rnv, rn_st_t *rn_st, int p);
extern int rn_group(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);
extern int rn_choice(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);
extern int rn_ileave(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);
extern int rn_after(rnv_t *rnv, rn_st_t *rn_st, int p1,int p2);

extern int rn_newAnyName(rnv_t *rnv, rn_st_t *rn_st);
extern int rn_newAnyNameExcept(int nc);
extern int rn_newQName(rnv_t *rnv, rn_st_t *rn_st, int uri,int name);
extern int rn_newNsName(rnv_t *rnv, rn_st_t *rn_st, int uri);
extern int rn_newNameClassExcept(rnv_t *rnv, rn_st_t *rn_st, int nc1,int nc2);
extern int rn_newNameClassChoice(rnv_t *rnv, rn_st_t *rn_st, int nc1,int nc2);
extern int rn_newDatatype(rnv_t *rnv, rn_st_t *rn_st, int lib,int typ);

extern int rn_i_ps(rn_st_t *rn_st);
extern void rn_add_pskey(rnv_t *rnv, rn_st_t *rn_st, char *s);
extern void rn_add_psval(rnv_t *rnv, rn_st_t *rn_st, char *s);
extern void rn_end_ps(rnv_t *rnv, rn_st_t *rn_st);

extern void rn_init(rnv_t *rnv, rn_st_t *rn_st);
extern void rn_clear(rnv_t *rnv, rn_st_t *rn_st);

extern void rn_compress(rnv_t *rnv, rn_st_t *rn_st, int *starts,int n);
extern int rn_compress_last(rnv_t *rnv, rn_st_t *rn_st, int start);

#endif
