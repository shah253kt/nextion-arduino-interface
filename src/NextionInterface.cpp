#include "NextionInterface.h"

#define digits(x) int(floor(log(x) + 1))

NextionInterface::NextionInterface(Stream &stream)
    : m_stream(&stream),
      m_currentIndex(0)
{
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

void NextionInterface::update()
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

void NextionInterface::getText(const NextionComponent &component)
{
    m_componentRetrievingText = const_cast<NextionComponent *>(&component);
    getText(component.name());
}

void NextionInterface::getInteger(const NextionComponent &component)
{
    m_componentRetrievingInteger = const_cast<NextionComponent *>(&component);
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

void NextionInterface::sleep(bool isSleep)
{
    sendCommand(NextionConstants::Command::Sleep, isSleep);
}

// Private methods

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

void NextionInterface::processBuffer()
{
    using namespace NextionConstants;
    const auto returnCode = static_cast<ReturnCode>(m_buffer[0]);

    switch (returnCode)
    {
    case ReturnCode::TouchEvent:
    {
        if (payloadSize() != ExpectedPayloadSize::TOUCH_EVENT)
        {
            return;
        }

        const auto component = getComponent(m_buffer[1], m_buffer[2]);
        
        if (component != nullptr && component->onTouchEvent != nullptr)
        {
            component->onTouchEvent(static_cast<ClickEvent>(m_buffer[3]));
            return;
        }

        if (onTouchEvent == nullptr)
        {
            return;
        }

        onTouchEvent(m_buffer[1], m_buffer[2], static_cast<ClickEvent>(m_buffer[3]));
        break;
    }
    case ReturnCode::CurrentPageId:
    {
        if (onPageIdUpdated == nullptr || payloadSize() != ExpectedPayloadSize::CURRENT_PAGE_NUMBER)
        {
            return;
        }

        onPageIdUpdated(m_buffer[1]);
        break;
    }
    case ReturnCode::NumericDataEnclosed:
    {
        if (onNumericDataReceived == nullptr ||
            m_componentRetrievingInteger == nullptr ||
            payloadSize() != ExpectedPayloadSize::NUMERIC_DATA_ENCLOSED)
        {
            return;
        }

        uint32_t numericValue = m_buffer[1] | m_buffer[2] << 8 | m_buffer[3] << 16 | m_buffer[4] << 24;

        if (m_componentRetrievingInteger->onNumericDataReceived != nullptr)
        {
            m_componentRetrievingInteger->onNumericDataReceived(numericValue);
        }
        else
        {
            onNumericDataReceived(m_componentRetrievingInteger, numericValue);
        }

        break;
    }
    case ReturnCode::StringDataEnclosed:
    {
        if (onStringDataReceived == nullptr || m_componentRetrievingText == nullptr)
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

        if (m_componentRetrievingText->onStringDataReceived != nullptr)
        {
            m_componentRetrievingText->onStringDataReceived(payload);
        }
        else
        {
            onStringDataReceived(m_componentRetrievingText, payload);
        }

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

void NextionInterface::writeCommand(NextionConstants::Command command)
{
    m_stream->print(getCommand(command));
    m_stream->write(NextionConstants::COMMAND_SEPARATOR);
}

const char *NextionInterface::getCommand(NextionConstants::Command command)
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
    }

    return nullptr;
}

void NextionInterface::sendCommand(NextionConstants::Command command)
{
    m_stream->print(getCommand(command));
    writeTerminationBytes();
}

void NextionInterface::sendCommand(NextionConstants::Command command, const NextionComponent &component)
{
    sendCommand(command, component.name());
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
