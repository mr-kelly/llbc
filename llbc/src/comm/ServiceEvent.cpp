/**
 * @file    ServiceEvent.cpp
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2014/11/14
 * @version 1.0
 *
 * @brief
 */

#include "llbc/common/Export.h"
#include "llbc/common/BeforeIncl.h"

#include "llbc/comm/Packet.h"
#include "llbc/comm/protocol/ProtocolLayer.h"
#include "llbc/comm/protocol/ProtoReportLevel.h"
#include "llbc/comm/ServiceEvent.h"

namespace
{
    typedef LLBC_NS LLBC_ServiceEvent Base;
    typedef LLBC_NS LLBC_MessageBlock _Block;
    typedef LLBC_NS LLBC_SvcEvType _EvType;

    template <typename Ev>
    static _Block *__CreateEvBlock(Ev *ev)
    {
        _Block *block = LLBC_New1(_Block, sizeof(int) + sizeof(Ev *));
        block->Write(&ev->type, sizeof(int));
        block->Write(&ev, sizeof(Ev *));

        return block;
    }
}

__LLBC_NS_BEGIN

LLBC_ServiceEvent::LLBC_ServiceEvent(int type)
: type(type)
{
}

LLBC_ServiceEvent::~LLBC_ServiceEvent()
{
}

LLBC_SvcEv_SessionCreate::LLBC_SvcEv_SessionCreate()
: Base(_EvType::SessionCreate)
{
}

LLBC_SvcEv_SessionCreate::~LLBC_SvcEv_SessionCreate()
{
}

LLBC_SvcEv_SessionDestroy::LLBC_SvcEv_SessionDestroy()
: Base(_EvType::SessionDestroy)
{
}

LLBC_SvcEv_SessionDestroy::~LLBC_SvcEv_SessionDestroy()
{
}

LLBC_SvcEv_AsyncConn::LLBC_SvcEv_AsyncConn()
: Base(_EvType::AsyncConnResult)
{
}

LLBC_SvcEv_AsyncConn::~LLBC_SvcEv_AsyncConn()
{
}

LLBC_SvcEv_DataArrival::LLBC_SvcEv_DataArrival()
: Base(_EvType::DataArrival)
, packet(NULL)
{
}

LLBC_SvcEv_DataArrival::~LLBC_SvcEv_DataArrival()
{
    LLBC_XDelete(packet);
}

LLBC_SvcEv_ProtoReport::LLBC_SvcEv_ProtoReport()
: Base(_EvType::ProtoReport)
, sessionId(0)

, layer(LLBC_ProtocolLayer::End)
, level(LLBC_ProtoReportLevel::Error)
, report()
{
}

LLBC_SvcEv_ProtoReport::~LLBC_SvcEv_ProtoReport()
{
}

LLBC_SvcEv_SubscribeEv::LLBC_SvcEv_SubscribeEv()
: Base(_EvType::SubscribeEv)
, id(0)
, stub()
, deleg(NULL)
{
}

LLBC_SvcEv_SubscribeEv::~LLBC_SvcEv_SubscribeEv()
{
    LLBC_XDelete(deleg);
}

LLBC_SvcEv_UnsubscribeEv::LLBC_SvcEv_UnsubscribeEv()
: Base(_EvType::UnsubscribeEv)
, id(0)
, stub()
{
}

LLBC_SvcEv_UnsubscribeEv::~LLBC_SvcEv_UnsubscribeEv()
{
}

LLBC_SvcEv_FireEv::LLBC_SvcEv_FireEv()
: Base(_EvType::FireEv)
, ev(NULL)
{
}

LLBC_SvcEv_FireEv::~LLBC_SvcEv_FireEv()
{
    LLBC_XDelete(ev);
}

LLBC_MessageBlock *LLBC_SvcEvUtil::BuildSessionCreateEv(const LLBC_SockAddr_IN &local,
                                                        const LLBC_SockAddr_IN &peer,
                                                        bool isListen,
                                                        int sessionId,
                                                        LLBC_SocketHandle handle)
{
    typedef LLBC_SvcEv_SessionCreate _Ev;

    _Ev *ev = LLBC_New(_Ev);
    ev->isListen = isListen;
    ev->sessionId = sessionId;
    ev->local = local;
    ev->peer = peer;
    ev->handle = handle;

    return __CreateEvBlock(ev);
}

LLBC_MessageBlock *LLBC_SvcEvUtil::BuildSessionDestroyEv(int sessionId)
{
    typedef LLBC_SvcEv_SessionDestroy _Ev;

    _Ev *ev = LLBC_New(_Ev);
    ev->sessionId = sessionId;

    return __CreateEvBlock(ev);
}

LLBC_MessageBlock *LLBC_SvcEvUtil::BuildAsyncConnResultEv(bool connected,
                                                          const LLBC_String &reason,
                                                          const LLBC_SockAddr_IN &peer)
{
    typedef LLBC_SvcEv_AsyncConn _Ev;

    _Ev *ev = LLBC_New(_Ev);
    ev->connected = connected;
    ev->reason.append(reason);
    ev->peer = peer;

    return __CreateEvBlock(ev);
}

LLBC_MessageBlock *LLBC_SvcEvUtil::BuildDataArrivalEv(LLBC_Packet *packet)
{
    typedef LLBC_SvcEv_DataArrival _Ev;

    _Ev *ev = LLBC_New(_Ev);
    ev->packet = packet;

    return __CreateEvBlock(ev);
}

LLBC_MessageBlock *LLBC_SvcEvUtil::BuildProtoReportEv(int sessionId,
                                                      int layer,
                                                      int level,
                                                      const LLBC_String &report)
{
    typedef LLBC_SvcEv_ProtoReport _Ev;

    _Ev *ev = LLBC_New(_Ev);
    ev->sessionId = sessionId;
    ev->layer = layer;
    ev->level = level;
    ev->report.append(report);

    return __CreateEvBlock(ev);
}

LLBC_MessageBlock *LLBC_SvcEvUtil::BuildSubscribeEvEv(int id,
                                                      const LLBC_String &stub,
                                                      LLBC_IDelegate1<LLBC_Event *> *deleg)
{
    typedef LLBC_SvcEv_SubscribeEv _Ev;

    _Ev *ev = LLBC_New(_Ev);
    ev->id = id;
    ev->stub.append(stub);
    ev->deleg = deleg;

    return __CreateEvBlock(ev);
}

LLBC_MessageBlock *LLBC_SvcEvUtil::BuildUnsubscribeEvEv(int id, const LLBC_String &stub)
{
    typedef LLBC_SvcEv_UnsubscribeEv _Ev;

    _Ev *ev = LLBC_New(_Ev);
    ev->id = id;
    ev->stub.append(stub);

    return __CreateEvBlock(ev);
}

LLBC_MessageBlock *LLBC_SvcEvUtil::BuildFireEvEv(LLBC_Event *ev)
{
    typedef LLBC_SvcEv_FireEv _Ev;

    _Ev *e = LLBC_New(_Ev);
    e->ev = ev;

    return __CreateEvBlock(e);
}

__LLBC_NS_END

#include "llbc/common/AfterIncl.h"
