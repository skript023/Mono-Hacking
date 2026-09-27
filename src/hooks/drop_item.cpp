#include "hooking.hpp"
#include "script_mgr.hpp"
#include <commands/bool_command.hpp>
#include <commands/int_command.hpp>

namespace big
{
	bool_command _enable_override_drop("override_drop", "Override Drop", "Override drop item value.", false);
	int_command _drop_amount("drop_amount", "", "", 1, 100, 1);

	MonoObject* hooks::drop_item(MonoObject* item, int amount, Vector3 position, Quaternions rotation)
	{
		TRY_CLAUSE
		{
			item_data itm(item);
			itm.set_cheated(false);

			auto ret = detour_base::get_original<hooks::drop_item>()(
			    item,
			    amount,
			    position,
			    rotation);

			if (ret)
			{
				item_drop dropped(ret);
				auto data = dropped.get_data();
				data.set_cheated(false);

				if (_enable_override_drop.get_state())
				{
					dropped.set_stack(_drop_amount.get_state());
				}

				dropped.save();
			}

			return ret;
		}
		EXCEPT_CLAUSE
	}

	void hooks::drop_invalid_items(MonoObject* humanoid)
	{
		// Block Valheim from dropping out-of-bounds inventory items into the world on logout or size reduction
		return;
	}
}
