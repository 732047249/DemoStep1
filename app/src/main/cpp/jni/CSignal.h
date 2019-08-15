#pragma once

// for lock & synchronization
#include <pthread.h>

class CSignal {
public:
    CSignal(const char* name);
    ~CSignal(void);

public:
    bool wait();
    bool wait(long waitTimeoutMS);
    void notify();

private:
    char mName[256];
    bool mInternalState;

    pthread_cond_t mCondition;
    pthread_mutex_t mMutex;
};

