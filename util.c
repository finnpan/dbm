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


const void *tclistval0(const TCLIST *list, int index){
  return list->array[index+list->start].ptr;
}

/* Get the pointer to the region of an element of a list object. */
const void *tclistval(const TCLIST *list, int index, int *sp){
  assert(list && index >= 0 && sp);
  if(index >= list->num) return NULL;
  *sp = list->array[index+list->start].size;
  return tclistval0(list, index);
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
        if (sp) {
          *sp = rec->vsiz;
        }
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
  return tcmapget(map, kstr, ksiz, NULL);
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
  return map->msiz + map->rnum * (sizeof(*map->first) + sizeof(TCUNION_FOO)) +
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
  assert(kbuf);
  TCMAPREC *rec = (TCMAPREC *)((char *)kbuf - sizeof(*rec));
  uint32_t rksiz = rec->ksiz & TCMAPKMAXSIZ;
  if (sp) {
    *sp = rec->vsiz;
  }
  return (char *)kbuf + rksiz + TCALIGNPAD(rksiz);
}


/* Get the value string bound to the key fetched from the iterator of a map object. */
const char *tcmapiterval2(const char *kstr){
  assert(kstr);
  return tcmapiterval(kstr, NULL);
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
      kbuf = tclistval(args, 0, &ksiz);
      const char *vbuf;
      int vsiz;
      vbuf = tclistval(args, 1, &vsiz);
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
      kbuf = tclistval(args, 0, &ksiz);
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
      kbuf = tclistval(args, 0, &ksiz);
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
      kbuf = tclistval(args, i, &ksiz);
      vbuf = tclistval(args, i + 1, &vsiz);
      tcmdbput(mdb, kbuf, ksiz, vbuf, vsiz);
    }
  } else if(!strcmp(name, "outlist")){
    rv = tclistnew2(1);
    for(int i = 0; i < argc; i++){
      const char *kbuf;
      int ksiz;
      kbuf = tclistval(args, i, &ksiz);
      tcmdbout(mdb, kbuf, ksiz);
    }
  } else if(!strcmp(name, "getlist")){
    rv = tclistnew2(argc * 2);
    for(int i = 0; i < argc; i++){
      const char *kbuf;
      int ksiz;
      kbuf = tclistval(args, i, &ksiz);
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
      kbuf = tclistval(args, 0, &ksiz);
      int off = argc > 1 ? tcatoi(tclistval0(args, 1)) : 0;
      if(off < 0) off = 0;
      if(off > INT_MAX / 2 - 1) off = INT_MAX - 1;
      int len = argc > 2 ? tcatoi(tclistval0(args, 2)) : -1;
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
      kbuf = tclistval(args, 0, &ksiz);
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
      const char *regex = tclistval0(args, 0);
      int options = REG_EXTENDED | REG_NOSUB;
      if(*regex == '*'){
        options |= REG_ICASE;
        regex++;
      }
      regex_t rbuf;
      if(regcomp(&rbuf, regex, options) == 0){
        rv = tclistnew();
        int max = argc > 1 ? tcatoi(tclistval0(args, 1)) : 0;
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



/*************************************************************************************************
 * miscellaneous utilities
 *************************************************************************************************/


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
      const char *line = tclistval0(lines, i);
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
      const char *line = tclistval0(lines, i);
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
      const char *line = tclistval0(lines, i);
      if(tcstrifwm(line, "processor")) cnum++;
    }
    if(cnum > 0) tcmapprintf(info, "corenum", "%lld", (long long)cnum);
    tclistdel(lines);
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


/*************************************************************************************************
 * encoding utilities
 *************************************************************************************************/


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
      const char *line = tclistval0(list, i);
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

// END OF FILE
