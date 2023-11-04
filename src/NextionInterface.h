#pragma once

#include "Arduino.h"
#include "NextionConstants.h"

#include <LinkedList.h>

using ComponentId = uint8_t;

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
        delete m_name;
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
    void (*onNumericDataReceived)(uint32_t data);
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
    ~NextionInterface() = default;

    void registerComponent(NextionComponent &component);
    [[nodiscard]] NextionComponent *getComponent(uint8_t pageId, ComponentId componentId);

    void update();
    void reset();
    void sendRaw(const char *raw);

    void setText(const NextionComponent &component, const char *value);
    void setInteger(const NextionComponent &component, int value);

    void getText(const NextionComponent &component);
    void getInteger(const NextionComponent &component);

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

    void getCurrentPageId();

    void convertTextToNumeric(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);
    void convertTextToNumeric(const NextionComponent &source, const NextionComponent &destination, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);

    void convertNumericToText(const char *sourceObjectName, const char *destinationObjectName, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);
    void convertNumericToText(const NextionComponent &source, const NextionComponent &destination, uint8_t length, NextionConstants::ConversionFormat format = NextionConstants::ConversionFormat::Integer);

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

    void sleep(bool sleepMode);

    void (*onTouchEvent)(uint8_t pageId, ComponentId componentId, NextionConstants::ClickEvent event);
    void (*onPageIdUpdated)(uint8_t pageId);
    void (*onNumericDataReceived)(const NextionComponent *component, uint32_t data);
    void (*onStringDataReceived)(const NextionComponent *component, char *data);
    void (*onUnhandledReturnCodeReceived)(uint8_t returnCode);

private:
    Stream *m_stream;
    uint8_t m_buffer[NextionConstants::MAX_BUFFER_SIZE];
    uint8_t m_currentIndex;
    LinkedList<NextionComponent *> m_components;

    NextionComponent *m_componentRetrievingText;
    NextionComponent *m_componentRetrievingInteger;

    [[nodiscard]] bool isBufferTerminated();
    void processBuffer();
    [[nodiscard]] uint8_t payloadSize();

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

    void sendCommand(NextionConstants::Command command, const NextionComponent &component);

    template <typename T>
    void sendParameterList(T param)
    {
        m_stream->print(param);
    }

    void sendParameterList(const NextionComponent &component);

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

    void setText(const char *objectName, const char *value);
    void setInteger(const char *objectName, int value);

    template <typename T>
    void get(T item)
    {
        sendCommand(NextionConstants::Command::Get, item);
    }

    void getText(const char *objectName);
    void getInteger(const char *objectName);

    void convert(const char *source, const char *destination, uint8_t length, NextionConstants::ConversionFormat format);
};
