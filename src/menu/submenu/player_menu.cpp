#include "../view.hpp"
#include "script.hpp"

namespace big
{
    void view::player_submenu()
    {
        canvas::add_tab<regular_submenu>("Player", SubmenuPlayer, [](regular_submenu* sub)
        {
            
        });
    }
}