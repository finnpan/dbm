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


#include "util.h"
#include "net.h"



/*************************************************************************************************
 * basic utilities
 *************************************************************************************************/


#define SOCKPATHBUFSIZ 108               // size of a socket path buffer
#define SOCKRCVTIMEO   0.25              // timeout of the recv call of socket
#define SOCKSNDTIMEO   0.25              // timeout of the send call of socket
#define SOCKCNCTTIMEO  5.0               // timeout of the connect call of socket
#define SOCKLINEBUFSIZ 4096              // size of a line buffer of socket
#define SOCKLINEMAXSIZ (16*1024*1024)    // maximum size of a line of socket
#define HTTPBODYMAXSIZ (256*1024*1024)   // maximum size of the entity body of HTTP
#define TRILLIONNUM    1000000000000     // trillion number


/* String containing the version information. */
const char *ttversion = _TT_VERSION;


/* Get the address of a host. */
bool ttgethostaddr(const char *name, char *addr){
  assert(name && addr);
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  if(getaddrinfo(name, NULL, &hints, &result) != 0){
    *addr = '\0';
    return false;
  }
  if(!result){
    freeaddrinfo(result);
    return false;
  }
  if(result->ai_addr->sa_family != AF_INET){
    freeaddrinfo(result);
    return false;
  }
  if(getnameinfo(result->ai_addr, result->ai_addrlen,
                 addr, TTADDRBUFSIZ, NULL, 0, NI_NUMERICHOST) != 0){
    freeaddrinfo(result);
    return false;
  }
  freeaddrinfo(result);
  return true;
}


/* Open a client socket of TCP/IP stream to a server. */
int ttopensock(const char *addr, int port){
  assert(addr && port >= 0);
  struct sockaddr_in sain;
  memset(&sain, 0, sizeof(sain));
  sain.sin_family = AF_INET;
  if(inet_aton(addr, &sain.sin_addr) == 0) return -1;
  uint16_t snum = port;
  sain.sin_port = htons(snum);
  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if(fd == -1) return -1;
  int optint = 1;
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optint, sizeof(optint));
  struct timeval opttv;
  opttv.tv_sec = (int)SOCKRCVTIMEO;
  opttv.tv_usec = (SOCKRCVTIMEO - (int)SOCKRCVTIMEO) * 1000000;
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&opttv, sizeof(opttv));
  opttv.tv_sec = (int)SOCKSNDTIMEO;
  opttv.tv_usec = (SOCKSNDTIMEO - (int)SOCKSNDTIMEO) * 1000000;
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&opttv, sizeof(opttv));
  optint = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&optint, sizeof(optint));
  double dl = tctime() + SOCKCNCTTIMEO;
  do {
    int ocs = PTHREAD_CANCEL_DISABLE;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ocs);
    int rv = connect(fd, (struct sockaddr *)&sain, sizeof(sain));
    int en = errno;
    pthread_setcancelstate(ocs, NULL);
    if(rv == 0) return fd;
    if(en != EINTR && en != EAGAIN && en != EINPROGRESS && en != EALREADY && en != ETIMEDOUT)
      break;
  } while(tctime() <= dl);
  close(fd);
  return -1;
}


/* Open a client socket of UNIX domain stream to a server. */
int ttopensockunix(const char *path){
  assert(path);
  struct sockaddr_un saun;
  memset(&saun, 0, sizeof(saun));
  saun.sun_family = AF_UNIX;
  snprintf(saun.sun_path, SOCKPATHBUFSIZ, "%s", path);
  int fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if(fd == -1) return -1;
  int optint = 1;
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optint, sizeof(optint));
  struct timeval opttv;
  opttv.tv_sec = (int)SOCKRCVTIMEO;
  opttv.tv_usec = (SOCKRCVTIMEO - (int)SOCKRCVTIMEO) * 1000000;
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&opttv, sizeof(opttv));
  opttv.tv_sec = (int)SOCKSNDTIMEO;
  opttv.tv_usec = (SOCKSNDTIMEO - (int)SOCKSNDTIMEO) * 1000000;
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&opttv, sizeof(opttv));
  double dl = tctime() + SOCKCNCTTIMEO;
  do {
    int ocs = PTHREAD_CANCEL_DISABLE;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ocs);
    int rv = connect(fd, (struct sockaddr *)&saun, sizeof(saun));
    int en = errno;
    pthread_setcancelstate(ocs, NULL);
    if(rv == 0) return fd;
    if(en != EINTR && en != EAGAIN && en != EINPROGRESS && en != EALREADY && en != ETIMEDOUT)
      break;
  } while(tctime() <= dl);
  close(fd);
  return -1;
}


/* Open a server socket of TCP/IP stream to clients. */
int ttopenservsock(const char *addr, int port){
  assert(port >= 0);
  struct sockaddr_in sain;
  memset(&sain, 0, sizeof(sain));
  sain.sin_family = AF_INET;
  if(inet_aton(addr ? addr : "0.0.0.0", &sain.sin_addr) == 0) return -1;
  uint16_t snum = port;
  sain.sin_port = htons(snum);
  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if(fd == -1) return -1;
  int optint = 1;
  if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optint, sizeof(optint)) != 0){
    close(fd);
    return -1;
  }
  if(bind(fd, (struct sockaddr *)&sain, sizeof(sain)) != 0 ||
     listen(fd, SOMAXCONN) != 0){
    close(fd);
    return -1;
  }
  return fd;
}


/* Open a server socket of UNIX domain stream to clients. */
int ttopenservsockunix(const char *path){
  assert(path);
  if(*path == '\0') return -1;
  struct sockaddr_un saun;
  memset(&saun, 0, sizeof(saun));
  saun.sun_family = AF_UNIX;
  snprintf(saun.sun_path, SOCKPATHBUFSIZ, "%s", path);
  int fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if(fd == -1) return -1;
  if(bind(fd, (struct sockaddr *)&saun, sizeof(saun)) != 0 ||
     listen(fd, SOMAXCONN) != 0){
    close(fd);
    return -1;
  }
  return fd;
}


/* Accept a TCP/IP connection from a client. */
int ttacceptsock(int fd, char *addr, int *pp){
  assert(fd >= 0);
  do {
    struct sockaddr_in sain;
    memset(&sain, 0, sizeof(sain));
    sain.sin_family = AF_INET;
    socklen_t slen = sizeof(sain);
    int cfd = accept(fd, (struct sockaddr *)&sain, &slen);
    if(cfd >= 0){
      int optint = 1;
      setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optint, sizeof(optint));
      struct timeval opttv;
      opttv.tv_sec = (int)SOCKRCVTIMEO;
      opttv.tv_usec = (SOCKRCVTIMEO - (int)SOCKRCVTIMEO) * 1000000;
      setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&opttv, sizeof(opttv));
      opttv.tv_sec = (int)SOCKSNDTIMEO;
      opttv.tv_usec = (SOCKSNDTIMEO - (int)SOCKSNDTIMEO) * 1000000;
      setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&opttv, sizeof(opttv));
      optint = 1;
      setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (char *)&optint, sizeof(optint));
      if(addr){
        if(getnameinfo((struct sockaddr *)&sain, sizeof(sain), addr, TTADDRBUFSIZ,
                       NULL, 0, NI_NUMERICHOST) != 0) sprintf(addr, "0.0.0.0");
      }
      if(pp) *pp = (int)ntohs(sain.sin_port);
      return cfd;
    }
  } while(errno == EINTR || errno == EAGAIN);
  return -1;
}


/* Accept a UNIX domain connection from a client. */
int ttacceptsockunix(int fd){
  assert(fd >= 0);
  do {
    int cfd = accept(fd, NULL, NULL);
    if(cfd >= 0){
      int optint = 1;
      setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optint, sizeof(optint));
      struct timeval opttv;
      opttv.tv_sec = (int)SOCKRCVTIMEO;
      opttv.tv_usec = (SOCKRCVTIMEO - (int)SOCKRCVTIMEO) * 1000000;
      setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&opttv, sizeof(opttv));
      opttv.tv_sec = (int)SOCKSNDTIMEO;
      opttv.tv_usec = (SOCKSNDTIMEO - (int)SOCKSNDTIMEO) * 1000000;
      setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&opttv, sizeof(opttv));
      return cfd;
    }
  } while(errno == EINTR || errno == EAGAIN);
  return -1;
}


/* Shutdown and close a socket. */
bool ttclosesock(int fd){
  assert(fd >= 0);
  bool err = false;
  if(shutdown(fd, 2) != 0 && errno != ENOTCONN && errno != ECONNRESET) err = true;
  if(close(fd) != 0 && errno != ENOTCONN && errno != ECONNRESET) err = true;
  return !err;
}


/* Wait an I/O event of a socket. */
bool ttwaitsock(int fd, int mode, double timeout){
  assert(fd >= 0 && mode >= 0 && timeout >= 0);
  while(true){
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    double integ, fract;
    fract = modf(timeout, &integ);
    struct timespec ts;
    ts.tv_sec = integ;
    ts.tv_nsec = fract * 1000000000.0;
    int rv = -1;
    switch(mode){
      case 0:
        rv = pselect(fd + 1, &set, NULL, NULL, &ts, NULL);
        break;
      case 1:
        rv = pselect(fd + 1, NULL, &set, NULL, &ts, NULL);
        break;
      case 2:
        rv = pselect(fd + 1, NULL, NULL, &set, &ts, NULL);
        break;
    }
    if(rv > 0) return true;
    if(rv == 0 || errno != EINVAL) break;
  }
  return false;
}


/* Create a socket object. */
TTSOCK *ttsocknew(int fd){
  assert(fd >= 0);
  TTSOCK *sock = tcmalloc(sizeof(*sock));
  sock->fd = fd;
  sock->rp = sock->buf;
  sock->ep = sock->buf;
  sock->end = false;
  sock->to = 0.0;
  sock->dl = HUGE_VAL;
  return sock;
}


/* Delete a socket object. */
void ttsockdel(TTSOCK *sock){
  assert(sock);
  free(sock);
}


/* Set the lifetime of a socket object. */
void ttsocksetlife(TTSOCK *sock, double lifetime){
  assert(sock && lifetime >= 0);
  sock->to = lifetime >= INT_MAX ? 0.0 : lifetime;
  sock->dl = tctime() + lifetime;
}


/* Send data by a socket. */
bool ttsocksend(TTSOCK *sock, const void *buf, int size){
  assert(sock && buf && size >= 0);
  const char *rp = buf;
  do {
    int ocs = PTHREAD_CANCEL_DISABLE;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ocs);
    if(sock->to > 0.0 && !ttwaitsock(sock->fd, 1, sock->to)){
      pthread_setcancelstate(ocs, NULL);
      return false;
    }
    int wb = send(sock->fd, rp, size, 0);
    int en = errno;
    pthread_setcancelstate(ocs, NULL);
    switch(wb){
      case -1:
        if(en != EINTR && en != EAGAIN && en != EWOULDBLOCK){
          sock->end = true;
          return false;
        }
        if(tctime() > sock->dl){
          sock->end = true;
          return false;
        }
        break;
      case 0:
        break;
      default:
        rp += wb;
        size -= wb;
        break;
    }
  } while(size > 0);
  return true;
}


