/*
  Copyright Ⓒ 1997, 1998, 1999, 2000, 2001  joost witteveen
  Copyright Ⓒ 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009  Clint Adams

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#ifndef FAKEROOT_COMMUNICATE_H
#define FAKEROOT_COMMUNICATE_H

#include "config.h"
#include "fakerootconfig.h"

#define LCHOWN_SUPPORT

/* I've got a chicken-and-egg problem here. I want to have
   stat64 support, only running on glibc2.1 or later. To
   find out what glibc we've got installed, I need to
   #include <features.h>.
   But, before including that file, I have to define _LARGEFILE64_SOURCE
   etc, cause otherwise features.h will not define it's internal defines.
   As I assume that pre-2.1 libc's will just ignore those _LARGEFILE64_SOURCE
   defines, I hope I can get away with this approach:
*/

/*First, unconditionally define these, so that glibc 2.1 features.h defines
  the needed 64 bits defines*/
#ifndef _LARGEFILE64_SOURCE
# define _LARGEFILE64_SOURCE
#endif
#ifndef _LARGEFILE_SOURCE
# define _LARGEFILE_SOURCE
#endif

/* Then include features.h, to find out what glibc we run */
#ifdef HAVE_FEATURES_H
# include <features.h>
#endif

#ifdef HAVE_SYS_FEATURE_TESTS_H
# include <sys/feature_tests.h>
#endif

#ifdef __APPLE__
# include <AvailabilityMacros.h>
# ifndef MAC_OS_X_VERSION_10_5 1050
#  define MAC_OS_X_VERSION_10_5 1050
# endif
# ifndef MAC_OS_X_VERSION_10_6 1060
#  define MAC_OS_X_VERSION_10_6 1060
# endif
# if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
#  define HAVE_APPLE_STAT64 1
# endif
#endif

/* Then decide whether we do or do not use the stat64 support */
#if defined HAVE_APPLE_STAT64 \
	|| (defined(sun) && !defined(__SunOS_5_5_1) && !defined(_LP64)) \
	|| (!defined __UCLIBC__ && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1))) \
	|| (defined __UCLIBC__ && defined __UCLIBC_HAS_LFS__)
# define STAT64_SUPPORT
#else
# ifndef __APPLE__
#  warning Not using stat64 support
# endif
/* if glibc is 2.0 or older, undefine these again */
# undef STAT64_SUPPORT
# undef _LARGEFILE64_SOURCE
# undef _LARGEFILE_SOURCE
#endif

/* Sparc glibc 2.0.100 is broken, dlsym segfaults on --fxstat64..
#define STAT64_SUPPORT */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_FTS_H
#include <fts.h>
#endif /* HAVE_FTS_H */

#ifndef FAKEROOT_FAKENET
# define FAKEROOTKEY_ENV          "FAKEROOTKEY"
#endif /* ! FAKEROOT_FAKENET */

#define FAKEROOTUID_ENV           "FAKEROOTUID"
#define FAKEROOTGID_ENV           "FAKEROOTGID"
#define FAKEROOTEUID_ENV          "FAKEROOTEUID"
#define FAKEROOTEGID_ENV          "FAKEROOTEGID"
#define FAKEROOTSUID_ENV          "FAKEROOTSUID"
#define FAKEROOTSGID_ENV          "FAKEROOTSGID"
#define FAKEROOTFUID_ENV          "FAKEROOTFUID"
#define FAKEROOTFGID_ENV          "FAKEROOTFGID"
#define FAKEROOTDONTTRYCHOWN_ENV  "FAKEROOTDONTTRYCHOWN"

#define FAKELIBDIR                "/usr/lib/fakeroot"
#define FAKELIBNAME               "libfakeroot.so.0"
#ifdef FAKEROOT_FAKENET
# define FD_BASE_ENV              "FAKEROOT_FD_BASE"
#endif /* FAKEROOT_FAKENET */
#ifdef FAKEROOT_DB_PATH
# define DB_SEARCH_PATHS_ENV      "FAKEROOT_DB_SEARCH_PATHS"
#endif /* FAKEROOT_DB_PATH */

