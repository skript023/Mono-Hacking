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
		int precision  = 0;
		uint32_t subId = 0;

		bool isInt = false;

		std::optional<qjs::Value> callback;
		std::vector<std::string> values;
	};

	struct JSMenu
	{
		std::string type; // "tabmenu" | "submenu"
		std::string title;
		uint32_t id;
		std::vector<JSOption> options{};

		uint64_t build_version    = 0; // naik setiap add_tab dipanggil
		uint64_t rendered_version = 0; // versi terakhir yg dirender
	};

	inline std::unordered_map<uint32_t, JSMenu> g_js_menus;
	class JSMenuBuilder
	{
	public:
		explicit JSMenuBuilder(uint32_t menu_id) :
		    m_menu_id(menu_id)
		{
		}

		void add_bool(std::string text, qjs::Value description);
		void add_slider(std::string text, double min, double max, double step);
		void add_choose(std::string text, std::vector<std::string> values, qjs::Value callback);
		void add_option(std::string text, qjs::Value callback);
		void add_submenu(std::string text, uint32_t submenu_id, qjs::Value callback);

	private:
		uint32_t m_menu_id;
	};

	class JSCanvasBindings
	{
	public:
		static void add_tab(std::string title, uint32_t id, qjs::Value callback);
		static void add_submenu(std::string title, uint32_t id, qjs::Value callback);
	};

	class js_gui
	{
	public:
		static void bind(qjs::Context& context);
		static void register_gui();
		static void shutdown();
	};
}
