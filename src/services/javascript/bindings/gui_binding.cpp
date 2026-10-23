#include "gui_binding.hpp"

#include "menu/submenu.hpp"
#include "task_mgr.hpp"
#include "../js_lock.hpp"

#include <astra/host/menu/store_mgr.hpp>
#include <variant>

#pragma warning(disable : 4244)
namespace big
{
	qjs::Context* g_js_context = nullptr;

	namespace
	{
		using js_arg = std::variant<int, double, std::string>;

		struct option_view
		{
			std::string type;
			std::string text;
			std::string desc;
			std::string tag;
			double min{};
			double max{};
			double step{1};
			int precision{};
			uint32_t sub_id{};
			bool is_int{};
			std::vector<std::string> values;
		};

		struct menu_view
		{
			uint64_t version{};
			std::vector<option_view> options;
		};

		std::unordered_map<uint32_t, std::shared_ptr<const menu_view>> s_views;
		std::unordered_set<uint32_t> s_registered;
		std::unordered_map<std::string, bool> s_bool_states;
		std::unordered_map<std::string, float> s_float_states;
		std::unordered_map<std::string, int> s_int_states;
		std::unordered_map<std::string, int> s_choose_positions;
	}

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

	static void invoke_menu_callback(uint32_t menu_id, uint64_t version, std::size_t index, const std::vector<js_arg>& args)
	{
		auto menu = g_js_menus.find(menu_id);
		if (!g_js_context || menu == g_js_menus.end() || menu->second.build_version != version
		    || index >= menu->second.options.size())
		{
			return;
		}
		std::vector<qjs::Value> values;
		for (const auto& arg : args)
		{
			values.push_back(std::visit(
			    [](const auto& value) {
				    return g_js_context->newValue(value);
			    },
			    arg));
		}
		invoke_js_callback(menu->second.options[index].callback, std::move(values));
	}

	static void queue_menu_callback(uint32_t menu_id, uint64_t version, std::size_t index, std::vector<js_arg> args = {})
	{
		async_run([menu_id, version, index, args = std::move(args)] {
			std::lock_guard lock(g_js_mutex);
			invoke_menu_callback(menu_id, version, index, args);
		});
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
		double fraction = std::abs(step - std::round(step));
		while (opt.precision < 4 && fraction > 1e-6)
		{
			fraction *= 10;
			fraction = std::abs(fraction - std::round(fraction));
			++opt.precision;
		}
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
		auto& menu = g_js_menus[id];
		menu.type  = "submenu";
		menu.title = std::move(title);
		menu.id    = id;
		menu.options.clear();
		++menu.build_version;
		auto builder = g_js_context->newValue(std::make_shared<JSMenuBuilder>(id));
		callback.call(std::vector<qjs::Value>{std::move(builder)});
	}

	static void render_menu(uint32_t id, regular_submenu* sub)
	{
		auto found = s_views.find(id);
		if (found == s_views.end() || !found->second)
		{
			return;
		}
		const auto view = found->second;
		for (std::size_t index = 0; index < view->options.size(); ++index)
		{
			const auto& opt = view->options[index];
			if (opt.type == "bool")
			{
				sub->add_option<bool_option<bool>>(opt.text.c_str(),
				    opt.desc.c_str(),
				    &s_bool_states[opt.tag],
				    [tag = opt.tag] {
					    async_run([tag, value = s_bool_states[tag]] {
						    std::lock_guard lock(g_js_mutex);
						    store::set_bool(tag, value);
					    });
				    });
			}
			else if (opt.type == "slider")
			{
				if (opt.is_int)
				{
					sub->add_option<bool_slider_int_option>(opt.text.c_str(),
					    opt.desc.c_str(),
					    &s_bool_states[opt.tag],
					    &s_int_states[opt.tag],
					    (int)opt.min,
					    (int)opt.max,
					    (int)opt.step,
					    1,
					    true,
					    [id, version = view->version, index, tag = opt.tag] {
						    async_run([id, version, index, tag, value = s_int_states[tag]] {
							    std::lock_guard lock(g_js_mutex);
							    store::set_int(tag, value);
							    invoke_menu_callback(id, version, index, {value});
						    });
					    });
				}
				else
				{
					sub->add_option<bool_slider_float_option>(opt.text.c_str(),
					    opt.desc.c_str(),
					    &s_bool_states[opt.tag],
					    &s_float_states[opt.tag],
					    opt.min,
					    opt.max,
					    opt.step,
					    opt.precision,
					    true,
					    [id, version = view->version, index, tag = opt.tag] {
						    async_run([id, version, index, tag, value = s_float_states[tag]] {
							    std::lock_guard lock(g_js_mutex);
							    store::set_float(tag, value);
							    invoke_menu_callback(id, version, index, {static_cast<double>(value)});
						    });
					    });
				}
			}
			else if (opt.type == "choose")
			{
				if (opt.values.empty())
				{
					continue;
				}

				sub->add_option<choose_option<std::string, int>>(opt.text.c_str(),
				    opt.desc.c_str(),
				    &opt.values,
				    &s_choose_positions[opt.tag],
				    true,
				    [view, id, index] {
					    const auto& values = view->options[index].values;
					    const int position = s_choose_positions[view->options[index].tag];
					    if (position < 0 || position >= static_cast<int>(values.size()))
					    {
						    return;
					    }
					    queue_menu_callback(id, view->version, index, {position + 1, values[position]});
				    });
			}
			else if (opt.type == "option")
			{
				sub->add_option<reguler_option>(opt.text.c_str(), opt.desc.c_str(), [id, version = view->version, index] {
					queue_menu_callback(id, version, index);
				});
			}
			else if (opt.type == "submenu")
			{
				sub->add_option<sub_option>(opt.text.c_str(), opt.desc.c_str(), opt.sub_id, [id, version = view->version, index] {
					queue_menu_callback(id, version, index);
				});
			}
		}
	}

	void js_gui::register_gui()
	{
		std::unique_lock lock(g_js_mutex, std::try_to_lock);
		if (!lock)
		{
			return;
		}

		for (auto& [id, menu] : g_js_menus)
		{
			if (menu.rendered_version == menu.build_version)
			{
				continue;
			}

			auto view     = std::make_shared<menu_view>();
			view->version = menu.build_version;
			for (const auto& opt : menu.options)
			{
				view->options.push_back({opt.type, opt.text, opt.desc, opt.tag, opt.min, opt.max, opt.step, opt.precision, opt.subId, opt.isInt, opt.values});
				if (opt.type == "bool" && !s_bool_states.contains(opt.tag))
				{
					s_bool_states[opt.tag] = store::get_bool(opt.tag);
				}
				else if (opt.type == "slider")
				{
					if (!s_float_states.contains(opt.tag))
					{
						s_float_states[opt.tag] = store::get_float(opt.tag);
					}
					if (!s_int_states.contains(opt.tag))
					{
						s_int_states[opt.tag] = store::get_int(opt.tag);
					}
				}
			}
			s_views[id] = std::move(view);

			if (s_registered.insert(id).second)
			{
				auto render = [id](regular_submenu* sub) {
					render_menu(id, sub);
				};
				if (menu.type == "tabmenu")
				{
					canvas::add_tab<regular_submenu>(menu.title.c_str(), menu.id, render);
				}
				else
				{
					canvas::add_submenu<regular_submenu>(menu.title.c_str(), menu.id, render);
				}
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
		std::lock_guard lock(g_js_mutex);
		g_js_menus.clear();
		g_js_context = nullptr;
	}
} // namespace js::gui
