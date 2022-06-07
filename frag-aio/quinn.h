#pragma once
#include "../plugin_sdk/plugin_sdk.hpp"

namespace quinn
{
    void load();
    void unload();
    void farm();
    void combo();
    void harass();
    void on_update();
    void on_draw();
    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);
};
