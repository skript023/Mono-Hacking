#include "hooking.hpp"
#include "script_mgr.hpp"

namespace big
{
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

            return ret;
        } EXCEPT_CLAUSE
	}
}