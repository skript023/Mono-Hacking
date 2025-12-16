#include "javascript_binding.hpp"
#include "utility/unity.hpp"
#include "commands/commands.hpp"
#include "commands/command.hpp"

namespace big
{
	static std::vector<uint64_t> convert_sequence(const JSValue& js_val, Context& ctx)
	{
		std::vector<uint64_t> result;

		// Cek apakah array
		if (!JS_IsArray(ctx.ctx, js_val))
			return result;

		// Ambil length
		JSValue js_len = JS_GetPropertyStr(ctx.ctx, js_val, "length");
		int64_t len = 0;
		JS_ToInt64(ctx.ctx, &len, js_len);
		JS_FreeValue(ctx.ctx, js_len);

		for (int64_t i = 0; i < len; i++)
		{
			JSValue elem = JS_GetPropertyUint32(ctx.ctx, js_val, i);
			int64_t val = 0;
			JS_ToBigInt64(ctx.ctx, &val, elem);
			JS_FreeValue(ctx.ctx, elem);
			result.push_back(val);
		}

		return result;
	}


	static void call_command(const std::string& command_name, JSValue js_args, Context& ctx)
	{
		std::optional<JSValue> maybe_args;
		if (!ctx.isUndefined(js_args) && !ctx.isNull(js_args))
		{
			maybe_args = js_args;
		}

		if (auto cmd = big::commands::get_command<command>(joaat(command_name)))
			cmd->call();
	}

	static void js_log_info(std::string const& args)
	{
		LOG(INFO) << args;
	}

	static void js_console_log(const qjs::rest<std::string>& args)
	{
		std::stringstream out;
		for (const auto& message : args)
		{
			out << " " << message;
		}

		LOG(INFO) << std::format("[JS] {}", out.str());
	}

	static double js_get_local_player()
	{
		return (double)(uintptr_t)unity::get_local_player();
	}

	static double js_get_zone_system()
	{
		return (double)(uintptr_t)unity::get_zone_system();
	}

	static double js_get_env_man()
	{
		return (double)(uintptr_t)unity::get_env_man();
	}

	void javacript_binding::bind(Context& ctx)
	{
		auto& module = ctx.addModule("commands");
		module.function("call", [&ctx](const std::string& cmd, JSValue js_args) {
			call_command(cmd, js_args, ctx);
		});

		auto& logger = ctx.addModule("Logger");
		logger.function<&js_log_info>("info");

		auto& unity = ctx.addModule("Unity");
		unity.function<&js_get_local_player>("get_local_player");
		unity.function<&js_get_zone_system>("get_zone_system");
		unity.function<&js_get_env_man>("get_env_man");

		auto console_object = ctx.newObject();
		auto global = ctx.global();

		console_object.add<&js_console_log>("log");
		global["console"] = console_object;
	}
}