#include "mono_binding.hpp"
#include "mono/mono.hpp"
#include "quickjs.h"

#include "utility/unity.hpp"

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

	static double js_mono_get_class(std::string const& classname, std::string const& assembly)
	{
		MonoClass* klass = big::mono::get_class(classname.c_str(), assembly.c_str());

		if (!klass)
			return (double)0ull;
		return (double)(uintptr_t)klass;
	}

	static double js_mono_get_method(std::string const& classname, std::string const& method_name, double count, std::string const& assembly)
	{
		int param_count = (int)count;

		MonoMethod* method = big::mono::get_method(classname.c_str(), method_name.c_str(), param_count, assembly.c_str());

		return static_cast<double>((uintptr_t)method);
	}

	static double js_mono_invoke_method(double method_ptr, double obj_ptr, const qjs::rest<std::string>& argv)
	{
		MonoMethod* method = (MonoMethod*)(uintptr_t)method_ptr;

		void* obj = reinterpret_cast<void*>((uintptr_t)obj_ptr);

		std::vector<void*> args;

		for (auto& s : argv)
		{
			int64_t iv;
			double dv;
			bool bv;
			std::string ms;

			if (big::unity::is_bool(s, bv))
			{
				args.push_back(&bv);
			}
			else if (big::unity::is_int(s, iv))
			{
				args.push_back(&dv);
			}
			else if (big::unity::is_double(s, dv))
			{
				args.push_back(&iv);
			}
			else
			{
				ms = s;
				args.push_back(&ms);
			}
		}

		MonoObject* result = big::mono::invoke_method(method, obj, args.empty() ? nullptr : args.data());

		return static_cast<double>((uintptr_t)result);
	}

	static double js_mono_get_field(double klass, std::string const& fieldname)
	{
		auto mono_class = reinterpret_cast<MonoClass*>((uintptr_t)klass);
		MonoClassField* field = big::mono::get_field(mono_class, fieldname.c_str());

		return static_cast<double>((uintptr_t)field);
	}

	static void js_mono_set_field_int(double obj_val, double field_val, double argv)
	{
		auto obj = reinterpret_cast<MonoObject*>((uintptr_t)obj_val);
		auto field = reinterpret_cast<MonoClassField*>((uintptr_t)field_val);
		auto value = (int)argv;

		big::mono::set_field_value(obj, field, &value);
	}

	static void js_mono_set_field_float(double obj_val, double field_val, double argv)
	{
		auto obj = reinterpret_cast<MonoObject*>((uintptr_t)obj_val);
		auto field = reinterpret_cast<MonoClassField*>((uintptr_t)field_val);
		auto value = (float)argv;

		big::mono::set_field_value(obj, field, &value);
	}

	static void js_mono_set_field_string(double obj_val, double field_val, std::string const& argv)
	{
		auto obj = reinterpret_cast<MonoObject*>((uintptr_t)obj_val);
		auto field = reinterpret_cast<MonoClassField*>((uintptr_t)field_val);
		auto value = argv.c_str();

		big::mono::set_field_value(obj, field, &value);
	}

	static double js_mono_get_field_value(double obj_val, double field_val)
	{
		auto obj = reinterpret_cast<MonoObject*>((uintptr_t)obj_val);
		auto field = reinterpret_cast<MonoClassField*>((uintptr_t)field_val);

		void* out_value = nullptr;
		big::mono::get_field_value(obj, field, &out_value);

		return (double)(uintptr_t)out_value;
	}
	static double js_mono_get_field_offset(double field_u64)
	{
		// ... (Logika sama)
		uint32_t offset = big::mono::get_field_offset((MonoClassField*)(uintptr_t)field_u64);
		return (double)offset;
	}

	// Mengembalikan double (pointer)
	static double js_mono_get_static_field_value(std::string const& class_name, std::string const& field_name)
	{
		void* value = big::mono::get_static_field_value(class_name.c_str(), field_name.c_str());
		return (double)(uintptr_t)value;
	}

	// Mengembalikan double (pointer)
	static double js_mono_get_static_field_data(double class_u64)
	{
		MonoClass* klass = (MonoClass*)(uintptr_t)class_u64;
		void* data = big::mono::get_static_field_data(klass);
		return (double)(uintptr_t)data;
	}

	// Mengembalikan double (pointer)
	static double js_mono_get_static_field_data_vtable(double vtable_u64)
	{
		MonoVTable* vtable = (MonoVTable*)(uintptr_t)vtable_u64;
		void* data = big::mono::get_static_field_data(vtable);
		return (double)(uintptr_t)data;
	}

	// Mengembalikan double (pointer)
	static double js_mono_get_vtable(double class_u64)
	{
		MonoClass* klass = (MonoClass*)(uintptr_t)class_u64;
		MonoVTable* vtable = big::mono::get_vtable(klass);
		return (double)(uintptr_t)vtable;
	}

	// Mengembalikan double (pointer)
	static double js_mono_get_class_from_method(double method_u64)
	{
		MonoMethod* method = (MonoMethod*)(uintptr_t)method_u64;
		MonoClass* klass = big::mono::get_class_from_method(method);
		if (!klass)
			return 0.0;
		return (double)(uintptr_t)klass;
	}

	static double js_mono_get_compile_method(std::string const& classname, std::string const& method_name, double count, std::string const& assembly_name = "Assembly-CSharp")
	{
		int param_count = (int)count;
		void* compile_method = big::mono::get_compile_method(classname.c_str(), method_name.c_str(), param_count, assembly_name.c_str());

		return (double)(uintptr_t)compile_method;
	}

	static double js_mono_unbox(double obj_ptr)
	{
		auto obj = reinterpret_cast<MonoObject*>((uintptr_t)obj_ptr);

		if (!obj)
			return 0.0;

		void* data = big::mono::object_unbox(obj);

		return (double)(uintptr_t)data;
	}

	static MonoObject* list_get(MonoObject* list, int index)
	{
		MonoClass* klass = mono_object_get_class(list);

		MonoMethod* getItem = mono_class_get_method_from_name(klass, "get_Item", 1);

		if (!getItem)
			return nullptr;

		void* args[1];
		args[0] = &index;

		MonoObject* exc = nullptr;
		return mono_runtime_invoke(getItem, list, args, &exc);
	}

	/* ----------------- REGISTER helpers ----------------- */

