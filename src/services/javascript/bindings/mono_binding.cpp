#include "mono_binding.hpp"

#include "mono/mono.hpp"
#include "utility/unity.hpp"

#include <variant>

namespace js::mono
{

	/* ----------------- MONO FUNCTIONS ----------------- */

	static double js_mono_get_class(std::string const& classname, std::string const& assembly)
	{
		MonoClass* klass = big::mono::get_class(classname.c_str(), assembly.c_str());

		if (!klass)
		{
			return (double)0ull;
		}
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

		const auto parameter_types = big::mono::get_param_type_names(method);
		std::vector<std::variant<int64_t, int32_t, double, float, bool, big::MonoString*>> storage;
		storage.reserve(argv.size());
		for (std::size_t i = 0; i < argv.size(); ++i)
		{
			const auto& s = argv[i];
			const std::string_view type = i < parameter_types.size() ? std::string_view(parameter_types[i]) : std::string_view();
			int64_t iv;
			double dv;
			bool bv;

			if (type == "System.Single" && big::unity::is_double(s, dv))
			{
				storage.emplace_back(static_cast<float>(dv));
				args.push_back(&std::get<float>(storage.back()));
			}
			else if (type == "System.Double" && big::unity::is_double(s, dv))
			{
				storage.emplace_back(dv);
				args.push_back(&std::get<double>(storage.back()));
			}
			else if (type == "System.Int32" && big::unity::is_int(s, iv))
			{
				storage.emplace_back(static_cast<int32_t>(iv));
				args.push_back(&std::get<int32_t>(storage.back()));
			}
			else if (big::unity::is_bool(s, bv))
			{
				storage.emplace_back(bv);
				args.push_back(&std::get<bool>(storage.back()));
			}
			else if (big::unity::is_int(s, iv))
			{
				storage.emplace_back(iv);
				args.push_back(&std::get<int64_t>(storage.back()));
			}
			else if (big::unity::is_double(s, dv))
			{
				storage.emplace_back(dv);
				args.push_back(&std::get<double>(storage.back()));
			}
			else
			{
				storage.emplace_back(big::mono::to_mono_string(s));
				args.push_back(std::get<big::MonoString*>(storage.back()));
			}
		}

		MonoObject* result = big::mono::invoke_method(method, obj, args.empty() ? nullptr : args.data());

		return static_cast<double>((uintptr_t)result);
	}

	static double js_mono_get_field(double klass, std::string const& fieldname)
	{
		auto mono_class       = reinterpret_cast<MonoClass*>((uintptr_t)klass);
		MonoClassField* field = big::mono::get_field(mono_class, fieldname.c_str());

		return static_cast<double>((uintptr_t)field);
	}

	static void js_mono_set_field_int(double obj_val, double field_val, double argv)
	{
		auto obj   = reinterpret_cast<MonoObject*>((uintptr_t)obj_val);
		auto field = reinterpret_cast<MonoClassField*>((uintptr_t)field_val);
		auto value = (int)argv;

		big::mono::set_field_value(obj, field, &value);
	}

	static void js_mono_set_field_float(double obj_val, double field_val, double argv)
	{
		auto obj   = reinterpret_cast<MonoObject*>((uintptr_t)obj_val);
		auto field = reinterpret_cast<MonoClassField*>((uintptr_t)field_val);
		auto value = (float)argv;

		big::mono::set_field_value(obj, field, &value);
	}

	static void js_mono_set_field_string(double obj_val, double field_val, std::string const& argv)
	{
		auto obj   = reinterpret_cast<MonoObject*>((uintptr_t)obj_val);
		auto field = reinterpret_cast<MonoClassField*>((uintptr_t)field_val);
		big::mono::set_field_value(obj, field, big::mono::to_mono_string(argv));
	}

	static double js_mono_get_field_value(double obj_val, double field_val)
	{
		auto obj   = reinterpret_cast<MonoObject*>((uintptr_t)obj_val);
		auto field = reinterpret_cast<MonoClassField*>((uintptr_t)field_val);

		std::vector<std::uint8_t> out_value(std::max<std::size_t>(big::mono::get_field_size(field), sizeof(uintptr_t)));
		big::mono::get_field_value(obj, field, out_value.data());

		uintptr_t value{};
		std::memcpy(&value, out_value.data(), sizeof(value));
		return (double)value;
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
		void* data       = big::mono::get_static_field_data(klass);
		return (double)(uintptr_t)data;
	}

	// Mengembalikan double (pointer)
	static double js_mono_get_static_field_data_vtable(double vtable_u64)
	{
		MonoVTable* vtable = (MonoVTable*)(uintptr_t)vtable_u64;
		void* data         = big::mono::get_static_field_data(vtable);
		return (double)(uintptr_t)data;
	}

	// Mengembalikan double (pointer)
	static double js_mono_get_vtable(double class_u64)
	{
		MonoClass* klass   = (MonoClass*)(uintptr_t)class_u64;
		MonoVTable* vtable = big::mono::get_vtable(klass);
		return (double)(uintptr_t)vtable;
	}

	// Mengembalikan double (pointer)
	static double js_mono_get_class_from_method(double method_u64)
	{
		MonoMethod* method = (MonoMethod*)(uintptr_t)method_u64;
		MonoClass* klass   = big::mono::get_class_from_method(method);
		if (!klass)
		{
			return 0.0;
		}
		return (double)(uintptr_t)klass;
	}

