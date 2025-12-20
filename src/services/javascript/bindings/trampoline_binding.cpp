#include "trampoline_binding.hpp"
#include "hooking/detour_hook.hpp"

namespace js::trampoline
{
	using namespace big;

	constexpr size_t ABI_MAX_ARGS = 16;

	static JSClassID g_arg_wrapper_class;

	struct ArgWrapper
	{
		uintptr_t* ptr; // Menunjuk ke c->args[idx]
	};

	struct AbiContext
	{
		uintptr_t args[ABI_MAX_ARGS];
		uintptr_t ret;
		void* original;
	};

	static std::unordered_map<std::string, detour_hook*> g_hooks;

	static JSClassID g_ctx_class;

	// =========================================================
	// JS <-> C++ helpers
	// =========================================================

	static JSValue js_ctx_get_all_args(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* c = (AbiContext*)JS_GetOpaque(this_val, g_ctx_class);
		JSValue arr = JS_NewArray(ctx);

		for (uint32_t i = 0; i < ABI_MAX_ARGS; ++i)
		{
			JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, (double)c->args[i]));
		}
		return arr;
	}

	static JSValue js_ctx_get_args(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* c = (AbiContext*)JS_GetOpaque(this_val, g_ctx_class);
		uint32_t idx;
		JS_ToUint32(ctx, &idx, argv[0]);

		if (idx >= ABI_MAX_ARGS)
			return JS_ThrowRangeError(ctx, "arg index out of range");

		// Buat object wrapper
		JSValue obj = JS_NewObjectClass(ctx, g_arg_wrapper_class);
		ArgWrapper* w = (ArgWrapper*)js_malloc(ctx, sizeof(ArgWrapper));
		w->ptr = &c->args[idx]; // Simpan pointer ke elemen array

		JS_SetOpaque(obj, w);
		return obj;
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
		c->args[idx] = (uintptr_t)v;
		return JS_UNDEFINED;
	}

	static JSValue js_ctx_call_original(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* c = (AbiContext*)JS_GetOpaque(this_val, g_ctx_class);

		using Fn = uintptr_t(__fastcall*)(
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t,
		    uintptr_t);

		auto fn = (Fn)c->original;
		c->ret = fn(
		    c->args[0],
		    c->args[1],
		    c->args[2],
		    c->args[3],
		    c->args[4],
		    c->args[5],
		    c->args[6],
		    c->args[7],
		    c->args[8],
		    c->args[9],
		    c->args[10],
		    c->args[11],
		    c->args[12],
		    c->args[13],
		    c->args[14],
		    c->args[15]);

		return JS_NewFloat64(ctx, (double)c->ret);
	}

	static JSClassDef ctx_class_def{"NativeContext"};

	static JSClassDef arg_wrapper_def{"ArgWrapper"};

	static JSValue js_arg_as_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* w = (ArgWrapper*)JS_GetOpaque(this_val, g_arg_wrapper_class);
		if (!w || !w->ptr)
			return JS_EXCEPTION;

		// Cast uintptr_t ke const char*
		return JS_NewString(ctx, (const char*)*w->ptr);
	}

	// Fungsi untuk mengambil value sebagai number (default)
	static JSValue js_arg_as_number(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* w = (ArgWrapper*)JS_GetOpaque(this_val, g_arg_wrapper_class);
		if (!w || !w->ptr)
			return JS_EXCEPTION;

		return JS_NewFloat64(ctx, (double)*w->ptr);
	}
	struct MonoString
	{
		void* klass;
		void* monitor;
		int32_t length;
		char16_t chars[1];
	};

	static JSValue js_arg_as_mono_string(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
	{
		auto* w = (ArgWrapper*)JS_GetOpaque(this_val, g_arg_wrapper_class);
		if (!w || !w->ptr)
			return JS_EXCEPTION;

		auto* ms = (MonoString*)*w->ptr;
		if (!ms || ms->length < 0 || ms->length > 0x2000)
			return JS_NULL;

		int len = WideCharToMultiByte(
		    CP_UTF8,
		    0,
		    (wchar_t*)ms->chars,
		    ms->length,
		    nullptr,
		    0,
		    nullptr,
		    nullptr);

		std::string out(len, '\0');

		WideCharToMultiByte(
		    CP_UTF8,
		    0,
		    (wchar_t*)ms->chars,
		    ms->length,
		    out.data(),
		    len,
		    nullptr,
		    nullptr);

		return JS_NewStringLen(ctx, out.data(), out.size());
	}

	static void register_ctx_class(JSContext* ctx)
	{
		JS_NewClassID(&g_arg_wrapper_class);
		JS_NewClass(JS_GetRuntime(ctx), g_arg_wrapper_class, &arg_wrapper_def);

		JSValue arg_proto = JS_NewObject(ctx);
		JS_SetPropertyStr(ctx, arg_proto, "string", JS_NewCFunction(ctx, js_arg_as_string, "string", 0));
		JS_SetPropertyStr(ctx, arg_proto, "monoString", JS_NewCFunction(ctx, js_arg_as_mono_string, "monoString", 0));
		JS_SetPropertyStr(ctx, arg_proto, "number", JS_NewCFunction(ctx, js_arg_as_number, "number", 0));
		// Opsional: override valueOf agar bisa langsung dipakai di operasi math
		JS_SetPropertyStr(ctx, arg_proto, "valueOf", JS_NewCFunction(ctx, js_arg_as_number, "valueOf", 0));

		JS_SetClassProto(ctx, g_arg_wrapper_class, arg_proto);

		JS_NewClassID(&g_ctx_class);
		JS_NewClass(JS_GetRuntime(ctx), g_ctx_class, &ctx_class_def);

		JSValue proto = JS_NewObject(ctx);

		JS_SetPropertyStr(ctx, proto, "args", JS_NewCFunction(ctx, js_ctx_get_all_args, "args", 0));

		JS_SetPropertyStr(ctx, proto, "getArg", JS_NewCFunction(ctx, js_ctx_get_args, "getArg", 1));

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

		static uintptr_t __fastcall trampoline(
		    uintptr_t a0, uintptr_t a1, uintptr_t a2,
		    uintptr_t a3, uintptr_t a4, uintptr_t a5, 
			uintptr_t a6, uintptr_t a7, uintptr_t a8, 
			uintptr_t a9, uintptr_t a10, uintptr_t a11,
		    uintptr_t a12, uintptr_t a13, uintptr_t a14, uintptr_t a15)
		{
			AbiContext abi{};
			abi.args[0] = a0;
			abi.args[1] = a1;
			abi.args[2] = a2;
			abi.args[3] = a3;
			abi.args[4] = a4;
			abi.args[5] = a5;
			abi.args[6] = a6;
			abi.args[7] = a7;
			abi.args[8] = a8;
			abi.args[9] = a9;
			abi.args[10] = a10;
			abi.args[11] = a11;
			abi.args[12] = a12;
			abi.args[13] = a13;
			abi.args[14] = a14;
			abi.args[15] = a15;
			abi.original = original;

			if (JS_IsFunction(ctx, js_func))
			{
				JSValue obj = JS_NewObjectClass(ctx, g_ctx_class);
				JS_SetOpaque(obj, &abi);

				JSValue ret = JS_Call(ctx, js_func, JS_UNDEFINED, 1, &obj);
				JS_FreeValue(ctx, obj);

				if (!JS_IsException(ret))
				{
					double d;
					if (JS_ToFloat64(ctx, &d, ret) == 0)
					{
						JS_FreeValue(ctx, ret);
						return (uintptr_t)d;
					}
				}
				JS_FreeValue(ctx, ret);
			}

			using Fn = uintptr_t(__fastcall*)(
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t,
			    uintptr_t);

			return ((Fn)original)(
			    abi.args[0],
			    abi.args[1],
			    abi.args[2],
			    abi.args[3],
			    abi.args[4],
			    abi.args[5],
			    abi.args[6],
			    abi.args[7],
			    abi.args[8],
			    abi.args[9],
			    abi.args[10],
			    abi.args[11],
			    abi.args[12],
			    abi.args[13],
			    abi.args[14],
			    abi.args[15]);
		}
	};

	// =========================================================
	// JS API
	// =========================================================

	static bool js_add_detour(std::string const& name, double address, qjs::Value callback)
	{
		void* addr = (void*)(uintptr_t)address;

		if (!addr)
		{
			LOG(WARNING) << "Invalid address for detour";

			return false;
		}

		LOG(VERBOSE) << "Context for detour: " << (void*)callback.ctx << " JSValue: " << (void*)callback.v.tag;

		JsDetour::ctx = callback.ctx;
		JsDetour::js_func = JS_DupValue(callback.ctx, callback.v);

		auto* hook = new detour_hook(
		    name,
		    addr,
		    &JsDetour::trampoline);

		JsDetour::original = hook->get_original_ptr();
		g_hooks.emplace(name, hook);

		return true;
	}

	static void js_hook_enable(std::string const& name)
	{
		g_hooks[name.c_str()]->enable_immediately();
	}

	static void js_hook_disable(std::string const& name)
	{
		auto det = g_hooks[name.c_str()];
		det->disable_immediately();

		delete det;
	}

	static int js_detour_module_init(JSContext* ctx, JSModuleDef* m)
	{
		register_ctx_class(ctx);

		return 0;
	}

	void bind(qjs::Context& context)
	{
		auto ctx = context.ctx;

		register_ctx_class(ctx);

		auto& module = context.addModule("detour");

		module.function<&js_add_detour>("add");
		module.function<&js_hook_enable>("enable");
		module.function<&js_hook_disable>("disable");
	}
}