/* Send formatted data by a socket. */
bool ttsockprintf(TTSOCK *sock, const char *format, ...){
  assert(sock && format);
  bool err = false;
  TCXSTR *xstr = tcxstrnew();
  pthread_cleanup_push((void (*)(void *))tcxstrdel, xstr);
  va_list ap;
  va_start(ap, format);
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
      char *tmp, tbuf[TCNUMBUFSIZ*2];
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
            tlen = sprintf(tbuf, cbuf, va_arg(ap, long double));
          } else {
            tlen = sprintf(tbuf, cbuf, va_arg(ap, double));
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
        case '%':
          tcxstrcat(xstr, "%", 1);
          break;
      }
    } else {
      tcxstrcat(xstr, format, 1);
    }
    format++;
  }
  va_end(ap);
  if(!ttsocksend(sock, tcxstrptr(xstr), tcxstrsize(xstr))) err = true;
  pthread_cleanup_pop(1);
  return !err;
}


/* Receive data by a socket. */
bool ttsockrecv(TTSOCK *sock, char *buf, int size){
  assert(sock && buf && size >= 0);
  if(sock->rp + size <= sock->ep){
    memcpy(buf, sock->rp, size);
    sock->rp += size;
    return true;
  }
  bool err = false;
  char *wp = buf;
  while(size > 0){
    int c = ttsockgetc(sock);
    if(c == -1){
      err = true;
      break;
    }
    *(wp++) = c;
    size--;
  }
  return !err;
}


/* Receive one byte by a socket. */
int ttsockgetc(TTSOCK *sock){
  assert(sock);
  if(sock->rp < sock->ep) return *(unsigned char *)(sock->rp++);
  int en;
  do {
    int ocs = PTHREAD_CANCEL_DISABLE;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ocs);
    if(sock->to > 0.0 && !ttwaitsock(sock->fd, 0, sock->to)){
      pthread_setcancelstate(ocs, NULL);
      return -1;
    }
    int rv = recv(sock->fd, sock->buf, TTIOBUFSIZ, 0);
    en = errno;
    pthread_setcancelstate(ocs, NULL);
    if(rv > 0){
      sock->rp = sock->buf + 1;
      sock->ep = sock->buf + rv;
      return *(unsigned char *)sock->buf;
    } else if(rv == 0){
      sock->end = true;
      return -1;
    }
  } while((en == EINTR || en == EAGAIN || en == EWOULDBLOCK) && tctime() <= sock->dl);
  sock->end = true;
  return -1;
}


/* Push a character back to a socket. */
void ttsockungetc(TTSOCK *sock, int c){
  assert(sock);
  if(sock->rp <= sock->buf) return;
  sock->rp--;
  *(unsigned char *)sock->rp = c;
}


/* Receive one line by a socket. */
bool ttsockgets(TTSOCK *sock, char *buf, int size){
  assert(sock && buf && size > 0);
  bool err = false;
  size--;
  char *wp = buf;
  while(size > 0){
    int c = ttsockgetc(sock);
    if(c == '\n') break;
    if(c == -1){
      err = true;
      break;
    }
    if(c != '\r'){
      *(wp++) = c;
      size--;
    }
  }
  *wp = '\0';
  return !err;
}


/* Receive one line by a socket into allocated buffer. */
char *ttsockgets2(TTSOCK *sock){
  assert(sock);
  bool err = false;
  TCXSTR *xstr = tcxstrnew2(SOCKLINEBUFSIZ);
  pthread_cleanup_push((void (*)(void *))tcxstrdel, xstr);
  int size = 0;
  while(true){
    int c = ttsockgetc(sock);
    if(c == '\n') break;
    if(c == -1){
      err = true;
      break;
    }
    if(c != '\r'){
      unsigned char b = c;
      tcxstrcat(xstr, &b, sizeof(b));
      size++;
      if(size >= SOCKLINEMAXSIZ){
        err = true;
        break;
      }
    }
  }
  pthread_cleanup_pop(0);
  return tcxstrtomalloc(xstr);
}


/* Receive an 32-bit integer by a socket. */
uint32_t ttsockgetint32(TTSOCK *sock){
  assert(sock);
  uint32_t num;
  ttsockrecv(sock, (char *)&num, sizeof(num));
  return ntohl(num);
}


/* Receive an 64-bit integer by a socket. */
uint64_t ttsockgetint64(TTSOCK *sock){
  assert(sock);
  uint64_t num;
  ttsockrecv(sock, (char *)&num, sizeof(num));
  return ntohll(num);
}


/* Check whehter a socket is end. */
bool ttsockcheckend(TTSOCK *sock){
  assert(sock);
  return sock->end;
}


/* Fetch the resource of a URL by HTTP. */
int tthttpfetch(const char *url, TCMAP *reqheads, TCMAP *resheads, TCXSTR *resbody){
  assert(url);
  int code = -1;
  TCMAP *elems = tcurlbreak(url);
  pthread_cleanup_push((void (*)(void *))tcmapdel, elems);
  const char *scheme = tcmapget2(elems, "scheme");
  const char *host = tcmapget2(elems, "host");
  const char *port = tcmapget2(elems, "port");
  const char *authority = tcmapget2(elems, "authority");
  const char *path = tcmapget2(elems, "path");
  const char *query = tcmapget2(elems, "query");
  if(scheme && !tcstricmp(scheme, "http") && host){
    if(*host == '\0') host = "127.0.0.1";
    int pnum = port ? tcatoi(port) : 80;
    if(pnum < 1) pnum = 80;
    if(!path) path = "/";
    char addr[TTADDRBUFSIZ];
    int fd;
    if(ttgethostaddr(host, addr) && (fd = ttopensock(addr, pnum)) != -1){
      pthread_cleanup_push((void (*)(void *))ttclosesock, (void *)(intptr_t)fd);
      TTSOCK *sock = ttsocknew(fd);
      pthread_cleanup_push((void (*)(void *))ttsockdel, sock);
      TCXSTR *obuf = tcxstrnew();
      pthread_cleanup_push((void (*)(void *))tcxstrdel, obuf);
      if(query){
        tcxstrprintf(obuf, "GET %s?%s HTTP/1.1\r\n", path, query);
      } else {
        tcxstrprintf(obuf, "GET %s HTTP/1.1\r\n", path);
      }
      if(pnum == 80){
        tcxstrprintf(obuf, "Host: %s\r\n", host);
      } else {
        tcxstrprintf(obuf, "Host: %s:%d\r\n", host, pnum);
      }
      tcxstrprintf(obuf, "Connection: close\r\n", host, port);
      if(authority){
        char *enc = tcbaseencode(authority, strlen(authority));
        tcxstrprintf(obuf, "Authorization: Basic %s\r\n", enc);
        free(enc);
      }
      double tout = -1;
      if(reqheads){
        tcmapiterinit(reqheads);
        const char *name;
        while((name = tcmapiternext2(reqheads)) != NULL){
          if(strchr(name, ':') || !tcstricmp(name, "connection")) continue;
          if(!tcstricmp(name, "x-tt-timeout")){
            tout = tcatof(tcmapget2(reqheads, name));
          } else {
            char *cap = tcstrdup(name);
            tcstrtolower(cap);
            char *wp = cap;
            bool head = true;
            while(*wp != '\0'){
              if(head && *wp >= 'a' && *wp <= 'z') *wp -= 'a' - 'A';
              head = *wp == '-' || *wp == ' ';
              wp++;
            }
            tcxstrprintf(obuf, "%s: %s\r\n", cap, tcmapget2(reqheads, name));
            free(cap);
          }
        }
      }
      tcxstrprintf(obuf, "\r\n", host);
      if(tout > 0) ttsocksetlife(sock, tout);
      if(ttsocksend(sock, tcxstrptr(obuf), tcxstrsize(obuf))){
        char line[SOCKLINEBUFSIZ];
        if(ttsockgets(sock, line, SOCKLINEBUFSIZ) && tcstrfwm(line, "HTTP/")){
          tcstrsqzspc(line);
          const char *rp = strchr(line, ' ');
          code = tcatoi(rp + 1);
          if(resheads) tcmapput2(resheads, "STATUS", line);
        }
        if(code > 0){
          int clen = 0;
          bool chunked = false;
          while(ttsockgets(sock, line, SOCKLINEBUFSIZ) && *line != '\0'){
            tcstrsqzspc(line);
            char *pv = strchr(line, ':');
            if(!pv) continue;
            *(pv++) = '\0';
            while(*pv == ' '){
              pv++;
            }
            tcstrtolower(line);
            if(!strcmp(line, "content-length")){
              clen = tcatoi(pv);
            } else if(!strcmp(line, "transfer-encoding")){
              if(!tcstricmp(pv, "chunked")) chunked = true;
            }
            if(resheads) tcmapput2(resheads, line, pv);
          }
          if(!ttsockcheckend(sock) && resbody){
            bool err = false;
            char *body;
            int bsiz;
            if(code == 304){
              body = tcmemdup("", 0);
              bsiz = 0;
            } else if(chunked){
              int asiz = SOCKLINEBUFSIZ;
              body = tcmalloc(asiz);
              bsiz = 0;
              while(true){
                pthread_cleanup_push(free, body);
                if(!ttsockgets(sock, line, SOCKLINEBUFSIZ)) err = true;
                pthread_cleanup_pop(0);
                if(err || *line == '\0') break;
                int size = tcatoih(line);
                if(bsiz + size > HTTPBODYMAXSIZ){
                  err = true;
                  break;
                }
                if(bsiz + size > asiz){
                  asiz = bsiz * 2 + size;
                  body = tcrealloc(body, asiz);
                }
                pthread_cleanup_push(free, body);
                if(size > 0) ttsockrecv(sock, body + bsiz, size);
                if(ttsockgetc(sock) != '\r' || ttsockgetc(sock) != '\n') err = true;
                pthread_cleanup_pop(0);
                if(err || size < 1) break;
                bsiz += size;
              }
            } else if(clen > 0){
              if(clen > HTTPBODYMAXSIZ){
                body = tcmemdup("", 0);
                bsiz = 0;
                err = true;
              } else {
                body = tcmalloc(clen);
                bsiz = 0;
                pthread_cleanup_push(free, body);
                if(ttsockrecv(sock, body, clen)){
                  bsiz = clen;
                } else {
                  err = true;
                }
                pthread_cleanup_pop(0);
              }
            } else {
              int asiz = SOCKLINEBUFSIZ;
              body = tcmalloc(asiz);
              bsiz = 0;
              while(true){
                int c;
                pthread_cleanup_push(free, body);
                c = ttsockgetc(sock);
                pthread_cleanup_pop(0);
                if(c == -1) break;
                if(bsiz >= HTTPBODYMAXSIZ){
                  err = true;
                  break;
                }
                if(bsiz >= asiz){
                  asiz = bsiz * 2;
                  body = tcrealloc(body, asiz);
                }
                body[bsiz++] = c;
              }
            }
            if(err){
              code = -1;
            } else if(resbody){
              tcxstrcat(resbody, body, bsiz);
            }
            free(body);
          }
        }
      }
      pthread_cleanup_pop(1);
      pthread_cleanup_pop(1);
      pthread_cleanup_pop(1);
    }
  }
  pthread_cleanup_pop(1);
  return code;
}


