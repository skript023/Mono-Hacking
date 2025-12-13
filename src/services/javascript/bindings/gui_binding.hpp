#pragma once
#include "../quickjspp.hpp"

namespace big
{
	struct JSOption
	{
		std::string type; // "bool", "slider", "option", "submenu", "choose"
		std::string text;
		std::string desc;
		std::string tag;

		double min = 0, max = 0, step = 1;
		int precision = 0;
		uint32_t subId = 0;

		 bool isInt = false;

		JSValue callback = JS_UNDEFINED; // fungsi JS
		std::vector<std::string> values;
	};

	struct JSMenu
	{
		std::string type; // "tabmenu" | "submenu"
		std::string title;
		uint32_t id;
		std::vector<JSOption> options{};

		uint64_t build_version = 0;    // naik setiap add_tab dipanggil
		uint64_t rendered_version = 0; // versi terakhir yg dirender
	};

	inline std::unordered_map<uint32_t, JSMenu> g_js_menus;

	class js_gui
	{
	public:
		static void bind(qjs::Context& context);
		static void register_gui();

	};
}