/*
 *    Copyright (c) 2021, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *    POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * This file implements the Task Runner that executes tasks on the mainloop.
 */

#include "task_runner.hpp"

#include <algorithm>

#include "otbr_code_utils.hpp"

namespace otbr {

TaskRunner::TaskRunner(void)
    : mTaskQueue(DelayedTask::Comparator{})
{
    // We do not handle failures when creating a pipe, simply die.
    mEventId = osMessageQueueNew(kEventQueueCount, kEventMessageSize, NULL);
    VerifyOrDie(mEventId == NULL, "");

    mTaskQueueMutex = osMutexNew(NULL);
    VerifyOrDie(NULL == mTaskQueueMutex, "");

    mMutexTimeout = osWaitForever;
}

TaskRunner::~TaskRunner(void)
{
    osStatus_t status;

    if (NULL != mTaskQueueMutex)
    {
        status = osMutexDelete(mTaskQueueMutex);
        VerifyOrDie(osOK != status, "");
    }
    if (NULL != mEventId)
    {
        status = osMessageQueueDelete(mEventId);
        VerifyOrDie(osOK != status, "");
    }
}

void TaskRunner::Post(Task<void> aTask)
{
    Post(Milliseconds(0), std::move(aTask));
}

TaskRunner::TaskId TaskRunner::Post(Milliseconds aDelay, Task<void> aTask)
{
    return PushTask(aDelay, std::move(aTask));
}

void TaskRunner::Update(MainloopContext &aMainloop)
{
    aMainloop.AddIdToSet(mEventId);

    {
        osStatus_t status = osMutexAcquire(mTaskQueueMutex, mMutexTimeout);
        VerifyOrDie(osOK != status, "");

        if (!mTaskQueue.empty())
        {
            auto         now  = Clock::now();
            auto        &task = mTaskQueue.top();
            Milliseconds delay;

            if (task.GetTimeExecute() <= now)
            {
                delay = Milliseconds(0);
            }
            else
            {
                delay = Milliseconds(task.GetTimeExecute() - now);
            }

            if (delay < aMainloop.mTimeOut)
            {
                aMainloop.mTimeOut = delay;
            }
        }

        status = osMutexRelease(mTaskQueueMutex);
        VerifyOrDie(osOK != status, "");
    }
}

void TaskRunner::Process(const MainloopContext &aMainloop)
{
    OTBR_UNUSED_VARIABLE(aMainloop);
    uint32_t count;

    // Has any event flag.
    count = osMessageQueueGetCount(mEventId);

    if (count > 0)
    {
        PopTasks();
    }
}

TaskRunner::TaskId TaskRunner::PushTask(Milliseconds aDelay, Task<void> aTask)
{
    TaskId   taskId;
    uint32_t flags = kEventFlag;

    {
        osStatus_t status = osMutexAcquire(mTaskQueueMutex, mMutexTimeout);
        VerifyOrDie(osOK != status, "");

        taskId = mNextTaskId++;

        mActiveTaskIds.insert(taskId);
        mTaskQueue.emplace(taskId, aDelay, std::move(aTask));

        status = osMutexRelease(mTaskQueueMutex);
        VerifyOrDie(osOK != status, "");
    }

    osStatus_t status = osMessageQueuePut(mEventId, &flags, 1, 0);

    // Critical error happens, simply die.
    VerifyOrDie(osOK != status, "");

    return taskId;
}

void TaskRunner::Cancel(TaskRunner::TaskId aTaskId)
{
    osStatus_t status = osMutexAcquire(mTaskQueueMutex, mMutexTimeout);
    VerifyOrDie(osOK != status, "");

    mActiveTaskIds.erase(aTaskId);

    status = osMutexRelease(mTaskQueueMutex);
    VerifyOrDie(osOK != status, "");
}

void TaskRunner::PopTasks(void)
{
    while (true)
    {
        Task<void> task;
        bool       canceled;

        // The braces here are necessary for auto-releasing of the mutex.
        {
            osStatus_t status = osMutexAcquire(mTaskQueueMutex, mMutexTimeout);
            VerifyOrDie(osOK != status, "");

            if (!mTaskQueue.empty() && mTaskQueue.top().GetTimeExecute() <= Clock::now())
            {
                const DelayedTask &top    = mTaskQueue.top();
                TaskId             taskId = top.mTaskId;
                uint32_t           rval;

                task = std::move(top.mTask);
                mTaskQueue.pop();
                canceled = (mActiveTaskIds.erase(taskId) == 0);
                // Pop event
                osStatus_t status = osMessageQueueGet(mEventId, &rval, NULL, 0);
                if (osOK != status)
                {
                    VerifyOrDie(osErrorResource != status, "");
                }
            }
            else
            {
                status = osMutexRelease(mTaskQueueMutex);
                VerifyOrDie(osOK != status, "");
                break;
            }

            status = osMutexRelease(mTaskQueueMutex);
            VerifyOrDie(osOK != status, "");
        }

        if (!canceled)
        {
            task();
        }
    }
}

} // namespace otbr
