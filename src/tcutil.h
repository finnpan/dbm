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


#ifndef _TCUTIL_H                        /* duplication check */
#define _TCUTIL_H

#if defined(__cplusplus)
#define __TCUTIL_CLINKAGEBEGIN extern "C" {
#define __TCUTIL_CLINKAGEEND }
#else
#define __TCUTIL_CLINKAGEBEGIN
#define __TCUTIL_CLINKAGEEND
#endif
__TCUTIL_CLINKAGEBEGIN


#include <stdlib.h>
#if ! defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <math.h>



/*************************************************************************************************
 * basic utilities
 *************************************************************************************************/


#define TCMALLOC(TC_res, TC_size) \
  do { \
    if(!((TC_res) = malloc(TC_size))) tcmyfatal("out of memory"); \
  } while(false)


#define TCCALLOC(TC_res, TC_nmemb, TC_size) \
  do { \
    if(!((TC_res) = calloc((TC_nmemb), (TC_size)))) tcmyfatal("out of memory"); \
  } while(false)


#define TCREALLOC(TC_res, TC_ptr, TC_size) \
  do { \
    if(!((TC_res) = realloc((TC_ptr), (TC_size)))) tcmyfatal("out of memory"); \
  } while(false)


/* Allocate a region on memory.
   `size' specifies the size of the region.
   The return value is the pointer to the allocated region.
   This function handles failure of memory allocation implicitly.  Because the region of the
   return value is allocated with the `malloc' call, it should be released with the `free' call
   when it is no longer in use. */
void *tcmalloc(size_t size);


/* Allocate a nullified region on memory.
   `nmemb' specifies the number of elements.
   `size' specifies the size of each element.
   The return value is the pointer to the allocated nullified region.
   This function handles failure of memory allocation implicitly.  Because the region of the
   return value is allocated with the `calloc' call, it should be released with the `free' call
   when it is no longer in use. */
void *tccalloc(size_t nmemb, size_t size);


/* Re-allocate a region on memory.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   The return value is the pointer to the re-allocated region.
   This function handles failure of memory allocation implicitly.  Because the region of the
   return value is allocated with the `realloc' call, it should be released with the `free' call
   when it is no longer in use. */
void *tcrealloc(void *ptr, size_t size);


/* Duplicate a region on memory.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   The return value is the pointer to the allocated region of the duplicate.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcmemdup(const void *ptr, size_t size);


/* Duplicate a string on memory.
   `str' specifies the string.
   The return value is the allocated string equivalent to the specified string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcstrdup(const void *str);


/* type of the pointer to a comparison function.
   `aptr' specifies the pointer to the region of one key.
   `asiz' specifies the size of the region of one key.
   `bptr' specifies the pointer to the region of the other key.
   `bsiz' specifies the size of the region of the other key.
   `op' specifies the pointer to the optional opaque object.
   The return value is positive if the former is big, negative if the latter is big, 0 if both
   are equivalent. */
typedef int (*TCCMP)(const char *aptr, int asiz, const char *bptr, int bsiz, void *op);

/* type of the pointer to a encoding or decoding function.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   `op' specifies the pointer to the optional opaque object.
   If successful, the return value is the pointer to the result object allocated with `malloc'
   call, else, it is `NULL'. */
typedef void *(*TCCODEC)(const void *ptr, int size, int *sp, void *op);

/* type of the pointer to a callback function to process record duplication.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   `op' specifies the pointer to the optional opaque object.
   The return value is the pointer to the result object allocated with `malloc'.  It is
   released by the caller.  If it is `NULL', the record is not modified. */
typedef void *(*TCPDPROC)(const void *vbuf, int vsiz, int *sp, void *op);

/* type of the pointer to a iterator function.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `op' specifies the pointer to the optional opaque object.
   The return value is true to continue iteration or false to stop iteration. */
typedef bool (*TCITER)(const void *kbuf, int ksiz, const void *vbuf, int vsiz, void *op);



/*************************************************************************************************
 * extensible string
 *************************************************************************************************/


typedef struct {                         /* type of structure for an extensible string object */
  char *ptr;                             /* pointer to the region */
  int size;                              /* size of the region */
  int asize;                             /* size of the allocated region */
} TCXSTR;


/* Create an extensible string object.
   The return value is the new extensible string object. */
TCXSTR *tcxstrnew(void);


/* Create an extensible string object with the initial allocation size.
   `asiz' specifies the initial allocation size.
   The return value is the new extensible string object. */
TCXSTR *tcxstrnew2(int asiz);


/* Delete an extensible string object.
   `xstr' specifies the extensible string object.
   Note that the deleted object and its derivatives can not be used anymore. */
void tcxstrdel(TCXSTR *xstr);


/* Concatenate a region to the end of an extensible string object.
   `xstr' specifies the extensible string object.
   `ptr' specifies the pointer to the region to be appended.
   `size' specifies the size of the region. */
void tcxstrcat(TCXSTR *xstr, const void *ptr, int size);


/* Concatenate a character string to the end of an extensible string object.
   `xstr' specifies the extensible string object.
   `str' specifies the string to be appended. */
void tcxstrcat2(TCXSTR *xstr, const char *str);


/* Get the pointer of the region of an extensible string object.
   `xstr' specifies the extensible string object.
   The return value is the pointer of the region of the object.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string. */
const void *tcxstrptr(const TCXSTR *xstr);


/* Get the size of the region of an extensible string object.
   `xstr' specifies the extensible string object.
   The return value is the size of the region of the object. */
int tcxstrsize(const TCXSTR *xstr);


/* Clear an extensible string object.
   `xstr' specifies the extensible string object.
   The internal buffer of the object is cleared and the size is set zero. */
void tcxstrclear(TCXSTR *xstr);


/* Perform formatted output into an extensible string object.
   `xstr' specifies the extensible string object.
   `format' specifies the printf-like format string.  The conversion character `%' can be used
   with such flag characters as `s', `d', `o', `u', `x', `X', `c', `e', `E', `f', `g', `G', `@',
   `?', `b', and `%'.  `@' works as with `s' but escapes meta characters of XML.  `?' works as
   with `s' but escapes meta characters of URL.  `b' converts an integer to the string as binary
   numbers.  The other conversion character work as with each original.
   The other arguments are used according to the format string. */
void tcxstrprintf(TCXSTR *xstr, const char *format, ...);


