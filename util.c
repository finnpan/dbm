/*************************************************************************************************
 * The utility API of Tokyo Cabinet
 *                                                               Copyright (C) 2006-2012 FAL Labs
 * This file is part of Tokyo Cabinet.
 * Tokyo Cabinet is free software; you can redistribute it and/or modify it under the terms of
 * the GNU Lesser General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License or any later version.  Tokyo Cabinet is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * You should have received a copy of the GNU Lesser General Public License along with Tokyo
 * Cabinet; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA.
 *************************************************************************************************/


#include "util.h"



/*************************************************************************************************
 * basic utilities
 *************************************************************************************************/


/* Allocate a region on memory. */
void *tcmalloc(size_t size){
  assert(size > 0 && size < INT_MAX);
  char *p;
  TCMALLOC(p, size);
  return p;
}


/* Allocate a nullified region on memory. */
void *tccalloc(size_t nmemb, size_t size){
  assert(nmemb > 0 && nmemb < INT_MAX && size > 0 && size < INT_MAX);
  char *p;
  TCCALLOC(p, nmemb, size);
  return p;
}


/* Re-allocate a region on memory. */
void *tcrealloc(void *ptr, size_t size){
  assert(size >= 0 && size < INT_MAX);
  char *p;
  TCREALLOC(p, ptr, size);
  return p;
}


/* Duplicate a region on memory. */
void *tcmemdup(const void *ptr, size_t size){
  assert(ptr && size >= 0);
  char *p;
  TCMALLOC(p, size + 1);
  memcpy(p, ptr, size);
  p[size] = '\0';
  return p;
}


/* Duplicate a string on memory. */
char *tcstrdup(const void *str){
  assert(str);
  int size = strlen(str);
  char *p;
  TCMALLOC(p, size + 1);
  memcpy(p, str, size);
  p[size] = '\0';
  return p;
}



/*************************************************************************************************
 * extensible string
 *************************************************************************************************/


#define TCXSTRUNIT     12                // allocation unit size of an extensible string


/* private function prototypes */
static void tcvxstrprintf(TCXSTR *xstr, const char *format, va_list ap);


/* Create an extensible string object. */
TCXSTR *tcxstrnew(void){
  return tcxstrnew2(TCXSTRUNIT);
}


/* Create an extensible string object with the initial allocation size. */
TCXSTR *tcxstrnew2(int asiz){
  assert(asiz >= 0);
  asiz = tclmax(asiz, TCXSTRUNIT);
  TCXSTR *xstr;
  TCMALLOC(xstr, sizeof(*xstr));
  TCMALLOC(xstr->ptr, asiz);
  xstr->size = 0;
  xstr->asize = asiz;
  xstr->ptr[0] = '\0';
  return xstr;
}


/* Delete an extensible string object. */
void tcxstrdel(TCXSTR *xstr){
  assert(xstr);
  free(xstr->ptr);
  free(xstr);
}


/* Concatenate a region to the end of an extensible string object. */
void tcxstrcat(TCXSTR *xstr, const void *ptr, int size){
  assert(xstr && ptr && size >= 0);
  int nsize = xstr->size + size + 1;
  if(xstr->asize < nsize){
    while(xstr->asize < nsize){
      xstr->asize *= 2;
      if(xstr->asize < nsize) xstr->asize = nsize;
    }
    TCREALLOC(xstr->ptr, xstr->ptr, xstr->asize);
  }
  memcpy(xstr->ptr + xstr->size, ptr, size);
  xstr->size += size;
  xstr->ptr[xstr->size] = '\0';
}


/* Concatenate a character string to the end of an extensible string object. */
void tcxstrcat2(TCXSTR *xstr, const char *str){
  assert(xstr && str);
  int size = strlen(str);
  tcxstrcat(xstr, str, size);
}


/* Get the pointer of the region of an extensible string object. */
const void *tcxstrptr(const TCXSTR *xstr){
  assert(xstr);
  return xstr->ptr;
}


/* Get the size of the region of an extensible string object. */
int tcxstrsize(const TCXSTR *xstr){
  assert(xstr);
  return xstr->size;
}


/* Clear an extensible string object. */
void tcxstrclear(TCXSTR *xstr){
  assert(xstr);
  xstr->size = 0;
  xstr->ptr[0] = '\0';
}


/* Perform formatted output into an extensible string object. */
void tcxstrprintf(TCXSTR *xstr, const char *format, ...){
  assert(xstr && format);
  va_list ap;
  va_start(ap, format);
  tcvxstrprintf(xstr, format, ap);
  va_end(ap);
}


/* Allocate a formatted string on memory. */
char *tcsprintf(const char *format, ...){
  assert(format);
  TCXSTR *xstr = tcxstrnew();
  va_list ap;
  va_start(ap, format);
  tcvxstrprintf(xstr, format, ap);
  va_end(ap);
  return tcxstrtomalloc(xstr);
}


/* Convert an extensible string object into a usual allocated region. */
void *tcxstrtomalloc(TCXSTR *xstr){
  assert(xstr);
  char *ptr;
  ptr = xstr->ptr;
  free(xstr);
  return ptr;
}


/* Perform formatted output into an extensible string object. */
static void tcvxstrprintf(TCXSTR *xstr, const char *format, va_list ap){
  assert(xstr && format);
  while(*format != '\0'){
    if(*format == '%'){
      char cbuf[TCNUMBUFSIZ];
      cbuf[0] = '%';
      int cblen = 1;
      int lnum = 0;
      format++;
      while(strchr("0123456789 .+-hlLz", *format) && *format != '\0' &&
            cblen < TCNUMBUFSIZ - 1){
        if(*format == 'l' || *format == 'L') lnum++;
        cbuf[cblen++] = *(format++);
      }
      cbuf[cblen++] = *format;
      cbuf[cblen] = '\0';
      int tlen;
      char *tmp, tbuf[TCNUMBUFSIZ*4];
      switch(*format){
        case 's':
          tmp = va_arg(ap, char *);
          if(!tmp) tmp = "(null)";
          tcxstrcat2(xstr, tmp);
          break;
        case 'd':
          if(lnum >= 2){
            tlen = sprintf(tbuf, cbuf, va_arg(ap, long long));
          } else if(lnum >= 1){
            tlen = sprintf(tbuf, cbuf, va_arg(ap, long));
          } else {
            tlen = sprintf(tbuf, cbuf, va_arg(ap, int));
          }
          tcxstrcat(xstr, tbuf, tlen);
          break;
        case 'o': case 'u': case 'x': case 'X': case 'c':
          if(lnum >= 2){
            tlen = sprintf(tbuf, cbuf, va_arg(ap, unsigned long long));
          } else if(lnum >= 1){
            tlen = sprintf(tbuf, cbuf, va_arg(ap, unsigned long));
          } else {
            tlen = sprintf(tbuf, cbuf, va_arg(ap, unsigned int));
          }
          tcxstrcat(xstr, tbuf, tlen);
          break;
        case 'e': case 'E': case 'f': case 'g': case 'G':
          if(lnum >= 1){
            tlen = snprintf(tbuf, sizeof(tbuf), cbuf, va_arg(ap, long double));
          } else {
            tlen = snprintf(tbuf, sizeof(tbuf), cbuf, va_arg(ap, double));
          }
          if(tlen < 0 || tlen > sizeof(tbuf)){
            tbuf[sizeof(tbuf)-1] = '*';
            tlen = sizeof(tbuf);
          }
          tcxstrcat(xstr, tbuf, tlen);
          break;
        case '@':
          tmp = va_arg(ap, char *);
          if(!tmp) tmp = "(null)";
          while(*tmp){
            switch(*tmp){
              case '&': tcxstrcat(xstr, "&amp;", 5); break;
              case '<': tcxstrcat(xstr, "&lt;", 4); break;
              case '>': tcxstrcat(xstr, "&gt;", 4); break;
              case '"': tcxstrcat(xstr, "&quot;", 6); break;
              default:
                if(!((*tmp >= 0 && *tmp <= 0x8) || (*tmp >= 0x0e && *tmp <= 0x1f)))
                  tcxstrcat(xstr, tmp, 1);
                break;
            }
            tmp++;
          }
          break;
        case '?':
          tmp = va_arg(ap, char *);
          if(!tmp) tmp = "(null)";
          while(*tmp){
            unsigned char c = *(unsigned char *)tmp;
            if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || (c != '\0' && strchr("_-.", c))){
              tcxstrcat(xstr, tmp, 1);
            } else {
              tlen = sprintf(tbuf, "%%%02X", c);
              tcxstrcat(xstr, tbuf, tlen);
            }
            tmp++;
          }
          break;
        case 'b':
          if(lnum >= 2){
            tlen = tcnumtostrbin(va_arg(ap, unsigned long long), tbuf,
                                 tcatoi(cbuf + 1), (cbuf[1] == '0') ? '0' : ' ');
          } else if(lnum >= 1){
            tlen = tcnumtostrbin(va_arg(ap, unsigned long), tbuf,
                                 tcatoi(cbuf + 1), (cbuf[1] == '0') ? '0' : ' ');
          } else {
            tlen = tcnumtostrbin(va_arg(ap, unsigned int), tbuf,
                                 tcatoi(cbuf + 1), (cbuf[1] == '0') ? '0' : ' ');
          }
          tcxstrcat(xstr, tbuf, tlen);
          break;
        case '%':
          tcxstrcat(xstr, "%", 1);
          break;
      }
    } else {
      tcxstrcat(xstr, format, 1);
    }
    format++;
  }
}



/*************************************************************************************************
 * array list
 *************************************************************************************************/


#define TCLISTUNIT     64                // allocation unit number of a list handle


/* private function prototypes */
static int tclistelemcmp(const void *a, const void *b);


/* Create a list object. */
TCLIST *tclistnew(void){
  return tclistnew2(TCLISTUNIT);
}


/* Create a list object. */
TCLIST *tclistnew2(int anum){
  TCLIST *list;
  TCMALLOC(list, sizeof(*list));
  if(anum < 1) anum = 1;
  list->anum = anum;
  TCMALLOC(list->array, sizeof(list->array[0]) * list->anum);
  list->start = 0;
  list->num = 0;
  return list;
}


/* Delete a list object. */
void tclistdel(TCLIST *list){
  assert(list);
  TCLISTDATUM *array = list->array;
  int end = list->start + list->num;
  for(int i = list->start; i < end; i++){
    free(array[i].ptr);
  }
  free(list->array);
  free(list);
}


/* Get the number of elements of a list object. */
int tclistnum(const TCLIST *list){
  assert(list);
  return list->num;
}


/* Get the pointer to the region of an element of a list object. */
const void *tclistval(const TCLIST *list, int index, int *sp){
  assert(list && index >= 0 && sp);
  if(index >= list->num) return NULL;
  index += list->start;
  *sp = list->array[index].size;
  return list->array[index].ptr;
}


/* Get the string of an element of a list object. */
const char *tclistval2(const TCLIST *list, int index){
  assert(list && index >= 0);
  if(index >= list->num) return NULL;
  index += list->start;
  return list->array[index].ptr;
}


/* Add an element at the end of a list object. */
void tclistpush(TCLIST *list, const void *ptr, int size){
  assert(list && ptr && size >= 0);
  int index = list->start + list->num;
  if(index >= list->anum){
    list->anum += list->num + 1;
    TCREALLOC(list->array, list->array, list->anum * sizeof(list->array[0]));
  }
  TCLISTDATUM *array = list->array;
  TCMALLOC(array[index].ptr, tclmax(size + 1, TCXSTRUNIT));
  memcpy(array[index].ptr, ptr, size);
  array[index].ptr[size] = '\0';
  array[index].size = size;
  list->num++;
}


/* Add a string element at the end of a list object. */
void tclistpush2(TCLIST *list, const char *str){
  int size = strlen(str);
  tclistpush(list, str, size);
}


/* Remove a string element of the top of a list object. */
char *tclistshift(TCLIST *list){
  assert(list);
  if(list->num < 1) return NULL;
  int index = list->start;
  list->start++;
  list->num--;
  void *rv = list->array[index].ptr;
  if((list->start & 0xff) == 0 && list->start > (list->num >> 1)){
    memmove(list->array, list->array + list->start, list->num * sizeof(list->array[0]));
    list->start = 0;
  }
  return rv;
}


/* Overwrite an element at the specified location of a list object. */
void tclistover(TCLIST *list, int index, const void *ptr, int size){
  assert(list && index >= 0 && ptr && size >= 0);
  if(index >= list->num) return;
  index += list->start;
  if(size > list->array[index].size)
    TCREALLOC(list->array[index].ptr, list->array[index].ptr, size + 1);
  memcpy(list->array[index].ptr, ptr, size);
  list->array[index].size = size;
  list->array[index].ptr[size] = '\0';
}


/* Sort elements of a list object in lexical order. */
void tclistsort(TCLIST *list){
  assert(list);
  qsort(list->array + list->start, list->num, sizeof(list->array[0]), tclistelemcmp);
}


/* Clear a list object. */
void tclistclear(TCLIST *list){
  assert(list);
  TCLISTDATUM *array = list->array;
  int end = list->start + list->num;
  for(int i = list->start; i < end; i++){
    free(array[i].ptr);
  }
  list->start = 0;
  list->num = 0;
}


/* Add an allocated element at the end of a list object. */
void tclistpushmalloc(TCLIST *list, void *ptr, int size){
  assert(list && ptr && size >= 0);
  int index = list->start + list->num;
  if(index >= list->anum){
    list->anum += list->num + 1;
    TCREALLOC(list->array, list->array, list->anum * sizeof(list->array[0]));
  }
  TCLISTDATUM *array = list->array;
  TCREALLOC(array[index].ptr, ptr, size + 1);
  array[index].ptr[size] = '\0';
  array[index].size = size;
  list->num++;
}


/* Perform formatted output into a list object. */
void tclistprintf(TCLIST *list, const char *format, ...){
  assert(list && format);
  TCXSTR *xstr = tcxstrnew();
  va_list ap;
  va_start(ap, format);
  tcvxstrprintf(xstr, format, ap);
  va_end(ap);
  int size = tcxstrsize(xstr);
  char *ptr = tcxstrtomalloc(xstr);
  tclistpushmalloc(list, ptr, size);
}


/* Compare two list elements in lexical order.
   `a' specifies the pointer to one element.
   `b' specifies the pointer to the other element.
   The return value is positive if the former is big, negative if the latter is big, 0 if both
   are equivalent. */
static int tclistelemcmp(const void *a, const void *b){
  assert(a && b);
  unsigned char *ao = (unsigned char *)((TCLISTDATUM *)a)->ptr;
  unsigned char *bo = (unsigned char *)((TCLISTDATUM *)b)->ptr;
  int size = (((TCLISTDATUM *)a)->size < ((TCLISTDATUM *)b)->size) ?
    ((TCLISTDATUM *)a)->size : ((TCLISTDATUM *)b)->size;
  for(int i = 0; i < size; i++){
    if(ao[i] > bo[i]) return 1;
    if(ao[i] < bo[i]) return -1;
  }
  return ((TCLISTDATUM *)a)->size - ((TCLISTDATUM *)b)->size;
}



/*************************************************************************************************
 * hash map
 *************************************************************************************************/


#define TCMAPKMAXSIZ   0xfffff           // maximum size of each key
#define TCMAPDEFBNUM   4093              // default bucket number
#define TCMAPZMMINSIZ  131072            // minimum memory size to use nullified region
#define TCMAPCSUNIT    52                // small allocation unit size of map concatenation
#define TCMAPCBUNIT    252               // big allocation unit size of map concatenation
#define TCMAPTINYBNUM  31                // bucket number of a tiny map

/* get the first hash value */
#define TCMAPHASH1(TC_res, TC_kbuf, TC_ksiz)                            \
  do {                                                                  \
    const unsigned char *_TC_p = (const unsigned char *)(TC_kbuf);      \
    int _TC_ksiz = TC_ksiz;                                             \
    for((TC_res) = 19780211; _TC_ksiz--;){                              \
      (TC_res) = (TC_res) * 37 + *(_TC_p)++;                            \
    }                                                                   \
  } while(false)

/* get the second hash value */
#define TCMAPHASH2(TC_res, TC_kbuf, TC_ksiz)                            \
  do {                                                                  \
    const unsigned char *_TC_p = (const unsigned char *)(TC_kbuf) + TC_ksiz - 1; \
    int _TC_ksiz = TC_ksiz;                                             \
    for((TC_res) = 0x13579bdf; _TC_ksiz--;){                            \
      (TC_res) = (TC_res) * 31 + *(_TC_p)--;                            \
    }                                                                   \
  } while(false)

/* compare two keys */
#define TCKEYCMP(TC_abuf, TC_asiz, TC_bbuf, TC_bsiz)                    \
  ((TC_asiz > TC_bsiz) ? 1 : (TC_asiz < TC_bsiz) ? -1 : memcmp(TC_abuf, TC_bbuf, TC_asiz))


/* Create a map object. */
TCMAP *tcmapnew(void){
  return tcmapnew2(TCMAPDEFBNUM);
}


/* Create a map object with specifying the number of the buckets. */
TCMAP *tcmapnew2(uint32_t bnum){
  if(bnum < 1) bnum = 1;
  TCMAP *map;
  TCMALLOC(map, sizeof(*map));
  TCMAPREC **buckets;
  if(bnum >= TCMAPZMMINSIZ / sizeof(*buckets)){
    buckets = tczeromap(bnum * sizeof(*buckets));
  } else {
    TCCALLOC(buckets, bnum, sizeof(*buckets));
  }
  map->buckets = buckets;
  map->first = NULL;
  map->last = NULL;
  map->cur = NULL;
  map->bnum = bnum;
  map->rnum = 0;
  map->msiz = 0;
  return map;
}


/* Close a map object. */
void tcmapdel(TCMAP *map){
  assert(map);
  TCMAPREC *rec = map->first;
  while(rec){
    TCMAPREC *next = rec->next;
    free(rec);
    rec = next;
  }
  if(map->bnum >= TCMAPZMMINSIZ / sizeof(map->buckets[0])){
    tczerounmap(map->buckets);
  } else {
    free(map->buckets);
  }
  free(map);
}


/* Store a record into a map object. */
void tcmapput(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(map && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        map->msiz += vsiz - rec->vsiz;
        int psiz = TCALIGNPAD(ksiz);
        if(vsiz > rec->vsiz){
          TCMAPREC *old = rec;
          TCREALLOC(rec, rec, sizeof(*rec) + ksiz + psiz + vsiz + 1);
          if(rec != old){
            if(map->first == old) map->first = rec;
            if(map->last == old) map->last = rec;
            if(map->cur == old) map->cur = rec;
            *entp = rec;
            if(rec->prev) rec->prev->next = rec;
            if(rec->next) rec->next->prev = rec;
            dbuf = (char *)rec + sizeof(*rec);
          }
        }
        memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
        dbuf[ksiz+psiz+vsiz] = '\0';
        rec->vsiz = vsiz;
        return;
      }
    }
  }
  int psiz = TCALIGNPAD(ksiz);
  map->msiz += ksiz + vsiz;
  TCMALLOC(rec, sizeof(*rec) + ksiz + psiz + vsiz + 1);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
}


/* Store a string record into a map object. */
void tcmapput2(TCMAP *map, const char *kstr, const char *vstr){
  assert(map && kstr && vstr);
  tcmapput(map, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Store a new record into a map object. */
bool tcmapputkeep(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(map && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        return false;
      }
    }
  }
  int psiz = TCALIGNPAD(ksiz);
  map->msiz += ksiz + vsiz;
  TCMALLOC(rec, sizeof(*rec) + ksiz + psiz + vsiz + 1);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
  return true;
}


