#pragma once

#include "Arduino.h"

namespace NextionConstants
{
    constexpr uint8_t TERMINATION_BYTES[] = {0xFF, 0xFF, 0xFF};
    constexpr auto COMMAND_SEPARATOR = 0x20;
    constexpr auto PARAMETER_SEPARATOR = ',';

    enum class Command : uint16_t
    {
        Get,
        ChangePage,
        Refresh,
        Click,
        GetPageNumber
    };

    enum class ClickEvent : uint8_t
    {
        Released,
        Pressed
    };
}

class Nextion
{
public:
    Nextion(Stream &stream);
    ~Nextion() = default;

    void update();

    template <typename T>
    void get(T item)
    {
        sendCommand(NextionConstants::Command::Get, item);
    }

    template <typename T>
    void changePage(T page)
    {
        sendCommand(NextionConstants::Command::ChangePage, page);
    }

    template <typename T>
    void refresh(T item)
    {
        sendCommand(NextionConstants::Command::Refresh, item);
    }

    template <typename T>
    void click(T item, NextionConstants::ClickEvent event)
    {
        writeCommand(NextionConstants::Command::Click);
        sendParameterList(item, static_cast<uint8_t>(event));
        writeTerminationBytes();
    }

    void getCurrentPageNumber();

private:
    Stream *m_stream;

    void writeTerminationBytes();
    void writeCommand(NextionConstants::Command command);
    [[nodiscard]] const char *getCommand(NextionConstants::Command command);

    void sendCommand(NextionConstants::Command command);

    template <typename T>
    void sendCommand(NextionConstants::Command command, T payload)
    {
        writeCommand(command);
        m_stream->print(payload);
        writeTerminationBytes();
    }

    template <typename T>
    void sendParameterList(T param)
    {
        m_stream->print(param);
    }

    template <typename TFirst, typename... TRest>
    void sendParameterList(TFirst first, TRest... rest)
    {
        sendParameterList(first);

        if (sizeof...(rest) > 0)
        {
            m_stream->print(NextionConstants::PARAMETER_SEPARATOR);
            sendParameterList(rest...);
        }
    }
};
