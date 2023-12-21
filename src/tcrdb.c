/*************************************************************************************************
 * The remote database API of Tokyo Tyrant
 *                                                               Copyright (C) 2006-2010 FAL Labs
 * This file is part of Tokyo Tyrant.
 * Tokyo Tyrant is free software; you can redistribute it and/or modify it under the terms of
 * the GNU Lesser General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License or any later version.  Tokyo Tyrant is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * You should have received a copy of the GNU Lesser General Public License along with Tokyo
 * Tyrant; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA.
 *************************************************************************************************/


#include "tcutil.h"
#include "tcadb.h"
#include "ttutil.h"
#include "tcrdb.h"
#include "conf.h"

#define RDBRECONWAIT   0.1               // wait time to reconnect
#define RDBNUMCOLMAX   16                // maximum number of columns of the long double

typedef struct {                         // type of structure for a sort record
  const char *cbuf;                      // pointer to the column buffer
  int csiz;                              // size of the column buffer
  char *obuf;                            // pointer to the sort key
  int osiz;                              // size of the sort key
} RDBSORTREC;


/* private function prototypes */
static bool tcrdblockmethod(TCRDB *rdb);
static void tcrdbunlockmethod(TCRDB *rdb);
static bool tcrdbreconnect(TCRDB *rdb);
static bool tcrdbsend(TCRDB *rdb, const void *buf, int size);
static bool tcrdbtuneimpl(TCRDB *rdb, double timeout, int opts);
static bool tcrdbopenimpl(TCRDB *rdb, const char *host, int port);
static bool tcrdbcloseimpl(TCRDB *rdb);
static bool tcrdbputimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);
static bool tcrdbputkeepimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);
static bool tcrdbputcatimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);
static bool tcrdbputshlimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz,
                            int width);
static bool tcrdbputnrimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);
static bool tcrdboutimpl(TCRDB *rdb, const void *kbuf, int ksiz);
static void *tcrdbgetimpl(TCRDB *rdb, const void *kbuf, int ksiz, int *sp);
static bool tcrdbmgetimpl(TCRDB *rdb, TCMAP *recs);
static int tcrdbvsizimpl(TCRDB *rdb, const void *kbuf, int ksiz);
static bool tcrdbiterinitimpl(TCRDB *rdb);
static void *tcrdbiternextimpl(TCRDB *rdb, int *sp);
static TCLIST *tcrdbfwmkeysimpl(TCRDB *rdb, const void *pbuf, int psiz, int max);
static int tcrdbaddintimpl(TCRDB *rdb, const void *kbuf, int ksiz, int num);
static double tcrdbadddoubleimpl(TCRDB *rdb, const void *kbuf, int ksiz, double num);
static void *tcrdbextimpl(TCRDB *rdb, const char *name, int opts,
                          const void *kbuf, int ksiz, const void *vbuf, int vsiz, int *sp);
static bool tcrdbsyncimpl(TCRDB *rdb);
static bool tcrdboptimizeimpl(TCRDB *rdb, const char *params);
static bool tcrdbvanishimpl(TCRDB *rdb);
static bool tcrdbcopyimpl(TCRDB *rdb, const char *path);
static bool tcrdbrestoreimpl(TCRDB *rdb, const char *path, uint64_t ts, int opts);
static bool tcrdbsetmstimpl(TCRDB *rdb, const char *host, int port, uint64_t ts, int opts);
const char *tcrdbexprimpl(TCRDB *rdb);
static uint64_t tcrdbrnumimpl(TCRDB *rdb);
static uint64_t tcrdbsizeimpl(TCRDB *rdb);
static char *tcrdbstatimpl(TCRDB *rdb);
static TCLIST *tcrdbmiscimpl(TCRDB *rdb, const char *name, int opts, const TCLIST *args);



/*************************************************************************************************
 * API
 *************************************************************************************************/


/* Get the message string corresponding to an error code. */
const char *tcrdberrmsg(int ecode){
  switch(ecode){
    case TTESUCCESS: return "success";
    case TTEINVALID: return "invalid operation";
    case TTENOHOST: return "host not found";
    case TTEREFUSED: return "connection refused";
    case TTESEND: return "send error";
    case TTERECV: return "recv error";
    case TTEKEEP: return "existing record";
    case TTENOREC: return "no record found";
    case TTEMISC: return "miscellaneous error";
  }
  return "unknown error";
}


/* Create a remote database object. */
TCRDB *tcrdbnew(void){
  TCRDB *rdb = tcmalloc(sizeof(*rdb));
  if(pthread_mutex_init(&rdb->mmtx, NULL) != 0) tcmyfatal("pthread_mutex_init failed");
  if(pthread_key_create(&rdb->eckey, NULL) != 0) tcmyfatal("pthread_key_create failed");
  rdb->host = NULL;
  rdb->port = -1;
  rdb->expr = NULL;
  rdb->fd = -1;
  rdb->sock = NULL;
  rdb->timeout = UINT_MAX;
  rdb->opts = 0;
  tcrdbsetecode(rdb, TTESUCCESS);
  return rdb;
}


/* Delete a remote database object. */
void tcrdbdel(TCRDB *rdb){
  assert(rdb);
  if(rdb->fd >= 0) tcrdbclose(rdb);
  if(rdb->expr) tcfree(rdb->expr);
  if(rdb->host) tcfree(rdb->host);
  pthread_key_delete(rdb->eckey);
  pthread_mutex_destroy(&rdb->mmtx);
  tcfree(rdb);
}