/* Store a new string record into a map object. */
bool tcmapputkeep2(TCMAP *map, const char *kstr, const char *vstr){
  assert(map && kstr && vstr);
  return tcmapputkeep(map, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Concatenate a value at the end of the value of the existing record in a map object. */
void tcmapputcat(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(map && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        map->msiz += vsiz;
        int psiz = TCALIGNPAD(ksiz);
        int asiz = sizeof(*rec) + ksiz + psiz + rec->vsiz + vsiz + 1;
        int unit = (asiz <= TCMAPCSUNIT) ? TCMAPCSUNIT : TCMAPCBUNIT;
        asiz = (asiz - 1) + unit - (asiz - 1) % unit;
        TCMAPREC *old = rec;
        TCREALLOC(rec, rec, asiz);
        if(rec != old){
          if(map->first == old) map->first = rec;
          if(map->last == old) map->last = rec;
          if(map->cur == old) map->cur = rec;
          *entp = rec;
          if(rec->prev) rec->prev->next = rec;
          if(rec->next) rec->next->prev = rec;
          dbuf = (char *)rec + sizeof(*rec);
        }
        memcpy(dbuf + ksiz + psiz + rec->vsiz, vbuf, vsiz);
        rec->vsiz += vsiz;
        dbuf[ksiz+psiz+rec->vsiz] = '\0';
        return;
      }
    }
  }
  int psiz = TCALIGNPAD(ksiz);
  int asiz = sizeof(*rec) + ksiz + psiz + vsiz + 1;
  int unit = (asiz <= TCMAPCSUNIT) ? TCMAPCSUNIT : TCMAPCBUNIT;
  asiz = (asiz - 1) + unit - (asiz - 1) % unit;
  map->msiz += ksiz + vsiz;
  TCMALLOC(rec, asiz);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
}


/* Remove a record of a map object. */
bool tcmapout(TCMAP *map, const void *kbuf, int ksiz){
  assert(map && kbuf && ksiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        map->rnum--;
        map->msiz -= rksiz + rec->vsiz;
        if(rec->prev) rec->prev->next = rec->next;
        if(rec->next) rec->next->prev = rec->prev;
        if(rec == map->first) map->first = rec->next;
        if(rec == map->last) map->last = rec->prev;
        if(rec == map->cur) map->cur = rec->next;
        if(rec->left && !rec->right){
          *entp = rec->left;
        } else if(!rec->left && rec->right){
          *entp = rec->right;
        } else if(!rec->left){
          *entp = NULL;
        } else {
          *entp = rec->left;
          TCMAPREC *tmp = *entp;
          while(tmp->right){
            tmp = tmp->right;
          }
          tmp->right = rec->right;
        }
        free(rec);
        return true;
      }
    }
  }
  return false;
}


/* Remove a string record of a map object. */
bool tcmapout2(TCMAP *map, const char *kstr){
  assert(map && kstr);
  return tcmapout(map, kstr, strlen(kstr));
}


/* Retrieve a record in a map object. */
const void *tcmapget(const TCMAP *map, const void *kbuf, int ksiz, int *sp){
  assert(map && kbuf && ksiz >= 0 && sp);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  TCMAPREC *rec = map->buckets[hash%map->bnum];
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      rec = rec->left;
    } else if(hash < rhash){
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        rec = rec->left;
      } else if(kcmp > 0){
        rec = rec->right;
      } else {
        *sp = rec->vsiz;
        return dbuf + rksiz + TCALIGNPAD(rksiz);
      }
    }
  }
  return NULL;
}


/* Retrieve a string record in a map object. */
const char *tcmapget2(const TCMAP *map, const char *kstr){
  assert(map && kstr);
  int ksiz = strlen(kstr);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kstr, ksiz);
  TCMAPREC *rec = map->buckets[hash%map->bnum];
  TCMAPHASH2(hash, kstr, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      rec = rec->left;
    } else if(hash < rhash){
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kstr, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        rec = rec->left;
      } else if(kcmp > 0){
        rec = rec->right;
      } else {
        return dbuf + rksiz + TCALIGNPAD(rksiz);
      }
    }
  }
  return NULL;
}


/* Move a record to the edge of a map object. */
bool tcmapmove(TCMAP *map, const void *kbuf, int ksiz, bool head){
  assert(map && kbuf && ksiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  TCMAPREC *rec = map->buckets[hash%map->bnum];
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      rec = rec->left;
    } else if(hash < rhash){
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        rec = rec->left;
      } else if(kcmp > 0){
        rec = rec->right;
      } else {
        if(head){
          if(map->first == rec) return true;
          if(map->last == rec) map->last = rec->prev;
          if(rec->prev) rec->prev->next = rec->next;
          if(rec->next) rec->next->prev = rec->prev;
          rec->prev = NULL;
          rec->next = map->first;
          map->first->prev = rec;
          map->first = rec;
        } else {
          if(map->last == rec) return true;
          if(map->first == rec) map->first = rec->next;
          if(rec->prev) rec->prev->next = rec->next;
          if(rec->next) rec->next->prev = rec->prev;
          rec->prev = map->last;
          rec->next = NULL;
          map->last->next = rec;
          map->last = rec;
        }
        return true;
      }
    }
  }
  return false;
}


/* Initialize the iterator of a map object. */
void tcmapiterinit(TCMAP *map){
  assert(map);
  map->cur = map->first;
}


/* Get the next key of the iterator of a map object. */
const void *tcmapiternext(TCMAP *map, int *sp){
  assert(map && sp);
  TCMAPREC *rec;
  if(!map->cur) return NULL;
  rec = map->cur;
  map->cur = rec->next;
  *sp = rec->ksiz & TCMAPKMAXSIZ;
  return (char *)rec + sizeof(*rec);
}


/* Get the next key string of the iterator of a map object. */
const char *tcmapiternext2(TCMAP *map){
  assert(map);
  TCMAPREC *rec;
  if(!map->cur) return NULL;
  rec = map->cur;
  map->cur = rec->next;
  return (char *)rec + sizeof(*rec);
}


/* Get the number of records stored in a map object. */
uint64_t tcmaprnum(const TCMAP *map){
  assert(map);
  return map->rnum;
}


/* Get the total size of memory used in a map object. */
uint64_t tcmapmsiz(const TCMAP *map){
  assert(map);
  return map->msiz + map->rnum * (sizeof(*map->first) + sizeof(tcgeneric_t)) +
    map->bnum * sizeof(void *);
}


/* Create a list object containing all keys in a map object. */
TCLIST *tcmapkeys(const TCMAP *map){
  assert(map);
  TCLIST *list = tclistnew2(map->rnum);
  TCMAPREC *rec = map->first;
  while(rec){
    char *dbuf = (char *)rec + sizeof(*rec);
    tclistpush(list, dbuf, rec->ksiz & TCMAPKMAXSIZ);
    rec = rec->next;
  }
  return list;
}


/* Create a list object containing all values in a map object. */
TCLIST *tcmapvals(const TCMAP *map){
  assert(map);
  TCLIST *list = tclistnew2(map->rnum);
  TCMAPREC *rec = map->first;
  while(rec){
    char *dbuf = (char *)rec + sizeof(*rec);
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    tclistpush(list, dbuf + rksiz + TCALIGNPAD(rksiz), rec->vsiz);
    rec = rec->next;
  }
  return list;
}


/* Add an integer to a record in a map object. */
int tcmapaddint(TCMAP *map, const void *kbuf, int ksiz, int num){
  assert(map && kbuf && ksiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        if(rec->vsiz != sizeof(num)) return INT_MIN;
        int *resp = (int *)(dbuf + ksiz + TCALIGNPAD(ksiz));
        return *resp += num;
      }
    }
  }
  int psiz = TCALIGNPAD(ksiz);
  TCMALLOC(rec, sizeof(*rec) + ksiz + psiz + sizeof(num) + 1);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  memcpy(dbuf + ksiz + psiz, &num, sizeof(num));
  dbuf[ksiz+psiz+sizeof(num)] = '\0';
  rec->vsiz = sizeof(num);
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
  return num;
}


/* Add a real number to a record in a map object. */
double tcmapadddouble(TCMAP *map, const void *kbuf, int ksiz, double num){
  assert(map && kbuf && ksiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        if(rec->vsiz != sizeof(num)) return nan("");
        double *resp = (double *)(dbuf + ksiz + TCALIGNPAD(ksiz));
        return *resp += num;
      }
    }
  }
  int psiz = TCALIGNPAD(ksiz);
  TCMALLOC(rec, sizeof(*rec) + ksiz + psiz + sizeof(num) + 1);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  memcpy(dbuf + ksiz + psiz, &num, sizeof(num));
  dbuf[ksiz+psiz+sizeof(num)] = '\0';
  rec->vsiz = sizeof(num);
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
  return num;
}


/* Clear a map object. */
void tcmapclear(TCMAP *map){
  assert(map);
  TCMAPREC *rec = map->first;
  while(rec){
    TCMAPREC *next = rec->next;
    free(rec);
    rec = next;
  }
  TCMAPREC **buckets = map->buckets;
  int bnum = map->bnum;
  for(int i = 0; i < bnum; i++){
    buckets[i] = NULL;
  }
  map->first = NULL;
  map->last = NULL;
  map->cur = NULL;
  map->rnum = 0;
  map->msiz = 0;
}


/* Remove front records of a map object. */
void tcmapcutfront(TCMAP *map, int num){
  assert(map && num >= 0);
  tcmapiterinit(map);
  while(num-- > 0){
    int ksiz;
    const char *kbuf = tcmapiternext(map, &ksiz);
    if(!kbuf) break;
    tcmapout(map, kbuf, ksiz);
  }
}


/*************************************************************************************************
 * hash map (for experts)
 *************************************************************************************************/


/* Store a record and make it semivolatile in a map object. */
void tcmapput3(TCMAP *map, const void *kbuf, int ksiz, const char *vbuf, int vsiz){
  assert(map && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        map->msiz += vsiz - rec->vsiz;
        int psiz = TCALIGNPAD(ksiz);
        if(vsiz > rec->vsiz){
          TCMAPREC *old = rec;
          TCREALLOC(rec, rec, sizeof(*rec) + ksiz + psiz + vsiz + 1);
          if(rec != old){
            if(map->first == old) map->first = rec;
            if(map->last == old) map->last = rec;
            if(map->cur == old) map->cur = rec;
            *entp = rec;
            if(rec->prev) rec->prev->next = rec;
            if(rec->next) rec->next->prev = rec;
            dbuf = (char *)rec + sizeof(*rec);
          }
        }
        memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
        dbuf[ksiz+psiz+vsiz] = '\0';
        rec->vsiz = vsiz;
        if(map->last != rec){
          if(map->first == rec) map->first = rec->next;
          if(rec->prev) rec->prev->next = rec->next;
          if(rec->next) rec->next->prev = rec->prev;
          rec->prev = map->last;
          rec->next = NULL;
          map->last->next = rec;
          map->last = rec;
        }
        return;
      }
    }
  }
  int psiz = TCALIGNPAD(ksiz);
  map->msiz += ksiz + vsiz;
  TCMALLOC(rec, sizeof(*rec) + ksiz + psiz + vsiz + 1);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
}


/* Store a record of the value of two regions into a map object. */
void tcmapput4(TCMAP *map, const void *kbuf, int ksiz,
               const void *fvbuf, int fvsiz, const void *lvbuf, int lvsiz){
  assert(map && kbuf && ksiz >= 0 && fvbuf && fvsiz >= 0 && lvbuf && lvsiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        int vsiz = fvsiz + lvsiz;
        map->msiz += vsiz - rec->vsiz;
        int psiz = TCALIGNPAD(ksiz);
        ksiz += psiz;
        if(vsiz > rec->vsiz){
          TCMAPREC *old = rec;
          TCREALLOC(rec, rec, sizeof(*rec) + ksiz + vsiz + 1);
          if(rec != old){
            if(map->first == old) map->first = rec;
            if(map->last == old) map->last = rec;
            if(map->cur == old) map->cur = rec;
            *entp = rec;
            if(rec->prev) rec->prev->next = rec;
            if(rec->next) rec->next->prev = rec;
            dbuf = (char *)rec + sizeof(*rec);
          }
        }
        memcpy(dbuf + ksiz, fvbuf, fvsiz);
        memcpy(dbuf + ksiz + fvsiz, lvbuf, lvsiz);
        dbuf[ksiz+vsiz] = '\0';
        rec->vsiz = vsiz;
        return;
      }
    }
  }
  int vsiz = fvsiz + lvsiz;
  int psiz = TCALIGNPAD(ksiz);
  map->msiz += ksiz + vsiz;
  TCMALLOC(rec, sizeof(*rec) + ksiz + psiz + vsiz + 1);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  ksiz += psiz;
  memcpy(dbuf + ksiz, fvbuf, fvsiz);
  memcpy(dbuf + ksiz + fvsiz, lvbuf, lvsiz);
  dbuf[ksiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
}


/* Concatenate a value at the existing record and make it semivolatile in a map object. */
void tcmapputcat3(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(map && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        map->msiz += vsiz;
        int psiz = TCALIGNPAD(ksiz);
        int asiz = sizeof(*rec) + ksiz + psiz + rec->vsiz + vsiz + 1;
        int unit = (asiz <= TCMAPCSUNIT) ? TCMAPCSUNIT : TCMAPCBUNIT;
        asiz = (asiz - 1) + unit - (asiz - 1) % unit;
        TCMAPREC *old = rec;
        TCREALLOC(rec, rec, asiz);
        if(rec != old){
          if(map->first == old) map->first = rec;
          if(map->last == old) map->last = rec;
          if(map->cur == old) map->cur = rec;
          *entp = rec;
          if(rec->prev) rec->prev->next = rec;
          if(rec->next) rec->next->prev = rec;
          dbuf = (char *)rec + sizeof(*rec);
        }
        memcpy(dbuf + ksiz + psiz + rec->vsiz, vbuf, vsiz);
        rec->vsiz += vsiz;
        dbuf[ksiz+psiz+rec->vsiz] = '\0';
        if(map->last != rec){
          if(map->first == rec) map->first = rec->next;
          if(rec->prev) rec->prev->next = rec->next;
          if(rec->next) rec->next->prev = rec->prev;
          rec->prev = map->last;
          rec->next = NULL;
          map->last->next = rec;
          map->last = rec;
        }
        return;
      }
    }
  }
  int psiz = TCALIGNPAD(ksiz);
  int asiz = sizeof(*rec) + ksiz + psiz + vsiz + 1;
  int unit = (asiz <= TCMAPCSUNIT) ? TCMAPCSUNIT : TCMAPCBUNIT;
  asiz = (asiz - 1) + unit - (asiz - 1) % unit;
  map->msiz += ksiz + vsiz;
  TCMALLOC(rec, asiz);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
}


/* Store a record into a map object with a duplication handler. */
bool tcmapputproc(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz,
                  TCPDPROC proc, void *op){
  assert(map && kbuf && ksiz >= 0 && proc);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  int bidx = hash % map->bnum;
  TCMAPREC *rec = map->buckets[bidx];
  TCMAPREC **entp = map->buckets + bidx;
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      entp = &(rec->left);
      rec = rec->left;
    } else if(hash < rhash){
      entp = &(rec->right);
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        entp = &(rec->left);
        rec = rec->left;
      } else if(kcmp > 0){
        entp = &(rec->right);
        rec = rec->right;
      } else {
        int psiz = TCALIGNPAD(ksiz);
        int nvsiz;
        char *nvbuf = proc(dbuf + ksiz + psiz, rec->vsiz, &nvsiz, op);
        if(nvbuf == (void *)-1){
          map->rnum--;
          map->msiz -= rksiz + rec->vsiz;
          if(rec->prev) rec->prev->next = rec->next;
          if(rec->next) rec->next->prev = rec->prev;
          if(rec == map->first) map->first = rec->next;
          if(rec == map->last) map->last = rec->prev;
          if(rec == map->cur) map->cur = rec->next;
          if(rec->left && !rec->right){
            *entp = rec->left;
          } else if(!rec->left && rec->right){
            *entp = rec->right;
          } else if(!rec->left && !rec->left){
            *entp = NULL;
          } else {
            *entp = rec->left;
            TCMAPREC *tmp = *entp;
            while(tmp->right){
              tmp = tmp->right;
            }
            tmp->right = rec->right;
          }
          free(rec);
          return true;
        }
        if(!nvbuf) return false;
        map->msiz += nvsiz - rec->vsiz;
        if(nvsiz > rec->vsiz){
          TCMAPREC *old = rec;
          TCREALLOC(rec, rec, sizeof(*rec) + ksiz + psiz + nvsiz + 1);
          if(rec != old){
            if(map->first == old) map->first = rec;
            if(map->last == old) map->last = rec;
            if(map->cur == old) map->cur = rec;
            *entp = rec;
            if(rec->prev) rec->prev->next = rec;
            if(rec->next) rec->next->prev = rec;
            dbuf = (char *)rec + sizeof(*rec);
          }
        }
        memcpy(dbuf + ksiz + psiz, nvbuf, nvsiz);
        dbuf[ksiz+psiz+nvsiz] = '\0';
        rec->vsiz = nvsiz;
        free(nvbuf);
        return true;
      }
    }
  }
  if(!vbuf) return false;
  int psiz = TCALIGNPAD(ksiz);
  int asiz = sizeof(*rec) + ksiz + psiz + vsiz + 1;
  int unit = (asiz <= TCMAPCSUNIT) ? TCMAPCSUNIT : TCMAPCBUNIT;
  asiz = (asiz - 1) + unit - (asiz - 1) % unit;
  map->msiz += ksiz + vsiz;
  TCMALLOC(rec, asiz);
  char *dbuf = (char *)rec + sizeof(*rec);
  memcpy(dbuf, kbuf, ksiz);
  dbuf[ksiz] = '\0';
  rec->ksiz = ksiz | hash;
  memcpy(dbuf + ksiz + psiz, vbuf, vsiz);
  dbuf[ksiz+psiz+vsiz] = '\0';
  rec->vsiz = vsiz;
  rec->left = NULL;
  rec->right = NULL;
  rec->prev = map->last;
  rec->next = NULL;
  *entp = rec;
  if(!map->first) map->first = rec;
  if(map->last) map->last->next = rec;
  map->last = rec;
  map->rnum++;
  return true;
}


/* Retrieve a semivolatile record in a map object. */
const void *tcmapget3(TCMAP *map, const void *kbuf, int ksiz, int *sp){
  assert(map && kbuf && ksiz >= 0 && sp);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  TCMAPREC *rec = map->buckets[hash%map->bnum];
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      rec = rec->left;
    } else if(hash < rhash){
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        rec = rec->left;
      } else if(kcmp > 0){
        rec = rec->right;
      } else {
        if(map->last != rec){
          if(map->first == rec) map->first = rec->next;
          if(rec->prev) rec->prev->next = rec->next;
          if(rec->next) rec->next->prev = rec->prev;
          rec->prev = map->last;
          rec->next = NULL;
          map->last->next = rec;
          map->last = rec;
        }
        *sp = rec->vsiz;
        return dbuf + rksiz + TCALIGNPAD(rksiz);
      }
    }
  }
  return NULL;
}


/* Initialize the iterator of a map object at the record corresponding a key. */
void tcmapiterinit2(TCMAP *map, const void *kbuf, int ksiz){
  assert(map && kbuf && ksiz >= 0);
  if(ksiz > TCMAPKMAXSIZ) ksiz = TCMAPKMAXSIZ;
  uint32_t hash;
  TCMAPHASH1(hash, kbuf, ksiz);
  TCMAPREC *rec = map->buckets[hash%map->bnum];
  TCMAPHASH2(hash, kbuf, ksiz);
  hash &= ~TCMAPKMAXSIZ;
  while(rec){
    uint32_t rhash = rec->ksiz & ~TCMAPKMAXSIZ;
    uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
    if(hash > rhash){
      rec = rec->left;
    } else if(hash < rhash){
      rec = rec->right;
    } else {
      char *dbuf = (char *)rec + sizeof(*rec);
      int kcmp = TCKEYCMP(kbuf, ksiz, dbuf, rksiz);
      if(kcmp < 0){
        rec = rec->left;
      } else if(kcmp > 0){
        rec = rec->right;
      } else {
        map->cur = rec;
        return;
      }
    }
  }
}


