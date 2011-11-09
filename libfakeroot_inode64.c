/*
  Copyright Ⓒ 2009  Regis Duchesne

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*/

#ifdef __APPLE__
/*
   This file is for symbols which have the "$INODE64" version, i.e. symbols
   which use a 64-bit ino_t.

   In this file, 'struct stat' is an alias for 'struct stat64'.
*/
#define _DARWIN_USE_64_BIT_INODE

#include "config.h"
#include "communicate.h"

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5

#include <stdio.h>
#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#endif /* HAVE_SYS_ACL_H */
#if HAVE_FTS_H
#include <fts.h>
#endif /* HAVE_FTS_H */

#include "wrapped.h"
#include "wraptmpf.h"
#include "wrapdef.h"

#ifdef LIBFAKEROOT_DEBUGGING
extern int fakeroot_debug;

#endif /* LIBFAKEROOT_DEBUGGING */
int lstat$INODE64(const char *file_name,
          struct stat *st){

  int r;

#ifdef LIBFAKEROOT_DEBUGGING
  if (fakeroot_debug) {
    fprintf(stderr, "lstat$INODE64 file_name %s\n", file_name);
  }
#endif /* LIBFAKEROOT_DEBUGGING */
  r=next_lstat$INODE64(file_name, st);

  if(r)
    return -1;

  return FAKED_GET (FAKED_STAT64((struct stat64 *)st, 0), FAKED_LINK(file_name));
}


int stat$INODE64(const char *file_name,
         struct stat *st){
  int r;

#ifdef LIBFAKEROOT_DEBUGGING
  if (fakeroot_debug) {
    fprintf(stderr, "stat$INODE64 file_name %s\n", file_name);
  }
#endif /* LIBFAKEROOT_DEBUGGING */
  r=next_stat$INODE64(file_name,st);
  if(r)
    return -1;
  return FAKED_GET (FAKED_STAT64((struct stat64 *)st, 0), FAKED_FILE(file_name));
}


int fstat$INODE64(int fd,
          struct stat *st){
  int r;

#ifdef LIBFAKEROOT_DEBUGGING
  if (fakeroot_debug) {
    fprintf(stderr, "fstat$INODE64 fd %d\n", fd);
  }
#endif /* LIBFAKEROOT_DEBUGGING */
  r=next_fstat$INODE64(fd, st);
  if(r)
    return -1;
  send_get_stat64((struct stat64 *)st);

  return FAKED_GET (FAKED_STAT64((struct stat64 *)st, 0), FAKED_FD(FD));
}

#ifdef HAVE_FTS_READ
FTSENT *fts_read$INODE64(FTS *ftsp) {
  FTSENT *r;

#ifdef LIBFAKEROOT_DEBUGGING
  if (fakeroot_debug) {
    fprintf(stderr, "fts_read$INODE64\n");
  }
#endif /* LIBFAKEROOT_DEBUGGING */
  r=next_fts_read$INODE64(ftsp);
  if(r && r->fts_statp && r->fts_info != FTS_NSOK) {
    FAKED_GET (FAKED_STAT64((struct stat64 *)r->fts_statp, 0), FAKED_FTSENT(r));
  }

  return r;
}

FTSENT *fts_children$INODE64(FTS *ftsp,
                     int options) {
  FTSENT *first;
  FTSENT *r;

#ifdef LIBFAKEROOT_DEBUGGING
  if (fakeroot_debug) {
    fprintf(stderr, "fts_children$INODE64\n");
  }
#endif /* LIBFAKEROOT_DEBUGGING */
  first=next_fts_children$INODE64(ftsp, options);
  for(r = first; r; r = r->fts_link) {
    if(r->fts_statp && r->fts_info != FTS_NSOK) {
      FAKED_GET (FAKED_STAT64((struct stat64 *)r->fts_statp, 0), FAKED_FTSENT(r));
    }
  }

  return first;
}
#endif /* MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 */
#endif /* HAVE_FTS_READ */
#endif /* ifdef __APPLE__ */
