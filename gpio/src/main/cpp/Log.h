/*
* Log.h
*
* Created by Tilong on 2019/12/21.
*/

#ifndef B_LOG_H
#define B_LOG_H

#include <android/log.h>

#ifdef DEBUG
#define LOGD(TAG, fmt, args...) __android_log_print(ANDROID_LOG_DEBUG,  TAG, fmt, ##args)
#else // NDEBUG
#define LOGD(TAG, fmt, args...)
#endif

#define LOGI(TAG, fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGW(TAG, fmt, args...) __android_log_print(ANDROID_LOG_WARN,  TAG, fmt, ##args)
#define LOGE(TAG, fmt, args...) __android_log_print(ANDROID_LOG_ERROR,  TAG, fmt, ##args)
#endif //RROID_LOG_H
