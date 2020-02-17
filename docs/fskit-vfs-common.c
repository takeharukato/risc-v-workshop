//git@github.com:takeharukato/fs-kit.git
#define     OMODE_MASK      (O_RDONLY | O_WRONLY | O_RDWR)
#define     SLEEP_TIME      (10000.0)
#define     MAX_SYM_LINKS   16

#define     FREE_LIST       0
#define     USED_LIST       1
#define     LOCKED_LIST     2
#define     LIST_NUM        3

#define     FD_FILE         1
#define     FD_DIR          2
#define     FD_WD           4

#define     FD_ALL          (FD_FILE | FD_DIR | FD_WD)

#define     DEFAULT_FD_NUM  (128)
#define     VNNUM           256

typedef unsigned long       fsystem_id;

typedef struct vnode vnode;
typedef struct vnlist vnlist;
typedef struct vnlink vnlink;
typedef struct fsystem fsystem;
typedef struct nspace nspace;
typedef struct ofile ofile;
typedef struct ioctx ioctx;
typedef struct fdarray fdarray;

struct vnlist {
    vnode           *head;
    vnode           *tail;
    int             num;
};

struct vnlink {
    vnode           *prev;
    vnode           *next;
};

struct vnode {
    vnode_id        vnid;
    nspace *        ns;
    nspace *        mounted;
    char            remove;
    char            busy;
    char            inlist;
    char            watched;
    vnlink          nspace;
    vnlink          list;
    int             rcnt;
    void *          data;
};

struct fsystem {
    fsystem_id          fsid;
    bool                fixed;
    image_id            aid;
    char                name[IDENT_NAME_LENGTH];
    int                 rcnt;
    vnode_ops           ops;
};

struct nspace {
    nspace_id           nsid;
    my_dev_t            dev;
    my_ino_t            ino;
    fsystem             *fs;
    vnlist              vnodes;
    void                *data;
    vnode *             root;
    vnode *             mount;
    nspace *            prev;
    nspace *            next;
    char                shutdown;
};

struct ofile {
    short           type;
    ushort          flags;
    vnode *         vn;
    void *          cookie;
    long            rcnt;
    long            ocnt;

    fs_off_t            pos;
    int             omode;
};


struct ioctx {
    lock            lock;
    int             kerrno;
    vnode *         cwd;
    fdarray *       fds;
};

struct fdarray {
    long            rcnt;
    lock            lock;
    int             num;
    ulong           *alloc;
    ulong           *coes;
    ofile           *fds[1];
};

extern struct {
    const char *    name;
    vnode_ops *     ops;
} fixed_fs[];

static vnode_id     invalid_vnid = 0;
static vnode *      rootvn;
static int          max_glb_file;
static fdarray *    global_fds;
static vnlist       lists[LIST_NUM];
static nspace *     nshead;
static lock         vnlock;
static lock         fstablock;
static nspace **    nstab;
static fsystem **   fstab;
static int          nns;
static int          nfs;
static fsystem_id   nxfsid;
static nspace_id    nxnsid;
static SkipList     skiplist;
static int          vnnum;
static int          usdvnnum;
/*
 * get_dir and get_file: basic functions to parse a path and get the vnode
 * for either the parent directory or the file itself.
 */

static int
get_dir_fd(bool kernel, int fd, const char *path, char *filename, vnode **dvn)
{
    int         err;
    char        *p, *np;

    err = new_path(path, &p);
    if (err)
        goto error1;
    np = strrchr(p, '/');
    if (!np) {
        strcpy(filename, p);
        strcpy(p, ".");
    } else {
        strcpy(filename, np+1);
        np[1] = '.';
        np[2] = '\0';
    }
    err = parse_path_fd(kernel, fd, &p, TRUE, dvn);
    if (err)
        goto error2;
    free_path(p);
    return 0;
    
error2:
    free_path(p);
error1:
    return err;
}

static int
get_file_fd(bool kernel, int fd, const char *path, int eatsymlink, vnode **vn)
{
    int         err;
    char        *p;

    err = new_path(path, &p);
    if (err)
        goto error1;
    err = parse_path_fd(kernel, fd, &p, eatsymlink, vn);
    if (err)
        goto error2;
    free_path(p);
    return 0;
    
error2:
    free_path(p);
error1:
    return err;
}

