#include "joylink_porting_layer.h"
#include "joylink_log.h"
#include "joylink_ret_code.h"
#include "joylink_dev.h"
#ifdef JOYLINK_USING_SOFTAP
#include "joylink_softap_start.h"
#endif
#ifdef JOYLINK_USING_THUNDER_SLAVE
#include "joylink_config_handle.h"
#endif

#include <rtthread.h>
#include <easyflash.h>

static rt_bool_t is_init = RT_FALSE;

void* joylink_calloc(size_t size, size_t num)
{
    return rt_calloc(size, num);
}

void* joylink_malloc(size_t sz)
{
    return rt_malloc(sz);
}

void joylink_free(void *ptr)
{
    return rt_free(ptr);
}

/* joylink cloud work start */
int joylink_start(void)
{
    extern int joylink_main_start(void);
    rt_thread_t tid;

    if (is_init == RT_TRUE)
    {
        log_info("Joylink %s IoT SDK is already start!\n", _VERSION_);
        return 0;
    }

    tid = rt_thread_create("joylink",
                           (void (*)(void *parameter))joylink_main_start, RT_NULL,
                           12 * 1024, RT_THREAD_PRIORITY_MAX / 3, 10);
    if (tid)
    {
        is_init = RT_TRUE;
        log_info("Joylink %s IoT SDK start successfully.\n", _VERSION_);

        rt_thread_startup(tid);
        return 0;
    }
    else
    {
        is_init = RT_FALSE;
        log_error("Joylink %s IoT SDK start failed.\n", _VERSION_);
        return -1;
    }
}

/* joylink cloud work stop */
int joylink_stop(void)
{
    extern int joylink_main_stop(void);
    joylink_main_stop();

    is_init = RT_FALSE;
    log_info("Joylink %s IoT SDK stop successfully.\n", _VERSION_);

    return 0;
}

int joylink_config_reset(void)
{
    /* clean wifi information */
    ef_env_set_default();
    rt_thread_mdelay(1000);

    /* reset device */
    extern void rt_hw_cpu_reset(void);
    rt_hw_cpu_reset();

    return 0;
}

extern int joylink_softap_is_start(void);
extern int joylink_config_is_start(void);

int joylink_mode_change(void)
{
    if (is_init == RT_FALSE)
    {
        log_error("Joylink %s IoT SDK is not start!\n", _VERSION_);
        return -1;
    }

#if defined(JOYLINK_USING_SOFTAP) && defined(JOYLINK_USING_THUNDER_SLAVE)
    if (joylink_softap_is_start())
    {
        extern int joylink_softap_stop(void);
        joylink_softap_stop();
        rt_thread_mdelay(2000);

        /* stop softap config, start smartconfig or thunderconfig  */
        joylink_config_start(60 * 1000);
    }
    else if (joylink_config_is_start())
    {
        /* reset device */
        extern void rt_hw_cpu_reset(void);
        rt_hw_cpu_reset();
    }

    return 0;
#else
    log_error("Not support change netwok configuration mode, please check configuration.\n");
    return -1;
#endif
}
