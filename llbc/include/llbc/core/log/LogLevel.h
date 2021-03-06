/**
 * @file    LogLevel.h
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2013/06/09
 * @version 1.0
 *
 * @brief
 */
#ifndef __LLBC_LOG_LEVEL_H__
#define __LLBC_LOG_LEVEL_H__

#include "llbc/common/Common.h"

__LLBC_NS_BEGIN

/**
 * \brief The log level class encapsulation.
 */
class LLBC_EXPORT LLBC_LogLevel
{
public:
    /**
     * Level value enumeration.
     */
    enum
    {
        Begin,

        Debug = Begin,
        Info,
        Warn,
        Error,
        Fatal,

        End
    };

public:
    /**
     * Get Debug level string describe.
     * @return const LLBC_String & - Debug level describe.
     */
    static const LLBC_String &GetDebugDesc();

    /**
     * Get Info level string describe.
     * @return const LLBC_String & - Info level describe.
     */
    static const LLBC_String &GetInfoDesc();

    /**
     * Get Warn level string describe.
     * @return const LLBC_String & - Warn level describe.
     */
    static const LLBC_String &GetWarnDesc();

    /**
     * Get Error level string describe.
     * @return const LLBC_String & - Error level describe.
     */
    static const LLBC_String &GetErrorDesc();

    /**
     * Get Fatal level string describe.
     * @return const LLBC_String & - Fatal level describe.
     */
    static const LLBC_String &GetFatalDesc();

    /**
     * Get specific log level string describe.
     * @param[in] level - log level.
     * @return const LLBC_String & - level describe.
     */
    static const LLBC_String &GetLevelDesc(int level);

public:
    /**
     * Get specific log level by describe.
     * @param[in] str - log level describe.
     * @return int - log level.
     */
    static int Str2Levevl(const char *str);
};

__LLBC_NS_END

#endif // !__LLBC_LOG_LEVEL_H__