#ifdef __GNUC__
# define UNUSED __attribute__((unused))
#else
# define UNUSED
#endif

#ifndef S_ISTXT
# define S_ISTXT S_ISVTX
#endif

#ifndef ALLPERMS
# define ALLPERMS (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)/* 07777 */
#endif

/* Define big enough _constant size_ types for the various types of the
   stat struct. I cannot (or rather, shouldn't) use struct stat itself
   in the communication between the fake-daemon and the client (libfake),
   as the sizes elements of struct stat may depend on the compiler or
   compile time options of the C compiler, or the C library used. Thus,
   the fake-daemon may have to communicate with two clients that have
   different views of struct stat (this is the case for libc5 and
   libc6 (glibc2) compiled programmes on Linux). This currently isn't
   enabled any more, but used to be in libtricks.
*/

enum {chown_func,
        /*2*/  chmod_func,
        /*3*/  mknod_func,
               stat_func,
        /*5*/  unlink_func,
               debugdata_func,
               reqoptions_func,
               last_func};

#include "message.h"

extern const char *env_var_set(const char *env);
#ifndef STUPID_ALPHA_HACK
extern void send_stat(const struct stat *st, func_id_t f);
#else
extern void send_stat(const struct stat *st, func_id_t f,int ver);
#endif
extern void send_fakem(const struct fake_msg *buf);
#ifndef STUPID_ALPHA_HACK
extern void send_get_stat(struct stat *buf);
#else
extern void send_get_stat(struct stat *buf,int ver);
#endif
extern void cpyfakefake (struct fakestat *b1, const struct fakestat *b2);
#ifndef STUPID_ALPHA_HACK
extern void cpystatfakem(struct     stat *st, const struct fake_msg *buf);
#else
extern void cpystatfakem(struct     stat *st, const struct fake_msg *buf, int ver);
#endif

#ifndef FAKEROOT_FAKENET
extern int init_get_msg();
extern key_t get_ipc_key(key_t new_key);
# ifndef STUPID_ALPHA_HACK
extern void cpyfakemstat(struct fake_msg *b1, const struct stat *st);
# else
extern void cpyfakemstat(struct fake_msg *b1, const struct stat *st, int ver);
# endif
#else /* FAKEROOT_FAKENET */
# ifdef FAKEROOT_LIBFAKEROOT
extern volatile int comm_sd;
extern void lock_comm_sd(void);
extern void unlock_comm_sd(void);
# endif
#endif /* FAKEROOT_FAKENET */

#ifdef STAT64_SUPPORT
#ifndef STUPID_ALPHA_HACK
extern void send_stat64(const struct stat64 *st, func_id_t f);
extern void send_get_stat64(struct stat64 *buf);
#else
extern void send_stat64(const struct stat64 *st, func_id_t f, int ver);
extern void send_get_stat64(struct stat64 *buf, int ver);
#endif
extern void stat64from32(struct stat64 *s64, const struct stat *s32);
extern void stat32from64(struct stat *s32, const struct stat64 *s64);
#endif

#ifndef FAKEROOT_FAKENET
extern int msg_snd;
extern int msg_get;
extern int sem_id;
#endif /* ! FAKEROOT_FAKENET */


#define FAKEROOTUID_XATTR    "FAKEROOTUID"
#define FAKEROOTGID_XATTR    "FAKEROOTGID"
#define FAKEROOTMODE_XATTR   "FAKEROOTMODE"
#define FAKEROOTRDEV_XATTR   "FAKEROOTRDEV"

enum faked_stat_type
{
    FAKED_STAT_STAT = 1,
    FAKED_STAT_STAT64 = 2
};

struct faked_stat
{
    int type;
    union
    {
        struct stat64 *stat64;
        struct stat *stat;
    }
    stat;
};

struct faked_finfo
{
    int type;
    union
    {
        int fd;
        const char *path;
        struct
        {
            int dir_fd;
            const char *path;
            int options;
        }
        at;
        FTSENT *ftsent;
    }
    info;
};

