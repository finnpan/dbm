/*************************************************************************************************
 * The update log API of Tokyo Tyrant
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


#ifndef _NET_H                        /* duplication check */
#define _NET_H

#if defined(__cplusplus)
#define __NET_CLINKAGEBEGIN extern "C" {
#define __NET_CLINKAGEEND }
#else
#define __NET_CLINKAGEBEGIN
#define __NET_CLINKAGEEND
#endif
__NET_CLINKAGEBEGIN


#include "db.h"



/*************************************************************************************************
 * basic utilities
 *************************************************************************************************/


#define TTIOBUFSIZ     65536             /* size of an I/O buffer */
#define TTADDRBUFSIZ   1024              /* size of an address buffer */

typedef struct {                         /* type of structure for a socket */
  int fd;                                /* file descriptor */
  char buf[TTIOBUFSIZ];                  /* reading buffer */
  char *rp;                              /* reading pointer */
  char *ep;                              /* end pointer */
  bool end;                              /* end flag */
  double to;                             /* timeout */
  double dl;                             /* deadline time */
} TTSOCK;


/* String containing the version information. */
extern const char *ttversion;


/* Get the address of a host.
   `name' specifies the name of the host.
   `addr' specifies the pointer to the region into which the address is written.  The size of the
   buffer should be equal to or more than `TTADDRBUFSIZ' bytes.
   If successful, the return value is true, else, it is false. */
bool ttgethostaddr(const char *name, char *addr);


/* Open a client socket of TCP/IP stream to a server.
   `addr' specifies the address of the server.
   `port' specifies the port number of the server.
   The return value is the file descriptor of the stream, or -1 on error. */
int ttopensock(const char *addr, int port);


/* Open a client socket of UNIX domain stream to a server.
   `path' specifies the path of the socket file.
   The return value is the file descriptor of the stream, or -1 on error. */
int ttopensockunix(const char *path);


/* Open a server socket of TCP/IP stream to clients.
   `addr' specifies the address of the server.  If it is `NULL', every network address is binded.
   `port' specifies the port number of the server.
   The return value is the file descriptor of the stream, or -1 on error. */
int ttopenservsock(const char *addr, int port);


/* Open a server socket of UNIX domain stream to clients.
   `addr' specifies the address of the server.  If it is `NULL', every network address is binded.
   `port' specifies the port number of the server.
   The return value is the file descriptor of the stream, or -1 on error. */
int ttopenservsockunix(const char *path);


/* Accept a TCP/IP connection from a client.
   `fd' specifies the file descriptor.
   `addr' specifies the pointer to the region into which the client address is written.  The size
   of the buffer should be equal to or more than `TTADDRBUFSIZ' bytes.
   `pp' specifies the pointer to a variable to which the client port is assigned.  If it is
   `NULL', it is not used.
   The return value is the file descriptor of the stream, or -1 on error. */
int ttacceptsock(int fd, char *addr, int *pp);


/* Accept a UNIX domain connection from a client.
   `fd' specifies the file descriptor.
   The return value is the file descriptor of the stream, or -1 on error. */
int ttacceptsockunix(int fd);


/* Shutdown and close a socket.
   `fd' specifies the file descriptor.
   If successful, the return value is true, else, it is false. */
bool ttclosesock(int fd);


/* Wait an I/O event of a socket.
   `fd' specifies the file descriptor.
   `mode' specifies the kind of events; 0 for reading, 1 for writing, 2 for exception.
   `timeout' specifies the timeout in seconds.
   If successful, the return value is true, else, it is false. */
bool ttwaitsock(int fd, int mode, double timeout);


/* Create a socket object.
   `fd' specifies the file descriptor.
   The return value is the socket object. */
TTSOCK *ttsocknew(int fd);


/* Delete a socket object.
   `sock' specifies the socket object. */
void ttsockdel(TTSOCK *sock);


/* Set the lifetime of a socket object.
   `sock' specifies the socket object.
   `lifetime' specifies the lifetime seconds of each task.  By default, there is no limit. */
void ttsocksetlife(TTSOCK *sock, double lifetime);


/* Send data by a socket.
   `sock' specifies the socket object.
   `buf' specifies the pointer to the region of the data to send.
   `size' specifies the size of the buffer.
   If successful, the return value is true, else, it is false. */
bool ttsocksend(TTSOCK *sock, const void *buf, int size);


/* Send formatted data by a socket.
   `sock' specifies the socket object.
   `format' specifies the printf-like format string.
   The conversion character `%' can be used with such flag characters as `s', `d', `o', `u',
   `x', `X', `c', `e', `E', `f', `g', `G', `@', `?', `%'.  `@' works as with `s' but escapes meta
   characters of XML.  `?' works as with `s' but escapes meta characters of URL.  The other
   conversion character work as with each original.
   The other arguments are used according to the format string.
   If successful, the return value is true, else, it is false. */
