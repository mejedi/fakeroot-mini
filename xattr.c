#include "config.h"
#include "communicate.h"

#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#endif /* HAVE_SYS_ACL_H */
#if HAVE_FTS_H
#include <fts.h>
#endif /* HAVE_FTS_H */

#include <sys/xattr.h>

#include "wrapped.h"
#include "wraptmpf.h"
#include "wrapdef.h"


static int check_attr(const char *name)
{
    return strcmp(FAKEROOTUID_XATTR, name) != 0
        && strcmp(FAKEROOTGID_XATTR, name) != 0
        && strcmp(FAKEROOTMODE_XATTR, name) != 0
        && strcmp(FAKEROOTRDEV_XATTR, name) != 0;
}

ssize_t getxattr(const char *path, const char *name, void *value, size_t size, u_int32_t position, int options)
{
    if (check_attr(name)) {
        return next_getxattr(path, name, value, size, position, options);
    }
    errno = ENOATTR;
    return -1;
}

ssize_t fgetxattr(int fd, const char *name, void *value, size_t size, u_int32_t position, int options)
{
    if (check_attr(name)) {
        return next_fgetxattr(fd, name, value, size, position, options);
    }
    errno = ENOATTR;
    return -1;
}

static ssize_t filter_attrlist(char *namebuf, ssize_t size)
{
    char *in = namebuf, *out = namebuf, *end = namebuf + size;
    while (in != end) {
        size_t namesz = strlen(in) + 1;
        if (check_attr(in)) {
            memmove(out, in, namesz);
            out += namesz;
        }
        in += namesz;
    }
    return out - namebuf;
}

#define LISTXATTRBUF_SZ 1024

static ssize_t my_listxattr_estimate(const char *path, int options);

ssize_t listxattr(const char *path, char *namebuf, size_t size, int options)
{
    ssize_t st = next_listxattr(path, namebuf, size, options);
    if (st != -1) {
        if (namebuf && size)
            st = filter_attrlist(namebuf, st);
        else {
            /*
             * This is probably redundant since listxattr is permited to
             * return non-zero value even if there are no xattrs when in
             * estimation mode (namebuf=NULL). However some programs
             * call listxattr with namebuf=NULL to check for xattr
             * presence (ex: ls -l).
             */
            if (my_listxattr_estimate(path, options) == 0)
                st = 0;
        }
    }
    /*
     * If they ask for xattr listing with a valid buffer of a small size
     * but there were FAKEROOT* xattrs hence the call failed with ERANGE
     * we don't treat this as error.
     */
    if (st == -1 && namebuf && size>0 && size<=LISTXATTRBUF_SZ && errno == ERANGE
        && my_listxattr_estimate(path, options) == 0
    )
        st = 0;
    return st;
}

/*
 * Callstack often looks like that
 * my_listxattr->my_listxattr->..->listxattr,
 * I wonder whether this is an artifact of interposing or we have a bug
 * somewhere. Here we attempt to reduce stack pressure.
 */
static ssize_t my_listxattr_estimate(const char *path, int options)
{
    char buf[LISTXATTRBUF_SZ];
    return listxattr(path, buf, sizeof buf, options);
}

static ssize_t my_flistxattr_estimate(int fd, int options);

ssize_t flistxattr(int fd, char *namebuf, size_t size, int options)
{
    ssize_t st = next_flistxattr(fd, namebuf, size, options);
    if (st != -1) {
        if (namebuf)
            st = filter_attrlist(namebuf, st);
        else {
            if (my_flistxattr_estimate(fd, options) == 0)
                st = 0;
        }
    }
    if (st == -1 && namebuf && size>0 && size<=LISTXATTRBUF_SZ && errno == ERANGE
        && my_flistxattr_estimate(fd, options) == 0
    )
        st = 0;
    return st;
}

static ssize_t my_flistxattr_estimate(int fd, int options)
{
    char buf[LISTXATTRBUF_SZ];
    return flistxattr(fd, buf, sizeof buf, options);
}

int setxattr(const char *path, const char *name, const void *value, size_t size, u_int32_t position, int options)
{
    if (check_attr(name)) {
        return next_setxattr(path, name, value, size, position, options);
    }
    errno = EPERM;
    return -1;
}

int fsetxattr(int fd, const char *name, const void *value, size_t size, u_int32_t position, int options)
{
    if (check_attr(name)) {
        return next_fsetxattr(fd, name, value, size, position, options);
    }
    errno = EPERM;
    return -1;
}

int removexattr(const char *path, const char *name, int options)
{
    if (check_attr(name)) {
        return next_removexattr(path, name, options);
    }
    errno = EPERM;
    return -1;
}

int fremovexattr(int fd, const char *name, int options)
{
    if (check_attr(name)) {
        return next_fremovexattr(fd, name, options);
    }
    errno = EPERM;
    return -1;
}
