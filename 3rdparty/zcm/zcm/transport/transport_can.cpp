#include "zcm/transport.h"
#include "zcm/transport_registrar.h"
#include "zcm/transport_register.hpp"
#include "zcm/util/debug.h"

#include "generic_serial_transport.h"

#include "util/TimeUtil.hpp"

#include <unistd.h>
#include <string.h>
#include <iostream>
#include <limits>
#include <cstdio>
#include <cassert>
#include <unordered_map>
#include <algorithm>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/sockios.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#define MASK_29B ((1 << 29) - 1)
#define MASK_11B ((1 << 11) - 1)

// Define this the class name you want
#define ZCM_TRANS_CLASSNAME TransportCan
#define MTU (1<<14)

using namespace std;

struct ZCM_TRANS_CLASSNAME : public zcm_trans_t
{
    unordered_map<string, string> options;
    uint32_t msgId;
    uint32_t txId;
    string address;

    int soc = -1;
    bool socSettingsGood = false;
    struct sockaddr_can addr;
	struct ifreq ifr;

    zcm_trans_t* gst = nullptr;

    uint64_t timeoutLeftUs = 0;

    string* findOption(const string& s)
    {
        auto it = options.find(s);
        if (it == options.end()) return nullptr;
        return &it->second;
    }

    ZCM_TRANS_CLASSNAME(zcm_url_t* url)
    {
        trans_type = ZCM_BLOCKING;
        vtbl = &methods;

        // build 'options'
        auto* opts = zcm_url_opts(url);
        for (size_t i = 0; i < opts->numopts; ++i)
            options[opts->name[i]] = opts->value[i];

        msgId = 0;
        auto* msgIdStr = findOption("msgid");
        if (!msgIdStr) {
            ZCM_DEBUG("Msg Id unspecified");
            return;
        } else {
            char *endptr;
            msgId = (strtoul(msgIdStr->c_str(), &endptr, 10) & MASK_29B);
            if (*endptr != '\0') {
                ZCM_DEBUG("Msg Id unspecified");
                return;
            }
        }

        auto* txAddrMode = findOption("tx_addr_mode");
        bool extendedTx;
        if (!txAddrMode || string("extended") == txAddrMode->c_str()) {
            extendedTx = true;
        } else if (string("standard") == txAddrMode->c_str()) {
            extendedTx = false;
        } else {
            ZCM_DEBUG("Invalid rx_addr_mode. Use 'extended' or 'standard'");
            return;
        }
        if (!extendedTx && ((msgId & MASK_11B) != msgId)) {
            ZCM_DEBUG("Msg Id too long for standard can addresses. "
                      "Use 'tx_extended_addr=true'");
            return;
        }
        if (extendedTx && ((msgId & MASK_29B) != msgId)) {
            ZCM_DEBUG("Msg Id too long for extended can addresses.");
            return;
        }
        txId = extendedTx ? msgId | CAN_EFF_FLAG : msgId;

        struct can_filter rfilter[2];
        size_t numRxFilters = 0;

        auto* rxAddrMode = findOption("rx_addr_mode");
        if (!rxAddrMode || string("both") == rxAddrMode->c_str()) {
            rfilter[0].can_id   = msgId;
            rfilter[0].can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_SFF_MASK);
            rfilter[1].can_id   = msgId | CAN_EFF_FLAG;
            rfilter[1].can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_EFF_MASK);
            numRxFilters = 2;
        } else if (string("standard") == rxAddrMode->c_str()) {
            rfilter[0].can_id   = msgId;
            rfilter[0].can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_SFF_MASK);
            numRxFilters = 1;
        } else if (string("extended") == rxAddrMode->c_str()) {
            rfilter[0].can_id   = msgId | CAN_EFF_FLAG;
            rfilter[0].can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_EFF_MASK);
            numRxFilters = 1;
        } else {
            ZCM_DEBUG("Invalid rx_addr_mode. Use 'extended', 'standard', or 'both'");
            return;
        }

        address = zcm_url_address(url);

        if ((soc = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
            ZCM_DEBUG("Unable to make socket");
		    return;
	    }

        strcpy(ifr.ifr_name, address.c_str());
        ioctl(soc, SIOCGIFINDEX, &ifr);

        memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            ZCM_DEBUG("Failed to bind");
            return;
        }

        if (setsockopt(soc, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter,
                       numRxFilters * sizeof(struct can_filter)) < 0) {
            ZCM_DEBUG("Failed to set filter");
            return;
        }

        gst = zcm_trans_generic_serial_create(&ZCM_TRANS_CLASSNAME::get,
                                              &ZCM_TRANS_CLASSNAME::put,
                                              this,
                                              &ZCM_TRANS_CLASSNAME::timestamp_now,
                                              this,
                                              MTU, MTU * 10);
        socSettingsGood = true;
    }

    ~ZCM_TRANS_CLASSNAME()
    {
        if (gst) zcm_trans_generic_serial_destroy(gst);
        if (soc != -1 && close(soc) < 0) {
            ZCM_DEBUG("Failed to close");
	    }
        soc = -1;
    }

    bool good()
    {
        return soc != -1 && socSettingsGood;
    }

    static size_t get(uint8_t* data, size_t nData, void* usr)
    {
        ZCM_TRANS_CLASSNAME* me = cast((zcm_trans_t*) usr);

        uint64_t readStartUtime = TimeUtil::utime();

        struct can_frame frame;
        int nbytes = read(me->soc, &frame, sizeof(struct can_frame));
        if (nbytes != sizeof(struct can_frame)) {
            // Sleeping if read returned an error in case read didn't
            // expire all the remaining timeout
            if (nbytes < 0) {
                uint64_t diff = TimeUtil::utime() - readStartUtime;
                if (me->timeoutLeftUs <= diff) return 0;
                usleep(me->timeoutLeftUs - diff);
            }
            return 0;
        }

        // XXX This isn't okay. We're just throwing out data if it
        //     doesn't fit in data on this one call to get
        size_t ret = min(nData, (size_t) frame.can_dlc);
        memcpy(data, frame.data, ret);
        return ret;
    }

    static size_t sendFrame(const uint8_t* data, size_t nData, void* usr)
    {
        ZCM_TRANS_CLASSNAME* me = cast((zcm_trans_t*) usr);

        struct can_frame frame;
        frame.can_id = me->txId;

        size_t ret = min(nData, (size_t) CAN_MAX_DLEN);
        frame.can_dlc = ret;
        memcpy(frame.data, data, ret);

        if (write(me->soc, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            ZCM_DEBUG("Failed to write data");
            return 0;
        }

        return ret;
    }

    static size_t put(const uint8_t* data, size_t nData, void* usr)
    {
        size_t ret = 0;
        while (ret < nData) {
            size_t left = nData - ret;
            size_t written = sendFrame(&data[ret], left, usr);
            if (written == 0) return ret;
            ret += written;
        }
        return ret;
    }

    static uint64_t timestamp_now(void* usr)
    {
        ZCM_TRANS_CLASSNAME* me = cast((zcm_trans_t*) usr);

        struct timeval time;
        if (ioctl(me->soc, SIOCGSTAMP, &time) == -1) return 0;
        return time.tv_sec * 1e6 + time.tv_usec;
    }

    /********************** METHODS **********************/
    size_t get_mtu()
    {
        return zcm_trans_get_mtu(this->gst);
    }

    int sendmsg(zcm_msg_t msg)
    {
        int ret = zcm_trans_sendmsg(this->gst, msg);
        if (ret != ZCM_EOK) return ret;
        return serial_update_tx(this->gst);
    }

    int recvmsgEnable(const char* channel, bool enable)
    {
        return zcm_trans_recvmsg_enable(this->gst, channel, enable);
    }

    int recvmsg(zcm_msg_t* msg, unsigned timeoutMs)
    {
        uint64_t startUtime = TimeUtil::utime();
        timeoutLeftUs = timeoutMs * 1000;

        do {
            int ret = zcm_trans_recvmsg(this->gst, msg, 0);
            if (ret == ZCM_EOK) return ret;

            unsigned timeoutS = timeoutLeftUs / 1000000;
            unsigned timeoutUs = timeoutLeftUs - timeoutS * 1000000;
            struct timeval tm = {
                timeoutS,  /* seconds */
                timeoutUs, /* micros */
            };
            if (setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, (char *)&tm, sizeof(tm)) < 0) {
                ZCM_DEBUG("Failed to settimeout");
                return ZCM_EUNKNOWN;
            }

            serial_update_rx(this->gst);

            uint64_t diff = TimeUtil::utime() - startUtime;
            timeoutLeftUs = timeoutLeftUs > diff ? timeoutLeftUs - diff : 0;
        } while (timeoutLeftUs > 0);
        return ZCM_EAGAIN;
    }

    /********************** STATICS **********************/
    static zcm_trans_methods_t methods;
    static ZCM_TRANS_CLASSNAME *cast(zcm_trans_t *zt)
    {
        assert(zt->vtbl == &methods);
        return (ZCM_TRANS_CLASSNAME*)zt;
    }

    static size_t _get_mtu(zcm_trans_t *zt)
    { return cast(zt)->get_mtu(); }

    static int _sendmsg(zcm_trans_t *zt, zcm_msg_t msg)
    { return cast(zt)->sendmsg(msg); }

    static int _recvmsg_enable(zcm_trans_t *zt, const char *channel, bool enable)
    { return cast(zt)->recvmsgEnable(channel, enable); }

    static int _recvmsg(zcm_trans_t *zt, zcm_msg_t *msg, unsigned timeout)
    { return cast(zt)->recvmsg(msg, timeout); }

    static void _destroy(zcm_trans_t *zt)
    { delete cast(zt); }

    /** If you choose to use the registrar, use a static registration member **/
    static const TransportRegister reg;
};

zcm_trans_methods_t ZCM_TRANS_CLASSNAME::methods = {
    &ZCM_TRANS_CLASSNAME::_get_mtu,
    &ZCM_TRANS_CLASSNAME::_sendmsg,
    &ZCM_TRANS_CLASSNAME::_recvmsg_enable,
    &ZCM_TRANS_CLASSNAME::_recvmsg,
    NULL, // drops
    NULL, // update
    &ZCM_TRANS_CLASSNAME::_destroy,
};

static zcm_trans_t *create(zcm_url_t* url, char **opt_errmsg)
{
    if (opt_errmsg) *opt_errmsg = NULL; // Feature unused in this transport

    auto* trans = new ZCM_TRANS_CLASSNAME(url);
    if (trans->good())
        return trans;

    delete trans;
    return nullptr;
}

#ifdef USING_TRANS_CAN
const TransportRegister ZCM_TRANS_CLASSNAME::reg(
    "can", "Transfer data via a socket CAN connection on a single id "
           "(e.g. 'can://can0?msgid=65536&rx_extended_addr=standard&tx_extended_addr=true')", create);
#endif

extern "C" void register_can_plugin()
{
    (void)&ZCM_TRANS_CLASSNAME::reg;
}