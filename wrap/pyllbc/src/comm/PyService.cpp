/**
 * @file    PyService.cpp
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2014/08/20
 * @version 1.0
 *
 * @brief
 */

#include "pyllbc/common/Export.h"

#include "pyllbc/comm/PyObjCoder.h"
#include "pyllbc/comm/PyPacketHandler.h"
#include "pyllbc/comm/PyFacade.h"
#include "pyllbc/comm/ErrorHooker.h"
#include "pyllbc/comm/PyService.h"

namespace
{
    typedef pyllbc_Service This;

    void ResetMainloopFlag(void *flagPtr)
    {
        bool *flag = reinterpret_cast<bool *>(flagPtr);
        *flag = false;
    }
}

int pyllbc_Service::_maxLLBCSvcId = 0;
PyObject *pyllbc_Service::_streamCls = NULL;
pyllbc_ErrorHooker *pyllbc_Service::_errHooker = LLBC_New(pyllbc_ErrorHooker);

pyllbc_Service::pyllbc_Service(LLBC_IService::Type type, PyObject *pySvc)
: _llbcSvc(NULL)
, _llbcSvcType(type)

, _pySvc(pySvc)

, _inMainloop()

, _cppFacade(NULL)
, _facades()

, _handlers()
, _preHandlers()
#if LLBC_CFG_COMM_ENABLE_UNIFY_PRESUBSCRIBE
, _unifyPreHandler(NULL)
#endif // LLBC_CFG_COMM_ENABLE_UNIFY_PRESUBSCRIBE

, _codec((This::Codec)(PYLLBC_CFG_DFT_SVC_CODEC))
, _codecs()

, _beforeFrameCallables()
, _afterFrameCallables()

, _handlingBeforeFrameCallables(false)
, _handledBeforeFrameCallables(false)
, _handlingAfterFrameCallables(false)

, _started(false)
, _stoping(false)
{
    // Create llbc library Service object and set some service attributes.
    this->CreateLLBCService(type);

    // Create cobj python attribute key.
    _keyCObj = Py_BuildValue("s", "cobj");

    if (!This::_streamCls)
        This::_streamCls = pyllbc_s_TopModule->GetObject("Stream"); // Borrowed
}

pyllbc_Service::~pyllbc_Service()
{
    this->Stop();
    this->AfterStop();

    Py_DECREF(_keyCObj);
}

LLBC_IService::Type pyllbc_Service::GetType() const
{
    return _llbcSvcType;
}

PyObject *pyllbc_Service::GetPyService() const
{
    return _pySvc;
}

int pyllbc_Service::GetFPS() const
{
    return _llbcSvc->GetFPS();
}