/* Get the last happened error code of a remote database object. */
int tcrdbecode(TCRDB *rdb){
  assert(rdb);
  return (int)(intptr_t)pthread_getspecific(rdb->eckey);
}


/* Set the tuning parameters of a remote database object. */
bool tcrdbtune(TCRDB *rdb, double timeout, int opts){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbtuneimpl(rdb, timeout, opts);
  pthread_cleanup_pop(1);
  return rv;
}


/* Open a remote database. */
bool tcrdbopen(TCRDB *rdb, const char *host, int port){
  assert(rdb && host);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbopenimpl(rdb, host, port);
  pthread_cleanup_pop(1);
  return rv;
}


/* Open a remote database with a simple server expression. */
bool tcrdbopen2(TCRDB *rdb, const char *expr){
  assert(rdb && expr);
  bool err = false;
  int port;
  char *host = ttbreakservexpr(expr, &port);
  char *pv = strchr(expr, '#');
  double tout = 0.0;
  if(pv){
    TCLIST *elems = tcstrsplit(pv + 1, "#");
    int ln = tclistnum(elems);
    for(int i = 0; i < ln; i++){
      const char *elem = TCLISTVALPTR(elems, i);
      pv = strchr(elem, '=');
      if(!pv) continue;
      *(pv++) = '\0';
      if(!tcstricmp(elem, "host") || !tcstricmp(elem, "name")){
        tcfree(host);
        host = ttbreakservexpr(pv, NULL);
      } else if(!tcstricmp(elem, "port")){
        port = tcatoi(pv);
      } else if(!tcstricmp(elem, "tout") || !tcstricmp(elem, "timeout")){
        tout = tcatof(pv);
      }
    }
    tclistdel(elems);
  }
  if(tout > 0) tcrdbtune(rdb, tout, RDBTRECON);
  if(!tcrdbopen(rdb, host, port)) err = true;
  tcfree(host);
  return !err;
}


/* Close a remote database object. */
bool tcrdbclose(TCRDB *rdb){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbcloseimpl(rdb);
  pthread_cleanup_pop(1);
  return rv;
}


/* Store a record into a remote database object. */
bool tcrdbput(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbputimpl(rdb, kbuf, ksiz, vbuf, vsiz);
  pthread_cleanup_pop(1);
  return rv;
}


/* Store a new record into a remote database object. */
bool tcrdbputkeep(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbputkeepimpl(rdb, kbuf, ksiz, vbuf, vsiz);
  pthread_cleanup_pop(1);
  return rv;
}


/* Concatenate a value at the end of the existing record in a remote database object. */
bool tcrdbputcat(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbputcatimpl(rdb, kbuf, ksiz, vbuf, vsiz);
  pthread_cleanup_pop(1);
  return rv;
}


/* Concatenate a value at the end of the existing record and shift it to the left. */
bool tcrdbputshl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz, int width){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0 && width >= 0);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbputshlimpl(rdb, kbuf, ksiz, vbuf, vsiz, width);
  pthread_cleanup_pop(1);
  return rv;
}


/* Store a record into a remote database object without repsponse from the server. */
bool tcrdbputnr(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbputnrimpl(rdb, kbuf, ksiz, vbuf, vsiz);
  pthread_cleanup_pop(1);
  return rv;
}


/* Store a string record into a remote object without response from the server. */
bool tcrdbputnr2(TCRDB *rdb, const char *kstr, const char *vstr){
  assert(rdb && kstr && vstr);
  return tcrdbputnr(rdb, kstr, strlen(kstr), vstr, strlen(vstr));
}


/* Remove a record of a remote database object. */
bool tcrdbout(TCRDB *rdb, const void *kbuf, int ksiz){
  assert(rdb && kbuf && ksiz >= 0);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdboutimpl(rdb, kbuf, ksiz);
  pthread_cleanup_pop(1);
  return rv;
}


/* Retrieve a record in a remote database object. */
void *tcrdbget(TCRDB *rdb, const void *kbuf, int ksiz, int *sp){
  assert(rdb && kbuf && ksiz >= 0 && sp);
  if(!tcrdblockmethod(rdb)) return NULL;
  void *rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbgetimpl(rdb, kbuf, ksiz, sp);
  pthread_cleanup_pop(1);
  return rv;
}


/* Retrieve records in a remote database object. */
bool tcrdbget3(TCRDB *rdb, TCMAP *recs){
  assert(rdb && recs);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbmgetimpl(rdb, recs);
  pthread_cleanup_pop(1);
  return rv;
}


/* Get the size of the value of a record in a remote database object. */
int tcrdbvsiz(TCRDB *rdb, const void *kbuf, int ksiz){
  assert(rdb && kbuf && ksiz >= 0);
  if(!tcrdblockmethod(rdb)) return -1;
  int rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbvsizimpl(rdb, kbuf, ksiz);
  pthread_cleanup_pop(1);
  return rv;
}


/* Initialize the iterator of a remote database object. */
bool tcrdbiterinit(TCRDB *rdb){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbiterinitimpl(rdb);
  pthread_cleanup_pop(1);
  return rv;
}


/* Get the next key of the iterator of a remote database object. */
void *tcrdbiternext(TCRDB *rdb, int *sp){
  assert(rdb && sp);
  if(!tcrdblockmethod(rdb)) return NULL;
  void *rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbiternextimpl(rdb, sp);
  pthread_cleanup_pop(1);
  return rv;
}


