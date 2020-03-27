/* --------------------------------------------------
 * @file: joylink_extern_tool.C
 *
 * @brief: 
 *
 * @version: 2.0
 *
 * @date: 2018/07/26 PM
 *
 * --------------------------------------------------
 */

#include <stdio.h>
#include <unistd.h>

// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

#include <fal.h>
#include <rtthread.h>

#include "joylink_log.h"
#include "joylink_extern_user.h"

#define JOYLINK_DOWNLOAD_PART_NAME "download"

/* dowmload partition object */
static const struct fal_partition *dl_part = RT_NULL;

/* try to find the dowload partition and erase the partition */
int
joylink_memory_init(void *index, int flag)
{
    dl_part = fal_partition_find(JOYLINK_DOWNLOAD_PART_NAME);
    if (dl_part == RT_NULL)
    {
        log_error("Find partition(%s) error!\n", JOYLINK_DOWNLOAD_PART_NAME);
        return -1;
    }

    /* start write data */
    if (flag == MEMORY_WRITE)
    {
        /* erase dowmload section */
        if (fal_partition_erase_all(dl_part) < 0)
        {
            log_error("Partition (%s) erase error!\n", dl_part->name);
            return -1;
        }
    }

    return 0;
}

int
joylink_memory_write(int offset, char *data, int len)
{
    if (dl_part == RT_NULL || data == RT_NULL || len <= 0)
    {
        return -1;
    }
    
    return fal_partition_write(dl_part, offset, (uint8_t *)data, len);
}

int
joylink_memory_read(int offset, char *data, int len)
{
    if (dl_part == RT_NULL || data == RT_NULL || len <= 0)    
    {
        return -1;
    }

    return fal_partition_read(dl_part, offset, (uint8_t *)data, len);
}

int
joylink_memory_finish(void)
{
    return 0;
}