/* Serialize a real number. */
void ttpackdouble(double num, char *buf){
  assert(buf);
  double dinteg, dfract;
  dfract = modf(num, &dinteg);
  int64_t linteg, lfract;
  if(isnormal(dinteg) || dinteg == 0){
    linteg = dinteg;
    lfract = dfract * TRILLIONNUM;
  } else if(isinf(dinteg)){
    linteg = dinteg > 0 ? INT64_MAX : INT64_MIN;
    lfract = 0;
  } else {
    linteg = INT64_MIN;
    lfract = INT64_MIN;
  }
  linteg = htonll(linteg);
  memcpy(buf, &linteg, sizeof(linteg));
  lfract = htonll(lfract);
  memcpy(buf + sizeof(linteg), &lfract, sizeof(lfract));
}


/* Redintegrate a serialized real number. */
double ttunpackdouble(const char *buf){
  assert(buf);
  int64_t linteg, lfract;
  memcpy(&linteg, buf, sizeof(linteg));
  linteg = ntohll(linteg);
  memcpy(&lfract, buf + sizeof(linteg), sizeof(lfract));
  lfract = ntohll(lfract);
  if(lfract == INT64_MIN && linteg == INT64_MIN){
    return NAN;
  } else if(linteg == INT64_MAX){
    return INFINITY;
  } else if(linteg == INT64_MIN){
    return -INFINITY;
  }
  return linteg + (double)lfract / TRILLIONNUM;
}



/*************************************************************************************************
 * server utilities
 *************************************************************************************************/


#define TTADDRBUFSIZ   1024              // size of an address buffer
#define TTDEFTHNUM     5                 // default number of threads
#define TTEVENTMAX     256               // maximum number of events
#define TTWAITREQUEST  0.2               // waiting seconds for requests
#define TTWAITWORKER   0.1               // waiting seconds for finish of workers


/* private function prototypes */
static void *ttservtimer(void *argp);
static void ttservtask(TTSOCK *sock, TTREQ *req);
static void *ttservdeqtasks(void *argp);


/* Create a server object. */
TTSERV *ttservnew(void){
  TTSERV *serv = tcmalloc(sizeof(*serv));
  serv->host[0] = '\0';
  serv->addr[0] = '\0';
  serv->port = 0;
  serv->queue = tclistnew();
  if(pthread_mutex_init(&serv->qmtx, NULL) != 0) tcmyfatal("pthread_mutex_init failed");
  if(pthread_cond_init(&serv->qcnd, NULL) != 0) tcmyfatal("pthread_cond_init failed");
  if(pthread_mutex_init(&serv->tmtx, NULL) != 0) tcmyfatal("pthread_mutex_init failed");
  if(pthread_cond_init(&serv->tcnd, NULL) != 0) tcmyfatal("pthread_cond_init failed");
  serv->thnum = TTDEFTHNUM;
  serv->timeout = 0;
  serv->term = false;
  serv->do_log = NULL;
  serv->opq_log = NULL;
  serv->timernum = 0;
  serv->do_task = NULL;
  serv->opq_task = NULL;
  serv->do_term = NULL;
  serv->opq_term = NULL;
  return serv;
}


/* Delete a server object. */
void ttservdel(TTSERV *serv){
  assert(serv);
  pthread_cond_destroy(&serv->tcnd);
  pthread_mutex_destroy(&serv->tmtx);
  pthread_cond_destroy(&serv->qcnd);
  pthread_mutex_destroy(&serv->qmtx);
  tclistdel(serv->queue);
  free(serv);
}


/* Configure a server object. */
bool ttservconf(TTSERV *serv, const char *host, int port){
  assert(serv);
  bool err = false;
  if(port < 1){
    if(!host || host[0] == '\0'){
      err = true;
      serv->addr[0] = '\0';
      ttservlog(serv, TTLOGERROR, "invalid socket path");
    }
  } else {
    if(host && !ttgethostaddr(host, serv->addr)){
      err = true;
      serv->addr[0] = '\0';
      ttservlog(serv, TTLOGERROR, "ttgethostaddr failed");
    }
  }
  snprintf(serv->host, sizeof(serv->host), "%s", host ? host : "");
  serv->port = port;
  return !err;
}


/* Set tuning parameters of a server object. */
void ttservtune(TTSERV *serv, int thnum, double timeout){
  assert(serv && thnum > 0);
  serv->thnum = thnum;
  serv->timeout = timeout;
}


/* Set the logging handler of a server object. */
void ttservsetloghandler(TTSERV *serv, void (*do_log)(int, const char *, void *), void *opq){
  assert(serv && do_log);
  serv->do_log = do_log;
  serv->opq_log = opq;
}


/* Add a timed handler to a server object. */
void ttservaddtimedhandler(TTSERV *serv, double freq, void (*do_timed)(void *), void *opq){
  assert(serv && freq >= 0.0 && do_timed);
  if(serv->timernum >= TTTIMERMAX - 1) return;
  TTTIMER *timer = serv->timers + serv->timernum;
  timer->freq_timed = freq;
  timer->do_timed = do_timed;
  timer->opq_timed = opq;
  serv->timernum++;
}


/* Set the response handler of a server object. */
void ttservsettaskhandler(TTSERV *serv, void (*do_task)(TTSOCK *, void *, TTREQ *), void *opq){
  assert(serv && do_task);
  serv->do_task = do_task;
  serv->opq_task = opq;
}


/* Set the termination handler of a server object. */
void ttservsettermhandler(TTSERV *serv, void (*do_term)(void *), void *opq){
  assert(serv && do_term);
  serv->do_term = do_term;
  serv->opq_term = opq;
}