/* Get forward matching keys in a remote database object. */
TCLIST *tcrdbfwmkeys(TCRDB *rdb, const void *pbuf, int psiz, int max){
  assert(rdb && pbuf && psiz >= 0);
  if(!tcrdblockmethod(rdb)) return tclistnew2(1);
  TCLIST *rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbfwmkeysimpl(rdb, pbuf, psiz, max);
  pthread_cleanup_pop(1);
  return rv;
}


/* Get forward matching string keys in a remote database object. */
TCLIST *tcrdbfwmkeys2(TCRDB *rdb, const char *pstr, int max){
  assert(rdb && pstr);
  return tcrdbfwmkeys(rdb, pstr, strlen(pstr), max);
}


/* Add an integer to a record in a remote database object. */
int tcrdbaddint(TCRDB *rdb, const void *kbuf, int ksiz, int num){
  assert(rdb && kbuf && ksiz >= 0);
  if(!tcrdblockmethod(rdb)) return INT_MIN;
  int rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbaddintimpl(rdb, kbuf, ksiz, num);
  pthread_cleanup_pop(1);
  return rv;
}


/* Add a real number to a record in a remote database object. */
double tcrdbadddouble(TCRDB *rdb, const void *kbuf, int ksiz, double num){
  assert(rdb && kbuf && ksiz >= 0);
  if(!tcrdblockmethod(rdb)) return nan("");
  double rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbadddoubleimpl(rdb, kbuf, ksiz, num);
  pthread_cleanup_pop(1);
  return rv;
}


/* Call a function of the scripting language extension. */
void *tcrdbext(TCRDB *rdb, const char *name, int opts,
               const void *kbuf, int ksiz, const void *vbuf, int vsiz, int *sp){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(!tcrdblockmethod(rdb)) return NULL;
  void *rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbextimpl(rdb, name, opts, kbuf, ksiz, vbuf, vsiz, sp);
  pthread_cleanup_pop(1);
  return rv;
}


/* Synchronize updated contents of a remote database object with the file and the device. */
bool tcrdbsync(TCRDB *rdb){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbsyncimpl(rdb);
  pthread_cleanup_pop(1);
  return rv;
}


/* Optimize the storage of a remove database object. */
bool tcrdboptimize(TCRDB *rdb, const char *params){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdboptimizeimpl(rdb, params);
  pthread_cleanup_pop(1);
  return rv;
}


/* Remove all records of a remote database object. */
bool tcrdbvanish(TCRDB *rdb){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbvanishimpl(rdb);
  pthread_cleanup_pop(1);
  return rv;
}


/* Copy the database file of a remote database object. */
bool tcrdbcopy(TCRDB *rdb, const char *path){
  assert(rdb && path);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbcopyimpl(rdb, path);
  pthread_cleanup_pop(1);
  return rv;
}


/* Restore the database file of a remote database object from the update log. */
bool tcrdbrestore(TCRDB *rdb, const char *path, uint64_t ts, int opts){
  assert(rdb && path);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbrestoreimpl(rdb, path, ts, opts);
  pthread_cleanup_pop(1);
  return rv;
}


/* Set the replication master of a remote database object from the update log. */
bool tcrdbsetmst(TCRDB *rdb, const char *host, int port, uint64_t ts, int opts){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return false;
  bool rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbsetmstimpl(rdb, host, port, ts, opts);
  pthread_cleanup_pop(1);
  return rv;
}


/* Set the replication master of a remote database object with a simple server expression. */
bool tcrdbsetmst2(TCRDB *rdb, const char *expr, uint64_t ts, int opts){
  assert(rdb && expr);
  bool err = false;
  int port;
  char *host = ttbreakservexpr(expr, &port);
  if(!tcrdbsetmst(rdb, host, port, ts, opts)) err = true;
  tcfree(host);
  return !err;
}


/* Get the simple server expression of an abstract database object. */
const char *tcrdbexpr(TCRDB *rdb){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return NULL;
  const char *rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbexprimpl(rdb);
  pthread_cleanup_pop(1);
  return rv;
}


/* Get the number of records of a remote database object. */
uint64_t tcrdbrnum(TCRDB *rdb){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return 0;
  uint64_t rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbrnumimpl(rdb);
  pthread_cleanup_pop(1);
  return rv;
}


/* Get the size of the database of a remote database object. */
uint64_t tcrdbsize(TCRDB *rdb){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return 0;
  uint64_t rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbsizeimpl(rdb);
  pthread_cleanup_pop(1);
  return rv;
}


/* Get the status string of the database of a remote database object. */
char *tcrdbstat(TCRDB *rdb){
  assert(rdb);
  if(!tcrdblockmethod(rdb)) return NULL;
  char *rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbstatimpl(rdb);
  pthread_cleanup_pop(1);
  return rv;
}


/* Call a versatile function for miscellaneous operations of a remote database object. */
TCLIST *tcrdbmisc(TCRDB *rdb, const char *name, int opts, const TCLIST *args){
  assert(rdb && name && args);
  if(!tcrdblockmethod(rdb)) return NULL;
  TCLIST *rv;
  pthread_cleanup_push((void (*)(void *))tcrdbunlockmethod, rdb);
  rv = tcrdbmiscimpl(rdb, name, opts, args);
  pthread_cleanup_pop(1);
  return rv;
}

/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


/* Set the error code of a remote database object. */
void tcrdbsetecode(TCRDB *rdb, int ecode){
  assert(rdb);
  pthread_setspecific(rdb->eckey, (void *)(intptr_t)ecode);
}



/*************************************************************************************************
 * private features
 *************************************************************************************************/


