#include "gui_binding.hpp"
#include "menu/submenu.hpp"
#include "ui/store_mgr.hpp"

#pragma warning(disable : 4244)
namespace big
{
	JSContext* g_js_ctx;
	static JSClassID js_submenu_class_id;

	static void register_submenu_class(JSContext* ctx)
	{
		JS_NewClassID(&js_submenu_class_id);

		JSClassDef def{};
		def.class_name = "Submenu";

		JS_NewClass(JS_GetRuntime(ctx), js_submenu_class_id, &def);
	}

	static bool js_is_int(JSContext* ctx, JSValueConst v)
	{
		double d;
		if (JS_ToFloat64(ctx, &d, v))
			return false;

		return std::floor(d) == d && d >= std::numeric_limits<int32_t>::min() && d <= std::numeric_limits<int32_t>::max();
	}
	
	static JSValue js_sub_add_bool(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* menu = (JSMenu*)JS_GetOpaque2(ctx, this_val, js_submenu_class_id);
		if (!menu)
			return JS_EXCEPTION;

		if (argc < 1)
			return JS_ThrowTypeError(ctx, "add_bool(text, desc?)");

		JSOption opt{};
		opt.type = "bool";
		opt.text = JS_ToCString(ctx, argv[0]);
		opt.tag = opt.text;

		if (argc >= 2 && JS_IsString(argv[1]))
			opt.desc = JS_ToCString(ctx, argv[1]);

		menu->options.push_back(std::move(opt));
		return JS_UNDEFINED;
	}
	static JSValue js_sub_add_slider(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* menu = (JSMenu*)JS_GetOpaque2(ctx, this_val, js_submenu_class_id);
		if (!menu)
			return JS_EXCEPTION;

		if (argc < 4)
			return JS_ThrowTypeError(ctx, "add_slider(text, min, max, step)");

		JSOption opt{};
		opt.type = "slider";
		opt.text = JS_ToCString(ctx, argv[0]);
		opt.tag = opt.text;

		JS_ToFloat64(ctx, &opt.min, argv[1]);
		JS_ToFloat64(ctx, &opt.max, argv[2]);
		JS_ToFloat64(ctx, &opt.step, argv[3]);

		opt.isInt = js_is_int(ctx, argv[1]) && js_is_int(ctx, argv[2]) && js_is_int(ctx, argv[3]);

		menu->options.push_back(std::move(opt));
		return JS_UNDEFINED;
	}
	static JSValue js_sub_add_choose(
	    JSContext* ctx,
	    JSValueConst this_val,
	    int argc,
	    JSValueConst* argv)
	{
		auto* menu = (JSMenu*)JS_GetOpaque2(ctx, this_val, js_submenu_class_id);
		if (!menu)
			return JS_EXCEPTION;

		if (argc < 3 || !JS_IsArray(ctx, argv[1]) || !JS_IsFunction(ctx, argv[2]))
			return JS_ThrowTypeError(ctx, "add_choose(text, values[], callback)");

		JSOption opt{};
		opt.type = "choose";
		opt.text = JS_ToCString(ctx, argv[0]);
		opt.tag = opt.text;
		opt.callback = JS_DupValue(ctx, argv[2]);

		uint32_t len;
		JS_ToUint32(ctx, &len, JS_GetPropertyStr(ctx, argv[1], "length"));

		for (uint32_t i = 0; i < len; i++)
		{
			JSValue v = JS_GetPropertyUint32(ctx, argv[1], i);
			opt.values.emplace_back(JS_ToCString(ctx, v));
			JS_FreeValue(ctx, v);
		}

		menu->options.push_back(std::move(opt));
		return JS_UNDEFINED;
	}
	static JSValue js_sub_add_option(
	    JSContext* ctx,
	    JSValueConst this_val,
	    int argc,
	    JSValueConst* argv)
	{
		auto* menu = (JSMenu*)JS_GetOpaque2(ctx, this_val, js_submenu_class_id);
		if (!menu)
			return JS_EXCEPTION;

		JSOption opt{};
		opt.type = "option";
		opt.text = JS_ToCString(ctx, argv[0]);
		opt.tag = opt.text;

		if (argc >= 2 && JS_IsFunction(ctx, argv[1]))
			opt.callback = JS_DupValue(ctx, argv[1]);

		menu->options.push_back(std::move(opt));
		return JS_UNDEFINED;
	}
	static JSValue js_sub_add_submenu(
	    JSContext* ctx,
	    JSValueConst this_val,
	    int argc,
	    JSValueConst* argv)
	{
		auto* menu = (JSMenu*)JS_GetOpaque(this_val, 0);
		if (!menu)
			return JS_EXCEPTION;

		// add_submenu(text, subId, callback?)
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "add_submenu(text, subId, callback?)");

