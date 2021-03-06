/**
 * @file    Util_TextImpl.h
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2014/07/25
 * @version 1.0
 *
 * @brief
 */
#ifdef __LLBC_CORE_UTILS_UTIL_TEXT_H__

#include "llbc/core/utils/Util_Algorithm.h"

__LLBC_NS_BEGIN

template <>
inline LLBC_String LLBC_Num2Str(sint64 val, int radix)
{
    return LLBC_I64toA(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(uint64 val, int radix)
{
    return LLBC_UI64toA(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(sint32 val, int radix)
{
    return LLBC_Num2Str<sint64>(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(uint32 val, int radix)
{
    return LLBC_Num2Str<uint64>(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(sint16 val, int radix)
{
    return LLBC_Num2Str<sint64>(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(uint16 val, int radix)
{
    return LLBC_Num2Str<uint64>(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(sint8 val, int radix)
{
    return LLBC_Num2Str<sint64>(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(uint8 val, int radix)
{
    return LLBC_Num2Str<uint64>(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(long val, int radix)
{
    return LLBC_Num2Str<sint64>(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(ulong val, int radix)
{
    return LLBC_Num2Str<uint64>(val, radix);
}

template <>
inline LLBC_String LLBC_Num2Str(double val, int radix)
{
    char buf[64] = {0};

#if LLBC_TARGET_PLATFORM_NON_WIN32
    sprintf(buf, "%f", val);
#else // LLBC_TARGET_PLATFORM_WIN32
    sprintf_s(buf, sizeof(buf), "%f", val);
#endif // LLBC_TARGET_PLATFORM_NON_WIN32
    return buf;
}

template <>
inline LLBC_String LLBC_Num2Str(float val, int radix)
{
    return LLBC_Num2Str<double>(val, radix);
}

template <typename T>
inline LLBC_String LLBC_Num2Str(T val, int radix)
{
    if (radix != 10 && radix != 16)
        radix = 10;

    LLBC_String str;
    if (radix == 16)
        str += "0x";

    const ulong ptrVal = reinterpret_cast<ulong>(val);
    return (str + LLBC_Num2Str<ulong>(ptrVal, radix));
}

__LLBC_NS_END

#endif // __LLBC_CORE_UTILS_UTIL_TEXT_H__
