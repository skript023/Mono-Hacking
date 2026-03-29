#include "hooking.hpp"

namespace big
{
    
	void hooks::camera_render(MonoObject* Camera)
	{
        if (g_running)
        {
            g_settings.m_camera_obj = Camera;
        }

        return detour_base::get_original<camera_render>()(Camera);
	}
}