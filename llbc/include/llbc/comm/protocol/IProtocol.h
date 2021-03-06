/**
 * @file    IProtocol.h
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2014/11/12
 * @version 1.0
 *
 * @brief
 */
#ifndef __LLBC_COMM_IPROTOCOL_H__
#define __LLBC_COMM_IPROTOCOL_H__

#include "llbc/common/Common.h"
#include "llbc/core/Core.h"
#include "llbc/objbase/ObjBase.h"

#include "llbc/comm/protocol/ProtocolLayer.h"

__LLBC_NS_BEGIN

/**
 * Previous declare some classes.
 */
class LLBC_ICoderFactory;
class LLBC_ProtocolStack;
class LLBC_IProtocolFilter;

__LLBC_NS_END

__LLBC_NS_BEGIN

/**
 * \brief The protocol interface class encapsulation.
 */
class LLBC_EXPORT LLBC_IProtocol
{
    typedef LLBC_IProtocol This;

public:
    LLBC_IProtocol();
    virtual ~LLBC_IProtocol();

public:
    /**
     * Protocol create method.
     * @param[in] layer - the protocol layer.
     * @return This * - protocol.
     */
    template <typename Proto>
    static This *Create(LLBC_IProtocolFilter *filter = NULL);

    /**
     * Get the protocol layer.
     * @return int - the protocol layer.
     */
    virtual int GetLayer() const = 0;

public:
    /**
     * When one connection established, will call this method.
     * @param[in] local - the local address.
     * @param[in] peer  - the peer address.
     * @return int - return 0 if success, otherwise return -1.
     */
    virtual int Connect(LLBC_SockAddr_IN &local, LLBC_SockAddr_IN &peer) = 0;

    /**
     * When data send, will call this method.
     * @param[in] in   - the in data.
     * @param[out] out - the out data.
     * @return int - return 0 if success, otherwise return -1.
     */
    virtual int Send(void *in, void *&out) = 0;

    /**
     * When data received, will call this method.
     * @param[in] in   - the in data.
     * @param[out] out - the out data.
     * @return int - return 0 if success, otherwise return -1.
     */
    virtual int Recv(void *in, void *&out) = 0;

    /**
     * Add coder factory to protocol, only available in Codec-Layer.
     * @param[in] opcode - the opcode.
     * @param[in] coder  - the coder factory
     *  Note: When protocol deleted, it will not delete coder pointer,
     *        It means that you must self manage your coder memory.
     * @return int - return 0 if success, otherwise return -1.
     */
    virtual int AddCoder(int opcode, LLBC_ICoderFactory *coder) = 0;

public:
    /**
     * Set protocol filter to protocol.
     * @param[in] filter - the protocol filter.
     */
    void SetFilter(LLBC_IProtocolFilter *filter);

private:
    /**
     * Friend class: LLBC_ProtocolStack.
     *  Access methods:
     *      void SetStack()
     */
    friend class LLBC_ProtocolStack;

    /**
     * Set protocol stack to protocol.
     * @param[in] stack - the protocol stack.
     */
    void SetStack(LLBC_ProtocolStack *stack);

protected:
    LLBC_ProtocolStack* _stack;
    LLBC_IProtocolFilter *_filter;
};

__LLBC_NS_END

#include "llbc/comm/protocol/IProtocolImpl.h"

#endif // !__LLBC_COMM_IPROTOCOL_H__
