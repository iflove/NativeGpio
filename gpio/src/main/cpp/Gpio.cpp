/*
* Gpio.cpp
*
* Created by Tilong on 2019/12/21.
*/
#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "Log.h"
#include "JniContext.h"
#include "Gpio.h"


/****************************************************************
* Constants
****************************************************************/
#define TAG "JniGpio"

#define JNI_GPIO_CLASSNAME "com/beto/example/gpio/JniGpio"
#define J_RUNTIME_CLASSNAME "java/lang/Runtime"
#define J_RUNTIME_CLASS_GET_RUNTIME "getRuntime"
#define J_RUNTIME_CLASS_EXEC "exec"
#define J_RUNTIME_CLASS_EXEC_SIGN "(Ljava/lang/String;)Ljava/lang/Process;"

#define J_RUNTIME_CLASS_GET_RUNTIME_SIGN "()Ljava/lang/Runtime;"
#define J_PROCESS_CLASSNAME "java/lang/Process"
#define J_PROCESS_CLASS_GET_OUTPUT_STREAM "getOutputStream"
#define J_PROCESS_CLASS_GET_OUTPUT_STREAM_SIGN "()Ljava/io/OutputStream;"

#define J_OUTPUT_STREAM_CLASSNAME "java/io/OutputStream"
#define J_OUTPUT_STREAM_CLASS_WRITE "write"
#define J_OUTPUT_STREAM_CLASS_WRITE_SIGN "([B)V"
#define J_OUTPUT_STREAM_CLASS_FLUSH "flush"
#define J_OUTPUT_STREAM_CLASS_CLOSE "close"

#define J_NO_VOID_SIGN "()V"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

inline static void *mallocz(size_t size) {
    void *mem = malloc(size);
    if (!mem)
        return mem;

    memset(mem, 0, size);
    return mem;
}

inline static void freep(void **mem) {
    if (mem && *mem) {
        free(*mem);
        *mem = NULL;
    }
}

// 文件是否存在 0 存在 否则不存在
int file_exists(const char *filename) {
    return (access(filename, F_OK) == 0);
}

// 文件是否有读写权限
int file_rw(const char *filename) {
    return (access(filename, R_OK | W_OK) == 0);
}

// 检查文件是否有读写权限, 否则调用 su 赋予
bool ck_rw_permissions(const char *filename) {
    if (!file_rw(filename)) {
        JniMethodInfo grant_rw_permissions_info;
        if (!JniContext::getInstance()->findStaticMethodInfo(grant_rw_permissions_info, JNI_GPIO_CLASSNAME, "grantRWPermissions", "(Ljava/lang/String;)Z")) {
            LOGE(TAG, "Call JniGpio#grantRWPermissions() err");
            return false;
        }
        JNIEnv *pEnv = grant_rw_permissions_info.env;
        jstring path_name = pEnv->NewStringUTF(filename);
        jboolean ok = pEnv->CallStaticBooleanMethod(grant_rw_permissions_info.classId, grant_rw_permissions_info.methodId, path_name);
        pEnv->DeleteLocalRef(grant_rw_permissions_info.classId);
        pEnv->DeleteLocalRef(path_name);
        return ok;
    }
    return true;
}

//通知系统需要导出控制的GPIO引脚编号
static jboolean gpio_export(JNIEnv *env, jclass thiz, jint pin_num) {
    LOGD(TAG, "__%s__ pin_num=%d", __func__, pin_num);

    int fd, len, ret;
    char buf[MAX_BUF];
    const char *filename = SYSFS_GPIO_DIR "/export";

    if (!ck_rw_permissions(filename)) {
        LOGE(TAG, "%s no read/write permissions", filename);
        return JNI_FALSE;
    }

    fd = open(filename, O_WRONLY);
    if (fd < 0) {
        LOGE(TAG, "open %s fail", filename);
        return JNI_FALSE;
    }
    len = snprintf(buf, sizeof(buf), "%d", pin_num);
    write(fd, buf, len);
    close(fd);
    return JNI_TRUE;
}

//todo 描述函数 2020年 1月 6日 星期一 14时53分31
static jboolean gpio_unexport(JNIEnv *env, jclass thiz, jint pin_num) {
    LOGD(TAG, "__%s__ pin_num=%d", __func__, pin_num);

    int fd, len;
    char buf[MAX_BUF];
    const char *filename = SYSFS_GPIO_DIR "/unexport";

    if (!ck_rw_permissions(filename)) {
        LOGE(TAG, "%s no read/write permissions", filename);
        return JNI_FALSE;
    }

    fd = open(filename, O_WRONLY);
    if (fd < 0) {
        LOGE(TAG, "open %s fail", filename);
        return JNI_FALSE;
    }
    len = snprintf(buf, sizeof(buf), "%d", pin_num);
    write(fd, buf, len);
    close(fd);
    return JNI_TRUE;
}

