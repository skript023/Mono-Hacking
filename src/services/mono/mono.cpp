#pragma warning (disable:4311 4312)
#include "mono.hpp"
#include "core.hpp"

namespace big
{
	void mono::init_impl()
	{
		// Implementation details for initializing Mono
		auto module = memory::module("mono-2.0-bdwgc.dll");

		// Necessary functions to get method addresses
		mono_domain_assembly_open = module.get_export("mono_domain_assembly_open").as<mono_domain_assembly_open_t>();
		mono_assembly_get_image = module.get_export("mono_assembly_get_image").as<mono_assembly_get_image_t>();
		mono_class_from_name = module.get_export("mono_class_from_name").as<mono_class_from_name_t>();
		mono_class_get_method_from_name = module.get_export("mono_class_get_method_from_name").as<mono_class_get_method_from_name_t>();
		mono_compile_method = module.get_export("mono_compile_method").as<mono_compile_method_t>();
		mono_runtime_invoke = module.get_export("mono_runtime_invoke").as<mono_runtime_invoke_t>();
		mono_object_unbox = module.get_export("mono_object_unbox").as<mono_object_unbox_t>();
		mono_object_get_class = module.get_export("mono_object_get_class").as<mono_object_get_class_t>();

		mono_class_get_field_from_name = module.get_export("mono_class_get_field_from_name").as<mono_class_get_field_from_name_t>();
		mono_field_get_value = module.get_export("mono_field_get_value").as<mono_field_get_value_t>();
		mono_field_set_value = module.get_export("mono_field_set_value").as<mono_field_set_value_t>();
		mono_method_get_class = module.get_export("mono_method_get_class").as<mono_method_get_class_t>();
		mono_class_vtable = module.get_export("mono_class_vtable").as<mono_class_vtable_t>();
		mono_vtable_get_static_field_data = module.get_export("mono_vtable_get_static_field_data").as<mono_vtable_get_static_field_data_t>();
		mono_field_get_offset = module.get_export("mono_field_get_offset").as<mono_field_get_offset_t>();
		mono_class_get_name = module.get_export("mono_class_get_name").as<mono_class_get_name_t>();
		mono_class_get_namespace = module.get_export("mono_class_get_namespace").as<mono_class_get_namespace_t>();
		mono_string_to_utf8 = module.get_export("mono_string_to_utf8").as<mono_string_to_utf8_t>();
		mono_string_new_utf16 = module.get_export("mono_string_new_utf16").as<mono_string_new_utf16_t>();
		mono_array_addr_with_size = module.get_export("mono_array_addr_with_size").as<mono_array_addr_with_size_t>();
		mono_array_length = module.get_export("mono_array_length").as<mono_array_length_t>();
		mono_core::mono_array_addr_with_size = module.get_export("mono_array_addr_with_size").as<mono_core::mono_array_addr_with_size_t>();
		mono_core::mono_array_length = module.get_export("mono_array_length").as<mono_core::mono_array_length_t>();
		mono_free = module.get_export("mono_free").as<mono_free_t>();

		// Attach thread to prevent crashes
		mono_thread_attach = module.get_export("mono_thread_attach").as<mono_thread_attach_t>();
		mono_get_root_domain = module.get_export("mono_get_root_domain").as<mono_get_root_domain_t>();

		// Melampirkan thread ini ke domain Mono root agar aman
		mono_thread_attach(mono_get_root_domain());

		this->initalized = true;
	}
	MonoObject* mono::invoke_method_impl(MonoMethod* method, void* obj, void** params) const
	{
		static thread_local bool attached = false;

		if (!attached)
		{
			mono_thread_attach(mono_get_root_domain());
			attached = true;
		}

		MonoObject* execution;

		return mono_runtime_invoke(method, obj, params, &execution);
	}
	void* mono::get_compile_method_impl(const char* className, const char* methodName, int param_count, const char* assemblyName, const char* nameSpace) const
	{
		MonoDomain* domain = mono_get_root_domain();
		if (domain == nullptr)
		{
			LOG(WARNING) << "Failed to get Mono root domain.";

			return nullptr;
		}

		MonoAssembly* assembly = mono_domain_assembly_open(domain, get_assembly_path(assemblyName).string().c_str());
		if (assembly == nullptr)
		{
			LOG(WARNING) << "Failed to open assembly: " << assemblyName;

			return nullptr;
		}

		MonoImage* image = mono_assembly_get_image(assembly);
		if (image == nullptr)
		{
			LOG(WARNING) << "Failed to get image from assembly: " << assemblyName;

			return nullptr;
		}

		MonoClass* klass = mono_class_from_name(image, nameSpace, className);
		if (klass == nullptr)
		{
			LOG(WARNING) << "Failed to get class: " << className << "method name: " << methodName << " from assembly: " << assemblyName;

			return nullptr;
		}

		MonoMethod* method = mono_class_get_method_from_name(klass, methodName, param_count);
		if (method == nullptr)
		{
			LOG(WARNING) << "Failed to get method: " << methodName;

			return nullptr;
		}

		return mono_compile_method(method);
	}
	MonoMethod* mono::get_method_impl(const char* className, const char* methodName, int param_count, const char* assemblyName, const char* nameSpace) const
	{
		MonoDomain* domain = mono_get_root_domain();
		if (domain == nullptr)
		{
			LOG(WARNING) << "Failed to get Mono root domain.";

			return nullptr;
		}

		MonoAssembly* assembly = mono_domain_assembly_open(domain, get_assembly_path(assemblyName).string().c_str());
		if (assembly == nullptr)
		{
			LOG(WARNING) << "Failed to open assembly: " << assemblyName;

			return nullptr;
		}

		MonoImage* image = mono_assembly_get_image(assembly);
		if (image == nullptr)
		{
			LOG(WARNING) << "Failed to get image from assembly: " << assemblyName;

			return nullptr;
		}

		MonoClass* klass = mono_class_from_name(image, nameSpace, className);
		if (klass == nullptr)
		{
			LOG(WARNING) << "Failed to get class: " << className << "method name: " << methodName << " from assembly: " << assemblyName;

			return nullptr;
		}

		return mono_class_get_method_from_name(klass, methodName, param_count);
	}
	MonoClass* mono::get_class_impl(const char* className, const char* assemblyName, const char* nameSpace) const
	{
		MonoDomain* domain = mono_get_root_domain();
		if (domain == nullptr)
			return nullptr;

		MonoAssembly* assembly = mono_domain_assembly_open(domain, get_assembly_path(assemblyName).string().c_str());
		if (assembly == nullptr)
			return nullptr;

		MonoImage* image = mono_assembly_get_image(assembly);
		if (image == nullptr)
			return nullptr;

		MonoClass* klass = mono_class_from_name(image, nameSpace, className);

		return klass;
	}
	MonoClass* mono::get_class_from_method_impl(MonoMethod* method) const
	{
		return mono_method_get_class(method);
	}
	MonoClassField* mono::get_field_impl(const char* className, const char* fieldName, const char* assemblyName, const char* nameSpace) const
	{
		MonoDomain* domain = mono_get_root_domain();
		if (domain == nullptr)
			return nullptr;

		MonoAssembly* assembly = mono_domain_assembly_open(domain, get_assembly_path(assemblyName).string().c_str());
		if (assembly == nullptr)
			return nullptr;

		MonoImage* image = mono_assembly_get_image(assembly);
		if (image == nullptr)
			return nullptr;

		MonoClass* klass = mono_class_from_name(image, nameSpace, className);
		if (klass == nullptr)
			return nullptr;

		MonoClassField* field = mono_class_get_field_from_name(klass, fieldName);

		return field;
	}
	MonoClassField* mono::get_field_impl(MonoClass* pKlass, const char* fieldName) const
	{
		MonoClassField* field = mono_class_get_field_from_name(pKlass, fieldName);

		return field;
	}
	uint32_t mono::get_field_offset_impl(MonoClassField* field) const
	{
		if (!field)
			return 0u;

		return mono_field_get_offset(field);
	}
	void mono::get_field_value_impl(void* instance, MonoClassField* field, void* out) const
	{
		if (!instance || !field || !out)
			return;

		mono_field_get_value(instance, field, out);
	}
	void mono::set_field_value_impl(MonoObject* obj, MonoClassField* field, void* value)
	{
		if (!obj || !field || !value)
			return;

		mono_field_set_value(obj, field, value);
	}
	MonoVTable* mono::get_vtable_impl(MonoClass* pKlass) const
	{
		if (!pKlass)
			return nullptr;

		return mono_class_vtable(mono_get_root_domain(), pKlass);
	}
	void* mono::get_static_field_data_impl(MonoVTable* pVTable) const
	{
		if (!pVTable)
			return nullptr;

		return mono_vtable_get_static_field_data(pVTable);
	}
	void* mono::get_static_field_data_impl(MonoClass* pKlass) const
	{
		MonoVTable* vtable = get_vtable_impl(pKlass);
		if (vtable == nullptr)
			return nullptr;

		return mono_vtable_get_static_field_data(vtable);
	}
	void* mono::get_static_field_value_impl(const char* className, const char* fieldName, const char* assemblyName, const char* nameSpace) const
	{
		MonoClass* klass = get_class_impl(className, assemblyName, nameSpace);
		if (klass == nullptr)
			return nullptr;

		MonoClassField* field = get_field_impl(klass, fieldName);
		if (field == nullptr)
			return nullptr;

		auto addr = (uintptr_t)get_static_field_data_impl(klass);
		uint32_t offset = get_field_offset_impl(field);

		void* value = (void*)(addr + offset);

		return value;
	}
	void* mono::mono_object_unbox_impl(MonoObject* obj)
	{
		return mono_object_unbox(obj);
	}
	MonoThread* mono::mono_thread_attach_impl(MonoDomain* domain) const
	{
		return mono_thread_attach(domain);
	}
	MonoDomain* mono::get_root_domain_impl() const
	{
		return mono_get_root_domain();
	}
	std::string mono::from_mono_string_impl(MonoString* monoStr) const
	{
		if (!monoStr || monoStr->length < 0 || monoStr->length > 0x2000)
			return {};

		//auto raw = this->mono_string_to_utf8(monoStr);

		//if (!raw) return {};

		//std::string out(raw);

		//this->mono_free(raw);

		//return out;

		int len = WideCharToMultiByte(
		    CP_UTF8,
		    0,
		    (wchar_t*)monoStr->chars,
		    monoStr->length,
		    nullptr,
		    0,
		    nullptr,
		    nullptr);

		std::string out(len, '\0');

		WideCharToMultiByte(
		    CP_UTF8,
		    0,
		    (wchar_t*)monoStr->chars,
		    monoStr->length,
		    out.data(),
		    len,
		    nullptr,
		    nullptr);

		return out;
	}
	std::wstring_view mono::view_mono_string_impl(MonoString* monoStr) const
	{
		if (!monoStr || monoStr->length < 0 || monoStr->length > 0x2000)
			return {};

		const wchar_t* wchars = reinterpret_cast<const wchar_t*>(monoStr->chars);

		return std::wstring_view(wchars, monoStr->length);
	}
	MonoString* mono::to_mono_string_utf16(std::string const& str)
	{
		MonoDomain* domain = mono_get_root_domain();

		int size_needed = MultiByteToWideChar(
			CP_UTF8,
			0,
			str.c_str(),
			-1,
			nullptr,
			0
		);

		std::wstring wstr(size_needed, 0);

		MultiByteToWideChar(
			CP_UTF8,
			0,
			str.c_str(),
			-1,
			wstr.data(),
			size_needed
		);

		return mono_string_new_utf16(domain, (mono_unichar2*)wstr.c_str(), (int)wstr.length());
	}
	std::filesystem::path mono::get_assembly_path(const char* assemblyName) const
	{
		return std::filesystem::current_path() / std::filesystem::path(std::format("./Valheim_Data/Managed/{}.dll", assemblyName));
	}
}