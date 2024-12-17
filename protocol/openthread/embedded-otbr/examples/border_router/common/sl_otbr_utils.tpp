/***************************************************************************//**
 * @file
 * @brief Utils file.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "sl_otbr_utils.hpp"
#include "otbr_code_utils.hpp"

namespace otbr {

template <class Ft>
OtbrFuture<Ft>::OtbrFuture(void)
{
    mMsgId = osMessageQueueNew(1, sizeof(mState), NULL);
    VerifyOrDie(NULL == mMsgId, strerror(errno));
}

template <class Ft>
OtbrFuture<Ft>::~OtbrFuture(void)
{
    osStatus_t Status = osMessageQueueDelete(mMsgId);
    VerifyOrDie(osOK != Status, strerror(errno));
}

template <class Ft>
Ft OtbrFuture<Ft>::Get(void)
{
    osStatus_t Status = osMessageQueueGet(mMsgId, &mState, NULL, osWaitForever);
    VerifyOrDie(osOK != Status, strerror(errno));
    return mState;
}

template <class Pr>
void OtbrPromise<Pr>::SetValue(Pr aVal)
{
    osStatus_t Status = osMessageQueuePut(mF.mMsgId, &aVal, 1, 0);
    VerifyOrDie(osOK != Status, strerror(errno));
}

template <class At>
OtbrAtomic<At>::OtbrAtomic(void)
{
    mOsMutex = NULL;
}

template <class At>
void OtbrAtomic<At>::Init(void)
{
    if (NULL == mOsMutex)
    {
        mOsMutex = osMutexNew(NULL);
        VerifyOrDie(NULL == mOsMutex, "");
    }
}

template <class At>
void OtbrAtomic<At>::Init(At aVal)
{
    if (NULL == mOsMutex)
    {
        mOsMutex = osMutexNew(NULL);
        VerifyOrDie(NULL == mOsMutex, "");
    }

    Value = aVal;
}

template <class At>
OtbrAtomic<At>::~OtbrAtomic(void)
{
    if (NULL != mOsMutex)
    {
        osStatus_t status = osMutexDelete(mOsMutex);
        VerifyOrDie(osOK != status, "");
    }
}

template <class At>
void OtbrAtomic<At>::Start(void)
{
    VerifyOrDie(NULL == mOsMutex, "");
    // Timeout is not supported
    osStatus_t status = osMutexAcquire(mOsMutex, osWaitForever);
    VerifyOrDie(osOK != status, "");
}

template <class At>
void OtbrAtomic<At>::End(void)
{
    VerifyOrDie(NULL == mOsMutex, "");
    osStatus_t status = osMutexRelease(mOsMutex);
    VerifyOrDie(osOK != status, "");
}

}