static jboolean setDirection(JNIEnv *env, jclass thiz, jint pin_num, jint direction) {
    LOGD(TAG, "__%s__ pin_num=%d direction=%d", __func__, pin_num, direction);

    int fd, len;
    char buf[MAX_BUF];
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", pin_num);

    if (!file_exists(buf)) {
        LOGE(TAG, "find %s fail", buf);
        return JNI_FALSE;
    }
    if (!ck_rw_permissions(buf)) {
        LOGE(TAG, "%s no read/write permissions", buf);
        return JNI_FALSE;
    }

    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        LOGE(TAG, "open %s fail", buf);
        return JNI_FALSE;
    }

    if (direction)
        write(fd, "out", 4);
    else
        write(fd, "in", 3);

    close(fd);
    return JNI_TRUE;
}

// todo
static jint getDirection(JNIEnv *env, jclass clazz, jint pin_num) {
    LOGD(TAG, "__%s__ pin_num=%d", __func__, pin_num);

    int fd, len, ret = -1;
    char buf[MAX_BUF];
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", pin_num);

    if (!file_exists(buf)) {
        LOGE(TAG, "find %s fail", buf);
        return ret;
    }

    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        LOGE(TAG, "open %s fail", buf);
        return ret;
    }

    lseek(fd, 0, SEEK_SET);
    char buffer[32] = {0};
    size_t br = read(fd, buffer, 32);
    close(fd);

    char in_str[] = "in\n";
    char out_str[] = "out\n";

    if (strcmp(in_str, buffer) == 0) {
        ret = 0;
    } else if (strcmp(out_str, buffer) == 0) {
        ret = 1;
    } else {
        //todo impl high、low?
    }
    return ret;
}

static jboolean setValue(JNIEnv *env, jclass clazz, jint pin_num, jboolean value) {
    LOGD(TAG, "__%s__ pin_num=%d value=%d", __func__, pin_num, value);

    int fd, len;
    char buf[MAX_BUF];
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", pin_num);

    if (!file_exists(buf)) {
        LOGE(TAG, "find %s fail", buf);
        return JNI_FALSE;
    }
    if (!ck_rw_permissions(buf)) {
        LOGE(TAG, "%s no read/write permissions", buf);
        return JNI_FALSE;
    }

    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        LOGE(TAG, "open %s fail", buf);
        return JNI_FALSE;
    }

    if (value)
        write(fd, "1", 2);
    else
        write(fd, "0", 2);

    close(fd);
    return JNI_TRUE;
}

static jboolean getValue(JNIEnv *env, jclass clazz, jint pin_num) {
    LOGD(TAG, "__%s__ pin_num=%d", __func__, pin_num);

    int fd, len;
    char buf[MAX_BUF];
    char ch;
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", pin_num);

    if (!file_exists(buf)) {
        LOGE(TAG, "find %s fail", buf);
        return JNI_FALSE;
    }

    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        LOGE(TAG, "open %s fail", buf);
        return JNI_FALSE;
    }

    read(fd, &ch, 1);
    close(fd);

    return ch == '1';
}

static JNINativeMethod gJni_Methods_table[] = {
        {"export",       "(I)Z",  (void *) gpio_export},
        {"unexport",     "(I)Z",  (void *) gpio_unexport},
        {"setDirection", "(II)Z", (void *) setDirection},
        {"getDirection", "(I)I",  (void *) getDirection},
        {"getValue",     "(I)Z",  (void *) getValue},
        {"setValue",     "(IZ)Z", (void *) setValue},
};

//当动态库被加载时这个函数被系统调用
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGI(TAG, "Jni loaded!");
    JniContext *jni_context = JniContext::createInstance(vm);
    jni_context->registerNativeMethods(JNI_GPIO_CLASSNAME, gJni_Methods_table,
                                       NELEM(gJni_Methods_table));
    return JNI_VERSION_1_4;
}

//当动态库被卸载时这个函数被系统调用
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI(TAG, "Jni unloaded!");
    JniContext::destroyInstance();
}

