#include "javascript_binding.hpp"

#include "../javascript_manager.hpp"
#include "class/vector.hpp"
#include "commands/command.hpp"
#include "commands/commands.hpp"
#include "utility/unity.hpp"

namespace big
{
	static void call_command(const std::string& command_name, qjs::Value)
	{
		if (auto cmd = commands::get_command<command>(joaat(command_name)))
		{
			cmd->call();
		}
	}

	static void js_log_info(const std::string& message)
	{
		LOG(INFO) << message;
		javascript_manager::log_console(js_log_level::info, message);
	}

	static void js_console_log(const qjs::rest<std::string>& args)
	{
		std::stringstream out;
		bool first = true;
		for (const auto& message : args)
		{
			if (!first)
			{
				out << ' ';
			}
			out << message;
			first = false;
		}
		LOG(INFO) << std::format("[JS] {}", out.str());
		javascript_manager::log_console(js_log_level::info, out.str());
	}

	static void js_console_warn(const qjs::rest<std::string>& args)
	{
		std::stringstream out;
		bool first = true;
		for (const auto& message : args)
		{
			if (!first)
			{
				out << ' ';
			}
			out << message;
			first = false;
		}
		LOG(WARNING) << std::format("[JS] {}", out.str());
		javascript_manager::log_console(js_log_level::warn, out.str());
	}

	static void js_console_error(const qjs::rest<std::string>& args)
	{
		std::stringstream out;
		bool first = true;
		for (const auto& message : args)
		{
			if (!first)
			{
				out << ' ';
			}
			out << message;
			first = false;
		}
		LOG(WARNING) << std::format("[JS Error] {}", out.str());
		javascript_manager::log_console(js_log_level::error, out.str());
	}

	static double js_get_local_player()
	{
		return static_cast<double>(reinterpret_cast<uintptr_t>(unity::get_local_player()));
	}

	static double js_get_zone_system()
	{
		return static_cast<double>(reinterpret_cast<uintptr_t>(unity::get_zone_system()));
	}

	static double js_get_env_man()
	{
		return static_cast<double>(reinterpret_cast<uintptr_t>(unity::get_env_man()));
	}

	void javacript_binding::bind(qjs::Context& ctx)
	{
		auto global = ctx.global();

		auto& commands_module = ctx.addModule("commands");
		commands_module.function("call", &call_command);

		auto logger = ctx.newObject();
		logger.add<&js_log_info>("info");

		auto unity_api = ctx.newObject();
		unity_api.add<&js_get_local_player>("get_local_player");
		unity_api.add<&js_get_zone_system>("get_zone_system");
		unity_api.add<&js_get_env_man>("get_env_man");

		auto console = ctx.newObject();
		console.add<&js_console_log>("log");
		console.add<&js_console_warn>("warn");
		console.add<&js_console_error>("error");

		auto vector_prototype = ctx.newObject();
		vector_prototype.add<&Vector3::x>("x");
		vector_prototype.add<&Vector3::y>("y");
		vector_prototype.add<&Vector3::z>("z");

		auto default_constructor = ctx.newValue(qjs::ctor_wrapper<Vector3>{});
		JS_SetConstructor(ctx.ctx, default_constructor.v, vector_prototype.v);
		global["Vector3"] = default_constructor;

		auto constructor = ctx.newValue(qjs::ctor_wrapper<Vector3, float, float, float>{"Vector3"});
		JS_SetConstructor(ctx.ctx, constructor.v, vector_prototype.v);
		global["Vector3"] = constructor;
		ctx.registerClass<Vector3>("Vector3", std::move(vector_prototype));

		global["console"] = console;
		global["unity"]   = unity_api;
		global["logger"]  = logger;
	}
}
