#ifndef CAINIAO_MOUSE_MOTION_FILTER_H
#define CAINIAO_MOUSE_MOTION_FILTER_H

#include <stdint.h>

struct MouseDelta
{
    int8_t x;
    int8_t y;
};

class MouseMotionFilter
{
public:
    MouseMotionFilter(float sensitivityX, float sensitivityY)
        : sensitivityX_(sensitivityX), sensitivityY_(sensitivityY),
          residualX_(0.0f), residualY_(0.0f)
    {
    }

    MouseDelta update(int16_t gy, int16_t gz)
    {
        residualX_ += -static_cast<float>(gz) * sensitivityX_;
        residualY_ += -static_cast<float>(gy) * sensitivityY_;

        const int32_t wholeX = static_cast<int32_t>(residualX_);
        const int32_t wholeY = static_cast<int32_t>(residualY_);
        residualX_ -= static_cast<float>(wholeX);
        residualY_ -= static_cast<float>(wholeY);

        return MouseDelta{clampHid(wholeX), clampHid(wholeY)};
    }

    void reset()
    {
        residualX_ = 0.0f;
        residualY_ = 0.0f;
    }

private:
    static int8_t clampHid(int32_t value)
    {
        if (value > 127)
            return 127;
        if (value < -127)
            return -127;
        return static_cast<int8_t>(value);
    }

    float sensitivityX_;
    float sensitivityY_;
    float residualX_;
    float residualY_;
};

#endif