/* Start the service of a server object. */
bool ttservstart(TTSERV *serv){
  assert(serv);
  int lfd;
  if(serv->port < 1){
    lfd = ttopenservsockunix(serv->host);
    if(lfd == -1){
      ttservlog(serv, TTLOGERROR, "ttopenservsockunix failed");
      return false;
    }
  } else {
    lfd = ttopenservsock(serv->addr[0] != '\0' ? serv->addr : NULL, serv->port);
    if(lfd == -1){
      ttservlog(serv, TTLOGERROR, "ttopenservsock failed");
      return false;
    }
  }
  int epfd = epoll_create(TTEVENTMAX);
  if(epfd == -1){
    close(lfd);
    ttservlog(serv, TTLOGERROR, "epoll_create failed");
    return false;
  }
  ttservlog(serv, TTLOGSYSTEM, "service started: %d", getpid());
  bool err = false;
  for(int i = 0; i < serv->timernum; i++){
    TTTIMER *timer = serv->timers + i;
    timer->alive = false;
    timer->serv = serv;
    if(pthread_create(&(timer->thid), NULL, ttservtimer, timer) == 0){
      ttservlog(serv, TTLOGINFO, "timer thread %d started", i + 1);
      timer->alive = true;
    } else {
      ttservlog(serv, TTLOGERROR, "pthread_create (ttservtimer) failed");
      err = true;
    }
  }
  int thnum = serv->thnum;
  TTREQ reqs[thnum];
  for(int i = 0; i < thnum; i++){
    reqs[i].alive = true;
    reqs[i].serv = serv;
    reqs[i].epfd = epfd;
    reqs[i].mtime = tctime();
    reqs[i].keep = false;
    reqs[i].idx = i;
    if(pthread_create(&reqs[i].thid, NULL, ttservdeqtasks, reqs + i) == 0){
      ttservlog(serv, TTLOGINFO, "worker thread %d started", i + 1);
    } else {
      reqs[i].alive = false;
      err = true;
      ttservlog(serv, TTLOGERROR, "pthread_create (ttservdeqtasks) failed");
    }
  }
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = EPOLLIN;
  ev.data.fd = lfd;
  if(epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "epoll_ctl failed");
  }
  ttservlog(serv, TTLOGSYSTEM, "listening started");
  while(!serv->term){
    struct epoll_event events[TTEVENTMAX];
    int fdnum = epoll_wait(epfd, events, TTEVENTMAX, TTWAITREQUEST * 1000);
    if(fdnum != -1){
      for(int i = 0; i < fdnum; i++){
        if(events[i].data.fd == lfd){
          char addr[TTADDRBUFSIZ];
          int port;
          int cfd;
          if(serv->port < 1){
            cfd = ttacceptsockunix(lfd);
            sprintf(addr, "(unix)");
            port = 0;
          } else {
            cfd = ttacceptsock(lfd, addr, &port);
          }
          if(cfd != -1){
            ttservlog(serv, TTLOGINFO, "connected: %s:%d", addr, port);
            struct epoll_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLIN | EPOLLONESHOT;
            ev.data.fd = cfd;
            if(epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) != 0){
              close(cfd);
              err = true;
              ttservlog(serv, TTLOGERROR, "epoll_ctl failed");
            }
          } else {
            err = true;
            ttservlog(serv, TTLOGERROR, "ttacceptsock failed");
            if(epoll_ctl(epfd, EPOLL_CTL_DEL, lfd, NULL) != 0)
              ttservlog(serv, TTLOGERROR, "epoll_ctl failed");
            if(close(lfd) != 0) ttservlog(serv, TTLOGERROR, "close failed");
            tcsleep(TTWAITWORKER);
            if(serv->port < 1){
              lfd = ttopenservsockunix(serv->host);
              if(lfd == -1) ttservlog(serv, TTLOGERROR, "ttopenservsockunix failed");
            } else {
              lfd = ttopenservsock(serv->addr[0] != '\0' ? serv->addr : NULL, serv->port);
              if(lfd == -1) ttservlog(serv, TTLOGERROR, "ttopenservsock failed");
            }
            if(lfd >= 0){
              memset(&ev, 0, sizeof(ev));
              ev.events = EPOLLIN;
              ev.data.fd = lfd;
              if(epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) == 0){
                ttservlog(serv, TTLOGSYSTEM, "listening restarted");
              } else {
                ttservlog(serv, TTLOGERROR, "epoll_ctl failed");
              }
            }
          }
        } else {
          int cfd = events[i].data.fd;
          if(pthread_mutex_lock(&serv->qmtx) == 0){
            tclistpush(serv->queue, &cfd, sizeof(cfd));
            if(pthread_mutex_unlock(&serv->qmtx) != 0){
              err = true;
              ttservlog(serv, TTLOGERROR, "pthread_mutex_unlock failed");
            }
            if(pthread_cond_signal(&serv->qcnd) != 0){
              err = true;
              ttservlog(serv, TTLOGERROR, "pthread_cond_signal failed");
            }
          } else {
            err = true;
            ttservlog(serv, TTLOGERROR, "pthread_mutex_lock failed");
          }
        }
      }
    } else {
      if(errno == EINTR){
        ttservlog(serv, TTLOGINFO, "signal interruption");
      } else {
        err = true;
        ttservlog(serv, TTLOGERROR, "epoll_wait failed");
      }
    }
    if(serv->timeout > 0){
      double ctime = tctime();
      for(int i = 0; i < thnum; i++){
        double itime = ctime - reqs[i].mtime;
        if(itime > serv->timeout + TTWAITREQUEST + SOCKRCVTIMEO + SOCKSNDTIMEO &&
           pthread_cancel(reqs[i].thid) == 0){
          ttservlog(serv, TTLOGINFO, "worker thread %d canceled by timeout", i + 1);
          void *rv;
          if(pthread_join(reqs[i].thid, &rv) == 0){
            if(rv && rv != PTHREAD_CANCELED) err = true;
            reqs[i].mtime = tctime();
            if(pthread_create(&reqs[i].thid, NULL, ttservdeqtasks, reqs + i) != 0){
              reqs[i].alive = false;
              err = true;
              ttservlog(serv, TTLOGERROR, "pthread_create (ttservdeqtasks) failed");
            } else {
              ttservlog(serv, TTLOGINFO, "worker thread %d started", i + 1);
            }
          } else {
            reqs[i].alive = false;
            err = true;
            ttservlog(serv, TTLOGERROR, "pthread_join failed");
          }
        }
      }
    }
  }
  ttservlog(serv, TTLOGSYSTEM, "listening finished");
  if(pthread_cond_broadcast(&serv->qcnd) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "pthread_cond_broadcast failed");
  }
  if(pthread_cond_broadcast(&serv->tcnd) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "pthread_cond_broadcast failed");
  }
  tcsleep(TTWAITWORKER);
  if(serv->do_term) serv->do_term(serv->opq_term);
  for(int i = 0; i < thnum; i++){
    if(!reqs[i].alive) continue;
    if(pthread_cancel(reqs[i].thid) == 0)
      ttservlog(serv, TTLOGINFO, "worker thread %d was canceled", i + 1);
    void *rv;
    if(pthread_join(reqs[i].thid, &rv) == 0){
      ttservlog(serv, TTLOGINFO, "worker thread %d finished", i + 1);
      if(rv && rv != PTHREAD_CANCELED) err = true;
    } else {
      err = true;
      ttservlog(serv, TTLOGERROR, "pthread_join failed");
    }
  }
  if(tclistnum(serv->queue) > 0)
    ttservlog(serv, TTLOGINFO, "%d requests discarded", tclistnum(serv->queue));
  tclistclear(serv->queue);
  for(int i = 0; i < serv->timernum; i++){
    TTTIMER *timer = serv->timers + i;
    if(!timer->alive) continue;
    void *rv;
    if(pthread_cancel(timer->thid) == 0)
      ttservlog(serv, TTLOGINFO, "timer thread %d was canceled", i + 1);
    if(pthread_join(timer->thid, &rv) == 0){
      ttservlog(serv, TTLOGINFO, "timer thread %d finished", i + 1);
      if(rv && rv != PTHREAD_CANCELED) err = true;
    } else {
      err = true;
      ttservlog(serv, TTLOGERROR, "pthread_join failed");
    }
  }
  if(close(epfd) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "epoll_close failed");
  }
  if(serv->port < 1 && unlink(serv->host) == -1){
    err = true;
    ttservlog(serv, TTLOGERROR, "unlink failed");
  }
  if(lfd >= 0 && close(lfd) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "close failed");
  }
  ttservlog(serv, TTLOGSYSTEM, "service finished");
  serv->term = false;
  return !err;
}


/* Send the terminate signal to a server object. */
bool ttservkill(TTSERV *serv){
  assert(serv);
  serv->term = true;
  return true;
}


/* Call the logging function of a server object. */
void ttservlog(TTSERV *serv, int level, const char *format, ...){
  assert(serv && format);
  if(!serv->do_log) return;
  char buf[TTIOBUFSIZ];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, TTIOBUFSIZ, format, ap);
  va_end(ap);
  serv->do_log(level, buf, serv->opq_log);
}


/* Check whether a server object is killed. */
bool ttserviskilled(TTSERV *serv){
  assert(serv);
  return serv->term;
}


/* Break a simple server expression. */
char *ttbreakservexpr(const char *expr, int *pp){
  assert(expr);
  char *host = tcstrdup(expr);
  char *pv = strchr(host, '#');
  if(pv) *pv = '\0';
  int port = -1;
  pv = strchr(host, ':');
  if(pv){
    *(pv++) = '\0';
    if(*pv >= '0' && *pv <= '9') port = tcatoi(pv);
  }
  if(port < 0) port = TTDEFPORT;
  if(pp) *pp = port;
  tcstrtrim(host);
  if(*host == '\0'){
    free(host);
    host = tcstrdup("127.0.0.1");
  }
  return host;
}


/* Call the timed function of a server object.
   `argp' specifies the argument structure of the server object.
   The return value is `NULL' on success and other on failure. */
static void *ttservtimer(void *argp){
  TTTIMER *timer = argp;
  TTSERV *serv = timer->serv;
  bool err = false;
  if(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "pthread_setcancelstate failed");
  }
  tcsleep(TTWAITWORKER);
  double freqi;
  double freqd = modf(timer->freq_timed, &freqi);
  while(!serv->term){
    if(pthread_mutex_lock(&serv->tmtx) == 0){
      struct timeval tv;
      struct timespec ts;
      if(gettimeofday(&tv, NULL) == 0){
        ts.tv_sec = tv.tv_sec + (int)freqi;
        ts.tv_nsec = tv.tv_usec * 1000.0 + freqd * 1000000000.0;
        if(ts.tv_nsec >= 1000000000){
          ts.tv_nsec -= 1000000000;
          ts.tv_sec++;
        }
      } else {
        ts.tv_sec = (1ULL << (sizeof(time_t) * 8 - 1)) - 1;
        ts.tv_nsec = 0;
      }
      int code = pthread_cond_timedwait(&serv->tcnd, &serv->tmtx, &ts);
      if(code == 0 || code == ETIMEDOUT || code == EINTR){
        if(pthread_mutex_unlock(&serv->tmtx) != 0){
          err = true;
          ttservlog(serv, TTLOGERROR, "pthread_mutex_unlock failed");
          break;
        }
        if(code != 0 && !serv->term) timer->do_timed(timer->opq_timed);
      } else {
        pthread_mutex_unlock(&serv->tmtx);
        err = true;
        ttservlog(serv, TTLOGERROR, "pthread_cond_timedwait failed");
      }
    } else {
      err = true;
      ttservlog(serv, TTLOGERROR, "pthread_mutex_lock failed");
    }
  }
  return err ? "error" : NULL;
}


/* Call the task function of a server object.
   `req' specifies the request object.
   `sock' specifies the socket object. */
static void ttservtask(TTSOCK *sock, TTREQ *req){
  TTSERV *serv = req->serv;
  if(!serv->do_task) return;
  serv->do_task(sock, serv->opq_task, req);
}


/* Dequeue tasks of a server object and dispatch them.
   `argp' specifies the argument structure of the server object.
   The return value is `NULL' on success and other on failure. */
static void *ttservdeqtasks(void *argp){
  TTREQ *req = argp;
  TTSERV *serv = req->serv;
  bool err = false;
  if(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "pthread_setcancelstate failed");
  }
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGPIPE);
  sigset_t oldsigset;
  sigemptyset(&sigset);
  if(pthread_sigmask(SIG_BLOCK, &sigset, &oldsigset) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "pthread_sigmask failed");
  }
  bool empty = false;
  while(!serv->term){
    if(pthread_mutex_lock(&serv->qmtx) == 0){
      struct timeval tv;
      struct timespec ts;
      if(gettimeofday(&tv, NULL) == 0){
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000.0 + TTWAITREQUEST * 1000000000.0;
        if(ts.tv_nsec >= 1000000000){
          ts.tv_nsec -= 1000000000;
          ts.tv_sec++;
        }
      } else {
        ts.tv_sec = (1ULL << (sizeof(time_t) * 8 - 1)) - 1;
        ts.tv_nsec = 0;
      }
      int code = empty ? pthread_cond_timedwait(&serv->qcnd, &serv->qmtx, &ts) : 0;
      if(code == 0 || code == ETIMEDOUT || code == EINTR){
        void *val = tclistshift(serv->queue);
        if(pthread_mutex_unlock(&serv->qmtx) != 0){
          err = true;
          ttservlog(serv, TTLOGERROR, "pthread_mutex_unlock failed");
        }
        if(val){
          empty = false;
          int cfd = *(int *)val;
          free(val);
          pthread_cleanup_push((void (*)(void *))close, (void *)(intptr_t)cfd);
          TTSOCK *sock = ttsocknew(cfd);
          pthread_cleanup_push((void (*)(void *))ttsockdel, sock);
          bool reuse;
          do {
            if(serv->timeout > 0) ttsocksetlife(sock, serv->timeout);
            req->mtime = tctime();
            req->keep = false;
            ttservtask(sock, req);
            reuse = false;
            if(sock->end){
              req->keep = false;
            } else if(sock->ep > sock->rp){
              reuse = true;
            }
          } while(reuse);
          pthread_cleanup_pop(1);
          pthread_cleanup_pop(0);
          if(req->keep){
            struct epoll_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLIN | EPOLLONESHOT;
            ev.data.fd = cfd;
            if(epoll_ctl(req->epfd, EPOLL_CTL_MOD, cfd, &ev) != 0){
              close(cfd);
              err = true;
              ttservlog(serv, TTLOGERROR, "epoll_ctl failed");
            }
          } else {
            if(epoll_ctl(req->epfd, EPOLL_CTL_DEL, cfd, NULL) != 0){
              err = true;
              ttservlog(serv, TTLOGERROR, "epoll_ctl failed");
            }
            if(!ttclosesock(cfd)){
              err = true;
              ttservlog(serv, TTLOGERROR, "close failed");
            }
            ttservlog(serv, TTLOGINFO, "connection finished");
          }
        } else {
          empty = true;
        }
      } else {
        pthread_mutex_unlock(&serv->qmtx);
        err = true;
        ttservlog(serv, TTLOGERROR, "pthread_cond_timedwait failed");
      }
    } else {
      err = true;
      ttservlog(serv, TTLOGERROR, "pthread_mutex_lock failed");
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    req->mtime = tctime();
  }
  if(pthread_sigmask(SIG_SETMASK, &oldsigset, NULL) != 0){
    err = true;
    ttservlog(serv, TTLOGERROR, "pthread_sigmask failed");
  }
  return err ? "error" : NULL;
}



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


