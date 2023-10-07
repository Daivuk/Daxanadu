#include "InputContext.h"

#include <onut/Strings.h>


Input Input::None;


Input::Input()
{
}


Input::Input(OInput::State in_key)
    : key(in_key)
{
}


Input::Input(onut::GamePad::Button in_button)
    : button(in_button)
{
}


bool Input::is_bound() const
{
    return key != OInput::State::None ||
           button != OGamePad::Button::None;
}


bool Input::is_pressed() const
{
    if (key != OInput::State::None)
    {
        return OInputPressed(key);
    }
    else if (button != OGamePad::Button::None)
    {
        return OGamePadPressed(button);
    }
    return false;
}


bool Input::is_just_pressed() const
{
    if (key != OInput::State::None)
    {
        return OInputJustPressed(key);
    }
    else if (button != OGamePad::Button::None)
    {
        return OGamePadJustPressed(button);
    }
    return false;
}


bool Input::is_just_released() const
{
    if (key != OInput::State::None)
    {
        return OInputJustReleased(key);
    }
    else if (button != OGamePad::Button::None)
    {
        return OGamePadJustReleased(button);
    }
    return false;
}


std::string Input::get_name() const
{
    if (key != OInput::State::None)
    {
        switch (key)
        {
            case OInput::State::KeyEscape: return "ESCAPE";
            case OInput::State::Key1: return "1";
            case OInput::State::Key2: return "2";
            case OInput::State::Key3: return "3";
            case OInput::State::Key4: return "4";
            case OInput::State::Key5: return "5";
            case OInput::State::Key6: return "6";
            case OInput::State::Key7: return "7";
            case OInput::State::Key8: return "8";
            case OInput::State::Key9: return "9";
            case OInput::State::Key0: return "0";
            case OInput::State::KeyMinus: return "MINUS"; /* - on main keyboard */
            case OInput::State::KeyEquals: return "EQUAL";
            case OInput::State::KeyBackspace: return "BACKSPACE"; /* backspace */
            case OInput::State::KeyTab: return "TAB";
            case OInput::State::KeyQ: return "Q";
            case OInput::State::KeyW: return "W";
            case OInput::State::KeyE: return "E";
            case OInput::State::KeyR: return "R";
            case OInput::State::KeyT: return "T";
            case OInput::State::KeyY: return "Y";
            case OInput::State::KeyU: return "U";
            case OInput::State::KeyI: return "I";
            case OInput::State::KeyO: return "O";
            case OInput::State::KeyP: return "P";
            case OInput::State::KeyLeftBracket: return "[";
            case OInput::State::KeyRightBracket: return "]";
            case OInput::State::KeyEnter: return "ENTER"; /* Enter on main keyboard */
            case OInput::State::KeyLeftControl: return "LEFT CONTROL";
            case OInput::State::KeyA: return "A";
            case OInput::State::KeyS: return "S";
            case OInput::State::KeyD: return "D";
            case OInput::State::KeyF: return "F";
            case OInput::State::KeyG: return "G";
            case OInput::State::KeyH: return "H";
            case OInput::State::KeyJ: return "J";
            case OInput::State::KeyK: return "K";
            case OInput::State::KeyL: return "L";
            case OInput::State::KeySemiColon: return "SEMI COLON";
            case OInput::State::KeyApostrophe: return "APOSTROPHE";
            case OInput::State::KeyGrave: return "GRAVE";    /* accent grave */
            case OInput::State::KeyLeftShift: return "LEFT SHIFT";
            case OInput::State::KeyBackslash: return "BACKSLASH";
            case OInput::State::KeyZ: return "Z";
            case OInput::State::KeyX: return "X";
            case OInput::State::KeyC: return "C";
            case OInput::State::KeyV: return "V";
            case OInput::State::KeyB: return "B";
            case OInput::State::KeyN: return "N";
            case OInput::State::KeyM: return "M";
            case OInput::State::KeyComma: return "COMMA";
            case OInput::State::KeyPeriod: return "PERIOD"; /* . on main keyboard */
            case OInput::State::KeySlash: return "SLASH"; /* / on main keyboard */
            case OInput::State::KeyRightShift: return "RIGHT SHIFT";
            case OInput::State::KeyMultiply: return "NUM MUL"; /* * on numeric keypad */
            case OInput::State::KeyLeftAlt: return "LEFT ALT"; /* left Alt */
            case OInput::State::KeySpaceBar: return "SPACE BAR";
            case OInput::State::KeyCapsLock: return "CAPS LOCK";
            case OInput::State::KeyF1: return "F1";
            case OInput::State::KeyF2: return "F2";
            case OInput::State::KeyF3: return "F3";
            case OInput::State::KeyF4: return "F4";
            case OInput::State::KeyF5: return "F5";
            case OInput::State::KeyF6: return "F6";
            case OInput::State::KeyF7: return "F7";
            case OInput::State::KeyF8: return "F8";
            case OInput::State::KeyF9: return "F9";
            case OInput::State::KeyF10: return "F10";
            case OInput::State::KeyNumLock: return "NUM LOCK";
            case OInput::State::KeyScrollLock: return "SCROLL LOCK"; /* Scroll Lock */
            case OInput::State::KeyNumPad7: return "NUM 7";
            case OInput::State::KeyNumPad8: return "NUM 8";
            case OInput::State::KeyNumPad9: return "NUM 9";
            case OInput::State::KeyNumPadMinus: return "NUM MINUS"; /* - on numeric keypad */
            case OInput::State::KeyNumPad4: return "NUM 4";
            case OInput::State::KeyNumPad5: return "NUM 5";
            case OInput::State::KeyNumPad6: return "NUM 6";
            case OInput::State::KeyNumPadAdd: return "NUM ADD"; /* + on numeric keypad */
            case OInput::State::KeyNumPad1: return "NUM 1";
            case OInput::State::KeyNumPad2: return "NUM 2";
            case OInput::State::KeyNumPad3: return "NUM 3";
            case OInput::State::KeyNumPad0: return "NUM 0";
            case OInput::State::KeyNumPadPeriod: return "NUM PERIOD"; /* . on numeric keypad */
            //case OInput::State::KeyOEM102: return ""; /* <> or \| on RT 102-key keyboard (Non-U.S.) */
            case OInput::State::KeyF11: return "F11";
            case OInput::State::KeyF12: return "F12";
            case OInput::State::KeyF13: return "F13"; /* (NEC PC98) */
            case OInput::State::KeyF14: return "F14"; /* (NEC PC98) */
            case OInput::State::KeyF15: return "F15"; /* (NEC PC98) */
            case OInput::State::KeyKana: return "KANA"; /* (Japanese keyboard) */
            case OInput::State::KeyAbntC1: return "ABNT C1"; /* /? on Brazilian keyboard */
            case OInput::State::KeyConvert: return "CONVERT"; /* (Japanese keyboard) */
            case OInput::State::KeyNoConvert: return "NO CONVERT"; /* (Japanese keyboard) */
            case OInput::State::KeyYen: return "YEN"; /* (Japanese keyboard) */
            case OInput::State::KeyAbntC2: return "ABNT C2"; /* Numpad . on Brazilian keyboard */
            case OInput::State::KeyNumPadEquals: return "NUM EQUAL"; /* = on numeric keypad (NEC PC98) */
            case OInput::State::KeyPreviousTrack: return "PREV TRACK"; /* Previous Track (OINPUT_CIRCUMFLEX on Japanese keyboard) */
            case OInput::State::KeyAt: return "AT"; /* (NEC PC98) */
            case OInput::State::KeyColon: return ":"; /* (NEC PC98) */
            case OInput::State::KeyUnderline: return "UNDERLINE"; /* (NEC PC98) */
            case OInput::State::KeyKanji: return "KANJI"; /* (Japanese keyboard) */
            case OInput::State::KeyStop: return "STOP"; /* (NEC PC98) */
            case OInput::State::KeyAx: return "AX"; /* (Japan AX) */
            case OInput::State::KeyUnlabeled: return "J3100"; /* (J3100) */
            case OInput::State::KeyNextTrack: return "NEXT TRACK"; /* Next Track */
            case OInput::State::KeyNumPadEnter: return "NUM ENTER"; /* Enter on numeric keypad */
            case OInput::State::KeyRightControl: return "RIGHT CONTROL";
            case OInput::State::KeyMute: return "MUTE"; /* Mute */
            case OInput::State::KeyCalculator: return "CALCULATOR"; /* Calculator */
            case OInput::State::KeyPlayPause: return "PLAY"; /* Play / Pause */
            case OInput::State::KeyMediaStop: return "MEDIA STOP"; /* Media Stop */
            case OInput::State::KeyVolumeDown: return "VOL DOWN"; /* Volume - */
            case OInput::State::KeyVolumeUp: return "VOL UP"; /* Volume + */
            case OInput::State::KeyWebHome: return "WEB HOME"; /* Web home */
            case OInput::State::KeyNumPadComma: return "NUM COMMA"; /* , on numeric keypad (NEC PC98) */
            case OInput::State::KeyNumPadDivide: return "NUM SLASH"; /* / on numeric keypad */
            case OInput::State::Key_SYSRQ: return "SYSRQ";
            case OInput::State::KeyRightAlt: return "RIGHT ALT"; /* right Alt */
            //case OInput::State::KeyAltCar: return ""; /* right Alt */
            case OInput::State::KeyPause: return "PAUSE"; /* Pause */
            case OInput::State::KeyHome: return "HOME"; /* Home on arrow keypad */
            case OInput::State::KeyUp: return "UP"; /* UpArrow on arrow keypad */
            case OInput::State::KeyPageUp: return "PAGE UP"; /* PgUp on arrow keypad */
            case OInput::State::KeyLeft: return "LEFT"; /* LeftArrow on arrow keypad */
            case OInput::State::KeyRight: return "RIGHT"; /* RightArrow on arrow keypad */
            case OInput::State::KeyEnd: return "END"; /* End on arrow keypad */
            case OInput::State::KeyDown: return "DOWN"; /* DownArrow on arrow keypad */
            case OInput::State::KeyPageDown: return "PAGE DOWN"; /* PgDn on arrow keypad */
            case OInput::State::KeyInsert: return "INSERT"; /* Insert on arrow keypad */
            case OInput::State::KeyDelete: return "DELETE"; /* Delete on arrow keypad */
            case OInput::State::KeyLeftWindows: return "LEFT OS"; /* Left Windows key */
            case OInput::State::KeyRightWindows: return "RIGHT OS"; /* Right Windows key */
            case OInput::State::KeyAppMenu: return "APP MENU"; /* AppMenu key */
            case OInput::State::KeyPower: return "Power"; /* System Power */
            case OInput::State::KeySleep: return "Sleep"; /* System Sleep */
            case OInput::State::KeyWake: return "Wake"; /* System Wake */
            case OInput::State::KeyWebSearch: return "Web Search"; /* Web Search */
            case OInput::State::KeyWebFavorites: return "Web Favorites"; /* Web Favorites */
            case OInput::State::KeyWebRefresh: return "Web Refresh"; /* Web Refresh */
            case OInput::State::KeyWebStop: return "Web Stop"; /* Web Stop */
            case OInput::State::KeyWebForward: return "Web Forward"; /* Web Forward */
            case OInput::State::KeyWebBack: return "Web Back"; /* Web Back */
            case OInput::State::KeyMyComputer: return "My Computer"; /* My Computer */
            case OInput::State::KeyMailL: return "Mail"; /* Mail */
            case OInput::State::KeyMediaSelect: return "Media Select"; /* Media Select */
            case OInput::State::Mouse1: return "Left Mouse";
            case OInput::State::Mouse2: return "Right Mouse";
            case OInput::State::Mouse3: return "Middle Mouse";
            case OInput::State::Mouse4: return "Mouse 4";
        }
        return "UNKNOWN";
    }
    else if (button != OGamePad::Button::None)
    {
        switch (button)
        {
            case OGamePadA: return "GamePad A";
            case OGamePadB: return "GamePad B";
            case OGamePadX: return "GamePad X";
            case OGamePadY: return "GamePad Y";
            case OGamePadDPadUp: return "DPad Up";
            case OGamePadDPadDown: return "DPad Down";
            case OGamePadDPadLeft: return "DPad Left";
            case OGamePadDPadRight: return "DPad Right";
            case OGamePadLeftTrigger: return "LT";
            case OGamePadLeftBumper: return "LB";
            case OGamePadRightTrigger: return "RT";
            case OGamePadRightBumper: return "RB";
            case OGamePadLeftThumb: return "Left Stick";
            case OGamePadRightThumb: return "Right Stick";
            case OGamePadStart: return "Start";
            case OGamePadBack: return "Back";
            case OGamePadLeftThumbLeft: return "LStick Left";
            case OGamePadLeftThumbRight: return "LStick Right";
            case OGamePadLeftThumbUp: return "LStick Up";
            case OGamePadLeftThumbDown: return "LStick Down";
            case OGamePadRightThumbLeft: return "RStick Left";
            case OGamePadRightThumbRight: return "RStick Right";
            case OGamePadRightThumbUp: return "RStick Up";
            case OGamePadRightThumbDown: return "RStick Down";
        }
        return "UNKNOWN";
    }
    return "";
}


