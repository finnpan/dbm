/*************************************************************************************************
 * Scripting language extension of Tokyo Tyrant
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


#include "ttscrext.h"



/*************************************************************************************************
 * by default
 *************************************************************************************************/



typedef struct _SCREXT {                 // type of structure of the script extension
  struct _SCREXT **screxts;              // script extension objects
  int thnum;                             // number of native threads
  int thid;                              // thread ID
  char *path;                            // path of the initializing script
  TCADB *adb;                            // abstract database object
  TCULOG *ulog;                          // update log object
  uint32_t sid;                          // server ID
  TCMDB *stash;                          // global stash object
  TCMDB *lock;                           // global lock object
  void (*logger)(int, const char *, void *);  // logging function
  void *logopq;                          // opaque pointer for the logging function
  bool term;                             // terminate flag
} SCREXT;


/* Initialize the global scripting language extension. */
void *scrextnew(void **screxts, int thnum, int thid, const char *path,
                TCADB *adb, TCULOG *ulog, uint32_t sid, TCMDB *stash, TCMDB *lock,
                void (*logger)(int, const char *, void *), void *logopq){
  SCREXT *scr = tcmalloc(sizeof(*scr));
  scr->screxts = (SCREXT **)screxts;
  scr->thnum = thnum;
  scr->thid = thid;
  scr->path = tcstrdup(path);
  scr->adb = adb;
  scr->ulog = ulog;
  scr->sid = sid;
  scr->stash = stash;
  scr->lock = lock;
  scr->logger = logger;
  scr->logopq = logopq;
  scr->term = false;
  return scr;
}


/* Destroy the global scripting language extension. */
bool scrextdel(void *scr){
  SCREXT *myscr = scr;
  tcfree(myscr->path);
  tcfree(myscr);
  return true;
}


/* Call a method of the scripting language extension. */
char *scrextcallmethod(void *scr, const char *name,
                       const void *kbuf, int ksiz, const void *vbuf, int vsiz, int *sp){
  SCREXT *myscr = scr;
  if(!strcmp(name, "put")){
    if(!tculogadbput(myscr->ulog, myscr->sid, 0, myscr->adb, kbuf, ksiz, vbuf, vsiz))
      return NULL;
    char *msg = tcstrdup("ok");
    *sp = strlen(msg);
    return msg;
  } else if(!strcmp(name, "putkeep")){
    if(!tculogadbputkeep(myscr->ulog, myscr->sid, 0, myscr->adb, kbuf, ksiz, vbuf, vsiz))
      return NULL;
    char *msg = tcstrdup("ok");
    *sp = strlen(msg);
    return msg;
  } else if(!strcmp(name, "putcat")){
    if(!tculogadbputcat(myscr->ulog, myscr->sid, 0, myscr->adb, kbuf, ksiz, vbuf, vsiz))
      return NULL;
    char *msg = tcstrdup("ok");
    *sp = strlen(msg);
    return msg;
  } else if(!strcmp(name, "out")){
    if(!tculogadbout(myscr->ulog, myscr->sid, 0, myscr->adb, kbuf, ksiz)) return NULL;
    char *msg = tcstrdup("ok");
    *sp = strlen(msg);
    return msg;
  } else if(!strcmp(name, "get")){
    return tcadbget(myscr->adb, kbuf, ksiz, sp);
  } else if(!strcmp(name, "log")){
    char *msg = tcmemdup(kbuf, ksiz);
    myscr->logger(TTLOGINFO, msg, myscr->logopq);
    tcfree(msg);
    msg = tcstrdup("ok");
    *sp = strlen(msg);
    return msg;
  }
  int psiz = strlen(myscr->path);
  int nsiz = strlen(name);
  char *msg = tcmalloc(psiz + nsiz + ksiz + vsiz + 4);
  char *wp = msg;
  memcpy(wp, myscr->path, psiz);
  wp += psiz;
  *(wp++) = ':';
  memcpy(wp, name, nsiz);
  wp += nsiz;
  *(wp++) = ':';
  memcpy(wp, kbuf, ksiz);
  wp += ksiz;
  *(wp++) = ':';
  memcpy(wp, vbuf, vsiz);
  wp += vsiz;
  *sp = wp - msg;
  return msg;
}


/* Send the terminate signal to the scripting language extension */
bool scrextkill(void *scr){
  SCREXT *myscr = scr;
  myscr->term = true;
  return true;
}