#define NULLDEV        "/dev/null"       // path of the null device


/* Switch the process into the background. */
bool ttdaemonize(void){
  fflush(stdout);
  fflush(stderr);
  switch(fork()){
    case -1: return false;
    case 0: break;
    default: _exit(0);
  }
  if(setsid() == -1) return false;
  switch(fork()){
    case -1: return false;
    case 0: break;
    default: _exit(0);
  }
  umask(0);
  if(chdir(MYPATHSTR) == -1) return false;
  close(0);
  close(1);
  close(2);
  int fd = open(NULLDEV, O_RDWR, 0);
  if(fd != -1){
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    if(fd > 2) close(fd);
  }
  return true;
}


/* Get the load average of the system. */
double ttgetloadavg(void){
  double avgs[3];
  int anum = getloadavg(avgs, sizeof(avgs) / sizeof(*avgs));
  if(anum < 1) return 0.0;
  return anum == 1 ? avgs[0] : avgs[1];
}


/* Convert a string to a time stamp. */
uint64_t ttstrtots(const char *str){
  assert(str);
  if(!tcstricmp(str, "now")) str = "-1";
  int64_t ts = tcatoi(str);
  if(ts < 0) ts = tctime() * 1000000;
  return ts;
}


/* Get the command name of a command ID number. */
const char *ttcmdidtostr(int id){
  switch(id){
    case TTCMDPUT: return "put";
    case TTCMDPUTKEEP: return "putkeep";
    case TTCMDPUTCAT: return "putcat";
    case TTCMDPUTNR: return "putnr";
    case TTCMDOUT: return "out";
    case TTCMDGET: return "get";
    case TTCMDMGET: return "mget";
    case TTCMDVSIZ: return "vsiz";
    case TTCMDITERINIT: return "iterinit";
    case TTCMDITERNEXT: return "iternext";
    case TTCMDFWMKEYS: return "fwmkeys";
    case TTCMDADDINT: return "addint";
    case TTCMDADDDOUBLE: return "adddouble";
    case TTCMDVANISH: return "vanish";
    case TTCMDRESTORE: return "restore";
    case TTCMDSETMST: return "setmst";
    case TTCMDRNUM: return "rnum";
    case TTCMDSIZE: return "size";
    case TTCMDSTAT: return "stat";
    case TTCMDMISC: return "misc";
    case TTCMDREPL: return "repl";
  }
  return "(unknown)";
}



#define TCULAIOCBNUM   64                // number of AIO tasks
#define TCULTMDEVALW   30.0              // allowed time deviance
#define TCREPLTIMEO    60.0              // timeout of the replication socket


/* private function prototypes */
static bool tculogflushaiocbp(struct aiocb *aiocbp);



/*************************************************************************************************
 * API
 *************************************************************************************************/


/* Create an update log object. */
TCULOG *tculognew(void){
  TCULOG *ulog = tcmalloc(sizeof(*ulog));
  for(int i = 0; i < TCULRMTXNUM; i++){
    if(pthread_mutex_init(ulog->rmtxs + i, NULL) != 0) tcmyfatal("pthread_mutex_init failed");
  }
  if(pthread_rwlock_init(&ulog->rwlck, NULL) != 0) tcmyfatal("pthread_rwlock_init failed");
  if(pthread_cond_init(&ulog->cnd, NULL) != 0) tcmyfatal("pthread_cond_init failed");
  if(pthread_mutex_init(&ulog->wmtx, NULL) != 0) tcmyfatal("pthread_mutex_init failed");
  ulog->base = NULL;
  ulog->limsiz = 0;
  ulog->max = 0;
  ulog->fd = -1;
  ulog->size = 0;
  ulog->aiocbs = NULL;
  ulog->aiocbi = 0;
  ulog->aioend = 0;
  return ulog;
}


/* Delete an update log object. */
void tculogdel(TCULOG *ulog){
  assert(ulog);
  if(ulog->base) tculogclose(ulog);
  if(ulog->aiocbs) free(ulog->aiocbs);
  pthread_mutex_destroy(&ulog->wmtx);
  pthread_cond_destroy(&ulog->cnd);
  pthread_rwlock_destroy(&ulog->rwlck);
  for(int i = TCULRMTXNUM - 1; i >= 0; i--){
    pthread_mutex_destroy(ulog->rmtxs + i);
  }
  free(ulog);
}


/* Set AIO control of an update log object. */
bool tculogsetaio(TCULOG *ulog){
#if defined(_SYS_LINUX_)
  assert(ulog);
  if(ulog->base || ulog->aiocbs) return false;
  struct aiocb *aiocbs = tcmalloc(sizeof(*aiocbs) * TCULAIOCBNUM);
  for(int i = 0; i < TCULAIOCBNUM; i++){
    memset(aiocbs + i, 0, sizeof(*aiocbs));
  }
  ulog->aiocbs = aiocbs;
  return true;
#else
  assert(ulog);
  return true;
#endif
}


/* Open files of an update log object. */
bool tculogopen(TCULOG *ulog, const char *base, uint64_t limsiz){
  assert(ulog && base);
  if(ulog->base) return false;
  struct stat sbuf;
  if(stat(base, &sbuf) == -1 || !S_ISDIR(sbuf.st_mode)) return false;
  TCLIST *names = tcreaddir(base);
  if(!names) return false;
  int ln = tclistnum(names);
  int max = 0;
  for(int i = 0; i < ln; i++){
    const char *name = tclistval2(names, i);
    if(!tcstrbwm(name, TCULSUFFIX)) continue;
    int id = tcatoi(name);
    char *path = tcsprintf("%s/%08d%s", base, id, TCULSUFFIX);
    if(stat(path, &sbuf) == 0 && S_ISREG(sbuf.st_mode) && id > max) max = id;
    free(path);
  }
  tclistdel(names);
  if(max < 1) max = 1;
  ulog->base = tcstrdup(base);
  ulog->limsiz = (limsiz > 0) ? limsiz : INT64_MAX / 2;
  ulog->max = max;
  ulog->fd = -1;
  ulog->size = sbuf.st_size;
  struct aiocb *aiocbs = ulog->aiocbs;
  if(aiocbs){
    for(int i = 0; i < TCULAIOCBNUM; i++){
      struct aiocb *aiocbp = aiocbs + i;
      aiocbp->aio_fildes = 0;
      aiocbp->aio_buf = NULL;
      aiocbp->aio_nbytes = 0;
    }
  }
  ulog->aiocbi = 0;
  ulog->aioend = 0;
  return true;
}


/* Close files of an update log object. */
bool tculogclose(TCULOG *ulog){
  assert(ulog);
  if(!ulog->base) return false;
  bool err = false;
  struct aiocb *aiocbs = ulog->aiocbs;
  if(aiocbs){
    for(int i = 0; i < TCULAIOCBNUM; i++){
      if(!tculogflushaiocbp(aiocbs + i)) err = true;
    }
  }
  if(ulog->fd != -1 && close(ulog->fd) != 0) err = true;
  free(ulog->base);
  ulog->base = NULL;
  return !err;
}


/* Get the mutex index of a record. */
int tculogrmtxidx(TCULOG *ulog, const char *kbuf, int ksiz){
  assert(ulog && kbuf && ksiz >= 0);
  if(!ulog->base || !ulog->aiocbs) return 0;
  uint32_t hash = 19780211;
  while(ksiz--){
    hash = hash * 41 + *(uint8_t *)kbuf++;
  }
  return hash % TCULRMTXNUM;
}


/* Begin the critical section of an update log object. */
bool tculogbegin(TCULOG *ulog, int idx){
  assert(ulog);
  if(!ulog->base) return false;
  if(idx < 0){
    for(int i = 0; i < TCULRMTXNUM; i++){
      if(pthread_mutex_lock(ulog->rmtxs + i) != 0){
        for(i--; i >= 0; i--){
          pthread_mutex_unlock(ulog->rmtxs + i);
        }
        return false;
      }
    }
    return true;
  }
  return pthread_mutex_lock(ulog->rmtxs + idx) == 0;
}


/* End the critical section of an update log object. */
bool tculogend(TCULOG *ulog, int idx){
  assert(ulog);
  if(idx < 0){
    bool err = false;
    for(int i = TCULRMTXNUM - 1; i >= 0; i--){
      if(pthread_mutex_unlock(ulog->rmtxs + i) != 0) err = true;
    }
    return !err;
  }
  return pthread_mutex_unlock(ulog->rmtxs + idx) == 0;
}


