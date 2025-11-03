#include "imgui.h"
#include "script.hpp"
#include "../view.hpp"

namespace big
{
    void view::stats_submenu()
    {
        canvas::add_tab<regular_submenu>("Stats", SubmenuStats, [](regular_submenu* sub)
        {
            
        });
    }
}