/* Get the value bound to the key fetched from the iterator of a map object. */
const void *tcmapiterval(const void *kbuf, int *sp){
  assert(kbuf && sp);
  TCMAPREC *rec = (TCMAPREC *)((char *)kbuf - sizeof(*rec));
  uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
  *sp = rec->vsiz;
  return (char *)kbuf + rksiz + TCALIGNPAD(rksiz);
}


/* Get the value string bound to the key fetched from the iterator of a map object. */
const char *tcmapiterval2(const char *kstr){
  assert(kstr);
  TCMAPREC *rec = (TCMAPREC *)(kstr - sizeof(*rec));
  uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
  return kstr + rksiz + TCALIGNPAD(rksiz);
}


/* Perform formatted output into a map object. */
void tcmapprintf(TCMAP *map, const char *kstr, const char *format, ...){
  assert(map && kstr && format);
  TCXSTR *xstr = tcxstrnew();
  va_list ap;
  va_start(ap, format);
  tcvxstrprintf(xstr, format, ap);
  va_end(ap);
  tcmapput(map, kstr, strlen(kstr), tcxstrptr(xstr), tcxstrsize(xstr));
  tcxstrdel(xstr);
}



/*************************************************************************************************
 * on-memory hash database
 *************************************************************************************************/


#define TCMDBMNUM      8                 // number of internal maps
#define TCMDBDEFBNUM   65536             // default bucket number

/* get the first hash value */
#define TCMDBHASH(TC_res, TC_kbuf, TC_ksiz)                             \
  do {                                                                  \
    const unsigned char *_TC_p = (const unsigned char *)(TC_kbuf) + TC_ksiz - 1; \
    int _TC_ksiz = TC_ksiz;                                             \
    for((TC_res) = 0x20071123; _TC_ksiz--;){                            \
      (TC_res) = (TC_res) * 33 + *(_TC_p)--;                            \
    }                                                                   \
    (TC_res) &= TCMDBMNUM - 1;                                          \
  } while(false)


const char *tcmdbpath(TCMDB *mdb){
  assert(mdb);
  const char *rv = "*";
  return rv;
}


TCLIST *tcmdbmisc(TCMDB *mdb, const char *name, const TCLIST *args){
  assert(mdb && name && args);
  int argc = tclistnum(args);
  TCLIST *rv;
  if(!strcmp(name, "put") || !strcmp(name, "putkeep") || !strcmp(name, "putcat")){
    if(argc > 1){
      rv = tclistnew2(1);
      const char *kbuf;
      int ksiz;
      TCLISTVAL(kbuf, args, 0, ksiz);
      const char *vbuf;
      int vsiz;
      TCLISTVAL(vbuf, args, 1, vsiz);
      bool err = false;
      if(!strcmp(name, "put")){
        tcmdbput(mdb, kbuf, ksiz, vbuf, vsiz);
      } else if(!strcmp(name, "putkeep")){
        if(!tcmdbputkeep(mdb, kbuf, ksiz, vbuf, vsiz)) err = true;
      } else if(!strcmp(name, "putcat")){
        tcmdbputcat(mdb, kbuf, ksiz, vbuf, vsiz);
      }
      if(err){
        tclistdel(rv);
        rv = NULL;
      }
    } else {
      rv = NULL;
    }
  } else if(!strcmp(name, "out")){
    if(argc > 0){
      rv = tclistnew2(1);
      const char *kbuf;
      int ksiz;
      TCLISTVAL(kbuf, args, 0, ksiz);
      if(!tcmdbout(mdb, kbuf, ksiz)){
        tclistdel(rv);
        rv = NULL;
      }
    } else {
      rv = NULL;
    }
  } else if(!strcmp(name, "get")){
    if(argc > 0){
      rv = tclistnew2(1);
      const char *kbuf;
      int ksiz;
      TCLISTVAL(kbuf, args, 0, ksiz);
      int vsiz;
      char *vbuf = tcmdbget(mdb, kbuf, ksiz, &vsiz);
      if(vbuf){
        tclistpush(rv, vbuf, vsiz);
        free(vbuf);
      } else {
        tclistdel(rv);
        rv = NULL;
      }
    } else {
      rv = NULL;
    }
  } else if(!strcmp(name, "putlist")){
    rv = tclistnew2(1);
    argc--;
    for(int i = 0; i < argc; i += 2){
      const char *kbuf, *vbuf;
      int ksiz, vsiz;
      TCLISTVAL(kbuf, args, i, ksiz);
      TCLISTVAL(vbuf, args, i + 1, vsiz);
      tcmdbput(mdb, kbuf, ksiz, vbuf, vsiz);
    }
  } else if(!strcmp(name, "outlist")){
    rv = tclistnew2(1);
    for(int i = 0; i < argc; i++){
      const char *kbuf;
      int ksiz;
      TCLISTVAL(kbuf, args, i, ksiz);
      tcmdbout(mdb, kbuf, ksiz);
    }
  } else if(!strcmp(name, "getlist")){
    rv = tclistnew2(argc * 2);
    for(int i = 0; i < argc; i++){
      const char *kbuf;
      int ksiz;
      TCLISTVAL(kbuf, args, i, ksiz);
      int vsiz;
      char *vbuf = tcmdbget(mdb, kbuf, ksiz, &vsiz);
      if(vbuf){
        tclistpush(rv, kbuf, ksiz);
        tclistpush(rv, vbuf, vsiz);
        free(vbuf);
      }
    }
  } else if(!strcmp(name, "getpart")){
    if(argc > 0){
      const char *kbuf;
      int ksiz;
      TCLISTVAL(kbuf, args, 0, ksiz);
      int off = argc > 1 ? tcatoi(TCLISTVALPTR(args, 1)) : 0;
      if(off < 0) off = 0;
      if(off > INT_MAX / 2 - 1) off = INT_MAX - 1;
      int len = argc > 2 ? tcatoi(TCLISTVALPTR(args, 2)) : -1;
      if(len < 0 || len > INT_MAX / 2) len = INT_MAX / 2;
      int vsiz;
      char *vbuf = tcmdbget(mdb, kbuf, ksiz, &vsiz);
      if(vbuf){
        if(off < vsiz){
          rv = tclistnew2(1);
          vsiz -= off;
          if(vsiz > len) vsiz = len;
          if(off > 0) memmove(vbuf, vbuf + off, vsiz);
          tclistpushmalloc(rv, vbuf, vsiz);
        } else {
          rv = NULL;
          free(vbuf);
        }
      } else {
        rv = NULL;
      }
    } else {
      rv = NULL;
    }
  } else if(!strcmp(name, "iterinit")){
    rv = tclistnew2(1);
    if(argc > 0){
      const char *kbuf;
      int ksiz;
      TCLISTVAL(kbuf, args, 0, ksiz);
      tcmdbiterinit2(mdb, kbuf, ksiz);
    } else {
      tcmdbiterinit(mdb);
    }
  } else if(!strcmp(name, "iternext")){
    rv = tclistnew2(1);
    int ksiz;
    char *kbuf = tcmdbiternext(mdb, &ksiz);
    if(kbuf){
      tclistpush(rv, kbuf, ksiz);
      int vsiz;
      char *vbuf = tcmdbget(mdb, kbuf, ksiz, &vsiz);
      if(vbuf){
        tclistpush(rv, vbuf, vsiz);
        free(vbuf);
      }
      free(kbuf);
    } else {
      tclistdel(rv);
      rv = NULL;
    }
  } else if(!strcmp(name, "vanish")){
    rv = tclistnew2(1);
    tcmdbvanish(mdb);
  } else if(!strcmp(name, "regex")){
    if(argc > 0){
      const char *regex = TCLISTVALPTR(args, 0);
      int options = REG_EXTENDED | REG_NOSUB;
      if(*regex == '*'){
        options |= REG_ICASE;
        regex++;
      }
      regex_t rbuf;
      if(regcomp(&rbuf, regex, options) == 0){
        rv = tclistnew();
        int max = argc > 1 ? tcatoi(TCLISTVALPTR(args, 1)) : 0;
        if(max < 1) max = INT_MAX;
        tcmdbiterinit(mdb);
        char *kbuf;
        int ksiz;
        while(max > 0 && (kbuf = tcmdbiternext(mdb, &ksiz))){
          if(regexec(&rbuf, kbuf, 0, NULL, 0) == 0){
            int vsiz;
            char *vbuf = tcmdbget(mdb, kbuf, ksiz, &vsiz);
            if(vbuf){
              tclistpush(rv, kbuf, ksiz);
              tclistpush(rv, vbuf, vsiz);
              free(vbuf);
              max--;
            }
          }
          free(kbuf);
        }
        regfree(&rbuf);
      } else {
        rv = NULL;
      }
    } else {
      rv = NULL;
    }
  } else {
    rv = NULL;
  }
  return rv;
}


/* Create an on-memory hash database object. */
TCMDB *tcmdbnew(void){
  return tcmdbnew2(TCMDBDEFBNUM);
}


/* Create an on-memory hash database with specifying the number of the buckets. */
TCMDB *tcmdbnew2(uint32_t bnum){
  TCMDB *mdb;
  if(bnum < 1) bnum = TCMDBDEFBNUM;
  bnum = bnum / TCMDBMNUM + 17;
  TCMALLOC(mdb, sizeof(*mdb));
  TCMALLOC(mdb->mmtxs, sizeof(pthread_rwlock_t) * TCMDBMNUM);
  TCMALLOC(mdb->imtx, sizeof(pthread_mutex_t));
  TCMALLOC(mdb->maps, sizeof(TCMAP *) * TCMDBMNUM);
  if(pthread_mutex_init(mdb->imtx, NULL) != 0) tcmyfatal("mutex error");
  for(int i = 0; i < TCMDBMNUM; i++){
    if(pthread_rwlock_init((pthread_rwlock_t *)mdb->mmtxs + i, NULL) != 0)
      tcmyfatal("rwlock error");
    mdb->maps[i] = tcmapnew2(bnum);
  }
  mdb->iter = -1;
  return mdb;
}


/* Delete an on-memory hash database object. */
void tcmdbdel(TCMDB *mdb){
  assert(mdb);
  for(int i = TCMDBMNUM - 1; i >= 0; i--){
    tcmapdel(mdb->maps[i]);
    pthread_rwlock_destroy((pthread_rwlock_t *)mdb->mmtxs + i);
  }
  pthread_mutex_destroy(mdb->imtx);
  free(mdb->maps);
  free(mdb->imtx);
  free(mdb->mmtxs);
  free(mdb);
}


/* Store a record into an on-memory hash database. */
void tcmdbput(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(mdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return;
  tcmapput(mdb->maps[mi], kbuf, ksiz, vbuf, vsiz);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
}

/* Store a new record into an on-memory hash database. */
bool tcmdbputkeep(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(mdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return false;
  bool rv = tcmapputkeep(mdb->maps[mi], kbuf, ksiz, vbuf, vsiz);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  return rv;
}


/* Concatenate a value at the end of the existing record in an on-memory hash database. */
void tcmdbputcat(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(mdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return;
  tcmapputcat(mdb->maps[mi], kbuf, ksiz, vbuf, vsiz);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
}


/* Remove a record of an on-memory hash database. */
bool tcmdbout(TCMDB *mdb, const void *kbuf, int ksiz){
  assert(mdb && kbuf && ksiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return false;
  bool rv = tcmapout(mdb->maps[mi], kbuf, ksiz);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  return rv;
}


/* Retrieve a record in an on-memory hash database. */
void *tcmdbget(TCMDB *mdb, const void *kbuf, int ksiz, int *sp){
  assert(mdb && kbuf && ksiz >= 0 && sp);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_rdlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return NULL;
  int vsiz;
  const char *vbuf = tcmapget(mdb->maps[mi], kbuf, ksiz, &vsiz);
  char *rv;
  if(vbuf){
    rv = tcmemdup(vbuf, vsiz);
    *sp = vsiz;
  } else {
    rv = NULL;
  }
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  return rv;
}


/* Get the size of the value of a record in an on-memory hash database object. */
int tcmdbvsiz(TCMDB *mdb, const void *kbuf, int ksiz){
  assert(mdb && kbuf && ksiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_rdlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return -1;
  int vsiz;
  const char *vbuf = tcmapget(mdb->maps[mi], kbuf, ksiz, &vsiz);
  if(!vbuf) vsiz = -1;
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  return vsiz;
}


/* Initialize the iterator of an on-memory hash database. */
void tcmdbiterinit(TCMDB *mdb){
  assert(mdb);
  if(pthread_mutex_lock(mdb->imtx) != 0) return;
  for(int i = 0; i < TCMDBMNUM; i++){
    tcmapiterinit(mdb->maps[i]);
  }
  mdb->iter = 0;
  pthread_mutex_unlock(mdb->imtx);
}


/* Get the next key of the iterator of an on-memory hash database. */
void *tcmdbiternext(TCMDB *mdb, int *sp){
  assert(mdb && sp);
  if(pthread_mutex_lock(mdb->imtx) != 0) return NULL;
  if(mdb->iter < 0 || mdb->iter >= TCMDBMNUM){
    pthread_mutex_unlock(mdb->imtx);
    return NULL;
  }
  int mi = mdb->iter;
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0){
    pthread_mutex_unlock(mdb->imtx);
    return NULL;
  }
  int ksiz;
  const char *kbuf;
  while(!(kbuf = tcmapiternext(mdb->maps[mi], &ksiz)) && mi < TCMDBMNUM - 1){
    pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
    mi = ++mdb->iter;
    if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0){
      pthread_mutex_unlock(mdb->imtx);
      return NULL;
    }
  }
  char *rv;
  if(kbuf){
    rv = tcmemdup(kbuf, ksiz);
    *sp = ksiz;
  } else {
    rv = NULL;
  }
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  pthread_mutex_unlock(mdb->imtx);
  return rv;
}


/* Get forward matching keys in an on-memory hash database object. */
TCLIST *tcmdbfwmkeys(TCMDB *mdb, const void *pbuf, int psiz, int max){
  assert(mdb && pbuf && psiz >= 0);
  TCLIST* keys = tclistnew();
  if(pthread_mutex_lock(mdb->imtx) != 0) return keys;
  if(max < 0) max = INT_MAX;
  for(int i = 0; i < TCMDBMNUM && tclistnum(keys) < max; i++){
    if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + i) == 0){
      TCMAP *map = mdb->maps[i];
      TCMAPREC *cur = map->cur;
      tcmapiterinit(map);
      const char *kbuf;
      int ksiz;
      while(tclistnum(keys) < max && (kbuf = tcmapiternext(map, &ksiz)) != NULL){
        if(ksiz >= psiz && !memcmp(kbuf, pbuf, psiz)) tclistpush(keys, kbuf, ksiz);
      }
      map->cur = cur;
      pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + i);
    }
  }
  pthread_mutex_unlock(mdb->imtx);
  return keys;
}


/* Get the number of records stored in an on-memory hash database. */
uint64_t tcmdbrnum(TCMDB *mdb){
  assert(mdb);
  uint64_t rnum = 0;
  for(int i = 0; i < TCMDBMNUM; i++){
    rnum += tcmaprnum(mdb->maps[i]);
  }
  return rnum;
}


/* Get the total size of memory used in an on-memory hash database object. */
uint64_t tcmdbmsiz(TCMDB *mdb){
  assert(mdb);
  uint64_t msiz = 0;
  for(int i = 0; i < TCMDBMNUM; i++){
    msiz += tcmapmsiz(mdb->maps[i]);
  }
  return msiz;
}


/* Add an integer to a record in an on-memory hash database object. */
int tcmdbaddint(TCMDB *mdb, const void *kbuf, int ksiz, int num){
  assert(mdb && kbuf && ksiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return INT_MIN;
  int rv = tcmapaddint(mdb->maps[mi], kbuf, ksiz, num);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  return rv;
}


/* Add a real number to a record in an on-memory hash database object. */
double tcmdbadddouble(TCMDB *mdb, const void *kbuf, int ksiz, double num){
  assert(mdb && kbuf && ksiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return nan("");
  double rv = tcmapadddouble(mdb->maps[mi], kbuf, ksiz, num);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  return rv;
}


/* Clear an on-memory hash database object. */
void tcmdbvanish(TCMDB *mdb){
  assert(mdb);
  for(int i = 0; i < TCMDBMNUM; i++){
    if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + i) == 0){
      tcmapclear(mdb->maps[i]);
      pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + i);
    }
  }
}


/* Remove front records of a map object. */
void tcmdbcutfront(TCMDB *mdb, int num){
  assert(mdb && num >= 0);
  num = num / TCMDBMNUM + 1;
  for(int i = 0; i < TCMDBMNUM; i++){
    if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + i) == 0){
      tcmapcutfront(mdb->maps[i], num);
      pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + i);
    }
  }
}



/*************************************************************************************************
 * on-memory hash database (for experts)
 *************************************************************************************************/


/* Store a record and make it semivolatile in an on-memory hash database object. */
void tcmdbput3(TCMDB *mdb, const void *kbuf, int ksiz, const char *vbuf, int vsiz){
  assert(mdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return;
  tcmapput3(mdb->maps[mi], kbuf, ksiz, vbuf, vsiz);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
}


/* Store a record of the value of two regions into an on-memory hash database object. */
void tcmdbput4(TCMDB *mdb, const void *kbuf, int ksiz,
               const void *fvbuf, int fvsiz, const void *lvbuf, int lvsiz){
  assert(mdb && kbuf && ksiz >= 0 && fvbuf && fvsiz >= 0 && lvbuf && lvsiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return;
  tcmapput4(mdb->maps[mi], kbuf, ksiz, fvbuf, fvsiz, lvbuf, lvsiz);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
}


/* Concatenate a value and make it semivolatile in on-memory hash database object. */
void tcmdbputcat3(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(mdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return;
  tcmapputcat3(mdb->maps[mi], kbuf, ksiz, vbuf, vsiz);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
}


/* Store a record into a on-memory hash database object with a duplication handler. */
bool tcmdbputproc(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz,
                  TCPDPROC proc, void *op){
  assert(mdb && kbuf && ksiz >= 0 && proc);
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0) return false;
  bool rv = tcmapputproc(mdb->maps[mi], kbuf, ksiz, vbuf, vsiz, proc, op);
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  return rv;
}


/* Initialize the iterator of an on-memory map database object in front of a key. */
void tcmdbiterinit2(TCMDB *mdb, const void *kbuf, int ksiz){
  if(pthread_mutex_lock(mdb->imtx) != 0) return;
  unsigned int mi;
  TCMDBHASH(mi, kbuf, ksiz);
  if(pthread_rwlock_rdlock((pthread_rwlock_t *)mdb->mmtxs + mi) != 0){
    pthread_mutex_unlock(mdb->imtx);
    return;
  }
  int vsiz;
  if(tcmapget(mdb->maps[mi], kbuf, ksiz, &vsiz)){
    for(int i = 0; i < TCMDBMNUM; i++){
      tcmapiterinit(mdb->maps[i]);
    }
    tcmapiterinit2(mdb->maps[mi], kbuf, ksiz);
    mdb->iter = mi;
  }
  pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + mi);
  pthread_mutex_unlock(mdb->imtx);
}


/* Process each record atomically of an on-memory hash database object. */
void tcmdbforeach(TCMDB *mdb, TCITER iter, void *op){
  assert(mdb && iter);
  for(int i = 0; i < TCMDBMNUM; i++){
    if(pthread_rwlock_wrlock((pthread_rwlock_t *)mdb->mmtxs + i) != 0){
      while(i >= 0){
        pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + i);
        i--;
      }
      return;
    }
  }
  bool cont = true;
  for(int i = 0; cont && i < TCMDBMNUM; i++){
    TCMAP *map = mdb->maps[i];
    TCMAPREC *cur = map->cur;
    tcmapiterinit(map);
    int ksiz;
    const char *kbuf;
    while(cont && (kbuf = tcmapiternext(map, &ksiz)) != NULL){
      int vsiz;
      const char *vbuf = tcmapiterval(kbuf, &vsiz);
      if(!iter(kbuf, ksiz, vbuf, vsiz, op)) cont = false;
    }
    map->cur = cur;
  }
  for(int i = TCMDBMNUM - 1; i >= 0; i--){
    pthread_rwlock_unlock((pthread_rwlock_t *)mdb->mmtxs + i);
  }
}