static int
get_file_vn(nspace_id nsid, vnode_id vnid, const char *path, int eatsymlink,
        vnode **vn)
{
    int         err;
    char        *p;

    err = new_path(path, &p);
    if (err)
        goto error1;
    err = parse_path_vn(nsid, vnid, &p, eatsymlink, vn);
    if (err)
        goto error2;
    free_path(p);
    return 0;
    
error2:
    free_path(p);
error1:
    return err;
}

static int
parse_path_fd(bool kernel, int fd, char **pstart, int eatsymlink, vnode **vnp)
{
    vnode           *bvn;
    ofile           *f;
    ioctx           *io;
    char            *path;

    path = *pstart;
    if (path && (*path == '/')) {
        do
            path++;
        while (*path == '/');
        bvn = rootvn;
        inc_vnode(bvn);
    } else
        if (fd >= 0) {
            f = get_fd(kernel, fd, FD_ALL);
            if (!f)
                return EBADF;
            bvn = f->vn;
            inc_vnode(bvn);
            put_fd(f);
        } else {
            io = get_cur_ioctx();
            LOCK(io->lock);
            bvn = io->cwd;
            inc_vnode(bvn);
            UNLOCK(io->lock);
        }
    return parse_path(bvn, pstart, path, eatsymlink, vnp);
}

static int
parse_path_vn(nspace_id nsid, vnode_id vnid, char **pstart, int eatsymlink,
    vnode **vnp)
{
    int             err;
    vnode           *bvn;
    char            *path;

    path = *pstart;
    if (path && (*path == '/')) {
        do
            path++;
        while (*path == '/');
        bvn = rootvn;
        inc_vnode(bvn);
    } else {
        err = load_vnode(nsid, vnid, FALSE, &bvn);
        if (err)
            return err;
    }
    return parse_path(bvn, pstart, path, eatsymlink, vnp);

error1:
    dec_vnode(bvn, FALSE);
    return err;
}

static int
parse_path(vnode *bvn, char **pstart, char *path, int eatsymlink, vnode **vnp)
{
    int             err;
    int             iter;
    char            *p, *np, *newpath, **fred;
    vnode_id        vnid;
    vnode           *vn;

    if (!path) {
        *vnp = bvn;
        return 0;
    }

    iter = 0;
    p = path;
    vn = NULL;

    while(TRUE) {

    /*
    exit if we're done
    */

        if (*p == '\0') {
            err = 0;
            break;
        }

    /*
    isolate the next component
    */

        np = strchr(p+1, '/');
        if (np) {
            *np = '\0';
            do
                np++;
            while (*np == '/');
        } else
            np = strchr(p+1, '\0');
        
    /*
    filter '..' if at the root of a namespace
    */

        if (!strcmp(p, "..") && is_root(bvn, &vn)) {
            dec_vnode(bvn, FALSE);
            bvn = vn;
        }

    /*
    ask the file system to eat this component
    */

        newpath = NULL;
        fred = &newpath;
        if (!eatsymlink && (*np == '\0'))
            fred = NULL;

        err = (*bvn->ns->fs->ops.walk)(bvn->ns->data, bvn->data, p, fred,
                &vnid);
        p = np;
        if (!err) {
            if (newpath)
                vn = bvn;
            else {
                LOCK(vnlock);
                vn = lookup_vnode(bvn->ns->nsid, vnid);
                UNLOCK(vnlock);
                dec_vnode(bvn, FALSE);
            }
        } else {
            dec_vnode(bvn, FALSE);
            break;
        }

    /*
    deal with symbolic links
    */

        if (newpath) {

    /*
    protection against cyclic graphs (with bad symbolic links).
    */

            iter++;
            if (iter > MAX_SYM_LINKS) {
                dec_vnode(vn, FALSE);
                err = ELOOP;
                break;
            }

            p = cat_paths(newpath, np);
            if (!p) {
                dec_vnode(vn, FALSE);
                err = ENOMEM;
                break;
            }
            free_path(*pstart);
            *pstart = p;
            if (*p == '/') {
                do
                    p++;
                while (*p == '/');
                dec_vnode(vn, FALSE);
                bvn = rootvn;
                inc_vnode(bvn);
            } else
                bvn = vn;
            continue;
        }

    /*
    reached a mounting point
    */

        if (is_mount_vnode(vn, &bvn)) {
            dec_vnode(vn, FALSE);
            continue;
        }

        bvn = vn;
    }

    if (!err)
        *vnp = bvn;

    return err;
}



