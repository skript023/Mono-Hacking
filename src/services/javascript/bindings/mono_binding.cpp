#include "mono_binding.hpp"
#include "mono/mono.hpp"

namespace js::mono
{
	static uint64_t js_to_u64(JSContext* ctx, JSValueConst v)
	{
		uint64_t out = 0;

		/* Jika BigInt */
		if (JS_VALUE_GET_TAG(v) == JS_TAG_BIG_INT)
		{
#ifdef CONFIG_BIGNUM
			if (JS_GetBigUint64(ctx, &out, v) == 0)
				return out;
#else
			// fallback: convert BigInt → int64
			int64_t tmp = 0;
			if (JS_ToBigInt64(ctx, &tmp, v) == 0)
				return (uint64_t)tmp;
#endif
			// BigInt error → fallback Number
		}

		/* Number biasa */
		double d = 0;
		if (JS_ToFloat64(ctx, &d, v) < 0)
			return 0;

		return (uint64_t)d;
	}

	static JSValue js_mono_get_class(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		const char* assembly_name = JS_ToCString(ctx, argv[0]);
		const char* class_name = JS_ToCString(ctx, argv[1]);

		MonoClass* klass = big::mono::get_class(class_name, assembly_name);

		JS_FreeCString(ctx, assembly_name);
		JS_FreeCString(ctx, class_name);

		if (!klass)
			return JS_NULL;

		return JS_NewBigUint64(ctx, (uint64_t)klass);
	}

	static JSValue js_mono_get_method(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		if (argc < 4)
			return JS_ThrowTypeError(ctx, "get_method(asm, class, name, paramCount)");

		const char* asm_name = JS_ToCString(ctx, argv[0]);
		const char* cls_name = JS_ToCString(ctx, argv[1]);
		const char* mth_name = JS_ToCString(ctx, argv[2]);

		int param_count = 0;
		JS_ToInt32(ctx, &param_count, argv[3]);

		MonoMethod* method =
		    big::mono::get_method(cls_name, mth_name, param_count, asm_name);

		JS_FreeCString(ctx, asm_name);
		JS_FreeCString(ctx, cls_name);
		JS_FreeCString(ctx, mth_name);

		if (!method)
			return JS_NULL;

		return JS_NewBigUint64(ctx, (uint64_t)method);
	}

	static JSValue js_mono_invoke_method(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "invoke_method(methodPtr, objPtr, argsArray)");

		// method pointer
		uint64_t method_u64 = js_to_u64(ctx, argv[0]);
		MonoMethod* method = (MonoMethod*)method_u64;

		// object pointer
		void* obj = nullptr;
		if (!JS_IsNull(argv[1]) && !JS_IsUndefined(argv[1]))
		{
			uint64_t obj_u64 = js_to_u64(ctx, argv[1]);
			obj = (void*)obj_u64;
		}

		// argument array
		std::vector<void*> args;
		if (JS_IsArray(ctx, argv[2]))
		{
			uint32_t len = 0;
			JS_GetPropertyUint32(ctx, argv[2], 0);
			JSValue len_val = JS_GetPropertyStr(ctx, argv[2], "length");
			JS_ToUint32(ctx, &len, len_val);
			JS_FreeValue(ctx, len_val);

			args.reserve(len);

			for (uint32_t i = 0; i < len; i++)
			{
				JSValue elem = JS_GetPropertyUint32(ctx, argv[2], i);
				uint64_t pv = js_to_u64(ctx, elem);
				args.push_back((void*)pv);
				JS_FreeValue(ctx, elem);
			}
		}

		MonoObject* result =
		    big::mono::invoke_method(method, obj, args.empty() ? nullptr : args.data());

		if (!result)
			return JS_NULL;

