/*
   In this file, we want 'struct stat' to have a 32-bit 'ino_t'.
   We use 'struct stat64' when we need a 64-bit 'ino_t'.
*/
#define _DARWIN_NO_64_BIT_INODE
#include "communicate.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/acl.h>
#include <sys/xattr.h>

#include "wrapped.h"
#include "wraptmpf.h"
#include "wrapdef.h"


static int getxattr_helper(const struct faked_finfo *fi, const char *name, intmax_t *pvalue)
{
    char buf[128];
    ssize_t sz;
    int fts_info;

    switch (fi->type) {
    case FAKED_FINFO_FD:
        sz = next_fgetxattr(fi->info.fd, name, buf, sizeof buf, 0, 0);
        break;
    case FAKED_FINFO_FILE:
        sz = next_getxattr(fi->info.path, name, buf, sizeof buf, 0, 0);
        break;
    case FAKED_FINFO_LINK:
        sz = next_getxattr(fi->info.path, name, buf, sizeof buf, 0, XATTR_NOFOLLOW);
        break;
    case FAKED_FINFO_FTSENT:
        fts_info = fi->info.ftsent->fts_info;
        sz = next_getxattr(fi->info.ftsent->fts_accpath, name, buf, sizeof buf, 0,
                           fts_info == FTS_SL || fts_info == FTS_SLNONE ? XATTR_NOFOLLOW : 0);
        break;
    default:
        fprintf(stderr, "unexpected finfo type (%d)\n", fi->type);
        errno = ENOSYS;
        return -1;
    }

    if (sz == -1) {
        switch (errno) {
        case ENOATTR:
            *pvalue = -1;
            return 0;
        default:
            return -1;
        }
    }

    buf[sz] = '\0';
    *pvalue = strtoimax(buf, 0, 10);
    return 0;
}

static int setxattr_helper(const struct faked_finfo *fi, const char *name, intmax_t value)
{
    char buf[64];
    snprintf(buf, sizeof buf, "%"PRIdMAX, value);

    switch (fi->type) {
    case FAKED_FINFO_FD:
        return next_fsetxattr(fi->info.fd, name, buf, strlen(buf), 0, 0);
    case FAKED_FINFO_FILE:
        return next_setxattr(fi->info.path, name, buf, strlen(buf), 0, 0);
    case FAKED_FINFO_LINK:
        return next_setxattr(fi->info.path, name, buf, strlen(buf), 0, XATTR_NOFOLLOW);
    case FAKED_FINFO_FTSENT:
    default:
        fprintf(stderr, "unexpected finfo type (%d)\n", fi->type);
        errno = ENOSYS;
        return -1;
    }
}

int faked_set(int f, struct faked_stat st, struct faked_finfo fi)
{
    intmax_t  uid;
    intmax_t  gid;
    intmax_t  mode;
    intmax_t  rdev;

    switch (st.type) {
    case FAKED_STAT_STAT:
        uid = st.stat.stat->st_uid;
        gid = st.stat.stat->st_gid;
        mode = st.stat.stat->st_mode;
        rdev = st.stat.stat->st_rdev;
        break;
    case FAKED_STAT_STAT64:
        uid = st.stat.stat64->st_uid;
        gid = st.stat.stat64->st_gid;
        mode = st.stat.stat64->st_mode;
        rdev = st.stat.stat64->st_rdev;
        break;
    }

    switch (f) {
        case chown_func:
            if (uid != (uid_t)-1)
                if (setxattr_helper(&fi, FAKEROOTUID_XATTR, uid) == -1)
                    return -1;
            if (gid != (uid_t)-1)
                if (setxattr_helper(&fi, FAKEROOTGID_XATTR, gid) == -1)
                    return -1;
            break;
        case chmod_func:
            if (setxattr_helper(&fi, FAKEROOTMODE_XATTR, mode) == -1)
                return -1;
            break;
        case mknod_func:
            if (setxattr_helper(&fi, FAKEROOTMODE_XATTR, mode) == -1
                || setxattr_helper(&fi, FAKEROOTRDEV_XATTR, rdev) == -1
            )
                return -1;
            break;
        default:
            /* don't care */
            break;
    }
    return 0;
}