/* Allocate a formatted string on memory.
   `format' specifies the printf-like format string.  The conversion character `%' can be used
   with such flag characters as `s', `d', `o', `u', `x', `X', `c', `e', `E', `f', `g', `G', `@',
   `?', `b', and `%'.  `@' works as with `s' but escapes meta characters of XML.  `?' works as
   with `s' but escapes meta characters of URL.  `b' converts an integer to the string as binary
   numbers.  The other conversion character work as with each original.
   The other arguments are used according to the format string.
   The return value is the pointer to the region of the result string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcsprintf(const char *format, ...);


/* Convert an extensible string object into a usual allocated region.
   `xstr' specifies the extensible string object.
   The return value is the pointer to the allocated region of the object.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when it
   is no longer in use.  Because the region of the original object is deleted, it should not be
   deleted again. */
void *tcxstrtomalloc(TCXSTR *xstr);



/*************************************************************************************************
 * array list
 *************************************************************************************************/


typedef struct {                         /* type of structure for an element of a list */
  char *ptr;                             /* pointer to the region */
  int size;                              /* size of the effective region */
} TCLISTDATUM;

typedef struct {                         /* type of structure for an array list */
  TCLISTDATUM *array;                    /* array of data */
  int anum;                              /* number of the elements of the array */
  int start;                             /* start index of used elements */
  int num;                               /* number of used elements */
} TCLIST;


/* Create a list object.
   The return value is the new list object. */
TCLIST *tclistnew(void);


/* Create a list object with expecting the number of elements.
   `anum' specifies the number of elements expected to be stored in the list.
   The return value is the new list object. */
TCLIST *tclistnew2(int anum);


/* Delete a list object.
   `list' specifies the list object.
   Note that the deleted object and its derivatives can not be used anymore. */
void tclistdel(TCLIST *list);


/* Get the number of elements of a list object.
   `list' specifies the list object.
   The return value is the number of elements of the list. */
int tclistnum(const TCLIST *list);


/* Get the pointer to the region of an element of a list object.
   `list' specifies the list object.
   `index' specifies the index of the element.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   The return value is the pointer to the region of the value.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  If `index' is equal to or more than
   the number of elements, the return value is `NULL'. */
const void *tclistval(const TCLIST *list, int index, int *sp);


/* Get the string of an element of a list object.
   `list' specifies the list object.
   `index' specifies the index of the element.
   The return value is the string of the value.
   If `index' is equal to or more than the number of elements, the return value is `NULL'. */
const char *tclistval2(const TCLIST *list, int index);


/* Add an element at the end of a list object.
   `list' specifies the list object.
   `ptr' specifies the pointer to the region of the new element.
   `size' specifies the size of the region. */
void tclistpush(TCLIST *list, const void *ptr, int size);


/* Add a string element at the end of a list object.
   `list' specifies the list object.
   `str' specifies the string of the new element. */
void tclistpush2(TCLIST *list, const char *str);


/* Remove a string element of the top of a list object.
   `list' specifies the list object.
   The return value is the string of the removed element.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use.  If the list is empty, the return
   value is `NULL'. */
char *tclistshift(TCLIST *list);


/* Overwrite an element at the specified location of a list object.
   `list' specifies the list object.
   `index' specifies the index of the element to be overwritten.
   `ptr' specifies the pointer to the region of the new content.
   `size' specifies the size of the new content.
   If `index' is equal to or more than the number of elements, this function has no effect. */
void tclistover(TCLIST *list, int index, const void *ptr, int size);


/* Sort elements of a list object in lexical order.
   `list' specifies the list object. */
void tclistsort(TCLIST *list);


/* Clear a list object.
   `list' specifies the list object.
   All elements are removed. */
void tclistclear(TCLIST *list);


/* Add an allocated element at the end of a list object.
   `list' specifies the list object.
   `ptr' specifies the pointer to the region allocated with `malloc' call.
   `size' specifies the size of the region.
   Note that the specified region is released when the object is deleted. */
void tclistpushmalloc(TCLIST *list, void *ptr, int size);


/* Perform formatted output into a list object.
   `list' specifies the list object.
   `format' specifies the printf-like format string.  The conversion character `%' can be used
   with such flag characters as `s', `d', `o', `u', `x', `X', `c', `e', `E', `f', `g', `G', `@',
   `?', `b', and `%'.  `@' works as with `s' but escapes meta characters of XML.  `?' works as
   with `s' but escapes meta characters of URL.  `b' converts an integer to the string as binary
   numbers.  The other conversion character work as with each original.
   The other arguments are used according to the format string. */
void tclistprintf(TCLIST *list, const char *format, ...);


/* Alias of `tclistval' but not checking size. */
#define TCLISTVAL(TC_ptr, TC_list, TC_index, TC_size) \
  do { \
    (TC_ptr) = (TC_list)->array[(TC_index)+(TC_list)->start].ptr; \
    (TC_size) = (TC_list)->array[(TC_index)+(TC_list)->start].size; \
  } while(false)


/* Alias of `tclistval' but not checking size and not using the third parameter. */
#define TCLISTVALPTR(TC_list, TC_index) \
  ((void *)((TC_list)->array[(TC_index)+(TC_list)->start].ptr))



/*************************************************************************************************
 * hash map
 *************************************************************************************************/


typedef struct _TCMAPREC {               /* type of structure for an element of a map */
  int32_t ksiz;                          /* size of the region of the key */
  int32_t vsiz;                          /* size of the region of the value */
  struct _TCMAPREC *left;                /* pointer to the left child */
  struct _TCMAPREC *right;               /* pointer to the right child */
  struct _TCMAPREC *prev;                /* pointer to the previous element */
  struct _TCMAPREC *next;                /* pointer to the next element */
} TCMAPREC;

typedef struct {                         /* type of structure for a map */
  TCMAPREC **buckets;                    /* bucket array */
  TCMAPREC *first;                       /* pointer to the first element */
  TCMAPREC *last;                        /* pointer to the last element */
  TCMAPREC *cur;                         /* pointer to the current element */
  uint32_t bnum;                         /* number of buckets */
  uint64_t rnum;                         /* number of records */
  uint64_t msiz;                         /* total size of records */
} TCMAP;


/* Create a map object.
   The return value is the new map object. */
TCMAP *tcmapnew(void);


/* Create a map object with specifying the number of the buckets.
   `bnum' specifies the number of the buckets.
   The return value is the new map object. */
TCMAP *tcmapnew2(uint32_t bnum);


/* Delete a map object.
   `map' specifies the map object.
   Note that the deleted object and its derivatives can not be used anymore. */
void tcmapdel(TCMAP *map);


