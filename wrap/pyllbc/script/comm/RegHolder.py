# -*- coding: utf-8 -*-

from weakref import ref
from types import TypeType, ClassType
import functools

import llbc

class pyllbcSvcRegInfo(object):
    """
    Service auto register information holder.
    """
    # info type enumeration
    BeginType = 0
    Coder = BeginType + 0 # coder type
    Handler = BeginType + 1 # handler type
    PreHandler = BeginType + 2 # prehandler type
    UnifyPreHandler = BeginType + 3 # unify prehandler type

    ExcHandler = BeginType + 4 # packet-handler's exception handler
    ExcPreHandler = BeginType + 5 # packet-prehandler's exception handler
    DftExcHandler = BeginType + 6 # default packet-handler's exception handler
    DftExcPreHandler = BeginType + 7 # default packet-prehandler's exception handler

    FrameExcHandler = BeginType + 8 # frame exception handler

    UnSpecific = BeginType + 9 # unspecific

    def __init__(self, wrapped):
        self._wrapped = ref(wrapped)

        self._type = self.UnSpecific
        self._fmt = ''
        self._enopcode = None
        self._deopcodes = []

        self._hldropcodes = []
        self._prehldropcodes = []
        self._exc_hldropcodes = []
        self._exc_prehldropcodes = []

        self._svcs = set()

        self._asfacade = False

    @property
    def wrapped(self):
        return self._wrapped()

    @property
    def regtype(self):
        return self._type
    @regtype.setter
    def regtype(self, value):
        self._type = value

    @property
    def fmt(self):
        return self._fmt
    @fmt.setter
    def fmt(self, value):
        self._fmt = value

    @property
    def enopcode(self):
        return self._enopcode
    @enopcode.setter
    def enopcode(self, value):
        self._enopcode = value

    @property
    def deopcodes(self):
        return self._deopcodes

    @property
    def hldropcodes(self):
        return self._hldropcodes

    @property
    def prehldropcodes(self):
        return self._prehldropcodes

    @property
    def exc_hldropcodes(self):
        return self._exc_hldropcodes

    @property
    def exc_prehldropcodes(self):
        return self._exc_prehldropcodes

    @property
    def svcs(self):
        return self._svcs

    @property
    def asfacade(self):
        return self._asfacade
    @asfacade.setter
    def asfacade(self, value):
        self._asfacade = value

    def add_deocodes(self, *opcodes):
        for op in opcodes:
            if op not in self._deopcodes:
                self._deopcodes.append(op)
        return self

    def add_hldropcodes(self, *opcodes):
        for op in opcodes:
            if op not in self._hldropcodes:
                self._hldropcodes.append(op)
        return self

    def add_prehldropcodes(self, *opcodes):
        for op in opcodes:
            if op not in self._prehldropcodes:
                self._prehldropcodes.append(op)
        return self

    def add_exc_hldropcodes(self, *opcodes):
        for op in opcodes:
            if op not in self._exc_hldropcodes:
                self._exc_hldropcodes.append(op)
        return self

    def add_exc_prehldropcodes(self, *opcodes):
        for op in opcodes:
            if op not in self._exc_prehldropcodes:
                self._exc_prehldropcodes.append(op)
        return self

    def add_svcs(self, *svcs):
        self._svcs.update(svcs)
        return self

    def decorate(self, to_svc):
        wrapped = self._wrapped()
        if wrapped is None:
            return
        elif self._svcs and to_svc.name not in self._svcs:
            return

        ty = self._type
        if ty == self.Coder and \
                to_svc.type != llbc.Service.RAW:
            if self._enopcode is not None:
                # print 'register encoder {}:{}'.format(self._enopcode, wrapped)
                to_svc.registerencoder(self._enopcode, wrapped)
            if self._deopcodes:
                # print 'register decoder {}:{}'.format(self._deopcodes, wrapped)
                map(lambda op: to_svc.registerdecoder(op, wrapped), self._deopcodes)

        elif ty == self.Handler:
            map(lambda op: to_svc.subscribe(op, wrapped()), self._hldropcodes)
        elif ty == self.PreHandler:
            map(lambda op: to_svc.presubscribe(op, wrapped()), self._prehldropcodes)
        elif ty == self.UnifyPreHandler:
            to_svc.unify_presubscribe(wrapped())

        elif ty == self.ExcHandler:
            map(lambda op: to_svc.set_subscribe_exc_handler(op, wrapped()), self._exc_hldropcodes)
        elif ty == self.ExcPreHandler:
            map(lambda op: to_svc.set_presubscribe_exc_handler(op, wrapped()), self._exc_prehldropcodes)
        elif ty == self.DftExcHandler:
            to_svc.set_default_subscribe_exc_handler(wrapped())
        elif ty == self.DftExcPreHandler:
            to_svc.set_default_presubscribe_exc_handler(wrapped())

        if self._asfacade:
            to_svc.registerfacade(wrapped())

    def decorate_cls(self, to_svc_cls):
        wrapped = self._wrapped()
        if wrapped is None:
            return

        ty = self._type
        if ty == self.FrameExcHandler:
            to_svc_cls.set_frame_exc_handler(wrapped())

    def __str__(self):
        return 'wrapped:{}, regtype:{}, fmt:{}, enopcode:{}, deopcodes:{}, hldropcodes:{}, prehldropcodes:{}, \
                exc_hldropcodes: {}, exc_prehldropcodes: {}, svcs:{}, asfacade:{}'.format(
                self.wrapped, self._type, self._fmt, self._enopcode, self._deopcodes, self._hldropcodes, self._prehldropcodes, 
                self._exc_hldropcodes, self._exc_prehldropcodes, self._svcs, self._asfacade)

