#include <android/log.h>
#include <gst/gst.h>

#include "csio.h"

int csio_init()
{
    GST_DEBUG("%s: enter",__FUNCTION__);
    __android_log_print (ANDROID_LOG_ERROR, "csio",
                         "%s enter.",__FUNCTION__);

    __android_log_print (ANDROID_LOG_ERROR, "csio",
                         "%s exit.",__FUNCTION__);
    GST_DEBUG("%s: exit",__FUNCTION__);
}