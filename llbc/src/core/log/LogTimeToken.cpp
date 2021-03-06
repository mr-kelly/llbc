/**
 * @file    LogTimeToken.cpp
 * @author  Longwei Lai<lailongwei@126.com>
 * @date    2013/06/10
 * @version 1.0
 *
 * @brief
 */

#include "llbc/common/Export.h"
#include "llbc/common/BeforeIncl.h"

#include "llbc/core/time/Time.h"

#include "llbc/core/log/LogData.h"
#include "llbc/core/log/LogFormattingInfo.h"
#include "llbc/core/log/LogTimeToken.h"

__LLBC_NS_BEGIN

LLBC_LogTimeToken::LLBC_LogTimeToken()
{
}

LLBC_LogTimeToken::~LLBC_LogTimeToken()
{
}

int LLBC_LogTimeToken::Initialize(LLBC_LogFormattingInfo *formatter, const LLBC_String &str)
{
    this->SetFormatter(formatter);
    return LLBC_RTN_OK;
}

int LLBC_LogTimeToken::GetType() const
{
    return LLBC_LogTokenType::TimeToken;
}

void LLBC_LogTimeToken::Format(const LLBC_LogData &data, LLBC_String &formattedData) const
{
    LLBC_Time logTime(data.logTime);
    int index = static_cast<int>(formattedData.size());
    formattedData.append(logTime.Format());

    LLBC_LogFormattingInfo *formatter = this->GetFormatter();
    formatter->Format(formattedData, index);
}

__LLBC_NS_END

#include "llbc/common/AfterIncl.h"