/*************************************************************************************************
 * memory pool
 *************************************************************************************************/


#define TCMPOOLUNIT    128               // allocation unit size of memory pool elements


/* Global memory pool object. */
TCMPOOL *tcglobalmemorypool = NULL;


/* private function prototypes */
static void tcmpooldelglobal(void);


/* Create a memory pool object. */
TCMPOOL *tcmpoolnew(void){
  TCMPOOL *mpool;
  TCMALLOC(mpool, sizeof(*mpool));
  TCMALLOC(mpool->mutex, sizeof(pthread_mutex_t));
  if(pthread_mutex_init(mpool->mutex, NULL) != 0) tcmyfatal("locking failed");
  mpool->anum = TCMPOOLUNIT;
  TCMALLOC(mpool->elems, sizeof(mpool->elems[0]) * mpool->anum);
  mpool->num = 0;
  return mpool;
}


/* Delete a memory pool object. */
void tcmpooldel(TCMPOOL *mpool){
  assert(mpool);
  TCMPELEM *elems = mpool->elems;
  for(int i = mpool->num - 1; i >= 0; i--){
    elems[i].del(elems[i].ptr);
  }
  free(elems);
  pthread_mutex_destroy(mpool->mutex);
  free(mpool->mutex);
  free(mpool);
}


/* Relegate an arbitrary object to a memory pool object. */
void *tcmpoolpush(TCMPOOL *mpool, void *ptr, void (*del)(void *)){
  assert(mpool && del);
  if(!ptr) return NULL;
  if(pthread_mutex_lock(mpool->mutex) != 0) tcmyfatal("locking failed");
  int num = mpool->num;
  if(num >= mpool->anum){
    mpool->anum *= 2;
    TCREALLOC(mpool->elems, mpool->elems, mpool->anum * sizeof(mpool->elems[0]));
  }
  mpool->elems[num].ptr = ptr;
  mpool->elems[num].del = del;
  mpool->num++;
  pthread_mutex_unlock(mpool->mutex);
  return ptr;
}


/* Create a list object relegated to a memory pool object. */
TCLIST *tcmpoollistnew(TCMPOOL *mpool){
  assert(mpool);
  TCLIST *list = tclistnew();
  tcmpoolpush(mpool, list, (void (*)(void *))tclistdel);
  return list;
}


/* Get the global memory pool object. */
TCMPOOL *tcmpoolglobal(void){
  if(tcglobalmemorypool) return tcglobalmemorypool;
  tcglobalmemorypool = tcmpoolnew();
  atexit(tcmpooldelglobal);
  return tcglobalmemorypool;
}


/* Detete the global memory pool object. */
static void tcmpooldelglobal(void){
  if(tcglobalmemorypool) tcmpooldel(tcglobalmemorypool);
}



/*************************************************************************************************
 * miscellaneous utilities
 *************************************************************************************************/


#define TCRANDDEV      "/dev/urandom"    // path of the random device file
#define TCDISTMAXLEN   4096              // maximum size of a string for distance checking
#define TCDISTBUFSIZ   16384             // size of a distance buffer
#define TCLDBLCOLMAX   16                // maximum number of columns of the long double


/* File descriptor of random number generator. */
int tcrandomdevfd = -1;


/* Get the larger value of two integers. */
long tclmax(long a, long b){
  return (a > b) ? a : b;
}


/* Get the lesser value of two integers. */
long tclmin(long a, long b){
  return (a < b) ? a : b;
}


/* Compare two strings with case insensitive evaluation. */
int tcstricmp(const char *astr, const char *bstr){
  assert(astr && bstr);
  while(*astr != '\0'){
    if(*bstr == '\0') return 1;
    int ac = (*astr >= 'A' && *astr <= 'Z') ? *astr + ('a' - 'A') : *(unsigned char *)astr;
    int bc = (*bstr >= 'A' && *bstr <= 'Z') ? *bstr + ('a' - 'A') : *(unsigned char *)bstr;
    if(ac != bc) return ac - bc;
    astr++;
    bstr++;
  }
  return (*bstr == '\0') ? 0 : -1;
}


/* Check whether a string begins with a key. */
bool tcstrfwm(const char *str, const char *key){
  assert(str && key);
  while(*key != '\0'){
    if(*str != *key || *str == '\0') return false;
    key++;
    str++;
  }
  return true;
}


/* Check whether a string begins with a key with case insensitive evaluation. */
bool tcstrifwm(const char *str, const char *key){
  assert(str && key);
  while(*key != '\0'){
    if(*str == '\0') return false;
    int sc = *str;
    if(sc >= 'A' && sc <= 'Z') sc += 'a' - 'A';
    int kc = *key;
    if(kc >= 'A' && kc <= 'Z') kc += 'a' - 'A';
    if(sc != kc) return false;
    key++;
    str++;
  }
  return true;
}


/* Check whether a string ends with a key. */
bool tcstrbwm(const char *str, const char *key){
  assert(str && key);
  int slen = strlen(str);
  int klen = strlen(key);
  for(int i = 1; i <= klen; i++){
    if(i > slen || str[slen-i] != key[klen-i]) return false;
  }
  return true;
}


/* Check whether a string ends with a key with case insensitive evaluation. */
bool tcstribwm(const char *str, const char *key){
  assert(str && key);
  int slen = strlen(str);
  int klen = strlen(key);
  for(int i = 1; i <= klen; i++){
    if(i > slen) return false;
    int sc = str[slen-i];
    if(sc >= 'A' && sc <= 'Z') sc += 'a' - 'A';
    int kc = key[klen-i];
    if(kc >= 'A' && kc <= 'Z') kc += 'a' - 'A';
    if(sc != kc) return false;
  }
  return true;
}


/* Convert the letters of a string into lower case. */
char *tcstrtolower(char *str){
  assert(str);
  char *wp = str;
  while(*wp != '\0'){
    if(*wp >= 'A' && *wp <= 'Z') *wp += 'a' - 'A';
    wp++;
  }
  return str;
}


/* Cut space characters at head or tail of a string. */
char *tcstrtrim(char *str){
  assert(str);
  const char *rp = str;
  char *wp = str;
  bool head = true;
  while(*rp != '\0'){
    if(*rp > '\0' && *rp <= ' '){
      if(!head) *(wp++) = *rp;
    } else {
      *(wp++) = *rp;
      head = false;
    }
    rp++;
  }
  *wp = '\0';
  while(wp > str && wp[-1] > '\0' && wp[-1] <= ' '){
    *(--wp) = '\0';
  }
  return str;
}


/* Squeeze space characters in a string and trim it. */
char *tcstrsqzspc(char *str){
  assert(str);
  char *rp = str;
  char *wp = str;
  bool spc = true;
  while(*rp != '\0'){
    if(*rp > 0 && *rp <= ' '){
      if(!spc) *(wp++) = *rp;
      spc = true;
    } else {
      *(wp++) = *rp;
      spc = false;
    }
    rp++;
  }
  *wp = '\0';
  for(wp--; wp >= str; wp--){
    if(*wp > 0 && *wp <= ' '){
      *wp = '\0';
    } else {
      break;
    }
  }
  return str;
}


/* Convert a UTF-8 string into a UCS-2 array. */
void tcstrutftoucs(const char *str, uint16_t *ary, int *np){
  assert(str && ary && np);
  const unsigned char *rp = (unsigned char *)str;
  unsigned int wi = 0;
  while(*rp != '\0'){
    int c = *(unsigned char *)rp;
    if(c < 0x80){
      ary[wi++] = c;
    } else if(c < 0xe0){
      if(rp[1] >= 0x80){
        ary[wi++] = ((rp[0] & 0x1f) << 6) | (rp[1] & 0x3f);
        rp++;
      }
    } else if(c < 0xf0){
      if(rp[1] >= 0x80 && rp[2] >= 0x80){
        ary[wi++] = ((rp[0] & 0xf) << 12) | ((rp[1] & 0x3f) << 6) | (rp[2] & 0x3f);
        rp += 2;
      }
    }
    rp++;
  }
  *np = wi;
}


/* Convert a UCS-2 array into a UTF-8 string. */
int tcstrucstoutf(const uint16_t *ary, int num, char *str){
  assert(ary && num >= 0 && str);
  unsigned char *wp = (unsigned char *)str;
  for(int i = 0; i < num; i++){
    unsigned int c = ary[i];
    if(c < 0x80){
      *(wp++) = c;
    } else if(c < 0x800){
      *(wp++) = 0xc0 | (c >> 6);
      *(wp++) = 0x80 | (c & 0x3f);
    } else {
      *(wp++) = 0xe0 | (c >> 12);
      *(wp++) = 0x80 | ((c & 0xfff) >> 6);
      *(wp++) = 0x80 | (c & 0x3f);
    }
  }
  *wp = '\0';
  return (char *)wp - str;
}


/* Create a list object by splitting a string. */
TCLIST *tcstrsplit(const char *str, const char *delims){
  assert(str && delims);
  TCLIST *list = tclistnew();
  while(true){
    const char *sp = str;
    while(*str != '\0' && !strchr(delims, *str)){
      str++;
    }
    tclistpush(list, sp, str - sp);
    if(*str == '\0') break;
    str++;
  }
  return list;
}


/* Convert a string to an integer. */
int64_t tcatoi(const char *str){
  assert(str);
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  int sign = 1;
  int64_t num = 0;
  if(*str == '-'){
    str++;
    sign = -1;
  } else if(*str == '+'){
    str++;
  }
  while(*str != '\0'){
    if(*str < '0' || *str > '9') break;
    num = num * 10 + *str - '0';
    str++;
  }
  return num * sign;
}


/* Convert a string with a metric prefix to an integer. */
int64_t tcatoix(const char *str){
  assert(str);
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  int sign = 1;
  if(*str == '-'){
    str++;
    sign = -1;
  } else if(*str == '+'){
    str++;
  }
  long double num = 0;
  while(*str != '\0'){
    if(*str < '0' || *str > '9') break;
    num = num * 10 + *str - '0';
    str++;
  }
  if(*str == '.'){
    str++;
    long double base = 10;
    while(*str != '\0'){
      if(*str < '0' || *str > '9') break;
      num += (*str - '0') / base;
      str++;
      base *= 10;
    }
  }
  num *= sign;
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  if(*str == 'k' || *str == 'K'){
    num *= 1LL << 10;
  } else if(*str == 'm' || *str == 'M'){
    num *= 1LL << 20;
  } else if(*str == 'g' || *str == 'G'){
    num *= 1LL << 30;
  } else if(*str == 't' || *str == 'T'){
    num *= 1LL << 40;
  } else if(*str == 'p' || *str == 'P'){
    num *= 1LL << 50;
  } else if(*str == 'e' || *str == 'E'){
    num *= 1LL << 60;
  }
  if(num > INT64_MAX) return INT64_MAX;
  if(num < INT64_MIN) return INT64_MIN;
  return num;
}


/* Convert a string to a real number. */
double tcatof(const char *str){
  assert(str);
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  int sign = 1;
  if(*str == '-'){
    str++;
    sign = -1;
  } else if(*str == '+'){
    str++;
  }
  if(tcstrifwm(str, "inf")) return HUGE_VAL * sign;
  if(tcstrifwm(str, "nan")) return nan("");
  long double num = 0;
  int col = 0;
  while(*str != '\0'){
    if(*str < '0' || *str > '9') break;
    num = num * 10 + *str - '0';
    str++;
    if(num > 0) col++;
  }
  if(*str == '.'){
    str++;
    long double fract = 0.0;
    long double base = 10;
    while(col < TCLDBLCOLMAX && *str != '\0'){
      if(*str < '0' || *str > '9') break;
      fract += (*str - '0') / base;
      str++;
      col++;
      base *= 10;
    }
    num += fract;
  }
  if(*str == 'e' || *str == 'E'){
    str++;
    num *= pow(10, tcatoi(str));
  }
  return num * sign;
}


/* Check whether a string matches a regular expression. */
bool tcregexmatch(const char *str, const char *regex){
  assert(str && regex);
  int options = REG_EXTENDED | REG_NOSUB;
  if(*regex == '*'){
    options |= REG_ICASE;
    regex++;
  }
  regex_t rbuf;
  if(regcomp(&rbuf, regex, options) != 0) return false;
  bool rv = regexec(&rbuf, str, 0, NULL, 0) == 0;
  regfree(&rbuf);
  return rv;
}


/* Get the time of day in seconds. */
double tctime(void){
  struct timeval tv;
  if(gettimeofday(&tv, NULL) == -1) return 0.0;
  return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}


/* Format a date as a string in W3CDTF. */
void tcdatestrwww(int64_t t, int jl, char *buf){
  assert(buf);
  if(t == INT64_MAX) t = time(NULL);
  if(jl == INT_MAX) jl = tcjetlag();
  time_t tt = (time_t)t + jl;
  struct tm ts;
  if(!gmtime_r(&tt, &ts)) memset(&ts, 0, sizeof(ts));
  ts.tm_year += 1900;
  ts.tm_mon += 1;
  jl /= 60;
  char tzone[16];
  if(jl == 0){
    sprintf(tzone, "Z");
  } else if(jl < 0){
    jl *= -1;
    sprintf(tzone, "-%02d:%02d", jl / 60, jl % 60);
  } else {
    sprintf(tzone, "+%02d:%02d", jl / 60, jl % 60);
  }
  sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d%s",
          ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec, tzone);
}


/* Get the jet lag of the local time. */
int tcjetlag(void){
#if defined(_SYS_LINUX_)
  tzset();
  return -timezone;
#else
  time_t t = 86400;
  struct tm gts;
  if(!gmtime_r(&t, &gts)) return 0;
  struct tm lts;
  t = 86400;
  if(!localtime_r(&t, &lts)) return 0;
  return mktime(&lts) - mktime(&gts);
#endif
}


/*************************************************************************************************
 * miscellaneous utilities (for experts)
 *************************************************************************************************/


/* Convert a hexadecimal string to an integer. */
int64_t tcatoih(const char *str){
  assert(str);
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X')){
    str += 2;
  }
  int64_t num = 0;
  while(true){
    if(*str >= '0' && *str <= '9'){
      num = num * 0x10 + *str - '0';
    } else if(*str >= 'a' && *str <= 'f'){
      num = num * 0x10 + *str - 'a' + 10;
    } else if(*str >= 'A' && *str <= 'F'){
      num = num * 0x10 + *str - 'A' + 10;
    } else {
      break;
    }
    str++;
  }
  return num;
}


/* Skip space characters at head of a string. */
const char *tcstrskipspc(const char *str){
  assert(str);
  while(*str > '\0' && *str <= ' '){
    str++;
  }
  return str;
}


