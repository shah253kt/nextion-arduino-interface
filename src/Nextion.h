#pragma once

#include "Arduino.h"

namespace NextionConstants
{
    constexpr uint8_t TERMINATION_BYTES[] = {0xFF, 0xFF, 0xFF};
    constexpr auto COMMAND_SEPARATOR = 0x20;
    constexpr auto PARAMETER_SEPARATOR = ',';
    constexpr auto NUMERIC_ATTRIBUTE = ".val";
    constexpr auto TEXT_ATTRIBUTE = ".txt";

    enum class Command : uint16_t
    {
        Reset,
        Get,
        ChangePage,
        Refresh,
        Click,
        GetPageNumber,
        Convert,
        SetVisibility,
        EnableTouchEvent
    };

    enum class ClickEvent : uint8_t
    {
        Released,
        Pressed
    };

    enum class ConversionFormat : uint8_t
    {
        Integer,
        CommaSeparated,
        Hexadecimal
    };
}

class Nextion
{
public:
    Nextion(Stream &stream);
    ~Nextion() = default;

    void update();
    void reset();
    void sendRaw(const char *raw);
    void assignText(const char *objectName, const char *value);
    void assignNumeric(const char *objectName, int value);

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
    void convertTextToNumeric(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);
    void convertNumericToText(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);

    template <typename T>
    void setVisibility(T item, bool visible)
    {
        writeCommand(NextionConstants::Command::SetVisibility);
        sendParameterList(item, visible ? 1 : 0);
        writeTerminationBytes();
    }

    template <typename T>
    void setTouchEvent(T item, bool enable)
    {
        writeCommand(NextionConstants::Command::EnableTouchEvent);
        sendParameterList(item, enable ? 1 : 0);
        writeTerminationBytes();
    }

    template <typename T>
    void enableTouchEvent(T item)
    {
        setTouchEvent(item, true);
    }

    template <typename T>
    void disableTouchEvent(T item)
    {
        setTouchEvent(item, false);
    }

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

    void convert(const char *source, const char *destination, uint8_t length, NextionConstants::ConversionFormat format);
};
