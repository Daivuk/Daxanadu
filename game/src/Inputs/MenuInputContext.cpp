#include "MenuInputContext.h"
#include "GameplayInputContext.h"


MenuInputContext::MenuInputContext(GameplayInputContext* gameplay_input_context)
    : m_gameplay_input_context(gameplay_input_context)
{
    // Set default bindings
    nav_left.bind(0, Input::None, OKeyLeft);
    nav_left.bind(1, Input::None, OKeyA);
    nav_left.bind(2, Input::None, OGamePadDPadLeft);
    nav_left.bind(3, Input::None, OGamePadLeftThumbLeft);

    nav_right.bind(0, Input::None, OKeyRight);
    nav_right.bind(1, Input::None, OKeyD);
    nav_right.bind(2, Input::None, OGamePadDPadRight);
    nav_right.bind(3, Input::None, OGamePadLeftThumbRight);

    nav_up.bind(0, Input::None, OKeyUp);
    nav_up.bind(1, Input::None, OKeyW);
    nav_up.bind(2, Input::None, OGamePadDPadUp);
    nav_up.bind(3, Input::None, OGamePadLeftThumbUp);

    nav_down.bind(0, Input::None, OKeyDown);
    nav_down.bind(1, Input::None, OKeyS);
    nav_down.bind(2, Input::None, OGamePadDPadDown);
    nav_down.bind(3, Input::None, OGamePadLeftThumbDown);

    nav_forward.bind(0, Input::None, OKeyEnter);
    nav_forward.bind(1, Input::None, OKeySpaceBar);
    nav_forward.bind(2, Input::None, OGamePadA);
    nav_forward.bind(3, Input::None, OGamePadStart);
    //nav_forward.bind(4, Input::None, OMouse1);

    nav_backward.bind(0, Input::None, OKeyEscape);
    nav_backward.bind(1, Input::None, OKeyBackspace);
    nav_backward.bind(2, Input::None, OGamePadB);
    nav_backward.bind(4, Input::None, OMouse2);
}


inputs_t MenuInputContext::read_inputs(int controller_id)
{
    inputs_t inputs;

    if (controller_id == 0)
    {
        const std::vector<InputAction> filters = {
            m_gameplay_input_context->left,
            m_gameplay_input_context->right,
            m_gameplay_input_context->up,
            m_gameplay_input_context->down,
            m_gameplay_input_context->jump,
            m_gameplay_input_context->attack
        };

        // Make sure gameplay input doesn't override some of our inputs first.
        // If the user for example uses ESDF for movement, they will expect the in-game menus to use the same inputs.
        // But S is hardcoded here for "down". We need to filter it out, and it is now used for "left".
        InputAction filtered_nav_left = nav_left.filter(filters);
        InputAction filtered_nav_right = nav_right.filter(filters);
        InputAction filtered_nav_up = nav_up.filter(filters);
        InputAction filtered_nav_down = nav_down.filter(filters);
        InputAction filtered_nav_a = nav_forward;//.filter(filters);
        InputAction filtered_nav_b = nav_backward;//.filter(filters);

        inputs.left = filtered_nav_left.is_activated() || m_gameplay_input_context->left.is_activated();
        inputs.right = filtered_nav_right.is_activated() || m_gameplay_input_context->right.is_activated();
        inputs.up = filtered_nav_up.is_activated() || m_gameplay_input_context->up.is_activated();
        inputs.down = filtered_nav_down.is_activated() || m_gameplay_input_context->down.is_activated();
        inputs.select = false;
        inputs.start = false;
        inputs.a = filtered_nav_a.is_activated() || m_gameplay_input_context->jump.is_activated();
        inputs.b = filtered_nav_b.is_activated() || m_gameplay_input_context->attack.is_activated();
    }

    return inputs;
}
