#pragma once

#include <onut/GamePad.h>
#include <onut/Input.h>


static const int INPUT_STATE_COUNT = 8;


struct inputs_t
{
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool select = false;
    bool start = false;
    bool a = false;
    bool b = false;
};


class Input
{
public:
    static Input None;

    Input();
    Input(OInput::State in_key);
    Input(onut::GamePad::Button in_button);

    bool operator==(Input input) const { return key == input.key && button == input.button; }
    bool operator==(OInput::State in_key) const { return key == in_key; }
    bool operator==(onut::GamePad::Button in_button) const { return button == in_button; }

    // Can only be one or another
    OInput::State key = OInput::State::None;
    onut::GamePad::Button button = onut::GamePad::Button::None;

    bool is_bound() const;
    bool is_pressed() const;
    bool is_just_pressed() const;
    bool is_just_released() const;
    std::string get_name() const;
    std::string to_setting_string() const;
    void from_setting_string(const std::string& value);
};


class InputAction
{
public:
    Input modifiers[INPUT_STATE_COUNT];
    Input states[INPUT_STATE_COUNT];

    bool is_activated() const;
    void bind(int index, Input modifier, Input state);
    std::string get_modifier_name(int index) const;
    std::string get_state_name(int index) const;
    std::string to_setting_value(int index) const;
    void from_setting_value(int index, const std::string& value);
    InputAction filter(const std::vector<InputAction>& filters);
};


class InputContext
{
public:
    virtual ~InputContext() {}

    virtual inputs_t read_inputs(int controller_id) = 0;
};