		JSOption opt{};
		opt.type = "submenu";
		opt.text = JS_ToCString(ctx, argv[0]);
		JS_ToUint32(ctx, &opt.subId, argv[1]);
		opt.tag = opt.text;

		if (argc >= 3 && JS_IsFunction(ctx, argv[2]))
			opt.callback = JS_DupValue(ctx, argv[2]);

		menu->options.push_back(std::move(opt));
		return JS_UNDEFINED;
	}
	static JSValue js_gui_add_tab(JSContext* ctx, JSValueConst,
	    int argc, JSValueConst* argv)
	{
		if (argc < 3 || !JS_IsFunction(ctx, argv[2]))
			return JS_ThrowTypeError(ctx, "add_tab(title, id, callback)");

		const char* title = JS_ToCString(ctx, argv[0]);
		uint32_t id;
		JS_ToUint32(ctx, &id, argv[1]);

		JSMenu& menu = g_js_menus[id];
		menu.type = "tabmenu";
		menu.title = title;
		menu.id = id;

		menu.options.clear();
		menu.build_version++;

		JSValue sub = JS_NewObjectClass(ctx, js_submenu_class_id);
		JS_SetOpaque(sub, &menu);

		JS_SetPropertyStr(ctx, sub, "add_bool", JS_NewCFunction(ctx, js_sub_add_bool, "add_bool", 2));
		JS_SetPropertyStr(ctx, sub, "add_slider", JS_NewCFunction(ctx, js_sub_add_slider, "add_slider", 4));
		JS_SetPropertyStr(ctx, sub, "add_choose", JS_NewCFunction(ctx, js_sub_add_choose, "add_choose", 3));
		JS_SetPropertyStr(ctx, sub, "add_option", JS_NewCFunction(ctx, js_sub_add_option, "add_option", 2));

		JS_Call(ctx, argv[2], JS_UNDEFINED, 1, &sub);
		JS_FreeCString(ctx, title);
		return JS_UNDEFINED;
	}

	static JSValue js_gui_add_submenu(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (argc < 3 || !JS_IsFunction(ctx, argv[2]))
			return JS_ThrowTypeError(ctx, "submenu(title, id, callback)");

		const char* title = JS_ToCString(ctx, argv[0]);
		uint32_t id;
		JS_ToUint32(ctx, &id, argv[1]);

		JSMenu& menu = g_js_menus[id];
		menu.type = "submenu";
		menu.title = title;
		menu.id = id;

		JSValue sub = JS_NewObjectClass(ctx, js_submenu_class_id);
		JS_SetOpaque(sub, &menu);

		JS_SetPropertyStr(ctx, sub, "add_bool", JS_NewCFunction(ctx, js_sub_add_bool, "add_bool", 2));
		JS_SetPropertyStr(ctx, sub, "add_slider", JS_NewCFunction(ctx, js_sub_add_slider, "add_slider", 4));
		JS_SetPropertyStr(ctx, sub, "add_choose", JS_NewCFunction(ctx, js_sub_add_choose, "add_choose", 3));
		JS_SetPropertyStr(ctx, sub, "add_option", JS_NewCFunction(ctx, js_sub_add_option, "add_option", 2));

		JS_Call(ctx, argv[2], JS_UNDEFINED, 1, &sub);
		JS_FreeCString(ctx, title);
		return JS_UNDEFINED;
	}
	void js_gui::register_gui()
	{
		static std::unordered_map<std::string, bool> boolStates;
		static std::unordered_map<std::string, float> floatStates;
		static std::unordered_map<std::string, int> intStates;
		static std::unordered_map<std::string, int> choosePositions;

		for (auto& [id, menu] : g_js_menus)
		{
			if (menu.rendered_version == menu.build_version)
				continue;

			auto render = [&menu](regular_submenu* sub) {
				for (auto& opt : menu.options)
				{
					if (opt.type == "bool")
					{
						if (!boolStates.count(opt.tag))
							boolStates[opt.tag] = store::get_bool(opt.tag);

						sub->add_option<bool_option<bool>>(
						    opt.text.c_str(),
						    opt.desc.c_str(),
						    &boolStates[opt.tag],
						    [tag = opt.tag]() {
							    store::set_bool(tag, boolStates[tag]);
						    });
					}
					else if (opt.type == "slider")
					{
						if (!floatStates.count(opt.tag))
							floatStates[opt.tag] = store::get_float(opt.tag);

						if (opt.isInt)
						{
							sub->add_option<bool_slider_int_option>(
								opt.text.c_str(),
								opt.desc.c_str(),
								&boolStates[opt.tag],
								&intStates[opt.tag],
								(int)opt.min,
								(int)opt.max,
							    (int)opt.step,
								1,
								true,
							    [ctx = g_js_ctx, cb = opt.callback, tag = opt.tag]() {
								    auto data = store::get_float(tag, floatStates[tag]);
								    if (JS_IsUndefined(cb))
									    return;

								    JSValue args[1];
								    args[0] = JS_NewInt32(ctx, data);

								    JS_Call(ctx, cb, JS_UNDEFINED, 1, args);

								    JS_FreeValue(ctx, args[0]);
							    });
						}
						else
						{
							sub->add_option<bool_slider_float_option>(
							    opt.text.c_str(),
							    opt.desc.c_str(),
							    &boolStates[opt.tag],
							    &floatStates[opt.tag],
							    opt.min,
							    opt.max,
							    opt.step,
							    opt.precision,
							    true,
							    [ctx = g_js_ctx, cb = opt.callback, tag = opt.tag]() {
								    auto data = store::get_float(tag, floatStates[tag]);
								    if (JS_IsUndefined(cb))
									    return;

								    JSValue args[1];
								    args[0] = JS_NewFloat64(ctx, data);

								    JS_Call(ctx, cb, JS_UNDEFINED, 1, args);

								    JS_FreeValue(ctx, args[0]);
							    });
						}
					}
					else if (opt.type == "choose")
					{
						if (!choosePositions.count(opt.tag))
							choosePositions[opt.tag] = 0;

						auto& pos = choosePositions[opt.tag];
						auto& vals = opt.values;

						if (vals.empty())
							continue;

						sub->add_option<choose_option<std::string, int>>(
						    opt.text.c_str(),
						    opt.desc.c_str(),
						    &vals,
						    &pos,
						    true,
						    [ctx = g_js_ctx, cb = opt.callback, &vals, &pos]() {
							    if (JS_IsUndefined(cb))
								    return;

							    JSValue args[2];
							    args[0] = JS_NewInt32(ctx, pos + 1);
							    args[1] = JS_NewString(ctx, vals[pos].c_str());

							    JS_Call(ctx, cb, JS_UNDEFINED, 2, args);

							    JS_FreeValue(ctx, args[0]);
							    JS_FreeValue(ctx, args[1]);
						    });
					}
					else if (opt.type == "option")
					{
						sub->add_option<reguler_option>(
						    opt.text.c_str(),
						    opt.desc.c_str(),
						    [ctx = g_js_ctx, cb = opt.callback]() {
							    if (JS_IsUndefined(cb))
								    return;
							    JS_Call(ctx, cb, JS_UNDEFINED, 0, nullptr);
						    });
					}
					else if (opt.type == "submenu")
					{
						sub->add_option<sub_option>(
						    opt.text.c_str(),
						    opt.desc.c_str(),
						    opt.subId,
						    [ctx = g_js_ctx, cb = opt.callback]() {
							    if (JS_IsUndefined(cb))
								    return;
							    JS_Call(ctx, cb, JS_UNDEFINED, 0, nullptr);
						    });
					}

				}
			};

			if (menu.type == "tabmenu")
				canvas::add_tab<regular_submenu>(menu.title.c_str(), menu.id, render);
			else
				canvas::add_submenu<regular_submenu>(menu.title.c_str(), menu.id, render);

			menu.rendered_version = menu.build_version;
		}
	}

	static int js_gui_module_init(JSContext* ctx, JSModuleDef* m)
	{
		JS_SetModuleExport(ctx, m, "add_tab", JS_NewCFunction(ctx, js_gui_add_tab, "add_tab", 3));

		JS_SetModuleExport(ctx, m, "add_submenu", JS_NewCFunction(ctx, js_gui_add_submenu, "add_submenu", 3));

		return 0; // WAJIB
	}

	static JSModuleDef* js_gui_init(JSContext* ctx, const char* module_name)
	{
		JSModuleDef* m = JS_NewCModule(ctx, module_name, js_gui_module_init);

		if (!m)
			return nullptr;

		JS_AddModuleExport(ctx, m, "add_tab");
		JS_AddModuleExport(ctx, m, "add_submenu");

		return m;
	}

	void js_gui::bind(qjs::Context& context)
	{
		auto ctx = context.ctx;
		g_js_ctx = ctx;

		register_submenu_class(ctx);
		JSModuleDef* m = js_gui_init(ctx, "canvas");

		if (!m)
			LOG(WARNING) << "Failed to init gui module";
	}
} // namespace js::gui