/* Normalize a UCS-2 array. */
int tcstrucsnorm(uint16_t *ary, int num, int opts){
  assert(ary && num >= 0);
  bool spcmode = opts & TCUNSPACE;
  bool lowmode = opts & TCUNLOWER;
  bool nacmode = opts & TCUNNOACC;
  bool widmode = opts & TCUNWIDTH;
  int wi = 0;
  for(int i = 0; i < num; i++){
    int c = ary[i];
    int high = c >> 8;
    if(high == 0x00){
      if(c <= 0x0020 || c == 0x007f){
        // control characters
        if(spcmode){
          ary[wi++] = 0x0020;
          if(wi < 2 || ary[wi-2] == 0x0020) wi--;
        } else if(c == 0x0009 || c == 0x000a || c == 0x000d){
          ary[wi++] = c;
        } else {
          ary[wi++] = 0x0020;
        }
      } else if(c == 0x00a0){
        // no-break space
        if(spcmode){
          ary[wi++] = 0x0020;
          if(wi < 2 || ary[wi-2] == 0x0020) wi--;
        } else {
          ary[wi++] = c;
        }
      } else {
        // otherwise
        if(lowmode){
          if(c < 0x007f){
            if(c >= 0x0041 && c <= 0x005a) c += 0x20;
          } else if(c >= 0x00c0 && c <= 0x00de && c != 0x00d7){
            c += 0x20;
          }
        }
        if(nacmode){
          if(c >= 0x00c0 && c <= 0x00c5){
            c = 'A';
          } else if(c == 0x00c7){
            c = 'C';
          } if(c >= 0x00c7 && c <= 0x00cb){
            c = 'E';
          } if(c >= 0x00cc && c <= 0x00cf){
            c = 'I';
          } else if(c == 0x00d0){
            c = 'D';
          } else if(c == 0x00d1){
            c = 'N';
          } if((c >= 0x00d2 && c <= 0x00d6) || c == 0x00d8){
            c = 'O';
          } if(c >= 0x00d9 && c <= 0x00dc){
            c = 'U';
          } if(c == 0x00dd || c == 0x00de){
            c = 'Y';
          } else if(c == 0x00df){
            c = 's';
          } else if(c >= 0x00e0 && c <= 0x00e5){
            c = 'a';
          } else if(c == 0x00e7){
            c = 'c';
          } if(c >= 0x00e7 && c <= 0x00eb){
            c = 'e';
          } if(c >= 0x00ec && c <= 0x00ef){
            c = 'i';
          } else if(c == 0x00f0){
            c = 'd';
          } else if(c == 0x00f1){
            c = 'n';
          } if((c >= 0x00f2 && c <= 0x00f6) || c == 0x00f8){
            c = 'o';
          } if(c >= 0x00f9 && c <= 0x00fc){
            c = 'u';
          } if(c >= 0x00fd && c <= 0x00ff){
            c = 'y';
          }
        }
        ary[wi++] = c;
      }
    } else if(high == 0x01){
      // latin-1 extended
      if(lowmode){
        if(c <= 0x0137){
          if((c & 1) == 0) c++;
        } else if(c == 0x0138){
          c += 0;
        } else if(c <= 0x0148){
          if((c & 1) == 1) c++;
        } else if(c == 0x0149){
          c += 0;
        } else if(c <= 0x0177){
          if((c & 1) == 0) c++;
        } else if(c == 0x0178){
          c = 0x00ff;
        } else if(c <= 0x017e){
          if((c & 1) == 1) c++;
        } else if(c == 0x017f){
          c += 0;
        }
      }
      if(nacmode){
        if(c == 0x00ff){
          c = 'y';
        } else if(c <= 0x0105){
          c = ((c & 1) == 0) ? 'A' : 'a';
        } else if(c <= 0x010d){
          c = ((c & 1) == 0) ? 'C' : 'c';
        } else if(c <= 0x0111){
          c = ((c & 1) == 0) ? 'D' : 'd';
        } else if(c <= 0x011b){
          c = ((c & 1) == 0) ? 'E' : 'e';
        } else if(c <= 0x0123){
          c = ((c & 1) == 0) ? 'G' : 'g';
        } else if(c <= 0x0127){
          c = ((c & 1) == 0) ? 'H' : 'h';
        } else if(c <= 0x0131){
          c = ((c & 1) == 0) ? 'I' : 'i';
        } else if(c == 0x0134){
          c = 'J';
        } else if(c == 0x0135){
          c = 'j';
        } else if(c == 0x0136){
          c = 'K';
        } else if(c == 0x0137){
          c = 'k';
        } else if(c == 0x0138){
          c = 'k';
        } else if(c >= 0x0139 && c <= 0x0142){
          c = ((c & 1) == 1) ? 'L' : 'l';
        } else if(c >= 0x0143 && c <= 0x0148){
          c = ((c & 1) == 1) ? 'N' : 'n';
        } else if(c >= 0x0149 && c <= 0x014b){
          c = ((c & 1) == 0) ? 'N' : 'n';
        } else if(c >= 0x014c && c <= 0x0151){
          c = ((c & 1) == 0) ? 'O' : 'o';
        } else if(c >= 0x0154 && c <= 0x0159){
          c = ((c & 1) == 0) ? 'R' : 'r';
        } else if(c >= 0x015a && c <= 0x0161){
          c = ((c & 1) == 0) ? 'S' : 's';
        } else if(c >= 0x0162 && c <= 0x0167){
          c = ((c & 1) == 0) ? 'T' : 't';
        } else if(c >= 0x0168 && c <= 0x0173){
          c = ((c & 1) == 0) ? 'U' : 'u';
        } else if(c == 0x0174){
          c = 'W';
        } else if(c == 0x0175){
          c = 'w';
        } else if(c == 0x0176){
          c = 'Y';
        } else if(c == 0x0177){
          c = 'y';
        } else if(c == 0x0178){
          c = 'Y';
        } else if(c >= 0x0179 && c <= 0x017e){
          c = ((c & 1) == 1) ? 'Z' : 'z';
        } else if(c == 0x017f){
          c = 's';
        }
      }
      ary[wi++] = c;
    } else if(high == 0x03){
      // greek
      if(lowmode){
        if(c >= 0x0391 && c <= 0x03a9){
          c += 0x20;
        } else if(c >= 0x03d8 && c <= 0x03ef){
          if((c & 1) == 0) c++;
        } else if(c == 0x0374 || c == 0x03f7 || c == 0x03fa){
          c++;
        }
      }
      ary[wi++] = c;
    } else if(high == 0x04){
      // cyrillic
      if(lowmode){
        if(c <= 0x040f){
          c += 0x50;
        } else if(c <= 0x042f){
          c += 0x20;
        } else if(c >= 0x0460 && c <= 0x0481){
          if((c & 1) == 0) c++;
        } else if(c >= 0x048a && c <= 0x04bf){
          if((c & 1) == 0) c++;
        } else if(c == 0x04c0){
          c = 0x04cf;
        } else if(c >= 0x04c1 && c <= 0x04ce){
          if((c & 1) == 1) c++;
        } else if(c >= 0x04d0){
          if((c & 1) == 0) c++;
        }
      }
      ary[wi++] = c;
    } else if(high == 0x20){
      if(c == 0x2002 || c == 0x2003 || c == 0x2009){
        // en space, em space, thin space
        if(spcmode){
          ary[wi++] = 0x0020;
          if(wi < 2 || ary[wi-2] == 0x0020) wi--;
        } else {
          ary[wi++] = c;
        }
      } else if(c == 0x2010){
        // hyphen
        ary[wi++] = widmode ? 0x002d : c;
      } else if(c == 0x2015){
        // fullwidth horizontal line
        ary[wi++] = widmode ? 0x002d : c;
      } else if(c == 0x2019){
        // apostrophe
        ary[wi++] = widmode ? 0x0027 : c;
      } else if(c == 0x2033){
        // double quotes
        ary[wi++] = widmode ? 0x0022 : c;
      } else {
        // (otherwise)
        ary[wi++] = c;
      }
    } else if(high == 0x22){
      if(c == 0x2212){
        // minus sign
        ary[wi++] = widmode ? 0x002d : c;
      } else {
        // (otherwise)
        ary[wi++] = c;
      }
    } else if(high == 0x30){
      if(c == 0x3000){
        // fullwidth space
        if(spcmode){
          ary[wi++] = 0x0020;
          if(wi < 2 || ary[wi-2] == 0x0020) wi--;
        } else if(widmode){
          ary[wi++] = 0x0020;
        } else {
          ary[wi++] = c;
        }
      } else {
        // (otherwise)
        ary[wi++] = c;
      }
    } else if(high == 0xff){
      if(c == 0xff01){
        // fullwidth exclamation
        ary[wi++] = widmode ? 0x0021 : c;
      } else if(c == 0xff03){
        // fullwidth igeta
        ary[wi++] = widmode ? 0x0023 : c;
      } else if(c == 0xff04){
        // fullwidth dollar
        ary[wi++] = widmode ? 0x0024 : c;
      } else if(c == 0xff05){
        // fullwidth parcent
        ary[wi++] = widmode ? 0x0025 : c;
      } else if(c == 0xff06){
        // fullwidth ampersand
        ary[wi++] = widmode ? 0x0026 : c;
      } else if(c == 0xff0a){
        // fullwidth asterisk
        ary[wi++] = widmode ? 0x002a : c;
      } else if(c == 0xff0b){
        // fullwidth plus
        ary[wi++] = widmode ? 0x002b : c;
      } else if(c == 0xff0c){
        // fullwidth comma
        ary[wi++] = widmode ? 0x002c : c;
      } else if(c == 0xff0e){
        // fullwidth period
        ary[wi++] = widmode ? 0x002e : c;
      } else if(c == 0xff0f){
        // fullwidth slash
        ary[wi++] = widmode ? 0x002f : c;
      } else if(c == 0xff1a){
        // fullwidth colon
        ary[wi++] = widmode ? 0x003a : c;
      } else if(c == 0xff1b){
        // fullwidth semicolon
        ary[wi++] = widmode ? 0x003b : c;
      } else if(c == 0xff1d){
        // fullwidth equal
        ary[wi++] = widmode ? 0x003d : c;
      } else if(c == 0xff1f){
        // fullwidth question
        ary[wi++] = widmode ? 0x003f : c;
      } else if(c == 0xff20){
        // fullwidth atmark
        ary[wi++] = widmode ? 0x0040 : c;
      } else if(c == 0xff3c){
        // fullwidth backslash
        ary[wi++] = widmode ? 0x005c : c;
      } else if(c == 0xff3e){
        // fullwidth circumflex
        ary[wi++] = widmode ? 0x005e : c;
      } else if(c == 0xff3f){
        // fullwidth underscore
        ary[wi++] = widmode ? 0x005f : c;
      } else if(c == 0xff5c){
        // fullwidth vertical line
        ary[wi++] = widmode ? 0x007c : c;
      } else if(c >= 0xff21 && c <= 0xff3a){
        // fullwidth alphabets
        if(widmode){
          if(lowmode){
            ary[wi++] = c - 0xfee0 + 0x20;
          } else {
            ary[wi++] = c - 0xfee0;
          }
        } else if(lowmode){
          ary[wi++] = c + 0x20;
        } else {
          ary[wi++] = c;
        }
      } else if(c >= 0xff41 && c <= 0xff5a){
        // fullwidth small alphabets
        ary[wi++] = widmode ? c - 0xfee0 : c;
      } else if(c >= 0xff10 && c <= 0xff19){
        // fullwidth numbers
        ary[wi++] = widmode ? c - 0xfee0 : c;
      } else if(c == 0xff61){
        // halfwidth full stop
        ary[wi++] = widmode ? 0x3002 : c;
      } else if(c == 0xff62){
        // halfwidth left corner
        ary[wi++] = widmode ? 0x300c : c;
      } else if(c == 0xff63){
        // halfwidth right corner
        ary[wi++] = widmode ? 0x300d : c;
      } else if(c == 0xff64){
        // halfwidth comma
        ary[wi++] = widmode ? 0x3001 : c;
      } else if(c == 0xff65){
        // halfwidth middle dot
        ary[wi++] = widmode ? 0x30fb : c;
      } else if(c == 0xff66){
        // halfwidth wo
        ary[wi++] = widmode ? 0x30f2 : c;
      } else if(c >= 0xff67 && c <= 0xff6b){
        // halfwidth small a-o
        ary[wi++] = widmode ? (c - 0xff67) * 2 + 0x30a1 : c;
      } else if(c >= 0xff6c && c <= 0xff6e){
        // halfwidth small ya-yo
        ary[wi++] = widmode ? (c - 0xff6c) * 2 + 0x30e3 : c;
      } else if(c == 0xff6f){
        // halfwidth small tu
        ary[wi++] = widmode ? 0x30c3 : c;
      } else if(c == 0xff70){
        // halfwidth prolonged mark
        ary[wi++] = widmode ? 0x30fc : c;
      } else if(c >= 0xff71 && c <= 0xff75){
        // halfwidth a-o
        if(widmode){
          ary[wi] = (c - 0xff71) * 2 + 0x30a2;
          if(c == 0xff73 && i < num - 1 && ary[i+1] == 0xff9e){
            ary[wi] = 0x30f4;
            i++;
          }
          wi++;
        } else {
          ary[wi++] = c;
        }
      } else if(c >= 0xff76 && c <= 0xff7a){
        // halfwidth ka-ko
        if(widmode){
          ary[wi] = (c - 0xff76) * 2 + 0x30ab;
          if(i < num - 1 && ary[i+1] == 0xff9e){
            ary[wi]++;
            i++;
          }
          wi++;
        } else {
          ary[wi++] = c;
        }
      } else if(c >= 0xff7b && c <= 0xff7f){
        // halfwidth sa-so
        if(widmode){
          ary[wi] = (c - 0xff7b) * 2 + 0x30b5;
          if(i < num - 1 && ary[i+1] == 0xff9e){
            ary[wi]++;
            i++;
          }
          wi++;
        } else {
          ary[wi++] = c;
        }
      } else if(c >= 0xff80 && c <= 0xff84){
        // halfwidth ta-to
        if(widmode){
          ary[wi] = (c - 0xff80) * 2 + 0x30bf + (c >= 0xff82 ? 1 : 0);
          if(i < num - 1 && ary[i+1] == 0xff9e){
            ary[wi]++;
            i++;
          }
          wi++;
        } else {
          ary[wi++] = c;
        }
      } else if(c >= 0xff85 && c <= 0xff89){
        // halfwidth na-no
        ary[wi++] = widmode ? c - 0xcebb : c;
      } else if(c >= 0xff8a && c <= 0xff8e){
        // halfwidth ha-ho
        if(widmode){
          ary[wi] = (c - 0xff8a) * 3 + 0x30cf;
          if(i < num - 1 && ary[i+1] == 0xff9e){
            ary[wi]++;
            i++;
          } else if(i < num - 1 && ary[i+1] == 0xff9f){
            ary[wi] += 2;
            i++;
          }
          wi++;
        } else {
          ary[wi++] = c;
        }
      } else if(c >= 0xff8f && c <= 0xff93){
        // halfwidth ma-mo
        ary[wi++] = widmode ? c - 0xceb1 : c;
      } else if(c >= 0xff94 && c <= 0xff96){
        // halfwidth ya-yo
        ary[wi++] = widmode ? (c - 0xff94) * 2 + 0x30e4 : c;
      } else if(c >= 0xff97 && c <= 0xff9b){
        // halfwidth ra-ro
        ary[wi++] = widmode ? c - 0xceae : c;
      } else if(c == 0xff9c){
        // halfwidth wa
        ary[wi++] = widmode ? 0x30ef : c;
      } else if(c == 0xff9d){
        // halfwidth nn
        ary[wi++] = widmode ? 0x30f3 : c;
      } else {
        // otherwise
        ary[wi++] = c;
      }
    } else {
      // otherwise
      ary[wi++] = c;
    }
  }
  if(spcmode){
    while(wi > 0 && ary[wi-1] == 0x0020){
      wi--;
    }
  }
  return wi;
}


/* Suspend execution of the current thread. */
bool tcsleep(double sec){
  if(!isnormal(sec) || sec <= 0.0) return false;
  if(sec <= 1.0 / sysconf(_SC_CLK_TCK)) return sched_yield() == 0;
  double integ, fract;
  fract = modf(sec, &integ);
  struct timespec req, rem;
  req.tv_sec = integ;
  req.tv_nsec = tclmin(fract * 1000.0 * 1000.0 * 1000.0, 999999999);
  while(nanosleep(&req, &rem) != 0){
    if(errno != EINTR) return false;
    req = rem;
  }
  return true;
}


/* Get the current system information. */
TCMAP *tcsysinfo(void){
#if defined(_SYS_LINUX_)
  TCMAP *info = tcmapnew2(TCMAPTINYBNUM);
  struct rusage rbuf;
  memset(&rbuf, 0, sizeof(rbuf));
  if(getrusage(RUSAGE_SELF, &rbuf) == 0){
    tcmapprintf(info, "utime", "%0.6f",
                rbuf.ru_utime.tv_sec + rbuf.ru_utime.tv_usec / 1000000.0);
    tcmapprintf(info, "stime", "%0.6f",
                rbuf.ru_stime.tv_sec + rbuf.ru_stime.tv_usec / 1000000.0);
  }
  TCLIST *lines = tcreadfilelines("/proc/self/status");
  if(lines){
    int ln = tclistnum(lines);
    for(int i = 0; i < ln; i++){
      const char *line = TCLISTVALPTR(lines, i);
      const char *rp = strchr(line, ':');
      if(!rp) continue;
      rp++;
      while(*rp > '\0' && *rp <= ' '){
        rp++;
      }
      if(tcstrifwm(line, "VmSize:")){
        int64_t size = tcatoix(rp);
        if(size > 0) tcmapprintf(info, "size", "%lld", (long long)size);
      } else if(tcstrifwm(line, "VmRSS:")){
        int64_t size = tcatoix(rp);
        if(size > 0) tcmapprintf(info, "rss", "%lld", (long long)size);
      }
    }
    tclistdel(lines);
  }
  lines = tcreadfilelines("/proc/meminfo");
  if(lines){
    int ln = tclistnum(lines);
    for(int i = 0; i < ln; i++){
      const char *line = TCLISTVALPTR(lines, i);
      const char *rp = strchr(line, ':');
      if(!rp) continue;
      rp++;
      while(*rp > '\0' && *rp <= ' '){
        rp++;
      }
      if(tcstrifwm(line, "MemTotal:")){
        int64_t size = tcatoix(rp);
        if(size > 0) tcmapprintf(info, "total", "%lld", (long long)size);
      } else if(tcstrifwm(line, "MemFree:")){
        int64_t size = tcatoix(rp);
        if(size > 0) tcmapprintf(info, "free", "%lld", (long long)size);
      } else if(tcstrifwm(line, "Cached:")){
        int64_t size = tcatoix(rp);
        if(size > 0) tcmapprintf(info, "cached", "%lld", (long long)size);
      }
    }
    tclistdel(lines);
  }
  lines = tcreadfilelines("/proc/cpuinfo");
  if(lines){
    int cnum = 0;
    int ln = tclistnum(lines);
    for(int i = 0; i < ln; i++){
      const char *line = TCLISTVALPTR(lines, i);
      if(tcstrifwm(line, "processor")) cnum++;
    }
    if(cnum > 0) tcmapprintf(info, "corenum", "%lld", (long long)cnum);
    tclistdel(lines);
  }
  return info;
#elif defined(_SYS_FREEBSD_) || defined(_SYS_MACOSX_)
  TCMAP *info = tcmapnew2(TCMAPTINYBNUM);
  struct rusage rbuf;
  memset(&rbuf, 0, sizeof(rbuf));
  if(getrusage(RUSAGE_SELF, &rbuf) == 0){
    tcmapprintf(info, "utime", "%0.6f",
                rbuf.ru_utime.tv_sec + rbuf.ru_utime.tv_usec / 1000000.0);
    tcmapprintf(info, "stime", "%0.6f",
                rbuf.ru_stime.tv_sec + rbuf.ru_stime.tv_usec / 1000000.0);
    long tck = sysconf(_SC_CLK_TCK);
    int64_t size = (((double)rbuf.ru_ixrss + rbuf.ru_idrss + rbuf.ru_isrss) / tck) * 1024.0;
    if(size > 0) tcmapprintf(info, "rss", "%lld", (long long)size);
  }
  return info;
#else
  TCMAP *info = tcmapnew2(TCMAPTINYBNUM);
  struct rusage rbuf;
  memset(&rbuf, 0, sizeof(rbuf));
  if(getrusage(RUSAGE_SELF, &rbuf) == 0){
    tcmapprintf(info, "utime", "%0.6f",
                rbuf.ru_utime.tv_sec + rbuf.ru_utime.tv_usec / 1000000.0);
    tcmapprintf(info, "stime", "%0.6f",
                rbuf.ru_stime.tv_sec + rbuf.ru_stime.tv_usec / 1000000.0);
  }
  return info;
#endif
}


/*************************************************************************************************
 * filesystem utilities
 *************************************************************************************************/


#define TCFILEMODE     00644             // permission of a creating file
#define TCIOBUFSIZ     16384             // size of an I/O buffer


/* Get the canonicalized absolute path of a file. */
char *tcrealpath(const char *path){
  assert(path);
  char buf[PATH_MAX+1];
  if(realpath(path, buf)) return tcstrdup(buf);
  if(errno == ENOENT){
    const char *pv = strrchr(path, MYPATHCHR);
    if(pv){
      if(pv == path) return tcstrdup(path);
      char *prefix = tcmemdup(path, pv - path);
      if(!realpath(prefix, buf)){
        free(prefix);
        return NULL;
      }
      free(prefix);
      pv++;
    } else {
      if(!realpath(MYCDIRSTR, buf)) return NULL;
      pv = path;
    }
    if(buf[0] == MYPATHCHR && buf[1] == '\0') buf[0] = '\0';
    char *str;
    TCMALLOC(str, strlen(buf) + strlen(pv) + 2);
    sprintf(str, "%s%c%s", buf, MYPATHCHR, pv);
    return str;
  }
  return NULL;
}


/* Get the status information of a file. */
bool tcstatfile(const char *path, bool *isdirp, int64_t *sizep, int64_t *mtimep){
  assert(path);
  struct stat sbuf;
  if(stat(path, &sbuf) != 0) return false;
  if(isdirp) *isdirp = S_ISDIR(sbuf.st_mode);
  if(sizep) *sizep = sbuf.st_size;
  if(mtimep) *mtimep = sbuf.st_mtime;
  return true;
}


/* Read whole data of a file. */
void *tcreadfile(const char *path, int limit, int *sp){
  int fd = path ? open(path, O_RDONLY, TCFILEMODE) : 0;
  if(fd == -1) return NULL;
  if(fd == 0){
    TCXSTR *xstr = tcxstrnew();
    char buf[TCIOBUFSIZ];
    limit = limit > 0 ? limit : INT_MAX;
    int rsiz;
    while((rsiz = read(fd, buf, tclmin(TCIOBUFSIZ, limit))) > 0){
      tcxstrcat(xstr, buf, rsiz);
      limit -= rsiz;
    }
    if(sp) *sp = tcxstrsize(xstr);
    return tcxstrtomalloc(xstr);
  }
  struct stat sbuf;
  if(fstat(fd, &sbuf) == -1 || !S_ISREG(sbuf.st_mode)){
    close(fd);
    return NULL;
  }
  limit = limit > 0 ? tclmin((int)sbuf.st_size, limit) : sbuf.st_size;
  char *buf;
  TCMALLOC(buf, sbuf.st_size + 1);
  char *wp = buf;
  int rsiz;
  while((rsiz = read(fd, wp, limit - (wp - buf))) > 0){
    wp += rsiz;
  }
  *wp = '\0';
  close(fd);
  if(sp) *sp = wp - buf;
  return buf;
}


/* Read every line of a file. */
TCLIST *tcreadfilelines(const char *path){
  int fd = path ? open(path, O_RDONLY, TCFILEMODE) : 0;
  if(fd == -1) return NULL;
  TCLIST *list = tclistnew();
  TCXSTR *xstr = tcxstrnew();
  char buf[TCIOBUFSIZ];
  int rsiz;
  while((rsiz = read(fd, buf, TCIOBUFSIZ)) > 0){
    for(int i = 0; i < rsiz; i++){
      switch(buf[i]){
        case '\r':
          break;
        case '\n':
          tclistpush(list, tcxstrptr(xstr), tcxstrsize(xstr));
          tcxstrclear(xstr);
          break;
        default:
          tcxstrcat(xstr, buf + i, 1);
          break;
      }
    }
  }
  tclistpush(list, tcxstrptr(xstr), tcxstrsize(xstr));
  tcxstrdel(xstr);
  if(path) close(fd);
  return list;
}


