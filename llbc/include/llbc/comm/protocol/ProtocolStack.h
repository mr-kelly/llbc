/**
 * @file    ProtocolStack.h
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2014/11/12
 * @version 1.0
 *
 * @brief
 */
#ifndef __LLBC_COMM_PROTOCOL_STACK_H__
#define __LLBC_COMM_PROTOCOL_STACK_H__

#include "llbc/common/Common.h"
#include "llbc/core/Core.h"
#include "llbc/objbase/ObjBase.h"

#include "llbc/comm/protocol/ProtocolLayer.h"

__LLBC_NS_BEGIN

/**
 * Previous declare some classes.
 */
class LLBC_Packet;
class LLBC_IProtocol;
class LLBC_IProtocolFilter;
class LLBC_ICoderFactory;
class LLBC_Session;
class LLBC_IService;

__LLBC_NS_END

__LLBC_NS_BEGIN

/**
 * \brief The protocol layer class encapsulation.
 */
class LLBC_EXPORT LLBC_ProtocolStack
{
public:
    /**
     * The stack type enumeration.
     */
    enum StackType
    {
        RawStack,
        CodecStack,

        FullStack
    };

public:
    /**
     * Create protocol stack by given type.
     * @param[in] type - the stack type.
     */
    LLBC_ProtocolStack(StackType type);
    /**
     * Destructor.
     */
    ~LLBC_ProtocolStack();

public:
    /**
     * Set service to protocol stack, use to report something to service.
     * @param[in] svc - the service.
     */
    void SetService(LLBC_IService *svc);

    /**
     * Set session to protocol stack, use to report something to service.
     * @param[in] session - the session.
     */
    void SetSession(LLBC_Session *session);

public:
    /**
     * Set protocol to protocol stack.
     * @param[in] proto - the protocol.
     * @return int - return 0 if success, otherwise reutrn -1.
     */
    int AddProtocol(LLBC_IProtocol *proto);

    /**
     * Add coder to protocol stack.
     * @param[in] opcode - the opcode.
     * @param[in] coder  - the coder factory.
     *                    Warn: The protocol factory will not delete coder object,
     *                          Just simple hold it pointer.
     * @return int - return 0 if success, otherwise return -1.
     */
    int AddCoder(int opcode, LLBC_ICoderFactory *coder);

    /**
     * Set protocol filter to specified layer protocol.
     * @param[in] filter  - the protocol filter.
     * @param[in] toProto - protocol layer.
     * @return int - return 0 if success, otherwise return -1.
     */
    int SetFilter(LLBC_IProtocolFilter *filter, int toProto);

public:
    int Connect(LLBC_SockAddr_IN &local, LLBC_SockAddr_IN &peer);

    /**
     * When packet send, will use this protocol stack method to filter and encode packet.
     * @param[in] willEncode - the will send packet.
     * @param[in] encoded    - the filtered and encoded packet. 
     * @return int - return 0 if success, otherwise return -1.
     */
    int SendCodec(LLBC_Packet *willEncode, LLBC_Packet *&encoded);
    
    /**
     * When packet send, will use this protocol stack method to compress packet and convert to message-block to send.
     * @param[in] packet - the will compress and convert packet.
     * @param[in] block  - the message block.
     * @return int - return 0 if success, otherwise return -1.
     */
    int SendRaw(LLBC_Packet *packet, LLBC_MessageBlock *&block);

    /**
     * When packet send, will use this protocol stack method to convert packet to message-block type to send.
     * @param[in] packet - the packet.
     * @param[in] block  - the message block.
     * @return int - return 0 if success, otherwise return -1.
     */
    int Send(LLBC_Packet *packet, LLBC_MessageBlock *&block);

    /**
     * When message receive, will use this protocol stack method to convert message-block to undecoded.
     * @param[in] block   - the message block.
     * @param[in] packets - the packets.
     * @return int - return 0 if success, otherwise return -1.
     */
    int RecvRaw(LLBC_MessageBlock *block, std::vector<LLBC_Packet *> &packets);

    /**
     * When packet receive, will use this protocol stack method to decode and filter.
     * @param[in] willDecode  - the will decode and filter packet.
     * @param[in] decoded     - the decoded and filtered packet.
     * @return int - return 0 if success, otherwise return -1.
     */
    int RecvCodec(LLBC_Packet *willDecode, LLBC_Packet *&decoded);

    /**
     * When packet recv, will use this protocol stack method to convert message-block type to packets.
     * @param[in] block   - the message block.
     * @param[in] packets - the converted packet list.
     */
    int Recv(LLBC_MessageBlock *block, std::vector<LLBC_Packet *> &packets);

private:
    /**
     * Declare friend class: LLBC_IProtocol.
     * Access method list:
     *      Report(LLBC_IProtocol *, const LLBC_String &).
     */
    friend class LLBC_RawProtocol;
    friend class LLBC_PacketProtocol;
    friend class LLBC_CodecProtocol;
    friend class LLBC_CompressProtocol;
    friend class LLBC_FilterProtocol;

    /**
     * Report error, call by protocol.
     * @param[in] sessionId - the session Id.
     * @param[in] proto     - the reporter.
     * @param[in] level     - the report level.
     * @param[in] err       - the error describe.
     */
    void Report(LLBC_IProtocol *proto, int level, const LLBC_String &err);
    void Report(int sessionId, LLBC_IProtocol *proto, int level, const LLBC_String &err);

private:
    StackType _type;
    bool _builded;

    LLBC_IService *_svc;
    LLBC_Session *_session;

    LLBC_IProtocol *_protos[LLBC_ProtocolLayer::End];
};

__LLBC_NS_END

#endif // !__LLBC_COMM_PROTOCOL_STACK_H__