bool ttsockprintf(TTSOCK *sock, const char *format, ...);


/* Receive data by a socket.
   `sock' specifies the socket object.
   `buf' specifies the pointer to the region of the data to be received.
   `size' specifies the size of the buffer.
   If successful, the return value is true, else, it is false.   False is returned if the socket
   is closed before receiving the specified size of data. */
bool ttsockrecv(TTSOCK *sock, char *buf, int size);


/* Receive one byte by a socket.
   `sock' specifies the socket object.
   The return value is the received byte.  If some error occurs or the socket is closed by the
   server, -1 is returned. */
int ttsockgetc(TTSOCK *sock);


/* Push a character back to a socket.
   `sock' specifies the socket object.
   `c' specifies the character. */
void ttsockungetc(TTSOCK *sock, int c);


/* Receive one line by a socket.
   `sock' specifies the socket object.
   `buf' specifies the pointer to the region of the data to be received.
   `size' specifies the size of the buffer.
   If successful, the return value is true, else, it is false.   False is returned if the socket
   is closed before receiving linefeed. */
bool ttsockgets(TTSOCK *sock, char *buf, int size);


/* Receive one line by a socket into allocated buffer.
   `sock' specifies the socket object.
   If successful, the return value is the pointer to the result buffer, else, it is `NULL'.
   `NULL' is returned if the socket is closed before receiving linefeed.
   Because  the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *ttsockgets2(TTSOCK *sock);


/* Receive an 32-bit integer by a socket.
   `sock' specifies the socket object.
   The return value is the 32-bit integer. */
uint32_t ttsockgetint32(TTSOCK *sock);


/* Receive an 64-bit integer by a socket.
   `sock' specifies the socket object.
   The return value is the 64-bit integer. */
uint64_t ttsockgetint64(TTSOCK *sock);


/* Check whehter a socket is end.
   `sock' specifies the socket object.
   The return value is true if the socket is end, else, it is false. */
bool ttsockcheckend(TTSOCK *sock);


/* Fetch the resource of a URL by HTTP.
   `url' specifies the URL.
   `reqheads' specifies a map object contains request header names and their values.  The header
   "X-TT-Timeout" specifies the timeout in seconds.  If it is `NULL', it is not used.
   `resheads' specifies a map object to store response headers their values.  If it is NULL, it
   is not used.  Each key of the map is an uncapitalized header name.  The key "STATUS" means the
   status line.
   `resbody' specifies a extensible string object to store the entity body of the result.  If it
   is `NULL', it is not used.
   The return value is the response code or -1 on network error. */
int tthttpfetch(const char *url, TCMAP *reqheads, TCMAP *resheads, TCXSTR *resbody);


/* Serialize a real number.
   `num' specifies a real number.
   `buf' specifies the pointer to the region into which the result is written.  The size of the
   buffer should be 16 bytes. */
void ttpackdouble(double num, char *buf);


/* Redintegrate a serialized real number.
   `buf' specifies the pointer to the region of the serialized real number.  The size of the
   buffer should be 16 bytes.
   The return value is the original real number. */
double ttunpackdouble(const char *buf);



/*************************************************************************************************
 * server utilities
 *************************************************************************************************/


#define TTDEFPORT      1978              /* default port of the server */
#define TTMAGICNUM     0xc8              /* magic number of each command */
#define TTCMDPUT       0x10              /* ID of put command */
#define TTCMDPUTKEEP   0x11              /* ID of putkeep command */
#define TTCMDPUTCAT    0x12              /* ID of putcat command */
#define TTCMDPUTSHL    0x13              /* ID of putshl command */
#define TTCMDPUTNR     0x18              /* ID of putnr command */
#define TTCMDOUT       0x20              /* ID of out command */
#define TTCMDGET       0x30              /* ID of get command */
#define TTCMDMGET      0x31              /* ID of mget command */
#define TTCMDVSIZ      0x38              /* ID of vsiz command */
#define TTCMDITERINIT  0x50              /* ID of iterinit command */
#define TTCMDITERNEXT  0x51              /* ID of iternext command */
#define TTCMDFWMKEYS   0x58              /* ID of fwmkeys command */
#define TTCMDADDINT    0x60              /* ID of addint command */
#define TTCMDADDDOUBLE 0x61              /* ID of adddouble command */
#define TTCMDEXT       0x68              /* ID of ext command */
#define TTCMDSYNC      0x70              /* ID of sync command */
#define TTCMDOPTIMIZE  0x71              /* ID of optimize command */
#define TTCMDVANISH    0x72              /* ID of vanish command */
#define TTCMDCOPY      0x73              /* ID of copy command */
#define TTCMDRESTORE   0x74              /* ID of restore command */
#define TTCMDSETMST    0x78              /* ID of setmst command */
#define TTCMDRNUM      0x80              /* ID of rnum command */
#define TTCMDSIZE      0x81              /* ID of size command */
#define TTCMDSTAT      0x88              /* ID of stat command */
#define TTCMDMISC      0x90              /* ID of misc command */
#define TTCMDREPL      0xa0              /* ID of repl command */

