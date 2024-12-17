/*
 *    Copyright (c) 2017, The OpenThread Authors.
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
 *   This file includes definition for mDNS publisher based on lwip.
 */

#ifndef OTBR_AGENT_MDNS_LWIP_HPP_
#define OTBR_AGENT_MDNS_LWIP_HPP_

#include "openthread-br/config.h"

#include <memory>
#include <set>
#include <vector>

#include "mdns.hpp"
#include "common/code_utils.hpp"
#include "common/mainloop.hpp"
#include "common/time.hpp"
#include "lwip/apps/mdns.h"

/**
 * @addtogroup border-router-mdns
 *
 * @brief
 *   This module includes definition for lwip-based mDNS publisher.
 *
 * @{
 */

namespace otbr {

namespace Mdns {

/**
 * This class implements mDNS publisher with lwip.
 *
 */
class PublisherLwip : public Publisher
{
public:
    PublisherLwip(StateCallback aStateCallback);
    ~PublisherLwip(void) override;

    void UnpublishService(const std::string &aName, const std::string &aType, ResultCallback &&aCallback) override;
    void UnpublishHost(const std::string &aName, ResultCallback &&aCallback) override;
    // void      UnpublishKey(const std::string &aName, ResultCallback &&aCallback) override;
    void      SubscribeService(const std::string &aType, const std::string &aInstanceName) override;
    void      UnsubscribeService(const std::string &aType, const std::string &aInstanceName) override;
    void      SubscribeHost(const std::string &aHostName) override;
    void      UnsubscribeHost(const std::string &aHostName) override;
    otbrError Start(void) override;
    bool      IsStarted(void) const override;
    void      Stop(void) override;

    static void MdnsLwipReport(struct netif *aNetif, u8_t aResult);
    static void MdnsLwipSrvTxt(struct mdns_service *aService, void *aTxtUser);

protected:
    otbrError PublishServiceImpl(const std::string &aHostName,
                                 const std::string &aName,
                                 const std::string &aType,
                                 const SubTypeList &aSubTypeList,
                                 uint16_t           aPort,
                                 const TxtData     &aTxtData,
                                 ResultCallback   &&aCallback) override;
    otbrError PublishHostImpl(const std::string &aName,
                              const AddressList &aAddresses,
                              ResultCallback   &&aCallback) override;
    // otbrError PublishKeyImpl(const std::string &aName, const KeyData &aKeyData, ResultCallback &&aCallback) override;
    void OnServiceResolveFailedImpl(const std::string &aType,
                                    const std::string &aInstanceName,
                                    int32_t            aErrorCode) override;
    void OnHostResolveFailedImpl(const std::string &aHostName, int32_t aErrorCode) override;
    // otbrError DnsErrorToOtbrError(int32_t aErrorCode) override;

private:
    class LwipServiceRegistration : public ServiceRegistration
    {
    public:
        LwipServiceRegistration(const std::string &aHostName,
                                const std::string &aName,
                                const std::string &aType,
                                const SubTypeList &aSubTypeList,
                                uint16_t           aPort,
                                const TxtData     &aTxtData,
                                ResultCallback   &&aCallback,
                                PublisherLwip     *aPublisher)
            : ServiceRegistration(aHostName,
                                  aName,
                                  aType,
                                  aSubTypeList,
                                  aPort,
                                  aTxtData,
                                  std::move(aCallback),
                                  aPublisher)
        {
            aId = -1;
        }

        ~LwipServiceRegistration(void) override;

        void setId(int _aId) { aId = _aId; }

    private:
        int aId;
    };

    class LwipHostRegistration : public HostRegistration
    {
    public:
        LwipHostRegistration(const std::string &aName,
                             const AddressList &aAddresses,
                             ResultCallback   &&aCallback,
                             PublisherLwip     *aPublisher)
            : HostRegistration(aName, aAddresses, std::move(aCallback), aPublisher)
        {
        }

        ~LwipHostRegistration(void) override;

    private:
    };

    class LwipKeyRegistration : public KeyRegistration
    {
    public:
        LwipKeyRegistration(const std::string &aName,
                            const KeyData     &aKeyData,
                            ResultCallback   &&aCallback,
                            PublisherLwip     *aPublisher)
            : KeyRegistration(aName, aKeyData, std::move(aCallback), aPublisher)
        {
        }

