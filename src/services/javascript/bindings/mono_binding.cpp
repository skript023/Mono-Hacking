#include "mono_binding.hpp"
#include "mono/mono.hpp"
#include "quickjs.h"
#include <vector>
#include <cstdint>

namespace js::mono
{

	// Helper: convert JS value (Number or BigInt) -> uint64_t (safe)
	static uint64_t js_to_u64(JSContext* ctx, JSValueConst v)
	{
		// BigInt path
		if (JS_VALUE_GET_TAG(v) == JS_TAG_BIG_INT)
		{
#ifdef CONFIG_BIGNUM
			uint64_t out = 0;
			if (JS_GetBigUint64(ctx, &out, v) == 0)
				return out;
#else
			int64_t tmp = 0;
			if (JS_ToBigInt64(ctx, &tmp, v) == 0)
				return (uint64_t)tmp;
#endif
			// fallthrough to number fallback if BigInt->uint64 failed
		}

		// Number fallback
		double d = 0;
		if (JS_ToFloat64(ctx, &d, v) < 0)
			return 0;
		return (uint64_t)d;
	}

	// Use JS_NewBigInt64 for returns (portable)
	static JSValue make_ptr_value(JSContext* ctx, void* p)
	{
		return JS_NewFloat64(ctx, (double)(uintptr_t)p);
	}

	static inline double js_value_to_double_checked(JSContext* ctx, JSValueConst v)
	{
		double d = 0.0;
		if (JS_ToFloat64(ctx, &d, v) < 0)
		{
			// not a number convertible
			return 0.0;
		}
		return d;
	}

	template<typename T>
	T value_to_ptr(JSContext* ctx, JSValueConst v)
	{
		double d = js_value_to_double_checked(ctx, v);
		return reinterpret_cast<T>(static_cast<uintptr_t>((uint64_t)d));
	}

	/* ----------------- MONO FUNCTIONS ----------------- */

	static JSValue js_mono_get_class(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "get_class(className, asmName)");

		const char* class_name = JS_ToCString(ctx, argv[0]);
		const char* asm_name = JS_ToCString(ctx, argv[1]);

		MonoClass* klass = big::mono::get_class(class_name, asm_name);

		JS_FreeCString(ctx, class_name);
		JS_FreeCString(ctx, asm_name);