#define TTTIMERMAX     8                 /* maximum number of timers */

typedef struct _TTTIMER {                /* type of structure for a timer */
  pthread_t thid;                        /* thread ID */
  bool alive;                            /* alive flag */
  struct _TTSERV *serv;                  /* server object */
  double freq_timed;                     /* frequency of timed handler */
  void (*do_timed)(void *);              /* call back function for timed handler */
  void *opq_timed;                       /* opaque pointer for timed handler */
} TTTIMER;

typedef struct _TTREQ {                  /* type of structure for a server */
  pthread_t thid;                        /* thread ID */
  bool alive;                            /* alive flag */
  struct _TTSERV *serv;                  /* server object */
  int epfd;                              /* polling file descriptor */
  double mtime;                          /* last modified time */
  bool keep;                             /* keep-alive flag */
  int idx;                               /* ordinal index */
} TTREQ;

typedef struct _TTSERV {                 /* type of structure for a server */
  char host[TTADDRBUFSIZ];               /* host name */
  char addr[TTADDRBUFSIZ];               /* host address */
  uint16_t port;                         /* port number */
  TCLIST *queue;                         /* queue of requests */
  pthread_mutex_t qmtx;                  /* mutex for the queue */
  pthread_cond_t qcnd;                   /* condition variable for the queue */
  pthread_mutex_t tmtx;                  /* mutex for the timer */
  pthread_cond_t tcnd;                   /* condition variable for the timer */
  int thnum;                             /* number of threads */
  double timeout;                        /* timeout milliseconds of each task */
  bool term;                             /* terminate flag */
  void (*do_log)(int, const char *, void *);  /* call back function for logging */
  void *opq_log;                         /* opaque pointer for logging */
  TTTIMER timers[TTTIMERMAX];            /* timer objects */
  int timernum;                          /* number of timer objects */
  void (*do_task)(TTSOCK *, void *, TTREQ *);  /* call back function for task */
  void *opq_task;                        /* opaque pointer for task */
  void (*do_term)(void *);               /* call back gunction for termination */
  void *opq_term;                        /* opaque pointer for termination */
} TTSERV;

enum {                                   /* enumeration for logging levels */
  TTLOGDEBUG,                            /* debug */
  TTLOGINFO,                             /* information */
  TTLOGERROR,                            /* error */
  TTLOGSYSTEM                            /* system */
};


/* Create a server object.
   The return value is the server object. */
TTSERV *ttservnew(void);


/* Delete a server object.
   `serv' specifies the server object. */
void ttservdel(TTSERV *serv);


/* Configure a server object.
   `serv' specifies the server object.
   `host' specifies the name or the address.  If it is `NULL', If it is `NULL', every network
   address is binded.
   `port' specifies the port number.  If it is not less than 0, UNIX domain socket is binded and
   the host name is treated as the path of the socket file.
   If successful, the return value is true, else, it is false. */
bool ttservconf(TTSERV *serv, const char *host, int port);


/* Set tuning parameters of a server object.
   `serv' specifies the server object.
   `thnum' specifies the number of worker threads.  By default, the number is 5.
   `timeout' specifies the timeout seconds of each task.  If it is not more than 0, no timeout is
   specified.  By default, there is no timeout. */
void ttservtune(TTSERV *serv, int thnum, double timeout);


/* Set the logging handler of a server object.
   `serv' specifies the server object.
   `do_log' specifies the pointer to a function to do with a log message.  Its first parameter is
   the log level, one of `TTLOGDEBUG', `TTLOGINFO', `TTLOGERROR'.  Its second parameter is the
   message string.  Its third parameter is the opaque pointer.
   `opq' specifies the opaque pointer to be passed to the handler.  It can be `NULL'. */
void ttservsetloghandler(TTSERV *serv, void (*do_log)(int, const char *, void *), void *opq);


/* Add a timed handler to a server object.
   `serv' specifies the server object.
   `freq' specifies the frequency of execution in seconds.
   `do_timed' specifies the pointer to a function to do with a event.  Its parameter is the
   opaque pointer.
   `opq' specifies the opaque pointer to be passed to the handler.  It can be `NULL'. */
void ttservaddtimedhandler(TTSERV *serv, double freq, void (*do_timed)(void *), void *opq);


/* Set the response handler of a server object.
   `serv' specifies the server object.
   `do_task' specifies the pointer to a function to do with a task.  Its first parameter is
   the socket object connected to the client.  Its second parameter is the opaque pointer.  Its
   third parameter is the request object.
   `opq' specifies the opaque pointer to be passed to the handler.  It can be `NULL'. */
