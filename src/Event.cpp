#include "Event.hpp"

#include <iostream>

namespace Cutlass
{
    Event::Event()
    {
        for (uint32_t i = 0; i < sizeof(keyIndex) / sizeof(uint32_t); ++i)
            mKeys.emplace(static_cast<KeyCode>(keyIndex[i]), 0);

        mMouseX = mMouseY = 0;

        mWindowShouldClose = false;
    }

    KeyState Event::getKeyState(KeyCode key)
    {
        if(mKeys[key] == 1)
            return KeyState::ePressed;
        if (mKeys[key] > 1)
            return KeyState::eHeld;
        if (mKeys[key] == UINT32_MAX)
            return KeyState::eReleased;
        if (mKeys[key] == 0)
            return KeyState::eNone;

        std::cerr << "ERROR!\n";
        return KeyState::eNone;
    }

    uint32_t Event::getKeyFrame(KeyCode key)
    {
        return mKeys[key] == UINT32_MAX ? 0 : mKeys[key];
    }

    void Event::getMousePos(double &x, double &y)
    {
        x = mMouseX;
        y = mMouseY;
    }

    bool Event::windowShouldClose()
    {
        return mWindowShouldClose;
    }

    std::unordered_map<KeyCode, uint32_t> & Event::getKeyRefInternal()
    {
        return mKeys;
    }

    void Event::updateInternal(double mouseX, double mouseY, bool windowShouldClose)
    {
        mMouseX = mouseX;
        mMouseY = mouseY;
        mWindowShouldClose = windowShouldClose;
    }
}; // namespace Cutlass
