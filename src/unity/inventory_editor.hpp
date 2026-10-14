#pragma once

namespace big
{
	class inventory_editor
	{
	public:
		static void draw_window();
		static void toggle_window();

	private:
		static void draw_window_impl();
	};
}
