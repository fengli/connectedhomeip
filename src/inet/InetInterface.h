/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 * @file
 *  This file defines the <tt>Inet::InterfaceId</tt> type alias and related
 *  classes for iterating on the list of system network interfaces and the list
 *  of system interface addresses.
 */

#pragma once

#include <inet/InetConfig.h>

#include <inet/IPAddress.h>
#include <inet/InetError.h>
#include <lib/support/DLLUtil.h>

#if CHIP_SYSTEM_CONFIG_USE_LWIP
#include <lwip/netif.h>
#endif // CHIP_SYSTEM_CONFIG_USE_LWIP

#if CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS
struct if_nameindex;
struct ifaddrs;
#endif // CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS

#if CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF
#include <device.h>

struct net_if;
struct net_if_ipv4;
struct net_if_ipv6;
#endif // CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF

#include <stddef.h>
#include <stdint.h>

namespace chip {
namespace Inet {

class IPAddress;
class IPPrefix;

/**
 * Indicator for system network interfaces.
 */
class InterfaceId
{
public:
#if CHIP_SYSTEM_CONFIG_USE_LWIP
    using PlatformType                       = struct netif *;
    static constexpr size_t kMaxIfNameLength = 13; // Names are formatted as %c%c%d
#endif                                             // CHIP_SYSTEM_CONFIG_USE_LWIP

#if CHIP_SYSTEM_CONFIG_USE_SOCKETS && CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS
    using PlatformType                       = unsigned int;
    static constexpr size_t kMaxIfNameLength = IF_NAMESIZE;
#endif // CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS

#if CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF
    using PlatformType                       = int;
    static constexpr size_t kMaxIfNameLength = Z_DEVICE_MAX_NAME_LEN;
#endif

public:
    ~InterfaceId() = default;

    constexpr InterfaceId() : mPlatformInterface(kPlatformNull) {}
    explicit constexpr InterfaceId(PlatformType interface) : mPlatformInterface(interface) {}

    constexpr InterfaceId(const InterfaceId & other) : mPlatformInterface(other.mPlatformInterface) {}
    constexpr InterfaceId & operator=(const InterfaceId & other)
    {
        mPlatformInterface = other.mPlatformInterface;
        return *this;
    }

    static constexpr InterfaceId Null() { return InterfaceId(); }

    constexpr bool operator==(const InterfaceId & other) const { return mPlatformInterface == other.mPlatformInterface; }
    constexpr bool operator!=(const InterfaceId & other) const { return mPlatformInterface != other.mPlatformInterface; }

    /**
     * Test for inequivalence with the null interface.
     */
    bool IsPresent() const { return mPlatformInterface != kPlatformNull; }

    /**
     * Get the underlying platform representation of the interface.
     */
    PlatformType GetPlatformInterface() const { return mPlatformInterface; }

    /**
     * Get the name of the network interface
     *
     * @param[in]   nameBuf     Region of memory to write the interface name.
     * @param[in]   nameBufSize Size of the region denoted by \c nameBuf.
     *
     * @retval  CHIP_NO_ERROR               Successful result, interface name written.
     * @retval  CHIP_ERROR_BUFFER_TOO_SMALL Buffer is too small for the interface name.
     * @retval  other                       Another system or platform error.
     *
     *  Writes the name of the network interface as a \c NUL terminated text string at \c nameBuf.
     *  The name of the unspecified network interface is the empty string.
     */
    CHIP_ERROR GetInterfaceName(char * nameBuf, size_t nameBufSize) const;