/* Lock a method of the remote database object.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
static bool tcrdblockmethod(TCRDB *rdb){
  assert(rdb);
  if(pthread_mutex_lock(&rdb->mmtx) != 0){
    tcrdbsetecode(rdb, TCEMISC);
    return false;
  }
  return true;
}


/* Unlock a method of the remote database object.
   `rdb' specifies the remote database object. */
static void tcrdbunlockmethod(TCRDB *rdb){
  assert(rdb);
  if(pthread_mutex_unlock(&rdb->mmtx) != 0) tcrdbsetecode(rdb, TCEMISC);
}


/* Reconnect a remote database.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
static bool tcrdbreconnect(TCRDB *rdb){
  assert(rdb);
  if(rdb->sock){
    ttsockdel(rdb->sock);
    ttclosesock(rdb->fd);
    rdb->fd = -1;
    rdb->sock = NULL;
  }
  int fd;
  if(rdb->port < 1){
    fd = ttopensockunix(rdb->host);
  } else {
    char addr[TTADDRBUFSIZ];
    if(!ttgethostaddr(rdb->host, addr)){
      tcrdbsetecode(rdb, TTENOHOST);
      return false;
    }
    fd = ttopensock(addr, rdb->port);
  }
  if(fd == -1){
    tcrdbsetecode(rdb, TTEREFUSED);
    return false;
  }
  rdb->fd = fd;
  rdb->sock = ttsocknew(fd);
  return true;
}


/* Send data of a remote database object.
   `rdb' specifies the remote database object.
   `buf' specifies the pointer to the region of the data to send.
   `size' specifies the size of the buffer.
   If successful, the return value is true, else, it is false. */
static bool tcrdbsend(TCRDB *rdb, const void *buf, int size){
  assert(rdb && buf && size >= 0);
  if(ttsockcheckend(rdb->sock)){
    if(!(rdb->opts & RDBTRECON)) return false;
    tcsleep(RDBRECONWAIT);
    if(!tcrdbreconnect(rdb)) return false;
    if(ttsocksend(rdb->sock, buf, size)) return true;
    tcrdbsetecode(rdb, TTESEND);
    return false;
  }
  ttsocksetlife(rdb->sock, rdb->timeout);
  if(ttsocksend(rdb->sock, buf, size)) return true;
  tcrdbsetecode(rdb, TTESEND);
  if(!(rdb->opts & RDBTRECON)) return false;
  tcsleep(RDBRECONWAIT);
  if(!tcrdbreconnect(rdb)) return false;
  ttsocksetlife(rdb->sock, rdb->timeout);
  if(ttsocksend(rdb->sock, buf, size)) return true;
  tcrdbsetecode(rdb, TTESEND);
  return false;
}


/* Set the tuning parameters of a remote database object.
   `rdb' specifies the remote database object.
   `timeout' specifies the timeout of each query in seconds.
   `opts' specifies options by bitwise-or.
   If successful, the return value is true, else, it is false. */
static bool tcrdbtuneimpl(TCRDB *rdb, double timeout, int opts){
  assert(rdb);
  if(rdb->fd >= 0){
    tcrdbsetecode(rdb, TTEINVALID);
    return false;
  }
  rdb->timeout = (timeout > 0.0) ? timeout : UINT_MAX;
  rdb->opts = opts;
  return true;
}


/* Open a remote database.
   `rdb' specifies the remote database object.
   `host' specifies the name or the address of the server.
   `port' specifies the port number.
   If successful, the return value is true, else, it is false. */
static bool tcrdbopenimpl(TCRDB *rdb, const char *host, int port){
  assert(rdb && host);
  if(rdb->fd >= 0){
    tcrdbsetecode(rdb, TTEINVALID);
    return false;
  }
  int fd;
  if(port < 1){
    fd = ttopensockunix(host);
  } else {
    char addr[TTADDRBUFSIZ];
    if(!ttgethostaddr(host, addr)){
      tcrdbsetecode(rdb, TTENOHOST);
      return false;
    }
    fd = ttopensock(addr, port);
  }
  if(fd == -1){
    tcrdbsetecode(rdb, TTEREFUSED);
    return false;
  }
  if(rdb->host) tcfree(rdb->host);
  rdb->host = tcstrdup(host);
  rdb->port = port;
  rdb->expr = tcsprintf("%s:%d", host, port);
  rdb->fd = fd;
  rdb->sock = ttsocknew(fd);
  return true;
}


/* Close a remote database object.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
static bool tcrdbcloseimpl(TCRDB *rdb){
  assert(rdb);
  if(rdb->fd < 0){
    tcrdbsetecode(rdb, TTEINVALID);
    return false;
  }
  bool err = false;
  ttsockdel(rdb->sock);
  if(!ttclosesock(rdb->fd)){
    tcrdbsetecode(rdb, TTEMISC);
    err = true;
  }
  tcfree(rdb->expr);
  tcfree(rdb->host);
  rdb->expr = NULL;
  rdb->host = NULL;
  rdb->port = -1;
  rdb->fd = -1;
  rdb->sock = NULL;
  return !err;
}


/* Store a record into a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false. */
static bool tcrdbputimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  int rsiz = 2 + sizeof(uint32_t) * 2 + ksiz + vsiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDPUT;
  uint32_t num;
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)vsiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  memcpy(wp, vbuf, vsiz);
  wp += vsiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Store a new record into a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false. */
static bool tcrdbputkeepimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  int rsiz = 2 + sizeof(uint32_t) * 2 + ksiz + vsiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDPUTKEEP;
  uint32_t num;
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)vsiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  memcpy(wp, vbuf, vsiz);
  wp += vsiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEKEEP);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Concatenate a value at the end of the existing record in a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false. */