/*
 * get_vnode
 */

int
get_vnode(nspace_id nsid, vnode_id vnid, void **data)
{
    int         err;
    vnode       *vn;

    err = load_vnode(nsid, vnid, TRUE, &vn);
    if (err)
        return err;
    *data = vn->data;
    return 0;
}


/*
 * put_vnode
 */

int
put_vnode(nspace_id nsid, vnode_id vnid)
{
    vnode           *vn;

    LOCK(vnlock);
    vn = lookup_vnode(nsid, vnid);
    if (!vn) {
        UNLOCK(vnlock);
        return ENOENT;
    }
    UNLOCK(vnlock);
    dec_vnode(vn, TRUE);
    return 0;
}

/*
 * new_vnode
 */

int
new_vnode(nspace_id nsid, vnode_id vnid, void *data)
{
    int         err;
    vnode       *vn;

    LOCK(vnlock);
    vn = steal_vnode(FREE_LIST);
    if (!vn) {
        vn = steal_vnode(USED_LIST);
        if (!vn) {
            PANIC("OUT OF VNODE!!!\n");
            UNLOCK(vnlock);
            return ENOMEM;
        }
        flush_vnode(vn, TRUE);
    }

    vn->ns = nsidtons(nsid);
    if (!vn->ns) {
        UNLOCK(vnlock);
        return ENOENT;
    }
    vn->vnid = vnid;
    vn->data = data;
    vn->rcnt = 1;
    err = sort_vnode(vn);
    UNLOCK(vnlock);
    return err;
}

/*
 * remove_vnode
 */

int
remove_vnode(nspace_id nsid, vnode_id vnid)
{
    vnode       *vn;

    LOCK(vnlock);
    vn = lookup_vnode(nsid, vnid);
    if (!vn || (vn->rcnt == 0)) {
        UNLOCK(vnlock);
        return ENOENT;
    }
    vn->remove = TRUE;
    UNLOCK(vnlock);
    return 0;
}

/*
 * unremove_vnode
 */

int
unremove_vnode(nspace_id nsid, vnode_id vnid)
{
    vnode       *vn;

    LOCK(vnlock);
    vn = lookup_vnode(nsid, vnid);
    if (!vn || (vn->rcnt == 0)) {
        UNLOCK(vnlock);
        return ENOENT;
    }
    vn->remove = FALSE;
    UNLOCK(vnlock);
    return 0;
}

/*
 * is_vnode_removed
 */

int
is_vnode_removed(nspace_id nsid, vnode_id vnid)
{
    vnode       *vn;
    int         res;

    LOCK(vnlock);
    vn = lookup_vnode(nsid, vnid);
    if (!vn) {
        UNLOCK(vnlock);
        return ENOENT;
    }
    res = vn->remove;
    UNLOCK(vnlock);
    return res;
}

/*
 * miscelleanous vnode functions
 */


static void
inc_vnode(vnode *vn)
{
    LOCK(vnlock);
    vn->rcnt++;
    UNLOCK(vnlock);
}

static void
dec_vnode(vnode *vn, char r)
{
    vnode       *ovn;

    LOCK(vnlock);
    vn->rcnt--;
    if (vn->rcnt == 0)
        if (vn->remove) {
            vn->busy = TRUE;
            move_vnode(vn, LOCKED_LIST);
            UNLOCK(vnlock);
            (*vn->ns->fs->ops.remove_vnode)(vn->ns->data, vn->data, r);
            LOCK(vnlock);
            clear_vnode(vn);
            move_vnode(vn, FREE_LIST);
        } else {
            move_vnode(vn, USED_LIST);
            if (lists[USED_LIST].num > usdvnnum) {
                ovn = steal_vnode(USED_LIST);
                flush_vnode(ovn, r);
                move_vnode(ovn, FREE_LIST);
            }
        }
    UNLOCK(vnlock);

    return;
}

static void
clear_vnode(vnode *vn)
{
    DeleteSL(skiplist, vn);

    if (vn->nspace.prev)
        vn->nspace.prev->nspace.next = vn->nspace.next;
    else
        vn->ns->vnodes.head = vn->nspace.next;
    if (vn->nspace.next)
        vn->nspace.next->nspace.prev = vn->nspace.prev;
    else
        vn->ns->vnodes.tail = vn->nspace.prev;
    vn->nspace.next = vn->nspace.prev = NULL;

    vn->vnid = invalid_vnid;
    vn->ns = NULL;
    vn->remove = FALSE;
    vn->data = NULL;
    vn->rcnt = 0;
    vn->busy = FALSE;
    vn->mounted = NULL;
}