    /**
     * Search the list of network interfaces for the indicated name.
     *
     * @param[in]   intfName    Name of the network interface to find.
     * @param[out]  intfId      Indicator of the network interface to assign.
     *
     * @retval  CHIP_NO_ERROR                   Success, network interface indicated.
     * @retval  INET_ERROR_UNKNOWN_INTERFACE    No network interface found.
     * @retval  other                           Another system or platform error.
     *
     * @note
     *  On LwIP, this function must be called with the LwIP stack lock acquired.
     */
    static CHIP_ERROR InterfaceNameToId(const char * intfName, InterfaceId & intfId);

private:
#if CHIP_SYSTEM_CONFIG_USE_LWIP
    static constexpr PlatformType kPlatformNull = nullptr;
#endif // CHIP_SYSTEM_CONFIG_USE_LWIP

#if CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS
    static constexpr PlatformType kPlatformNull = 0;
#endif // CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS

#if CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF
    static constexpr PlatformType kPlatformNull = 0;
#endif
    PlatformType mPlatformInterface;
};

/**
 * Compute a prefix length from a variable-length netmask.
 */
extern uint8_t NetmaskToPrefixLength(const uint8_t * netmask, uint16_t netmaskLen);

/**
 * @brief   Iterator for the list of system network interfaces.
 *
 * @details
 *  Use objects of this class to iterate the list of system network interfaces.
 *
 *  Methods on an individual instance of this class are *not* thread-safe;
 *  however separate instances may be used simultaneously by multiple threads.
 *
 *  On multi-threaded LwIP systems, instances are thread-safe relative to other
 *  threads accessing the global LwIP state provided that the other threads hold
 *  the LwIP core lock while mutating the list of netifs, and that netif object
 *  themselves are never destroyed.
 *
 *  On sockets-based systems, iteration is always stable in the face of changes
 *  to the underlying system's interfaces.
 *
 *  On LwIP systems, iteration is stable except in the case where the currently
 *  selected interface is removed from the list, in which case iteration ends
 *  immediately.
 */
class InterfaceIterator
{
public:
    /**
     * Constructs an InterfaceIterator object.
     *
     *  Starts the iterator at the first network interface. On some platforms,
     *  this constructor may allocate resources recycled by the destructor.
     */
    InterfaceIterator();
    ~InterfaceIterator();

    /**
     * Test whether the iterator is positioned on an interface
     *
     * @return  \c true if the iterator is positioned on an interface;
     *          \c false if positioned beyond the end of the interface list.
     */
    bool HasCurrent();

    /**
     * Advance the iterator to the next network interface.
     *
     * @return  \c false if advanced beyond the end, else \c true.
     *
     *  Advances the internal iterator to the next network interface or to a position
     *  beyond the end of the interface list.
     *
     *  On multi-threaded LwIP systems, this method is thread-safe relative to other
     *  threads accessing the global LwIP state provided that: 1) the other threads
     *  hold the LwIP core lock while mutating the list of netifs; and 2) netif objects
     *  themselves are never destroyed.
     *
     *  Iteration is stable in the face of changes to the underlying system's
     *  interfaces, *except* in the case of LwIP systems when the currently selected
     *  interface is removed from the list, which causes iteration to end immediately.
     */
    bool Next();

    /**
     * InterfaceId InterfaceIterator::GetInterfaceId(void)
     *
     * Returns the network interface id at the current iterator position.
     *
     * @retval  id                   The current network interface id.
     * @retval  InterfaceId()   If advanced beyond the end of the list.
     */
    InterfaceId GetInterfaceId();

    /**
     * Get the name of the current network interface
     *
     * @param[in]   nameBuf     Region of memory to write the interface name.
     * @param[in]   nameBufSize Size of the region denoted by \c nameBuf.
     *
     * @retval  CHIP_NO_ERROR               Successful result, interface name written.
     * @retval  CHIP_ERROR_INCORRECT_STATE  Iterator is positioned beyond the end of the list.
     * @retval  CHIP_ERROR_BUFFER_TOO_SMALL Name is too large to be written in buffer.
     * @retval  other                       Another system or platform error.
     *
     *  Writes the name of the network interface as \c NUL terminated text string at \c nameBuf.
     */
    CHIP_ERROR GetInterfaceName(char * nameBuf, size_t nameBufSize);

    /**
     * Returns whether the current network interface is up.
     *
     * @return  \c true if current network interface is up, \c false if not
     *          or if the iterator is positioned beyond the end of the list.
     */
    bool IsUp();

    /**
     * Returns whether the current network interface supports multicast.
     *
     * @return  \c true if current network interface supports multicast, \c false
     *          if not, or if the iterator is positioned beyond the end of the list.
     */
    bool SupportsMulticast();