/* Write a message into an update log object. */
bool tculogwrite(TCULOG *ulog, uint64_t ts, uint32_t sid, uint32_t mid,
                 const void *ptr, int size){
  assert(ulog && ptr && size >= 0);
  if(!ulog->base) return false;
  if(ts < 1) ts = (uint64_t)(tctime() * 1000000);
  bool err = false;
  if(pthread_rwlock_wrlock(&ulog->rwlck) != 0) return false;
  pthread_cleanup_push((void (*)(void *))pthread_rwlock_unlock, &ulog->rwlck);
  if(ulog->fd == -1){
    char *path = tcsprintf("%s/%08d%s", ulog->base, ulog->max, TCULSUFFIX);
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 00644);
    free(path);
    struct stat sbuf;
    if(fd != -1 && fstat(fd, &sbuf) == 0){
      ulog->fd = fd;
      ulog->size = sbuf.st_size;
    } else {
      err = true;
    }
  }
  int rsiz = sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint32_t) * 2 + size;
  unsigned char stack[TTIOBUFSIZ];
  unsigned char *buf = (rsiz < TTIOBUFSIZ) ? stack : tcmalloc(rsiz);
  pthread_cleanup_push(free, (buf == stack) ? NULL : buf);
  unsigned char *wp = buf;
  *(wp++) = TCULMAGICNUM;
  uint64_t llnum = htonll(ts);
  memcpy(wp, &llnum, sizeof(llnum));
  wp += sizeof(llnum);
  uint16_t snum = htons(sid);
  memcpy(wp, &snum, sizeof(snum));
  wp += sizeof(snum);
  snum = htons(mid);
  memcpy(wp, &snum, sizeof(snum));
  wp += sizeof(snum);
  uint32_t lnum = htonl(size);
  memcpy(wp, &lnum, sizeof(lnum));
  wp += sizeof(lnum);
  memcpy(wp, ptr, size);
  if(ulog->fd != -1){
    struct aiocb *aiocbs = (struct aiocb *)ulog->aiocbs;
    if(aiocbs){
      struct aiocb *aiocbp = aiocbs + ulog->aiocbi;
      if(aiocbp->aio_buf){
        off_t aioend = aiocbp->aio_offset + aiocbp->aio_nbytes;
        if(tculogflushaiocbp(aiocbp)){
          ulog->aioend = aioend;
        } else {
          err = true;
        }
      }
      aiocbp->aio_fildes = ulog->fd;
      aiocbp->aio_offset = ulog->size;
      aiocbp->aio_buf = tcmemdup(buf, rsiz);
      aiocbp->aio_nbytes = rsiz;
      while(aio_write(aiocbp) != 0){
        if(errno != EAGAIN){
          free((char *)aiocbp->aio_buf);
          aiocbp->aio_buf = NULL;
          err = true;
          break;
        }
        for(int i = 0; i < TCULAIOCBNUM; i++){
          if(i == ulog->aiocbi) continue;
          if(!tculogflushaiocbp(aiocbs + i)){
            err = true;
            break;
          }
        }
      }
      ulog->aiocbi = (ulog->aiocbi + 1) % TCULAIOCBNUM;
    } else {
      if(!tcwrite(ulog->fd, buf, rsiz)) err = true;
    }
    if(!err){
      ulog->size += rsiz;
      if(ulog->size >= ulog->limsiz){
        if(aiocbs){
          for(int i = 0; i < TCULAIOCBNUM; i++){
            if(!tculogflushaiocbp(aiocbs + i)) err = true;
          }
          ulog->aiocbi = 0;
          ulog->aioend = 0;
        }
        char *path = tcsprintf("%s/%08d%s", ulog->base, ulog->max + 1, TCULSUFFIX);
        int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 00644);
        free(path);
        if(fd != 0){
          if(close(ulog->fd) != 0) err = true;
          ulog->fd = fd;
          ulog->size = 0;
          ulog->max++;
        } else {
          err = true;
        }
      }
      if(pthread_cond_broadcast(&ulog->cnd) != 0) err = true;
    }
  } else {
    err = true;
  }
  pthread_cleanup_pop(1);
  pthread_cleanup_pop(1);
  return !err;
}


/* Create a log reader object. */
TCULRD *tculrdnew(TCULOG *ulog, uint64_t ts){
  assert(ulog);
  if(!ulog->base) return NULL;
  if(pthread_rwlock_rdlock(&ulog->rwlck) != 0) return NULL;
  TCLIST *names = tcreaddir(ulog->base);
  if(!names){
    pthread_rwlock_unlock(&ulog->rwlck);
    return NULL;
  }
  int ln = tclistnum(names);
  int max = 0;
  for(int i = 0; i < ln; i++){
    const char *name = tclistval2(names, i);
    if(!tcstrbwm(name, TCULSUFFIX)) continue;
    int id = tcatoi(name);
    char *path = tcsprintf("%s/%08d%s", ulog->base, id, TCULSUFFIX);
    struct stat sbuf;
    if(stat(path, &sbuf) == 0 && S_ISREG(sbuf.st_mode) && id > max) max = id;
    free(path);
  }
  tclistdel(names);
  if(max < 1) max = 1;
  uint64_t bts = (ts > TCULTMDEVALW * 1000000) ? ts - TCULTMDEVALW * 1000000 : 0;
  int num = 0;
  for(int i = max; i > 0; i--){
    char *path = tcsprintf("%s/%08d%s", ulog->base, i, TCULSUFFIX);
    int fd = open(path, O_RDONLY, 00644);
    free(path);
    if(fd == -1) break;
    int rsiz = sizeof(uint8_t) + sizeof(uint64_t);
    unsigned char buf[rsiz];
    uint64_t fts = INT64_MAX;
    if(tcread(fd, buf, rsiz)){
      memcpy(&fts, buf + sizeof(uint8_t), sizeof(ts));
      fts = ntohll(fts);
    }
    close(fd);
    num = i;
    if(bts >= fts) break;
  }
  if(num < 1) num = 1;
  TCULRD *urld = tcmalloc(sizeof(*urld));
  urld->ulog = ulog;
  urld->ts = ts;
  urld->num = num;
  urld->fd = -1;
  urld->rbuf = tcmalloc(TTIOBUFSIZ);
  urld->rsiz = TTIOBUFSIZ;
  pthread_rwlock_unlock(&ulog->rwlck);
  return urld;
}


/* Delete a log reader object. */
void tculrddel(TCULRD *ulrd){
  assert(ulrd);
  if(ulrd->fd != -1) close(ulrd->fd);
  free(ulrd->rbuf);
  free(ulrd);
}


/* Wait the next message is written. */
void tculrdwait(TCULRD *ulrd){
  assert(ulrd);
  TCULOG *ulog = ulrd->ulog;
  if(pthread_mutex_lock(&ulog->wmtx) != 0) return;
  pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &ulog->wmtx);
  int ocs = PTHREAD_CANCEL_DISABLE;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ocs);
  struct timeval tv;
  struct timespec ts;
  if(gettimeofday(&tv, NULL) == 0){
    ts.tv_sec = tv.tv_sec + 1;
    ts.tv_nsec = tv.tv_usec * 1000;
  } else {
    ts.tv_sec = (1ULL << (sizeof(time_t) * 8 - 1)) - 1;
    ts.tv_nsec = 0;
  }
  pthread_cond_timedwait(&ulog->cnd, &ulog->wmtx, &ts);
  pthread_setcancelstate(ocs, NULL);
  pthread_cleanup_pop(1);
}


/* Read a message from a log reader object. */
const void *tculrdread(TCULRD *ulrd, int *sp, uint64_t *tsp, uint32_t *sidp, uint32_t *midp){
  assert(ulrd && sp && tsp && sidp && midp);
  TCULOG *ulog = ulrd->ulog;
  if(pthread_rwlock_rdlock(&ulog->rwlck) != 0) return NULL;
  if(ulrd->fd == -1){
    char *path = tcsprintf("%s/%08d%s", ulog->base, ulrd->num, TCULSUFFIX);
    ulrd->fd = open(path, O_RDONLY, 00644);
    free(path);
    if(ulrd->fd == -1){
      pthread_rwlock_unlock(&ulog->rwlck);
      return NULL;
    }
  }
  int rsiz = sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint32_t) * 2;
  unsigned char buf[rsiz];
  uint64_t ts;
  uint32_t sid, mid, size;
  while(true){
    if(ulog->aiocbs && ulrd->num == ulog->max){
      struct stat sbuf;
      if(fstat(ulrd->fd, &sbuf) == -1 ||
         (sbuf.st_size < ulog->size && sbuf.st_size >= ulog->aioend)){
        pthread_rwlock_unlock(&ulog->rwlck);
        return NULL;
      }
    }
    if(!tcread(ulrd->fd, buf, rsiz)){
      if(ulrd->num < ulog->max){
        close(ulrd->fd);
        ulrd->num++;
        char *path = tcsprintf("%s/%08d%s", ulog->base, ulrd->num, TCULSUFFIX);
        ulrd->fd = open(path, O_RDONLY, 00644);
        free(path);
        if(ulrd->fd == -1){
          pthread_rwlock_unlock(&ulog->rwlck);
          return NULL;
        }
        continue;
      }
      pthread_rwlock_unlock(&ulog->rwlck);
      return NULL;
    }
    const unsigned char *rp = buf;
    if(*rp != TCULMAGICNUM){
      pthread_rwlock_unlock(&ulog->rwlck);
      return NULL;
    }
    rp += sizeof(uint8_t);
    memcpy(&ts, rp, sizeof(ts));
    ts = ntohll(ts);
    rp += sizeof(ts);
    uint16_t snum;
    memcpy(&snum, rp, sizeof(snum));
    sid = ntohs(snum);
    rp += sizeof(snum);
    memcpy(&snum, rp, sizeof(snum));
    mid = ntohs(snum);
    rp += sizeof(snum);
    memcpy(&size, rp, sizeof(size));
    size = ntohl(size);
    rp += sizeof(size);
    if(ulrd->rsiz < size + 1){
      ulrd->rbuf = tcrealloc(ulrd->rbuf, size + 1);
      ulrd->rsiz = size + 1;
    }
    if(!tcread(ulrd->fd, ulrd->rbuf, size)){
      pthread_rwlock_unlock(&ulog->rwlck);
      return NULL;
    }
    if(ts < ulrd->ts) continue;
    break;
  }
  *sp = size;
  *tsp = ts;
  *sidp = sid;
  *midp = mid;
  ulrd->rbuf[size] = '\0';
  pthread_rwlock_unlock(&ulog->rwlck);
  return ulrd->rbuf;
}


