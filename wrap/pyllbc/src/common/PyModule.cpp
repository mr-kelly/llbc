/**
 * @file    PyModule.cpp
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2014/08/10
 * @version 1.0
 *
 * @brief
 */

#include "pyllbc/common/Export.h"

#include "pyllbc/common/Macro.h"
#include "pyllbc/common/Errors.h"
#include "pyllbc/common/PyModule.h"

namespace
{
    typedef pyllbc_Module This;
}

pyllbc_Module::pyllbc_Module(const LLBC_String &name, This *parent)
: _name(name)
, _module(NULL)
, _methods()

, _parent(NULL)
, _subModules()

, _lazyAddObjs()

, _dict(NULL)
{
    ASSERT(!_name.empty() &&
        "pyllbc_Module::ctor(): module name must be not null!");

    if (parent)
        parent->AddSubModule(this);
}

pyllbc_Module::~pyllbc_Module()
{
    /* Cleanup all lazy add objs. */
    for (_Objs::iterator it = _lazyAddObjs.begin();
         it != _lazyAddObjs.end();
         it++)
    {
        PyObject *obj = it->second;
        Py_DECREF(obj);
    }

    /* Destroy all sub modules. */
    LLBC_STLHelper::DeleteContainer(_subModules, false);

    /* Dereference self python module object. */
    Py_XDECREF(_module);
}

PyObject *pyllbc_Module::GetPyModule() const
{
    return _module;
}

PyObject *pyllbc_Module::GetModuleDict() const
{
    if (!_dict)
    {
        pyllbc_SetError("specific module not init, could not get module dict");
        return NULL;
    }

    return _dict;
}

PyObject *pyllbc_Module::GetObject(PyObject *name)
{
    PyObject *dict = this->GetModuleDict();
    if (!dict)
        return NULL;

    PyObject *obj = PyDict_GetItem(dict, name);
    if (!obj)
    {
        pyllbc_SetError("could not found module object");
        return NULL;
    }

    return obj;
}

PyObject *pyllbc_Module::GetObject(const LLBC_String &name)
{
    PyObject *pyName = 
        PyString_FromStringAndSize(name.data(), name.size());

    PyObject *obj = this->GetObject(pyName);

    Py_DECREF(pyName);

    return obj;
}

const LLBC_String &pyllbc_Module::GetModuleName() const
{
    return _name;
}

This *pyllbc_Module::GetParentModule() const
{
    return _parent;
}

This *pyllbc_Module::GetSubModule(const LLBC_String &name) const
{
    std::vector<LLBC_String> names;
    LLBC_SplitString(name, ".", names);

    if (names.empty())
    {
        pyllbc_SetError("module name empty", LLBC_ERROR_INVALID);
        return NULL;
    }
    
    _Modules::const_iterator it = _subModules.find(names[0]);
    if (it == _subModules.end())
    {
        pyllbc_SetError("not found sub module", LLBC_ERROR_NOT_FOUND);
        return NULL;
    }

    This *module = it->second;
    for (size_t i = 1; i < names.size(); i++)
    {
        const LLBC_String &splittedName = names[i];
        if (splittedName.empty())
        {
            pyllbc_SetError("module name empty", LLBC_ERROR_INVALID);
            return NULL;
        }

        if (!(module = module->GetSubModule(splittedName)))
            return NULL;
    }

    return module;
}