static bool tcrdbputcatimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  int rsiz = 2 + sizeof(uint32_t) * 2 + ksiz + vsiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDPUTCAT;
  uint32_t num;
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)vsiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  memcpy(wp, vbuf, vsiz);
  wp += vsiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Concatenate a value at the end of the existing record and shift it to the left.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `width' specifies the width of the record.
   If successful, the return value is true, else, it is false. */
static bool tcrdbputshlimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz,
                            int width){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0 && width >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  int rsiz = 2 + sizeof(uint32_t) * 3 + ksiz + vsiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDPUTSHL;
  uint32_t num;
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)vsiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)width);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  memcpy(wp, vbuf, vsiz);
  wp += vsiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Store a record into a remote database object without response from the server.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false. */
static bool tcrdbputnrimpl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  int rsiz = 2 + sizeof(uint32_t) * 2 + ksiz + vsiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDPUTNR;
  uint32_t num;
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)vsiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  memcpy(wp, vbuf, vsiz);
  wp += vsiz;
  if(!tcrdbsend(rdb, buf, wp - buf)) err = true;
  pthread_cleanup_pop(1);
  return !err;
}


/* Remove a record of a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false. */
static bool tcrdboutimpl(TCRDB *rdb, const void *kbuf, int ksiz){
  assert(rdb && kbuf && ksiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  int rsiz = 2 + sizeof(uint32_t) + ksiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDOUT;
  uint32_t num;
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTENOREC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Retrieve a record in a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the corresponding
   record. */
static void *tcrdbgetimpl(TCRDB *rdb, const void *kbuf, int ksiz, int *sp){
  assert(rdb && kbuf && ksiz >= 0 && sp);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return NULL;
    }
    if(!tcrdbreconnect(rdb)) return NULL;
  }
  char *vbuf = NULL;
  int rsiz = 2 + sizeof(uint32_t) + ksiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDGET;
  uint32_t num;
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      int vsiz = ttsockgetint32(rdb->sock);
      if(!ttsockcheckend(rdb->sock) && vsiz >= 0){
        vbuf = tcmalloc(vsiz + 1);
        if(ttsockrecv(rdb->sock, vbuf, vsiz)){
          vbuf[vsiz] = '\0';
          *sp = vsiz;
        } else {
          tcrdbsetecode(rdb, TTERECV);
          tcfree(vbuf);
          vbuf = NULL;
        }
      } else {
        tcrdbsetecode(rdb, TTERECV);
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTENOREC);
    }
  }
  pthread_cleanup_pop(1);
  return vbuf;
}


/* Retrieve records in a remote database object.
   `rdb' specifies the remote database object.
   `recs' specifies a map object containing the retrieval keys.
   If successful, the return value is true, else, it is false. */
static bool tcrdbmgetimpl(TCRDB *rdb, TCMAP *recs){
  assert(rdb && recs);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  TCXSTR *xstr = tcxstrnew();
  pthread_cleanup_push((void (*)(void *))tcxstrdel, xstr);
  uint8_t magic[2];
  magic[0] = TTMAGICNUM;
  magic[1] = TTCMDMGET;
  tcxstrcat(xstr, magic, sizeof(magic));
  uint32_t num;
  num = (uint32_t)tcmaprnum(recs);
  num = htonl(num);
  tcxstrcat(xstr, &num, sizeof(num));
  tcmapiterinit(recs);
  const char *kbuf;
  int ksiz;
  while((kbuf = tcmapiternext(recs, &ksiz)) != NULL){
    num = htonl((uint32_t)ksiz);
    tcxstrcat(xstr, &num, sizeof(num));
    tcxstrcat(xstr, kbuf, ksiz);
  }
  tcmapclear(recs);
  char stack[TTIOBUFSIZ];
  if(tcrdbsend(rdb, tcxstrptr(xstr), tcxstrsize(xstr))){
    int code = ttsockgetc(rdb->sock);
    int rnum = ttsockgetint32(rdb->sock);
    if(code == 0){
      if(!ttsockcheckend(rdb->sock) && rnum >= 0){
        for(int i = 0; i < rnum; i++){
          int rksiz = ttsockgetint32(rdb->sock);
          int rvsiz = ttsockgetint32(rdb->sock);
          if(ttsockcheckend(rdb->sock)){
            tcrdbsetecode(rdb, TTERECV);
            err = true;
            break;
          }
          int rsiz = rksiz + rvsiz;
          char *rbuf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz + 1);
          if(ttsockrecv(rdb->sock, rbuf, rsiz)){
            tcmapput(recs, rbuf, rksiz, rbuf + rksiz, rvsiz);
          } else {
            tcrdbsetecode(rdb, TTERECV);
            err = true;
          }
          if(rbuf != stack) tcfree(rbuf);
        }
      } else {
        tcrdbsetecode(rdb, TTERECV);
        err = true;
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTENOREC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Get the size of the value of a record in a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1. */
static int tcrdbvsizimpl(TCRDB *rdb, const void *kbuf, int ksiz){
  assert(rdb && kbuf && ksiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return -1;
    }
    if(!tcrdbreconnect(rdb)) return -1;
  }
  int vsiz = -1;
  int rsiz = 2 + sizeof(uint32_t) + ksiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDVSIZ;
  uint32_t num;
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      vsiz = ttsockgetint32(rdb->sock);
      if(ttsockcheckend(rdb->sock)){
        tcrdbsetecode(rdb, TTERECV);
        vsiz = -1;
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTENOREC);
    }
  }
  pthread_cleanup_pop(1);
  return vsiz;
}


/* Initialize the iterator of a remote database object.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
static bool tcrdbiterinitimpl(TCRDB *rdb){
  assert(rdb);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  unsigned char buf[TTIOBUFSIZ];
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDITERINIT;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  return !err;
}


/* Get the next key of the iterator of a remote database object.
   `rdb' specifies the remote database object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'. */
