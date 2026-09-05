#pragma once
#include "mono_functions.hpp"
#include "memory/module.hpp"
#include "cache.hpp"
#include "array.hpp"
#include <unordered_map>

namespace big
{
	class mono
	{
		void init_impl();
		MonoObject* invoke_method_impl(MonoMethod* method, void* obj, void** params) const;
		void* get_compile_method_impl(const char* className, const char* methodName, int param_count, const char* assemblyName, const char* nameSpace) const;
        MonoMethod* get_method_impl(const char* className, const char* methodName, int param_count, const char* assemblyName, const char* nameSpace) const;
        MonoClass* get_class_impl(const char* className, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "") const;
        MonoClass* get_class_from_method_impl(MonoMethod* method) const;
        MonoClassField* get_field_impl(const char* className, const char* fieldName, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "") const;
		MonoClassField* get_field_impl(MonoClass* pKlass, const char* fieldName) const;
		uint32_t get_field_offset_impl(MonoClassField* field) const;
		void get_field_value_impl(void* instance, MonoClassField* field, void* out) const;
		void set_field_value_impl(MonoObject* obj, MonoClassField* field, void* value);
		MonoVTable* get_vtable_impl(MonoClass* pKlass) const;
        void* get_static_field_data_impl(MonoVTable* pVTable) const;
        void* get_static_field_data_impl(MonoClass* pKlass) const;
		void* get_static_field_value_impl(const char* className, const char* fieldName, const char* assemblyName, const char* nameSpace) const;
		void* mono_object_unbox_impl(MonoObject* obj);
		MonoThread* mono_thread_attach_impl(MonoDomain* domain) const;
		MonoDomain* get_root_domain_impl() const;
		std::string from_mono_string_impl(MonoString* monoStr) const;
		std::wstring_view view_mono_string_impl(MonoString* monoStr) const;
		MonoString* to_mono_string_utf16(std::string const& str);
		std::filesystem::path get_assembly_path(const char* assemblyName) const;
		MonoImage* get_image_impl(const char* assemblyName) const;
		static mono& get_instance()
        {
            static mono instance;
            return instance;
		}
	public:
        static void init() { get_instance().init_impl(); };
		static std::string from_mono_string(MonoString* monoStr) { return get_instance().from_mono_string_impl(monoStr); };
		static std::wstring_view view_mono_string(MonoString* monoStr) { return get_instance().view_mono_string_impl(monoStr); };
		static MonoString* to_mono_string(std::string const& str) { return get_instance().to_mono_string_utf16(str); }
        static MonoObject* invoke_method(MonoMethod* method, void* obj = nullptr, void** params = nullptr)
        {
            return get_instance().invoke_method_impl(method, obj, params);
		};
		template<typename... Args>
		static MonoObject* invoke(MonoMethod* method, void* obj, Args... args) 
		{
			if constexpr (sizeof...(args) > 0)
			{
				void* params[] = { &args... };
				return invoke_method(method, obj, params);
			} 
			else 
			{
				return invoke_method(method, obj, nullptr); // Sesuai kebutuhan Mono
			}
		}
        static void* get_compile_method(const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
        {
            return get_instance().get_compile_method_impl(className, methodName, param_count, assemblyName, nameSpace);
		};
        static MonoMethod* get_method(const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
        {
			return get_instance().get_method_impl(className, methodName, param_count, assemblyName, nameSpace);
		};
        static MonoClass* get_class(const char* className, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
		{
			return get_instance().get_class_impl(className, assemblyName, nameSpace);
		};
        static MonoClass* get_class_from_method(MonoMethod* method)
		{
			return get_instance().get_class_from_method_impl(method);
		};
		static MonoClassField* get_field(const char* className, const char* fieldName, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
		{
			return get_instance().get_field_impl(className, fieldName, assemblyName, nameSpace);
		};
		static MonoClassField* get_field(MonoClass* pKlass, const char* fieldName)
		{
			return get_instance().get_field_impl(pKlass, fieldName);
		};
		static uint32_t get_field_offset(MonoClassField* field)
		{
			return get_instance().get_field_offset_impl(field);
		};
		static void get_field_value(void* instance, MonoClassField* field, void* out)
		{
			get_instance().get_field_value_impl(instance, field, out);
		};
		static void set_field_value(MonoObject* obj, MonoClassField* field, void* value)
		{
			get_instance().set_field_value_impl(obj, field, value);
		};
		static MonoVTable* get_vtable(MonoClass* pKlass)
		{
			return get_instance().get_vtable_impl(pKlass);
		};
		static void* get_static_field_data(MonoVTable* pVTable)
		{
			return get_instance().get_static_field_data_impl(pVTable);
		};
		static void* get_static_field_data(MonoClass* pKlass)
		{
			return get_instance().get_static_field_data_impl(pKlass);
		};
		static void* get_static_field_value(const char* className, const char* fieldName, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
		{
			return get_instance().get_static_field_value_impl(className, fieldName, assemblyName, nameSpace);
		};
		static MonoThread* thread_attach(MonoDomain* domain)
		{
			return get_instance().mono_thread_attach_impl(domain);
		}
		static MonoDomain* get_root_domain()
		{
			return get_instance().get_root_domain_impl();
		}
		static void* object_unbox(MonoObject* obj)
		{
			return get_instance().mono_object_unbox_impl(obj);
		}
        static bool is_initialized() { return get_instance().initalized; };
		template<const_str Class, const_str Field, typename T>
		static T get_static_field_value()
		{
			static auto klass = mono::get_class(Class.value, "assembly_valheim");
			static auto field = mono::get_field(klass, Field.value);

			void* static_field = mono::get_static_field_data(klass);

			uint32_t offset = mono::get_field_offset(field);
			void* address = (void*)((uintptr_t)static_field + offset);

			MonoObject* value = *(MonoObject**)address;

			if constexpr (std::is_same_v<T, MonoObject*>)
        		return value;

			return T(value);
		}
		template<const_str Class, const_str Field, typename T>
		static T get_field_value(MonoObject* obj)
		{
			static auto klass = get_class(Class.value, "assembly_valheim");
			static auto field = get_field(klass, Field.value);

			if constexpr (std::is_same_v<T, std::string>)
			{
				MonoString* out{};
				get_field_value(obj, field, out);

				return from_mono_string(out);
			}

			T out{};
			get_field_value(obj, field, &out);

			return out;
		}
		template<const_str Class, const_str Field, typename T>
		static bool set_field_value(MonoObject* obj, T&& value)
		{
			static auto klass = get_class(Class.value, "assembly_valheim");
			static auto field = get_field(klass, Field.value);


			if (!klass || !field)
				return false;

			auto temp = std::forward<T>(value);
			
			set_field_value(obj, field, &temp);

			return true;
		}
		template<typename T = MonoObject*>
		static mono_array_view<T> list(MonoObject* list)
		{
			if (!list)
				return {};

			auto klass = object_get_class(list);

			if (!klass)
				return {};

			static MonoClassField* items_field = get_field(klass, "_items");
			static MonoClassField* size_field  = get_field(klass, "_size");

			if (!items_field || !size_field)
				return {};

			MonoObject* items{};
			int size{};

			get_field_value(list, items_field, &items);
			get_field_value(list, size_field, &size);

			if (!items || size <= 0)
				return {};

			return mono_array_view<T>(
				reinterpret_cast<MonoArray*>(items),
				size
			);
		}
		template<typename T = MonoObject*>
		static std::vector<T> from_list(MonoObject* list)
		{
			std::vector<T> out;
			if (!list)
				return out;

			auto klass = object_get_class(list);
			if (!klass)
				return out;

			static auto getCount = class_get_method_from_name(klass, "get_Count", 0);
			static auto getItem  = class_get_method_from_name(klass, "get_Item", 1);

			if (!getCount || !getItem)
				return out;

			auto ret = invoke_method(getCount, list);
			if (!ret)
				return out;

			int count = *(int*)object_unbox(ret);

			out.reserve(count);

			for (int i = 0; i < count; ++i)
			{
				void* args[1] = { &i };
				auto item = invoke_method(getItem, list, args);

				if (!item)
					continue;

				if constexpr (std::is_same_v<T, MonoObject*>)
				{
					out.push_back(item);
				}
				else
				{
					out.emplace_back(item);
				}
			}

			return out;
		}
	private:
		bool initalized = false;

	public:
		static MonoClass* object_get_class(MonoObject* obj)
		{
			return get_instance().mono_object_get_class(obj);
		}
		static MonoMethod* class_get_method_from_name(MonoClass* obj, const char* name, int param)
		{
			return get_instance().mono_class_get_method_from_name(obj, name, param);
		}
		static MonoImage* assembly_get_image(MonoAssembly* assembly)
		{
			return get_instance().mono_assembly_get_image(assembly);
		}
		static MonoAssembly* domain_assembly_open(MonoDomain* domain, const char* name)
		{
			return get_instance().mono_domain_assembly_open(domain, name);
		}
		static MonoClass* get_class_from_name(MonoImage* image, const char* name_space, const char* name)
		{
			return get_instance().mono_class_from_name(image, name_space, name);
		}
		static const char* class_get_name(MonoClass* klass)
		{
			return get_instance().mono_class_get_name(klass);
		}
		static const char* class_get_namespace(MonoClass* klass)
		{
			return get_instance().mono_class_get_namespace(klass);
		}
		static void* array_with_size(MonoArray* array, int size, uintptr_t idx)
		{
			return get_instance().mono_array_addr_with_size(array, size, idx);
		}
		static int array_length(MonoArray* array)
		{
			return get_instance().mono_array_length(array);
		}
		static void free(void* ptr) { get_instance().mono_free(ptr); }
		static std::string_view get_name(MonoObject* obj)
		{
			MonoClass* elem_class = mono::object_get_class(obj);
			return mono::class_get_name(elem_class);
		}
	private:
        // --- Member untuk Fungsi Runtime dan Domain (Menggunakan alias _t) ---

        mono_thread_attach_t mono_thread_attach = nullptr;
        mono_get_root_domain_t mono_get_root_domain = nullptr;
        mono_domain_assembly_open_t mono_domain_assembly_open = nullptr;
        mono_assembly_get_image_t mono_assembly_get_image = nullptr;
        mono_class_from_name_t mono_class_from_name = nullptr;
        mono_class_get_method_from_name_t mono_class_get_method_from_name = nullptr;
        mono_compile_method_t mono_compile_method = nullptr;
        mono_runtime_invoke_t mono_runtime_invoke = nullptr;
		mono_object_unbox_t mono_object_unbox = nullptr;
		mono_object_get_class_t mono_object_get_class = nullptr;

        // --- Member untuk Fungsi Class dan Field ---

        mono_class_get_field_from_name_t mono_class_get_field_from_name = nullptr;
        mono_field_get_value_t mono_field_get_value = nullptr;
        mono_field_set_value_t mono_field_set_value = nullptr;
        mono_method_get_class_t mono_method_get_class = nullptr;
        mono_class_vtable_t mono_class_vtable = nullptr;
        mono_vtable_get_static_field_data_t mono_vtable_get_static_field_data = nullptr;
        mono_field_get_offset_t mono_field_get_offset = nullptr;
		mono_class_get_name_t mono_class_get_name = nullptr;
		mono_class_get_namespace_t mono_class_get_namespace = nullptr;
		mono_string_to_utf8_t mono_string_to_utf8 = nullptr;
		mono_string_new_utf16_t mono_string_new_utf16 = nullptr;
		mono_array_addr_with_size_t mono_array_addr_with_size = nullptr;
		mono_array_length_t mono_array_length = nullptr;
		mono_free_t mono_free = nullptr;

		// mono_domain_assembly_open increments/retains runtime assembly state.  Do not
		// call it for every entity getter; images are stable for the domain lifetime.
		mutable std::mutex m_image_cache_mutex;
		mutable std::unordered_map<std::string, MonoImage*> m_image_cache;
	};
}
