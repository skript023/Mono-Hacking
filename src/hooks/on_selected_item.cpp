#include "hooking.hpp"
#include <script_mgr.hpp>
#include "unity/item_drop.hpp"
#include "commands/int_command.hpp"
#include "commands/float_command.hpp"
#include "commands/number_command.hpp"

namespace big
{
    number_command<float> _durability("durability", "Item Durability", "", 1.f, 10000.f, 100.f);
    number_command<int> _quality("quality", "Item Quality", "", 1, 100, 1);
    number_command<int> _stack("stack", "Item Stack", "", 1, 100, 1);
    number_command<int> _variant("variant", "Item Variant", "", 1, 100, 0);

	void hooks::on_selected_item(void* this_ptr, void* grid_ptr, MonoObject* item_data_obj, iVector2 pos, int mod)
	{
        if (item_data_obj != nullptr)
        {
            item_data data(item_data_obj);
            
            data.set_durability(_durability.get_state());
            data.set_quality(_quality.get_state());
            data.set_stack(_stack.get_state());
            data.set_variant(_variant.get_state());
        }

		return detour_base::get_original<on_selected_item>()(this_ptr, grid_ptr, item_data_obj, pos, mod);
	}
}