        ~LwipKeyRegistration(void) override;

    private:
    };

    struct Subscription : private ::NonCopyable
    {
        PublisherLwip *mPublisherLwip;

        explicit Subscription(PublisherLwip &aPublisherLwip)
            : mPublisherLwip(&aPublisherLwip)
        {
        }
    };

    struct HostSubscription : public Subscription
    {
        explicit HostSubscription(PublisherLwip &aLwipPublisher, std::string aHostName)
            : Subscription(aLwipPublisher)
            , mHostName(std::move(aHostName))
        {
        }

        ~HostSubscription() { Release(); }

        void        Release(void);
        void        Resolve(void);
        static void HandleResolveResult(const char *aName,
                                        uint16_t    aClazz,
                                        uint16_t    aType,
                                        const void *aRdata,
                                        size_t      aSize,
                                        void       *aContext);

        void HandleResolveResult(const char *aName, uint16_t aClazz, uint16_t aType, const void *aRdata, size_t aSize);

        std::string        mHostName;
        DiscoveredHostInfo mHostInfo;
    };

    struct ServiceResolver
    {
        ~ServiceResolver()
        {
            // if (mServiceResolver)
            {
                // avahi_service_resolver_free(mServiceResolver);
            } // if (mRecordBrowser)
            {
                // avahi_record_browser_free(mRecordBrowser);
            }
        }

        static void HandleResolveServiceResult(const char *aName,
                                               const char *aType,
                                               const char *aDomain,
                                               const char *aHostName,
                                               uint16_t    aPort,
                                               void       *aContext);

        void HandleResolveServiceResult(const char *aName,
                                        const char *aType,
                                        const char *aDomain,
                                        const char *aHostName,

                                        uint16_t aPort);

        static void HandleResolveHostResult(const char *aName,
                                            uint16_t    aClazz,
                                            uint16_t    aType,
                                            const void *aRdata,
                                            size_t      aSize,

                                            void *aContext);

        void HandleResolveHostResult(const char *aName,
                                     uint16_t    aClazz,
                                     uint16_t    aType,
                                     const void *aRdata,
                                     size_t      aSize);

        std::string            mType;
        PublisherLwip         *mPublisherLwip;
        DiscoveredInstanceInfo mInstanceInfo;
    };

    struct ServiceSubscription : public Subscription
    {
        explicit ServiceSubscription(PublisherLwip &aPublisherLwip, std::string aType, std::string aInstanceName)
            : Subscription(aPublisherLwip)
            , mType(std::move(aType))
            , mInstanceName(std::move(aInstanceName))
        {
        }

        ~ServiceSubscription() { Release(); }

        void Release(void);
        void Browse(void);
        void Resolve(uint32_t aInterfaceIndex,

                     const std::string &aInstanceName,
                     const std::string &aType);
        void AddServiceResolver(const std::string &aInstanceName, ServiceResolver *aServiceResolver);
        void RemoveServiceResolver(const std::string &aInstanceName);

        static void HandleBrowseResult(const char *aName,
                                       const char *aType,
                                       const char *aDomain,

                                       void *aContext);

        void HandleBrowseResult(const char *aName, const char *aType, const char *aDomain);

        std::string mType;
        std::string mInstanceName;

        using ServiceResolversMap = std::map<std::string, std::set<ServiceResolver *>>;
        ServiceResolversMap mServiceResolvers;
    };

    typedef std::vector<std::unique_ptr<ServiceSubscription>> ServiceSubscriptionList;
    typedef std::vector<std::unique_ptr<HostSubscription>>    HostSubscriptionList;

    static void HandleClientState(void *aContext);
    void        HandleClientState();

    // ServiceRegistration *FindServiceRegistration(const LwipEntryGroup *aEntryGroup);
    // HostRegistration    *FindHostRegistration(const LwipEntryGroup *aEntryGroup);
    // KeyRegistration     *FindKeyRegistration(const LwipEntryGroup *aEntryGroup);

    bool          mInitialized;
    State         mState;
    StateCallback mStateCallback;

    ServiceSubscriptionList mSubscribedServices;
    HostSubscriptionList    mSubscribedHosts;
};

} // namespace Mdns

} // namespace otbr

/**
 * @}
 */
#endif // OTBR_AGENT_MDNS_LWIP_HPP_
