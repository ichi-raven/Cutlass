#pragma once

#include <unordered_map>
#include <cstdint>

namespace Cutlass
{
    enum class KeyState
    {
        ePressed,
        eReleased,
        eHeld,
        eNone,
    };

    enum class Key
    {
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        e0 = 48,
        e1 = 49,
        e2 = 50,
        e3 = 51,
        e4 = 52,
        e5 = 53,
        e6 = 54,
        e7 = 55,
        e8 = 56,
        e9 = 57,
        Semicolon = 59,
        Equal = 61,
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        LeftBracket = 91,
        BackSlash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        World1 = 161,
        World2 = 162,
        Escape = 256,
        Enter = 257,
        Tab = 258,
        BackSpace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        CapsLock= 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        F13 = 302,
        F14 = 303,
        F15 = 304,
        F16 = 305,
        F17 = 306,
        F18 = 307,
        F19 = 308,
        F20 = 309,
        F21 = 310,
        F22 = 311,
        F23 = 312,
        F24 = 313,
        F25 = 314,
        KP_0 = 320,
        KP_1 = 321,
        KP_2 = 322,
        KP_3 = 323,
        KP_4 = 324,
        KP_5 = 325,
        KP_6 = 326,
        KP_7 = 327,
        KP_8 = 328,
        KP_9 = 329,
        KP_DECIMAL = 330,
        KP_DIVIDE = 331,
        KP_MULTIPLY = 332,
        KP_SUBTRACT = 333,
        KP_ADD = 334,
        KP_ENTER = 335,
        KP_EQUAL = 336,
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348,
        LAST = Menu
    };