/* Store a record into a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If a record with the same key exists in the map, it is overwritten. */
void tcmapput(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a string record into a map object.
   `map' specifies the map object.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If a record with the same key exists in the map, it is overwritten. */
void tcmapput2(TCMAP *map, const char *kstr, const char *vstr);


/* Store a new record into a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the map, this function has no effect. */
bool tcmapputkeep(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a new string record into a map object.
   `map' specifies the map object.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the map, this function has no effect. */
bool tcmapputkeep2(TCMAP *map, const char *kstr, const char *vstr);


/* Concatenate a value at the end of the value of the existing record in a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If there is no corresponding record, a new record is created. */
void tcmapputcat(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Remove a record of a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true.  False is returned when no record corresponds to
   the specified key. */
bool tcmapout(TCMAP *map, const void *kbuf, int ksiz);


/* Remove a string record of a map object.
   `map' specifies the map object.
   `kstr' specifies the string of the key.
   If successful, the return value is true.  False is returned when no record corresponds to
   the specified key. */
bool tcmapout2(TCMAP *map, const char *kstr);


/* Retrieve a record in a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the
   corresponding record.  `NULL' is returned when no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string. */
const void *tcmapget(const TCMAP *map, const void *kbuf, int ksiz, int *sp);


/* Retrieve a string record in a map object.
   `map' specifies the map object.
   `kstr' specifies the string of the key.
   If successful, the return value is the string of the value of the corresponding record.
   `NULL' is returned when no record corresponds. */
const char *tcmapget2(const TCMAP *map, const char *kstr);


/* Move a record to the edge of a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of a key.
   `ksiz' specifies the size of the region of the key.
   `head' specifies the destination which is the head if it is true or the tail if else.
   If successful, the return value is true.  False is returned when no record corresponds to
   the specified key. */
bool tcmapmove(TCMAP *map, const void *kbuf, int ksiz, bool head);


/* Initialize the iterator of a map object.
   `map' specifies the map object.
   The iterator is used in order to access the key of every record stored in the map object. */
void tcmapiterinit(TCMAP *map);


/* Get the next key of the iterator of a map object.
   `map' specifies the map object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'.  `NULL' is returned when no record can be fetched from the iterator.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.
   The order of iteration is assured to be the same as the stored order. */
const void *tcmapiternext(TCMAP *map, int *sp);


/* Get the next key string of the iterator of a map object.
   `map' specifies the map object.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'.  `NULL' is returned when no record can be fetched from the iterator.
   The order of iteration is assured to be the same as the stored order. */
const char *tcmapiternext2(TCMAP *map);


/* Get the number of records stored in a map object.
   `map' specifies the map object.
   The return value is the number of the records stored in the map object. */
uint64_t tcmaprnum(const TCMAP *map);


/* Get the total size of memory used in a map object.
   `map' specifies the map object.
   The return value is the total size of memory used in a map object. */
uint64_t tcmapmsiz(const TCMAP *map);


/* Create a list object containing all keys in a map object.
   `map' specifies the map object.
   The return value is the new list object containing all keys in the map object.
   Because the object of the return value is created with the function `tclistnew', it should
   be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcmapkeys(const TCMAP *map);


/* Create a list object containing all values in a map object.
   `map' specifies the map object.
   The return value is the new list object containing all values in the map object.
   Because the object of the return value is created with the function `tclistnew', it should
   be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcmapvals(const TCMAP *map);


/* Add an integer to a record in a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   The return value is the summation value.
   If the corresponding record exists, the value is treated as an integer and is added to.  If no
   record corresponds, a new record of the additional value is stored. */
int tcmapaddint(TCMAP *map, const void *kbuf, int ksiz, int num);


/* Add a real number to a record in a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   The return value is the summation value.
   If the corresponding record exists, the value is treated as a real number and is added to.  If
   no record corresponds, a new record of the additional value is stored. */
double tcmapadddouble(TCMAP *map, const void *kbuf, int ksiz, double num);


/* Clear a map object.
   `map' specifies the map object.
   All records are removed. */
void tcmapclear(TCMAP *map);


/* Remove front records of a map object.
   `map' specifies the map object.
   `num' specifies the number of records to be removed. */
void tcmapcutfront(TCMAP *map, int num);


/*************************************************************************************************
 * hash map (for experts)
 *************************************************************************************************/


/* Store a record and make it semivolatile in a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If a record with the same key exists in the map, it is overwritten.  The record is moved to
   the tail. */
void tcmapput3(TCMAP *map, const void *kbuf, int ksiz, const char *vbuf, int vsiz);


/* Store a record of the value of two regions into a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `fvbuf' specifies the pointer to the former region of the value.
   `fvsiz' specifies the size of the former region of the value.
   `lvbuf' specifies the pointer to the latter region of the value.
   `lvsiz' specifies the size of the latter region of the value.
   If a record with the same key exists in the map, it is overwritten. */
void tcmapput4(TCMAP *map, const void *kbuf, int ksiz,
               const void *fvbuf, int fvsiz, const void *lvbuf, int lvsiz);


/* Concatenate a value at the existing record and make it semivolatile in a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If there is no corresponding record, a new record is created. */
void tcmapputcat3(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a record into a map object with a duplication handler.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.  `NULL' means that record addition is
   ommited if there is no corresponding record.
   `vsiz' specifies the size of the region of the value.
   `proc' specifies the pointer to the callback function to process duplication.  It receives
   four parameters.  The first parameter is the pointer to the region of the value.  The second
   parameter is the size of the region of the value.  The third parameter is the pointer to the
   variable into which the size of the region of the return value is assigned.  The fourth
   parameter is the pointer to the optional opaque object.  It returns the pointer to the result
   object allocated with `malloc'.  It is released by the caller.  If it is `NULL', the record is
   not modified.  If it is `(void *)-1', the record is removed.
   `op' specifies an arbitrary pointer to be given as a parameter of the callback function.  If
   it is not needed, `NULL' can be specified.
   If successful, the return value is true, else, it is false. */
bool tcmapputproc(TCMAP *map, const void *kbuf, int ksiz, const void *vbuf, int vsiz,
                  TCPDPROC proc, void *op);


/* Retrieve a semivolatile record in a map object.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the
   corresponding record.  `NULL' is returned when no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  The internal region of the returned
   record is moved to the tail so that the record will survive for a time under LRU cache
   algorithm removing records from the head. */
const void *tcmapget3(TCMAP *map, const void *kbuf, int ksiz, int *sp);


/* Initialize the iterator of a map object at the record corresponding a key.
   `map' specifies the map object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If there is no record corresponding the condition, the iterator is not modified. */
void tcmapiterinit2(TCMAP *map, const void *kbuf, int ksiz);


/* Get the value bound to the key fetched from the iterator of a map object.
   `kbuf' specifies the pointer to the region of the iteration key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   The return value is the pointer to the region of the value of the corresponding record.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string. */
const void *tcmapiterval(const void *kbuf, int *sp);


/* Get the value string bound to the key fetched from the iterator of a map object.
   `kstr' specifies the string of the iteration key.
   The return value is the pointer to the region of the value of the corresponding record. */
const char *tcmapiterval2(const char *kstr);


/* Perform formatted output into a map object.
   `map' specifies the map object.
   `kstr' specifies the string of the key.
   `format' specifies the printf-like format string.  The conversion character `%' can be used
   with such flag characters as `s', `d', `o', `u', `x', `X', `c', `e', `E', `f', `g', `G', `@',
   `?', `b', and `%'.  `@' works as with `s' but escapes meta characters of XML.  `?' works as
   with `s' but escapes meta characters of URL.  `b' converts an integer to the string as binary
   numbers.  The other conversion character work as with each original.
   The other arguments are used according to the format string. */
void tcmapprintf(TCMAP *map, const char *kstr, const char *format, ...);



/*************************************************************************************************
 * on-memory hash database
 *************************************************************************************************/


typedef struct {                         /* type of structure for a on-memory hash database */
  void **mmtxs;                          /* mutexes for method */
  void *imtx;                            /* mutex for iterator */
  TCMAP **maps;                          /* internal map objects */
  int iter;                              /* index of maps for the iterator */
} TCMDB;


/* Create an on-memory hash database object.
   The return value is the new on-memory hash database object.
   The object can be shared by plural threads because of the internal mutex. */
TCMDB *tcmdbnew(void);


/* Create an on-memory hash database object with specifying the number of the buckets.
   `bnum' specifies the number of the buckets.
   The return value is the new on-memory hash database object.
   The object can be shared by plural threads because of the internal mutex. */
TCMDB *tcmdbnew2(uint32_t bnum);


/* Delete an on-memory hash database object.
   `mdb' specifies the on-memory hash database object. */
void tcmdbdel(TCMDB *mdb);


/* Store a record into an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If a record with the same key exists in the database, it is overwritten. */
void tcmdbput(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a new record into an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcmdbputkeep(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Concatenate a value at the end of the existing record in an on-memory hash database.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If there is no corresponding record, a new record is created. */
void tcmdbputcat(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Remove a record of an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true.  False is returned when no record corresponds to
   the specified key. */
bool tcmdbout(TCMDB *mdb, const void *kbuf, int ksiz);


/* Retrieve a record in an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the
   corresponding record.  `NULL' is returned when no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcmdbget(TCMDB *mdb, const void *kbuf, int ksiz, int *sp);


/* Get the size of the value of a record in an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1. */
int tcmdbvsiz(TCMDB *mdb, const void *kbuf, int ksiz);


/* Initialize the iterator of an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   The iterator is used in order to access the key of every record stored in the on-memory
   database. */
void tcmdbiterinit(TCMDB *mdb);


/* Get the next key of the iterator of an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'.  `NULL' is returned when no record can be fetched from the iterator.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use.  The order of iteration is assured to be the same as the stored
   order. */
void *tcmdbiternext(TCMDB *mdb, int *sp);


/* Get forward matching keys in an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `pbuf' specifies the pointer to the region of the prefix.
   `psiz' specifies the size of the region of the prefix.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding keys.  This function does never fail.
   It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use.  Note that this function
   may be very slow because every key in the database is scanned. */
TCLIST *tcmdbfwmkeys(TCMDB *mdb, const void *pbuf, int psiz, int max);


/* Get the number of records stored in an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   The return value is the number of the records stored in the database. */
uint64_t tcmdbrnum(TCMDB *mdb);


/* Get the total size of memory used in an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   The return value is the total size of memory used in the database. */
uint64_t tcmdbmsiz(TCMDB *mdb);


/* Add an integer to a record in an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   The return value is the summation value.
   If the corresponding record exists, the value is treated as an integer and is added to.  If no
   record corresponds, a new record of the additional value is stored. */
int tcmdbaddint(TCMDB *mdb, const void *kbuf, int ksiz, int num);


/* Add a real number to a record in an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   The return value is the summation value.
   If the corresponding record exists, the value is treated as a real number and is added to.  If
   no record corresponds, a new record of the additional value is stored. */
double tcmdbadddouble(TCMDB *mdb, const void *kbuf, int ksiz, double num);


/* Clear an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   All records are removed. */
void tcmdbvanish(TCMDB *mdb);


/* Remove front records of an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `num' specifies the number of records to be removed. */
void tcmdbcutfront(TCMDB *mdb, int num);



/*************************************************************************************************
 * on-memory hash database (for experts)
 *************************************************************************************************/


/* Store a record and make it semivolatile in an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If a record with the same key exists in the map, it is overwritten.  The record is moved to
   the tail. */
void tcmdbput3(TCMDB *mdb, const void *kbuf, int ksiz, const char *vbuf, int vsiz);


/* Store a record of the value of two regions into an on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `fvbuf' specifies the pointer to the former region of the value.
   `fvsiz' specifies the size of the former region of the value.
   `lvbuf' specifies the pointer to the latter region of the value.
   `lvsiz' specifies the size of the latter region of the value.
   If a record with the same key exists in the database, it is overwritten. */
void tcmdbput4(TCMDB *mdb, const void *kbuf, int ksiz,
               const void *fvbuf, int fvsiz, const void *lvbuf, int lvsiz);


/* Concatenate a value and make it semivolatile in on-memory hash database object.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If there is no corresponding record, a new record is created. */
void tcmdbputcat3(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a record into a on-memory hash database object with a duplication handler.
   `mdb' specifies the on-memory hash database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.  `NULL' means that record addition is
   ommited if there is no corresponding record.
   `vsiz' specifies the size of the region of the value.
   `proc' specifies the pointer to the callback function to process duplication.  It receives
   four parameters.  The first parameter is the pointer to the region of the value.  The second
   parameter is the size of the region of the value.  The third parameter is the pointer to the
   variable into which the size of the region of the return value is assigned.  The fourth
   parameter is the pointer to the optional opaque object.  It returns the pointer to the result
   object allocated with `malloc'.  It is released by the caller.  If it is `NULL', the record is
   not modified.  If it is `(void *)-1', the record is removed.
   `op' specifies an arbitrary pointer to be given as a parameter of the callback function.  If
   it is not needed, `NULL' can be specified.
   If successful, the return value is true, else, it is false. */
bool tcmdbputproc(TCMDB *mdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz,
                  TCPDPROC proc, void *op);


/* Initialize the iterator of an on-memory map database object in front of a key.
   `mdb' specifies the on-memory map database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If there is no record corresponding the condition, the iterator is not modified. */
void tcmdbiterinit2(TCMDB *mdb, const void *kbuf, int ksiz);


/* Process each record atomically of an on-memory hash database object.
   `iter' specifies the pointer to the iterator function called for each record.  It receives
   five parameters.  The first parameter is the pointer to the region of the key.  The second
   parameter is the size of the region of the key.  The third parameter is the pointer to the
   region of the value.  The fourth parameter is the size of the region of the value.  The fifth
   parameter is the pointer to the optional opaque object.  It returns true to continue iteration
   or false to stop iteration.
   `op' specifies an arbitrary pointer to be given as a parameter of the iterator function.  If
   it is not needed, `NULL' can be specified. */
void tcmdbforeach(TCMDB *mdb, TCITER iter, void *op);



/*************************************************************************************************
 * memory pool
 *************************************************************************************************/


typedef struct {                         /* type of an element of memory pool */
  void *ptr;                             /* pointer */
  void (*del)(void *);                   /* deleting function */
} TCMPELEM;

typedef struct {                         /* type of structure for a memory pool object */
  void *mutex;                           /* mutex for operations */
  TCMPELEM *elems;                       /* array of elements */
  int anum;                              /* number of the elements of the array */
  int num;                               /* number of used elements */
} TCMPOOL;


/* Create a memory pool object.
   The return value is the new memory pool object. */
TCMPOOL *tcmpoolnew(void);


/* Delete a memory pool object.
   `mpool' specifies the memory pool object.
   Note that the deleted object and its derivatives can not be used anymore. */
void tcmpooldel(TCMPOOL *mpool);


/* Relegate an arbitrary object to a memory pool object.
   `mpool' specifies the memory pool object.
   `ptr' specifies the pointer to the object to be relegated.  If it is `NULL', this function has
   no effect.
   `del' specifies the pointer to the function to delete the object.
   The return value is the pointer to the given object.
   This function assures that the specified object is deleted when the memory pool object is
   deleted. */
void *tcmpoolpush(TCMPOOL *mpool, void *ptr, void (*del)(void *));


/* Create a list object relegated to a memory pool object.
   The return value is the new list object under the memory pool. */
TCLIST *tcmpoollistnew(TCMPOOL *mpool);


/* Get the global memory pool object.
   The return value is the global memory pool object.
   The global memory pool object is a singleton and assured to be deleted when the porcess is
   terminating normally. */
TCMPOOL *tcmpoolglobal(void);



/*************************************************************************************************
 * miscellaneous utilities
 *************************************************************************************************/


/* Get the larger value of two integers.
   `a' specifies an integer.
   `b' specifies the other integer.
   The return value is the larger value of the two. */
long tclmax(long a, long b);


/* Get the lesser value of two integers.
   `a' specifies an integer.
   `b' specifies the other integer.
   The return value is the lesser value of the two. */
long tclmin(long a, long b);


/* Compare two strings with case insensitive evaluation.
   `astr' specifies a string.
   `bstr' specifies of the other string.
   The return value is positive if the former is big, negative if the latter is big, 0 if both
   are equivalent. */
int tcstricmp(const char *astr, const char *bstr);


/* Check whether a string begins with a key.
   `str' specifies the target string.
   `key' specifies the forward matching key string.
   The return value is true if the target string begins with the key, else, it is false. */
bool tcstrfwm(const char *str, const char *key);


/* Check whether a string begins with a key with case insensitive evaluation.
   `str' specifies the target string.
   `key' specifies the forward matching key string.
   The return value is true if the target string begins with the key, else, it is false. */
bool tcstrifwm(const char *str, const char *key);


/* Check whether a string ends with a key.
   `str' specifies the target string.
   `key' specifies the backward matching key string.
   The return value is true if the target string ends with the key, else, it is false. */
bool tcstrbwm(const char *str, const char *key);


/* Check whether a string ends with a key with case insensitive evaluation.
   `str' specifies the target string.
   `key' specifies the backward matching key string.
   The return value is true if the target string ends with the key, else, it is false. */
bool tcstribwm(const char *str, const char *key);


/* Convert the letters of a string into lower case.
   `str' specifies the string to be converted.
   The return value is the string itself. */
char *tcstrtolower(char *str);


/* Cut space characters at head or tail of a string.
   `str' specifies the string to be converted.
   The return value is the string itself. */
char *tcstrtrim(char *str);


/* Squeeze space characters in a string and trim it.
   `str' specifies the string to be converted.
   The return value is the string itself. */
char *tcstrsqzspc(char *str);


/* Convert a UTF-8 string into a UCS-2 array.
   `str' specifies the UTF-8 string.
   `ary' specifies the pointer to the region into which the result UCS-2 codes are written.  The
   size of the buffer should be sufficient.
   `np' specifies the pointer to a variable into which the number of elements of the result array
   is assigned. */
void tcstrutftoucs(const char *str, uint16_t *ary, int *np);


/* Convert a UCS-2 array into a UTF-8 string.
   `ary' specifies the array of UCS-2 codes.
   `num' specifies the number of the array.
   `str' specifies the pointer to the region into which the result UTF-8 string is written.  The
   size of the buffer should be sufficient.
   The return value is the length of the result string. */
int tcstrucstoutf(const uint16_t *ary, int num, char *str);


/* Create a list object by splitting a string.
   `str' specifies the source string.
   `delim' specifies a string containing delimiting characters.
   The return value is a list object of the split elements.
   If two delimiters are successive, it is assumed that an empty element is between the two.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcstrsplit(const char *str, const char *delims);


/* Convert a string to an integer.
   `str' specifies the string.
   The return value is the integer.  If the string does not contain numeric expression, 0 is
   returned.
   This function is equivalent to `atoll' except that it does not depend on the locale. */
int64_t tcatoi(const char *str);


/* Convert a string with a metric prefix to an integer.
   `str' specifies the string, which can be trailed by a binary metric prefix.  "K", "M", "G",
   "T", "P", and "E" are supported.  They are case-insensitive.
   The return value is the integer.  If the string does not contain numeric expression, 0 is
   returned.  If the integer overflows the domain, `INT64_MAX' or `INT64_MIN' is returned
   according to the sign. */
int64_t tcatoix(const char *str);


/* Convert a string to a real number.
   `str' specifies the string.
   The return value is the real number.  If the string does not contain numeric expression, 0.0
   is returned.
   This function is equivalent to `atof' except that it does not depend on the locale. */
double tcatof(const char *str);


/* Check whether a string matches a regular expression.
   `str' specifies the target string.
   `regex' specifies the regular expression string.  If it begins with `*', the trailing
   substring is used as a case-insensitive regular expression.
   The return value is true if matching is success, else, it is false. */
bool tcregexmatch(const char *str, const char *regex);


/* Get the time of day in seconds.
   The return value is the time of day in seconds.  The accuracy is in microseconds. */
double tctime(void);


/* Format a date as a string in W3CDTF.
   `t' specifies the source time in seconds from the epoch.  If it is `INT64_MAX', the current
   time is specified.
   `jl' specifies the jet lag of a location in seconds.  If it is `INT_MAX', the local jet lag is
   specified.
   `buf' specifies the pointer to the region into which the result string is written.  The size
   of the buffer should be equal to or more than 48 bytes.
   W3CDTF represents a date as "YYYY-MM-DDThh:mm:ddTZD". */
void tcdatestrwww(int64_t t, int jl, char *buf);


/* Get the jet lag of the local time.
   The return value is the jet lag of the local time in seconds. */
int tcjetlag(void);


/*************************************************************************************************
 * miscellaneous utilities (for experts)
 *************************************************************************************************/


enum {                                   /* enumeration for UCS normalization */
  TCUNSPACE = 1 << 0,                    /* white space normalization */
  TCUNLOWER = 1 << 1,                    /* lower case normalization */
  TCUNNOACC = 1 << 2,                    /* strip accent marks */
  TCUNWIDTH = 1 << 3                     /* half-width normalization */
};

enum {                                   /* enumeration for KWIC generator */
  TCKWMUTAB = 1 << 0,                    /* mark up by tabs */
  TCKWMUCTRL = 1 << 1,                   /* mark up by control characters */
  TCKWMUBRCT = 1 << 2,                   /* mark up by brackets */
  TCKWNOOVER = 1 << 24,                  /* no overlap */
  TCKWPULEAD = 1 << 25                   /* pick up the lead string */
};

/* Convert a hexadecimal string to an integer.
   `str' specifies the string.
   The return value is the integer.  If the string does not contain numeric expression, 0 is
   returned. */
int64_t tcatoih(const char *str);


/* Skip space characters at head of a string.
   `str' specifies the string.
   The return value is the pointer to the first non-space character. */
const char *tcstrskipspc(const char *str);


/* Normalize a UCS-2 array.
   `ary' specifies the array of UCS-2 codes.
   `num' specifies the number of elements of the array.
   `opts' specifies options by bitwise-or: `TCUNSPACE' specifies that white space characters are
   normalized into the ASCII space and they are squeezed into one, `TCUNLOWER' specifies that
   alphabetical characters are normalized into lower cases, `TCUNNOACC' specifies that
   alphabetical characters with accent marks are normalized without accent marks, `TCUNWIDTH'
   specifies that full-width characters are normalized into half-width characters.
   The return value is the number of elements of the result array. */
int tcstrucsnorm(uint16_t *ary, int num, int opts);


/* Suspend execution of the current thread.
   `sec' specifies the interval of the suspension in seconds.
   If successful, the return value is true, else, it is false. */
bool tcsleep(double sec);


/* Get the current system information.
   The return value is a map object of the current system information or `NULL' on failure.
   The key "utime" indicates the user time of the CPU.  The key "stime" indicates the system time
   of the CPU.  The key "size" indicates the process size in bytes.  The "rss" indicates the
   resident set size in bytes.  "total" indicates the total size of the real memory.  "free"
   indicates the free size of the real memory.  "cached" indicates the cached size of the real
   memory.
   Because the object of the return value is created with the function `tcmapnew', it should be
   deleted with the function `tcmapdel' when it is no longer in use. */
TCMAP *tcsysinfo(void);


/*************************************************************************************************
 * filesystem utilities
 *************************************************************************************************/


/* Get the canonicalized absolute path of a file.
   `path' specifies the path of the file.
   The return value is the canonicalized absolute path of a file, or `NULL' if the path is
   invalid.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcrealpath(const char *path);


/* Get the status information of a file.
   `path' specifies the path of the file.
   `isdirp' specifies the pointer to a variable into which whether the file is a directory is
   assigned.  If it is `NULL', it is ignored.
   `sizep' specifies the pointer to a variable into which the size of the file is assigned.  If
   it is `NULL', it is ignored.
   `ntimep' specifies the pointer to a variable into which the size of the file is assigned.  If
   it is `NULL', it is ignored.
   If successful, the return value is true, else, it is false. */
bool tcstatfile(const char *path, bool *isdirp, int64_t *sizep, int64_t *mtimep);


/* Read whole data of a file.
   `path' specifies the path of the file.  If it is `NULL', the standard input is specified.
   `limit' specifies the limiting size of reading data.  If it is not more than 0, the limitation
   is not specified.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.  If it is `NULL', it is not used.
   The return value is the pointer to the allocated region of the read data, or `NULL' if the
   file could not be opened.
   Because an additional zero code is appended at the end of the region of the return value, the
   return value can be treated as a character string.  Because the region of the return value is
   allocated with the `malloc' call, it should be released with the `free' call when when is no
   longer in use.  */
void *tcreadfile(const char *path, int limit, int *sp);


/* Read every line of a file.
   `path' specifies the path of the file.  If it is `NULL', the standard input is specified.
   The return value is a list object of every lines if successful, else it is `NULL'.
   Line separators are cut out.  Because the object of the return value is created with the
   function `tclistnew', it should be deleted with the function `tclistdel' when it is no longer
   in use. */
TCLIST *tcreadfilelines(const char *path);


/* Write data into a file.
   `path' specifies the path of the file.  If it is `NULL', the standard output is specified.
   `ptr' specifies the pointer to the data region.
   `size' specifies the size of the region.
   If successful, the return value is true, else, it is false. */
bool tcwritefile(const char *path, const void *ptr, int size);


/* Copy a file.
   `src' specifies the path of the source file.
   `dest' specifies the path of the destination file.
   The return value is true if successful, else, it is false.
   If the destination file exists, it is overwritten. */
bool tccopyfile(const char *src, const char *dest);


/* Read names of files in a directory.
   `path' specifies the path of the directory.
   The return value is a list object of names if successful, else it is `NULL'.
   Links to the directory itself and to the parent directory are ignored.
   Because the object of the return value is created with the function `tclistnew', it should
   be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcreaddir(const char *path);


/* Expand a pattern into a list of matched paths.
   `pattern' specifies the matching pattern.
   The return value is a list object of matched paths.  If no path is matched, an empty list is
   returned.
   Because the object of the return value is created with the function `tclistnew', it should
   be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcglobpat(const char *pattern);


/* Remove a file or a directory and its sub ones recursively.
   `path' specifies the path of the link.
   If successful, the return value is true, else, it is false.  False is returned when the link
   does not exist or the permission is denied. */
bool tcremovelink(const char *path);


/* Write data into a file.
   `fd' specifies the file descriptor.
   `buf' specifies the buffer to be written.
   `size' specifies the size of the buffer.
   The return value is true if successful, else, it is false. */
bool tcwrite(int fd, const void *buf, size_t size);


/* Read data from a file.
   `fd' specifies the file descriptor.
   `buf' specifies the buffer to store into.
   `size' specifies the size of the buffer.
   The return value is true if successful, else, it is false. */
bool tcread(int fd, void *buf, size_t size);


/* Lock a file.
   `fd' specifies the file descriptor.
   `ex' specifies whether an exclusive lock or a shared lock is performed.
   `nb' specifies whether to request with non-blocking.
   The return value is true if successful, else, it is false. */
bool tclock(int fd, bool ex, bool nb);


/* Unlock a file.
   `fd' specifies the file descriptor.
   The return value is true if successful, else, it is false. */
bool tcunlock(int fd);


/* Execute a shell command.
   `args' specifies an array of the command name and its arguments.
   `anum' specifies the number of elements of the array.
   The return value is the exit code of the command or `INT_MAX' on failure.
   The command name and the arguments are quoted and meta characters are escaped. */
int tcsystem(const char **args, int anum);



/*************************************************************************************************
 * encoding utilities
 *************************************************************************************************/


/* Encode a serial object with URL encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   The return value is the result string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call if when is no longer in use. */
char *tcurlencode(const char *ptr, int size);


/* Decode a string encoded with URL encoding.
   `str' specifies the encoded string.
   `sp' specifies the pointer to a variable into which the size of the region of the return
   value is assigned.
   The return value is the pointer to the region of the result.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
char *tcurldecode(const char *str, int *sp);


/* Break up a URL into elements.
   `str' specifies the URL string.
   The return value is the map object whose keys are the name of elements.  The key "self"
   specifies the URL itself.  The key "scheme" indicates the scheme.  The key "host" indicates
   the host of the server.  The key "port" indicates the port number of the server.  The key
   "authority" indicates the authority information.  The key "path" indicates the path of the
   resource.  The key "file" indicates the file name without the directory section.  The key
   "query" indicates the query string.  The key "fragment" indicates the fragment string.
   Supported schema are HTTP, HTTPS, FTP, and FILE.  Absolute URL and relative URL are supported.
   Because the object of the return value is created with the function `tcmapnew', it should be
   deleted with the function `tcmapdel' when it is no longer in use. */
TCMAP *tcurlbreak(const char *str);


/* Encode a serial object with Base64 encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   The return value is the result string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call if when is no longer in use. */
char *tcbaseencode(const char *ptr, int size);


/* Decode a string encoded with Base64 encoding.
   `str' specifies the encoded string.
   `sp' specifies the pointer to a variable into which the size of the region of the return
   value is assigned.
   The return value is the pointer to the region of the result.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
char *tcbasedecode(const char *str, int *sp);


/* Encode a serial object with Quoted-printable encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   The return value is the result string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call if when is no longer in use. */
char *tcquoteencode(const char *ptr, int size);


/* Decode a string encoded with Quoted-printable encoding.
   `str' specifies the encoded string.
   `sp' specifies the pointer to a variable into which the size of the region of the return
   value is assigned.
   The return value is the pointer to the region of the result.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
char *tcquotedecode(const char *str, int *sp);


/* Split a string of MIME into headers and the body.
   `ptr' specifies the pointer to the region of MIME data.
   `size' specifies the size of the region.
   `headers' specifies a map object to store headers.  If it is `NULL', it is not used.  Each key
   of the map is an uncapitalized header name.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   The return value is the pointer to the region of the body data.
   If the content type is defined, the header map has the key "TYPE" specifying the type.  If the
   character encoding is defined, the key "CHARSET" indicates the encoding name.  If the boundary
   string of multipart is defined, the key "BOUNDARY" indicates the string.  If the content
   disposition is defined, the key "DISPOSITION" indicates the direction.  If the file name is
   defined, the key "FILENAME" indicates the name.  If the attribute name is defined, the key
   "NAME" indicates the name.  Because the region of the return value is allocated with the
   `malloc' call, it should be released with the `free' call when it is no longer in use. */
char *tcmimebreak(const char *ptr, int size, TCMAP *headers, int *sp);


/* Split multipart data of MIME into its parts.
   `ptr' specifies the pointer to the region of multipart data of MIME.
   `size' specifies the size of the region.
   `boundary' specifies the boundary string.
   The return value is a list object.  Each element of the list is the data of a part.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcmimeparts(const char *ptr, int size, const char *boundary);


/* Encode a serial object with hexadecimal encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   The return value is the result string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call if when is no longer in use. */
char *tchexencode(const char *ptr, int size);


/* Decode a string encoded with hexadecimal encoding.
   `str' specifies the encoded string.
   `sp' specifies the pointer to a variable into which the size of the region of the return
   value is assigned.
   The return value is the pointer to the region of the result.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
char *tchexdecode(const char *str, int *sp);


/* Compress a serial object with TCBS encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the result object, else, it is `NULL'.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcbsencode(const char *ptr, int size, int *sp);


/* Decompress a serial object compressed with TCBS encoding.
   `ptr' specifies the pointer to the region.
   `size' specifies the size of the region.
   `sp' specifies the pointer to a variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the result object, else, it is `NULL'.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when it
   is no longer in use. */
char *tcbsdecode(const char *ptr, int size, int *sp);


/* Escape meta characters in a string with the entity references of XML.
   `str' specifies the string.
   The return value is the pointer to the escaped string.
   This function escapes only `&', `<', `>', and `"'.  Because the region of the return value
   is allocated with the `malloc' call, it should be released with the `free' call when it is no
   longer in use. */
char *tcxmlescape(const char *str);



/*************************************************************************************************
 * encoding utilities (for experts)
 *************************************************************************************************/

/* Decode a data region in the x-www-form-urlencoded or multipart-form-data format.
   `ptr' specifies the pointer to the data region.
   `size' specifies the size of the data region.
   `type' specifies the value of the content-type header.  If it is `NULL', the type is specified
   as x-www-form-urlencoded.
   `params' specifies a map object into which the result parameters are stored. */
void tcwwwformdecode2(const void *ptr, int size, const char *type, TCMAP *params);


/* Escape meta characters in a string with backslash escaping of the C language.
   `str' specifies the string.
   The return value is the pointer to the escaped string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call if when is no longer in use. */
char *tccstrescape(const char *str);


/* Escape meta characters in a string with backslash escaping of JSON.
   `str' specifies the string.
   The return value is the pointer to the escaped string.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call if when is no longer in use. */
char *tcjsonescape(const char *str);

/* Store a list object into a map object with the type information.
   `map' specifies the container map object.
   `kstr' specifies the string of the key.
   `obj' specifies the list object to be stored. */
void tcmapputlist(TCMAP *map, const char *kstr, const TCLIST *obj);


/* Store a map object into a map object with the type information.
   `map' specifies the container map object.
   `kstr' specifies the string of the key.
   `obj' specifies the map object to be stored. */
void tcmapputmap(TCMAP *map, const char *kstr, const TCMAP *obj);


/*************************************************************************************************
 * bit operation utilities
 *************************************************************************************************/


typedef struct {                         /* type of structure for a bit stream object */
  uint8_t *sp;                           /* start pointer */
  uint8_t *cp;                           /* current pointer */
  int idx;                               /* bit index */
  int size;                              /* size of used region */
} TCBITSTRM;


/* Initialize a bit stream object as writer. */
void tcbitstrminitw(TCBITSTRM *bitstrm, char* obuf);


/* Concatenate a bit to a bit stream object. */
void tcbitstrmcat(TCBITSTRM *bitstrm, int sign);


/* Set the end mark to a bit stream object. */
void tcbitstrmsetend(TCBITSTRM *bitstrm);


/* Get the size of the used region of a bit stream object. */
int tcbitstrmsize(TCBITSTRM *bitstrm);


/* Initialize a bit stream object as reader. */
void tcbitstrminitr(TCBITSTRM *bitstrm, const char *ptr, int size);


/* Read a bit from a bit stream object. */
void tcbitstrmread(TCBITSTRM *bitstrm, int *sign);


/* Get the number of bits of a bit stream object. */
int tcbitstrmnum(TCBITSTRM *bitstrm);



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


#include <stdio.h>

#define _TC_LIBVER     911
#define _TC_FORMATVER  "1.0"

enum {                                   /* enumeration for error codes */
  TCESUCCESS,                            /* success */
  TCETHREAD,                             /* threading error */
  TCEINVALID,                            /* invalid operation */
  TCENOFILE,                             /* file not found */
  TCENOPERM,                             /* no permission */
  TCEMETA,                               /* invalid meta data */
  TCERHEAD,                              /* invalid record header */
  TCEOPEN,                               /* open error */
  TCECLOSE,                              /* close error */
  TCETRUNC,                              /* trunc error */
  TCESYNC,                               /* sync error */
  TCESTAT,                               /* stat error */
  TCESEEK,                               /* seek error */
  TCEREAD,                               /* read error */
  TCEWRITE,                              /* write error */
  TCEMMAP,                               /* mmap error */
  TCELOCK,                               /* lock error */
  TCEUNLINK,                             /* unlink error */
  TCERENAME,                             /* rename error */
  TCEMKDIR,                              /* mkdir error */
  TCERMDIR,                              /* rmdir error */
  TCEKEEP,                               /* existing record */
  TCENOREC,                              /* no record found */
  TCEMISC = 9999                         /* miscellaneous error */
};


/* Get the message string corresponding to an error code.
   `ecode' specifies the error code.
   The return value is the message string of the error code. */
const char *tcerrmsg(int ecode);


/* Show error message on the standard error output and exit.
   `message' specifies an error message.
   This function does not return. */
void *tcmyfatal(const char *message);


/* Allocate a large nullified region.
   `size' specifies the size of the region.
   The return value is the pointer to the allocated nullified region.
   This function handles failure of memory allocation implicitly.  The region of the return value
   should be released with the function `tczerounmap' when it is no longer in use. */
void *tczeromap(uint64_t size);


/* Free a large nullfied region.
   `ptr' specifies the pointer to the region. */
void tczerounmap(void *ptr);


/* Lock the absolute path of a file.
   `path' specifies the path of the file.
   If successful, the return value is true, else, it is false. */
bool tcpathlock(const char *path);


/* Unock the absolute path of a file.
   `path' specifies the path of the file.
   If successful, the return value is true, else, it is false. */
bool tcpathunlock(const char *path);


/* Convert an integer to the string as binary numbers.
   `num' specifies the integer.
   `buf' specifies the pointer to the region into which the result string is written.  The size
   of the buffer should be equal to or more than 65 bytes.
   `col' specifies the number of columns.  If it is not more than 0, it depends on the integer.
   `fc' specifies the filling character.
   The return value is the length of the result string. */
int tcnumtostrbin(uint64_t num, char *buf, int col, int fc);


/* Get the aligned offset of a file offset.
   `off' specifies the file offset.
   The return value is the aligned offset. */
uint64_t tcpagealign(uint64_t off);


/* Get the alignment of a variable type. */
#define TCALIGNOF(TC_a) \
  ((int)offsetof(struct { int8_t TC_top; TC_a TC_bot; }, TC_bot))


/* Get the size of padding bytes for pointer alignment. */
typedef union { int32_t i; int64_t l; double d; void *p; TCCMP f; } tcgeneric_t;
#define TCALIGNPAD(TC_hsiz) \
  (((TC_hsiz | ~-TCALIGNOF(tcgeneric_t)) + 1) - TC_hsiz)



/*************************************************************************************************
 * for ZLIB
 *************************************************************************************************/


enum {
  _TCZMZLIB,
  _TCZMRAW,
  _TCZMGZIP
};


extern char *(*_tc_deflate)(const char *, int, int *, int);
extern char *(*_tc_inflate)(const char *, int, int *, int);
extern unsigned int (*_tc_getcrc)(const char *, int);


extern char *(*_tc_bzcompress)(const char *, int, int *);
extern char *(*_tc_bzdecompress)(const char *, int, int *);


/* set a buffer for a variable length number */
void tcsetvnumbuf32 (uint32_t TC_num, int* TC_len, char* TC_buf);
void tcsetvnumbuf64 (uint64_t TC_num, int* TC_len, char* TC_buf);


/* read a variable length buffer */
void tcreadvnumbuf32 (const char* TC_buf, int TC_step, uint32_t* TC_num);
void tcreadvnumbuf64 (const char* TC_buf, int TC_step, uint64_t* TC_num);


__TCUTIL_CLINKAGEEND
#endif                                   /* duplication check */


/* END OF FILE */
