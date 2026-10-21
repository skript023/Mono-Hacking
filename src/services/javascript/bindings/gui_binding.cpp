#include "gui_binding.hpp"

#include "menu/submenu.hpp"

#include <astra/host/menu/store_mgr.hpp>

#pragma warning(disable : 4244)
namespace big
{
	qjs::Context* g_js_context = nullptr;

	static void invoke_js_callback(const std::optional<qjs::Value>& callback, std::vector<qjs::Value> args = {})
	{
		if (!callback || !callback->isFunction())
		{
			return;
		}
		try
		{
			callback->call(args);
		}
		catch (const qjs::exception&)
		{
			auto error = qjs::Context::get(callback->ctx).getException();
			LOG(WARNING) << "[JS GUI Error] " << error.as<std::string>();
		}
	}

	void JSMenuBuilder::add_bool(std::string text, qjs::Value description)
	{
		JSOption opt{};
		opt.type = "bool";
		opt.text = std::move(text);
		opt.tag  = opt.text;
		if (description.isString())
		{
			opt.desc = description.as<std::string>();
		}
		g_js_menus.at(m_menu_id).options.push_back(std::move(opt));
	}

	void JSMenuBuilder::add_slider(std::string text, double min, double max, double step)
	{
		JSOption opt{};
		opt.type  = "slider";
		opt.text  = std::move(text);
		opt.tag   = opt.text;
		opt.min   = min;
		opt.max   = max;
		opt.step  = step;
		opt.isInt = std::floor(min) == min && std::floor(max) == max && std::floor(step) == step;
		g_js_menus.at(m_menu_id).options.push_back(std::move(opt));
	}

	void JSMenuBuilder::add_choose(std::string text, std::vector<std::string> values, qjs::Value callback)
	{
		if (!callback.isFunction())
		{
			throw std::runtime_error("choose callback must be a function");
		}
		JSOption opt{};
		opt.type = "choose";
		opt.text = std::move(text);
		opt.tag  = opt.text;
		opt.callback.emplace(std::move(callback));
		opt.values = std::move(values);
		g_js_menus.at(m_menu_id).options.push_back(std::move(opt));
	}

	void JSMenuBuilder::add_option(std::string text, qjs::Value callback)
	{
		JSOption opt{};
		opt.type = "option";
		opt.text = std::move(text);
		opt.tag  = opt.text;
		if (callback.isFunction())
		{
			opt.callback.emplace(std::move(callback));
		}
		g_js_menus.at(m_menu_id).options.push_back(std::move(opt));
	}

	void JSMenuBuilder::add_submenu(std::string text, uint32_t submenu_id, qjs::Value callback)
	{
		JSOption opt{};
		opt.type  = "submenu";
		opt.text  = std::move(text);
		opt.tag   = opt.text;
		opt.subId = submenu_id;
		if (callback.isFunction())
		{
			opt.callback.emplace(std::move(callback));
		}
		g_js_menus.at(m_menu_id).options.push_back(std::move(opt));
	}

	void JSCanvasBindings::add_tab(std::string title, uint32_t id, qjs::Value callback)
	{
		if (!callback.isFunction())
		{
			throw std::runtime_error("add_tab(title, id, callback): callback must be a function");
		}
		auto& menu = g_js_menus[id];
		menu.type  = "tabmenu";
		menu.title = std::move(title);
		menu.id    = id;
		menu.options.clear();
		++menu.build_version;
		auto builder = g_js_context->newValue(std::make_shared<JSMenuBuilder>(id));
		callback.call(std::vector<qjs::Value>{std::move(builder)});
	}