/* Write data into a file. */
bool tcwritefile(const char *path, const void *ptr, int size){
  assert(ptr && size >= 0);
  int fd = 1;
  if(path && (fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, TCFILEMODE)) == -1) return false;
  bool err = false;
  if(!tcwrite(fd, ptr, size)) err = true;
  if(close(fd) == -1) err = true;
  return !err;
}


/* Copy a file. */
bool tccopyfile(const char *src, const char *dest){
  int ifd = open(src, O_RDONLY, TCFILEMODE);
  if(ifd == -1) return false;
  int ofd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, TCFILEMODE);
  if(ofd == -1){
    close(ifd);
    return false;
  }
  bool err = false;
  while(true){
    char buf[TCIOBUFSIZ];
    int size = read(ifd, buf, TCIOBUFSIZ);
    if(size > 0){
      if(!tcwrite(ofd, buf, size)){
        err = true;
        break;
      }
    } else if(size == -1){
      if(errno != EINTR){
        err = true;
        break;
      }
    } else {
      break;
    }
  }
  if(close(ofd) == -1) err = true;
  if(close(ifd) == -1) err = true;
  return !err;
}


/* Read names of files in a directory. */
TCLIST *tcreaddir(const char *path){
  assert(path);
  DIR *DD;
  struct dirent *dp;
  if(!(DD = opendir(path))) return NULL;
  TCLIST *list = tclistnew();
  while((dp = readdir(DD)) != NULL){
    if(!strcmp(dp->d_name, MYCDIRSTR) || !strcmp(dp->d_name, MYPDIRSTR)) continue;
    tclistpush(list, dp->d_name, strlen(dp->d_name));
  }
  closedir(DD);
  return list;
}


/* Expand a pattern into a list of matched paths. */
TCLIST *tcglobpat(const char *pattern){
  assert(pattern);
  TCLIST *list = tclistnew();
  glob_t gbuf;
  memset(&gbuf, 0, sizeof(gbuf));
  if(glob(pattern, GLOB_ERR | GLOB_NOSORT, NULL, &gbuf) == 0){
    for(int i = 0; i < gbuf.gl_pathc; i++){
      tclistpush2(list, gbuf.gl_pathv[i]);
    }
    globfree(&gbuf);
  }
  return list;
}


/* Remove a file or a directory and its sub ones recursively. */
bool tcremovelink(const char *path){
  assert(path);
  struct stat sbuf;
  if(lstat(path, &sbuf) == -1) return false;
  if(unlink(path) == 0) return true;
  TCLIST *list;
  if(!S_ISDIR(sbuf.st_mode) || !(list = tcreaddir(path))) return false;
  bool tail = path[0] != '\0' && path[strlen(path)-1] == MYPATHCHR;
  for(int i = 0; i < tclistnum(list); i++){
    const char *elem = TCLISTVALPTR(list, i);
    if(!strcmp(MYCDIRSTR, elem) || !strcmp(MYPDIRSTR, elem)) continue;
    char *cpath;
    if(tail){
      cpath = tcsprintf("%s%s", path, elem);
    } else {
      cpath = tcsprintf("%s%c%s", path, MYPATHCHR, elem);
    }
    tcremovelink(cpath);
    free(cpath);
  }
  tclistdel(list);
  return rmdir(path) == 0 ? true : false;
}


/* Write data into a file. */
bool tcwrite(int fd, const void *buf, size_t size){
  assert(fd >= 0 && buf && size >= 0);
  const char *rp = buf;
  do {
    int wb = write(fd, rp, size);
    switch(wb){
      case -1: if(errno != EINTR) return false;
      case 0: break;
      default:
        rp += wb;
        size -= wb;
        break;
    }
  } while(size > 0);
  return true;
}


/* Read data from a file. */
bool tcread(int fd, void *buf, size_t size){
  assert(fd >= 0 && buf && size >= 0);
  char *wp = buf;
  do {
    int rb = read(fd, wp, size);
    switch(rb){
      case -1: if(errno != EINTR) return false;
      case 0: return size < 1;
      default:
        wp += rb;
        size -= rb;
    }
  } while(size > 0);
  return true;
}


/* Lock a file. */
bool tclock(int fd, bool ex, bool nb){
  assert(fd >= 0);
  struct flock lock;
  memset(&lock, 0, sizeof(struct flock));
  lock.l_type = ex ? F_WRLCK : F_RDLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = 0;
  while(fcntl(fd, nb ? F_SETLK : F_SETLKW, &lock) == -1){
    if(errno != EINTR) return false;
  }
  return true;
}


/* Unlock a file. */
bool tcunlock(int fd){
  assert(fd >= 0);
  struct flock lock;
  memset(&lock, 0, sizeof(struct flock));
  lock.l_type = F_UNLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;
  lock.l_pid = 0;
  while(fcntl(fd, F_SETLKW, &lock) == -1){
    if(errno != EINTR) return false;
  }
  return true;
}


/* Execute a shell command. */
int tcsystem(const char **args, int anum){
  assert(args && anum >= 0);
  if(anum < 1) return -1;
  TCXSTR *phrase = tcxstrnew2(anum * TCNUMBUFSIZ + 1);
  for(int i = 0; i < anum; i++){
    const char *rp = args[i];
    int len = strlen(rp);
    char *token;
    TCMALLOC(token, len * 2 + 1);
    char *wp = token;
    while(*rp != '\0'){
      switch(*rp){
        case '"': case '\\': case '$': case '`':
          *(wp++) = '\\';
          *(wp++) = *rp;
          break;
        default:
          *(wp++) = *rp;
          break;
      }
      rp++;
    }
    *wp = '\0';
    if(tcxstrsize(phrase)) tcxstrcat(phrase, " ", 1);
    tcxstrprintf(phrase, "\"%s\"", token);
    free(token);
  }
  int rv = system(tcxstrptr(phrase));
  if(WIFEXITED(rv)){
    rv = WEXITSTATUS(rv);
  } else {
    rv = INT_MAX;
  }
  tcxstrdel(phrase);
  return rv;
}



/*************************************************************************************************
 * encoding utilities
 *************************************************************************************************/


#define TCENCBUFSIZ    32                // size of a buffer for encoding name
#define TCXMLATBNUM    31                // bucket number of XML attributes


/* Encode a serial object with URL encoding. */
char *tcurlencode(const char *ptr, int size){
  assert(ptr && size >= 0);
  char *buf;
  TCMALLOC(buf, size * 3 + 1);
  char *wp = buf;
  for(int i = 0; i < size; i++){
    int c = ((unsigned char *)ptr)[i];
    if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
       (c >= '0' && c <= '9') || (c != '\0' && strchr("_-.!~*'()", c))){
      *(wp++) = c;
    } else {
      wp += sprintf(wp, "%%%02X", c);
    }
  }
  *wp = '\0';
  return buf;
}


/* Decode a string encoded with URL encoding. */
char *tcurldecode(const char *str, int *sp){
  assert(str && sp);
  char *buf = tcstrdup(str);
  char *wp = buf;
  while(*str != '\0'){
    if(*str == '%'){
      str++;
      if(((str[0] >= '0' && str[0] <= '9') || (str[0] >= 'A' && str[0] <= 'F') ||
          (str[0] >= 'a' && str[0] <= 'f')) &&
         ((str[1] >= '0' && str[1] <= '9') || (str[1] >= 'A' && str[1] <= 'F') ||
          (str[1] >= 'a' && str[1] <= 'f'))){
        unsigned char c = *str;
        if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if(c >= 'a' && c <= 'z'){
          *wp = c - 'a' + 10;
        } else {
          *wp = c - '0';
        }
        *wp *= 0x10;
        str++;
        c = *str;
        if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if(c >= 'a' && c <= 'z'){
          *wp += c - 'a' + 10;
        } else {
          *wp += c - '0';
        }
        str++;
        wp++;
      } else {
        break;
      }
    } else if(*str == '+'){
      *wp = ' ';
      str++;
      wp++;
    } else {
      *wp = *str;
      str++;
      wp++;
    }
  }
  *wp = '\0';
  *sp = wp - buf;
  return buf;
}


/* Break up a URL into elements. */
TCMAP *tcurlbreak(const char *str){
  assert(str);
  TCMAP *map = tcmapnew2(TCMAPTINYBNUM);
  char *trim = tcstrdup(str);
  tcstrtrim(trim);
  const char *rp = trim;
  char *norm;
  TCMALLOC(norm, strlen(trim) * 3 + 1);
  char *wp = norm;
  while(*rp != '\0'){
    if(*rp > 0x20 && *rp < 0x7f){
      *(wp++) = *rp;
    } else {
      wp += sprintf(wp, "%%%02X", *(unsigned char *)rp);
    }
    rp++;
  }
  *wp = '\0';
  rp = norm;
  tcmapput2(map, "self", rp);
  bool serv = false;
  if(tcstrifwm(rp, "http://")){
    tcmapput2(map, "scheme", "http");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "https://")){
    tcmapput2(map, "scheme", "https");
    rp += 8;
    serv = true;
  } else if(tcstrifwm(rp, "ftp://")){
    tcmapput2(map, "scheme", "ftp");
    rp += 6;
    serv = true;
  } else if(tcstrifwm(rp, "sftp://")){
    tcmapput2(map, "scheme", "sftp");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "ftps://")){
    tcmapput2(map, "scheme", "ftps");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "tftp://")){
    tcmapput2(map, "scheme", "tftp");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "ldap://")){
    tcmapput2(map, "scheme", "ldap");
    rp += 7;
    serv = true;
  } else if(tcstrifwm(rp, "ldaps://")){
    tcmapput2(map, "scheme", "ldaps");
    rp += 8;
    serv = true;
  } else if(tcstrifwm(rp, "file://")){
    tcmapput2(map, "scheme", "file");
    rp += 7;
    serv = true;
  }
  char *ep;
  if((ep = strchr(rp, '#')) != NULL){
    tcmapput2(map, "fragment", ep + 1);
    *ep = '\0';
  }
  if((ep = strchr(rp, '?')) != NULL){
    tcmapput2(map, "query", ep + 1);
    *ep = '\0';
  }
  if(serv){
    if((ep = strchr(rp, '/')) != NULL){
      tcmapput2(map, "path", ep);
      *ep = '\0';
    } else {
      tcmapput2(map, "path", "/");
    }
    if((ep = strchr(rp, '@')) != NULL){
      *ep = '\0';
      if(rp[0] != '\0') tcmapput2(map, "authority", rp);
      rp = ep + 1;
    }
    if((ep = strchr(rp, ':')) != NULL){
      if(ep[1] != '\0') tcmapput2(map, "port", ep + 1);
      *ep = '\0';
    }
    if(rp[0] != '\0') tcmapput2(map, "host", rp);
  } else {
    tcmapput2(map, "path", rp);
  }
  free(norm);
  free(trim);
  if((rp = tcmapget2(map, "path")) != NULL){
    if((ep = strrchr(rp, '/')) != NULL){
      if(ep[1] != '\0') tcmapput2(map, "file", ep + 1);
    } else {
      tcmapput2(map, "file", rp);
    }
  }
  if((rp = tcmapget2(map, "file")) != NULL && (!strcmp(rp, ".") || !strcmp(rp, "..")))
    tcmapout2(map, "file");
  return map;
}


/* Encode a serial object with Base64 encoding. */
char *tcbaseencode(const char *ptr, int size){
  assert(ptr && size >= 0);
  char *tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const unsigned char *obj = (const unsigned char *)ptr;
  char *buf;
  TCMALLOC(buf, 4 * (size + 2) / 3 + 1);
  char *wp = buf;
  for(int i = 0; i < size; i += 3){
    switch(size - i){
      case 1:
        *wp++ = tbl[obj[0] >> 2];
        *wp++ = tbl[(obj[0] & 3) << 4];
        *wp++ = '=';
        *wp++ = '=';
        break;
      case 2:
        *wp++ = tbl[obj[0] >> 2];
        *wp++ = tbl[((obj[0] & 3) << 4) + (obj[1] >> 4)];
        *wp++ = tbl[(obj[1] & 0xf) << 2];
        *wp++ = '=';
        break;
      default:
        *wp++ = tbl[obj[0] >> 2];
        *wp++ = tbl[((obj[0] & 3) << 4) + (obj[1] >> 4)];
        *wp++ = tbl[((obj[1] & 0xf) << 2) + (obj[2] >> 6)];
        *wp++ = tbl[obj[2] & 0x3f];
        break;
    }
    obj += 3;
  }
  *wp = '\0';
  return buf;
}


/* Decode a string encoded with Base64 encoding. */
char *tcbasedecode(const char *str, int *sp){
  assert(str && sp);
  int cnt = 0;
  int bpos = 0;
  int eqcnt = 0;
  int len = strlen(str);
  unsigned char *obj;
  TCMALLOC(obj, len + 4);
  unsigned char *wp = obj;
  while(bpos < len && eqcnt == 0){
    int bits = 0;
    int i;
    for(i = 0; bpos < len && i < 4; bpos++){
      if(str[bpos] >= 'A' && str[bpos] <= 'Z'){
        bits = (bits << 6) | (str[bpos] - 'A');
        i++;
      } else if(str[bpos] >= 'a' && str[bpos] <= 'z'){
        bits = (bits << 6) | (str[bpos] - 'a' + 26);
        i++;
      } else if(str[bpos] >= '0' && str[bpos] <= '9'){
        bits = (bits << 6) | (str[bpos] - '0' + 52);
        i++;
      } else if(str[bpos] == '+'){
        bits = (bits << 6) | 62;
        i++;
      } else if(str[bpos] == '/'){
        bits = (bits << 6) | 63;
        i++;
      } else if(str[bpos] == '='){
        bits <<= 6;
        i++;
        eqcnt++;
      }
    }
    if(i == 0 && bpos >= len) continue;
    switch(eqcnt){
      case 0:
        *wp++ = (bits >> 16) & 0xff;
        *wp++ = (bits >> 8) & 0xff;
        *wp++ = bits & 0xff;
        cnt += 3;
        break;
      case 1:
        *wp++ = (bits >> 16) & 0xff;
        *wp++ = (bits >> 8) & 0xff;
        cnt += 2;
        break;
      case 2:
        *wp++ = (bits >> 16) & 0xff;
        cnt += 1;
        break;
    }
  }
  obj[cnt] = '\0';
  *sp = cnt;
  return (char *)obj;
}


/* Encode a serial object with Quoted-printable encoding. */
char *tcquoteencode(const char *ptr, int size){
  assert(ptr && size >= 0);
  const unsigned char *rp = (const unsigned char *)ptr;
  char *buf;
  TCMALLOC(buf, size * 3 + 1);
  char *wp = buf;
  int cols = 0;
  for(int i = 0; i < size; i++){
    if(rp[i] == '=' || (rp[i] < 0x20 && rp[i] != '\r' && rp[i] != '\n' && rp[i] != '\t') ||
       rp[i] > 0x7e){
      wp += sprintf(wp, "=%02X", rp[i]);
      cols += 3;
    } else {
      *(wp++) = rp[i];
      cols++;
    }
  }
  *wp = '\0';
  return buf;
}


/* Decode a string encoded with Quoted-printable encoding. */
char *tcquotedecode(const char *str, int *sp){
  assert(str && sp);
  char *buf;
  TCMALLOC(buf, strlen(str) + 1);
  char *wp = buf;
  for(; *str != '\0'; str++){
    if(*str == '='){
      str++;
      if(*str == '\0'){
        break;
      } else if(str[0] == '\r' && str[1] == '\n'){
        str++;
      } else if(str[0] != '\n' && str[0] != '\r'){
        if(*str >= 'A' && *str <= 'Z'){
          *wp = (*str - 'A' + 10) * 16;
        } else if(*str >= 'a' && *str <= 'z'){
          *wp = (*str - 'a' + 10) * 16;
        } else {
          *wp = (*str - '0') * 16;
        }
        str++;
        if(*str == '\0') break;
        if(*str >= 'A' && *str <= 'Z'){
          *wp += *str - 'A' + 10;
        } else if(*str >= 'a' && *str <= 'z'){
          *wp += *str - 'a' + 10;
        } else {
          *wp += *str - '0';
        }
        wp++;
      }
    } else {
      *wp = *str;
      wp++;
    }
  }
  *wp = '\0';
  *sp = wp - buf;
  return buf;
}

/* Split a string of MIME into headers and the body. */
char *tcmimebreak(const char *ptr, int size, TCMAP *headers, int *sp){
  assert(ptr && size >= 0 && sp);
  const char *head = NULL;
  int hlen = 0;
  for(int i = 0; i < size; i++){
    if(i < size - 4 && ptr[i] == '\r' && ptr[i+1] == '\n' &&
       ptr[i+2] == '\r' && ptr[i+3] == '\n'){
      head = ptr;
      hlen = i;
      ptr += i + 4;
      size -= i + 4;
      break;
    } else if(i < size - 2 && ptr[i] == '\n' && ptr[i+1] == '\n'){
      head = ptr;
      hlen = i;
      ptr += i + 2;
      size -= i + 2;
      break;
    }
  }
  if(head && headers){
    char *hbuf;
    TCMALLOC(hbuf, hlen + 1);
    int wi = 0;
    for(int i = 0; i < hlen; i++){
      if(head[i] == '\r') continue;
      if(i < hlen - 1 && head[i] == '\n' && (head[i+1] == ' ' || head[i+1] == '\t')){
        hbuf[wi++] = ' ';
        i++;
      } else {
        hbuf[wi++] = head[i];
      }
    }
    hbuf[wi] = '\0';
    TCLIST *list = tcstrsplit(hbuf, "\n");
    int ln = tclistnum(list);
    for(int i = 0; i < ln; i++){
      const char *line = TCLISTVALPTR(list, i);
      const char *pv = strchr(line, ':');
      if(pv){
        char *name = tcmemdup(line, pv - line);
        for(int j = 0; name[j] != '\0'; j++){
          if(name[j] >= 'A' && name[j] <= 'Z') name[j] -= 'A' - 'a';
        }
        pv++;
        while(*pv == ' ' || *pv == '\t'){
          pv++;
        }
        tcmapput2(headers, name, pv);
        free(name);
      }
    }
    tclistdel(list);
    free(hbuf);
    const char *pv = tcmapget2(headers, "content-type");
    if(pv){
      const char *ep = strchr(pv, ';');
      if(ep){
        tcmapput(headers, "TYPE", 4, pv, ep - pv);
        do {
          ep++;
          while(ep[0] == ' '){
            ep++;
          }
          if(tcstrifwm(ep, "charset=")){
            ep += 8;
            while(*ep > '\0' && *ep <= ' '){
              ep++;
            }
            if(ep[0] == '"') ep++;
            pv = ep;
            while(ep[0] != '\0' && ep[0] != ' ' && ep[0] != '"' && ep[0] != ';'){
              ep++;
            }
            tcmapput(headers, "CHARSET", 7, pv, ep - pv);
          } else if(tcstrifwm(ep, "boundary=")){
            ep += 9;
            while(*ep > '\0' && *ep <= ' '){
              ep++;
            }
            if(ep[0] == '"'){
              ep++;
              pv = ep;
              while(ep[0] != '\0' && ep[0] != '"'){
                ep++;
              }
            } else {
              pv = ep;
              while(ep[0] != '\0' && ep[0] != ' ' && ep[0] != '"' && ep[0] != ';'){
                ep++;
              }
            }
            tcmapput(headers, "BOUNDARY", 8, pv, ep - pv);
          }
        } while((ep = strchr(ep, ';')) != NULL);
      } else {
        tcmapput(headers, "TYPE", 4, pv, strlen(pv));
      }
    }
    if((pv = tcmapget2(headers, "content-disposition")) != NULL){
      char *ep = strchr(pv, ';');
      if(ep){
        tcmapput(headers, "DISPOSITION", 11, pv, ep - pv);
        do {
          ep++;
          while(ep[0] == ' '){
            ep++;
          }
          if(tcstrifwm(ep, "filename=")){
            ep += 9;
            if(ep[0] == '"') ep++;
            pv = ep;
            while(ep[0] != '\0' && ep[0] != '"'){
              ep++;
            }
            tcmapput(headers, "FILENAME", 8, pv, ep - pv);
          } else if(tcstrifwm(ep, "name=")){
            ep += 5;
            if(ep[0] == '"') ep++;
            pv = ep;
            while(ep[0] != '\0' && ep[0] != '"'){
              ep++;
            }
            tcmapput(headers, "NAME", 4, pv, ep - pv);
          }
        } while((ep = strchr(ep, ';')) != NULL);
      } else {
        tcmapput(headers, "DISPOSITION", 11, pv, strlen(pv));
      }
    }
  }
  *sp = size;
  char *rv = tcmemdup(ptr, size);
  return rv;
}


