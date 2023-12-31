#include "Daxanadu.h"
#include "version.h"

#include <onut/onut.h>
#include <onut/Files.h>
#include <onut/Settings.h>
#include <onut/Timing.h>


Daxanadu* daxanadu = nullptr;


void initSettings()
{
    printf("%s\n", DAX_VERSION_FULL_TEXT);

    oSettings->setGameName("Daxanadu");
    oSettings->setResolution({1400, 900});
    oSettings->setIsFixedStep(false);
    oSettings->setShowFPS(true);
    oSettings->setIsResizableWindow(true);
    oSettings->setStartMaximized(false);

    oSettings->setUserSettingDefault("dialog_speed", "0");
    oSettings->setUserSettingDefault("music_volume", "5");
    oSettings->setUserSettingDefault("sound_volume", "5");
    oSettings->setUserSettingDefault("npc_blinking", "0");
    oSettings->setUserSettingDefault("king_golds", "0");
    oSettings->setUserSettingDefault("coins_despawn", "0");
    oSettings->setUserSettingDefault("mist_quality", "0");
    oSettings->setUserSettingDefault("cigarettes", "0");
    oSettings->setUserSettingDefault("double_golds", "0");
    oSettings->setUserSettingDefault("double_xp", "0");
    oSettings->setUserSettingDefault("equip_in_shops", "0");
    oSettings->setUserSettingDefault("start_full_health", "0");
    oSettings->setUserSettingDefault("secret_items_counter", "0");
    oSettings->setUserSettingDefault("show_level_popup", "0");
    oSettings->setUserSettingDefault("show_tracker", "0");
    oSettings->setUserSettingDefault("keep_gold", "0");
    oSettings->setUserSettingDefault("keep_xp", "0");
    oSettings->setUserSettingDefault("xp_wingboots", "0");
    oSettings->setUserSettingDefault("xp_speed", "0");
    oSettings->setUserSettingDefault("pendant", "0");
    oSettings->setUserSettingDefault("ap_address", "archipelago.gg:38281");
    oSettings->setUserSettingDefault("ap_slot", "John Doe");
    oSettings->setUserSettingDefault("ap_password", "");
    oSettings->setUserSettingDefault("fast_cpu", "0");
}


void init()
{
    //oTiming->setUpdateFps(60.0988);

    daxanadu = new Daxanadu();
}


void shutdown()
{
    delete daxanadu;
    daxanadu = nullptr;
}


void update()
{
    daxanadu->update(ODT);
}


void render()
{
    daxanadu->render();
}


void postRender()
{
}


void renderUI()
{
}