llbc.inl.SvcRegInfo = pyllbcSvcRegInfo

class pyllbcSvcRegsHolder(object):
    """
    Service register info holder.
    """
    _bound_regs = {} # key: service name, value: register set.
    _unbound_regs = set()

    _cls_decorated = False

    @classmethod
    def update(cls, reg):
        if not reg.svcs:
            cls._unbound_regs.update((reg, ))
        else:
            if reg in cls._unbound_regs:
                cls._unbound_regs.remove(reg)
            for svc in reg.svcs:
                if svc not in cls._bound_regs:
                    cls._bound_regs.update({svc: set()})
                cls._bound_regs[svc].update((reg, ))

    @classmethod
    def decorate(cls, to_svc):
        bound_regs = cls._bound_regs.get(to_svc.name)
        if bound_regs:
            map(lambda reg: reg.decorate(to_svc), bound_regs)
        map(lambda reg: reg.decorate(to_svc), cls._unbound_regs)

    @classmethod
    def decorate_cls(cls):
        if cls._cls_decorated:
            return

        will_decorate = llbc.Service
        for bound_regs in cls._bound_regs.itervalues():
            map(lambda reg: reg.decorate_cls(will_decorate), bound_regs)
        map(lambda reg: reg.decorate_cls(will_decorate), cls._unbound_regs)

        cls._cls_decorated = True

llbc.inl.SvcRegsHolder = pyllbcSvcRegsHolder

def pyllbc_extractreg(wrapped, ty):
    """
    Extract wrapped class/function library register info data, if not exist, will create it.
    """
    if not isinstance(wrapped, (TypeType, ClassType)):
        raise llbc.error('@forsend/@forrecv/@handler/@prehandler/@unify_prehandler/@exc_handler/@exc_prehandler/@bindto decorator ' \
                'must decorate class type object, could not decorate {}'.format(type(wrapped)))

    RegCls = llbc.inl.SvcRegInfo

    libkey = '__pyllbcreg__'
    if not hasattr(wrapped, libkey):
        setattr(wrapped, libkey, RegCls(wrapped))

    reg = getattr(wrapped, libkey)
    if reg.regtype == RegCls.UnSpecific:
        reg.regtype = ty
    elif ty != RegCls.UnSpecific and ty != reg.regtype:
        raise llbc.error('conflict to use @handler/@prehandler/@unify_prehandler/@exc_handler/@exc_prehandler \
                and @forsend/@forrecv decorator in the same class: {}'.format(wrapped))

    return reg