		return JS_NewBigUint64(ctx, (uint64_t)result);
	}

	static JSValue js_mono_get_field(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "get_field(asm, class, field)");

		const char* asm_name = JS_ToCString(ctx, argv[0]);
		const char* cls_name = JS_ToCString(ctx, argv[1]);
		const char* fld_name = JS_ToCString(ctx, argv[2]);

		MonoClassField* field =
		    big::mono::get_field(cls_name, fld_name, asm_name);

		JS_FreeCString(ctx, asm_name);
		JS_FreeCString(ctx, cls_name);
		JS_FreeCString(ctx, fld_name);

		if (!field)
			return JS_NULL;

		return JS_NewBigUint64(ctx, (uint64_t)field);
	}

	static JSValue js_mono_set_field(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "set_field(objPtr, fieldPtr, valuePtr)");

		uint64_t obj_u64 = js_to_u64(ctx, argv[0]);
		uint64_t fld_u64 = js_to_u64(ctx, argv[1]);
		uint64_t val_u64 = js_to_u64(ctx, argv[2]);

		MonoObject* obj = (MonoObject*)obj_u64;
		MonoClassField* field = (MonoClassField*)fld_u64;
		void* value = (void*)val_u64;

		big::mono::set_field_value(obj, field, value);

		return JS_UNDEFINED;
	}

	static JSValue js_mono_get_field_value(JSContext* ctx, JSValueConst this_val,
		int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "get_field_value(objPtr, fieldPtr)");
		uint64_t obj_u64 = js_to_u64(ctx, argv[0]);
		uint64_t fld_u64 = js_to_u64(ctx, argv[1]);
		MonoObject* obj = (MonoObject*)obj_u64;
		MonoClassField* field = (MonoClassField*)fld_u64;
		void* out_value = nullptr;
		big::mono::get_field_value((void*)obj, field, &out_value);
		return JS_NewBigUint64(ctx, (uint64_t)out_value);
	}

	static JSValue js_mono_get_field_offset(JSContext* ctx, JSValueConst this_val,
		int argc, JSValueConst* argv)
	{
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "get_field_offset(fieldPtr)");
		uint64_t fld_u64 = js_to_u64(ctx, argv[0]);
		MonoClassField* field = (MonoClassField*)fld_u64;
		uint32_t offset = big::mono::get_field_offset(field);
		return JS_NewUint32(ctx, offset);
	}

	static JSValue js_mono_get_static_field_value(JSContext* ctx, JSValueConst this_val,
		int argc, JSValueConst* argv)
	{
		if (argc < 2)
			return JS_ThrowTypeError(ctx, "get_static_field_value(className, fieldName)");
		const char* class_name = JS_ToCString(ctx, argv[0]);
		const char* field_name = JS_ToCString(ctx, argv[1]);
		void* value = big::mono::get_static_field_value(class_name, field_name);
		JS_FreeCString(ctx, class_name);
		JS_FreeCString(ctx, field_name);
		return JS_NewBigUint64(ctx, (uint64_t)value);
	}

	static JSValue js_mono_get_static_field_data(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_static_field_data(classPtr)");
		uint64_t class_u64 = js_to_u64(ctx, argv[0]);
		MonoClass* klass = (MonoClass*)class_u64;
		void* data = big::mono::get_static_field_data(klass);
		return JS_NewBigUint64(ctx, (uint64_t)data);
	}

	static JSValue js_mono_get_static_field_data_vtable(JSContext* ctx, JSValueConst this_val,
		int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_static_field_data_vtable(vtablePtr)");
		uint64_t vtable_u64 = js_to_u64(ctx, argv[0]);
		MonoVTable* vtable = (MonoVTable*)vtable_u64;
		void* data = big::mono::get_static_field_data(vtable);
		return JS_NewBigUint64(ctx, (uint64_t)data);
	}

	static JSValue js_mono_get_vtable(JSContext* ctx, JSValueConst this_val,
		int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_vtable(classPtr)");
		uint64_t class_u64 = js_to_u64(ctx, argv[0]);
		MonoClass* klass = (MonoClass*)class_u64;
		MonoVTable* vtable = big::mono::get_vtable(klass);
		return JS_NewBigUint64(ctx, (uint64_t)vtable);
	}

	static JSValue js_mono_get_class_from_method(JSContext* ctx, JSValueConst this_val,
	    int argc, JSValueConst* argv)
	{
		if (argc < 1)
			return JS_ThrowTypeError(ctx, "get_class_from_method (methodPtr)");
		uint64_t method_u64 = js_to_u64(ctx, argv[0]);
		MonoMethod* method = (MonoMethod*)method_u64;
		MonoClass* klass = big::mono::get_class_from_method(method);
		if (!klass)
			return JS_NULL;
		return JS_NewBigUint64(ctx, (uint64_t)klass);
	}

	static JSValue js_mono_get_compile_method(JSContext* ctx, JSValueConst this_val,
		int argc, JSValueConst* argv)
	{
		if (argc < 3)
			return JS_ThrowTypeError(ctx, "get_compile_method(className, methodName, paramCount, assemblyName)");
		const char* class_name = JS_ToCString(ctx, argv[0]);
		const char* method_name = JS_ToCString(ctx, argv[1]);
		int param_count = 0;
		JS_ToInt32(ctx, &param_count, argv[2]);
		const char* assembly_name = "Assembly-CSharp";
		if (argc >= 4)
			assembly_name = JS_ToCString(ctx, argv[3]);
		void* compile_method = big::mono::get_compile_method(class_name, method_name, param_count, assembly_name);
		JS_FreeCString(ctx, class_name);
		JS_FreeCString(ctx, method_name);
		if (argc >= 4)
			JS_FreeCString(ctx, assembly_name);
		if (!compile_method)
			return JS_NULL;
		return JS_NewBigUint64(ctx, (uint64_t)compile_method);
	}

	void bind(qjs::Context& ctx)
	{
		auto& mono = ctx.addModule("mono");
		mono.function<&js_mono_get_class>("get_class");
		mono.function<&js_mono_get_method>("get_method");
		mono.function<&js_mono_invoke_method>("invoke_method");
		mono.function<&js_mono_get_field>("get_field");
		mono.function<&js_mono_set_field>("set_field");
		mono.function<&js_mono_get_field_value>("get_field_value");
		mono.function<&js_mono_get_field_offset>("get_field_offset");
		mono.function<&js_mono_get_static_field_value>("get_static_field_value");
		mono.function<&js_mono_get_static_field_data>("get_static_field_data");
		mono.function<&js_mono_get_static_field_data_vtable>("get_static_field_data_vtable");
		mono.function<&js_mono_get_vtable>("get_vtable");
		mono.function<&js_mono_get_class_from_method>("get_class_from_method");
		mono.function<&js_mono_get_compile_method>("get_compile_method");
	}
}