//todo Finish jni try ex crash
static bool su_chmod(const char *path) {
    JNIEnv *pEnv;
    bool ret = JNI_FALSE;
    JniMethodInfo get_runtime_info;
    jobject runtime = NULL;
    JniMethodInfo exec_info;
    jobject process = NULL;
    JniMethodInfo out_stream_info;
    jobject out_stream = NULL;
    JniMethodInfo write_info;
    JniMethodInfo flush_info;
    JniMethodInfo close_info;

    do {
        //1.获取 java.lang.Runtime getRuntime() MethodID
        if (!JniContext::getInstance()->findStaticMethodInfo(get_runtime_info, J_RUNTIME_CLASSNAME, J_RUNTIME_CLASS_GET_RUNTIME, J_RUNTIME_CLASS_GET_RUNTIME_SIGN)) {
            LOGE(TAG, "Find Runtime/getRuntime err");
            return ret;
        }
        pEnv = get_runtime_info.env;

        //2. 调用静态方法获取runtime对象
        runtime = pEnv->CallStaticObjectMethod(get_runtime_info.classId, get_runtime_info.methodId);
        if (runtime == NULL || JniContext::getInstance()->checkExc()) {
            LOGE(TAG, "Get runtime err");
            break;
        }

        //3. 获取 runtime exec(String) MethodID
        if (!JniContext::getInstance()->findMethodInfo(exec_info, J_RUNTIME_CLASSNAME, J_RUNTIME_CLASS_EXEC, J_RUNTIME_CLASS_EXEC_SIGN)) {
            LOGE(TAG, "Find runtime/exec err");
            break;
        }

        //4. 调用对象方法获取 Process 对象
        jstring command = pEnv->NewStringUTF("su");
        process = pEnv->CallObjectMethod(runtime, exec_info.methodId, command);
        pEnv->DeleteLocalRef(command);
        pEnv->DeleteLocalRef(exec_info.classId);

        if (process == NULL || JniContext::getInstance()->checkExc()) {
            LOGE(TAG, "exec su err.");
            break;
        }

        //5. 获取 Process getOutputStream() MethodID
        if (!JniContext::getInstance()->findMethodInfo(out_stream_info, J_PROCESS_CLASSNAME, J_PROCESS_CLASS_GET_OUTPUT_STREAM, J_PROCESS_CLASS_GET_OUTPUT_STREAM_SIGN)) {
            LOGE(TAG, "Find Process/getOutputStream err");
            break;
        }
        //6. 获取输出流对象
        out_stream = pEnv->CallObjectMethod(process, out_stream_info.methodId);
        pEnv->DeleteLocalRef(out_stream_info.classId);
        if (out_stream == NULL || JniContext::getInstance()->checkExc()) {
            LOGE(TAG, "Get out_stream err");
            break;
        }

        if (!JniContext::getInstance()->findMethodInfo(write_info, J_OUTPUT_STREAM_CLASSNAME, J_OUTPUT_STREAM_CLASS_WRITE, J_OUTPUT_STREAM_CLASS_WRITE_SIGN)) {
            LOGE(TAG, "Find OutputStream/write err");
            break;
        }
        const char *bytes = "reboot\n";
        jstring content = pEnv->NewStringUTF(bytes);
        jsize length = pEnv->GetStringUTFLength(content);
        jbyteArray dates = pEnv->NewByteArray(length);
        pEnv->SetByteArrayRegion(dates, 0, length, (jbyte *) bytes);
        //write
        pEnv->CallVoidMethod(out_stream, write_info.methodId, dates);

        pEnv->DeleteLocalRef(write_info.classId);
        pEnv->DeleteLocalRef(content);
        pEnv->DeleteLocalRef(dates);

        if (!JniContext::getInstance()->findMethodInfo(flush_info, J_OUTPUT_STREAM_CLASSNAME, J_OUTPUT_STREAM_CLASS_FLUSH, J_NO_VOID_SIGN)) {
            LOGE(TAG, "Find OutputStream/flush err");
            break;
        }
        //flush
        pEnv->CallVoidMethod(out_stream, flush_info.methodId);
        pEnv->DeleteLocalRef(flush_info.classId);

        if (!JniContext::getInstance()->findMethodInfo(close_info, J_OUTPUT_STREAM_CLASSNAME, J_OUTPUT_STREAM_CLASS_CLOSE, J_NO_VOID_SIGN)) {
            LOGE(TAG, "Find OutputStream/close err");
            break;
        }
        //close
        pEnv->CallVoidMethod(out_stream, close_info.methodId);
        pEnv->DeleteLocalRef(close_info.classId);

        ret = JNI_TRUE;
    } while (0);

    pEnv->DeleteLocalRef(get_runtime_info.classId);
    if (runtime != NULL) {
        pEnv->DeleteLocalRef(runtime);
    }
    pEnv->DeleteLocalRef(exec_info.classId);
    if (process != NULL) {
        pEnv->DeleteLocalRef(process);
    }
    if (out_stream != NULL) {
        pEnv->DeleteLocalRef(out_stream);
    }
    return ret;
}