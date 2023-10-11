#include "Nextion.h"

Nextion::Nextion(Stream &stream)
    : m_stream(&stream)
{
}

void Nextion::update()
{
}

void Nextion::getCurrentPageNumber()
{
    sendCommand(NextionConstants::Command::GetPageNumber);
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
    }

    return nullptr;
}

void Nextion::sendCommand(NextionConstants::Command command)
{
    writeCommand(command);
    writeTerminationBytes();
}
