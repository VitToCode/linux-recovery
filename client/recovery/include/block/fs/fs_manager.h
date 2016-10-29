#ifndef FS_MANAGER_H
#define FS_MANAGER_H

#define FS_GET_TYPE(type, head, member)      (type *)(head->member)
#define FS_GET_PARAM(fs)     (fs->params)
#define FS_SET_PARAMS(fs, p) (fs->params = p)
#define FS_SET_PRIVATE(fs, p) (fs->priv = p)

#define FS_GET_BM(fs)   (struct block_manager *)(fs->priv)
#define FS_GET_MTD_DEV(fs)  (struct mtd_dev_info *)(fs->params->mtd)

enum {
    FS_FLAG_UNLOCK,
    FS_FLAG_NOECC,
    FS_FLAG_AUTOPLACE,
    FS_FLAG_WRITEOOB,
    FS_FLAG_OOBSIZE,
    FS_FLAG_PAD,
    FS_FLAG_MARKBAD,
    FS_FLAG_NOSKIPBAD,
};

#define FS_FLAG_BITS(N) (1<<FS_FLAG_##N)
#define FS_FLAG_IS_SET(fs, N) ((fs->flag & FS_FLAG_BITS(N)) != 0)
#define FS_FLAG_SET(fs, N) (fs->flag |= FS_FLAG_BITS(N))
struct fs_operation_params {
    pid_t tid;
    int operation_method;
    char *buf;
    int64_t offset;
    int64_t length;
    int64_t max_mapped_size;
    int64_t content_start;
    void *mtd;
};

struct filesystem {
    char *name;
    struct list_head  list_cell;
    int (*init)(struct filesystem *fs);
    int (*alloc_params)(struct filesystem *fs);
    int (*free_params)(struct filesystem *fs);
    int64_t (*erase)(struct filesystem *fs);
    int64_t (*read)(struct filesystem *fs);
    int64_t (*write)(struct filesystem *fs);
    int64_t (*get_operate_start_address)(struct filesystem *fs);
    unsigned long (*get_leb_size)(struct filesystem *fs);
    int64_t (*get_max_mapped_size_in_partition)(struct filesystem *fs);
    int tagsize;
    unsigned int flag;
    struct fs_operation_params *params;
    void *priv;
};

#include "jffs2.h"
#include "ubifs.h"
#include "yaffs2.h"

extern int target_endian;
int fs_alloc_params(struct filesystem *this);
int fs_free_params(struct filesystem *this);
int fs_register(struct list_head *head, struct filesystem* this);
int fs_unregister(struct list_head *head, struct filesystem* this);
struct filesystem* fs_get_registered_by_name(struct list_head *head,
        char *filetype);
struct filesystem* fs_get_suppoted_by_name(char *filetype);
void fs_set_content_boundary(struct filesystem *this, int64_t max_mapped_size, 
                    int64_t content_start);
void fs_set_private_data(struct filesystem* this, void *data);

// void fs_set_parameter(struct filesystem* fs,
//                       struct fs_operation_params *p);
#endif