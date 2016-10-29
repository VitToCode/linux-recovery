#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <utils/log.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <types.h>
#include <linux/jffs2.h>
#include <lib/mtd/jffs2-user.h>
#include <lib/libcommon.h>
#include <lib/mtd/mtd-user.h>
#include <autoconf.h>
#include <lib/crc/libcrc.h>
#include <utils/list.h>
#include <block/fs/fs_manager.h>
#include <block/block_manager.h>
#include <block/mtd/mtd.h>

#define LOG_TAG  "fs_ubifs"

static int ubifs_init(struct filesystem *fs) {
    return true;
};

static int64_t ubifs_erase(struct filesystem *fs) {
    return mtd_basic_erase(fs);
}

static int64_t ubifs_read(struct filesystem *fs) {
    return mtd_basic_read(fs);
}

static int64_t ubifs_write(struct filesystem *fs) {
    return mtd_basic_write(fs);
}

static int64_t ubifs_get_operate_start_address(struct filesystem *fs) {
    return fs->params->offset;
}

static unsigned long ubifs_get_leb_size(struct filesystem *fs) {
    struct mtd_dev_info *mtd = FS_GET_MTD_DEV(fs);
    return mtd->eb_size;
}
static int64_t ubifs_get_max_mapped_size_in_partition(struct filesystem *fs) {
    return mtd_block_scan(fs);
}

struct filesystem fs_ubifs = {
    .name = BM_FILE_TYPE_UBIFS,
    .init = ubifs_init,
    .alloc_params = fs_alloc_params,
    .free_params = fs_free_params,
    .erase = ubifs_erase,
    .read = ubifs_read,
    .write = ubifs_write,
    .get_operate_start_address = ubifs_get_operate_start_address,
    .get_leb_size = ubifs_get_leb_size,
    .get_max_mapped_size_in_partition =
            ubifs_get_max_mapped_size_in_partition,
};
