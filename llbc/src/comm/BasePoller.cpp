/**
 * @file    BasePoller.cpp
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2014/11/13
 * @version 1.0
 *
 * @brief
 */

#include "llbc/common/Export.h"
#include "llbc/common/BeforeIncl.h"

#include "llbc/comm/Packet.h"
#include "llbc/comm/Socket.h"
#include "llbc/comm/Session.h"
#include "llbc/comm/ServiceEvent.h"
#include "llbc/comm/PollerType.h"
#include "llbc/comm/BasePoller.h"
#include "llbc/comm/SelectPoller.h"
#include "llbc/comm/IocpPoller.h"
#include "llbc/comm/EpollPoller.h"
#include "llbc/comm/PollerMgr.h"
#include "llbc/comm/IService.h"

namespace
{
    typedef LLBC_NS LLBC_BasePoller This;
}

__LLBC_NS_BEGIN

This::_Handler This::_handlers[LLBC_PollerEvent::End] =
{
    &This::HandleEv_AddSock,
    &This::HandleEv_AsyncConn,
    &This::HandleEv_Send,
    &This::HandleEv_Close,
    &This::HandleEv_Monitor,
    &This::HandleEv_TakeOverSession
};

LLBC_BasePoller::LLBC_BasePoller()
: _started(false)
, _stopping(false)

, _id(-1)
, _brotherCount(0)
, _svc(NULL)
, _pollerMgr(NULL)

, _sockets()
, _sessions()

, _connecting()
{
}

LLBC_BasePoller::~LLBC_BasePoller()
{
}

This *LLBC_BasePoller::Create(int type)
{
    This *poller = NULL;
    switch (type)
    {
    case LLBC_PollerType::SelectPoller:
        poller = LLBC_New(LLBC_SelectPoller);
        break;

#if LLBC_TARGET_PLATFORM_WIN32
    case LLBC_PollerType::IocpPoller:
        poller = LLBC_New(LLBC_IocpPoller);
        break;
#endif

#if LLBC_TARGET_PLATFORM_LINUX || LLBC_TARGET_PLATFORM_ANDROID
    case LLBC_PollerType::EpollPoller:
        poller = LLBC_New(LLBC_EpollPoller);
        break;
#endif

    default:
        break;
    }

    if (!poller)
        LLBC_SetLastError(LLBC_ERROR_INVALID);

    return poller;
}

void LLBC_BasePoller::SetPollerId(int id)
{
    _id = id;
}

void LLBC_BasePoller::SetBrothersCount(int count)
{
    _brotherCount = count;
}

void LLBC_BasePoller::SetService(LLBC_IService *svc)
{
    _svc = svc;
}

void LLBC_BasePoller::SetPollerMgr(LLBC_PollerMgr *mgr)
{
    _pollerMgr = mgr;
}

int LLBC_BasePoller::Start()
{
    ASSERT(false && "Please implement LLBC_BasePoller::Start() method!");
    LLBC_SetLastError(LLBC_ERROR_NOT_IMPL);
    return LLBC_RTN_FAILED;
}

void LLBC_BasePoller::Stop()
{
    if (!_started || _stopping)
        return;

    _stopping = true;
    while (_started)
        LLBC_ThreadManager::Sleep(20);

    _stopping = false;
}

void LLBC_BasePoller::Cleanup()
{
    // Notify poller manager I'm stop.
    _pollerMgr->OnPollerStop(_id);

    // Cleanup all queued events.
    LLBC_PollerEvent ev;
    LLBC_MessageBlock *block;
    while (this->TryPop(block) == LLBC_RTN_OK)
    {
        block->Read(&ev, sizeof(LLBC_PollerEvent));
        LLBC_PollerEvUtil::DestroyEv(ev);

        delete block;
    }

    // Delete all sessions.
#if LLBC_TARGET_PLATFORM_WIN32
    for (_Sessions::iterator it = _sessions.begin();
         it != _sessions.end();
         it++)
        it->second->GetSocket()->DeleteAllOverlappeds();
#endif // LLBC_TARGET_PLATFORM_WIN32
    LLBC_STLHelper::DeleteContainer(_sessions);
    _sockets.clear();

    // Delete all connecting sockets.
    for (_Connecting::iterator it = _connecting.begin();
         it != _connecting.end();
         it++)
        LLBC_Delete(it->second.socket);
    _connecting.clear();

    _started = false;
}

void LLBC_BasePoller::HandleQueuedEvents(int waitTime)
{
    LLBC_MessageBlock *block;
    while (this->TimedPop(block, waitTime) == LLBC_RTN_OK)
    {
        LLBC_PollerEvent &ev = 
            *reinterpret_cast< LLBC_PollerEvent *>(block->GetData());

        (this->*_handlers[ev.type])(ev);

        LLBC_Delete(block);
    }
}