static void *tcrdbiternextimpl(TCRDB *rdb, int *sp){
  assert(rdb && sp);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return NULL;
    }
    if(!tcrdbreconnect(rdb)) return NULL;
  }
  char *vbuf = NULL;
  unsigned char buf[TTIOBUFSIZ];
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDITERNEXT;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      int vsiz = ttsockgetint32(rdb->sock);
      if(!ttsockcheckend(rdb->sock) && vsiz >= 0){
        vbuf = tcmalloc(vsiz + 1);
        if(ttsockrecv(rdb->sock, vbuf, vsiz)){
          vbuf[vsiz] = '\0';
          *sp = vsiz;
        } else {
          tcrdbsetecode(rdb, TTERECV);
          tcfree(vbuf);
          vbuf = NULL;
        }
      } else {
        tcrdbsetecode(rdb, TTERECV);
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTENOREC);
    }
  }
  return vbuf;
}


/* Get forward matching keys in a remote database object.
   `rdb' specifies the remote database object.
   `pbuf' specifies the pointer to the region of the prefix.
   `psiz' specifies the size of the region of the prefix.
   `max' specifies the maximum number of keys to be fetched.
   The return value is a list object of the corresponding keys. */
static TCLIST *tcrdbfwmkeysimpl(TCRDB *rdb, const void *pbuf, int psiz, int max){
  assert(rdb && pbuf && psiz >= 0);
  TCLIST *keys = tclistnew();
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return NULL;
    }
    if(!tcrdbreconnect(rdb)) return NULL;
  }
  int rsiz = 2 + sizeof(uint32_t) * 2 + psiz;
  if(max < 0) max = INT_MAX;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDFWMKEYS;
  uint32_t num;
  num = htonl((uint32_t)psiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)max);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, pbuf, psiz);
  wp += psiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      int knum = ttsockgetint32(rdb->sock);
      if(!ttsockcheckend(rdb->sock) && knum >= 0){
        for(int i = 0; i < knum; i++){
          int ksiz = ttsockgetint32(rdb->sock);
          if(ttsockcheckend(rdb->sock)){
            tcrdbsetecode(rdb, TTERECV);
            break;
          }
          char *kbuf = (ksiz < TTIOBUFSIZ) ? stack : tcmalloc(ksiz + 1);
          if(ttsockrecv(rdb->sock, kbuf, ksiz)){
            tclistpush(keys, kbuf, ksiz);
          } else {
            tcrdbsetecode(rdb, TTERECV);
          }
          if(kbuf != (char *)stack) tcfree(kbuf);
        }
      } else {
        tcrdbsetecode(rdb, TTERECV);
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTENOREC);
    }
  }
  pthread_cleanup_pop(1);
  return keys;
}


/* Add an integer to a record in a remote database object.
   `rdb' specifies the remote database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is `INT_MIN'. */
static int tcrdbaddintimpl(TCRDB *rdb, const void *kbuf, int ksiz, int num){
  assert(rdb && kbuf && ksiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return INT_MIN;
    }
    if(!tcrdbreconnect(rdb)) return INT_MIN;
  }
  int sum = INT_MIN;
  int rsiz = 2 + sizeof(uint32_t) * 2 + ksiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDADDINT;
  uint32_t lnum;
  lnum = htonl((uint32_t)ksiz);
  memcpy(wp, &lnum, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  lnum = htonl((uint32_t)num);
  memcpy(wp, &lnum, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      sum = ttsockgetint32(rdb->sock);
      if(ttsockcheckend(rdb->sock)){
        tcrdbsetecode(rdb, TTERECV);
        sum = -1;
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEKEEP);
    }
  }
  pthread_cleanup_pop(1);
  return sum;
}


/* Add a real number to a record in a remote database object.
   `rdb' specifies the remote database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is Not-a-Number. */
static double tcrdbadddoubleimpl(TCRDB *rdb, const void *kbuf, int ksiz, double num){
  assert(rdb && kbuf && ksiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return nan("");
    }
    if(!tcrdbreconnect(rdb)) return nan("");
  }
  double sum = nan("");
  int rsiz = 2 + sizeof(uint32_t) + sizeof(uint64_t) * 2 + ksiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDADDDOUBLE;
  uint32_t lnum;
  lnum = htonl((uint32_t)ksiz);
  memcpy(wp, &lnum, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  char dbuf[sizeof(uint64_t)*2];
  ttpackdouble(num, (char *)wp);
  wp += sizeof(dbuf);
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      if(ttsockrecv(rdb->sock, dbuf, sizeof(dbuf)) && !ttsockcheckend(rdb->sock)){
        sum = ttunpackdouble(dbuf);
      } else {
        tcrdbsetecode(rdb, TTERECV);
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEKEEP);
    }
  }
  pthread_cleanup_pop(1);
  return sum;
}


/* Call a function of the scripting language extension.
   `rdb' specifies the remote database object.
   `name' specifies the function name.
   `opts' specifies options by bitwise-or.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the response. */
