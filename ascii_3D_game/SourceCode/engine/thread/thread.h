#pragma once

#define USE_THREAD 0

#if USE_THREAD

#include "common.h"

int threadInitialize(int threadNum, size_t stackSize);

void threadDestroy();

void threadSetJob(int id, void (*job)(void*), void* arg);

//スレッドを起こす/
void prepareThreads();
//スレッドを休ませる/
void sleepThreads();
//仕事が全員ない状態になるまで待つ/
void wateForThreads();

#endif