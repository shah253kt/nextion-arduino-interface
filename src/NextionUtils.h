#include "Arduino.h"

namespace Utils
{
    [[nodiscard]] inline uint16_t rgbTo565(uint16_t r, uint16_t g, uint16_t b)
    {
        r = (r & 11111000) << 8;
        g = (g & 11111100) << 3;
        b >>= 3;
        return r | g | b;
    }
}