void ttservsettaskhandler(TTSERV *serv, void (*do_task)(TTSOCK *, void *, TTREQ *), void *opq);


/* Set the termination handler of a server object.
   `serv' specifies the server object.
   `do_term' specifies the pointer to a function to do with a task.  Its parameter is the opaque
   pointer.
   `opq' specifies the opaque pointer to be passed to the handler.  It can be `NULL'. */
void ttservsettermhandler(TTSERV *serv, void (*do_term)(void *), void *opq);


/* Start the service of a server object.
   `serv' specifies the server object.
   If successful, the return value is true, else, it is false. */
bool ttservstart(TTSERV *serv);


/* Send the terminate signal to a server object.
   `serv' specifies the server object.
   If successful, the return value is true, else, it is false. */
bool ttservkill(TTSERV *serv);


/* Call the logging function of a server object.
   `serv' specifies the server object.
   `level' specifies the logging level.
   `format' specifies the message format.
   The other arguments are used according to the format string. */
void ttservlog(TTSERV *serv, int level, const char *format, ...);


/* Check whether a server object is killed.
   `serv' specifies the server object.
   The return value is true if the server is killed, or false if not. */
bool ttserviskilled(TTSERV *serv);


/* Break a simple server expression.
   `expr' specifies the simple server expression.  It is composed of two substrings separated
   by ":".  The former field specifies the name or the address of the server.  The latter field
   specifies the port number.  If the latter field is omitted, the default port number is
   specified.
   `pp' specifies the pointer to a variable to which the port number is assigned.  If it is
   `NULL', it is not used.
   The return value is the string of the host name.
   Because  the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *ttbreakservexpr(const char *expr, int *pp);



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


#define _TT_VERSION    "1.1.41"
#define _TT_LIBVER     324
#define _TT_PROTVER    "0.91"


/* Switch the process into the background.
   If successful, the return value is true, else, it is false. */
bool ttdaemonize(void);


/* Get the load average of the system.
   The return value is the load average of the system. */
double ttgetloadavg(void);


/* Convert a string to a time stamp.
   `str' specifies the string.
   The return value is the time stamp. */
uint64_t ttstrtots(const char *str);


/* Get the command name of a command ID number.
   `id' specifies the command ID number.
   The return value is the string of the command name. */
const char *ttcmdidtostr(int id);



/*************************************************************************************************
 * API of update log
 *************************************************************************************************/


#define TCULSUFFIX     ".ulog"           /* suffix of update log files */
#define TCULMAGICNUM   0xc9              /* magic number of each command */
#define TCULMAGICNOP   0xca              /* magic number of NOP command */
#define TCULRMTXNUM    31                /* number of mutexes of records */

typedef struct {                         /* type of structure for an update log */
  pthread_mutex_t rmtxs[TCULRMTXNUM];    /* mutex for records */
  pthread_rwlock_t rwlck;                /* mutex for operation */
  pthread_cond_t cnd;                    /* condition variable */
  pthread_mutex_t wmtx;                  /* mutex for waiting condition */
  char *base;                            /* path of the base directory */
  uint64_t limsiz;                       /* limit size */
  int max;                               /* number of maximum ID */
  int fd;                                /* current file descriptor */
  uint64_t size;                         /* current size */
  void *aiocbs;                          /* AIO tasks */
  int aiocbi;                            /* index of AIO tasks */
  uint64_t aioend;                       /* end offset of AIO tasks */
} TCULOG;

typedef struct {                         /* type of structure for a log reader */
  TCULOG *ulog;                          /* update log object */
  uint64_t ts;                           /* beginning timestamp */
  int num;                               /* number of current ID */
  int fd;                                /* current file descriptor */
  char *rbuf;                            /* record buffer */
  int rsiz;                              /* size of the record buffer */
} TCULRD;

typedef struct {                         /* type of structure for a replication */
  int fd;                                /* file descriptor */
  TTSOCK *sock;                          /* socket object */
  char *rbuf;                            /* record buffer */
  int rsiz;                              /* size of the record buffer */
  uint16_t mid;                          /* master server ID number */
} TCREPL;


/* Create an update log object.
   The return value is the new update log object. */
TCULOG *tculognew(void);


/* Delete an update log object.
   `ulog' specifies the update log object. */
void tculogdel(TCULOG *ulog);


/* Set AIO control of an update log object.
   `ulog' specifies the update log object.
   If successful, the return value is true, else, it is false. */
bool tculogsetaio(TCULOG *ulog);


/* Open files of an update log object.
   `ulog' specifies the update log object.
   `base' specifies the path of the base directory.
   `limsiz' specifies the limit size of each file.  If it is not more than 0, no limit is
   specified.
   If successful, the return value is true, else, it is false. */