int pyllbc_Service::SetFPS(int fps)
{
    if (_llbcSvc->SetFPS(fps) != LLBC_RTN_OK)
    {
        pyllbc_TransferLLBCError(__FILE__, __LINE__);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Service::GetFrameInterval() const
{
    return _llbcSvc->GetFrameInterval();
}

This::Codec pyllbc_Service::GetCodec() const
{
    return _codec;
}

int pyllbc_Service::SetCodec(Codec codec)
{
    if (codec != This::JsonCodec &&
        codec != This::BinaryCodec)
    {
        pyllbc_SetError("invalid codec type", LLBC_ERROR_INVALID);
        return LLBC_RTN_FAILED;
    }
    else if (_started)
    {
        pyllbc_SetError("service already start, could not change codec strategy");
        return LLBC_RTN_FAILED;
    }

    if (codec != _codec)
        _codec = codec;

    return LLBC_RTN_OK;
}

int pyllbc_Service::Start(int pollerCount)
{
    if (_started)
    {
        pyllbc_SetError("service already started", LLBC_ERROR_REENTRY);
        return LLBC_RTN_FAILED;
    }

    _started = true;

    if (_llbcSvc->Start(pollerCount) != LLBC_RTN_OK)
    {
        _started = false;
        pyllbc_TransferLLBCError(__FILE__, __LINE__);

        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

bool pyllbc_Service::IsStarted() const
{
    return _started;
}

void pyllbc_Service::Stop()
{
    if (!this->IsStarted())
        return;

    if (_inMainloop)
    {
        _stoping = true;
    }
    else
    {
        this->AfterStop();
        _started = false;
    }
}

int pyllbc_Service::RegisterFacade(PyObject *facade)
{
    if (!_facades.insert(facade).second)
    {
        PyObject *pyFacadeStr = PyObject_Str(facade);
        LLBC_String facadeStr = PyString_AsString(pyFacadeStr);
        Py_DECREF(pyFacadeStr);

        LLBC_String errStr;
        pyllbc_SetError(errStr.format("repeat to register facade: %s", facadeStr.c_str()), LLBC_ERROR_REPEAT);

        return LLBC_RTN_FAILED;
    }

    Py_INCREF(facade);

    return LLBC_RTN_OK;
}

int pyllbc_Service::RegisterCodec(int opcode, PyObject *codec)
{
    if (_codec != This::BinaryCodec)
    {
        pyllbc_SetError("current codec strategy not BINARY, don't need register codec");
        return LLBC_RTN_FAILED;
    }
    else if (_llbcSvcType == LLBC_IService::Raw)
    {
        pyllbc_SetError("RAW type service don't need register codec");
        return LLBC_RTN_FAILED;
    }
    else if (!PyCallable_Check(codec))
    {
        pyllbc_SetError("codec not callable");
        return LLBC_RTN_FAILED;
    }

    if (!_codecs.insert(std::make_pair(opcode, codec)).second)
    {
        LLBC_String err;
        pyllbc_SetError(err.append_format(
            "repeat to register specify opcode's codec, opcode: %d", opcode));

        return LLBC_RTN_FAILED;
    }

    Py_INCREF(codec);

    return LLBC_RTN_OK;
}

int pyllbc_Service::Listen(const char *ip, uint16 port)
{
    int sessionId;
    if ((sessionId = _llbcSvc->Listen(ip, port)) == 0)
    {
        pyllbc_TransferLLBCError(__FILE__, __LINE__);
        return 0;
    }

    return sessionId;
}

int pyllbc_Service::Connect(const char *ip, uint16 port)
{
    int sessionId;
    if ((sessionId = _llbcSvc->Connect(ip, port)) == 0)
    {
        pyllbc_TransferLLBCError(__FILE__, __LINE__);
        return 0;
    }

    return sessionId;
}

int pyllbc_Service::AsyncConn(const char *ip, uint16 port)
{
    if (_llbcSvc->AsyncConn(ip, port) != LLBC_RTN_OK)
    {
        pyllbc_TransferLLBCError(__FILE__, __LINE__);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

void pyllbc_Service::RemoveSession(int sessionId)
{
    _llbcSvc->RemoveSession(sessionId);
}

int pyllbc_Service::Send(int sessionId, int opcode, PyObject *data, int status, PyObject *parts)
{
    // Started check.
    if (UNLIKELY(!this->IsStarted()))
    {
        pyllbc_SetError("service not start");
        return LLBC_RTN_FAILED;
    }

    // Build parts, if exists.
    LLBC_PacketHeaderParts *cLayerParts = NULL;
    if (parts && _llbcSvcType != LLBC_IService::Raw)
    {
        if (!(cLayerParts = this->BuildCLayerParts(parts)))
            return LLBC_RTN_FAILED;
    }

    // Serialize python layer 'data' object to stream.
    LLBC_Stream stream;
    const int ret = this->SerializePyObj2Stream(data, stream);
    if (UNLIKELY(ret != LLBC_RTN_OK))
    {
        LLBC_XDelete(cLayerParts);
        return LLBC_RTN_FAILED;
    }

    // Build packet & send.
    LLBC_Packet *packet = LLBC_New(LLBC_Packet);
    packet->Write(stream.GetBuf(), stream.GetPos());

    packet->SetSessionId(sessionId);
    if (_llbcSvcType != LLBC_IService::Raw)
    {
        packet->SetOpcode(opcode);
        packet->SetStatus(status);

        if (cLayerParts)
        {
            cLayerParts->SetToPacket(*packet);
            LLBC_Delete(cLayerParts);
        }
    }

    if (UNLIKELY(_llbcSvc->Send(packet) == LLBC_RTN_FAILED))
    {
        pyllbc_TransferLLBCError(__FILE__, __LINE__);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Service::Multicast(const LLBC_SessionIdList &sessionIds, int opcode, PyObject *data, int status, PyObject *parts)
{
    // Started check.
    if (UNLIKELY(!this->IsStarted()))
    {
        pyllbc_SetError("service not start");
        return LLBC_RTN_FAILED;
    }

    // Session Ids array is empty, done.
    if (sessionIds.empty())
        return LLBC_RTN_OK;

    // Build parts, if exists.
    LLBC_PacketHeaderParts *cLayerParts = NULL;
    if (parts && _llbcSvcType != LLBC_IService::Raw)
    {
        if (!(cLayerParts = this->BuildCLayerParts(parts)))
            return LLBC_RTN_FAILED;
    }

    // Serialize python layer 'data' object to stream.
    LLBC_Stream stream;
    if (this->SerializePyObj2Stream(data, stream) != LLBC_RTN_OK)
    {
        LLBC_XDelete(cLayerParts);
        return LLBC_RTN_FAILED;
    }

    // Send it.
    const void *bytes = stream.GetBuf();
    const size_t len = stream.GetPos();
    return _llbcSvc->Multicast2(sessionIds, opcode, bytes, len, status, cLayerParts);
}

int pyllbc_Service::Broadcast(int opcode, PyObject *data, int status, PyObject *parts)
{
    // Started check.
    if (UNLIKELY(!this->IsStarted()))
    {
        pyllbc_SetError("service not start");
        return LLBC_RTN_FAILED;
    }

    // Build parts, if exists.
    LLBC_PacketHeaderParts *cLayerParts = NULL;
    if (parts && _llbcSvcType != LLBC_IService::Raw)
    {
        if (!(cLayerParts = this->BuildCLayerParts(parts)))
            return LLBC_RTN_FAILED;
    }

    // Serialize python layer 'data' object to stream.
    LLBC_Stream stream;
    if (this->SerializePyObj2Stream(data, stream) != LLBC_RTN_OK)
    {
        LLBC_XDelete(cLayerParts);
        return LLBC_RTN_FAILED;
    }

    // Send it.
    const void *bytes = stream.GetBuf();
    const size_t len = stream.GetPos();
    return _llbcSvc->Broadcast2(opcode, bytes, len, status, cLayerParts);
}

int pyllbc_Service::Subscribe(int opcode, PyObject *handler, int flags)
{
    if (_started)
    {
        pyllbc_SetError("service already started", LLBC_ERROR_INITED);
        return LLBC_RTN_FAILED;
    }
    else if (_llbcSvcType == LLBC_IService::Raw && opcode != 0)
    {
        pyllbc_SetError(LLBC_String().format(
            "RAW type service could not subscribe opcode[%d] != 0's packet", opcode), LLBC_ERROR_INVALID);
        return LLBC_RTN_FAILED;
    }

    _PacketHandlers::const_iterator it = _handlers.find(opcode);
    if (it != _handlers.end())
    {
        const LLBC_String handlerDesc = pyllbc_ObjUtil::GetObjStr(handler);

        LLBC_String err;
        err.append_format("repeat to subscribeopcode: %d:%s, ", opcode, handlerDesc.c_str());
        err.append_format("the opcode already subscribed by ");
        err.append_format("%s", it->second->ToString().c_str());

        pyllbc_SetError(err, LLBC_ERROR_REPEAT);

        return LLBC_RTN_FAILED;
    }

    pyllbc_PacketHandler *wrapHandler = LLBC_New1(pyllbc_PacketHandler, opcode);
    if (wrapHandler->SetHandler(handler) != LLBC_RTN_OK)
    {
        LLBC_Delete(wrapHandler);
        return LLBC_RTN_FAILED;
    }

    _handlers.insert(std::make_pair(opcode, wrapHandler));
    _llbcSvc->Subscribe(opcode, _cppFacade, &pyllbc_Facade::OnDataReceived);

    return LLBC_RTN_OK;
}

int pyllbc_Service::PreSubscribe(int opcode, PyObject *preHandler, int flags)
{
    if (_started)
    {
        pyllbc_SetError("service already started", LLBC_ERROR_INITED);
        return LLBC_RTN_FAILED;
    }
    else if (_llbcSvcType == LLBC_IService::Raw && opcode != 0)
    {
        pyllbc_SetError("RAW type service could not pre-subscribe opcode != 0's packet", LLBC_ERROR_INVALID);
        return LLBC_RTN_FAILED;
    }

    pyllbc_PacketHandler *wrapHandler = LLBC_New1(pyllbc_PacketHandler, opcode);
    if (wrapHandler->SetHandler(preHandler) != LLBC_RTN_OK)
    {
        LLBC_Delete(wrapHandler);
        return LLBC_RTN_FAILED;
    }

    if (!_preHandlers.insert(std::make_pair(opcode, wrapHandler)).second)
    {
        LLBC_Delete(wrapHandler);

        LLBC_String err;
        pyllbc_SetError(err.format(
            "repeat to pre-subscribe opcode: %d, the opcode already pre-subscribed", opcode), LLBC_ERROR_REPEAT);

        return LLBC_RTN_FAILED;
    }

    _llbcSvc->PreSubscribe(opcode, _cppFacade, &pyllbc_Facade::OnDataPreReceived);

    return LLBC_RTN_OK;
}

#if LLBC_CFG_COMM_ENABLE_UNIFY_PRESUBSCRIBE
int pyllbc_Service::UnifyPreSubscribe(PyObject *preHandler, int flags)
{
    if (_started)
    {
        pyllbc_SetError("service already started", LLBC_ERROR_INITED);
        return LLBC_RTN_FAILED;
    }

    pyllbc_PacketHandler *wrapHandler = LLBC_New1(pyllbc_PacketHandler, 0);
    if (wrapHandler->SetHandler(preHandler) != LLBC_RTN_OK)
    {
        LLBC_Delete(wrapHandler);
        return LLBC_RTN_FAILED;
    }

    if (_unifyPreHandler)
    {
        pyllbc_SetError("repeat to unify pre-subscribe packet");
        return LLBC_RTN_FAILED;
    }

    _unifyPreHandler = wrapHandler;
    _llbcSvc->UnifyPreSubscribe(_cppFacade, &pyllbc_Facade::OnDataUnifyPreReceived);

    return LLBC_RTN_OK;
}
#endif // LLBC_CFG_COMM_ENABLE_UNIFY_PRESUBSCRIBE

int pyllbc_Service::Post(PyObject *callable)
{
    if (!PyCallable_Check(callable))
    {
        const LLBC_String objDesc = pyllbc_ObjUtil::GetObjStr(callable);
        pyllbc_SetError(LLBC_String().format("frame callable object not callable: %s", objDesc.c_str()));

        return LLBC_RTN_FAILED;
    }

    if (_handlingBeforeFrameCallables &&
        _handlingAfterFrameCallables)
    {
        pyllbc_SetError("could not push callable object to service, internal error!");
        return LLBC_RTN_FAILED;
    }

    if (_beforeFrameCallables.find(callable) != _beforeFrameCallables.end() ||
        _afterFrameCallables.find(callable) != _afterFrameCallables.end())
    {
        const LLBC_String objDesc = pyllbc_ObjUtil::GetObjStr(callable);
        pyllbc_SetError(LLBC_String().format(
            "repeat to add callable to service, callable: %s", objDesc.c_str()));

        return LLBC_RTN_FAILED;
    }

    Py_INCREF(callable);
    if (_handlingBeforeFrameCallables)
    {
        _afterFrameCallables.insert(callable);
    }
    else
    {
        if (!_handledBeforeFrameCallables)
        {
            _beforeFrameCallables.insert(callable);
        }
        else
        {
            if (!_handlingAfterFrameCallables)
                _afterFrameCallables.insert(callable);
            else
                _beforeFrameCallables.insert(callable);
        }
    }

    return LLBC_RTN_OK;
}

bool pyllbc_Service::MainLoop()
{
    if (UNLIKELY(!_started))
        return false;

    _inMainloop = true;
    LLBC_InvokeGuard guard(&ResetMainloopFlag, &_inMainloop);

    if (!_stoping)
    {
        this->HandleFrameCallables(_beforeFrameCallables, _handlingBeforeFrameCallables);
        _handledBeforeFrameCallables = true;
    }

    if (!_stoping)
        _llbcSvc->OnSvc(false);

    if (!_stoping)
    {
        this->HandleFrameCallables(_afterFrameCallables, _handlingAfterFrameCallables);
    }

    _handledBeforeFrameCallables = false;

    if (_stoping)
    {
        this->AfterStop();

        _stoping = false;
        _started = false;

        return false;
    }

    // Transfer hooked error to python.
    This::_errHooker->TransferHookedErrorToPython();

    return true;
}

pyllbc_ErrorHooker *pyllbc_Service::GetErrHooker()
{
    return This::_errHooker;
}

void pyllbc_Service::CreateLLBCService(LLBC_IService::Type svcType)
{
    ASSERT(!_llbcSvc && "llbc service pointer not NULL");

    _llbcSvc = LLBC_IService::Create(svcType);
    _llbcSvc->SetId(++This::_maxLLBCSvcId); // llbc library ServiceId we not use, so, let's simple set it.
    _llbcSvc->SetDriveMode(LLBC_IService::ExternalDrive);
    _llbcSvc->DisableTimerScheduler();

    _cppFacade = LLBC_New1(pyllbc_Facade, this);
    _llbcSvc->RegisterFacade(_cppFacade);
}

void pyllbc_Service::AfterStop()
{
    _cppFacade = NULL;

    // Recreate service.
    LLBC_XDelete(_llbcSvc);
    this->CreateLLBCService(_llbcSvcType);

    // Cleanup all python layer facades.
    for (_Facades::iterator it = _facades.begin();
         it != _facades.end();
         it++)
        Py_DECREF(*it);
    _facades.clear();

    // Cleanup all python layer codecs.
    for (_Codecs::iterator it = _codecs.begin();
         it != _codecs.end();
         it++)
        Py_DECREF(it->second);
    _codecs.clear();

    // Cleanup all python layer handlers/prehandlers/unify_prehandlers(if enabled).
    LLBC_STLHelper::DeleteContainer(_handlers);
    LLBC_STLHelper::DeleteContainer(_preHandlers);
#if LLBC_CFG_COMM_ENABLE_UNIFY_PRESUBSCRIBE
    LLBC_XDelete(_unifyPreHandler);
#endif // LLBC_CFG_COMM_ENABLE_UNIFY_PRESUBSCRIBE

    // Destroy all frame callables.
    _handledBeforeFrameCallables = false;
    this->DestroyFrameCallables(_beforeFrameCallables, _handlingBeforeFrameCallables);
    this->DestroyFrameCallables(_afterFrameCallables, _handlingAfterFrameCallables);

}

void pyllbc_Service::HandleFrameCallables(pyllbc_Service::_FrameCallables &callables, bool &usingFlag)
{
    usingFlag = true;
    for (_FrameCallables::iterator it = callables.begin();
         it != callables.end();
         it++)
    {
        PyObject *callable = *it;
        PyObject *ret = PyObject_CallFunctionObjArgs(callable, _pySvc, NULL);
        if (ret)
        {
            Py_DECREF(ret);
        }
        else
        {
            const LLBC_String objDesc = pyllbc_ObjUtil::GetObjStr(callable);
            pyllbc_TransferPyError(LLBC_String().format("When call frame-callable: %s", objDesc.c_str()));
            break;
        }
    }

    this->DestroyFrameCallables(callables, usingFlag);
}

void pyllbc_Service::DestroyFrameCallables(_FrameCallables &callables, bool &usingFlag)
{
    for (_FrameCallables::iterator it = callables.begin();
         it != callables.end();
         it++)
        Py_DECREF(*it);

    callables.clear();
    usingFlag = false;
}

LLBC_PacketHeaderParts *pyllbc_Service::BuildCLayerParts(PyObject *pyLayerParts)
{
    // Python layer parts(dict type) convert rules describe:
    //   python type       c++ type
    // --------------------------
    //   int/long/bool -->   sint64
    //     float4/8    -->  float/double
    //   str/bytearray -->  LLBC_String

    if (!PyDict_Check(pyLayerParts))
    {
        pyllbc_SetError("parts instance not dict type");
        return NULL;
    }

    LLBC_PacketHeaderParts *cLayerParts = LLBC_New(LLBC_PacketHeaderParts);

    Py_ssize_t pos = 0;
    PyObject *key, *value;
    while (PyDict_Next(pyLayerParts, &pos, &key, &value)) // key & value are borrowed.
    {
        const int serialNo = static_cast<int>(PyInt_AsLong(key));
        if (UNLIKELY(serialNo == -1 && PyErr_Occurred()))
        {
            pyllbc_TransferPyError("When fetch header part serial no");
            LLBC_Delete(cLayerParts);

            return NULL;
        }

        // Value type check order:
        //   int->
        //     str->
        //       float->
        //         long->
        //           bool->
        //             bytearray->
        //               other objects
        if (PyInt_CheckExact(value))
        {
            const sint64 cValue = PyInt_AS_LONG(value);
            cLayerParts->SetPart<sint64>(serialNo, cValue);
        }
        else if (PyString_CheckExact(value))
        {
            char *strBeg;
            Py_ssize_t strLen;
            if (UNLIKELY(PyString_AsStringAndSize(value, &strBeg, &strLen) == -1))
            {
                pyllbc_TransferPyError("When fetch header part value");
                LLBC_Delete(cLayerParts);

                return NULL;
            }

            cLayerParts->SetPart(serialNo, strBeg, strLen);

        }
        else if (PyFloat_CheckExact(value))
        {
            const double cValue = PyFloat_AS_DOUBLE(value);
            cLayerParts->SetPart<double>(serialNo, cValue);
        }
        else if (PyLong_CheckExact(value))
        {
            const sint64 cValue = PyLong_AsLongLong(value);
            cLayerParts->SetPart<sint64>(serialNo, cValue);
        }
        else if (PyBool_Check(value))
        {
            const int pyBoolCheck = PyObject_IsTrue(value);
            if (UNLIKELY(pyBoolCheck == -1))
            {
                pyllbc_TransferPyError("when fetch header part value");
                LLBC_Delete(cLayerParts);

                return NULL;
            }

            cLayerParts->SetPart<uint8>(serialNo, pyBoolCheck);
        }
        else if (PyByteArray_CheckExact(value))
        {
            char *bytesBeg = PyByteArray_AS_STRING(value);
            Py_ssize_t bytesLen = PyByteArray_GET_SIZE(value);

            cLayerParts->SetPart(serialNo, bytesBeg, bytesLen);
        }
        else // Other types, we simple get the object string representations.
        {
            LLBC_String strRepr = pyllbc_ObjUtil::GetObjStr(value);
            if (UNLIKELY(strRepr.empty() && PyErr_Occurred()))
            {
                LLBC_Delete(cLayerParts);
                return NULL;
            }

            cLayerParts->SetPart(serialNo, strRepr.data(), strRepr.size());
        }
    }

    return cLayerParts;
}

int pyllbc_Service::SerializePyObj2Stream(PyObject *pyObj, LLBC_Stream &stream)
{
    if (_codec == This::JsonCodec)
    {
        std::string out;
        if (UNLIKELY(pyllbc_ObjCoder::Encode(pyObj, out) != LLBC_RTN_OK))
            return LLBC_RTN_FAILED;

        stream.WriteBuffer(out.data(), out.size());

        return LLBC_RTN_OK;
    }
    else
    {
        // Create python layer Stream instance.
        PyObject *arg = PyTuple_New(2);
        PyTuple_SetItem(arg, 0, PyInt_FromLong(0)); // stream init size = 0.
        
        Py_INCREF(pyObj);
        PyTuple_SetItem(arg, 1, pyObj); // initWrite = pyObj(steal reference).

        PyObject *pyStreamObj = PyObject_CallObject(This::_streamCls, arg);
        if (UNLIKELY(!pyStreamObj))
        {
            Py_DECREF(arg);
            pyllbc_TransferPyError();

            return LLBC_RTN_FAILED;
        }

        // Get cobj property.
        PyObject *cobj = PyObject_GetAttr(pyStreamObj, _keyCObj);
        if (UNLIKELY(!cobj))
        {
            Py_DECREF(pyStreamObj);
            Py_DECREF(arg);

            pyllbc_SetError("could not get llbc.Stream property 'cobj'");

            return LLBC_RTN_FAILED;
        }

        // Convert to pyllbc_Stream *.
        pyllbc_Stream *cstream = NULL;
        PyArg_Parse(cobj, "l", &cstream);

        // Let stream attach to inlStream.
        LLBC_Stream &inlStream = cstream->GetLLBCStream();

        stream.Attach(inlStream);
        (void)inlStream.Detach();
        stream.SetAttachAttr(false);

        Py_DECREF(cobj);
        Py_DECREF(pyStreamObj);
        Py_DECREF(arg);

        return LLBC_RTN_OK;
    }
}