	static double js_mono_get_compile_method(std::string const& classname, std::string const& method_name, double count, std::string const& assembly_name = "Assembly-CSharp")
	{
		int param_count = (int)count;
		void* compile_method =
		    big::mono::get_compile_method(classname.c_str(), method_name.c_str(), param_count, assembly_name.c_str());

		return (double)(uintptr_t)compile_method;
	}

	static double js_mono_unbox(double obj_ptr)
	{
		auto obj = reinterpret_cast<MonoObject*>((uintptr_t)obj_ptr);

		if (!obj)
		{
			return 0.0;
		}

		void* data = big::mono::object_unbox(obj);

		return (double)(uintptr_t)data;
	}
	static std::shared_ptr<big::Vector3> js_mono_unbox_vec3(double obj_ptr)
	{
		auto obj = reinterpret_cast<MonoObject*>((uintptr_t)obj_ptr);
		if (!obj)
		{
			return {};
		}

		big::Vector3* v = (big::Vector3*)big::mono::object_unbox(obj);
		return v ? std::make_shared<big::Vector3>(*v) : std::shared_ptr<big::Vector3>{};
	}
	struct MonoBindings
	{
		static double get_class(std::string const& name, std::string const& assembly)
		{
			return js_mono_get_class(name, assembly);
		}
		static double get_method(std::string const& type, std::string const& name, double count, std::string const& assembly)
		{
			return js_mono_get_method(type, name, count, assembly);
		}
		static double invoke_method(double method, double object, const qjs::rest<std::string>& args)
		{
			return js_mono_invoke_method(method, object, args);
		}
		static double get_field(double type, std::string const& name)
		{
			return js_mono_get_field(type, name);
		}
		static void set_field_int(double object, double field, double value)
		{
			js_mono_set_field_int(object, field, value);
		}
		static void set_field_float(double object, double field, double value)
		{
			js_mono_set_field_float(object, field, value);
		}
		static void set_field_string(double object, double field, std::string const& value)
		{
			js_mono_set_field_string(object, field, value);
		}
		static double get_field_value(double object, double field)
		{
			return js_mono_get_field_value(object, field);
		}
		static double get_field_offset(double field)
		{
			return js_mono_get_field_offset(field);
		}
		static double get_static_field_value(std::string const& type, std::string const& field)
		{
			return js_mono_get_static_field_value(type, field);
		}
		static double get_static_field_data(double type)
		{
			return js_mono_get_static_field_data(type);
		}
		static double get_static_field_data_vtable(double table)
		{
			return js_mono_get_static_field_data_vtable(table);
		}
		static double get_vtable(double type)
		{
			return js_mono_get_vtable(type);
		}
		static double get_class_from_method(double method)
		{
			return js_mono_get_class_from_method(method);
		}
		static double get_compile_method(std::string const& type, std::string const& name, double count, std::string const& assembly)
		{
			return js_mono_get_compile_method(type, name, count, assembly);
		}
		static double unbox(double object)
		{
			return js_mono_unbox(object);
		}
		static std::shared_ptr<big::Vector3> unbox_vec3(double object)
		{
			return js_mono_unbox_vec3(object);
		}
		static double get_root_domain()
		{
			return (double)(uintptr_t)big::mono::get_root_domain();
		}
	};

	static double js_mono_get_root_domain()
	{
		return (double)(uintptr_t)big::mono::get_root_domain();
	}

	// Register as global.mono (compatible with existing code that uses global object)
	void bind(qjs::Context& context)
	{
		auto mono_object = context.newObject();
		auto global      = context.global();
		auto& module     = context.addModule("mono");
		module.class_<MonoBindings>("Mono")
		    .constructor<>("Mono")
		    .static_fun<&MonoBindings::get_root_domain>("get_root_domain")
		    .static_fun<&MonoBindings::get_class>("get_class")
		    .static_fun<&MonoBindings::get_method>("get_method")
		    .static_fun<&MonoBindings::get_field>("get_field")
		    .static_fun<&MonoBindings::invoke_method>("invoke_method")
		    .static_fun<&MonoBindings::set_field_int>("set_field_int")
		    .static_fun<&MonoBindings::set_field_float>("set_field_float")
		    .static_fun<&MonoBindings::set_field_string>("set_field_string")
		    .static_fun<&MonoBindings::get_field_value>("get_field_value")
		    .static_fun<&MonoBindings::get_field_offset>("get_field_offset")
		    .static_fun<&MonoBindings::get_static_field_value>("get_static_field_value")
		    .static_fun<&MonoBindings::get_static_field_data>("get_static_field_data")
		    .static_fun<&MonoBindings::get_static_field_data_vtable>("get_static_field_data_vtable")
		    .static_fun<&MonoBindings::get_vtable>("get_vtable")
		    .static_fun<&MonoBindings::get_class_from_method>("get_class_from_method")
		    .static_fun<&MonoBindings::get_compile_method>("get_compile_method")
		    .static_fun<&MonoBindings::unbox>("unbox")
		    .static_fun<&MonoBindings::unbox_vec3>("unbox_vec3");

		mono_object.add<&js_mono_unbox_vec3>("unbox_vec3");
		mono_object.add<&js_mono_get_root_domain>("get_root_domain");

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

		module.add("mono", qjs::Value{mono_object});
		global["mono"] = mono_object;

		big::mono::thread_attach(big::mono::get_root_domain());
	}
} // namespace js::mono