static int
sort_vnode(vnode *vn)
{
    if (!InsertSL(skiplist, vn))
        return ENOMEM;

    vn->nspace.next = vn->ns->vnodes.head;
    vn->nspace.prev = NULL;
    if (vn->ns->vnodes.head)
        vn->ns->vnodes.head->nspace.prev = vn;
    else
        vn->ns->vnodes.tail = vn;
    vn->ns->vnodes.head = vn;
    return 0;
}

static void
move_vnode(vnode *vn, int list)
{
    if (vn->list.prev)
        vn->list.prev->list.next = vn->list.next;
    else
        lists[vn->inlist].head = vn->list.next;
    if (vn->list.next)
        vn->list.next->list.prev = vn->list.prev;
    else
        lists[vn->inlist].tail = vn->list.prev;
    lists[vn->inlist].num--;
    vn->inlist = list;
    vn->list.next = NULL;
    vn->list.prev = lists[list].tail;
    if (vn->list.prev)
        vn->list.prev->list.next = vn;
    else
        lists[list].head = vn;
    lists[list].tail = vn;
    lists[list].num++;
}

static vnode *
steal_vnode(int list)
{
    vnode       *vn;

    vn = lists[list].head;
    if (!vn)
        return NULL;
    move_vnode(vn, LOCKED_LIST);
    return vn;
}

static void
flush_vnode(vnode *vn, char r)
{
    int     err;

    vn->busy = TRUE;
    UNLOCK(vnlock);
    err = (*vn->ns->fs->ops.release_vnode)(vn->ns->data, vn->data, r);
    if (err)
        PANIC("ERROR WRITING VNODE!!!\n");
    LOCK(vnlock);
    vn->busy = FALSE;
    clear_vnode(vn);
}

static vnode *
lookup_vnode(nspace_id nsid, vnode_id vnid)
{
    vnode       fakevn;
    nspace      fakens;

    fakens.nsid = nsid;
    fakevn.ns = &fakens;
    fakevn.vnid = vnid;
    return SearchSL(skiplist, &fakevn);
}

static int
load_vnode(nspace_id nsid, vnode_id vnid, char r, vnode **vnp)
{
    int             err;
    vnode           *vn;

    LOCK(vnlock);
    while (TRUE) {
        vn = lookup_vnode(nsid, vnid);
        if (vn)
            if (vn->busy) {
                UNLOCK(vnlock);
                snooze(SLEEP_TIME);
                LOCK(vnlock);
                continue;
            } else
                break;

        vn = steal_vnode(FREE_LIST);
        if (!vn) {
            vn = steal_vnode(USED_LIST);
            if (!vn)
                PANIC("OUT OF VNODE!!!\n");
        } else
            break;

        flush_vnode(vn, r);
        move_vnode(vn, FREE_LIST);
    }

    if (vn->ns == NULL) {
        vn->ns = nsidtons(nsid);
        if (!vn->ns) {
            err = ENOENT;
            goto error1;
        }
        vn->vnid = vnid;
        vn->busy = TRUE;
        err = sort_vnode(vn);
        if (err)
            goto error2;
        move_vnode(vn, LOCKED_LIST);
        UNLOCK(vnlock);
        err = (*vn->ns->fs->ops.read_vnode)(vn->ns->data, vnid, r, &vn->data);
        LOCK(vnlock);
        vn->busy = FALSE;
        if (err)
            goto error2;
        vn->rcnt = 1;
    } else {
        vn->rcnt++;
        if (vn->rcnt == 1)
            move_vnode(vn, LOCKED_LIST);
    }
    *vnp = vn;
    UNLOCK(vnlock);
    return 0;

error2:
    clear_vnode(vn);
error1:
    move_vnode(vn, FREE_LIST);
    UNLOCK(vnlock);
    return err;
}

static int
compare_vnode(vnode *vna, vnode *vnb)
{
    if (vna->vnid > vnb->vnid)
        return 1;
    else
        if (vna->vnid < vnb->vnid)
            return -1;
        else
            if (vna->ns->nsid > vnb->ns->nsid)
                return 1;
            else
                if (vna->ns->nsid < vnb->ns->nsid)
                    return -1;
                else
                    return 0;
}

