#include "GameplayInputContext.h"

#include <onut/Settings.h>


GameplayInputContext::GameplayInputContext()
{
    set_default_bindings();

    // Load settings
    for (int i = 0; i < INPUT_STATE_COUNT; ++i)
    {
        left.from_setting_value(i, oSettings->getUserSetting("bind_left" + std::to_string(i)));
        right.from_setting_value(i, oSettings->getUserSetting("bind_right" + std::to_string(i)));
        up.from_setting_value(i, oSettings->getUserSetting("bind_up" + std::to_string(i)));
        down.from_setting_value(i, oSettings->getUserSetting("bind_down" + std::to_string(i)));
        inventory.from_setting_value(i, oSettings->getUserSetting("bind_inventory" + std::to_string(i)));
        pause.from_setting_value(i, oSettings->getUserSetting("bind_pause" + std::to_string(i)));
        jump.from_setting_value(i, oSettings->getUserSetting("bind_jump" + std::to_string(i)));
        attack.from_setting_value(i, oSettings->getUserSetting("bind_attack" + std::to_string(i)));
        magic.from_setting_value(i, oSettings->getUserSetting("bind_magic" + std::to_string(i)));
        use_item.from_setting_value(i, oSettings->getUserSetting("bind_use_item" + std::to_string(i)));
    }
}


void GameplayInputContext::set_default_bindings()
{
    for (int i = 0; i < INPUT_STATE_COUNT; ++i)
    {
        left.bind(i, Input::None, Input::None);
        right.bind(i, Input::None, Input::None);
        up.bind(i, Input::None, Input::None);
        down.bind(i, Input::None, Input::None);
        inventory.bind(i, Input::None, Input::None);
        pause.bind(i, Input::None, Input::None);
        jump.bind(i, Input::None, Input::None);
        attack.bind(i, Input::None, Input::None);
        magic.bind(i, Input::None, Input::None);
        use_item.bind(i, Input::None, Input::None);
    }

    left.bind(0, Input::None, OKeyA);
    left.bind(1, Input::None, OGamePadDPadLeft);
    left.bind(2, Input::None, OGamePadLeftThumbLeft);

    right.bind(0, Input::None, OKeyD);
    right.bind(1, Input::None, OGamePadDPadRight);
    right.bind(2, Input::None, OGamePadLeftThumbRight);

    up.bind(0, Input::None, OKeyW);
    up.bind(1, Input::None, OGamePadDPadUp);
    up.bind(2, Input::None, OGamePadLeftThumbUp);

    down.bind(0, Input::None, OKeyS);
    down.bind(1, Input::None, OGamePadDPadDown);
    down.bind(2, Input::None, OGamePadLeftThumbDown);

    inventory.bind(0, Input::None, OKeyTab);
    inventory.bind(1, Input::None, OGamePadBack);

    pause.bind(0, Input::None, OKeyEscape);
    pause.bind(1, Input::None, OGamePadStart);

    jump.bind(0, Input::None, OKeySpaceBar);
    jump.bind(1, Input::None, OGamePadA);

    attack.bind(0, Input::None, OMouse1);
    attack.bind(1, Input::None, OGamePadX);

    magic.bind(0, Input::None, OMouse2);
    magic.bind(1, OGamePadDPadUp, OGamePadX);
    magic.bind(2, OGamePadLeftThumbUp, OGamePadX);

    use_item.bind(0, Input::None, OKeyE);
    use_item.bind(1, OGamePadDPadDown, OGamePadX);
    use_item.bind(2, OGamePadLeftThumbDown, OGamePadX);

    for (int i = 0; i < INPUT_STATE_COUNT; ++i)
    {
        oSettings->setUserSettingDefault("bind_left" + std::to_string(i), left.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_right" + std::to_string(i), right.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_up" + std::to_string(i), up.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_down" + std::to_string(i), down.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_inventory" + std::to_string(i), inventory.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_pause" + std::to_string(i), pause.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_jump" + std::to_string(i), jump.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_attack" + std::to_string(i), attack.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_magic" + std::to_string(i), magic.to_setting_value(i));
        oSettings->setUserSettingDefault("bind_use_item" + std::to_string(i), use_item.to_setting_value(i));
    }
}


void GameplayInputContext::save_settings()
{
    for (int i = 0; i < INPUT_STATE_COUNT; ++i)
    {
        oSettings->setUserSetting("bind_left" + std::to_string(i), left.to_setting_value(i));
        oSettings->setUserSetting("bind_right" + std::to_string(i), right.to_setting_value(i));
        oSettings->setUserSetting("bind_up" + std::to_string(i), up.to_setting_value(i));
        oSettings->setUserSetting("bind_down" + std::to_string(i), down.to_setting_value(i));
        oSettings->setUserSetting("bind_inventory" + std::to_string(i), inventory.to_setting_value(i));
        oSettings->setUserSetting("bind_pause" + std::to_string(i), pause.to_setting_value(i));
        oSettings->setUserSetting("bind_jump" + std::to_string(i), jump.to_setting_value(i));
        oSettings->setUserSetting("bind_attack" + std::to_string(i), attack.to_setting_value(i));
        oSettings->setUserSetting("bind_magic" + std::to_string(i), magic.to_setting_value(i));
        oSettings->setUserSetting("bind_use_item" + std::to_string(i), use_item.to_setting_value(i));
    }
}


inputs_t GameplayInputContext::read_inputs(int controller_id)
{
    inputs_t inputs;

    if (controller_id == 0)
    {
        inputs.left = left.is_activated();
        inputs.right = right.is_activated();
        inputs.up = up.is_activated();
        inputs.down = down.is_activated();
        inputs.select = inventory.is_activated();
        inputs.start = pause.is_activated();
        inputs.a = jump.is_activated();
        inputs.b = attack.is_activated();
    }

    //if (controller_id == 1)
    //{
    //    inputs.a = magic.is_activated();
    //    inputs.b = use_item.is_activated();
    //}

    return inputs;
}
