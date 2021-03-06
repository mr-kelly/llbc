/**
 * @file    LogThreadIdToken.cpp
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2103/06/10
 * @version 1.0
 *
 * @brief
 */

#include "llbc/common/Export.h"
#include "llbc/common/BeforeIncl.h"

#include "llbc/core/utils/Util_Text.h"

#include "llbc/core/log/LogData.h"
#include "llbc/core/log/LogFormattingInfo.h"
#include "llbc/core/log/LogThreadIdToken.h"

__LLBC_NS_BEGIN

LLBC_LogThreadIdToken::LLBC_LogThreadIdToken()
{
}

LLBC_LogThreadIdToken::~LLBC_LogThreadIdToken()
{
}

int LLBC_LogThreadIdToken::Initialize(LLBC_LogFormattingInfo *formatter, const LLBC_String &str)
{
    this->SetFormatter(formatter);
    return LLBC_RTN_OK;
}

int LLBC_LogThreadIdToken::GetType() const
{
    return LLBC_LogTokenType::ThreadIdToken;
}

void LLBC_LogThreadIdToken::Format(const LLBC_LogData &data, LLBC_String &formattedData) const
{
    int index = static_cast<int>(formattedData.size());

#if LLBC_TARGET_PLATFORM_LINUX
    formattedData.append(LLBC_Num2Str((uint32)(data.threadHandle)));
#elif LLBC_TARGET_PLATFORM_WIN32
    formattedData.append(LLBC_Num2Str((uint32)(data.threadHandle)));
#elif LLBC_TARGET_PLATFORM_IPHONE
    formattedData.append(LLBC_Ptr2Str(data.threadHandle));
#elif LLBC_TARGET_PLATFORM_MAC
    formattedData.append(LLBC_Ptr2Str(data.threadHandle));
#elif LLBC_TARGET_PLATFORM_ANDROID
    formattedData.append(LLBC_Ptr2Str(data.threadHandle));
#endif

    LLBC_LogFormattingInfo *formatter = this->GetFormatter();
    formatter->Format(formattedData, index);
}

__LLBC_NS_END

#include "llbc/common/AfterIncl.h"
