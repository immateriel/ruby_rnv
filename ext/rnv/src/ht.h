/* $Id: ht.h,v 1.5 2004/01/02 00:24:54 dvd Exp $ */

#ifndef HT_H
#define HT_H 1

struct hashtable {
  int (*hash)(void *user, int i);
  int (*equal)(void *user, int i1,int i2);
  int tablen,used,limit;
  int *table;
  void *user;
};

extern void ht_init(struct hashtable *ht,int len,int (*hash)(void *, int),int (*equal)(void *,int,int));
extern void ht_clear(struct hashtable *ht);
extern void ht_dispose(struct hashtable *ht);
extern int ht_get(struct hashtable *ht,int i);
extern void ht_put(struct hashtable *ht,int i);
extern int ht_del(struct hashtable *ht,int i);
extern int ht_deli(struct hashtable *ht,int i); /* delete only if i refers to itself */

#endif