/*
 * path management functions
 */

int
new_path(const char *path, char **copy)
{
    const char  *q, *r;
    char        *p;
    int         l, s;

    if (!path) {
        *copy = NULL;
        return 0;
    }
    l = strlen(path);
    if (l == 0)
        return ENOENT;
    s = l;
    if (path[l-1] == '/')
        s++;

    if (l >= MAXPATHLEN)
        return ENAMETOOLONG;

    q = path;
    while(*q != '\0') {
        while (*q == '/')
            q++;
        r = q;
        while ((*q != '/') && (*q != '\0'))
            q++;
        if (q - r >= FILE_NAME_LENGTH)
            return ENAMETOOLONG;        
    }

    p = (char *) malloc(s+1);
    if (!p)
        return ENOMEM;

    /* ### do real checking: MAXPATHLEN, max file name len, buffer address... */

    strcpy(p, path);    
    if (p[l-1] == '/') {
        p[l] = '.';
        p[l+1] = '\0';
    }
    *copy = p;
    return 0;
}

void
free_path(char *p)
{
    if (p) {
        free(p);
    }
}

static char *
cat_paths(char *a, char *b)
{
    char        *p;

    p = (char *) realloc(a, strlen(a) + strlen(b) + 2);
    if (!p)
        return NULL;
    strcat(p, "/");
    strcat(p, b);
    return p;
}

/* 
 * mount point management functions
 */

static int
is_mount_vnode(vnode *mount, vnode **root)
{
    nspace      *ns;

    LOCK(vnlock);
    ns = mount->mounted;
    if (ns) {
        *root = ns->root;
        ns->root->rcnt++;
    }
    UNLOCK(vnlock);
    return (ns != NULL);        
}

static int
is_mount_vnid(nspace_id nsid, vnode_id vnid, vnode_id *mount)
{
    nspace      *ns;

    for(ns = nshead; ns; ns = ns->next) {
        if (!ns->mount)
            continue;
        if (ns->mount->ns->nsid != nsid)
            continue;
        if (ns->mount->vnid != vnid)
            continue;
        *mount = ns->root->vnid;
        return TRUE;
    }
    return FALSE;
}

static int
is_root(vnode *root, vnode **mount)
{
    if ((root->ns->root == root) && root->ns->mount) {
        *mount = root->ns->mount;
        inc_vnode(*mount);
        return TRUE;
    } else
        return FALSE;
}

/*
 * file descriptor management functions
 */
 
static ofile *
get_fd(bool kernel, int fd, int type)
{
    ofile       *f;
    fdarray     *fds;

    f = NULL;
    if (kernel)
        fds = global_fds;
    else
        fds = get_cur_ioctx()->fds;
    LOCK(fds->lock);
    if ((fd >= 0) && (fd < fds->num) && fds->fds[fd]) {
        f = fds->fds[fd];
        if (f->type & type)
            atomic_add(&f->rcnt, 1);
        else
            f = NULL;
    }
    UNLOCK(fds->lock);
    return f;
}


static int
put_fd(ofile *f)
{
    long            cnt;

    cnt = atomic_add(&f->rcnt, -1);
    if (cnt == 1)
        invoke_free(f);
    return 0;
}

