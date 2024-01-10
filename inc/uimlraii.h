
#pragma once

//
// UIML RAII 助手库

#ifndef __cplusplus
#error "UIML RAII header file is only intended to use in C++!"
#endif

#include "FreeRTOS.h"

// 信号量自动上锁解锁保护器
class FreertosSemaphoreLocker
{
  public:
    FreertosSemaphoreLocker(const SemaphoreHandle_t handle, size_t blockTime) : semaphore(handle)
    {
        xSemaphoreTake(semaphore, blockTime);
    }
    ~FreertosSemaphoreLocker() { xSemaphoreGive(semaphore); }

  private:
    const SemaphoreHandle_t semaphore;
};