/* Store a record into a database object. */
bool tculogdbput(TCULOG *ulog, uint32_t sid, uint32_t mid, TCMDB *mdb,
                  const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(ulog && mdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  bool err = false;
  int rmidx = tculogrmtxidx(ulog, kbuf, ksiz);
  bool dolog = tculogbegin(ulog, rmidx);
  tcmdbput(mdb, kbuf, ksiz, vbuf, vsiz);
  if(dolog){
    unsigned char mstack[TTIOBUFSIZ];
    int msiz = sizeof(uint8_t) * 3 + sizeof(uint32_t) * 2 + ksiz + vsiz;
    unsigned char *mbuf = (msiz < TTIOBUFSIZ) ? mstack : tcmalloc(msiz + 1);
    unsigned char *wp = mbuf;
    *(wp++) = TTMAGICNUM;
    *(wp++) = TTCMDPUT;
    uint32_t lnum;
    lnum = htonl(ksiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    lnum = htonl(vsiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    memcpy(wp, kbuf, ksiz);
    wp += ksiz;
    memcpy(wp, vbuf, vsiz);
    wp += vsiz;
    *(wp++) = err ? 1 : 0;
    if(!tculogwrite(ulog, 0, sid, mid, mbuf, msiz)) err = true;
    if(mbuf != mstack) free(mbuf);
    tculogend(ulog, rmidx);
  }
  return !err;
}


/* Store a new record into a database object. */
bool tculogdbputkeep(TCULOG *ulog, uint32_t sid, uint32_t mid, TCMDB *mdb,
                      const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(ulog && mdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  bool err = false;
  int rmidx = tculogrmtxidx(ulog, kbuf, ksiz);
  bool dolog = tculogbegin(ulog, rmidx);
  if(!tcmdbputkeep(mdb, kbuf, ksiz, vbuf, vsiz)) err = true;
  if(dolog){
    unsigned char mstack[TTIOBUFSIZ];
    int msiz = sizeof(uint8_t) * 3 + sizeof(uint32_t) * 2 + ksiz + vsiz;
    unsigned char *mbuf = (msiz < TTIOBUFSIZ) ? mstack : tcmalloc(msiz + 1);
    unsigned char *wp = mbuf;
    *(wp++) = TTMAGICNUM;
    *(wp++) = TTCMDPUTKEEP;
    uint32_t lnum;
    lnum = htonl(ksiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    lnum = htonl(vsiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    memcpy(wp, kbuf, ksiz);
    wp += ksiz;
    memcpy(wp, vbuf, vsiz);
    wp += vsiz;
    *(wp++) = err ? 1 : 0;
    if(!tculogwrite(ulog, 0, sid, mid, mbuf, msiz)) err = true;
    if(mbuf != mstack) free(mbuf);
    tculogend(ulog, rmidx);
  }
  return !err;
}


/* Concatenate a value at the end of the existing record in a database object. */
bool tculogdbputcat(TCULOG *ulog, uint32_t sid, uint32_t mid, TCMDB *mdb,
                     const void *kbuf, int ksiz, const void *vbuf, int vsiz){
  assert(ulog && mdb && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  bool err = false;
  int rmidx = tculogrmtxidx(ulog, kbuf, ksiz);
  bool dolog = tculogbegin(ulog, rmidx);
  tcmdbputcat(mdb, kbuf, ksiz, vbuf, vsiz);
  if(dolog){
    unsigned char mstack[TTIOBUFSIZ];
    int msiz = sizeof(uint8_t) * 3 + sizeof(uint32_t) * 2 + ksiz + vsiz;
    unsigned char *mbuf = (msiz < TTIOBUFSIZ) ? mstack : tcmalloc(msiz + 1);
    unsigned char *wp = mbuf;
    *(wp++) = TTMAGICNUM;
    *(wp++) = TTCMDPUTCAT;
    uint32_t lnum;
    lnum = htonl(ksiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    lnum = htonl(vsiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    memcpy(wp, kbuf, ksiz);
    wp += ksiz;
    memcpy(wp, vbuf, vsiz);
    wp += vsiz;
    *(wp++) = err ? 1 : 0;
    if(!tculogwrite(ulog, 0, sid, mid, mbuf, msiz)) err = true;
    if(mbuf != mstack) free(mbuf);
    tculogend(ulog, rmidx);
  }
  return !err;
}


/* Remove a record of a database object. */
bool tculogdbout(TCULOG *ulog, uint32_t sid, uint32_t mid, TCMDB *mdb,
                  const void *kbuf, int ksiz){
  assert(ulog && mdb && kbuf && ksiz >= 0);
  bool err = false;
  int rmidx = tculogrmtxidx(ulog, kbuf, ksiz);
  bool dolog = tculogbegin(ulog, rmidx);
  if(!tcmdbout(mdb, kbuf, ksiz)) err = true;
  if(dolog){
    unsigned char mstack[TTIOBUFSIZ];
    int msiz = sizeof(uint8_t) * 3 + sizeof(uint32_t) + ksiz;
    unsigned char *mbuf = (msiz < TTIOBUFSIZ) ? mstack : tcmalloc(msiz + 1);
    unsigned char *wp = mbuf;
    *(wp++) = TTMAGICNUM;
    *(wp++) = TTCMDOUT;
    uint32_t lnum;
    lnum = htonl(ksiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    memcpy(wp, kbuf, ksiz);
    wp += ksiz;
    *(wp++) = err ? 1 : 0;
    if(!tculogwrite(ulog, 0, sid, mid, mbuf, msiz)) err = true;
    if(mbuf != mstack) free(mbuf);
    tculogend(ulog, rmidx);
  }
  return !err;
}


/* Add an integer to a record in a database object. */
int tculogdbaddint(TCULOG *ulog, uint32_t sid, uint32_t mid, TCMDB *mdb,
                    const void *kbuf, int ksiz, int num){
  assert(ulog && mdb && kbuf && ksiz >= 0);
  int rmidx = tculogrmtxidx(ulog, kbuf, ksiz);
  bool dolog = num != 0 && tculogbegin(ulog, rmidx);
  int rnum = tcmdbaddint(mdb, kbuf, ksiz, num);
  if(dolog){
    unsigned char mstack[TTIOBUFSIZ];
    int msiz = sizeof(uint8_t) * 3 + sizeof(uint32_t) * 2 + ksiz;
    unsigned char *mbuf = (msiz < TTIOBUFSIZ) ? mstack : tcmalloc(msiz + 1);
    unsigned char *wp = mbuf;
    *(wp++) = TTMAGICNUM;
    *(wp++) = TTCMDADDINT;
    uint32_t lnum;
    lnum = htonl(ksiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    lnum = htonl(num);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    memcpy(wp, kbuf, ksiz);
    wp += ksiz;
    *(wp++) = (rnum == INT_MIN) ? 1 : 0;
    if(!tculogwrite(ulog, 0, sid, mid, mbuf, msiz)) rnum = INT_MIN;
    if(mbuf != mstack) free(mbuf);
    tculogend(ulog, rmidx);
  }
  return rnum;
}


/* Add a real number to a record in a database object. */
double tculogdbadddouble(TCULOG *ulog, uint32_t sid, uint32_t mid, TCMDB *mdb,
                          const void *kbuf, int ksiz, double num){
  assert(ulog && mdb && kbuf && ksiz >= 0);
  int rmidx = tculogrmtxidx(ulog, kbuf, ksiz);
  bool dolog = num != 0 && tculogbegin(ulog, rmidx);
  double rnum = tcmdbadddouble(mdb, kbuf, ksiz, num);
  if(dolog){
    unsigned char mstack[TTIOBUFSIZ];
    int msiz = sizeof(uint8_t) * 3 + sizeof(uint32_t) + sizeof(uint64_t) * 2 + ksiz;
    unsigned char *mbuf = (msiz < TTIOBUFSIZ) ? mstack : tcmalloc(msiz + 1);
    unsigned char *wp = mbuf;
    *(wp++) = TTMAGICNUM;
    *(wp++) = TTCMDADDDOUBLE;
    uint32_t lnum;
    lnum = htonl(ksiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    ttpackdouble(num, (char *)wp);
    wp += sizeof(uint64_t) * 2;
    memcpy(wp, kbuf, ksiz);
    wp += ksiz;
    *(wp++) = isnan(rnum) ? 1 : 0;
    if(!tculogwrite(ulog, 0, sid, mid, mbuf, msiz)) rnum = INT_MIN;
    if(mbuf != mstack) free(mbuf);
    tculogend(ulog, rmidx);
  }
  return rnum;
}


/* Remove all records of a database object. */
bool tculogdbvanish(TCULOG *ulog, uint32_t sid, uint32_t mid, TCMDB *mdb){
  assert(ulog && mdb);
  bool err = false;
  bool dolog = tculogbegin(ulog, -1);
  tcmdbvanish(mdb);
  if(dolog){
    unsigned char mbuf[sizeof(uint8_t)*3];
    unsigned char *wp = mbuf;
    *(wp++) = TTMAGICNUM;
    *(wp++) = TTCMDVANISH;
    *(wp++) = err ? 1 : 0;
    if(!tculogwrite(ulog, 0, sid, mid, mbuf, wp - mbuf)) err = true;
    tculogend(ulog, -1);
  }
  return !err;
}


/* Call a versatile function for miscellaneous operations of a database object. */
TCLIST *tculogdbmisc(TCULOG *ulog, uint32_t sid, uint32_t mid, TCMDB *mdb,
                      const char *name, const TCLIST *args){
  assert(ulog && mdb && name && args);
  bool dolog = tculogbegin(ulog, -1);
  TCLIST *rv = tcmdbmisc(mdb, name, args);
  if(dolog){
    int nsiz = strlen(name);
    int anum = tclistnum(args);
    int msiz = sizeof(uint8_t) * 3 + sizeof(uint32_t) * 2 + nsiz;
    for(int i = 0; i < anum; i++){
      int esiz;
      tclistval(args, i, &esiz);
      msiz += sizeof(uint32_t) + esiz;
    }
    unsigned char mstack[TTIOBUFSIZ];
    unsigned char *mbuf = (msiz < TTIOBUFSIZ) ? mstack : tcmalloc(msiz + 1);
    unsigned char *wp = mbuf;
    *(wp++) = TTMAGICNUM;
    *(wp++) = TTCMDMISC;
    uint32_t lnum;
    lnum = htonl(nsiz);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    lnum = htonl(anum);
    memcpy(wp, &lnum, sizeof(lnum));
    wp += sizeof(lnum);
    memcpy(wp, name, nsiz);
    wp += nsiz;
    for(int i = 0; i < anum; i++){
      int esiz;
      const char *ebuf = tclistval(args, i, &esiz);
      lnum = htonl(esiz);
      memcpy(wp, &lnum, sizeof(lnum));
      wp += sizeof(lnum);
      memcpy(wp, ebuf, esiz);
      wp += esiz;
    }
    *(wp++) = rv ? 0 : 1;
    if(!tculogwrite(ulog, 0, sid, mid, mbuf, msiz)){
      if(rv) tclistdel(rv);
      rv = NULL;
    }
    if(mbuf != mstack) free(mbuf);
    tculogend(ulog, -1);
  }
  return rv;
}


/* Restore a database object. */
bool tculogdbrestore(TCMDB *mdb, const char *path, uint64_t ts, bool con, TCULOG *ulog){
  assert(mdb && path);
  bool err = false;
  TCULOG *sulog = tculognew();
  if(tculogopen(sulog, path, 0)){
    TCULRD *ulrd = tculrdnew(sulog, ts);
    if(ulrd){
      const char *rbuf;
      int rsiz;
      uint64_t rts;
      uint32_t rsid, rmid;
      while((rbuf = tculrdread(ulrd, &rsiz, &rts, &rsid, &rmid)) != NULL){
        bool cc;
        if(!tculogdbredo(mdb, rbuf, rsiz, ulog, rsid, rmid, &cc) || (con && !cc)){
          err = true;
          break;
        }
      }
      tculrddel(ulrd);
    } else {
      err = true;
    }
    if(!tculogclose(sulog)) err = true;
  } else {
    err = true;
  }
  tculogdel(sulog);
  return !err;
}


/* Redo an update log message. */
bool tculogdbredo(TCMDB *mdb, const char *ptr, int size, TCULOG *ulog,
                   uint32_t sid, uint32_t mid, bool *cp){
  assert(mdb && ptr && size >= 0);
  if(size < sizeof(uint8_t) * 3) return false;
  const unsigned char *rp = (unsigned char *)ptr;
  int magic = *(rp++);
  int cmd = *(rp++);
  bool exp = (((unsigned char *)ptr)[size-1] == 0) ? true : false;
  size -= sizeof(uint8_t) * 3;
  if(magic != TTMAGICNUM) return false;
  bool err = false;
  *cp = true;
  switch(cmd){
    case TTCMDPUT:
      if(size >= sizeof(uint32_t) * 2){
        uint32_t ksiz;
        memcpy(&ksiz, rp, sizeof(ksiz));
        ksiz = ntohl(ksiz);
        rp += sizeof(ksiz);
        uint32_t vsiz;
        memcpy(&vsiz, rp, sizeof(vsiz));
        vsiz = ntohl(vsiz);
        rp += sizeof(vsiz);
        if(tculogdbput(ulog, sid, mid, mdb, rp, ksiz, rp + ksiz, vsiz) != exp) *cp = false;
      } else {
        err = true;
      }
      break;
    case TTCMDPUTKEEP:
      if(size >= sizeof(uint32_t) * 2){
        uint32_t ksiz;
        memcpy(&ksiz, rp, sizeof(ksiz));
        ksiz = ntohl(ksiz);
        rp += sizeof(ksiz);
        uint32_t vsiz;
        memcpy(&vsiz, rp, sizeof(vsiz));
        vsiz = ntohl(vsiz);
        rp += sizeof(vsiz);
        if(tculogdbputkeep(ulog, sid, mid, mdb, rp, ksiz, rp + ksiz, vsiz) != exp) *cp = false;
      } else {
        err = true;
      }
      break;
    case TTCMDPUTCAT:
      if(size >= sizeof(uint32_t) * 2){
        uint32_t ksiz;
        memcpy(&ksiz, rp, sizeof(ksiz));
        ksiz = ntohl(ksiz);
        rp += sizeof(ksiz);
        uint32_t vsiz;
        memcpy(&vsiz, rp, sizeof(vsiz));
        vsiz = ntohl(vsiz);
        rp += sizeof(vsiz);
        if(tculogdbputcat(ulog, sid, mid, mdb, rp, ksiz, rp + ksiz, vsiz) != exp) *cp = false;
      } else {
        err = true;
      }
      break;
    case TTCMDOUT:
      if(size >= sizeof(uint32_t)){
        uint32_t ksiz;
        memcpy(&ksiz, rp, sizeof(ksiz));
        ksiz = ntohl(ksiz);
        rp += sizeof(ksiz);
        if(tculogdbout(ulog, sid, mid, mdb, rp, ksiz) != exp) *cp = false;
      } else {
        err = true;
      }
      break;
    case TTCMDADDINT:
      if(size >= sizeof(uint32_t) * 2){
        uint32_t ksiz;
        memcpy(&ksiz, rp, sizeof(ksiz));
        ksiz = ntohl(ksiz);
        rp += sizeof(ksiz);
        int32_t num;
        memcpy(&num, rp, sizeof(num));
        num = ntohl(num);
        rp += sizeof(num);
        int rnum = tculogdbaddint(ulog, sid, mid, mdb, rp, ksiz, num);
        if(exp && rnum == INT_MIN) *cp = false;
      } else {
        err = true;
      }
      break;
    case TTCMDADDDOUBLE:
      if(size >= sizeof(uint32_t) + sizeof(uint64_t) * 2){
        uint32_t ksiz;
        memcpy(&ksiz, rp, sizeof(ksiz));
        ksiz = ntohl(ksiz);
        rp += sizeof(ksiz);
        double num = ttunpackdouble((char *)rp);
        rp += sizeof(uint64_t) * 2;
        double rnum = tculogdbadddouble(ulog, sid, mid, mdb, rp, ksiz, num);
        if(exp && isnan(rnum)) *cp = false;
      } else {
        err = true;
      }
      break;
    case TTCMDVANISH:
      if(size == 0){
        if(tculogdbvanish(ulog, sid, mid, mdb) != exp) *cp = false;
      } else {
        err = true;
      }
      break;
    case TTCMDMISC:
      if(size >= sizeof(uint32_t) * 2){
        uint32_t nsiz;
        memcpy(&nsiz, rp, sizeof(nsiz));
        nsiz = ntohl(nsiz);
        rp += sizeof(nsiz);
        uint32_t anum;
        memcpy(&anum, rp, sizeof(anum));
        anum = ntohl(anum);
        rp += sizeof(anum);
        char *name = tcmemdup(rp, nsiz);
        rp += nsiz;
        TCLIST *args = tclistnew2(anum);
        for(int i = 0; i < anum; i++){
          uint32_t esiz;
          memcpy(&esiz, rp, sizeof(esiz));
          esiz = ntohl(esiz);
          rp += sizeof(esiz);
          tclistpush(args, rp, esiz);
          rp += esiz;
        }
        TCLIST *res = tculogdbmisc(ulog, sid, mid, mdb, name, args);
        if(res){
          if(!exp) *cp = false;
          tclistdel(res);
        } else {
          if(exp) *cp = false;
        }
        tclistdel(args);
        free(name);
      } else {
        err = true;
      }
      break;
    default:
      err = true;
      break;
  }
  return !err;
}


/* Create a replication object. */
TCREPL *tcreplnew(void){
  TCREPL *repl = tcmalloc(sizeof(*repl));
  repl->fd = -1;
  repl->sock = NULL;
  return repl;
}


/* Delete a replication object. */
void tcrepldel(TCREPL *repl){
  assert(repl);
  if(repl->fd >= 0) tcreplclose(repl);
  free(repl);
}


/* Open a replication object. */
bool tcreplopen(TCREPL *repl, const char *host, int port, uint64_t ts, uint32_t sid){
  assert(repl && host && port >= 0);
  if(repl->fd >= 0) return false;
  if(ts < 1) ts = 1;
  if(sid < 1) sid = INT_MAX;
  char addr[TTADDRBUFSIZ];
  if(!ttgethostaddr(host, addr)) return false;
  int fd = ttopensock(addr, port);
  if(fd == -1) return false;
  unsigned char buf[TTIOBUFSIZ];
  unsigned char *wp = buf;
  *(wp++) = TTMAGICNUM;
  *(wp++) = TTCMDREPL;
  uint64_t llnum = htonll(ts);
  memcpy(wp, &llnum, sizeof(llnum));
  wp += sizeof(llnum);
  uint32_t lnum = htonl(sid);
  memcpy(wp, &lnum, sizeof(lnum));
  wp += sizeof(lnum);
  repl->fd = fd;
  repl->sock = ttsocknew(fd);
  repl->rbuf = tcmalloc(TTIOBUFSIZ);
  repl->rsiz = TTIOBUFSIZ;
  if(!ttsocksend(repl->sock, buf, wp - buf)){
    tcreplclose(repl);
    return false;
  }
  repl->mid = ttsockgetint32(repl->sock);
  if(ttsockcheckend(repl->sock) || repl->mid < 1){
    tcreplclose(repl);
    return false;
  }
  return true;
}


/* Close a remote database object. */
bool tcreplclose(TCREPL *repl){
  assert(repl);
  if(repl->fd < 0) return false;
  bool err = false;
  free(repl->rbuf);
  ttsockdel(repl->sock);
  if(!ttclosesock(repl->fd)) err = true;
  repl->fd = -1;
  repl->sock = NULL;
  return !err;
}


/* Read a message from a replication object. */
const char *tcreplread(TCREPL *repl, int *sp, uint64_t *tsp, uint32_t *sidp){
  assert(repl && sp && tsp);
  int ocs = PTHREAD_CANCEL_DISABLE;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &ocs);
  ttsocksetlife(repl->sock, TCREPLTIMEO);
  int c = ttsockgetc(repl->sock);
  if(c == TCULMAGICNOP){
    *sp = 0;
    *tsp = 0;
    *sidp = 0;
    return "";
  }
  if(c != TCULMAGICNUM){
    pthread_setcancelstate(ocs, NULL);
    return NULL;
  }
  uint64_t ts = ttsockgetint64(repl->sock);
  uint32_t sid = ttsockgetint32(repl->sock);
  uint32_t rsiz = ttsockgetint32(repl->sock);
  if(repl->rsiz < rsiz + 1){
    repl->rbuf = tcrealloc(repl->rbuf, rsiz + 1);
    repl->rsiz = rsiz + 1;
  }
  if(ttsockcheckend(repl->sock) || !ttsockrecv(repl->sock, repl->rbuf, rsiz) ||
     ttsockcheckend(repl->sock)){
    pthread_setcancelstate(ocs, NULL);
    return NULL;
  }
  *sp = rsiz;
  *tsp = ts;
  *sidp = sid;
  pthread_setcancelstate(ocs, NULL);
  return repl->rbuf;
}


/* Flush a AIO task.
   `aiocbp' specifies the pointer to the AIO task object.
   If successful, the return value is true, else, it is false. */
static bool tculogflushaiocbp(struct aiocb *aiocbp){
  assert(aiocbp);
  if(!aiocbp->aio_buf) return true;
  bool err = false;
  while(true){
    int rv = aio_error(aiocbp);
    if(rv == 0) break;
    if(rv != EINPROGRESS){
      err = true;
      break;
    }
    if(aio_suspend((void *)&aiocbp, 1, NULL) == -1) err = true;
  }
  free((char *)aiocbp->aio_buf);
  aiocbp->aio_buf = NULL;
  if(aio_return(aiocbp) != aiocbp->aio_nbytes) err = true;
  return !err;
}


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
static bool tcrdbvanishimpl(TCRDB *rdb);
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
  if(rdb->expr) free(rdb->expr);
  if(rdb->host) free(rdb->host);
  pthread_key_delete(rdb->eckey);
  pthread_mutex_destroy(&rdb->mmtx);
  free(rdb);
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
      const char *elem = tclistval0(elems, i);
      pv = strchr(elem, '=');
      if(!pv) continue;
      *(pv++) = '\0';
      if(!tcstricmp(elem, "host") || !tcstricmp(elem, "name")){
        free(host);
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
  free(host);
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
  free(host);
  return !err;
}


/* Get the simple server expression of a database object. */
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
    tcrdbsetecode(rdb, TTEMISC);
    return false;
  }
  return true;
}


/* Unlock a method of the remote database object.
   `rdb' specifies the remote database object. */
static void tcrdbunlockmethod(TCRDB *rdb){
  assert(rdb);
  if(pthread_mutex_unlock(&rdb->mmtx) != 0) tcrdbsetecode(rdb, TTEMISC);
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
  if(rdb->host) free(rdb->host);
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
  free(rdb->expr);
  free(rdb->host);
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
          free(vbuf);
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
          if(rbuf != stack) free(rbuf);
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
          free(vbuf);
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
          if(kbuf != (char *)stack) free(kbuf);
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


/* Get the simple server expression of a database object.
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
          if(ebuf != stack) free(ebuf);
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