static int
new_fd(bool kernel, int nfd, ofile *f, int fd, bool coe)
{
    int         i, j, num, end;
    fdarray     *fds;
    ofile       *of;
    int         err;
    long        cnt;

    if (kernel)
        fds = global_fds;
    else
        fds = get_cur_ioctx()->fds;

    LOCK(fds->lock);

    num = fds->num;

    if (!f) {
        if ((fd < 0) || (fd >= num)) {
            err = EBADF;
            goto error1;
        }
        f = fds->fds[fd];
        if (!f) {
            err = EBADF;
            goto error1;
        }
    }

    atomic_add(&f->rcnt, 1);
    atomic_add(&f->ocnt, 1);

    if (nfd >= 0) {
        if (nfd >= num) {
            err = EBADF;
            goto error2;
        }
        of = fds->fds[nfd];
        fds->fds[nfd] = f;
        SETBIT(fds->alloc, nfd, TRUE);
        SETBIT(fds->coes, nfd, coe);

        UNLOCK(fds->lock);
        if (of) {
            cnt = atomic_add(&of->ocnt, -1);
            if (cnt == 1)
                invoke_close(of);
            cnt = atomic_add(&of->rcnt, -1);
            if (cnt == 1)
                invoke_free(of);
        }
        return nfd;
    }

    end = num & ~31;
    for(j=0; j<end; j+=32)
        if (fds->alloc[j/32] != 0xffffffff)
            for(i=j; i<j+32; i++)
                if (!GETBIT(fds->alloc, i))
                    goto found;
    for(i=end; i<num; i++)
        if (!GETBIT(fds->alloc, i))
            goto found;

    err = EMFILE;
    goto error2;

found:

    SETBIT(fds->alloc, i, 1);
    fds->fds[i] = f;
    SETBIT(fds->coes, i, coe);
    UNLOCK(fds->lock);
    return i;

error2:
    atomic_add(&f->rcnt, -1);
    atomic_add(&f->ocnt, -1);
error1:
    UNLOCK(fds->lock);
    return err;
}

static int
remove_fd(bool kernel, int fd, int type)
{
    ofile       *f;
    fdarray     *fds;
    long        cnt;
    int         err;

    f = NULL;
    if (kernel)
        fds = global_fds;
    else
        fds = get_cur_ioctx()->fds;
    LOCK(fds->lock);
    if ((fd >= 0) && (fd < fds->num) && fds->fds[fd]) {
        f = fds->fds[fd];
        if (f->type == type) {
            SETBIT(fds->alloc, fd, 0);
            fds->fds[fd] = NULL;
        } else
            f = NULL;
    }
    UNLOCK(fds->lock);
    if (f == NULL)
        return EBADF;

    err = 0;
    cnt = atomic_add(&f->ocnt, -1);
    if (cnt == 1)
        err = invoke_close(f);
    cnt = atomic_add(&f->rcnt, -1);
    if (cnt == 1)
        invoke_free(f);
    return err;
}

static int
get_coe(bool kernel, int fd, int type, bool *coe)
{
    ofile       *f;
    fdarray     *fds;

    f = NULL;
    if (kernel)
        fds = global_fds;
    else
        fds = get_cur_ioctx()->fds;
    LOCK(fds->lock);
    if ((fd >= 0) && (fd < fds->num) && fds->fds[fd]) {
        f = fds->fds[fd];
        if (f->type == type) {
            *coe = GETBIT(fds->coes, fd);
            UNLOCK(fds->lock);
            return 0;
        }
    }
    UNLOCK(fds->lock);
    return EBADF;
}

static int
set_coe(bool kernel, int fd, int type, bool coe)
{
    ofile       *f;
    fdarray     *fds;

    f = NULL;
    if (kernel)
        fds = global_fds;
    else
        fds = get_cur_ioctx()->fds;
    LOCK(fds->lock);
    if ((fd >= 0) && (fd < fds->num) && fds->fds[fd]) {
        f = fds->fds[fd];
        if (f->type == type) {
            SETBIT(fds->coes, fd, coe);
            UNLOCK(fds->lock);
            return 0;
        }
    }
    UNLOCK(fds->lock);
    return EBADF;
}

static int
get_omode(bool kernel, int fd, int type, int *omode)
{
    ofile       *f;
    fdarray     *fds;

    f = NULL;
    if (kernel)
        fds = global_fds;
    else
        fds = get_cur_ioctx()->fds;
    LOCK(fds->lock);
    if ((fd >= 0) && (fd < fds->num) && fds->fds[fd]) {
        f = fds->fds[fd];
        if (f->type == type) {
            *omode = f->omode;
            UNLOCK(fds->lock);
            return 0;
        }
    }
    UNLOCK(fds->lock);
    return EBADF;
}

static int
invoke_close(ofile *f)
{
    int     err;
    vnode   *vn;

    vn = f->vn;
    switch (f->type) {
    case FD_FILE:
        err = (*vn->ns->fs->ops.close)(vn->ns->data, vn->data, f->cookie);
        break;
    case FD_DIR:
        err = (*vn->ns->fs->ops.closedir)(vn->ns->data, vn->data, f->cookie);
        break;
    case FD_WD:
    default:
        err = 0;
        break;
    }
    return err;
}

