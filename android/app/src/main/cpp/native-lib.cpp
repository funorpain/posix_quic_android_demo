#include <jni.h>
#include <string>
#include <sstream>
#include <pthread.h>
#include <android/log.h>
#include <unistd.h>
#include "./quic_client.h"

static int pfd[2];
static pthread_t loggingThread;
static const char *LOG_TAG = "native-lib.cpp";

static void *loggingFunction(void *) {
    ssize_t len;
    char buf[2048];

    while ((len = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if (buf[len - 1] == '\n') {
            --len;
        }

        buf[len] = 0;  // add null-terminator

        __android_log_write(ANDROID_LOG_DEBUG, LOG_TAG, buf); // Set any log level you want
    }

    return 0;
}

static int runLoggingThread() { // run this function to redirect your output to android log
    setvbuf(stdout, 0, _IOLBF, 0); // make stdout line-buffered
    setvbuf(stderr, 0, _IONBF, 0); // make stderr unbuffered

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if (pthread_create(&loggingThread, 0, loggingFunction, 0) == -1) {
        return -1;
    }

    pthread_detach(loggingThread);

    return 0;
}

void *threadproc(void *args) {
    runLoggingThread();

    int ret = quic_client("172.27.202.3", 9700);
    __android_log_print(ANDROID_LOG_DEBUG, "quic", "threadproc returns %d", ret);
    return 0;
}

extern "C" JNIEXPORT jstring

JNICALL
Java_com_example_quicdemo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, threadproc, NULL);
    pthread_detach(tid);

    std::ostringstream oss;
    oss << "pthread_create returns " << ret;
    std::string hello = oss.str();
    return env->NewStringUTF(hello.c_str());
}