/* Split multipart data of MIME into its parts. */
TCLIST *tcmimeparts(const char *ptr, int size, const char *boundary){
  assert(ptr && size >= 0 && boundary);
  TCLIST *list = tclistnew();
  int blen = strlen(boundary);
  if(blen < 1) return list;
  const char *pv = NULL;
  for(int i = 0; i < size; i++){
    if(ptr[i] == '-' && ptr[i+1] == '-' && i + 2 + blen < size &&
       tcstrfwm(ptr + i + 2, boundary) && strchr("\t\n\v\f\r ", ptr[i+2+blen])){
      pv = ptr + i + 2 + blen;
      if(*pv == '\r') pv++;
      if(*pv == '\n') pv++;
      size -= pv - ptr;
      ptr = pv;
      break;
    }
  }
  if(!pv) return list;
  for(int i = 0; i < size; i++){
    if(ptr[i] == '-' && ptr[i+1] == '-' && i + 2 + blen < size &&
       tcstrfwm(ptr + i + 2, boundary) && strchr("\t\n\v\f\r -", ptr[i+2+blen])){
      const char *ep = ptr + i;
      if(ep > ptr && ep[-1] == '\n') ep--;
      if(ep > ptr && ep[-1] == '\r') ep--;
      if(ep > pv) tclistpush(list, pv, ep - pv);
      pv = ptr + i + 2 + blen;
      if(*pv == '\r') pv++;
      if(*pv == '\n') pv++;
    }
  }
  return list;
}


/* Encode a serial object with hexadecimal encoding. */
char *tchexencode(const char *ptr, int size){
  assert(ptr && size >= 0);
  const unsigned char *rp = (const unsigned char *)ptr;
  char *buf;
  TCMALLOC(buf, size * 2 + 1);
  char *wp = buf;
  for(int i = 0; i < size; i++){
    wp += sprintf(wp, "%02x", rp[i]);
  }
  *wp = '\0';
  return buf;
}


/* Decode a string encoded with hexadecimal encoding. */
char *tchexdecode(const char *str, int *sp){
  assert(str && sp);
  int len = strlen(str);
  char *buf;
  TCMALLOC(buf, len + 1);
  char *wp = buf;
  for(int i = 0; i < len; i += 2){
    while(str[i] >= '\0' && str[i] <= ' '){
      i++;
    }
    int num = 0;
    int c = str[i];
    if(c == '\0') break;
    if(c >= '0' && c <= '9'){
      num = c - '0';
    } else if(c >= 'a' && c <= 'f'){
      num = c - 'a' + 10;
    } else if(c >= 'A' && c <= 'F'){
      num = c - 'A' + 10;
    } else if(c == '\0'){
      break;
    }
    c = str[i+1];
    if(c >= '0' && c <= '9'){
      num = num * 0x10 + c - '0';
    } else if(c >= 'a' && c <= 'f'){
      num = num * 0x10 + c - 'a' + 10;
    } else if(c >= 'A' && c <= 'F'){
      num = num * 0x10 + c - 'A' + 10;
    } else if(c == '\0'){
      break;
    }
    *(wp++) = num;
  }
  *wp = '\0';
  *sp = wp - buf;
  return buf;
}


/* Escape meta characters in a string with the entity references of XML. */
char *tcxmlescape(const char *str){
  assert(str);
  const char *rp = str;
  int bsiz = 0;
  while(*rp != '\0'){
    switch(*rp){
      case '&':
        bsiz += 5;
        break;
      case '<':
        bsiz += 4;
        break;
      case '>':
        bsiz += 4;
        break;
      case '"':
        bsiz += 6;
        break;
      default:
        bsiz++;
        break;
    }
    rp++;
  }
  char *buf;
  TCMALLOC(buf, bsiz + 1);
  char *wp = buf;
  while(*str != '\0'){
    switch(*str){
      case '&':
        memcpy(wp, "&amp;", 5);
        wp += 5;
        break;
      case '<':
        memcpy(wp, "&lt;", 4);
        wp += 4;
        break;
      case '>':
        memcpy(wp, "&gt;", 4);
        wp += 4;
        break;
      case '"':
        memcpy(wp, "&quot;", 6);
        wp += 6;
        break;
      default:
        *(wp++) = *str;
        break;
    }
    str++;
  }
  *wp = '\0';
  return buf;
}


/*************************************************************************************************
 * encoding utilities (for experts)
 *************************************************************************************************/


/* Decode a data region in the x-www-form-urlencoded or multipart-form-data format. */
void tcwwwformdecode2(const void *ptr, int size, const char *type, TCMAP *params){
  assert(ptr && size >= 0 && params);
  if(type && tcstrfwm(tcstrskipspc(type), "multipart/")){
    const char *brd = strstr(type, "boundary=");
    if(brd){
      brd += 9;
      if(*brd == '"') brd++;
      char *bstr = tcstrdup(brd);
      char *wp = strchr(bstr, ';');
      if(wp) *wp = '\0';
      wp = strchr(bstr, '"');
      if(wp) *wp = '\0';
      TCLIST *parts = tcmimeparts(ptr, size, bstr);
      int pnum = tclistnum(parts);
      for(int i = 0; i < pnum; i++){
        int psiz;
        const char *part = tclistval(parts, i, &psiz);
        TCMAP *hmap = tcmapnew2(TCMAPTINYBNUM);
        int bsiz;
        char *body = tcmimebreak(part, psiz, hmap, &bsiz);
        int nsiz;
        const char *name = tcmapget(hmap, "NAME", 4, &nsiz);
        char numbuf[TCNUMBUFSIZ];
        if(!name){
          nsiz = sprintf(numbuf, "part:%d", i + 1);
          name = numbuf;
        }
        const char *tenc = tcmapget2(hmap, "content-transfer-encoding");
        if(tenc){
          if(tcstrifwm(tenc, "base64")){
            char *ebuf = tcbasedecode(body, &bsiz);
            free(body);
            body = ebuf;
          } else if(tcstrifwm(tenc, "quoted-printable")){
            char *ebuf = tcquotedecode(body, &bsiz);
            free(body);
            body = ebuf;
          }
        }
        tcmapputkeep(params, name, nsiz, body, bsiz);
        const char *fname = tcmapget2(hmap, "FILENAME");
        if(fname){
          if(*fname == '/'){
            fname = strrchr(fname, '/') + 1;
          } else if(((*fname >= 'a' && *fname <= 'z') || (*fname >= 'A' && *fname <= 'Z')) &&
                    fname[1] == ':' && fname[2] == '\\'){
            fname = strrchr(fname, '\\') + 1;
          }
          if(*fname != '\0'){
            char key[nsiz+10];
            sprintf(key, "%s_filename", name);
            tcmapput2(params, key, fname);
          }
        }
        free(body);
        tcmapdel(hmap);
      }
      tclistdel(parts);
      free(bstr);
    }
  } else {
    const char *rp = ptr;
    const char *pv = rp;
    const char *ep = rp + size;
    char stack[TCIOBUFSIZ];
    while(rp < ep){
      if(*rp == '&' || *rp == ';'){
        while(pv < rp && *pv > '\0' && *pv <= ' '){
          pv++;
        }
        if(rp > pv){
          int len = rp - pv;
          char *rbuf;
          if(len < sizeof(stack)){
            rbuf = stack;
          } else {
            TCMALLOC(rbuf, len + 1);
          }
          memcpy(rbuf, pv, len);
          rbuf[len] = '\0';
          char *sep = strchr(rbuf, '=');
          if(sep){
            *(sep++) = '\0';
          } else {
            sep = "";
          }
          int ksiz;
          char *kbuf = tcurldecode(rbuf, &ksiz);
          int vsiz;
          char *vbuf = tcurldecode(sep, &vsiz);
          if(!tcmapputkeep(params, kbuf, ksiz, vbuf, vsiz)){
            tcmapputcat(params, kbuf, ksiz, "", 1);
            tcmapputcat(params, kbuf, ksiz, vbuf, vsiz);
          }
          free(vbuf);
          free(kbuf);
          if(rbuf != stack) free(rbuf);
        }
        pv = rp + 1;
      }
      rp++;
    }
    while(pv < rp && *pv > '\0' && *pv <= ' '){
      pv++;
    }
    if(rp > pv){
      int len = rp - pv;
      char *rbuf;
      if(len < sizeof(stack)){
        rbuf = stack;
      } else {
        TCMALLOC(rbuf, len + 1);
      }
      memcpy(rbuf, pv, len);
      rbuf[len] = '\0';
      char *sep = strchr(rbuf, '=');
      if(sep){
        *(sep++) = '\0';
      } else {
        sep = "";
      }
      int ksiz;
      char *kbuf = tcurldecode(rbuf, &ksiz);
      int vsiz;
      char *vbuf = tcurldecode(sep, &vsiz);
      if(!tcmapputkeep(params, kbuf, ksiz, vbuf, vsiz)){
        tcmapputcat(params, kbuf, ksiz, "", 1);
        tcmapputcat(params, kbuf, ksiz, vbuf, vsiz);
      }
      free(vbuf);
      free(kbuf);
      if(rbuf != stack) free(rbuf);
    }
  }
}


/* Escape meta characters in a string with backslash escaping of the C language. */
char *tccstrescape(const char *str){
  assert(str);
  int asiz = TCXSTRUNIT * 2;
  char *buf;
  TCMALLOC(buf, asiz + 4);
  int wi = 0;
  bool hex = false;
  int c;
  while((c = *(unsigned char *)str) != '\0'){
    if(wi >= asiz){
      asiz *= 2;
      TCREALLOC(buf, buf, asiz + 4);
    }
    if(c < ' ' || c == 0x7f || c == '"' || c == '\'' || c == '\\'){
      switch(c){
        case '\t': wi += sprintf(buf + wi, "\\t"); break;
        case '\n': wi += sprintf(buf + wi, "\\n"); break;
        case '\r': wi += sprintf(buf + wi, "\\r"); break;
        case '\\': wi += sprintf(buf + wi, "\\\\"); break;
        default:
          wi += sprintf(buf + wi, "\\x%02X", c);
          hex = true;
          break;
      }
    } else {
      if(hex && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))){
        wi += sprintf(buf + wi, "\\x%02X", c);
        hex = true;
      } else {
        buf[wi++] = c;
        hex = false;
      }
    }
    str++;
  }
  buf[wi] = '\0';
  return buf;
}


/* Escape meta characters in a string with backslash escaping of JSON. */
char *tcjsonescape(const char *str){
  assert(str);
  int asiz = TCXSTRUNIT * 2;
  char *buf;
  TCMALLOC(buf, asiz + 6);
  int wi = 0;
  int c;
  while((c = *(unsigned char *)str) != '\0'){
    if(wi >= asiz){
      asiz *= 2;
      TCREALLOC(buf, buf, asiz + 6);
    }
    if(c < ' ' || c == 0x7f || c == '"' || c == '\'' || c == '\\'){
      switch(c){
        case '\t': wi += sprintf(buf + wi, "\\t"); break;
        case '\n': wi += sprintf(buf + wi, "\\n"); break;
        case '\r': wi += sprintf(buf + wi, "\\r"); break;
        case '\\': wi += sprintf(buf + wi, "\\\\"); break;
        default: wi += sprintf(buf + wi, "\\u%04X", c); break;
      }
    } else {
      buf[wi++] = c;
    }
    str++;
  }
  buf[wi] = '\0';
  return buf;
}



/*************************************************************************************************
 * template serializer
 *************************************************************************************************/


#define TCTMPLUNIT     65536             // allocation unit size of a template string
#define TCTMPLMAXDEP   256               // maximum depth of template blocks
#define TCTMPLBEGSEP   "[%"              // default beginning separator
#define TCTMPLENDSEP   "%]"              // default beginning separator
#define TCTYPRFXLIST   "[list]\0:"       // type prefix for a list object
#define TCTYPRFXMAP    "[map]\0:"        // type prefix for a list object


/* Store a list object into a map object with the type information. */
void tcmapputlist(TCMAP *map, const char *kstr, const TCLIST *obj){
  assert(map && kstr && obj);
  char vbuf[sizeof(TCTYPRFXLIST) - 1 + sizeof(obj)];
  memcpy(vbuf, TCTYPRFXLIST, sizeof(TCTYPRFXLIST) - 1);
  memcpy(vbuf + sizeof(TCTYPRFXLIST) - 1, &obj, sizeof(obj));
  tcmapput(map, kstr, strlen(kstr), vbuf, sizeof(vbuf));
}


/* Store a map object into a map object with the type information. */
void tcmapputmap(TCMAP *map, const char *kstr, const TCMAP *obj){
  assert(map && kstr && obj);
  char vbuf[sizeof(TCTYPRFXMAP) - 1 + sizeof(obj)];
  memcpy(vbuf, TCTYPRFXMAP, sizeof(TCTYPRFXMAP) - 1);
  memcpy(vbuf + sizeof(TCTYPRFXMAP) - 1, &obj, sizeof(obj));
  tcmapput(map, kstr, strlen(kstr), vbuf, sizeof(vbuf));
}



/*************************************************************************************************
 * bit operation utilities
 *************************************************************************************************/


/* Initialize a bit stream object as writer. */
void tcbitstrminitw(TCBITSTRM *bitstrm, char* obuf){
  bitstrm->sp = (uint8_t *)(obuf);
  bitstrm->cp = bitstrm->sp;
  *bitstrm->cp = 0;
  bitstrm->idx = 3;
  bitstrm->size = 1;
}


/* Concatenate a bit to a bit stream object. */
void tcbitstrmcat(TCBITSTRM *bitstrm, int sign){
  if(bitstrm->idx >= 8){
    *(++bitstrm->cp) = 0;
    bitstrm->idx = 0;
    bitstrm->size++;
  }
  *bitstrm->cp |= (sign << bitstrm->idx);
  bitstrm->idx++;
}


/* Set the end mark to a bit stream object. */
void tcbitstrmsetend(TCBITSTRM *bitstrm){
  if(bitstrm->idx >= 8){
    *(++bitstrm->cp) = 0;
    bitstrm->idx = 0;
    bitstrm->size++;
  }
  *bitstrm->sp |= bitstrm->idx & 7;
}


/* Get the size of the used region of a bit stream object. */
int tcbitstrmsize(TCBITSTRM *bitstrm){
  return bitstrm->size;
}


/* Initialize a bit stream object as reader. */
void tcbitstrminitr(TCBITSTRM *bitstrm, const char *ptr, int size){
  bitstrm->sp = (uint8_t *)(ptr);
  bitstrm->cp = bitstrm->sp;
  bitstrm->idx = 3;
  bitstrm->size = size;
}


/* Read a bit from a bit stream object. */
void tcbitstrmread(TCBITSTRM *bitstrm, int* sign){
  if(bitstrm->idx >= 8){
    bitstrm->cp++;
    bitstrm->idx = 0;
  }
  *sign = (*(bitstrm->cp) & (1 << bitstrm->idx)) > 0;
  bitstrm->idx++;
}


/* Get the number of bits of a bit stream object. */
int tcbitstrmnum(TCBITSTRM *bitstrm){
  int num = ((bitstrm->size - 1) << 3) + (*bitstrm->sp & 7) - 3;
  return num;
}



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


#define TCBSENCUNIT    8192             // unit size of TCBS encoding
#define TCBWTCNTMIN    64               // minimum element number of counting sort
#define TCBWTCNTLV     4                // maximum recursion level of counting sort
#define TCBWTBUFNUM    16384            // number of elements of BWT buffer

typedef struct {                         // type of structure for a BWT character
  int fchr;                              // character code of the first character
  int tchr;                              // character code of the last character
} TCBWTREC;


/* private function prototypes */
static void tcglobalinit(void);
static void tcglobaldestroy(void);
static void tcbwtsortstrcount(const char **arrays, int anum, int len, int level);
static void tcbwtsortstrinsert(const char **arrays, int anum, int len, int skip);
static void tcbwtsortstrheap(const char **arrays, int anum, int len, int skip);
static void tcbwtsortchrcount(unsigned char *str, int len);
static void tcbwtsortchrinsert(unsigned char *str, int len);
static void tcbwtsortreccount(TCBWTREC *arrays, int anum);
static void tcbwtsortrecinsert(TCBWTREC *array, int anum);
static int tcbwtsearchrec(TCBWTREC *array, int anum, int tchr);
static void tcmtfencode(char *ptr, int size);
static void tcmtfdecode(char *ptr, int size);
static int tcgammaencode(const char *ptr, int size, char *obuf);
static int tcgammadecode(const char *ptr, int size, char *obuf);


/* Get the message string corresponding to an error code. */
const char *tcerrmsg(int ecode){
  switch(ecode){
    case TCESUCCESS: return "success";
    case TCETHREAD: return "threading error";
    case TCEINVALID: return "invalid operation";
    case TCENOFILE: return "file not found";
    case TCENOPERM: return "no permission";
    case TCEMETA: return "invalid meta data";
    case TCERHEAD: return "invalid record header";
    case TCEOPEN: return "open error";
    case TCECLOSE: return "close error";
    case TCETRUNC: return "trunc error";
    case TCESTAT: return "stat error";
    case TCESEEK: return "seek error";
    case TCEREAD: return "read error";
    case TCEWRITE: return "write error";
    case TCEMMAP: return "mmap error";
    case TCELOCK: return "lock error";
    case TCEUNLINK: return "unlink error";
    case TCERENAME: return "rename error";
    case TCEMKDIR: return "mkdir error";
    case TCERMDIR: return "rmdir error";
    case TCEKEEP: return "existing record";
    case TCENOREC: return "no record found";
    case TCEMISC: return "miscellaneous error";
  }
  return "unknown error";
}


/* Show error message on the standard error output and exit. */
void *tcmyfatal(const char *message){
  assert(message);
  fprintf(stderr, "fatal error: %s\n", message);
  exit(1);
  return NULL;
}