std::string Input::to_setting_string() const
{
    if (key != OInput::State::None)
    {
        return "key:" + std::to_string((int)key);
    }
    else if (button != OGamePad::Button::None)
    {
        return "gamepad:" + std::to_string((int)button);
    }
    return "none";
}


void Input::from_setting_string(const std::string& value)
{
    key = OInput::State::None;
    button = OGamePad::Button::None;

    auto splits = onut::splitString(value, ":");
    if (splits[0] == "key")
    {
        try
        {
            key = (OInput::State)std::stoi(splits[1]);
        }
        catch (...)
        {
            key = OInput::State::None;
        }
    }
    else if (splits[0] == "gamepad")
    {
        try
        {
            button = (OGamePad::Button)std::stoi(splits[1]);
        }
        catch (...)
        {
            button = OGamePad::Button::None;
        }
    }
}


bool InputAction::is_activated() const
{
    for (int i = 0; i < INPUT_STATE_COUNT; ++i)
    {
        if (!states[i].is_bound()) continue;
        if (modifiers[i].is_bound())
        {
            if (modifiers[i].is_pressed() && states[i].is_just_pressed())
                return true;
        }
        if (states[i].is_pressed()) return true;
    }

    return false;
}


void InputAction::bind(int index, Input modifier, Input state)
{
    modifiers[index] = modifier;
    states[index] = state;
}


