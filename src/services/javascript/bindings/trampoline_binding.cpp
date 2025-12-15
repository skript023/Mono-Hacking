#include "trampoline_binding.hpp"
#include "hooking/detour_hook.hpp"

namespace js::trampoline
{
	using namespace big;

	constexpr size_t ABI_MAX_ARGS = 7;

	struct AbiContext
	{
		uint64_t args[ABI_MAX_ARGS];
		uint64_t ret;
		void* original;
	};

	static std::unordered_map<std::string, detour_hook*> g_hooks;

	static JSClassID g_ctx_class;

	// =========================================================
	// JS <-> C++ helpers
	// =========================================================

	static JSValue js_ctx_get_args(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* c = (AbiContext*)JS_GetOpaque(this_val, g_ctx_class);
		JSValue arr = JS_NewArray(ctx);

		for (uint32_t i = 0; i < ABI_MAX_ARGS; ++i)
		{
			JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, (double)c->args[i]));
		}
		return arr;
	}

	static JSValue js_ctx_set_arg(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* c = (AbiContext*)JS_GetOpaque(this_val, g_ctx_class);
		if (argc != 2)
			return JS_ThrowTypeError(ctx, "setArg(index, value)");

		uint32_t idx;
		JS_ToUint32(ctx, &idx, argv[0]);
		if (idx >= ABI_MAX_ARGS)
			return JS_ThrowRangeError(ctx, "arg index out of range");

		double v;
		JS_ToFloat64(ctx, &v, argv[1]);
		c->args[idx] = (uint64_t)v;
		return JS_UNDEFINED;
	}

	static JSValue js_ctx_call_original(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* c = (AbiContext*)JS_GetOpaque(this_val, g_ctx_class);

		using Fn = uint64_t(__fastcall*)(
		    uint64_t,
		    uint64_t,
		    uint64_t,
		    uint64_t,
		    uint64_t,
		    uint64_t,
		    uint64_t);

		auto fn = (Fn)c->original;
		c->ret = fn(
		    c->args[0],
		    c->args[1],
		    c->args[2],
		    c->args[3],
		    c->args[4],
		    c->args[5],
		    c->args[6]);

		return JS_NewFloat64(ctx, (double)c->ret);
	}

	static JSClassDef ctx_class_def{
	    "NativeContext"};

	static void register_ctx_class(JSContext* ctx)
	{
		JS_NewClassID(&g_ctx_class);
		JS_NewClass(JS_GetRuntime(ctx), g_ctx_class, &ctx_class_def);

		JSValue proto = JS_NewObject(ctx);

		JS_SetPropertyStr(ctx, proto, "args", JS_NewCFunction(ctx, js_ctx_get_args, "args", 0));

		JS_SetPropertyStr(ctx, proto, "setArg", JS_NewCFunction(ctx, js_ctx_set_arg, "setArg", 2));

		JS_SetPropertyStr(ctx, proto, "callOriginal", JS_NewCFunction(ctx, js_ctx_call_original, "callOriginal", 0));

		JS_SetClassProto(ctx, g_ctx_class, proto);
	}

	// =========================================================
	// ABI TRAMPOLINE
	// =========================================================

	struct JsDetour
	{
		inline static JSContext* ctx = nullptr;
		inline static JSValue js_func = JS_UNDEFINED;
		inline static void* original = nullptr;

		static uint64_t __fastcall trampoline(
		    uint64_t a0, uint64_t a1, uint64_t a2,
		    uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
		{
			AbiContext* abi = new AbiContext();
			abi->args[0] = a0;
			abi->args[1] = a1;
			abi->args[2] = a2;
			abi->args[3] = a3;
			abi->args[4] = a4;
			abi->args[5] = a5;
			abi->args[6] = a6;
			abi->original = original;

			if (JS_IsFunction(ctx, js_func))
			{
				JSValue obj = JS_NewObjectClass(ctx, g_ctx_class);
				JS_SetOpaque(obj, abi);

				JSValue ret = JS_Call(ctx, js_func, JS_UNDEFINED, 1, &obj);
				JS_FreeValue(ctx, obj);

				if (!JS_IsException(ret))
				{
					double d;
					if (JS_ToFloat64(ctx, &d, ret) == 0)
					{
						JS_FreeValue(ctx, ret);
						return (uint64_t)d;
					}
				}
				JS_FreeValue(ctx, ret);
			}

			using Fn = uint64_t(__fastcall*)(
			    uint64_t,
			    uint64_t,
			    uint64_t,
			    uint64_t,
			    uint64_t,
			    uint64_t,
			    uint64_t);

			return ((Fn)original)(
			    abi->args[0],
			    abi->args[1],
			    abi->args[2],
			    abi->args[3],
			    abi->args[4],
			    abi->args[5],
			    abi->args[6]);
		}
	};

	// =========================================================
	// JS API
	// =========================================================

	static JSValue js_add_detour(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		if (argc != 3)
			return JS_ThrowTypeError(ctx, "add(name, addr, cb)");

		const char* name = JS_ToCString(ctx, argv[0]);
		void* addr;
		double d;
		JS_ToFloat64(ctx, &d, argv[1]);
		addr = (void*)(uint64_t)d;

		JsDetour::ctx = ctx;
		JsDetour::js_func = JS_DupValue(ctx, argv[2]);

		auto* hook = new detour_hook(
		    name,
		    addr,
		    &JsDetour::trampoline);

		JsDetour::original = hook->get_original_ptr();
		g_hooks.emplace(name, hook);

		JS_FreeCString(ctx, name);
		return JS_UNDEFINED;
	}

	static JSValue js_hook_enable(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		const char* name = JS_ToCString(ctx, argv[0]);
		g_hooks[name]->enable_immediately();
		JS_FreeCString(ctx, name);
		return JS_UNDEFINED;
	}

	static JSValue js_hook_disable(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv)
	{
		const char* name = JS_ToCString(ctx, argv[0]);
		g_hooks[name]->disable_immediately();
		JS_FreeCString(ctx, name);
		return JS_UNDEFINED;
	}

	static int js_detour_module_init(JSContext* ctx, JSModuleDef* m)
	{
		register_ctx_class(ctx);

		JS_SetModuleExport(ctx, m, "add", JS_NewCFunction(ctx, js_add_detour, "add", 3));
		JS_SetModuleExport(ctx, m, "enable", JS_NewCFunction(ctx, js_hook_enable, "enable", 1));
		JS_SetModuleExport(ctx, m, "disable", JS_NewCFunction(ctx, js_hook_disable, "disable", 1));

		return 0;
	}

	void bind(qjs::Context& context)
	{
		JSModuleDef* m = JS_NewCModule(context.ctx, "detour", js_detour_module_init);
		JS_AddModuleExport(context.ctx, m, "add");
		JS_AddModuleExport(context.ctx, m, "enable");
		JS_AddModuleExport(context.ctx, m, "disable");
	}
}
