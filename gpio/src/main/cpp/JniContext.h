/*
* JniContext.h
*
* Created by Tilong on 2019/12/21.
*/

#ifndef B_JNICONTEXT_H
#define B_JNICONTEXT_H

#include <jni.h>
#include <pthread.h>

typedef struct JniMethodInfo {
    JNIEnv *env;
    jclass classId;
    jmethodID methodId;
} JniMethodInfo;

class JniContext {
public:
    static JniContext *createInstance(JavaVM *vm);

    static JniContext *getInstance();

    static void destroyInstance();

    virtual ~JniContext();

    int
    registerNativeMethods(const char *className, const JNINativeMethod *gMethods, int numMethods);

    jclass findClassId(const char *className, JNIEnv *env = 0);

    bool findMethodInfo(JniMethodInfo &methodinfo,
                        const char *className,
                        const char *methodName,
                        const char *paramCode);

    bool findStaticMethodInfo(JniMethodInfo &methodinfo,
                              const char *className,
                              const char *methodName,
                              const char *paramCode);

    void callJavaStaticVoidVoidMethod(const char *className, const char *methodName);

    int checkExc();

    void throwNew(const char *name, const char *msg);

    JavaVM *getJavaVM() {
        return vm;
    }

private:
    JniContext(JavaVM *vm);

    bool getEnv(JNIEnv **env);

    static JniContext *instance;

    JavaVM *vm;
    pthread_key_t s_threadKey;
};

#endif //B_JNICONTEXT_H
