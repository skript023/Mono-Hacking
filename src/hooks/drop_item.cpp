#include "hooking.hpp"
#include "script_mgr.hpp"
#include <commands/bool_command.hpp>
#include <commands/int_command.hpp>

namespace big
{
    bool_command _enable_override_drop("override_drop", "Override Drop", "Override drop item value.", false);
    int_command _drop_amount("drop_amount", "", "", 100.f, 10000.f, 100.f);

	MonoObject* hooks::drop_item(MonoObject* item, int amount, Vector3 position, Quaternions rotation)
	{
        TRY_CLAUSE
        {
            item_data itm(item);

            LOG(INFO) << "Location: " << position.x << ", " << position.y << ", " << position.z;

            auto ret = detour_base::get_original<hooks::drop_item>()(
                item,
                amount,
                position,
                rotation
            );

            item_drop dropped(ret);

            if (_enable_override_drop.get_state())
            {
                dropped.set_stack(_drop_amount.get_state());
            }

            return ret;
        } EXCEPT_CLAUSE
	}
}