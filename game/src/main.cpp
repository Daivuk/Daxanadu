#include <onut/onut.h>
#include <onut/Settings.h>
#include <onut/Timing.h>


void initSettings()
{
    oSettings->setGameName("Daxanadu");
    oSettings->setResolution({1600, 900});
    oSettings->setIsFixedStep(true);
    oSettings->setShowFPS(true);
}


void init()
{
    oTiming->setUpdateFps(59.94006);
}


void shutdown()
{
}


void update()
{
}


void render()
{
}


void postRender()
{
}


void renderUI()
{
}
