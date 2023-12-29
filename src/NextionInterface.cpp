#include "NextionInterface.h"

#define digits(n) ((n) == 0 ? 1 : ((n) < 0 ? 2 : 1) + static_cast<int>(std::log10(std::abs(n))))
#define TIMEOUT 100

DateTime dateTime;

NextionInterface::NextionInterface(Stream &stream)
    : m_stream(&stream),
      m_currentIndex(0),
      m_componentYear(new NextionComponent(0, 0, "rtc0")),
      m_componentMonth(new NextionComponent(0, 0, "rtc1")),
      m_componentDay(new NextionComponent(0, 0, "rtc2")),
      m_componentHour(new NextionComponent(0, 0, "rtc3")),
      m_componentMinute(new NextionComponent(0, 0, "rtc4")),
      m_componentSecond(new NextionComponent(0, 0, "rtc5")),
      m_componentDayOfTheWeek(new NextionComponent(0, 0, "rtc6"))
{
    m_componentYear->onNumericDataReceived = [](uint32_t value)
    {
        dateTime.year = static_cast<uint16_t>(value);
    };

    m_componentMonth->onNumericDataReceived = [](uint32_t value)
    {
        dateTime.month = static_cast<uint8_t>(value);
    };

    m_componentDay->onNumericDataReceived = [](uint32_t value)
    {
        dateTime.day = static_cast<uint8_t>(value);
    };

    m_componentHour->onNumericDataReceived = [](uint32_t value)
    {
        dateTime.hour = static_cast<uint8_t>(value);
    };

    m_componentMinute->onNumericDataReceived = [](uint32_t value)
    {
        dateTime.minute = static_cast<uint8_t>(value);
    };

    m_componentSecond->onNumericDataReceived = [](uint32_t value)
    {
        dateTime.second = static_cast<uint8_t>(value);
    };

    m_componentDayOfTheWeek->onNumericDataReceived = [](uint32_t value)
    {
        dateTime.dayOfTheWeek = static_cast<uint8_t>(value);
    };
}

NextionInterface::~NextionInterface()
{
    delete m_componentYear;
    delete m_componentMonth;
    delete m_componentDay;
    delete m_componentHour;
    delete m_componentMinute;
    delete m_componentSecond;
    delete m_componentDayOfTheWeek;
}

void NextionInterface::registerComponent(NextionComponent &component)
{
    m_components.add(&component);
}

NextionComponent *NextionInterface::getComponent(uint8_t pageId, ComponentId componentId)
{
    for (size_t i = 0; i < m_components.size(); i++)
    {
        const auto component = m_components.get(i);
        if (component->id() == componentId && component->pageId() == pageId)
        {
            return component;
        }
    }

    return nullptr;
}

bool NextionInterface::update()
{
    if (!m_stream->available())
    {
        return false;
    }

    const auto shouldUnblockAt = millis() + TIMEOUT;

    while (m_stream->available() && millis() < shouldUnblockAt)
    {
        if (m_currentIndex >= NextionConstants::MAX_BUFFER_SIZE)
        {
            // Buffer overflow
            m_currentIndex = 0;
        }

        m_buffer[m_currentIndex] = m_stream->read();

        if (!isBufferTerminated())
        {
            m_currentIndex++;
            continue;
        }

        const auto processResult = processBuffer();
        m_currentIndex = 0;

        if (processResult)
        {
            return true;
        }
    }

    return false;
}

void NextionInterface::reset()
{
    sendCommand(NextionConstants::Command::Reset);
}

void NextionInterface::sendRaw(const char *raw)
{
    m_stream->print(raw);
    writeTerminationBytes();
}

void NextionInterface::setText(const NextionComponent &component, const char *value)
{
    setText(component.name(), value);
}

void NextionInterface::setInteger(const NextionComponent &component, int value)
{
    setInteger(component.name(), value);
}

void NextionInterface::getText(NextionComponent &component)
{
    m_componentRetrievingText = &component;
    getText(component.name());
}

void NextionInterface::getInteger(NextionComponent &component)
{
    m_componentRetrievingInteger = &component;
    getInteger(component.name());
}

void NextionInterface::getCurrentPageId()
{
    sendCommand(NextionConstants::Command::GetPageId);
}