    /**
     * Returns whether the current network interface has a broadcast address.
     *
     * @return  \c true if current network interface has a broadcast address, \c false
     *          if not, or if the iterator is positioned beyond the end of the list.
     */
    bool HasBroadcastAddress();

protected:
#if CHIP_SYSTEM_CONFIG_USE_LWIP
    struct netif * mCurNetif;
#endif // CHIP_SYSTEM_CONFIG_USE_LWIP

#if CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS
    struct if_nameindex * mIntfArray;
    size_t mCurIntf;
    short mIntfFlags;
    bool mIntfFlagsCached;

    short GetFlags();
#endif // CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS

#if CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF
    InterfaceId::PlatformType mCurrentId = 1;
    net_if * mCurrentInterface           = nullptr;
#endif // CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF
};

/**
 * @brief   Iterator for the list of system network interface IP addresses.
 *
 * @details
 *  Use objects of this class to iterate the list of system network interface
 *  interface IP addresses.
 *
 *  Methods on an individual instance of this class are *not* thread-safe;
 *  however separate instances may be used simultaneously by multiple threads.
 *
 *  On multi-threaded LwIP systems, instances are thread-safe relative to other
 *  threads accessing the global LwIP state provided that: 1) other threads hold
 *  the LwIP core lock while mutating the list of netifs; and 2) netif object
 *  themselves are never destroyed.
 *
 *  On sockets-based systems, iteration is always stable in the face of changes
 *  to the underlying system's interfaces and/or addresses.
 *
 *  On LwIP systems, iteration is stable except in the case where the interface
 *  associated with the current address is removed, in which case iteration may
 *  end prematurely.
 */
class DLL_EXPORT InterfaceAddressIterator
{
public:
    /**
     * Constructs an InterfaceAddressIterator object.
     *
     *  Starts the iterator at the first network address. On some platforms,
     *  this constructor may allocate resources recycled by the destructor.
     */
    InterfaceAddressIterator();

    /**
     * Destroys an InterfaceAddressIterator object.
     *
     *  Recycles any resources allocated by the constructor.
     */
    ~InterfaceAddressIterator();

    /**
     * Test whether the iterator is positioned on an interface address
     *
     * @return  \c true if the iterator is positioned on an interface address;
     *          \c false if positioned beyond the end of the address list.
     */
    bool HasCurrent();

    /**
     * @fn      bool InterfaceAddressIterator::Next(void)
     *
     * @brief   Advance the iterator to the next interface address.
     *
     * @return  \c false if advanced beyond the end, else \c true.
     *
     * @details
     *     Advances the iterator to the next interface address or to a position
     *     beyond the end of the address list.
     *
     *     On LwIP, this method is thread-safe provided that: 1) other threads hold
     *     the LwIP core lock while mutating the netif list; and 2) netif objects
     *     themselves are never destroyed.  Additionally, iteration on LwIP systems
     *     will terminate early if the current interface is removed from the list.
     */
    bool Next();

    /**
     * @fn      IPAddress InterfaceAddressIterator::GetAddress(void)
     *
     * @brief   Get the current interface address.
     *
     * @return  the current interface address or \c IPAddress::Any if the iterator
     *          is positioned beyond the end of the address list.
     */
    IPAddress GetAddress();

    /**
     * @fn      uint8_t InterfaceAddressIterator::GetPrefixLength(void)
     *
     * @brief   Gets the network prefix associated with the current interface address.
     *
     * @return  the network prefix (in bits) or 0 if the iterator is positioned beyond
     *          the end of the address list.
     *
     * @details
     *     On LwIP, this method simply returns the hard-coded constant 64.
     *
     *     Note Well: the standard subnet prefix on all links other than PPP
     *     links is 64 bits. On PPP links and some non-broadcast multipoint access
     *     links, the convention is either 127 bits or 128 bits, but it might be
     *     something else. On most platforms, the system's interface address
     *     structure can represent arbitrary prefix lengths between 0 and 128.
     */
    uint8_t GetPrefixLength();

