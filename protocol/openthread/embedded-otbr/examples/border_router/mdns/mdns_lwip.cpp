/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 *
 */

#include "mdns_lwip.hpp"
#include <sstream>
#include "common/logging.hpp"

std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> result;
    std::stringstream        ss(s);
    std::string              item;

    while (getline(ss, item, delim))
    {
        result.push_back(item);
    }

    return result;
}

namespace otbr {
namespace Mdns {

void PublisherLwip::MdnsLwipReport(struct netif *aNetif, u8_t aResult)
{
    otbrLogInfo("mdns status[netif %d]: %d", aNetif->num, aResult);
    if (aResult == MDNS_PROBING_SUCCESSFUL)
    {
    }
    else if (aResult == MDNS_PROBING_CONFLICT)
    {
    }
}

void PublisherLwip::MdnsLwipSrvTxt(struct mdns_service *aService, void *aTxtUser)
{
    err_t                                   res;
    PublisherLwip::LwipServiceRegistration *serviceReg =
        static_cast<PublisherLwip::LwipServiceRegistration *>(aTxtUser);
    uint8_t *mData  = serviceReg->mTxtData.data();
    int      mSize  = serviceReg->mTxtData.size();
    int      mCount = 0;

    do
    {
        uint8_t itemSize = mData[mCount];
        mCount++;
        if (itemSize + mCount > mSize)
        {
            printf("WHY?\r\n");
            // serviceReg->mCallback()
        }

        res = mdns_resp_add_service_txtitem(aService, (char *)&mData[mCount], itemSize);
        if (res != ERR_OK)
        {
            printf("error %d\r\n", res);
            break;
        }
        mCount += itemSize;
    } while (mCount < mSize);
}

Publisher *Publisher::Create(StateCallback aCallback)
{
    return new PublisherLwip(aCallback);
}

void Publisher::Destroy(Publisher *aPublisher)
{
    delete static_cast<PublisherLwip *>(aPublisher);
}

PublisherLwip::PublisherLwip(StateCallback aStateCallback)
    : mState(State::kIdle)
    , mStateCallback(std::move(aStateCallback))
{
    mInitialized = false;
}

PublisherLwip::~PublisherLwip(void)
{
    Stop();
}

PublisherLwip::LwipServiceRegistration::~LwipServiceRegistration(void)
{
    // ReleaseGroup(mEntryGroup);
}

PublisherLwip::LwipHostRegistration::~LwipHostRegistration(void)
{
    // ReleaseGroup(mEntryGroup);
}

PublisherLwip::LwipKeyRegistration::~LwipKeyRegistration(void)
{
    // ReleaseGroup(mEntryGroup);
}

otbrError PublisherLwip::Start(void)
{
    otbrError error = OTBR_ERROR_NONE;

    if (mInitialized)
    {
        return error;
    }

    mdns_resp_register_name_result_cb(PublisherLwip::MdnsLwipReport);
    mdns_resp_init();
    mInitialized = true;

    return error;
}

bool PublisherLwip::IsStarted(void) const
{
    return mInitialized;
}

void PublisherLwip::Stop(void)
{
    if (mInitialized)
    {
        mInitialized = false;
    }
    else
    {
        return;
    }

    mServiceRegistrations.clear();
    mHostRegistrations.clear();

    mSubscribedServices.clear();
    mSubscribedHosts.clear();

    mState = Mdns::Publisher::State::kIdle;
}

void PublisherLwip::ServiceSubscription::Release(void)
{
}

void PublisherLwip::HostSubscription::Release(void)
{
}

void PublisherLwip::SubscribeService(const std::string &aType, const std::string &aInstanceName)
{
    OTBR_UNUSED_VARIABLE(aType);
    OTBR_UNUSED_VARIABLE(aInstanceName);
}

void PublisherLwip::UnsubscribeService(const std::string &aType, const std::string &aInstanceName)
{
    OTBR_UNUSED_VARIABLE(aType);
    OTBR_UNUSED_VARIABLE(aInstanceName);
}

void PublisherLwip::UnpublishService(const std::string &aName, const std::string &aType, ResultCallback &&aCallback)
{
    OTBR_UNUSED_VARIABLE(aName);
    OTBR_UNUSED_VARIABLE(aType);
    OTBR_UNUSED_VARIABLE(aCallback);
}

void PublisherLwip::UnpublishHost(const std::string &aName, ResultCallback &&aCallback)
{
    OTBR_UNUSED_VARIABLE(aName);
    OTBR_UNUSED_VARIABLE(aCallback);
}

void PublisherLwip::SubscribeHost(const std::string &aHostName)
{
    OTBR_UNUSED_VARIABLE(aHostName);
}

void PublisherLwip::UnsubscribeHost(const std::string &aHostName)
{
    OTBR_UNUSED_VARIABLE(aHostName);
}

otbrError PublisherLwip::PublishHostImpl(const std::string &aName,
                                         const AddressList &aAddresses,
                                         ResultCallback   &&aCallback)
{
    OTBR_UNUSED_VARIABLE(aName);
    OTBR_UNUSED_VARIABLE(aAddresses);
    OTBR_UNUSED_VARIABLE(aCallback);

    return OTBR_ERROR_NONE;
}

void PublisherLwip::OnServiceResolveFailedImpl(const std::string &aType,
                                               const std::string &aInstanceName,
                                               int32_t            aErrorCode)
{
    OTBR_UNUSED_VARIABLE(aType);
    OTBR_UNUSED_VARIABLE(aInstanceName);
    OTBR_UNUSED_VARIABLE(aErrorCode);
}

void PublisherLwip::OnHostResolveFailedImpl(const std::string &aHostName, int32_t aErrorCode)
{
    OTBR_UNUSED_VARIABLE(aHostName);
    OTBR_UNUSED_VARIABLE(aErrorCode);
}

otbrError PublisherLwip::PublishServiceImpl(const std::string &aHostName,
                                            const std::string &aName,
                                            const std::string &aType,
                                            const SubTypeList &aSubTypeList,
                                            uint16_t           aPort,
                                            const TxtData     &aTxtData,
                                            ResultCallback   &&aCallback)
{
    otbrError                error             = OTBR_ERROR_NONE;
    SubTypeList              sortedSubTypeList = SortSubTypeList(aSubTypeList);
    LwipServiceRegistration *serviceReg;
    // std::string               regType           = MakeRegType(aType, sortedSubTypeList);
    mdns_sd_proto proto = DNSSD_PROTO_TCP;
    int           result;

    std::vector<std::string> aTypeList = split(aType, '.');

    // if (mState != State::kReady)
    // {
    //     error = OTBR_ERROR_INVALID_STATE;
    //     std::move(aCallback)(error);
    //     ExitNow();
    // }

    if (aTypeList.size() == 0)
    {
        error = OTBR_ERROR_MDNS;
        std::move(aCallback)(error);
        ExitNow();
    }

    aCallback = HandleDuplicateServiceRegistration(aHostName,
                                                   aName,
                                                   aType,
                                                   sortedSubTypeList,
                                                   aPort,
                                                   aTxtData,
                                                   std::move(aCallback));
    VerifyOrExit(!aCallback.IsNull());

    serviceReg = new LwipServiceRegistration(aHostName,
                                             aName,
                                             aType,
                                             sortedSubTypeList,
                                             aPort,
                                             aTxtData,
                                             std::move(aCallback),
                                             this);

    if (aType.find("udp") != std::string::npos)
    {
        proto = DNSSD_PROTO_UDP;
    }

    mdns_resp_add_netif(netif_default, "SiWx917", 3600);
    result = mdns_resp_add_service(netif_default,
                                   aName.c_str(),
                                   aTypeList[0].c_str(),
                                   proto,
                                   aPort,
                                   3600,
                                   PublisherLwip::MdnsLwipSrvTxt,
                                   serviceReg);
    if (result >= 0)
    {
        serviceReg->setId(result);
        AddServiceRegistration(std::unique_ptr<LwipServiceRegistration>(serviceReg));
    }
    else
    {
        // Error
    }

    mdns_resp_announce(netif_default);

exit:
    return error;
}

} // namespace Mdns

} // namespace otbr