enum faked_finfo_type
{
    FAKED_FINFO_FD = 1,
    FAKED_FINFO_FILE = 2,
    FAKED_FINFO_LINK = 3,
    FAKED_FINFO_AT = 4,
    FAKED_FINFO_FTSENT = 5
};

int faked_get(struct faked_stat, struct faked_finfo);
int faked_set(int, struct faked_stat, struct faked_finfo);
struct faked_stat faked_stat(struct stat *);
struct faked_stat faked_stat64(struct stat64 *);
struct faked_finfo faked_fd(int);
struct faked_finfo faked_file(const char *);
struct faked_finfo faked_link(const char *);
struct faked_finfo faked_at(int, const char *, int);
struct faked_finfo faked_ftsent(FTSENT *);

/*
    This is the new protocol for information exchange with faked.
    Let's define it by example:

    FAKED_GET (FAKED_STAT(st, __STAT_VER), FAKED_FILE(filepath))

    In here we pass a stat structure to faked for patching. The structure
    is stat (not stat64) of the particular version (__STAT_VER). Assume we
    had a stat64 structure instead:

    FAKED_GET (FAKED_STAT64(st64, __STAT_VER), FAKED_FILE(filepath)))

    The daemon can use st_ino member of the passed stat structure to lookup
    internal tables as faked always did. The daemon is also given a filepath
    so it has some exciting oportunities like storing data in extended attrs.
    Instead of using FAKED_FILE to identify the particular file object
    one could use FAKED_FD, FAKED_LINK, FAKED_AT and FAKED_FTSENT, ex:

    FAKED_SET (chown_func, FAKED_STAT64(st64, __STAT_VER), FAKED_FD(fd))

    In this example we are also showcasing FAKED_SET operation.

    Last but not least both FAKED_GET and FAKED_SET return 0 if succeeded
    and -1 if failed.
 */
#if 1

#define FAKED_GET(args,extra) _FAKED_GET(args)
#define FAKED_SET(func,args,extra) _FAKED_SET(func,args)

#ifdef STUPID_ALPHA_HACK
#define _FAKED_GET(suffix,st,ver)  \
    (_FAKED_GET ## suffix(st,ver), 0)
#define _FAKED_SET(func,suffix,st,ver) \
    (_FAKED_SET ## suffix(st,func,ver), 0)
#else
#define _FAKED_GET(suffix,st,ver)  \
    (_FAKED_GET ## suffix(st), 0)
#define _FAKED_SET(func,suffix,st,ver) \
    (_FAKED_SET ## suffix(st,func), 0)
#endif

#define FAKED_STAT(st, ver) _FAKED_STAT,(st),(ver)
#define FAKED_STAT64(st, ver) _FAKED_STAT64,(st),(ver)

#define FAKED_FD(fd)
#define FAKED_FILE(path)
#define FAKED_LINK(path)
#define FAKED_AT(dirfd,path,options)
#define FAKED_FTSENT(ftsent)

#define _FAKED_GET_FAKED_STAT       send_get_stat
#define _FAKED_GET_FAKED_STAT64     send_get_stat64
#define _FAKED_SET_FAKED_STAT       send_stat
#define _FAKED_SET_FAKED_STAT64     send_stat64

#define FAKED_SET_NEEDS_STAT 1

#else

#define FAKED_GET(st,finfo)            faked_get(st,finfo)
#define FAKED_SET(fn,st,finfo)         faked_set(fn,st,finfo)

#define FAKED_STAT(st,ver)             faked_stat(st)
#define FAKED_STAT64(st,ver)           faked_stat64(st)

#define FAKED_FD(fd)                   faked_fd(fd)
#define FAKED_FILE(path)               faked_file(path)
#define FAKED_LINK(path)               faked_link(path)
#define FAKED_AT(dirfd,path,options)   faked_at(dirfd,path,options)
#define FAKED_FTSENT(ftsent)           faked_ftsent(ftsent)

#undef FAKED_SET_NEEDS_STAT

#endif

#endif