void LLBC_BasePoller::HandleEv_AddSock(LLBC_PollerEvent &ev)
{
    this->AddSession(this->CreateSession(ev.un.socket, ev.sessionId));
}

void LLBC_BasePoller::HandleEv_AsyncConn(LLBC_PollerEvent &ev)
{
    ASSERT(false && "Please implement LLBC_Base_Poller::HandleEv_AsyncConn() method!");
}

void LLBC_BasePoller::HandleEv_Send(LLBC_PollerEvent &ev)
{
    _Sessions::iterator it = 
        _sessions.find(ev.un.packet->GetSessionId());
    if (it == _sessions.end())
    {
        LLBC_Delete(ev.un.packet);
        return;
    }

    LLBC_Session *session = it->second;
    if (UNLIKELY(session->IsListen()))
        LLBC_Delete(ev.un.packet);
    else if (UNLIKELY(session->Send(ev.un.packet) != LLBC_RTN_OK))
        session->OnClose();
}

void LLBC_BasePoller::HandleEv_Close(LLBC_PollerEvent &ev)
{
    _Sessions::iterator it = _sessions.find(ev.sessionId);
    if (it == _sessions.end())
        return;

    LLBC_Session *session = it->second;
    session->OnClose();
}

void LLBC_BasePoller::HandleEv_Monitor(LLBC_PollerEvent &ev)
{
    ASSERT(false && "Please implement LLBC_BasePoller::HandleEv_Monitor() method!");
}

void LLBC_BasePoller::HandleEv_TakeOverSession(LLBC_PollerEvent &ev)
{
    this->AddSession(ev.un.session);
}

LLBC_Session *LLBC_BasePoller::CreateSession(LLBC_Socket *socket, int sessionId)
{
    if (sessionId == 0)
    {
        sessionId = _pollerMgr->AllocSessionId();
    }

    LLBC_Session *session = new LLBC_Session();
    session->SetId(sessionId);
    session->SetSocket(socket);
    session->SetService(_svc);

    socket->SetSession(session);

    return session;
}

void LLBC_BasePoller::AddToPoller(LLBC_Session *session)
{
    const int hash = session->GetId() % _brotherCount;

    if (hash == _id)
    {
        this->AddSession(session);
    }
    else
    {
        LLBC_MessageBlock *ev = 
            LLBC_PollerEvUtil::BuildTakeOverSessionEv(session);
        if (_pollerMgr->PushMsgToPoller(hash, ev) != LLBC_RTN_OK)
        {
            trace("LLBC_BasePoller::AddToPoller() could not found poller, hash val: %d\n", hash);
            LLBC_PollerEvUtil::DestroyEv(ev);
            return;
        }
    }
}

#if LLBC_TARGET_PLATFORM_NON_WIN32
void LLBC_BasePoller::AddSession(LLBC_Session *session)
#else
void LLBC_BasePoller::AddSession(LLBC_Session *session, bool needAddToIocp)
#endif // LLBC_TARGET_PLATFORM_NON_WIN32
{
    // Insert to socket & session map.
    session->SetPoller(this);
    _sessions.insert(std::make_pair(session->GetId(), session));
    _sockets.insert(std::make_pair(session->GetSocketHandle(), session));

    // Build event and push to service.
    LLBC_Socket *sock = session->GetSocket();
    LLBC_MessageBlock *block = 
        LLBC_SvcEvUtil::BuildSessionCreateEv(sock->GetLocalAddress(),
                                             sock->GetPeerAddress(),
                                             sock->IsListen(),
                                             session->GetId(),
                                             sock->Handle());

    _svc->Push(block);
}

void LLBC_BasePoller::RemoveSession(LLBC_Session *session)
{
    _sessions.erase(session->GetId());
    _sockets.erase(session->GetSocketHandle());

    LLBC_Delete(session);
}

void LLBC_BasePoller::SetConnectedSocketDftOpts(LLBC_Socket *sock)
{
    sock->UpdateLocalAddress();
    sock->UpdatePeerAddress();

    if (LLBC_CFG_COMM_DFT_SEND_BUF_SIZE > 0)
        sock->SetSendBufSize(LLBC_CFG_COMM_DFT_SEND_BUF_SIZE);
    if (LLBC_CFG_COMM_DFT_RECV_BUF_SIZE > 0)
        sock->SetRecvBufSize(LLBC_CFG_COMM_DFT_RECV_BUF_SIZE);
}

__LLBC_NS_END

#include "llbc/common/AfterIncl.h"
