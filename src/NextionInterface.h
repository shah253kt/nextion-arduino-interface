#pragma once

#include "Arduino.h"
#include "NextionConstants.h"

#include <LinkedList.h>

using ComponentId = uint8_t;

struct DateTime
{
    uint8_t day{};
    uint8_t month{};
    uint16_t year{};
    uint8_t hour{};
    uint8_t minute{};
    uint8_t second{};
    uint8_t dayOfTheWeek{};
};

class NextionComponent
{
public:
    explicit NextionComponent(uint8_t pageId, ComponentId id, const char *name)
        : m_pageId(pageId), m_id(id)
    {
        m_name = new char[strlen(name) + 1];
        strcpy(m_name, name);
    }

    ~NextionComponent()
    {
        delete[] m_name;
    }

    [[nodiscard]] uint8_t pageId() const
    {
        return m_pageId;
    }

    [[nodiscard]] ComponentId id() const
    {
        return m_id;
    }

    [[nodiscard]] char *name() const
    {
        return m_name;
    }

    void (*onTouchEvent)(NextionConstants::ClickEvent event);
    void (*onNumericDataReceived)(int32_t data);
    void (*onStringDataReceived)(char *data);

private:
    uint8_t m_pageId;
    ComponentId m_id;
    char *m_name;
};

class NextionInterface
{
public:
    explicit NextionInterface(Stream &stream);
    ~NextionInterface();

    void registerComponent(NextionComponent &component);
    [[nodiscard]] NextionComponent *getComponent(uint8_t pageId, ComponentId componentId);

    void clearBuffer();
    bool update();
    void reset();
    void sendRaw(const char *raw);

    void setText(const NextionComponent &component, const char *value);
    void setInteger(const NextionComponent &component, int value);

    void getText(NextionComponent &component);
    void getInteger(NextionComponent &component);

    template <typename T>
    void changePage(const T &page)
    {
        sendCommand(NextionConstants::Command::ChangePage, page);
    }

    void changePage(const NextionComponent &) = delete;

    template <typename T>
    void refresh(const T &item)
    {
        sendCommand(NextionConstants::Command::Refresh, item);
    }

    template <typename T>
    void click(const T &item, NextionConstants::ClickEvent event)
    {
        writeCommand(NextionConstants::Command::Click);
        sendParameterList(item, static_cast<uint8_t>(event));
        writeTerminationBytes();
    }

    void getCurrentPageId();

    void convertTextToNumeric(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);
    void convertTextToNumeric(const NextionComponent &source, const NextionComponent &destination, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);

    void convertNumericToText(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);
    void convertNumericToText(const NextionComponent &source, const NextionComponent &destination, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);

    void setVisibility(const char *componentName, bool visible);
    void setVisibility(const NextionComponent &component, bool visible);

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

    void sleep(bool sleepMode);

    void setDate(uint8_t day, uint8_t month, uint16_t year);
    void getDate();
    void setTime(uint8_t hour, uint8_t minute, uint8_t second);
    void getTime();
    char *getDayOfTheWeek(NextionConstants::DayOfTheWeek day);
    DateTime getDateTime();

    void setBackgroundColor(const NextionComponent &component, NextionConstants::Color color);
    void setBackgroundColor2(const NextionComponent &component, NextionConstants::Color color);
    void setForegroundColor(const NextionComponent &component, NextionConstants::Color color);
    void setForegroundColor2(const NextionComponent &component, NextionConstants::Color color);

    void setBackgroundColor(const NextionComponent &component, uint16_t color);
    void setBackgroundColor2(const NextionComponent &component, uint16_t color);
    void setForegroundColor(const NextionComponent &component, uint16_t color);
    void setForegroundColor2(const NextionComponent &component, uint16_t color);

    void setBackgroundColor(const char *objectName, uint16_t color);
    void setBackgroundColor2(const char *objectName, uint16_t color);
    void setForegroundColor(const char *objectName, uint16_t color);
    void setForegroundColor2(const char *objectName, uint16_t color);

    void (*onTouchEvent)(uint8_t pageId, ComponentId componentId, NextionConstants::ClickEvent event);
    void (*onPageIdUpdated)(uint8_t pageId);
    void (*onNumericDataReceived)(const NextionComponent *component, int32_t data);
    void (*onStringDataReceived)(const NextionComponent *component, char *data);
    void (*onUnhandledReturnCodeReceived)(uint8_t returnCode);

private:
    Stream *m_stream;
    uint8_t m_buffer[NextionConstants::MAX_BUFFER_SIZE];
    uint8_t m_currentIndex;
    LinkedList<NextionComponent *> m_components;

    NextionComponent *m_componentRetrievingText;
    NextionComponent *m_componentRetrievingInteger;

    NextionComponent *m_componentYear;
    NextionComponent *m_componentMonth;
    NextionComponent *m_componentDay;
    NextionComponent *m_componentHour;
    NextionComponent *m_componentMinute;
    NextionComponent *m_componentSecond;
    NextionComponent *m_componentDayOfTheWeek;

    [[nodiscard]] bool waitForResponse();

    [[nodiscard]] bool isBufferTerminated();
    [[nodiscard]] bool processBuffer();
    [[nodiscard]] uint8_t payloadSize();

    void writeTerminationBytes();
    void writeCommand(const NextionConstants::Command &command);
    [[nodiscard]] const char *getCommand(const NextionConstants::Command &command);

    void sendCommand(const NextionConstants::Command &command);
    void sendCommand(const NextionConstants::Command &command, const NextionComponent &component);

    template <typename T>
    void sendCommand(const NextionConstants::Command &command, const T &payload)
    {
        writeCommand(command);
        m_stream->print(payload);
        writeTerminationBytes();
    }

    template <typename T>
    void sendParameterList(const T &param)
    {
        m_stream->print(param);
    }

    void sendParameterList(const NextionComponent &component);

    template <typename TFirst, typename... TRest>
    void sendParameterList(const TFirst &first, TRest... rest)
    {
        sendParameterList(first);

        if (sizeof...(rest) > 0)
        {
            m_stream->print(NextionConstants::PARAMETER_SEPARATOR);
            sendParameterList(rest...);
        }
    }

    void setText(const char *objectName, const char *value);
    void setInteger(const char *objectName, int value);

    template <typename T>
    void get(T item)
    {
        sendCommand(NextionConstants::Command::Get, item);
        // const auto isTimeout = !waitForResponse();
    }

    template <typename T>
    void set(NextionConstants::Command command, T item)
    {
        m_stream->print(getCommand(command));
        m_stream->write(NextionConstants::ASSIGNMENT_CHARACTER);
        m_stream->print(item);
        writeTerminationBytes();
    }

    void getText(const char *objectName);
    void getInteger(const char *objectName);

    void convert(const char *source, const char *destination, uint8_t length, NextionConstants::ConversionFormat format);

    void setColor(const char *objectName, const char *attribute, const uint16_t color);
};