int faked_get(struct faked_stat st, struct faked_finfo fi)
{
    int reset_uid_gid;
    intmax_t  uid = -1;
    intmax_t  gid = -1;
    intmax_t  mode = -1;
    intmax_t  rdev = -1;

    if (getxattr_helper(&fi, FAKEROOTUID_XATTR, &uid) == -1
        || getxattr_helper(&fi, FAKEROOTGID_XATTR, &gid) == -1
        || getxattr_helper(&fi, FAKEROOTMODE_XATTR, &mode) == -1
    )
        goto fail;
    if (mode != -1 && (mode & (S_IFCHR|S_IFBLK)))
        if (getxattr_helper(&fi, FAKEROOTRDEV_XATTR, &rdev) == -1)
            goto fail;

    switch (st.type) {
    case FAKED_STAT_STAT:
        reset_uid_gid = (st.stat.stat->st_uid != 0);
        break;
    case FAKED_STAT_STAT64:
        reset_uid_gid = (st.stat.stat64->st_uid != 0);
        break;
    }

    /*
     * If FAKEROOTUID(GID) was not set pretend the file is owned by
     * root:wheel.  However if the file is REALLY owned by root,
     * preserve original UID(GID) of the file.  This kludge exists to
     * prevent warnings from PackageMaker.  PackageMaker compares files
     * in a package against the corresponding files on the system volume
     * and warns if files mode/owner/group doesn't match.
     */
    if (uid == -1 && reset_uid_gid)
        uid = 0;
    if (gid == -1 && reset_uid_gid)
        gid = 0;

    switch (st.type) {
    case FAKED_STAT_STAT:
        if (uid != -1)
            st.stat.stat->st_uid = uid;
        if (gid != -1)
            st.stat.stat->st_gid = gid;
        if (mode != -1)
            st.stat.stat->st_mode = mode;
        if (rdev != -1)
            st.stat.stat->st_rdev = rdev;
        break;
    case FAKED_STAT_STAT64:
        if (uid != -1)
            st.stat.stat64->st_uid = uid;
        if (gid != -1)
            st.stat.stat64->st_gid = gid;
        if (mode != -1)
            st.stat.stat64->st_mode = mode;
        if (rdev != -1)
            st.stat.stat64->st_rdev = rdev;
        break;
    }
    if (fi.type == FAKED_FINFO_FTSENT && mode != -1 && (mode & (S_IFCHR|S_IFBLK))) {
        fi.info.ftsent->fts_info = FTS_DEFAULT;
    }
    return 0;
fail:
    if (fi.type == FAKED_FINFO_FTSENT) {
        fi.info.ftsent->fts_info = FTS_ERR;
        fi.info.ftsent->fts_errno = errno;
    }
    return -1;
}

struct faked_stat faked_stat(struct stat *st)
{
    struct faked_stat fs = {0};
    fs.type = FAKED_STAT_STAT;
    fs.stat.stat = st;
    return fs;
}

struct faked_stat faked_stat64(struct stat64 *st)
{
    struct faked_stat fs = {0};
    fs.type = FAKED_STAT_STAT64;
    fs.stat.stat64 = st;
    return fs;
}

struct faked_finfo faked_fd(int fd)
{
    struct faked_finfo fi = {0};
    fi.type = FAKED_FINFO_FD;
    fi.info.fd = fd;
    return fi;
}

struct faked_finfo faked_file(const char *path)
{
    struct faked_finfo fi = {0};
    fi.type = FAKED_FINFO_FILE;
    fi.info.path = path;
    return fi;
}

struct faked_finfo faked_link(const char *path)
{
    struct faked_finfo fi = {0};
    fi.type = FAKED_FINFO_LINK;
    fi.info.path = path;
    return fi;
}

struct faked_finfo faked_at(int dir_fd, const char *path, int options)
{
    struct faked_finfo fi = {0};
    fi.type = FAKED_FINFO_AT;
    fi.info.at.dir_fd = dir_fd;
    fi.info.at.path = path;
    fi.info.at.options = options;
    return fi;
}

struct faked_finfo faked_ftsent(FTSENT *ftsent)
{
    struct faked_finfo fi = {0};
    fi.type = FAKED_FINFO_FTSENT;
    fi.info.ftsent = ftsent;
    return fi;
}

const char *env_var_set(const char *env)
{
    const char *s;

    s=getenv(env);

    if(s && *s)
        return s;
    else
        return NULL;
}
