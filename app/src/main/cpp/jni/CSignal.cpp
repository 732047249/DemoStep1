#include <cstdio>
#include <cstring>
#include <errno.h>

#include "CSignal.h"

CSignal::CSignal(const char* name) {
    strcpy(mName, name);
    // printf("+++ %p - init as: %s\n", this, mName);

    mInternalState = false;

    pthread_cond_init(&mCondition, NULL);
    pthread_mutex_init(&mMutex, NULL);
}

CSignal::~CSignal(void) {
    // printf("+++ %p - destroy: %s\n", this, mName);

    pthread_cond_destroy(&mCondition);
    pthread_mutex_destroy(&mMutex);
}

bool CSignal::wait() {
    // printf("+++ %p - wait: %s\n", this, mName);

    pthread_mutex_lock(&mMutex);

    while (!mInternalState) {
        pthread_cond_wait(&mCondition, &mMutex);
    }

    mInternalState = false;

    pthread_mutex_unlock(&mMutex);

    return true;
}

bool CSignal::wait(long waitTimeoutMS) {
    // printf("+++ %p - wait with timeout: %s - %d\n", this, mName, waitTimeoutMS);

    pthread_mutex_lock(&mMutex);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    if (waitTimeoutMS >= 1000) {
        ts.tv_sec += waitTimeoutMS / 1000;
    }
    ts.tv_nsec += (waitTimeoutMS % 1000) * 1000000L;
    while (ts.tv_nsec > 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L; // 9 zeros for nano seconds
    }

    while (!mInternalState) {
        int reason = pthread_cond_timedwait(&mCondition, &mMutex, &ts);
        if (reason == ETIMEDOUT) {
            // printf("wait for condition timeout");
            break;
        }
    }

    mInternalState = false;

    pthread_mutex_unlock(&mMutex);

    return true;
}

void CSignal::notify() {
    // printf("+++ %p - notify: %s\n", this, mName);

    pthread_mutex_lock(&mMutex);

    mInternalState = true;
    pthread_cond_signal(&mCondition);

    pthread_mutex_unlock(&mMutex);
}