#define MONO_SET_FUNC2(target_obj, name, fn, argc) \
	JS_SetPropertyStr(ctx, target_obj, name, JS_NewCFunction(ctx, fn, name, argc))

	// Register as actual ES module 'mono' so import * as m from 'mono' works.
	// Call js_init_module_mono(ctx, "mono") once before evaluating any module that imports it.
	static int js_mono_init(JSContext* ctx, JSModuleDef* m)
	{
		/*JS_SetModuleExport(ctx, m, "get_class", JS_NewCFunction(ctx, js_mono_get_class, "get_class", 2));
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
		JS_SetModuleExport(ctx, m, "get_compile_method", JS_NewCFunction(ctx, js_mono_get_compile_method, "get_compile_method", 4));*/
		return 0;
	}

	static JSModuleDef* js_init_module_mono(JSContext* ctx, const char* name)
	{
		/*JSModuleDef* m = JS_NewCModule(ctx, name, js_mono_init);
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
		JS_AddModuleExport(ctx, m, "get_compile_method");*/

		//return m;
	}

	
	// Register as global.mono (compatible with existing code that uses global object)
	void bind(qjs::Context& context)
	{
		auto ctx = context.ctx;
		
		auto mono_object = context.newObject();
		auto global = context.global();

		// Getters Class/Method/Field
		mono_object.add<&js_mono_get_class>("get_class");
		mono_object.add<&js_mono_get_method>("get_method");
		mono_object.add<&js_mono_get_field>("get_field");

		// Invoker
		mono_object.add<&js_mono_invoke_method>("invoke_method");

		// Field Set/Get
		mono_object.add<&js_mono_set_field_int>("set_field_int");
		mono_object.add<&js_mono_set_field_float>("set_field_float");
		mono_object.add<&js_mono_set_field_string>("set_field_string");
		mono_object.add<&js_mono_get_field_value>("get_field_value");
		mono_object.add<&js_mono_get_field_offset>("get_field_offset");

		// Static Field & VTable Getters
		mono_object.add<&js_mono_get_static_field_value>("get_static_field_value");
		mono_object.add<&js_mono_get_static_field_data>("get_static_field_data");
		mono_object.add<&js_mono_get_static_field_data_vtable>("get_static_field_data_vtable");

		// Utility Mono
		mono_object.add<&js_mono_get_vtable>("get_vtable");
		mono_object.add<&js_mono_get_class_from_method>("get_class_from_method");
		mono_object.add<&js_mono_get_compile_method>("get_compile_method");
		mono_object.add<&js_mono_unbox>("unbox");

		global["mono"] = mono_object;

		big::mono::thread_attach(big::mono::get_root_domain());
	}
} // namespace js::mono