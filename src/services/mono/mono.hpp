#pragma once
#include "mono_functions.hpp"

namespace big
{
	class mono
	{
		void init_impl();
		MonoObject* invoke_method_impl(MonoMethod* method, void* obj, void** params) const;
		void* get_compile_method_impl(const char* className, const char* methodName, int param_count, const char* assemblyName) const;
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
		void* get_static_field_value_impl(const char* className, const char* fieldName);
		std::filesystem::path get_assembly_path(const char* assemblyName) const;
        static mono& get_instance()
        {
            static mono instance;
            return instance;
		}
	public:
        static void init() { get_instance().init_impl(); };
        static MonoObject* invoke_method(MonoMethod* method, void* obj = nullptr, void** params = nullptr)
        {
            return get_instance().invoke_method_impl(method, obj, params);
		};
        static void* get_compile_method(const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp")
        {
            return get_instance().get_compile_method_impl(className, methodName, param_count, assemblyName);
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
		static void* get_static_field_value(const char* className, const char* fieldName)
		{
			return get_instance().get_static_field_value_impl(className, fieldName);
		};
        static bool is_initialized() { return get_instance().initalized; };
	private:
		bool initalized = false;
		HMODULE m_mono = nullptr;
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

        // --- Member untuk Fungsi Class dan Field ---

        mono_class_get_field_from_name_t mono_class_get_field_from_name = nullptr;
        mono_field_get_value_t mono_field_get_value = nullptr;
        mono_field_set_value_t mono_field_set_value = nullptr;
        mono_method_get_class_t mono_method_get_class = nullptr;
        mono_class_vtable_t mono_class_vtable = nullptr;
        mono_vtable_get_static_field_data_t mono_vtable_get_static_field_data = nullptr;
        mono_field_get_offset_t mono_field_get_offset = nullptr;
	};
}