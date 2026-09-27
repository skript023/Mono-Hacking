#include "hooking.hpp"
#include "utility/unity.hpp"
#include "notification/notification_service.hpp"

namespace big
{
	void hooks::on_map_middle_click(MonoObject* minimap, MonoObject* handler)
	{
		detour_base::get_original<on_map_middle_click>()(minimap, handler);

		if (g_settings.self.map_click_teleport && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
		{
			static MonoMethod* get_pos = mono::get_method("ZInput", "get_pointerPosition", 0, "assembly_utils");
			static MonoMethod* screen_to_world = mono::get_method("Minimap", "ScreenToWorldPoint", 1, "assembly_valheim");

			if (get_pos && screen_to_world)
			{
				MonoObject* mouse_obj = mono::invoke_method(get_pos, nullptr, nullptr);
				if (mouse_obj)
				{
					Vector3 mouse_pos = *reinterpret_cast<Vector3*>(mono::object_unbox(mouse_obj));
					void* args[1] = {&mouse_pos};
					MonoObject* world_obj = mono::invoke_method(screen_to_world, minimap, args);
					if (world_obj)
					{
						Vector3 world_pos = *reinterpret_cast<Vector3*>(mono::object_unbox(world_obj));
						unity::teleport_to_world_point(world_pos);
						notification::success("Teleport", "Teleported to map location.");
					}
				}
			}
		}
	}
}