bool tculogopen(TCULOG *ulog, const char *base, uint64_t limsiz);


/* Close files of an update log object.
   `ulog' specifies the update log object.
   If successful, the return value is true, else, it is false. */
bool tculogclose(TCULOG *ulog);


/* Get the mutex index of a record.
   `ulog' specifies the update log object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   The return value is the mutex index of a record. */
int tculogrmtxidx(TCULOG *ulog, const char *kbuf, int ksiz);


/* Begin the critical section of an update log object.
   `ulog' specifies the update log object.
   `idx' specifies the index of the record lock.  -1 means to lock all.
   If successful, the return value is true, else, it is false. */
bool tculogbegin(TCULOG *ulog, int idx);


/* End the critical section of an update log object.
   `ulog' specifies the update log object.
   `idx' specifies the index of the record lock.  -1 means to lock all.
   If successful, the return value is true, else, it is false. */
bool tculogend(TCULOG *ulog, int idx);


/* Write a message into an update log object.
   `ulog' specifies the update log object.
   `ts' specifies the timestamp.  If it is 0, the current time is specified.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `ptr' specifies the pointer to the region of the message.
   `size' specifies the size of the region.
   If successful, the return value is true, else, it is false. */
bool tculogwrite(TCULOG *ulog, uint64_t ts, uint32_t sid, uint32_t mid,
                 const void *ptr, int size);


/* Create a log reader object.
   `ulog' specifies the update log object.
   `ts' specifies the beginning timestamp.
   The return value is the new log reader object. */
TCULRD *tculrdnew(TCULOG *ulog, uint64_t ts);


/* Delete a log reader object.
   `ulrd' specifies the log reader object. */
void tculrddel(TCULRD *ulrd);


/* Wait the next message is written.
   `ulrd' specifies the log reader object. */
void tculrdwait(TCULRD *ulrd);


/* Read a message from a log reader object.
   `ulrd' specifies the log reader object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   `tsp' specifies the pointer to the variable into which the timestamp of the next message is
   assigned.
   `sidp' specifies the pointer to the variable into which the origin server ID of the next
   message is assigned.
   `midp' specifies the pointer to the variable into which the master server ID of the next
   message is assigned.
   If successful, the return value is the pointer to the region of the value of the next message.
   `NULL' is returned if no record is to be read. */
const void *tculrdread(TCULRD *ulrd, int *sp, uint64_t *tsp, uint32_t *sidp, uint32_t *midp);


/* Store a record into an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tculogadbput(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb,
                  const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a new record into an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tculogadbputkeep(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb,
                      const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Concatenate a value at the end of the existing record in an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tculogadbputcat(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb,
                     const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Concatenate a value at the end of the existing record and shift it to the left.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `width' specifies the width of the record.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tculogadbputshl(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb,
                     const void *kbuf, int ksiz, const void *vbuf, int vsiz, int width);


/* Remove a record of an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false. */
bool tculogadbout(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb,
                  const void *kbuf, int ksiz);


/* Add an integer to a record in an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is `INT_MIN'.
   If the corresponding record exists, the value is treated as an integer and is added to.  If no
   record corresponds, a new record of the additional value is stored. */
int tculogadbaddint(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb,
                    const void *kbuf, int ksiz, int num);


/* Add a real number to a record in an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is `NAN'.
   If the corresponding record exists, the value is treated as a real number and is added to.  If
   no record corresponds, a new record of the additional value is stored. */
double tculogadbadddouble(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb,
                          const void *kbuf, int ksiz, double num);


/* Synchronize updated contents of an abstract database object with the file and the device.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false. */
bool tculogadbsync(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb);


/* Optimize the storage of an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   `params' specifies the string of the tuning parameters, which works as with the tuning
   of parameters the function `tcadbopen'.  If it is `NULL', it is not used.
   If successful, the return value is true, else, it is false. */
bool tculogadboptimize(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb, const char *params);


/* Remove all records of an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   If successful, the return value is true, else, it is false. */
bool tculogadbvanish(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb);


/* Call a versatile function for miscellaneous operations of an abstract database object.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `adb' specifies the abstract database object.
   `name' specifies the name of the function.
   `args' specifies a list object containing arguments.
   If successful, the return value is a list object of the result.  `NULL' is returned on failure.
   All databases support "putlist", "outlist", and "getlist".  "putdup" is to store records.  It
   receives keys and values one after the other, and returns an empty list.  "outlist" is to
   remove records.  It receives keys, and returns an empty list.  "getlist" is to retrieve
   records.  It receives keys, and returns values.  Because the object of the return value is
   created with the function `tclistnew', it should be deleted with the function `tclistdel' when
   it is no longer in use. */
