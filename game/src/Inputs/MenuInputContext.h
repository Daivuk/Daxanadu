#pragma once

#include "InputContext.h"


class GameplayInputContext;


class MenuInputContext final : public InputContext
{
public:
    MenuInputContext(GameplayInputContext* gameplay_input_context);

    inputs_t read_inputs(int controller_id) override;

    InputAction nav_up;
    InputAction nav_down;
    InputAction nav_left;
    InputAction nav_right;
    InputAction nav_forward;
    InputAction nav_backward;

private:
    GameplayInputContext* m_gameplay_input_context = nullptr;
};