def pyllbc_frame_exc_handler(handler):
    """
    Decorate service per-frame exception handler.
    Handler must has __call__() method.
    """
    RegCls = llbc.inl.SvcRegInfo
    RegsHolder = llbc.inl.SvcRegsHolder

    reg = pyllbc_extractreg(handler, RegCls.FrameExcHandler)
    RegsHolder.update(reg)
    return handler

llbc.frame_exc_handler = pyllbc_frame_exc_handler

def pyllbc_handler(*opcodes):
    """
    Decorate packet-handler class, once decorate, library will use the class to handle packet.
    Handler must has __call__() method.
    """
    def generator(handler):
        RegCls = llbc.inl.SvcRegInfo
        RegsHolder = llbc.inl.SvcRegsHolder
        converted = [(opcode.OP if hasattr(opcode, 'OP') else \
                (opcode.OPCODE if hasattr(opcode, 'OPCODE') else opcode)) for opcode in opcodes]
        reg = pyllbc_extractreg(handler, RegCls.Handler)
        RegsHolder.update(reg.add_hldropcodes(*converted))
        return handler 

    return generator

llbc.handler = pyllbc_handler

def pyllbc_prehandler(*opcodes):
    """
    Decorate packet pre-handler class, once decorate, library will use the class to pre-handle packet.
    Handler must has __call__() method.
    """
    def generator(handler):
        RegCls = llbc.inl.SvcRegInfo
        RegsHolder = llbc.inl.SvcRegsHolder
        converted = [(opcode.OP if hasattr(opcode, 'OP') else \
                (opcode.OPCODE if hasattr(opcode, 'OPCODE') else opcode)) for opcode in opcodes]
        reg = pyllbc_extractreg(handler, RegCls.PreHandler)
        RegsHolder.update(reg.add_prehldropcodes(*converted))
        return handler

    return generator

llbc.prehandler = pyllbc_prehandler

def pyllbc_unify_prehandler(handler):
    """
    Decorate packet unify pre-handler class, once decorate, library will use the class to unify pre-handle packet.
    Handler must has handle() method or has __call__() method.
    """
    RegCls = llbc.inl.SvcRegInfo
    RegsHolder = llbc.inl.SvcRegsHolder
    reg = pyllbc_extractreg(handler, RegCls.UnifyPreHandler)
    RegsHolder.update(reg)
    return handler

llbc.unify_prehandler = pyllbc_unify_prehandler

def pyllbc_exc_handler(*opcodes):
    """
    Decorate packet handler exception handler class, once decorate, library will use the class to pre-handle packet.
    Handler must has __call__() method.
    """
    def generator(handler):
        RegCls = llbc.inl.SvcRegInfo
        RegsHolder = llbc.inl.SvcRegsHolder
        converted = [(opcode.OP if hasattr(opcode, 'OP') else \
                (opcode.OPCODE if hasattr(opcode, 'OPCODE') else opcode)) for opcode in opcodes]
        reg = pyllbc_extractreg(handler, RegCls.ExcHandler)
        RegsHolder.update(reg.add_exc_hldropcodes(*converted))
        return handler

    return generator

llbc.exc_handler = pyllbc_exc_handler

def pyllbc_dft_exc_handler(handler):
    """
    Decroate packet-handler's default exception handler class, once decorate, library will use the class to process 
    packet-handler's exception.
    Handler must has __call__() method.
    """
    RegCls = llbc.inl.SvcRegInfo
    RegsHolder = llbc.inl.SvcRegsHolder
    reg = pyllbc_extractreg(handler, RegCls.DftExcHandler)
    RegsHolder.update(reg)
    return handler

llbc.dft_exc_handler = pyllbc_dft_exc_handler

def pyllbc_dft_exc_prehandler(handler):
    """
    Decroate packet-prehandler's default exception handler class, once decorate, library will use the class to
    process packet-prehandler's exception.
    Handler must has __call__() method.
    """
    RegCls = llbc.inl.SvcRegInfo
    RegsHolder = llbc.inl.SvcRegsHolder
    reg = pyllbc_extractreg(handler, RegCls.DftExcPreHandler)
    RegsHolder.update(reg)
    return handler