static void *tcrdbextimpl(TCRDB *rdb, const char *name, int opts,
                          const void *kbuf, int ksiz, const void *vbuf, int vsiz, int *sp){
  assert(rdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return NULL;
    }
    if(!tcrdbreconnect(rdb)) return NULL;
  }
  char *xbuf = NULL;
  int nsiz = strlen(name);
  int rsiz = 2 + sizeof(uint32_t) * 4 + nsiz + ksiz + vsiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDEXT;
  uint32_t num;
  num = htonl((uint32_t)nsiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)opts);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)ksiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  num = htonl((uint32_t)vsiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, name, nsiz);
  wp += nsiz;
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  memcpy(wp, vbuf, vsiz);
  wp += vsiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      int xsiz = ttsockgetint32(rdb->sock);
      if(!ttsockcheckend(rdb->sock) && xsiz >= 0){
        xbuf = tcmalloc(xsiz + 1);
        if(ttsockrecv(rdb->sock, xbuf, xsiz)){
          xbuf[xsiz] = '\0';
          *sp = xsiz;
        } else {
          tcrdbsetecode(rdb, TTERECV);
          tcfree(xbuf);
          xbuf = NULL;
        }
      } else {
        tcrdbsetecode(rdb, TTERECV);
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
    }
  }
  pthread_cleanup_pop(1);
  return xbuf;
}


/* Synchronize updated contents of a remote database object with the file and the device.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
static bool tcrdbsyncimpl(TCRDB *rdb){
  assert(rdb);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  unsigned char buf[TTIOBUFSIZ];
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDSYNC;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  return !err;
}

/* Optimize the storage of a remove database object.
   `rdb' specifies the remote database object.
   `params' specifies the string of the tuning parameters.
   If successful, the return value is true, else, it is false. */
static bool tcrdboptimizeimpl(TCRDB *rdb, const char *params){
  assert(rdb);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  if(!params) params = "";
  int psiz = strlen(params);
  bool err = false;
  int rsiz = 2 + sizeof(uint32_t) + psiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDOPTIMIZE;
  uint32_t num;
  num = htonl((uint32_t)psiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, params, psiz);
  wp += psiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Remove all records of a remote database object.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
static bool tcrdbvanishimpl(TCRDB *rdb){
  assert(rdb);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  unsigned char buf[TTIOBUFSIZ];
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDVANISH;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  return !err;
}


/* Copy the database file of a remote database object.
   `rdb' specifies the remote database object.
   `path' specifies the path of the destination file.
   If successful, the return value is true, else, it is false. */
static bool tcrdbcopyimpl(TCRDB *rdb, const char *path){
  assert(rdb && path);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  int psiz = strlen(path);
  int rsiz = 2 + sizeof(uint32_t) + psiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDCOPY;
  uint32_t num;
  num = htonl((uint32_t)psiz);
  memcpy(wp, &num, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, path, psiz);
  wp += psiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Restore the database file of a remote database object from the update log.
   `rdb' specifies the remote database object.
   `path' specifies the path of the update log directory.
   `ts' specifies the beginning timestamp in microseconds.
   `opts' specifies options by bitwise-or.
   If successful, the return value is true, else, it is false. */
static bool tcrdbrestoreimpl(TCRDB *rdb, const char *path, uint64_t ts, int opts){
  assert(rdb && path);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  bool err = false;
  int psiz = strlen(path);
  int rsiz = 2 + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint32_t) + psiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDRESTORE;
  uint32_t lnum = htonl((uint32_t)psiz);
  memcpy(wp, &lnum, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  uint64_t llnum = htonll(ts);
  memcpy(wp, &llnum, sizeof(uint64_t));
  wp += sizeof(uint64_t);
  lnum = htonl((uint32_t)opts);
  memcpy(wp, &lnum, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, path, psiz);
  wp += psiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Set the replication master of a remote database object.
   `rdb' specifies the remote database object.
   `host' specifies the name or the address of the server.
   `port' specifies the port number.
   `ts' specifies the beginning timestamp in microseconds.
   `opts' specifies options by bitwise-or: `RDBROCHKCON' for consistency checking.
   If successful, the return value is true, else, it is false. */
static bool tcrdbsetmstimpl(TCRDB *rdb, const char *host, int port, uint64_t ts, int opts){
  assert(rdb);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return false;
    }
    if(!tcrdbreconnect(rdb)) return false;
  }
  if(!host) host = "";
  if(port < 0) port = 0;
  bool err = false;
  int hsiz = strlen(host);
  int rsiz = 2 + sizeof(uint32_t) * 3 + hsiz;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDSETMST;
  uint32_t lnum;
  lnum = htonl((uint32_t)hsiz);
  memcpy(wp, &lnum, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  lnum = htonl((uint32_t)port);
  memcpy(wp, &lnum, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  uint64_t llnum;
  llnum = htonll(ts);
  memcpy(wp, &llnum, sizeof(uint64_t));
  wp += sizeof(uint64_t);
  lnum = htonl((uint32_t)opts);
  memcpy(wp, &lnum, sizeof(uint32_t));
  wp += sizeof(uint32_t);
  memcpy(wp, host, hsiz);
  wp += hsiz;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code != 0){
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  return !err;
}


/* Get the simple server expression of an abstract database object.
   `rdb' specifies the remote database object.
   The return value is the simple server expression or `NULL' if the object does not connect to
   any database server. */
const char *tcrdbexprimpl(TCRDB *rdb){
  assert(rdb);
  if(!rdb->host){
    tcrdbsetecode(rdb, TTEINVALID);
    return NULL;
  }
  return rdb->expr;
}


/* Get the number of records of a remote database object.
   `rdb' specifies the remote database object.
   The return value is the number of records or 0 if the object does not connect to any database
   server. */
static uint64_t tcrdbrnumimpl(TCRDB *rdb){
  assert(rdb);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return 0;
    }
    if(!tcrdbreconnect(rdb)) return 0;
  }
  unsigned char buf[TTIOBUFSIZ];
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDRNUM;
  uint64_t rnum = 0;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      rnum = ttsockgetint64(rdb->sock);
      if(ttsockcheckend(rdb->sock)){
        rnum = 0;
        tcrdbsetecode(rdb, TTERECV);
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
    }
  }
  return rnum;
}