static int
invoke_free(ofile *f)
{
    vnode           *vn;
    op_free_cookie  *op = NULL;

    vn = f->vn;
    switch(f->type) {
    case FD_FILE:
        op = vn->ns->fs->ops.free_cookie;
        break;
    case FD_DIR:
        op = vn->ns->fs->ops.free_dircookie;
        break;
    case FD_WD:
        op = NULL;
        break;
    }
    if (op)
        (*op)(vn->ns->data, vn->data, f->cookie);
    dec_vnode(vn, FALSE);

    free(f);
    return 0;
}

/*
 * other routines
 */

static nspace *
nsidtons(nspace_id nsid)
{
    nspace      *ns;

    ns = nstab[nsid % nns];
    if (!ns || (ns->nsid != nsid) || ns->shutdown)
        return NULL;
    return ns;
}

static int
alloc_wd_fd(bool kernel, vnode *vn, bool coe, int *fdp)
{
    int             err;
    ofile           *f;
    int             nfd;

    /*
    find a file descriptor
    */

    f = (ofile *) calloc(sizeof(ofile), 1);
    if (!f) {
        err = ENOMEM;
        goto error1;
    }

    f->type = FD_WD;
    f->vn = vn;
    f->rcnt = 0;
    f->ocnt = 0;

    nfd = new_fd(kernel, -1, f, -1, coe);
    if (nfd < 0) {
        err = EMFILE;
        goto error2;
    }

    *fdp = nfd;
    return 0;

error2:
    free(f);
error1:
    return err;
}



/*
 * file system operations
 */

void *
install_file_system(vnode_ops *ops, const char *name, bool fixed, image_id aid)
{
    fsystem     *fs;
    int         i;

    fs = (fsystem *) malloc(sizeof(fsystem));
    if (!fs)
        return NULL;

    memcpy(&fs->ops, ops, sizeof(vnode_ops));
    strcpy(fs->name, name);
    fs->rcnt = 1;
    fs->fixed = fixed;
    fs->aid = aid;

    for(i=0; i<nfs; i++, nxfsid++)
        if (!fstab[nxfsid % nfs]) {
            fstab[nxfsid % nfs] = fs;
            fs->fsid = nxfsid;
            nxfsid++;
            break;
        }

    if (i == nfs) {
        free(fs);
        return NULL;
    }
    return (void *)fs;
}

static fsystem *
load_file_system(const char *name)
{
    return NULL;
}

static int
unload_file_system(fsystem *fs)
{
    fstab[fs->fsid % nfs] = NULL;
    free(fs);
    return 0;
}


static fsystem *
inc_file_system(const char *name)
{
    fsystem         *fs;
    int             i;

    fs = NULL;
    LOCK(fstablock);

    for(i=0; i<nfs; i++)
        if (fstab[i] && !strcmp(fstab[i]->name, name)) {
            fs = fstab[i];
            fs->rcnt++;
            break;
        }

    if (!fs)
        fs = load_file_system(name);

    UNLOCK(fstablock);

    return fs;
}

static int
dec_file_system(fsystem *fs)
{
    LOCK(fstablock);

    fs->rcnt--;
    if (!fs->fixed && (fs->rcnt == 0))
        unload_file_system(fs);

    UNLOCK(fstablock);
    return 0;
}


static fdarray *
new_fds(int num)
{
    fdarray *fds;
    size_t      sz;

    sz = sizeof(fdarray) + (num-1) * sizeof(void *) + 2*BITSZ(num);
    fds = (fdarray *) malloc(sz);
    if (!fds)
        return NULL;
    memset(fds, 0, sz);
    fds->rcnt = 1;
    if (new_lock(&fds->lock, "fdlock") < 0) {
        free(fds);
        return NULL;
    }
    fds->num = num;
    fds->alloc = (ulong *) &fds->fds[num];
    fds->coes = &fds->alloc[BITSZ(num) / sizeof(ulong)];
    return fds;
}

static int
free_fds(fdarray *fds)
{
    long    cnt;
    int     i;
    ofile   *f;

    for(i=0; i<fds->num; i++)
        if (fds->fds[i]) {
            f = fds->fds[i];
            cnt = atomic_add(&f->ocnt, -1);
            if (cnt == 1)
                invoke_close(f);
            cnt = atomic_add(&f->rcnt, -1);
            if (cnt == 1)
                invoke_free(f);
        }
    delete_sem(fds->lock.s);
    free(fds);
    return 0;
}