	void JSCanvasBindings::add_submenu(std::string title, uint32_t id, qjs::Value callback)
	{
		if (!callback.isFunction())
		{
			throw std::runtime_error("add_submenu(title, id, callback): callback must be a function");
		}
		auto& menu   = g_js_menus[id];
		menu.type    = "submenu";
		menu.title   = std::move(title);
		menu.id      = id;
		auto builder = g_js_context->newValue(std::make_shared<JSMenuBuilder>(id));
		callback.call(std::vector<qjs::Value>{std::move(builder)});
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
			{
				continue;
			}

			auto render = [&menu](regular_submenu* sub) {
				for (auto& opt : menu.options)
				{
					if (opt.type == "bool")
					{
						if (!boolStates.count(opt.tag))
						{
							boolStates[opt.tag] = store::get_bool(opt.tag);
						}

						sub->add_option<bool_option<bool>>(opt.text.c_str(),
						    opt.desc.c_str(),
						    &boolStates[opt.tag],
						    [tag = opt.tag]() {
							    store::set_bool(tag, boolStates[tag]);
						    });
					}
					else if (opt.type == "slider")
					{
						if (!floatStates.count(opt.tag))
						{
							floatStates[opt.tag] = store::get_float(opt.tag);
						}

						if (opt.isInt)
						{
							sub->add_option<bool_slider_int_option>(opt.text.c_str(),
							    opt.desc.c_str(),
							    &boolStates[opt.tag],
							    &intStates[opt.tag],
							    (int)opt.min,
							    (int)opt.max,
							    (int)opt.step,
							    1,
							    true,
							    [ctx = g_js_context, cb = opt.callback, tag = opt.tag]() {
								    auto data  = store::get_float(tag, floatStates[tag]);
								    auto value = ctx->newValue(static_cast<int>(data));
								    invoke_js_callback(cb, std::vector<qjs::Value>{std::move(value)});
							    });
						}
						else
						{
							sub->add_option<bool_slider_float_option>(opt.text.c_str(),
							    opt.desc.c_str(),
							    &boolStates[opt.tag],
							    &floatStates[opt.tag],
							    opt.min,
							    opt.max,
							    opt.step,
							    opt.precision,
							    true,
							    [ctx = g_js_context, cb = opt.callback, tag = opt.tag]() {
								    auto data  = store::get_float(tag, floatStates[tag]);
								    auto value = ctx->newValue(static_cast<double>(data));
								    invoke_js_callback(cb, std::vector<qjs::Value>{std::move(value)});
							    });
						}
					}
					else if (opt.type == "choose")
					{
						if (!choosePositions.count(opt.tag))
						{
							choosePositions[opt.tag] = 0;
						}

						auto& pos  = choosePositions[opt.tag];
						auto& vals = opt.values;

						if (vals.empty())
						{
							continue;
						}

						sub->add_option<choose_option<std::string, int>>(opt.text.c_str(),
						    opt.desc.c_str(),
						    &vals,
						    &pos,
						    true,
						    [ctx = g_js_context, cb = opt.callback, &vals, &pos]() {
							    auto index = ctx->newValue(static_cast<int>(pos + 1));
							    auto value = ctx->newValue(vals[pos]);
							    invoke_js_callback(cb, std::vector<qjs::Value>{std::move(index), std::move(value)});
						    });
					}
					else if (opt.type == "option")
					{
						sub->add_option<reguler_option>(opt.text.c_str(), opt.desc.c_str(), [cb = opt.callback]() {
							invoke_js_callback(cb);
						});
					}
					else if (opt.type == "submenu")
					{
						sub->add_option<sub_option>(opt.text.c_str(), opt.desc.c_str(), opt.subId, [cb = opt.callback]() {
							invoke_js_callback(cb);
						});
					}
				}
			};

			if (menu.type == "tabmenu")
			{
				canvas::add_tab<regular_submenu>(menu.title.c_str(), menu.id, render);
			}
			else
			{
				canvas::add_submenu<regular_submenu>(menu.title.c_str(), menu.id, render);
			}

			menu.rendered_version = menu.build_version;
		}
	}

	void js_gui::bind(qjs::Context& context)
	{
		g_js_context = &context;
		auto& module = context.addModule("canvas");
		module.class_<JSMenuBuilder>("MenuBuilder")
		    .fun<&JSMenuBuilder::add_bool>("add_bool")
		    .fun<&JSMenuBuilder::add_slider>("add_slider")
		    .fun<&JSMenuBuilder::add_choose>("add_choose")
		    .fun<&JSMenuBuilder::add_option>("add_option")
		    .fun<&JSMenuBuilder::add_submenu>("add_submenu");
		module.class_<JSCanvasBindings>("Canvas")
		    .constructor<>("Canvas")
		    .static_fun<&JSCanvasBindings::add_tab>("add_tab")
		    .static_fun<&JSCanvasBindings::add_submenu>("add_submenu");
		module.function("add_tab", &JSCanvasBindings::add_tab);
		module.function("add_submenu", &JSCanvasBindings::add_submenu);
	}

	void js_gui::shutdown()
	{
		g_js_menus.clear();
		g_js_context = nullptr;
	}
} // namespace js::gui