llbc.dft_exc_prehandler = pyllbc_dft_exc_prehandler

def pyllbc_exc_prehandler(*opcodes):
    """
    Decorate packet prehandler exception handler class, once decorate, library will use the class to pre-handle packet.
    Handler must has __call__() method.
    """
    def generator(handler):
        RegCls = llbc.inl.SvcRegInfo
        RegsHolder = llbc.inl.SvcRegsHolder
        converted = [(opcode.OP if hasattr(opcode, 'OP') else \
                (opcode.OPCODE if hasattr(opcode, 'OPCODE') else opcode)) for opcode in opcodes]
        reg = pyllbc_extractreg(handler, RegCls.ExcPreHandler)
        RegsHolder.update(reg.add_exc_prehldropcodes(*converted))
        return handler

    return generator

llbc.exc_prehandler = pyllbc_exc_prehandler

def pyllbc_construct_STATUS_WRAPPER_ASSIGNMENTS():
    assigned = set(functools.WRAPPER_ASSIGNMENTS)
    assigned.remove('__name__')
    return assigned
llbc.STATUS_WRAPPER_ASSIGNMENTS = pyllbc_construct_STATUS_WRAPPER_ASSIGNMENTS()

def pyllbc_status_wrapper(wrapped,
        assigned = llbc.STATUS_WRAPPER_ASSIGNMENTS,
        updated = functools.WRAPPER_UPDATES):
    """
    LLBC library status code handler wrapper decorator, use to decorate status code handler.
    """
    return functools.partial(functools.update_wrapper, wrapped=wrapped,
            assigned=assigned, updated=updated)

llbc.status_wrapper = pyllbc_status_wrapper

def pyllbc_status(*status):
    """
    Decorate packet-handler class's status handler, library will use the method to handle packet.
    Status handler must callable.
    TODO: Will implement
    """
    pass

llbc.status = pyllbc_status

def pyllbc_forsend(opcode):
    """
    Decorate coder clas, once decorate, library will use the class to encode packet.
    """
    def generator(coder):
        RegCls = llbc.inl.SvcRegInfo
        RegsHolder = llbc.inl.SvcRegsHolder

        reg = pyllbc_extractreg(coder, RegCls.Coder)
        reg.enopcode = opcode.OP if hasattr(opcode, 'OP') else \
                (opcode.OPCODE if hasattr(opcode, 'OPCODE') else opcode)
        RegsHolder.update(reg)
        return coder

    return generator

llbc.forsend = pyllbc_forsend

def pyllbc_forrecv(*opcodes):
    """
    Decorate coder class, once decorate, library will use the class to decode packet.
    """
    def generator(coder):
        RegCls = llbc.inl.SvcRegInfo
        RegsHolder = llbc.inl.SvcRegsHolder

        converted = [(opcode.OP if hasattr(opcode, 'OP') else \
                (opcode.OPCODE if hasattr(opcode, 'OPCODE') else opcode)) for opcode in opcodes]
        reg = pyllbc_extractreg(coder, RegCls.Coder)
        RegsHolder.update(reg.add_deocodes(*converted))
        return coder

    return generator

llbc.forrecv = pyllbc_forrecv

def pyllbc_bindto(*svcs):
    """
    Specific packet-handler/coder/facade bind to which service.
    """
    def generator(cls):
        RegCls = llbc.inl.SvcRegInfo
        RegsHolder = llbc.inl.SvcRegsHolder

        reg = pyllbc_extractreg(cls, RegCls.UnSpecific)
        RegsHolder.update(reg.add_svcs(*svcs))
        return cls

    return generator

llbc.bindto = pyllbc_bindto

def pyllbc_facade(cls):
    """
    Decorate facade class, once decorate, in startup, llbc library 
    will auto create facade object and register to specific services.
    """
    RegCls = llbc.inl.SvcRegInfo
    RegsHolder = llbc.inl.SvcRegsHolder

    reg = pyllbc_extractreg(cls, RegCls.UnSpecific)
    reg.asfacade = True
    RegsHolder.update(reg)
    return cls

llbc.facade = pyllbc_facade

