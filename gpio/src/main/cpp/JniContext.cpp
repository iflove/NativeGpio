/*
* JniContext.cpp
*
* Created by Tilong on 2019/12/21.
*/

#include "JniContext.h"
#include "Log.h"


#define TAG "JniContext"

JniContext *JniContext::instance = NULL;

JniContext *JniContext::createInstance(JavaVM *vm) {
    LOGI(TAG, "%s", __func__);
    instance = new JniContext(vm);
    return instance;
}

JniContext *JniContext::getInstance() {
    return instance;
}

void JniContext::destroyInstance() {
    delete instance;
    instance = NULL;
}

JniContext::JniContext(JavaVM *vm) {
    this->vm = vm;
}

JniContext::~JniContext() {
}

static void detach_current_thread(void *env) {
    JniContext::getInstance()->getJavaVM()->DetachCurrentThread();
}

bool JniContext::getEnv(JNIEnv **env) {
    bool bRet = false;

    switch (vm->GetEnv((void **) env, JNI_VERSION_1_4)) {
        case JNI_OK:
            bRet = true;
            break;
        case JNI_EDETACHED:
            pthread_key_create(&s_threadKey, detach_current_thread);
            if (vm->AttachCurrentThread(env, 0) < 0) {
                LOGE(TAG, "Failed to get the environment using AttachCurrentThread()");
                break;
            }
            if (pthread_getspecific(s_threadKey) == NULL)
                pthread_setspecific(s_threadKey, env);
            bRet = true;
            break;
        default:
            LOGE(TAG, "Failed to get the environment using GetEnv()");
            break;
    }

    return bRet;
}

int JniContext::registerNativeMethods(const char *className,
                                      const JNINativeMethod *gMethods, int numMethods) {
    LOGI(TAG, "%s Registering %s natives", __func__, className);
    int result = -1;
    JNIEnv *pEnv = 0;
    do {
        if (!getEnv(&pEnv)) {
            break;
        }
        jclass classID = findClassId(className, pEnv);
        if (pEnv->RegisterNatives(classID, gMethods, numMethods) < 0) {
            LOGE(TAG, "RegisterNatives failed for '%s'\n", className);
            return result;
        }
        result = 0;
        pEnv->DeleteLocalRef(classID);
    } while (0);
    return result;
}

jclass JniContext::findClassId(const char *className, JNIEnv *env) {
    JNIEnv *pEnv = env;
    jclass ret = 0;

    do {
        if (!pEnv) {
            if (!getEnv(&pEnv)) {
                break;
            }
        }

        ret = pEnv->FindClass(className);
        if (!ret) {
            LOGE(TAG, "Failed to find class of %s", className);
            break;
        }
    } while (0);

    return ret;
}

bool JniContext::findMethodInfo(JniMethodInfo &methodinfo, const char *className,
                                const char *methodName, const char *paramCode) {
    jmethodID methodID = 0;
    JNIEnv *pEnv = 0;
    bool bRet = false;

    do {
        if (!getEnv(&pEnv)) {
            break;
        }

        jclass classID = findClassId(className, pEnv);

        methodID = pEnv->GetMethodID(classID, methodName, paramCode);
        if (!methodID) {
            LOGE(TAG, "Failed to find method id of %s", methodName);
            break;
        }

        methodinfo.classId = classID;
        methodinfo.env = pEnv;
        methodinfo.methodId = methodID;

        bRet = true;
    } while (0);

    return bRet;
}

bool JniContext::findStaticMethodInfo(JniMethodInfo &methodinfo,
                                      const char *className,
                                      const char *methodName,
                                      const char *paramCode) {
    jmethodID methodID = 0;
    JNIEnv *pEnv = 0;
    bool bRet = false;

    do {
        if (!getEnv(&pEnv)) {
            break;
        }

        jclass classID = findClassId(className, pEnv);

        methodID = pEnv->GetStaticMethodID(classID, methodName, paramCode);
        if (!methodID) {
            LOGE(TAG, "Failed to find static method id of %s", methodName);
            break;
        }

        methodinfo.classId = classID;
        methodinfo.env = pEnv;
        methodinfo.methodId = methodID;

        bRet = true;
    } while (0);

    return bRet;
}

void JniContext::callJavaStaticVoidVoidMethod(const char *className, const char *methodName) {
    JniMethodInfo jniMethodInfo;
    if (findStaticMethodInfo(jniMethodInfo, className, methodName, "()V")) {
        jniMethodInfo.env->CallStaticVoidMethod(jniMethodInfo.classId, jniMethodInfo.methodId);
        jniMethodInfo.env->DeleteLocalRef(jniMethodInfo.classId);
    }
}

int JniContext::checkExc() {
    JNIEnv *env = 0;
    do {
        if (!getEnv(&env)) {
            break;
        }
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe(); // writes to logcat
            env->ExceptionClear();
            return 1;
        }
    } while (0);
    return 0;
}

void JniContext::throwNew(const char *name, const char *msg) {
    JNIEnv *env;
    do {
        if (!getEnv(&env)) {
            break;
        }
        // 查找异常类
        jclass cls = env->FindClass(name);
        /* 如果这个异常类没有找到，VM会抛出一个NowClassDefFoundError异常 */
        if (cls != NULL) {
            env->ThrowNew(cls, msg);  // 抛出指定名字的异常
        }
        /* 释放局部引用 */
        env->DeleteLocalRef(cls);
    } while (0);
}