/* Get the size of the database of a remote database object.
   `rdb' specifies the remote database object.
   The return value is the size of the database or 0 if the object does not connect to any
   database server. */
static uint64_t tcrdbsizeimpl(TCRDB *rdb){
  assert(rdb);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return 0;
    }
    if(!tcrdbreconnect(rdb)) return 0;
  }
  unsigned char buf[TTIOBUFSIZ];
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDSIZE;
  uint64_t size = 0;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      size = ttsockgetint64(rdb->sock);
      if(ttsockcheckend(rdb->sock)){
        size = 0;
        tcrdbsetecode(rdb, TTERECV);
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
    }
  }
  return size;
}


/* Get the status string of the database of a remote database object.
   `rdb' specifies the remote database object.
   The return value is the status message of the database or `NULL' if the object does not
   connect to any database server. */
static char *tcrdbstatimpl(TCRDB *rdb){
  assert(rdb);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return NULL;
    }
    if(!tcrdbreconnect(rdb)) return NULL;
  }
  unsigned char buf[TTIOBUFSIZ];
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDSTAT;
  uint32_t size = 0;
  if(tcrdbsend(rdb, buf, wp - buf)){
    int code = ttsockgetc(rdb->sock);
    if(code == 0){
      size = ttsockgetint32(rdb->sock);
      if(ttsockcheckend(rdb->sock) || size >= TTIOBUFSIZ ||
         !ttsockrecv(rdb->sock, (char *)buf, size)){
        size = 0;
        tcrdbsetecode(rdb, TTERECV);
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
    }
  }
  if(size < 1){
    tcrdbsetecode(rdb, TTEMISC);
    return NULL;
  }
  return tcmemdup(buf, size);
}


/* Call a versatile function for miscellaneous operations of a remote database object.
   `rdb' specifies the remote database object.
   `name' specifies the name of the function.
   `opts' specifies options by bitwise-or.
   `args' specifies a list object containing arguments.
   If successful, the return value is a list object of the result. */
static TCLIST *tcrdbmiscimpl(TCRDB *rdb, const char *name, int opts, const TCLIST *args){
  assert(rdb && name && args);
  if(rdb->fd < 0){
    if(!rdb->host || !(rdb->opts & RDBTRECON)){
      tcrdbsetecode(rdb, TTEINVALID);
      return NULL;
    }
    if(!tcrdbreconnect(rdb)) return NULL;
  }
  bool err = false;
  TCLIST *res = NULL;
  TCXSTR *xstr = tcxstrnew();
  pthread_cleanup_push((void (*)(void *))tcxstrdel, xstr);
  uint8_t magic[2];
  magic[0] = TTMAGICNUM;
  magic[1] = TTCMDMISC;
  tcxstrcat(xstr, magic, sizeof(magic));
  int nsiz = strlen(name);
  uint32_t num;
  num = htonl((uint32_t)nsiz);
  tcxstrcat(xstr, &num, sizeof(num));
  num = htonl((uint32_t)opts);
  tcxstrcat(xstr, &num, sizeof(num));
  num = tclistnum(args);
  num = htonl(num);
  tcxstrcat(xstr, &num, sizeof(num));
  tcxstrcat(xstr, name, nsiz);
  for(int i = 0; i < tclistnum(args); i++){
    int rsiz;
    const char *rbuf = tclistval(args, i, &rsiz);
    num = htonl((uint32_t)rsiz);
    tcxstrcat(xstr, &num, sizeof(num));
    tcxstrcat(xstr, rbuf, rsiz);
  }
  char stack[TTIOBUFSIZ];
  if(tcrdbsend(rdb, tcxstrptr(xstr), tcxstrsize(xstr))){
    int code = ttsockgetc(rdb->sock);
    int rnum = ttsockgetint32(rdb->sock);
    if(code == 0){
      if(!ttsockcheckend(rdb->sock) && rnum >= 0){
        res = tclistnew2(rnum);
        for(int i = 0; i < rnum; i++){
          int esiz = ttsockgetint32(rdb->sock);
          if(ttsockcheckend(rdb->sock)){
            tcrdbsetecode(rdb, TTERECV);
            err = true;
            break;
          }
          char *ebuf = (esiz < TTIOBUFSIZ) ? stack : tcmalloc(esiz + 1);
          if(ttsockrecv(rdb->sock, ebuf, esiz)){
            tclistpush(res, ebuf, esiz);
          } else {
            tcrdbsetecode(rdb, TTERECV);
            err = true;
          }
          if(ebuf != stack) tcfree(ebuf);
        }
      } else {
        tcrdbsetecode(rdb, TTERECV);
        err = true;
      }
    } else {
      tcrdbsetecode(rdb, code == -1 ? TTERECV : TTEMISC);
      err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  if(res && err){
    tclistdel(res);
    res = NULL;
  }
  return res;
}


// END OF FILE
