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

#include "mainloop.hpp"
#include "mainloop_manager.hpp"

namespace otbr {

MainloopProcessor::MainloopProcessor(void)
{
    MainloopManager::GetInstance().AddMainloopProcessor(this);
}

MainloopProcessor::~MainloopProcessor(void)
{
    MainloopManager::GetInstance().RemoveMainloopProcessor(this);
}

void MainloopContext::AddIdToSet(osMessageQueueId_t aId)
{
    mEventIdSet.insert(aId);

    mMaxId = std::max(mMaxId, aId);
}

void MainloopContext::ClearIdSet(void)
{
    mEventIdSet.clear();
}

int32_t MainloopContext::WaitEvents(void)
{
    Milliseconds timeout = mTimeOut + Clock::now();
    int32_t      rval    = 0;
    uint32_t     id;

    do
    {
        for (auto eventid : mEventIdSet)
        {
            id = osMessageQueueGetCount(eventid);
            if (id > 0)
            {
                rval++;
            }
        }
    } while ((rval == 0) && (timeout > Clock::now())); // && (timeout > Clock::now())

    return rval;
}

} // namespace otbr