TCLIST *tculogadbmisc(TCULOG *ulog, uint32_t sid, uint32_t mid, TCADB *adb,
                      const char *name, const TCLIST *args);


/* Restore an abstract database object.
   `adb' specifies the abstract database object.
   `path' specifies the path of the update log directory.
   `ts' specifies the beginning time stamp.
   `con' specifies whether consistency checking is performed.
   `ulog' specifies the update log object.
   If successful, the return value is true, else, it is false. */
bool tculogadbrestore(TCADB *adb, const char *path, uint64_t ts, bool con, TCULOG *ulog);


/* Redo an update log message.
   `adb' specifies the abstract database object.
   `ptr' specifies the pointer to the region of the message.
   `size' specifies the size of the region.
   `ulog' specifies the update log object.
   `sid' specifies the origin server ID of the message.
   `mid' specifies the master server ID of the message.
   `cp' specifies the pointer to the variable into which the result of consistency checking is
   assigned.
   If successful, the return value is true, else, it is false. */
bool tculogadbredo(TCADB *adb, const char *ptr, int size, TCULOG *ulog,
                   uint32_t sid, uint32_t mid, bool *cp);


/* Create a replication object.
   The return value is the new replicatoin object. */
TCREPL *tcreplnew(void);


/* Delete a replication object.
   `repl' specifies the replication object. */
void tcrepldel(TCREPL *repl);


/* Open a replication object.
   `repl' specifies the replication object.
   `host' specifies the name or the address of the server.
   `port' specifies the port number.
   `sid' specifies the server ID of self messages.
   If successful, the return value is true, else, it is false. */
bool tcreplopen(TCREPL *repl, const char *host, int port, uint64_t ts, uint32_t sid);


/* Close a remote database object.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
bool tcreplclose(TCREPL *repl);


/* Read a message from a replication object.
   `repl' specifies the replication object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   `tsp' specifies the pointer to the variable into which the timestamp of the next message is
   assigned.
   `sidp' specifies the pointer to the variable into which the origin server ID of the next
   message is assigned.
   If successful, the return value is the pointer to the region of the value of the next message.
   `NULL' is returned if no record is to be read.  Empty string is returned when the no-operation
   command has been received. */
const char *tcreplread(TCREPL *repl, int *sp, uint64_t *tsp, uint32_t *sidp);



/*************************************************************************************************
 * API of remote database
 *************************************************************************************************/


typedef struct {                         /* type of structure for a remote database */
  pthread_mutex_t mmtx;                  /* mutex for method */
  pthread_key_t eckey;                   /* key for thread specific error code */
  char *host;                            /* host name */
  int port;                              /* port number */
  char *expr;                            /* simple server expression */
  int fd;                                /* file descriptor */
  TTSOCK *sock;                          /* socket object */
  double timeout;                        /* timeout */
  int opts;                              /* options */
} TCRDB;

enum {                                   /* enumeration for error codes */
  TTESUCCESS,                            /* success */
  TTEINVALID,                            /* invalid operation */
  TTENOHOST,                             /* host not found */
  TTEREFUSED,                            /* connection refused */
  TTESEND,                               /* send error */
  TTERECV,                               /* recv error */
  TTEKEEP,                               /* existing record */
  TTENOREC,                              /* no record found */
  TTEMISC = 9999                         /* miscellaneous error */
};

enum {                                   /* enumeration for tuning options */
  RDBTRECON = 1 << 0                     /* reconnect automatically */
};

enum {                                   /* enumeration for scripting extension options */
  RDBXOLCKREC = 1 << 0,                  /* record locking */
  RDBXOLCKGLB = 1 << 1                   /* global locking */
};

enum {                                   /* enumeration for restore options */
  RDBROCHKCON = 1 << 0                   /* consistency checking */
};

enum {                                   /* enumeration for miscellaneous operation options */
  RDBMONOULOG = 1 << 0                   /* omission of update log */
};


/* Get the message string corresponding to an error code.
   `ecode' specifies the error code.
   The return value is the message string of the error code. */
const char *tcrdberrmsg(int ecode);


/* Create a remote database object.
   The return value is the new remote database object. */
TCRDB *tcrdbnew(void);


/* Delete a remote database object.
   `rdb' specifies the remote database object. */
void tcrdbdel(TCRDB *rdb);


/* Get the last happened error code of a remote database object.
   `rdb' specifies the remote database object.
   The return value is the last happened error code.
   The following error code is defined: `TTESUCCESS' for success, `TTEINVALID' for invalid
   operation, `TTENOHOST' for host not found, `TTEREFUSED' for connection refused, `TTESEND' for
   send error, `TTERECV' for recv error, `TTEKEEP' for existing record, `TTENOREC' for no record
   found, `TTEMISC' for miscellaneous error. */
int tcrdbecode(TCRDB *rdb);


