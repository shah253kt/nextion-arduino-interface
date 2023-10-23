#include "Nextion.h"

#define digits(x) int(floor(log(x) + 1))

Nextion::Nextion(Stream &stream)
    : m_stream(&stream),
      m_currentIndex(0)
{
}

void Nextion::update()
{
    if (!m_stream->available())
    {
        return;
    }

    while (m_stream->available())
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

        processBuffer();
        m_currentIndex = 0;
    }
}

void Nextion::reset()
{
    sendCommand(NextionConstants::Command::Reset);
}

void Nextion::sendRaw(const char *raw)
{
    m_stream->print(raw);
    writeTerminationBytes();
}

void Nextion::setText(const char *objectName, const char *value)
{
    char result[strlen(objectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + strlen(value) + 4];
    sprintf(result, "%s%s=\"%s\"", objectName, NextionConstants::TEXT_ATTRIBUTE, value);
    sendRaw(result);
}

void Nextion::setText(const NextionComponent &component, const char *value)
{
    setText(component.name, value);
}

void Nextion::setInteger(const char *objectName, int value)
{
    char result[strlen(objectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + digits(value) + 2];
    sprintf(result, "%s%s=%d", objectName, NextionConstants::NUMERIC_ATTRIBUTE, value);
    sendRaw(result);
}

void Nextion::setInteger(const NextionComponent &component, int value)
{
    setInteger(component.name, value);
}

void Nextion::getText(const char *objectName)
{
    char tmp[strlen(objectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + 1];
    sprintf(tmp, "%s%s", objectName, NextionConstants::TEXT_ATTRIBUTE);
    get(tmp);
}

void Nextion::getText(const NextionComponent &component)
{
    getText(component.name);
}

void Nextion::getInteger(const char *objectName)
{
    char tmp[strlen(objectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + 1];
    sprintf(tmp, "%s%s", objectName, NextionConstants::NUMERIC_ATTRIBUTE);
    get(tmp);
}

void Nextion::getInteger(const NextionComponent &component)
{
    getInteger(component.name);
}

void Nextion::getCurrentPageNumber()
{
    sendCommand(NextionConstants::Command::GetPageNumber);
}

void Nextion::convertTextToNumeric(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format)
{
    char src[strlen(sourceObjectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + 1];
    char dest[strlen(destinationObjectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + 1];
    sprintf(src, "%s%s", sourceObjectName, NextionConstants::TEXT_ATTRIBUTE);
    sprintf(dest, "%s%s", destinationObjectName, NextionConstants::NUMERIC_ATTRIBUTE);
    convert(src, dest, length, format);
}

void Nextion::convertTextToNumeric(const NextionComponent &source, const NextionComponent &destination, uint8_t length, NextionConstants::ConversionFormat format)
{
    convertTextToNumeric(source.name, destination.name, length, format);
}

void Nextion::convertNumericToText(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format)
{
    char src[strlen(sourceObjectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + 1];
    char dest[strlen(destinationObjectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + 1];
    sprintf(src, "%s%s", sourceObjectName, NextionConstants::NUMERIC_ATTRIBUTE);
    sprintf(dest, "%s%s", destinationObjectName, NextionConstants::TEXT_ATTRIBUTE);
    convert(src, dest, length, format);
}

void Nextion::convertNumericToText(const NextionComponent &source, const NextionComponent &destination, uint8_t length, NextionConstants::ConversionFormat format)
{
    convertNumericToText(source.name, destination.name, length, format);
}

void Nextion::sleep(bool isSleep)
{
    sendCommand(NextionConstants::Command::Sleep, isSleep);
}

// Private methods

bool Nextion::isBufferTerminated()
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

void Nextion::processBuffer()
{
    using namespace NextionConstants;

    switch (m_buffer[0])
    {
    case static_cast<uint8_t>(ReturnCode::TouchEvent):
    {
        if (onTouchEvent == nullptr || payloadSize() != ExpectedPayloadSize::TOUCH_EVENT)
        {
            return;
        }

        onTouchEvent(m_buffer[1], m_buffer[2], m_buffer[3] == static_cast<uint8_t>(ClickEvent::Pressed) ? ClickEvent::Pressed : ClickEvent::Released);
        break;
    }
    case static_cast<uint8_t>(ReturnCode::CurrentPageNumber):
    {
        if (onPageNumberUpdated == nullptr || payloadSize() != ExpectedPayloadSize::CURRENT_PAGE_NUMBER)
        {
            return;
        }

        onPageNumberUpdated(m_buffer[1]);
        break;
    }
    case static_cast<uint8_t>(ReturnCode::NumericDataEnclosed):
    {
        if (onNumericDataReceived == nullptr || payloadSize() != ExpectedPayloadSize::NUMERIC_DATA_ENCLOSED)
        {
            return;
        }

        onNumericDataReceived(m_buffer[1] | m_buffer[2] << 8 | m_buffer[3] << 16 | m_buffer[4] << 24);
        break;
    }
    case static_cast<uint8_t>(ReturnCode::StringDataEnclosed):
    {
        if (onStringDataReceived == nullptr)
        {
            return;
        }

        const auto dataLength = payloadSize();
        char payload[dataLength];

        for (auto i = 0; i < dataLength - 1; i++)
        {
            payload[i] = m_buffer[i + 1];
        }

        payload[dataLength - 1] = '\0';
        onStringDataReceived(payload);
        break;
    }
    default:
    {
        if (onUnhandledReturnCodeReceived)
        {
            onUnhandledReturnCodeReceived(m_buffer[0]);
        }

        break;
    }
    }
}

uint8_t Nextion::payloadSize()
{
    return m_currentIndex - NextionConstants::TERMINATION_BYTES_SIZE + 1;
}

void Nextion::writeTerminationBytes()
{
    for (auto i = 0; i < NextionConstants::TERMINATION_BYTES_SIZE; i++)
    {
        m_stream->write(NextionConstants::TERMINATION_BYTES[i]);
    }
}

void Nextion::writeCommand(NextionConstants::Command command)
{
    m_stream->print(getCommand(command));
    m_stream->write(NextionConstants::COMMAND_SEPARATOR);
}

const char *Nextion::getCommand(NextionConstants::Command command)
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
    case Command::GetPageNumber:
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
    }

    return nullptr;
}

void Nextion::sendCommand(NextionConstants::Command command)
{
    m_stream->print(getCommand(command));
    writeTerminationBytes();
}

void Nextion::sendCommand(NextionConstants::Command command, const NextionComponent &component)
{
    sendCommand(command, component.name);
}

void Nextion::sendParameterList(const NextionComponent &component)
{
    sendParameterList(component.name);
}

void Nextion::convert(const char *source, const char *destination, const uint8_t length, NextionConstants::ConversionFormat format)
{
    writeCommand(NextionConstants::Command::Convert);
    sendParameterList(source, destination, length, static_cast<uint8_t>(format));
    writeTerminationBytes();
}
