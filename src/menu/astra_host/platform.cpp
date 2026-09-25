#include <astra/host/canvas.hpp>
#include "pointers.hpp"
#include "utility/unity.hpp"

namespace big
{
	bool canvas::platform_is_key_pressed(int key)
	{
		return unity::is_key_pressed(key);
	}
	bool canvas::platform_is_controller_pressed(int button)
	{
		return unity::is_controller_pressed(button);
	}
	Vector2 canvas::platform_resolution()
	{
		return {static_cast<float>(g_pointers->m_resolution.x), static_cast<float>(g_pointers->m_resolution.y)};
	}
	void canvas::platform_prepare_render()
	{
		astra::set_srgb_output(false);
	}
}