void NextionInterface::convertTextToNumeric(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format)
{
    char src[strlen(sourceObjectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + 1];
    char dest[strlen(destinationObjectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + 1];
    sprintf(src, "%s%s", sourceObjectName, NextionConstants::TEXT_ATTRIBUTE);
    sprintf(dest, "%s%s", destinationObjectName, NextionConstants::NUMERIC_ATTRIBUTE);
    convert(src, dest, length, format);
}

void NextionInterface::convertTextToNumeric(const NextionComponent &source, const NextionComponent &destination, uint8_t length, NextionConstants::ConversionFormat format)
{
    convertTextToNumeric(source.name(), destination.name(), length, format);
}

void NextionInterface::convertNumericToText(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format)
{
    char src[strlen(sourceObjectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + 1];
    char dest[strlen(destinationObjectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + 1];
    sprintf(src, "%s%s", sourceObjectName, NextionConstants::NUMERIC_ATTRIBUTE);
    sprintf(dest, "%s%s", destinationObjectName, NextionConstants::TEXT_ATTRIBUTE);
    convert(src, dest, length, format);
}

void NextionInterface::convertNumericToText(const NextionComponent &source, const NextionComponent &destination, uint8_t length, NextionConstants::ConversionFormat format)
{
    convertNumericToText(source.name(), destination.name(), length, format);
}

void NextionInterface::setVisibility(const char *componentName, bool visible)
{
    writeCommand(NextionConstants::Command::SetVisibility);
    sendParameterList(componentName, visible ? 1 : 0);
    writeTerminationBytes();
}

void NextionInterface::setVisibility(const NextionComponent &component, bool visible)
{
    setVisibility(component.name(), visible);
}

void NextionInterface::sleep(bool isSleep)
{
    sendCommand(NextionConstants::Command::Sleep, isSleep);
}

void NextionInterface::setDate(uint8_t day, uint8_t month, uint16_t year)
{
    set(NextionConstants::Command::RtcDay, day);
    set(NextionConstants::Command::RtcMonth, month);
    set(NextionConstants::Command::RtcYear, year);
}

void NextionInterface::getDate()
{
    m_componentRetrievingInteger = m_componentDay;
    get(NextionConstants::Command::RtcDay);
    m_componentRetrievingInteger = m_componentMonth;
    get(NextionConstants::Command::RtcMonth);
    m_componentRetrievingInteger = m_componentYear;
    get(NextionConstants::Command::RtcYear);
    m_componentRetrievingInteger = m_componentDayOfTheWeek;
    get(NextionConstants::Command::RtcDayOfTheWeek);
}

void NextionInterface::setTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    set(NextionConstants::Command::RtcHour, hour);
    set(NextionConstants::Command::RtcMinute, minute);
    set(NextionConstants::Command::RtcSecond, second);
}

void NextionInterface::getTime()
{
    m_componentRetrievingInteger = m_componentHour;
    get(NextionConstants::Command::RtcHour);
    m_componentRetrievingInteger = m_componentMinute;
    get(NextionConstants::Command::RtcMinute);
    m_componentRetrievingInteger = m_componentSecond;
    get(NextionConstants::Command::RtcSecond);
}

char *NextionInterface::getDayOfTheWeek(NextionConstants::DayOfTheWeek day)
{
    switch (day)
    {
    case NextionConstants::DayOfTheWeek::Sunday:
    {
        return "Sunday";
    }
    case NextionConstants::DayOfTheWeek::Monday:
    {
        return "Monday";
    }
    case NextionConstants::DayOfTheWeek::Tuesday:
    {
        return "Tuesday";
    }
    case NextionConstants::DayOfTheWeek::Wednesday:
    {
        return "Wednesday";
    }
    case NextionConstants::DayOfTheWeek::Thursday:
    {
        return "Thursday";
    }
    case NextionConstants::DayOfTheWeek::Friday:
    {
        return "Friday";
    }
    case NextionConstants::DayOfTheWeek::Saturday:
    {
        return "Saturday";
    }
    }
}

void NextionInterface::setBackgroundColor(const NextionComponent &component, const NextionConstants::Color color)
{
    setBackgroundColor(component.name(), static_cast<uint16_t>(color));
}

void NextionInterface::setBackgroundColor2(const NextionComponent &component, const NextionConstants::Color color)
{
    setBackgroundColor2(component.name(), static_cast<uint16_t>(color));
}

void NextionInterface::setForegroundColor(const NextionComponent &component, const NextionConstants::Color color)
{
    setForegroundColor(component.name(), static_cast<uint16_t>(color));
}

void NextionInterface::setForegroundColor2(const NextionComponent &component, const NextionConstants::Color color)
{
    setForegroundColor2(component.name(), static_cast<uint16_t>(color));
}

void NextionInterface::setBackgroundColor(const NextionComponent &component, const uint16_t color)
{
    setBackgroundColor(component.name(), color);
}

void NextionInterface::setBackgroundColor2(const NextionComponent &component, const uint16_t color)
{
    setBackgroundColor2(component.name(), color);
}

void NextionInterface::setForegroundColor(const NextionComponent &component, const uint16_t color)
{
    setForegroundColor(component.name(), color);
}

void NextionInterface::setForegroundColor2(const NextionComponent &component, const uint16_t color)
{
    setForegroundColor2(component.name(), color);
}

void NextionInterface::setBackgroundColor(const char *objectName, const uint16_t color)
{
    setColor(objectName, NextionConstants::BACKGROUND_ATTRIBUTE, color);
}

void NextionInterface::setBackgroundColor2(const char *objectName, const uint16_t color)
{
    setColor(objectName, NextionConstants::BACKGROUND_2_ATTRIBUTE, color);
}

void NextionInterface::setForegroundColor(const char *objectName, const uint16_t color)
{
    setColor(objectName, NextionConstants::FOREGROUND_ATTRIBUTE, color);
}

void NextionInterface::setForegroundColor2(const char *objectName, const uint16_t color)
{
    setColor(objectName, NextionConstants::FOREGROUND_2_ATTRIBUTE, color);
}

DateTime NextionInterface::getDateTime()
{
    getDate();
    getTime();
    return dateTime;
}

// Private methods

bool NextionInterface::waitForResponse()
{
    const auto unblockAt = millis() + TIMEOUT;

    while (millis() < unblockAt)
    {
        if (update())
        {
            return true;
        }
    }

    return false;
}

bool NextionInterface::isBufferTerminated()
{
    if (m_currentIndex < NextionConstants::TERMINATION_BYTES_SIZE)
    {
        return false;
    }

    for (auto i = 0; i < NextionConstants::TERMINATION_BYTES_SIZE; i++)
    {
        if (NextionConstants::TERMINATION_BYTES[i] != m_buffer[m_currentIndex - NextionConstants::TERMINATION_BYTES_SIZE + i + 1])
        {
            return false;
        }
    }

    return true;
}

bool NextionInterface::processBuffer()
{
    using namespace NextionConstants;
    const auto returnCode = static_cast<ReturnCode>(m_buffer[0]);

    switch (returnCode)
    {
    case ReturnCode::TouchEvent:
    {
        if (payloadSize() != ExpectedPayloadSize::TOUCH_EVENT)
        {
            return false;
        }

        const auto component = getComponent(m_buffer[1], m_buffer[2]);

        if (component != nullptr && component->onTouchEvent != nullptr)
        {
            component->onTouchEvent(static_cast<ClickEvent>(m_buffer[3]));
            return false;
        }

        if (onTouchEvent == nullptr)
        {
            return false;
        }

        onTouchEvent(m_buffer[1], m_buffer[2], static_cast<ClickEvent>(m_buffer[3]));
        return true;
    }
    case ReturnCode::CurrentPageId:
    {
        if (onPageIdUpdated == nullptr || payloadSize() != ExpectedPayloadSize::CURRENT_PAGE_NUMBER)
        {
            return false;
        }

        onPageIdUpdated(m_buffer[1]);
        return true;
    }
    case ReturnCode::NumericDataEnclosed:
    {
        if (payloadSize() != ExpectedPayloadSize::NUMERIC_DATA_ENCLOSED)
        {
            return false;
        }

        uint32_t numericValue = m_buffer[1] | m_buffer[2] << 8 | m_buffer[3] << 16 | m_buffer[4] << 24;

        if (m_componentRetrievingInteger->onNumericDataReceived != nullptr)
        {
            m_componentRetrievingInteger->onNumericDataReceived(numericValue);
        }

        if (onNumericDataReceived != nullptr)
        {
            onNumericDataReceived(m_componentRetrievingInteger, numericValue);
        }

        return true;
    }
    case ReturnCode::StringDataEnclosed:
    {
        if (onStringDataReceived == nullptr || m_componentRetrievingText == nullptr)
        {
            return false;
        }

        const auto dataLength = payloadSize();
        char payload[dataLength];

        for (auto i = 0; i < dataLength - 1; i++)
        {
            payload[i] = m_buffer[i + 1];
        }

        payload[dataLength - 1] = '\0';

        if (m_componentRetrievingText->onStringDataReceived != nullptr)
        {
            m_componentRetrievingText->onStringDataReceived(payload);
        }
        else
        {
            onStringDataReceived(m_componentRetrievingText, payload);
        }

        return true;
    }
    default:
    {
        if (onUnhandledReturnCodeReceived)
        {
            onUnhandledReturnCodeReceived(m_buffer[0]);
        }

        return false;
    }
    }

    return false;
}

uint8_t NextionInterface::payloadSize()
{
    return m_currentIndex - NextionConstants::TERMINATION_BYTES_SIZE + 1;
}

void NextionInterface::writeTerminationBytes()
{
    for (auto i = 0; i < NextionConstants::TERMINATION_BYTES_SIZE; i++)
    {
        m_stream->write(NextionConstants::TERMINATION_BYTES[i]);
    }
}

void NextionInterface::writeCommand(const NextionConstants::Command &command)
{
    m_stream->print(getCommand(command));
    m_stream->print(NextionConstants::COMMAND_SEPARATOR);
}

const char *NextionInterface::getCommand(const NextionConstants::Command &command)
{
    using namespace NextionConstants;

    switch (command)
    {
    case Command::Reset:
    {
        return "rest";
    }
    case Command::Get:
    {
        return "get";
    }
    case Command::ChangePage:
    {
        return "page";
    }
    case Command::Refresh:
    {
        return "ref";
    }
    case Command::Click:
    {
        return "click";
    }
    case Command::GetPageId:
    {
        return "sendme";
    }
    case Command::Convert:
    {
        return "covx";
    }
    case Command::SetVisibility:
    {
        return "vis";
    }
    case Command::EnableTouchEvent:
    {
        return "tsw";
    }
    case Command::Sleep:
    {
        return "sleep";
    }
    case Command::RtcYear:
    {
        return "rtc0";
    }
    case Command::RtcMonth:
    {
        return "rtc1";
    }
    case Command::RtcDay:
    {
        return "rtc2";
    }
    case Command::RtcHour:
    {
        return "rtc3";
    }
    case Command::RtcMinute:
    {
        return "rtc4";
    }
    case Command::RtcSecond:
    {
        return "rtc5";
    }
    case Command::RtcDayOfTheWeek:
    {
        return "rtc6";
    }
    }

    // static_assert(false, "Failed to retrieve command. Unhandled command.");
    return nullptr;
}

void NextionInterface::sendCommand(const NextionConstants::Command &command)
{
    m_stream->print(getCommand(command));
    writeTerminationBytes();
}

void NextionInterface::sendCommand(const NextionConstants::Command &command, const NextionComponent &component)
{
    sendCommand(command, component.name());
}

template <>
void NextionInterface::sendCommand<NextionConstants::Command>(const NextionConstants::Command &command, const NextionConstants::Command &payload)
{
    writeCommand(command);
    m_stream->print(getCommand(payload));
    writeTerminationBytes();
}

void NextionInterface::sendParameterList(const NextionComponent &component)
{
    sendParameterList(component.name());
}

void NextionInterface::setText(const char *objectName, const char *value)
{
    char result[strlen(objectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + strlen(value) + 4];
    sprintf(result, "%s%s=\"%s\"", objectName, NextionConstants::TEXT_ATTRIBUTE, value);
    sendRaw(result);
}

void NextionInterface::setInteger(const char *objectName, int value)
{
    char result[strlen(objectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + digits(value) + 2];
    sprintf(result, "%s%s=%d", objectName, NextionConstants::NUMERIC_ATTRIBUTE, value);
    sendRaw(result);
}

void NextionInterface::getText(const char *objectName)
{
    char tmp[strlen(objectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + 1];
    sprintf(tmp, "%s%s", objectName, NextionConstants::TEXT_ATTRIBUTE);
    get(tmp);
}

void NextionInterface::getInteger(const char *objectName)
{
    char tmp[strlen(objectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + 1];
    sprintf(tmp, "%s%s", objectName, NextionConstants::NUMERIC_ATTRIBUTE);
    get(tmp);
}

void NextionInterface::convert(const char *source, const char *destination, const uint8_t length, NextionConstants::ConversionFormat format)
{
    writeCommand(NextionConstants::Command::Convert);
    sendParameterList(source, destination, length, static_cast<uint8_t>(format));
    writeTerminationBytes();
}

void NextionInterface::setColor(const char *objectName, const char *attribute, const uint16_t color)
{
    char result[strlen(objectName) + strlen(attribute) + digits(color) + 4];
    sprintf(result, "%s%s=%d", objectName, attribute, color);
    sendRaw(result);
}
