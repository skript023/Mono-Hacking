#include "javascript_binding.hpp"
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

	void javacript_binding::bind(Context& ctx)
	{
		
	}
}