#pragma once

#include "InputContext.h"


class GameplayInputContext final : public InputContext
{
public:
    GameplayInputContext();

    inputs_t read_inputs(int controller_id) override;
    void set_default_bindings();
    void save_settings();

    InputAction left;
    InputAction right;
    InputAction up;
    InputAction down;
    InputAction inventory;
    InputAction pause;
    InputAction jump;
    InputAction attack;
    InputAction magic;
    InputAction use_item;
};
