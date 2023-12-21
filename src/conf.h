/*************************************************************************************************
 * System-dependent configurations of Tokyo Cabinet
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


#ifndef _CONF_H                        // duplication check
#define _CONF_H


/*************************************************************************************************
 * system discrimination
 *************************************************************************************************/


#if defined(__linux__)
#define _SYS_LINUX_
#define TCSYSNAME   "Linux"
#endif

#if !defined(_SYS_LINUX_)
#error =======================================
#error Your platform is not supported.  Sorry.
#error =======================================
#endif


/*************************************************************************************************
 * common settings
 *************************************************************************************************/


#if defined(NDEBUG)
#define TCDODEBUG(TC_expr) \
  do { \
  } while(false)
#else
#define TCDODEBUG(TC_expr) \
  do { \
    TC_expr; \
  } while(false)
#endif


#if __BYTE_ORDER == __LITTLE_ENDIAN

#define TCBIGEND       0

#define htonll(TT_num)  __bswap_64(TT_num)
#define ntohll(TT_num)  __bswap_64(TT_num)

#define TCHTOIS(TC_num)   (TC_num)
#define TCHTOIL(TC_num)   (TC_num)
#define TCHTOILL(TC_num)  (TC_num)
#define TCITOHS(TC_num)   (TC_num)
#define TCITOHL(TC_num)   (TC_num)
#define TCITOHLL(TC_num)  (TC_num)

#else

#define TCBIGEND       1

#define htonll(TT_num) TT_num
#define ntohll(TT_num) TT_num

#define TCHTOIS(TC_num)   __bswap_16(TC_num)
#define TCITOHS(TC_num)   __bswap_16(TC_num)
#define TCHTOIL(TC_num)   __bswap_32(TC_num)
#define TCITOHL(TC_num)   __bswap_32(TC_num)
#define TCHTOILL(TC_num)  __bswap_64(TC_num)
#define TCITOHLL(TC_num)  __bswap_64(TC_num)

#endif


/*************************************************************************************************
 * general headers
 *************************************************************************************************/


#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <dirent.h>
#include <regex.h>
#include <glob.h>

#include <pthread.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <aio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dlfcn.h>
#include <sys/epoll.h>


/*************************************************************************************************
 * miscellaneous macros
 *************************************************************************************************/


#define MYPATHCHR       '/'
#define MYPATHSTR       "/"
#define MYEXTCHR        '.'
#define MYEXTSTR        "."
#define MYCDIRSTR       "."
#define MYPDIRSTR       ".."

#define TCNUMBUFSIZ    32                // size of a buffer for a number

#define epoll_reassoc(TC_epfd, TC_fd)  0
#define epoll_close    close


#endif                                   // duplication check

// END OF FILE