    /**
     * @fn       void InterfaceAddressIterator::GetAddressWithPrefix(IPPrefix & addrWithPrefix)
     *
     * @brief    Returns an IPPrefix containing the address and prefix length
     *           for the current address.
     */
    void GetAddressWithPrefix(IPPrefix & addrWithPrefix);

    /**
     * @fn      InterfaceId InterfaceAddressIterator::GetInterfaceId(void)
     *
     * @brief   Returns the network interface id associated with the current
     *          interface address.
     *
     * @return  the interface id or \c InterfaceId() if the iterator
     *          is positioned beyond the end of the address list.
     */
    InterfaceId GetInterfaceId();

    /**
     * @fn      CHIP_ERROR InterfaceAddressIterator::GetInterfaceName(char * nameBuf, size_t nameBufSize)
     *
     * @brief   Get the name of the network interface associated with the
     *          current interface address.
     *
     * @param[in]   nameBuf     region of memory to write the interface name
     * @param[in]   nameBufSize size of the region denoted by \c nameBuf
     *
     * @retval  CHIP_NO_ERROR           successful result, interface name written
     * @retval  CHIP_ERROR_BUFFER_TOO_SMALL    name is too large to be written in buffer
     * @retval  CHIP_ERROR_INCORRECT_STATE
     *                                  the iterator is not currently positioned on an
     *                                  interface address
     * @retval  other                   another system or platform error
     *
     * @details
     *     Writes the name of the network interface as \c NUL terminated text string
     *     at \c nameBuf.
     */
    CHIP_ERROR GetInterfaceName(char * nameBuf, size_t nameBufSize);

    /**
     * Returns whether the network interface associated with the current interface address is up.
     *
     * @return  \c true if current network interface is up, \c false if not, or
     *          if the iterator is not positioned on an interface address.
     */
    bool IsUp();

    /**
     * Returns whether the network interface associated with the current interface address supports multicast.
     *
     * @return  \c true if multicast is supported, \c false if not, or
     *          if the iterator is not positioned on an interface address.
     */
    bool SupportsMulticast();

    /**
     * Returns whether the network interface associated with the current interface address has an IPv4 broadcast address.
     *
     * @return  \c true if the interface has a broadcast address, \c false if not, or
     *          if the iterator is not positioned on an interface address.
     */
    bool HasBroadcastAddress();

private:
#if CHIP_SYSTEM_CONFIG_USE_LWIP
    enum
    {
        kBeforeStartIndex = -1
    };

    InterfaceIterator mIntfIter;
    int mCurAddrIndex;
#endif // CHIP_SYSTEM_CONFIG_USE_LWIP

#if CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS
    struct ifaddrs * mAddrsList;
    struct ifaddrs * mCurAddr;
#endif // CHIP_SYSTEM_CONFIG_USE_BSD_IFADDRS

#if CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF
    InterfaceIterator mIntfIter;
    net_if_ipv6 * mIpv6 = nullptr;
    int mCurAddrIndex   = -1;
#endif // CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF
};

#if CHIP_SYSTEM_CONFIG_USE_LWIP

inline InterfaceIterator::InterfaceIterator(void)
{
    mCurNetif = netif_list;
}

inline InterfaceIterator::~InterfaceIterator(void) {}

inline bool InterfaceIterator::HasCurrent(void)
{
    return mCurNetif != NULL;
}

inline InterfaceId InterfaceIterator::GetInterfaceId(void)
{
    return InterfaceId(mCurNetif);
}

inline InterfaceAddressIterator::InterfaceAddressIterator(void)
{
    mCurAddrIndex = kBeforeStartIndex;
}

inline InterfaceAddressIterator::~InterfaceAddressIterator(void) {}

#endif // CHIP_SYSTEM_CONFIG_USE_LWIP

#if CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF
inline InterfaceIterator::~InterfaceIterator()               = default;
inline InterfaceAddressIterator::~InterfaceAddressIterator() = default;
#endif // CHIP_SYSTEM_CONFIG_USE_ZEPHYR_NET_IF

} // namespace Inet
} // namespace chip