std::string InputAction::get_modifier_name(int i) const
{
    if (!modifiers[i].is_bound()) return "- NONE -";

    std::string ret = modifiers[i].get_name();
    std::transform(ret.begin(), ret.end(), ret.begin(), toupper);
    return ret;
}


std::string InputAction::get_state_name(int i) const
{
    if (!states[i].is_bound()) return "- NONE -";

    std::string ret = states[i].get_name();
    std::transform(ret.begin(), ret.end(), ret.begin(), toupper);
    return ret;
}


std::string InputAction::to_setting_value(int i) const
{
    if (modifiers[i].is_bound())
    {
        return modifiers[i].to_setting_string() + "+" + states[i].to_setting_string();
    }
    return states[i].to_setting_string();
}


void InputAction::from_setting_value(int i, const std::string& value)
{
    auto split_plus = onut::splitString(value, "+");
    if (split_plus.size() == 2)
    {
        modifiers[i].from_setting_string(split_plus[0]);
        states[i].from_setting_string(split_plus[1]);
    }
    else
    {
        modifiers[i] = Input::None;
        states[i].from_setting_string(split_plus[0]);
    }
}


InputAction InputAction::filter(const std::vector<InputAction>& filters)
{
    InputAction filtered = *this;

    for (const auto& filter : filters)
    {
        for (int i = 0; i < INPUT_STATE_COUNT; ++i)
        {
            if (filtered.states[i] == Input::None) continue;
            for (int j = 0; j < INPUT_STATE_COUNT; ++j)
            {
                if (filter.states[j] == Input::None) continue;
                if (filter.states[j] == filtered.states[i])
                {
                    filtered.modifiers[i] = Input::None;
                    filtered.states[i] = Input::None;
                    break;
                }
            }
        }
    }

    return filtered;
}
