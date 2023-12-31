#include "Arduino.h"

namespace NextionConstants
{
    constexpr uint8_t TERMINATION_BYTES[] = {0xFF, 0xFF, 0xFF};
    constexpr auto TERMINATION_BYTES_SIZE = sizeof(TERMINATION_BYTES) / sizeof(TERMINATION_BYTES[0]);
    constexpr auto COMMAND_SEPARATOR = ' ';
    constexpr auto ASSIGNMENT_CHARACTER = '=';
    constexpr auto PARAMETER_SEPARATOR = ',';
    constexpr auto NUMERIC_ATTRIBUTE = ".val";
    constexpr auto TEXT_ATTRIBUTE = ".txt";
    constexpr auto BACKGROUND_ATTRIBUTE = ".bco";
    constexpr auto BACKGROUND_2_ATTRIBUTE = ".bco2";
    constexpr auto FOREGROUND_ATTRIBUTE = ".pco";
    constexpr auto FOREGROUND_2_ATTRIBUTE = ".pco2";
    constexpr auto MAX_BUFFER_SIZE = 32;
    constexpr auto MAX_COMPONENT_NAME_LENGTH = 10;

    enum class Command : uint16_t
    {
        Reset,
        Get,
        ChangePage,
        Refresh,
        Click,
        GetPageId,
        Convert,
        SetVisibility,
        EnableTouchEvent,
        Sleep,
        RtcYear,
        RtcMonth,
        RtcDay,
        RtcHour,
        RtcMinute,
        RtcSecond,
        RtcDayOfTheWeek
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

    enum class Color : uint16_t
    {
        BLACK = 0,
        BLUE = 31,
        BROWN = 48192,
        GREEN = 2016,
        YELLOW = 65504,
        RED = 63488,
        GRAY = 33840,
        LIGHT_GRAY = 19017,
        DARK_GRAY = 14823,
        WHITE = 65535,
        ORANGE = 64577,
        LIGHT_ORANGE = 64804
    };

    enum class ReturnCode : uint8_t
    {
        // Return codes dependent on bkcmd value being greater than 0
        InvalidInstruction = 0x00,
        InstructionSuccessful = 0x01,
        InvalidComponentId = 0x02,
        InvalidPageId = 0x03,
        InvalidPictureId = 0x04,
        InvalidFontId = 0x05,
        InvalidFileOperation = 0x06,
        InvalidCrc = 0x09,
        InvalidBaudRateSetting = 0x11,
        InvalidWaveformId = 0x12,
        InvalidVariableNameOrAttribute = 0x1A,
        InvalidVariableOperation = 0x1B,
        AssignmentFailed = 0x1C,
        EepromOperationFailed = 0x1D,
        InvalidNumberOfParameters = 0x1E,
        InputOutputOperationFailed = 0x1F,
        EscapeCharacter = 0x20,
        VariableNameTooLong = 0x23,

        // Return codes not affected by bkcmd value
        SerialBufferOverflow = 0x24,
        TouchEvent = 0x65,
        CurrentPageId = 0x66,
        TouchCoordinateAwake = 0x67,
        TouchCoordinateSleep = 0x68,
        StringDataEnclosed = 0x70,
        NumericDataEnclosed = 0x71,
        AutoEnteredSleepMode = 0x86,
        AutoWakeFromSleep = 0x87,
        NextionReady = 0x88,
        StartMicroSdUpgrade = 0x89,
        TransparentDataFinished = 0xFD,
        TransparentDataReady = 0xFE
    };

    namespace ExpectedPayloadSize
    {
        constexpr auto TOUCH_EVENT = 4;
        constexpr auto CURRENT_PAGE_NUMBER = 2;
        constexpr auto NUMERIC_DATA_ENCLOSED = 5;
    }

    enum class DayOfTheWeek
    {
        Sunday,
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday
    };

    static inline uint8_t getExpectedResponseLength(ReturnCode returnCode)
    {
        switch (returnCode)
        {
        case ReturnCode::InvalidInstruction:
        case ReturnCode::InstructionSuccessful:
        case ReturnCode::InvalidComponentId:
        case ReturnCode::InvalidPageId:
        case ReturnCode::InvalidPictureId:
        case ReturnCode::InvalidFontId:
        case ReturnCode::InvalidFileOperation:
        case ReturnCode::InvalidCrc:
        case ReturnCode::InvalidBaudRateSetting:
        case ReturnCode::InvalidWaveformId:
        case ReturnCode::InvalidVariableNameOrAttribute:
        case ReturnCode::InvalidVariableOperation:
        case ReturnCode::AssignmentFailed:
        case ReturnCode::EepromOperationFailed:
        case ReturnCode::InvalidNumberOfParameters:
        case ReturnCode::InputOutputOperationFailed:
        case ReturnCode::EscapeCharacter:
        case ReturnCode::VariableNameTooLong:
        case ReturnCode::SerialBufferOverflow:
        {
            return 1 + TERMINATION_BYTES_SIZE;
        }
        case ReturnCode::TouchEvent:
        {
            return 4 + TERMINATION_BYTES_SIZE;
        }
        case ReturnCode::CurrentPageId:
        {
            return 2 + TERMINATION_BYTES_SIZE;
        }
        case ReturnCode::TouchCoordinateAwake:
        case ReturnCode::TouchCoordinateSleep:
        case ReturnCode::StringDataEnclosed:
        {
            return TERMINATION_BYTES_SIZE;
        }
        case ReturnCode::NumericDataEnclosed:
        {
            return 5 + TERMINATION_BYTES_SIZE;
        }
        case ReturnCode::AutoEnteredSleepMode:
        case ReturnCode::AutoWakeFromSleep:
        case ReturnCode::NextionReady:
        case ReturnCode::StartMicroSdUpgrade:
        case ReturnCode::TransparentDataFinished:
        case ReturnCode::TransparentDataReady:
        {
            return 1 + TERMINATION_BYTES_SIZE;
        }
        default:
        {
            return 0;
        }
        }
    }
}