    constexpr uint32_t keyIndex[] =
        {
            static_cast<uint32_t>(Key::Space),
            static_cast<uint32_t>(Key::Apostrophe),
            static_cast<uint32_t>(Key::Comma),
            static_cast<uint32_t>(Key::Minus),
            static_cast<uint32_t>(Key::Period),
            static_cast<uint32_t>(Key::Slash),
            static_cast<uint32_t>(Key::e0),
            static_cast<uint32_t>(Key::e1),
            static_cast<uint32_t>(Key::e2),
            static_cast<uint32_t>(Key::e3),
            static_cast<uint32_t>(Key::e4),
            static_cast<uint32_t>(Key::e5),
            static_cast<uint32_t>(Key::e6),
            static_cast<uint32_t>(Key::e7),
            static_cast<uint32_t>(Key::e8),
            static_cast<uint32_t>(Key::e9),
            static_cast<uint32_t>(Key::Semicolon),
            static_cast<uint32_t>(Key::Equal),
            static_cast<uint32_t>(Key::A),
            static_cast<uint32_t>(Key::B),
            static_cast<uint32_t>(Key::C),
            static_cast<uint32_t>(Key::D),
            static_cast<uint32_t>(Key::E),
            static_cast<uint32_t>(Key::F),
            static_cast<uint32_t>(Key::G),
            static_cast<uint32_t>(Key::H),
            static_cast<uint32_t>(Key::I),
            static_cast<uint32_t>(Key::J),
            static_cast<uint32_t>(Key::K),
            static_cast<uint32_t>(Key::L),
            static_cast<uint32_t>(Key::M),
            static_cast<uint32_t>(Key::N),
            static_cast<uint32_t>(Key::O),
            static_cast<uint32_t>(Key::P),
            static_cast<uint32_t>(Key::Q),
            static_cast<uint32_t>(Key::R),
            static_cast<uint32_t>(Key::S),
            static_cast<uint32_t>(Key::T),
            static_cast<uint32_t>(Key::U),
            static_cast<uint32_t>(Key::V),
            static_cast<uint32_t>(Key::W),
            static_cast<uint32_t>(Key::X),
            static_cast<uint32_t>(Key::Y),
            static_cast<uint32_t>(Key::Z),
            static_cast<uint32_t>(Key::LeftBracket),
            static_cast<uint32_t>(Key::BackSlash),
            static_cast<uint32_t>(Key::RightBracket),
            static_cast<uint32_t>(Key::GraveAccent),
            static_cast<uint32_t>(Key::World1),
            static_cast<uint32_t>(Key::World2),
            static_cast<uint32_t>(Key::Escape),
            static_cast<uint32_t>(Key::Enter),
            static_cast<uint32_t>(Key::Tab),
            static_cast<uint32_t>(Key::BackSpace),
            static_cast<uint32_t>(Key::Insert),
            static_cast<uint32_t>(Key::Delete),
            static_cast<uint32_t>(Key::Right),
            static_cast<uint32_t>(Key::Left),
            static_cast<uint32_t>(Key::Down),
            static_cast<uint32_t>(Key::Up),
            static_cast<uint32_t>(Key::PageUp),
            static_cast<uint32_t>(Key::PageDown),
            static_cast<uint32_t>(Key::Home),
            static_cast<uint32_t>(Key::End),
            static_cast<uint32_t>(Key::CapsLock),
            static_cast<uint32_t>(Key::ScrollLock),
            static_cast<uint32_t>(Key::NumLock),
            static_cast<uint32_t>(Key::PrintScreen),
            static_cast<uint32_t>(Key::Pause),
            static_cast<uint32_t>(Key::F1),
            static_cast<uint32_t>(Key::F2),
            static_cast<uint32_t>(Key::F3),
            static_cast<uint32_t>(Key::F4),
            static_cast<uint32_t>(Key::F5),
            static_cast<uint32_t>(Key::F6),
            static_cast<uint32_t>(Key::F7),
            static_cast<uint32_t>(Key::F8),
            static_cast<uint32_t>(Key::F9),
            static_cast<uint32_t>(Key::F10),
            static_cast<uint32_t>(Key::F11),
            static_cast<uint32_t>(Key::F12),
            static_cast<uint32_t>(Key::F13),
            static_cast<uint32_t>(Key::F14),
            static_cast<uint32_t>(Key::F15),
            static_cast<uint32_t>(Key::F16),
            static_cast<uint32_t>(Key::F17),
            static_cast<uint32_t>(Key::F18),
            static_cast<uint32_t>(Key::F19),
            static_cast<uint32_t>(Key::F20),
            static_cast<uint32_t>(Key::F21),
            static_cast<uint32_t>(Key::F22),
            static_cast<uint32_t>(Key::F23),
            static_cast<uint32_t>(Key::F24),
            static_cast<uint32_t>(Key::F25),
            static_cast<uint32_t>(Key::KP_0),
            static_cast<uint32_t>(Key::KP_1),
            static_cast<uint32_t>(Key::KP_2),
            static_cast<uint32_t>(Key::KP_3),
            static_cast<uint32_t>(Key::KP_4),
            static_cast<uint32_t>(Key::KP_5),
            static_cast<uint32_t>(Key::KP_6),
            static_cast<uint32_t>(Key::KP_7),
            static_cast<uint32_t>(Key::KP_8),
            static_cast<uint32_t>(Key::KP_9),
            static_cast<uint32_t>(Key::KP_DECIMAL),
            static_cast<uint32_t>(Key::KP_DIVIDE),
            static_cast<uint32_t>(Key::KP_MULTIPLY),
            static_cast<uint32_t>(Key::KP_SUBTRACT),
            static_cast<uint32_t>(Key::KP_ADD),
            static_cast<uint32_t>(Key::KP_ENTER),
            static_cast<uint32_t>(Key::KP_EQUAL),
            static_cast<uint32_t>(Key::LeftShift),
            static_cast<uint32_t>(Key::LeftControl),
            static_cast<uint32_t>(Key::LeftAlt),
            static_cast<uint32_t>(Key::LeftSuper),
            static_cast<uint32_t>(Key::RightShift),
            static_cast<uint32_t>(Key::RightControl),
            static_cast<uint32_t>(Key::RightAlt),
            static_cast<uint32_t>(Key::RightSuper),
            static_cast<uint32_t>(Key::Menu)
            };

    class Event
    {
    public:
        Event();

        KeyState getKeyState(Key key);

        uint32_t getKeyFrame(Key key);

        void getMousePos(double &x, double &y);

        bool windowShouldClose();

        bool windowResized();

        std::unordered_map<Key, uint32_t> &getKeyRefInternal();

        void updateInternal(double mouseX, double mouseY, bool windowShouldClose = false);

    private:
        std::unordered_map<Key, uint32_t> mKeys;
        double mMouseX;
        double mMouseY;
        bool mWindowShouldClose;
    };
};