/* Allocate a large nullified region. */
void *tczeromap(uint64_t size){
#if defined(_SYS_LINUX_)
  assert(size > 0);
  void *ptr = mmap(0, sizeof(size) + size,
                   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(ptr == MAP_FAILED) tcmyfatal("out of memory");
  *(uint64_t *)ptr = size;
  return (char *)ptr + sizeof(size);
#else
  assert(size > 0);
  void *ptr;
  TCCALLOC(ptr, 1, size);
  return ptr;
#endif
}


/* Free a large nullfied region. */
void tczerounmap(void *ptr){
#if defined(_SYS_LINUX_)
  assert(ptr);
  uint64_t size = *((uint64_t *)ptr - 1);
  munmap((char *)ptr - sizeof(size), sizeof(size) + size);
#else
  assert(ptr);
  free(ptr);
#endif
}


/* Global mutex object. */
static pthread_once_t tcglobalonce = PTHREAD_ONCE_INIT;
static pthread_rwlock_t tcglobalmutex;
static pthread_mutex_t tcpathmutex;
static TCMAP *tcpathmap;


/* Lock the absolute path of a file. */
bool tcpathlock(const char *path){
  assert(path);
  pthread_once(&tcglobalonce, tcglobalinit);
  if(pthread_mutex_lock(&tcpathmutex) != 0) return false;
  bool err = false;
  if(tcpathmap && !tcmapputkeep2(tcpathmap, path, "")) err = true;
  if(pthread_mutex_unlock(&tcpathmutex) != 0) err = true;
  return !err;
}


/* Unock the absolute path of a file. */
bool tcpathunlock(const char *path){
  assert(path);
  pthread_once(&tcglobalonce, tcglobalinit);
  if(pthread_mutex_lock(&tcpathmutex) != 0) return false;
  bool err = false;
  if(tcpathmap && !tcmapout2(tcpathmap, path)) err = true;
  if(pthread_mutex_unlock(&tcpathmutex) != 0) err = true;
  return !err;
}


/* Convert an integer to the string as binary numbers. */
int tcnumtostrbin(uint64_t num, char *buf, int col, int fc){
  assert(buf);
  char *wp = buf;
  int len = sizeof(num) * 8;
  bool zero = true;
  while(len-- > 0){
    if(num & (1ULL << 63)){
      *(wp++) = '1';
      zero = false;
    } else if(!zero){
      *(wp++) = '0';
    }
    num <<= 1;
  }
  if(col > 0){
    if(col > sizeof(num) * 8) col = sizeof(num) * 8;
    len = col - (wp - buf);
    if(len > 0){
      memmove(buf + len, buf, wp - buf);
      for(int i = 0; i < len; i++){
        buf[i] = fc;
      }
      wp += len;
    }
  } else if(zero){
    *(wp++) = '0';
  }
  *wp = '\0';
  return wp - buf;
}


/* Compress a serial object with TCBS encoding. */
char *tcbsencode(const char *ptr, int size, int *sp){
  assert(ptr && size >= 0 && sp);
  char *result;
  TCMALLOC(result, (size * 7) / 3 + (size / TCBSENCUNIT + 1) * sizeof(uint16_t) +
           TCBSENCUNIT * 2 + 0x200);
  char *pv = result + size + 0x100;
  char *wp = pv;
  char *tp = pv + size + 0x100;
  const char *end = ptr + size;
  while(ptr < end){
    int usiz = tclmin(TCBSENCUNIT, end - ptr);
    memcpy(tp, ptr, usiz);
    memcpy(tp + usiz, ptr, usiz);
    char *sp = wp;
    uint16_t idx = 0;
    wp += sizeof(idx);
    const char *arrays[usiz+1];
    for(int i = 0; i < usiz; i++){
      arrays[i] = tp + i;
    }
    const char *fp = arrays[0];
    if(usiz >= TCBWTCNTMIN){
      tcbwtsortstrcount(arrays, usiz, usiz, 0);
    } else if(usiz > 1){
      tcbwtsortstrinsert(arrays, usiz, usiz, 0);
    }
    for(int i = 0; i < usiz; i++){
      int tidx = arrays[i] - fp;
      if(tidx == 0){
        idx = i;
        *(wp++) = ptr[usiz-1];
      } else {
        *(wp++) = ptr[tidx-1];
      }
    }
    idx = TCHTOIS(idx);
    memcpy(sp, &idx, sizeof(idx));
    ptr += TCBSENCUNIT;
  }
  size = wp - pv;
  tcmtfencode(pv, size);
  int nsiz = tcgammaencode(pv, size, result);
  *sp = nsiz;
  return result;
}


/* Decompress a serial object compressed with TCBS encoding. */
char *tcbsdecode(const char *ptr, int size, int *sp){
  char *result;
  TCMALLOC(result, size * 9 + 0x200);
  char *wp = result + size + 0x100;
  int nsiz = tcgammadecode(ptr, size, wp);
  tcmtfdecode(wp, nsiz);
  ptr = wp;
  wp = result;
  const char *end = ptr + nsiz;
  while(ptr < end){
    uint16_t idx;
    memcpy(&idx, ptr, sizeof(idx));
    idx = TCITOHS(idx);
    ptr += sizeof(idx);
    int usiz = tclmin(TCBSENCUNIT, end - ptr);
    if(idx >= usiz) idx = 0;
    char rbuf[usiz+1];
    memcpy(rbuf, ptr, usiz);
    if(usiz >= TCBWTCNTMIN){
      tcbwtsortchrcount((unsigned char *)rbuf, usiz);
    } else if(usiz > 0){
      tcbwtsortchrinsert((unsigned char *)rbuf, usiz);
    }
    int fnums[0x100], tnums[0x100];
    memset(fnums, 0, sizeof(fnums));
    memset(tnums, 0, sizeof(tnums));
    TCBWTREC array[usiz+1];
    TCBWTREC *rp = array;
    for(int i = 0; i < usiz; i++){
      int fc = *(unsigned char *)(rbuf + i);
      rp->fchr = (fc << 23) + fnums[fc]++;
      int tc = *(unsigned char *)(ptr + i);
      rp->tchr = (tc << 23) + tnums[tc]++;
      rp++;
    }
    unsigned int fchr = array[idx].fchr;
    if(usiz >= TCBWTCNTMIN){
      tcbwtsortreccount(array, usiz);
    } else if(usiz > 1){
      tcbwtsortrecinsert(array, usiz);
    }
    for(int i = 0; i < usiz; i++){
      if(array[i].fchr == fchr){
        idx = i;
        break;
      }
    }
    for(int i = 0; i < usiz; i++){
      *(wp++) = array[idx].fchr >> 23;
      idx = tcbwtsearchrec(array, usiz, array[idx].fchr);
    }
    ptr += usiz;
  }
  *wp = '\0';
  *sp = wp - result;
  return result;
}


/* Get the aligned offset of a file offset. */
uint64_t tcpagealign(uint64_t off){
  int ps = sysconf(_SC_PAGESIZE);
  int diff = off & (ps - 1);
  return (diff > 0) ? off + ps - diff : off;
}


/* Initialize the global mutex object */
static void tcglobalinit(void){
  if(pthread_rwlock_init(&tcglobalmutex, NULL) != 0) tcmyfatal("rwlock error");
  if(pthread_mutex_init(&tcpathmutex, NULL) != 0) tcmyfatal("mutex error");
  tcpathmap = tcmapnew2(TCMAPTINYBNUM);
  atexit(tcglobaldestroy);
}


/* Destroy the global mutex object */
static void tcglobaldestroy(void){
  tcmapdel(tcpathmap);
  pthread_mutex_destroy(&tcpathmutex);
  pthread_rwlock_destroy(&tcglobalmutex);
}


/* Sort BWT string arrays by dicrionary order by counting sort.
   `array' specifies an array of string arrays.
   `anum' specifies the number of the array.
   `len' specifies the size of each string.
   `level' specifies the level of recursion. */
static void tcbwtsortstrcount(const char **arrays, int anum, int len, int level){
  assert(arrays && anum >= 0 && len >= 0);
  const char *nbuf[TCBWTBUFNUM];
  const char **narrays = nbuf;
  if(anum > TCBWTBUFNUM) TCMALLOC(narrays, sizeof(*narrays) * anum);
  int count[0x100], accum[0x100];
  memset(count, 0, sizeof(count));
  int skip = level < 0 ? 0 : level;
  for(int i = 0; i < anum; i++){
    count[((unsigned char *)arrays[i])[skip]]++;
  }
  memcpy(accum, count, sizeof(count));
  for(int i = 1; i < 0x100; i++){
    accum[i] = accum[i-1] + accum[i];
  }
  for(int i = 0; i < anum; i++){
    narrays[--accum[((unsigned char *)arrays[i])[skip]]] = arrays[i];
  }
  int off = 0;
  if(level >= 0 && level < TCBWTCNTLV){
    for(int i = 0; i < 0x100; i++){
      int c = count[i];
      if(c > 1){
        if(c >= TCBWTCNTMIN){
          tcbwtsortstrcount(narrays + off, c, len, level + 1);
        } else {
          tcbwtsortstrinsert(narrays + off, c, len, skip + 1);
        }
      }
      off += c;
    }
  } else {
    for(int i = 0; i < 0x100; i++){
      int c = count[i];
      if(c > 1){
        if(c >= TCBWTCNTMIN){
          tcbwtsortstrheap(narrays + off, c, len, skip + 1);
        } else {
          tcbwtsortstrinsert(narrays + off, c, len, skip + 1);
        }
      }
      off += c;
    }
  }
  memcpy(arrays, narrays, anum * sizeof(*narrays));
  if(narrays != nbuf) free(narrays);
}


/* Sort BWT string arrays by dicrionary order by insertion sort.
   `array' specifies an array of string arrays.
   `anum' specifies the number of the array.
   `len' specifies the size of each string.
   `skip' specifies the number of skipped bytes. */
static void tcbwtsortstrinsert(const char **arrays, int anum, int len, int skip){
  assert(arrays && anum >= 0 && len >= 0);
  for(int i = 1; i < anum; i++){
    int cmp = 0;
    const unsigned char *ap = (unsigned char *)arrays[i-1];
    const unsigned char *bp = (unsigned char *)arrays[i];
    for(int j = skip; j < len; j++){
      if(ap[j] != bp[j]){
        cmp = ap[j] - bp[j];
        break;
      }
    }
    if(cmp > 0){
      const char *swap = arrays[i];
      int j;
      for(j = i; j > 0; j--){
        int cmp = 0;
        const unsigned char *ap = (unsigned char *)arrays[j-1];
        const unsigned char *bp = (unsigned char *)swap;
        for(int k = skip; k < len; k++){
          if(ap[k] != bp[k]){
            cmp = ap[k] - bp[k];
            break;
          }
        }
        if(cmp < 0) break;
        arrays[j] = arrays[j-1];
      }
      arrays[j] = swap;
    }
  }
}


/* Sort BWT string arrays by dicrionary order by heap sort.
   `array' specifies an array of string arrays.
   `anum' specifies the number of the array.
   `len' specifies the size of each string.
   `skip' specifies the number of skipped bytes. */
static void tcbwtsortstrheap(const char **arrays, int anum, int len, int skip){
  assert(arrays && anum >= 0 && len >= 0);
  anum--;
  int bottom = (anum >> 1) + 1;
  int top = anum;
  while(bottom > 0){
    bottom--;
    int mybot = bottom;
    int i = mybot * 2;
    while(i <= top){
      if(i < top){
        int cmp = 0;
        const unsigned char *ap = (unsigned char *)arrays[i+1];
        const unsigned char *bp = (unsigned char *)arrays[i];
        for(int j = skip; j < len; j++){
          if(ap[j] != bp[j]){
            cmp = ap[j] - bp[j];
            break;
          }
        }
        if(cmp > 0) i++;
      }
      int cmp = 0;
      const unsigned char *ap = (unsigned char *)arrays[mybot];
      const unsigned char *bp = (unsigned char *)arrays[i];
      for(int j = skip; j < len; j++){
        if(ap[j] != bp[j]){
          cmp = ap[j] - bp[j];
          break;
        }
      }
      if(cmp >= 0) break;
      const char *swap = arrays[mybot];
      arrays[mybot] = arrays[i];
      arrays[i] = swap;
      mybot = i;
      i = mybot * 2;
    }
  }
  while(top > 0){
    const char *swap = arrays[0];
    arrays[0] = arrays[top];
    arrays[top] = swap;
    top--;
    int mybot = bottom;
    int i = mybot * 2;
    while(i <= top){
      if(i < top){
        int cmp = 0;
        const unsigned char *ap = (unsigned char *)arrays[i+1];
        const unsigned char *bp = (unsigned char *)arrays[i];
        for(int j = 0; j < len; j++){
          if(ap[j] != bp[j]){
            cmp = ap[j] - bp[j];
            break;
          }
        }
        if(cmp > 0) i++;
      }
      int cmp = 0;
      const unsigned char *ap = (unsigned char *)arrays[mybot];
      const unsigned char *bp = (unsigned char *)arrays[i];
      for(int j = 0; j < len; j++){
        if(ap[j] != bp[j]){
          cmp = ap[j] - bp[j];
          break;
        }
      }
      if(cmp >= 0) break;
      swap = arrays[mybot];
      arrays[mybot] = arrays[i];
      arrays[i] = swap;
      mybot = i;
      i = mybot * 2;
    }
  }
}


/* Sort BWT characters by code number by counting sort.
   `str' specifies a string.
   `len' specifies the length of the string. */
static void tcbwtsortchrcount(unsigned char *str, int len){
  assert(str && len >= 0);
  int cnt[0x100];
  memset(cnt, 0, sizeof(cnt));
  for(int i = 0; i < len; i++){
    cnt[str[i]]++;
  }
  int pos = 0;
  for(int i = 0; i < 0x100; i++){
    memset(str + pos, i, cnt[i]);
    pos += cnt[i];
  }
}


/* Sort BWT characters by code number by insertion sort.
   `str' specifies a string.
   `len' specifies the length of the string. */
static void tcbwtsortchrinsert(unsigned char *str, int len){
  assert(str && len >= 0);
  for(int i = 1; i < len; i++){
    if(str[i-1] - str[i] > 0){
      unsigned char swap = str[i];
      int j;
      for(j = i; j > 0; j--){
        if(str[j-1] - swap < 0) break;
        str[j] = str[j-1];
      }
      str[j] = swap;
    }
  }
}


/* Sort BWT records by code number by counting sort.
   `array' specifies an array of records.
   `anum' specifies the number of the array. */
static void tcbwtsortreccount(TCBWTREC *array, int anum){
  assert(array && anum >= 0);
  TCBWTREC nbuf[TCBWTBUFNUM];
  TCBWTREC *narray = nbuf;
  if(anum > TCBWTBUFNUM) TCMALLOC(narray, sizeof(*narray) * anum);
  int count[0x100], accum[0x100];
  memset(count, 0, sizeof(count));
  for(int i = 0; i < anum; i++){
    count[array[i].tchr>>23]++;
  }
  memcpy(accum, count, sizeof(count));
  for(int i = 1; i < 0x100; i++){
    accum[i] = accum[i-1] + accum[i];
  }
  for(int i = 0; i < 0x100; i++){
    accum[i] -= count[i];
  }
  for(int i = 0; i < anum; i++){
    narray[accum[array[i].tchr>>23]++] = array[i];
  }
  memcpy(array, narray, anum * sizeof(*narray));
  if(narray != nbuf) free(narray);
}


/* Sort BWT records by code number by insertion sort.
   `array' specifies an array of records..
   `anum' specifies the number of the array. */
static void tcbwtsortrecinsert(TCBWTREC *array, int anum){
  assert(array && anum >= 0);
  for(int i = 1; i < anum; i++){
    if(array[i-1].tchr - array[i].tchr > 0){
      TCBWTREC swap = array[i];
      int j;
      for(j = i; j > 0; j--){
        if(array[j-1].tchr - swap.tchr < 0) break;
        array[j] = array[j-1];
      }
      array[j] = swap;
    }
  }
}


/* Search the element of BWT records.
   `array' specifies an array of records.
   `anum' specifies the number of the array.
   `tchr' specifies the last code number. */
static int tcbwtsearchrec(TCBWTREC *array, int anum, int tchr){
  assert(array && anum >= 0);
  int bottom = 0;
  int top = anum;
  int mid;
  do {
    mid = (bottom + top) >> 1;
    if(array[mid].tchr == tchr){
      return mid;
    } else if(array[mid].tchr < tchr){
      bottom = mid + 1;
      if(bottom >= anum) break;
    } else {
      top = mid - 1;
    }
  } while(bottom <= top);
  return -1;
}


/* Initialization table for MTF encoder. */
const unsigned char tcmtftable[] = {
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
  0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
  0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
  0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
  0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
  0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
  0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
  0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
  0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
  0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
  0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
  0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
  0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
  0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};


/* Encode a region with MTF encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region. */
static void tcmtfencode(char *ptr, int size){
  unsigned char table1[0x100], table2[0x100], *table, *another;
  assert(ptr && size >= 0);
  memcpy(table1, tcmtftable, sizeof(tcmtftable));
  table = table1;
  another = table2;
  const char *end = ptr + size;
  char *wp = ptr;
  while(ptr < end){
    unsigned char c = *ptr;
    unsigned char *tp = table;
    unsigned char *tend = table + 0x100;
    while(tp < tend && *tp != c){
      tp++;
    }
    int idx = tp - table;
    *(wp++) = idx;
    if(idx > 0){
      memcpy(another, &c, 1);
      memcpy(another + 1, table, idx);
      memcpy(another + 1 + idx, table + idx + 1, 255 - idx);
      unsigned char *swap = table;
      table = another;
      another = swap;
    }
    ptr++;
  }
}


/* Decode a region compressed with MTF encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region. */
static void tcmtfdecode(char *ptr, int size){
  assert(ptr && size >= 0);
  unsigned char table1[0x100], table2[0x100], *table, *another;
  assert(ptr && size >= 0);
  memcpy(table1, tcmtftable, sizeof(tcmtftable));
  table = table1;
  another = table2;
  const char *end = ptr + size;
  char *wp = ptr;
  while(ptr < end){
    int idx = *(unsigned char *)ptr;
    unsigned char c = table[idx];
    *(wp++) = c;
    if(idx > 0){
      memcpy(another, &c, 1);
      memcpy(another + 1, table, idx);
      memcpy(another + 1 + idx, table + idx + 1, 255 - idx);
      unsigned char *swap = table;
      table = another;
      another = swap;
    }
    ptr++;
  }
}


/* Encode a region with Elias gamma encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   `obuf' specifies the pointer to the output buffer.
   The return value is the size of the output buffer. */
static int tcgammaencode(const char *ptr, int size, char *obuf){
  assert(ptr && size >= 0 && obuf);
  TCBITSTRM strm;
  tcbitstrminitw(&strm, obuf);
  const char *end = ptr + size;
  while(ptr < end){
    unsigned int c = *(unsigned char *)ptr;
    if(!c){
      tcbitstrmcat(&strm, 1);
    } else {
      c++;
      int plen = 8;
      while(plen > 0 && !(c & (1 << plen))){
        plen--;
      }
      int jlen = plen;
      while(jlen-- > 0){
        tcbitstrmcat(&strm, 0);
      }
      while(plen >= 0){
        int sign = (c & (1 << plen)) > 0;
        tcbitstrmcat(&strm, sign);
        plen--;
      }
    }
    ptr++;
  }
  tcbitstrmsetend(&strm);
  return tcbitstrmsize(&strm);
}


/* Decode a region compressed with Elias gamma encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   `obuf' specifies the pointer to the output buffer.
   The return value is the size of the output buffer. */
static int tcgammadecode(const char *ptr, int size, char *obuf){
  assert(ptr && size >= 0 && obuf);
  char *wp = obuf;
  TCBITSTRM strm;
  tcbitstrminitr(&strm, ptr, size);
  int bnum = tcbitstrmnum(&strm);
  while(bnum > 0){
    int sign;
    tcbitstrmread(&strm, &sign);
    bnum--;
    if(sign){
      *(wp++) = 0;
    } else {
      int plen = 1;
      while(bnum > 0){
        tcbitstrmread(&strm, &sign);
        bnum--;
        if(sign) break;
        plen++;
      }
      unsigned int c = 1;
      while(bnum > 0 && plen-- > 0){
        tcbitstrmread(&strm, &sign);
        bnum--;
        c = (c << 1) + (sign > 0);
      }
      *(wp++) = c - 1;
    }
  }
  return wp - obuf;
}



// END OF FILE
