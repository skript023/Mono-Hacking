#include "trampoline_binding.hpp"
#include "hooking/detour_hook.hpp"

namespace js::trampoline
{
	using namespace big;

	template<typename Ret, typename... Args>
	struct JsDetour
	{
		using Fn = Ret(__fastcall*)(Args...);

		inline static Fn original = nullptr;
		inline static JSContext* ctx = nullptr;
		inline static JSValue js_func = JS_UNDEFINED;

		static JsDetour& instance()
		{
			static JsDetour detour;
			return detour;
		}

		static Ret __fastcall trampoline(Args... args)
		{
			if (JS_IsFunction(ctx, js_func))
			{
				JSValue argv[sizeof...(Args)];
				pack_args(ctx, argv, args...);

				JSValue ret = JS_Call(
				    ctx,
				    js_func,
				    JS_UNDEFINED,
				    sizeof...(Args),
				    argv);

				free_args(ctx, argv);

				if (!JS_IsException(ret))
				{
					if constexpr (!std::is_void_v<Ret>)
					{
						Ret r{};
						unpack_return(ctx, ret, r);
						JS_FreeValue(ctx, ret);
						return r;
					}
				}

				JS_FreeValue(ctx, ret);
			}

			if constexpr (!std::is_void_v<Ret>)
				return original(args...);
			else
				original(args...);
		}
	};

	static void pack(JSContext* ctx, JSValue& v, void* p)
	{
		v = JS_NewBigUint64(ctx, (uint64_t)p);
	}

	static void pack(JSContext* ctx, JSValue& v, int x)
	{
		v = JS_NewInt32(ctx, x);
	}

	static void pack(JSContext* ctx, JSValue& v, float f)
	{
		v = JS_NewFloat64(ctx, f);
	}

	template<typename T>
	static void pack(JSContext* ctx, JSValue& v, T x)
	{
		static_assert(sizeof(T) <= 8);
		v = JS_NewUint64(ctx, (uint64_t)x);
	}

	template<typename... Args>
	void pack_args(JSContext* ctx, JSValue* argv, Args... args)
	{
		int i = 0;
		((pack(ctx, argv[i++], args)), ...);
	}

	static void unpack_return(JSContext* ctx, JSValue v, int& out)
	{
		JS_ToInt32(ctx, &out, v);
	}

	static void unpack_return(JSContext* ctx, JSValue v, float& out)
	{
		double d;
		JS_ToFloat64(ctx, &d, v);
		out = (float)d;
	}

	static void unpack_return(JSContext* ctx, JSValue v, void*& out)
	{
		int64_t p;
		JS_ToBigInt64(ctx, &p, v);
		out = (void*)(uint64_t)p;
	}

	static JSValue js_add_detour(
	    JSContext* ctx,
	    JSValueConst,
	    int argc,
	    JSValueConst* argv)
	{
		// addDetour(addr, jsFunc)
		if (argc < 2 || !JS_IsNumber(argv[0]) || !JS_IsFunction(ctx, argv[1]))
			return JS_ThrowTypeError(ctx, "addDetour(addr, func)");

		int64_t addr;
		JS_ToBigInt64(ctx, &addr, argv[0]);

		auto& det = JsDetour<void, void*>::instance(); // example

		det.ctx = ctx;
		det.js_func = JS_DupValue(ctx, argv[1]);

		detour_base::add<JsDetour<void, void*>::trampoline>(
		    new detour_hook(
		        "js_detour",
		        (void*)(uint64_t)addr,
		        &JsDetour<void, void*>::trampoline));

		return JS_UNDEFINED;
	}

	static int js_detour_module_init(JSContext* ctx, JSModuleDef* m)
	{
		JS_SetModuleExport(ctx, m, "add_detour", JS_NewCFunction(ctx, js_add_detour, "add_detour", 2));

		return 0; // WAJIB
	}

	static JSModuleDef* js_detour_init(JSContext* ctx, const char* module_name)
	{
		JSModuleDef* m = JS_NewCModule(ctx, module_name, js_detour_module_init);

		if (!m)
			return nullptr;

		JS_AddModuleExport(ctx, m, "add_detour");

		return m;
	}

	void bind(qjs::Context& context)
	{
		auto ctx = context.ctx;

		JSModuleDef* m = js_detour_init(ctx, "hooking");

		if (!m)
			LOG(WARNING) << "Failed to init hooking module";
	}
} // namespace js::trampoline