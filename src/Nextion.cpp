#include "Nextion.h"

#define digits(x) int(floor(log(x) + 1))

Nextion::Nextion(Stream &stream)
    : m_stream(&stream)
{
}

void Nextion::update()
{
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

void Nextion::assignText(const char *objectName, const char *value)
{
    char result[strlen(objectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + strlen(value) + 4];
    sprintf(result, "%s%s=\"%s\"", objectName, NextionConstants::TEXT_ATTRIBUTE, value);
    sendRaw(result);
}

void Nextion::assignNumeric(const char *objectName, int value)
{
    char result[strlen(objectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + digits(value) + 2];
    sprintf(result, "%s%s=%d", objectName, NextionConstants::NUMERIC_ATTRIBUTE, value);
    sendRaw(result);
}

void Nextion::getCurrentPageNumber()
{
    sendCommand(NextionConstants::Command::GetPageNumber);
}

void Nextion::convertTextToNumeric(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer)
{
    char src[strlen(sourceObjectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + 1];
    char dest[strlen(destinationObjectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + 1];
    sprintf(src, "%s%s", sourceObjectName, NextionConstants::TEXT_ATTRIBUTE);
    sprintf(dest, "%s%s", destinationObjectName, NextionConstants::NUMERIC_ATTRIBUTE);
    convert(src, dest, length, format);
}

void Nextion::convertNumericToText(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer)
{
    char src[strlen(sourceObjectName) + strlen(NextionConstants::TEXT_ATTRIBUTE) + 1];
    char dest[strlen(destinationObjectName) + strlen(NextionConstants::NUMERIC_ATTRIBUTE) + 1];
    sprintf(src, "%s%s", sourceObjectName, NextionConstants::NUMERIC_ATTRIBUTE);
    sprintf(dest, "%s%s", destinationObjectName, NextionConstants::TEXT_ATTRIBUTE);
    convert(src, dest, length, format);
}

void Nextion::writeTerminationBytes()
{
    static auto terminationBytesSize = sizeof(NextionConstants::TERMINATION_BYTES) / sizeof(NextionConstants::TERMINATION_BYTES[0]);
    for (auto i = 0; i < terminationBytesSize; i++)
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
    switch (command)
    {
    case NextionConstants::Command::Reset:
    {
        return "rest";
    }
    case NextionConstants::Command::Get:
    {
        return "get";
    }
    case NextionConstants::Command::ChangePage:
    {
        return "page";
    }
    case NextionConstants::Command::Refresh:
    {
        return "ref";
    }
    case NextionConstants::Command::Click:
    {
        return "click";
    }
    case NextionConstants::Command::GetPageNumber:
    {
        return "sendme";
    }
    case NextionConstants::Command::Convert:
    {
        return "covx";
    }
    case NextionConstants::Command::SetVisibility:
    {
        return "vis";
    }
    case NextionConstants::Command::EnableTouchEvent:
    {
        return "tsw";
    }
    }

    return nullptr;
}

void Nextion::sendCommand(NextionConstants::Command command)
{
    m_stream->print(getCommand(command));
    writeTerminationBytes();
}

void Nextion::convert(const char *source, const char *destination, const uint8_t length, NextionConstants::ConversionFormat format)
{
    writeCommand(NextionConstants::Command::Convert);
    sendParameterList(source, destination, length, static_cast<uint8_t>(format));
    writeTerminationBytes();
}