/* Set the tuning parameters of a hash database object.
   `rdb' specifies the remote database object.
   `timeout' specifies the timeout of each query in seconds.  If it is not more than 0, the
   timeout is not specified.
   `opts' specifies options by bitwise-or: `RDBTRECON' specifies that the connection is recovered
   automatically when it is disconnected.
   If successful, the return value is true, else, it is false.
   Note that the tuning parameters should be set before the database is opened. */
bool tcrdbtune(TCRDB *rdb, double timeout, int opts);


/* Open a remote database.
   `rdb' specifies the remote database object.
   `host' specifies the name or the address of the server.
   `port' specifies the port number.  If it is not more than 0, UNIX domain socket is used and
   the path of the socket file is specified by the host parameter.
   If successful, the return value is true, else, it is false. */
bool tcrdbopen(TCRDB *rdb, const char *host, int port);


/* Open a remote database with a simple server expression.
   `rdb' specifies the remote database object.
   `expr' specifies the simple server expression.  It is composed of two substrings separated
   by ":".  The former field specifies the name or the address of the server.  The latter field
   specifies the port number.  If the latter field is omitted, the default port number is
   specified.
   If successful, the return value is true, else, it is false. */
bool tcrdbopen2(TCRDB *rdb, const char *expr);


/* Close a remote database object.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
bool tcrdbclose(TCRDB *rdb);


/* Store a record into a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcrdbput(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a new record into a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcrdbputkeep(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Concatenate a value at the end of the existing record in a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcrdbputcat(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Concatenate a value at the end of the existing record and shift it to the left.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `width' specifies the width of the record.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcrdbputshl(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz, int width);


/* Store a record into a remote database object without response from the server.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcrdbputnr(TCRDB *rdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Remove a record of a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false. */
bool tcrdbout(TCRDB *rdb, const void *kbuf, int ksiz);


/* Retrieve a record in a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the corresponding
   record.  `NULL' is returned if no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcrdbget(TCRDB *rdb, const void *kbuf, int ksiz, int *sp);


/* Retrieve records in a remote database object.
   `rdb' specifies the remote database object.
   `recs' specifies a map object containing the retrieval keys.  As a result of this function,
   keys existing in the database have the corresponding values and keys not existing in the
   database are removed.
   If successful, the return value is true, else, it is false. */
bool tcrdbget3(TCRDB *rdb, TCMAP *recs);


/* Get the size of the value of a record in a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1. */
int tcrdbvsiz(TCRDB *rdb, const void *kbuf, int ksiz);



/* Initialize the iterator of a remote database object.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false.
   The iterator is used in order to access the key of every record stored in a database. */
bool tcrdbiterinit(TCRDB *rdb);


/* Get the next key of the iterator of a remote database object.
   `rdb' specifies the remote database object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the next key, else, it is
   `NULL'.  `NULL' is returned when no record is to be get out of the iterator.
   Because an additional zero code is appended at the end of the region of the return value, the
   return value can be treated as a character string.  Because the region of the return value is
   allocated with the `malloc' call, it should be released with the `free' call when it is no
   longer in use.  The iterator can be updated by multiple connections and then it is not assured
   that every record is traversed. */
void *tcrdbiternext(TCRDB *rdb, int *sp);


/* Get forward matching keys in a remote database object.
   `rdb' specifies the remote database object.
   `pbuf' specifies the pointer to the region of the prefix.
   `psiz' specifies the size of the region of the prefix.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding keys.  This function does never fail.
   It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcrdbfwmkeys(TCRDB *rdb, const void *pbuf, int psiz, int max);


/* Get forward matching string keys in a remote database object.
   `rdb' specifies the remote database object.
   `pstr' specifies the string of the prefix.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding keys.  This function does never fail.
   It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcrdbfwmkeys2(TCRDB *rdb, const char *pstr, int max);


/* Add an integer to a record in a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is `INT_MIN'.
   If the corresponding record exists, the value is treated as an integer and is added to.  If no
   record corresponds, a new record of the additional value is stored. */
int tcrdbaddint(TCRDB *rdb, const void *kbuf, int ksiz, int num);


/* Add a real number to a record in a remote database object.
   `rdb' specifies the remote database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is Not-a-Number.
   If the corresponding record exists, the value is treated as a real number and is added to.  If
   no record corresponds, a new record of the additional value is stored. */
double tcrdbadddouble(TCRDB *rdb, const void *kbuf, int ksiz, double num);


/* Call a function of the scripting language extension.
   `rdb' specifies the remote database object.
   `name' specifies the function name.
   `opts' specifies options by bitwise-or: `RDBXOLCKREC' for record locking, `RDBXOLCKGLB' for
   global locking.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the response.
   `NULL' is returned on failure.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcrdbext(TCRDB *rdb, const char *name, int opts,
               const void *kbuf, int ksiz, const void *vbuf, int vsiz, int *sp);


/* Synchronize updated contents of a remote database object with the file and the device.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
bool tcrdbsync(TCRDB *rdb);


/* Optimize the storage of a remove database object.
   `rdb' specifies the remote database object.
   `params' specifies the string of the tuning parameters.  If it is `NULL', it is not used.
   If successful, the return value is true, else, it is false. */