int pyllbc_Module::AddSubModule(pyllbc_Module *module)
{
    if (module->GetParentModule())
    {
        pyllbc_SetError("will add sub module already exist parent module", LLBC_ERROR_REENTRY);
        return LLBC_RTN_FAILED;
    }
    else if (this->GetSubModule(module->GetModuleName()))
    {
        pyllbc_SetError("sub module name repeat", LLBC_ERROR_REPEAT);
        return LLBC_RTN_FAILED;
    }

    pyllbc_ClearError();

    module->_parent = this;
    const LLBC_String &moduleName = module->GetModuleName();
    _subModules.insert(std::make_pair(moduleName, module));

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddMethod(const PyMethodDef &method)
{
    if (_module)
    {
        pyllbc_SetError(
            "module already build, not permit to add new method", LLBC_ERROR_INVALID);
        return LLBC_RTN_FAILED;
    }

    return _methods.AddMethod(method);
}

int pyllbc_Module::AddMethod(const LLBC_String &name, 
                             PyCFunction meth, 
                             int flags, 
                             const LLBC_String &doc)
{
    char *hName = LLBC_Malloc(char, name.size() + 1);
    LLBC_MemCpy(hName, name.c_str(), name.size());
    hName[name.size()] = '\0';

    char *hDoc = NULL;
    if (!doc.empty())
    {
        hDoc = LLBC_Malloc(char, doc.size() + 1);
        LLBC_MemCpy(hDoc, doc.c_str(), doc.size());
        hDoc[doc.size()] = '\0';
    }

    PyMethodDef md;
    md.ml_name = hName;
    md.ml_meth = meth;
    md.ml_flags = flags;
    md.ml_doc = hDoc;

    return this->AddMethod(md);
}

int pyllbc_Module::AddObject(const LLBC_String &name, PyObject *obj)
{
    if (name.empty())
    {
        pyllbc_SetError("object name must be not empty", LLBC_ERROR_INVALID);
        return LLBC_RTN_FAILED;
    }
    else if (_lazyAddObjs.find(name) != _lazyAddObjs.end())
    {
        pyllbc_SetError("object name repeat", LLBC_ERROR_REPEAT);
        return LLBC_RTN_FAILED;
    }

    if (!_module)
    {
        _lazyAddObjs.insert(std::make_pair(name, obj));
        return LLBC_RTN_OK;
    }

    if (PyModule_AddObject(_module, name.c_str(), obj) != 0)
    {
        pyllbc_SetError(PYLLBC_ERROR_COMMON);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddObject(const LLBC_String &name, bool obj)
{
    PyObject *o = PyBool_FromLong(obj ? 1 : 0);
    if (this->AddObject(name, o) != LLBC_RTN_OK)
    {
        Py_DECREF(o);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddObject(const LLBC_String &name, sint8 obj)
{
    PyObject *o = Py_BuildValue("c", obj);
    if (this->AddObject(name, o) != LLBC_RTN_OK)
    {
        Py_DECREF(o);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddObject(const LLBC_String &name, sint32 obj)
{
    PyObject *o = Py_BuildValue("i", obj);
    if (this->AddObject(name, o) != LLBC_RTN_OK)
    {
        Py_DECREF(o);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddObject(const LLBC_String &name, uint32 obj)
{
    PyObject *o = Py_BuildValue("I", obj);
    if (this->AddObject(name, o) != LLBC_RTN_OK)
    {
        Py_DECREF(o);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddObject(const LLBC_String &name, sint64 obj)
{
    PyObject *o = Py_BuildValue("L", obj);
    if (this->AddObject(name, o) != LLBC_RTN_OK)
    {
        Py_DECREF(o);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddObject(const LLBC_String &name, uint64 obj)
{
    PyObject *o = Py_BuildValue("K", obj);
    if (this->AddObject(name, o) != LLBC_RTN_OK)
    {
        Py_DECREF(o);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddObject(const LLBC_String &name, double obj)
{
    PyObject *o = Py_BuildValue("d", obj);
    if (this->AddObject(name, o) != LLBC_RTN_OK)
    {
        Py_DECREF(o);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::AddObject(const LLBC_String &name, const LLBC_String &obj)
{
    PyObject *o = Py_BuildValue("s", obj.c_str());
    if (this->AddObject(name, o) != LLBC_RTN_OK)
    {
        Py_DECREF(o);
        return LLBC_RTN_FAILED;
    }

    return LLBC_RTN_OK;
}

int pyllbc_Module::Build()
{
    if (!_module)
    {
        if (!(_module = Py_InitModule(_name.c_str(), _methods.GetMethods())))
        {
            pyllbc_SetError(PYLLBC_ERROR_COMMON);
            return LLBC_RTN_FAILED;
        }

        for (_Objs::iterator it = _lazyAddObjs.begin();
             it != _lazyAddObjs.end();
             )
        {
            PyObject *obj = it->second;
            const char *objName = it->first.c_str();
            if (PyModule_AddObject(_module, objName, obj) != 0)
            {
                pyllbc_SetError(PYLLBC_ERROR_COMMON);
                return LLBC_RTN_FAILED;
            }

            _lazyAddObjs.erase(it++);
        }
    }

    for (_Modules::iterator it = _subModules.begin();
         it != _subModules.end();
         it++)
    {
        This *subModule = it->second;
        if (subModule->Build() != LLBC_RTN_OK)
            return LLBC_RTN_FAILED;

        const LLBC_String &subModuleName = it->first;

        PyObject *modDict = PyModule_GetDict(_module);
        if (PyDict_GetItemString(modDict, subModuleName.c_str()))
            continue;

        PyObject *pySubModule = subModule->GetPyModule();

        Py_INCREF(pySubModule);
        if (this->AddObject(subModuleName, pySubModule) != LLBC_RTN_OK)
        {
            Py_DECREF(pySubModule);
            return LLBC_RTN_FAILED;
        }
    }

    _dict = PyModule_GetDict(_module);

    return LLBC_RTN_OK;
}