		if (!klass)
			return JS_NULL;
		return make_ptr_value(ctx, klass);
	}

	static JSValue js_mono_get_method(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 4)
			return JS_ThrowTypeError(ctx, "get_method(asm, class, name, paramCount)");

		const char* asm_name = JS_ToCString(ctx, argv[0]);
		const char* cls_name = JS_ToCString(ctx, argv[1]);
		const char* mth_name = JS_ToCString(ctx, argv[2]);

		int param_count = 0;
		JS_ToInt32(ctx, &param_count, argv[3]);

		MonoMethod* method = big::mono::get_method(cls_name, mth_name, param_count, asm_name);

		JS_FreeCString(ctx, asm_name);
		JS_FreeCString(ctx, cls_name);
		JS_FreeCString(ctx, mth_name);

		if (!method)
			return JS_NULL;
		return make_ptr_value(ctx, method);
	}

	static JSValue js_mono_invoke_method(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "invoke_method(methodPtr, objPtr, argsArray)");

		uint64_t method_u64 = js_to_u64(ctx, argv[0]);
		MonoMethod* method = (MonoMethod*)(uintptr_t)method_u64;

		void* obj = nullptr;
		if (!JS_IsNull(argv[1]) && !JS_IsUndefined(argv[1]))
		{
			uint64_t obj_u64 = js_to_u64(ctx, argv[1]);
			obj = (void*)(uintptr_t)obj_u64;
		}

		std::vector<void*> args;
		if (argc >= 3 && JS_IsArray(ctx, argv[2]))
		{
			JSValue len_val = JS_GetPropertyStr(ctx, argv[2], "length");
			uint32_t len = 0;
			if (JS_ToUint32(ctx, &len, len_val) >= 0)
			{
				args.reserve(len);
				for (uint32_t i = 0; i < len; ++i)
				{
					JSValue el = JS_GetPropertyUint32(ctx, argv[2], i);
					uint64_t pv = js_to_u64(ctx, el);
					args.push_back((void*)(uintptr_t)pv);
					JS_FreeValue(ctx, el);
				}
			}
			JS_FreeValue(ctx, len_val);
		}

		MonoObject* result = big::mono::invoke_method(method, obj, args.empty() ? nullptr : args.data());
		if (!result)
			return JS_NULL;
		return make_ptr_value(ctx, result);
	}

	static JSValue js_mono_get_field(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		// support get_field(asm,class,field) like original code
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "get_field(class, field)");

		MonoClass* cls_name = value_to_ptr<MonoClass*>(ctx, argv[0]);
		const char* fld_name = JS_ToCString(ctx, argv[1]);

		MonoClassField* field = big::mono::get_field(cls_name, fld_name);

		JS_FreeCString(ctx, fld_name);

		if (!field)
			return JS_NULL;
		return make_ptr_value(ctx, field);
	}

	static JSValue js_mono_set_field(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "set_field(objPtr, fieldPtr, valuePtr)");

		auto obj = value_to_ptr<MonoObject*>(ctx, argv[0]);
		auto field = value_to_ptr<MonoClassField*>(ctx, argv[1]);
		auto value = value_to_ptr<void*>(ctx, argv[2]);

		big::mono::set_field_value(obj, field, value);
		return JS_UNDEFINED;
	}

	static JSValue js_mono_get_field_value(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "get_field_value(objPtr, fieldPtr)");
		uint64_t obj_u64 = js_to_u64(ctx, argv[0]);
		uint64_t fld_u64 = js_to_u64(ctx, argv[1]);
		MonoObject* obj = (MonoObject*)(uintptr_t)obj_u64;
		MonoClassField* field = (MonoClassField*)(uintptr_t)fld_u64;
		void* out_value = nullptr;
		big::mono::get_field_value(obj, field, &out_value);
		return make_ptr_value(ctx, out_value);
	}

	static JSValue js_mono_get_field_offset(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_field_offset(fieldPtr)");
		uint64_t fld_u64 = js_to_u64(ctx, argv[0]);
		MonoClassField* field = (MonoClassField*)(uintptr_t)fld_u64;
		uint32_t offset = big::mono::get_field_offset(field);
		return JS_NewUint32(ctx, offset);
	}

	static JSValue js_mono_get_static_field_value(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "get_static_field_value(className, fieldName)");
		const char* class_name = JS_ToCString(ctx, argv[0]);
		const char* field_name = JS_ToCString(ctx, argv[1]);
		void* value = big::mono::get_static_field_value(class_name, field_name);
		JS_FreeCString(ctx, class_name);
		JS_FreeCString(ctx, field_name);
		return make_ptr_value(ctx, value);
	}

	static JSValue js_mono_get_static_field_data(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_static_field_data(classPtr)");
		uint64_t class_u64 = js_to_u64(ctx, argv[0]);
		MonoClass* klass = (MonoClass*)(uintptr_t)class_u64;
		void* data = big::mono::get_static_field_data(klass);
		return make_ptr_value(ctx, data);
	}

	static JSValue js_mono_get_static_field_data_vtable(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_static_field_data_vtable(vtablePtr)");
		uint64_t vtable_u64 = js_to_u64(ctx, argv[0]);
		MonoVTable* vtable = (MonoVTable*)(uintptr_t)vtable_u64;
		void* data = big::mono::get_static_field_data(vtable);
		return make_ptr_value(ctx, data);
	}

	static JSValue js_mono_get_vtable(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_vtable(classPtr)");
		uint64_t class_u64 = js_to_u64(ctx, argv[0]);
		MonoClass* klass = (MonoClass*)(uintptr_t)class_u64;
		MonoVTable* vtable = big::mono::get_vtable(klass);
		return make_ptr_value(ctx, vtable);
	}

	static JSValue js_mono_get_class_from_method(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_class_from_method(methodPtr)");
		uint64_t method_u64 = js_to_u64(ctx, argv[0]);
		MonoMethod* method = (MonoMethod*)(uintptr_t)method_u64;
		MonoClass* klass = big::mono::get_class_from_method(method);
		if (!klass)
			return JS_NULL;
		return make_ptr_value(ctx, klass);
	}

	static JSValue js_mono_get_compile_method(JSContext* ctx, JSValueConst /*this_val*/, int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "get_compile_method(className, methodName, paramCount [,assemblyName])");

		const char* class_name = JS_ToCString(ctx, argv[0]);
		const char* method_name = JS_ToCString(ctx, argv[1]);
		int param_count = 0;
		JS_ToInt32(ctx, &param_count, argv[2]);

		const char* assembly_name = nullptr;
		if (argc >= 4)
			assembly_name = JS_ToCString(ctx, argv[3]);
		else
			assembly_name = "Assembly-CSharp";

		void* compile_method = big::mono::get_compile_method(class_name, method_name, param_count, assembly_name);

		JS_FreeCString(ctx, class_name);
		JS_FreeCString(ctx, method_name);
		if (argc >= 4)
			JS_FreeCString(ctx, assembly_name);

		if (!compile_method)
			return JS_NULL;
		return make_ptr_value(ctx, compile_method);
	}

	/* ----------------- REGISTER helpers ----------------- */

