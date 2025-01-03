/*******************************************************************************
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

#ifndef _SL_OTBR_UTILS_H
#define _SL_OTBR_UTILS_H
#include "cmsis_os2.h"

namespace otbr {

template <class Ft> class OtbrFuture
{
public:
    Ft                 mState;
    osMessageQueueId_t mMsgId;

    OtbrFuture(void);

    ~OtbrFuture(void);

    Ft Get(void);
};

template <class Pr> class OtbrPromise
{
public:
    void SetValue(Pr aVal);

    OtbrFuture<Pr> GetFuture(void) { return mF; }

private:
    OtbrFuture<Pr> mF;
};

template <class At> class OtbrAtomic
{
public:
    At Value;

    OtbrAtomic(void);

    ~OtbrAtomic(void);

    /**
     * Create mutex for atomic operation.
     * This must be called after RTOS intialized and before using atomic.
     *
     */
    void Init(void);
    void Init(At aVal);

    /**
     * Blocking function to acquire this mutex.
     *
     */
    void Start(void);

    /**
     * Release this mutex.
     *
     */
    void End(void);

private:
    osMutexId_t mOsMutex;
};

} // namespace otbr
#include "sl_otbr_utils.tpp"
#endif // _SL_OTBR_UTILS_H