bool tcrdboptimize(TCRDB *rdb, const char *params);


/* Remove all records of a remote database object.
   `rdb' specifies the remote database object.
   If successful, the return value is true, else, it is false. */
bool tcrdbvanish(TCRDB *rdb);


/* Copy the database file of a remote database object.
   `rdb' specifies the remote database object.
   `path' specifies the path of the destination file.  If it begins with `@', the trailing
   substring is executed as a command line.
   If successful, the return value is true, else, it is false.  False is returned if the executed
   command returns non-zero code.
   The database file is assured to be kept synchronized and not modified while the copying or
   executing operation is in progress.  So, this function is useful to create a backup file of
   the database file. */
bool tcrdbcopy(TCRDB *rdb, const char *path);


/* Restore the database file of a remote database object from the update log.
   `rdb' specifies the remote database object.
   `path' specifies the path of the update log directory.
   `ts' specifies the beginning timestamp in microseconds.
   `opts' specifies options by bitwise-or: `RDBROCHKCON' for consistency checking.
   If successful, the return value is true, else, it is false. */
bool tcrdbrestore(TCRDB *rdb, const char *path, uint64_t ts, int opts);


/* Set the replication master of a remote database object.
   `rdb' specifies the remote database object.
   `host' specifies the name or the address of the server.  If it is `NULL', replication of the
   database is disabled.
   `port' specifies the port number.
   `ts' specifies the beginning timestamp in microseconds.
   `opts' specifies options by bitwise-or: `RDBROCHKCON' for consistency checking.
   If successful, the return value is true, else, it is false. */
bool tcrdbsetmst(TCRDB *rdb, const char *host, int port, uint64_t ts, int opts);


/* Set the replication master of a remote database object with a simple server expression.
   `rdb' specifies the remote database object.
   `expr' specifies the simple server expression.  It is composed of two substrings separated
   by ":".  The former field specifies the name or the address of the server.  The latter field
   specifies the port number.  If the latter field is omitted, the default port number is
   specified.
   `ts' specifies the beginning timestamp in microseconds.
   `opts' specifies options by bitwise-or: `RDBROCHKCON' for consistency checking.
   If successful, the return value is true, else, it is false. */
bool tcrdbsetmst2(TCRDB *rdb, const char *expr, uint64_t ts, int opts);


/* Get the simple server expression of an abstract database object.
   `rdb' specifies the remote database object.
   The return value is the simple server expression or `NULL' if the object does not connect to
   any database server. */
const char *tcrdbexpr(TCRDB *rdb);


/* Get the number of records of a remote database object.
   `rdb' specifies the remote database object.
   The return value is the number of records or 0 if the object does not connect to any database
   server. */
uint64_t tcrdbrnum(TCRDB *rdb);


/* Get the size of the database of a remote database object.
   `rdb' specifies the remote database object.
   The return value is the size of the database or 0 if the object does not connect to any
   database server. */
uint64_t tcrdbsize(TCRDB *rdb);


/* Get the status string of the database of a remote database object.
   `rdb' specifies the remote database object.
   The return value is the status message of the database or `NULL' if the object does not
   connect to any database server.  The message format is TSV.  The first field of each line
   means the parameter name and the second field means the value.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcrdbstat(TCRDB *rdb);


/* Call a versatile function for miscellaneous operations of a remote database object.
   `rdb' specifies the remote database object.
   `name' specifies the name of the function.  All databases support "putlist", "outlist", and
   "getlist".  "putlist" is to store records.  It receives keys and values one after the other,
   and returns an empty list.  "outlist" is to remove records.  It receives keys, and returns an
   empty list.  "getlist" is to retrieve records.  It receives keys, and returns keys and values
   of corresponding records one after the other.  Table database supports "setindex", "search",
   and "genuid".
   `opts' specifies options by bitwise-or: `RDBMONOULOG' for omission of the update log.
   `args' specifies a list object containing arguments.
   If successful, the return value is a list object of the result.  `NULL' is returned on failure.
   Because the object of the return value is created with the function `tclistnew', it
   should be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcrdbmisc(TCRDB *rdb, const char *name, int opts, const TCLIST *args);


/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


/* Set the error code of a remote database object.
   `rdb' specifies the remote database object.
   `ecode' specifies the error code. */
void tcrdbsetecode(TCRDB *rdb, int ecode);



__NET_CLINKAGEEND
#endif                                   /* duplication check */


/* END OF FILE */