#define MONO_SET_FUNC2(target_obj, name, fn, argc) \
	JS_SetPropertyStr(ctx, target_obj, name, JS_NewCFunction(ctx, fn, name, argc))

	// Register as actual ES module 'mono' so import * as m from 'mono' works.
	// Call js_init_module_mono(ctx, "mono") once before evaluating any module that imports it.
	static int js_mono_init(JSContext* ctx, JSModuleDef* m)
	{
		JS_SetModuleExport(ctx, m, "get_class", JS_NewCFunction(ctx, js_mono_get_class, "get_class", 2));
		JS_SetModuleExport(ctx, m, "get_method", JS_NewCFunction(ctx, js_mono_get_method, "get_method", 4));
		JS_SetModuleExport(ctx, m, "invoke_method", JS_NewCFunction(ctx, js_mono_invoke_method, "invoke_method", 3));
		JS_SetModuleExport(ctx, m, "get_field", JS_NewCFunction(ctx, js_mono_get_field, "get_field", 3));
		JS_SetModuleExport(ctx, m, "set_field", JS_NewCFunction(ctx, js_mono_set_field, "set_field", 3));
		JS_SetModuleExport(ctx, m, "get_field_value", JS_NewCFunction(ctx, js_mono_get_field_value, "get_field_value", 2));
		JS_SetModuleExport(ctx, m, "get_field_offset", JS_NewCFunction(ctx, js_mono_get_field_offset, "get_field_offset", 1));
		JS_SetModuleExport(ctx, m, "get_static_field_value", JS_NewCFunction(ctx, js_mono_get_static_field_value, "get_static_field_value", 2));
		JS_SetModuleExport(ctx, m, "get_static_field_data", JS_NewCFunction(ctx, js_mono_get_static_field_data, "get_static_field_data", 1));
		JS_SetModuleExport(ctx, m, "get_static_field_data_vtable", JS_NewCFunction(ctx, js_mono_get_static_field_data_vtable, "get_static_field_data_vtable", 1));
		JS_SetModuleExport(ctx, m, "get_vtable", JS_NewCFunction(ctx, js_mono_get_vtable, "get_vtable", 1));
		JS_SetModuleExport(ctx, m, "get_class_from_method", JS_NewCFunction(ctx, js_mono_get_class_from_method, "get_class_from_method", 1));
		JS_SetModuleExport(ctx, m, "get_compile_method", JS_NewCFunction(ctx, js_mono_get_compile_method, "get_compile_method", 4));
		return 0;
	}

	static JSModuleDef* js_init_module_mono(JSContext* ctx, const char* name)
	{
		JSModuleDef* m = JS_NewCModule(ctx, name, js_mono_init);
		if (!m)
			return nullptr;

		JS_AddModuleExport(ctx, m, "get_class");
		JS_AddModuleExport(ctx, m, "get_method");
		JS_AddModuleExport(ctx, m, "invoke_method");
		JS_AddModuleExport(ctx, m, "get_field");
		JS_AddModuleExport(ctx, m, "set_field");
		JS_AddModuleExport(ctx, m, "get_field_value");
		JS_AddModuleExport(ctx, m, "get_field_offset");
		JS_AddModuleExport(ctx, m, "get_static_field_value");
		JS_AddModuleExport(ctx, m, "get_static_field_data");
		JS_AddModuleExport(ctx, m, "get_static_field_data_vtable");
		JS_AddModuleExport(ctx, m, "get_vtable");
		JS_AddModuleExport(ctx, m, "get_class_from_method");
		JS_AddModuleExport(ctx, m, "get_compile_method");

		return m;
	}

	
	// Register as global.mono (compatible with existing code that uses global object)
	void bind(qjs::Context& context)
	{
		auto ctx = context.ctx;
		JSValue global = JS_GetGlobalObject(ctx);
		JSValue mono = JS_NewObject(ctx);

		MONO_SET_FUNC2(mono, "get_class", js_mono_get_class, 2);
		MONO_SET_FUNC2(mono, "get_method", js_mono_get_method, 4);
		MONO_SET_FUNC2(mono, "invoke_method", js_mono_invoke_method, 3);
		MONO_SET_FUNC2(mono, "get_field", js_mono_get_field, 3);
		MONO_SET_FUNC2(mono, "set_field", js_mono_set_field, 3);
		MONO_SET_FUNC2(mono, "get_field_value", js_mono_get_field_value, 2);
		MONO_SET_FUNC2(mono, "get_field_offset", js_mono_get_field_offset, 1);
		MONO_SET_FUNC2(mono, "get_static_field_value", js_mono_get_static_field_value, 2);
		MONO_SET_FUNC2(mono, "get_static_field_data", js_mono_get_static_field_data, 1);
		MONO_SET_FUNC2(mono, "get_static_field_data_vtable", js_mono_get_static_field_data_vtable, 1);
		MONO_SET_FUNC2(mono, "get_vtable", js_mono_get_vtable, 1);
		MONO_SET_FUNC2(mono, "get_class_from_method", js_mono_get_class_from_method, 1);
		MONO_SET_FUNC2(mono, "get_compile_method", js_mono_get_compile_method, 4);

		JS_SetPropertyStr(ctx, global, "mono", mono);

		JS_FreeValue(ctx, global);

		big::mono::thread_attach(big::mono::get_root_domain());
	}
} // namespace js::mono