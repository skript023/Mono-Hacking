#include "mono.hpp"

namespace big
{
	void mono::init_impl()
	{
		// Implementation details for initializing Mono
		m_mono = GetModuleHandleA("mono-2.0-bdwgc.dll");
		if (m_mono == NULL)
		{
			LOG(WARNING) << "Failed to find Mono module.";
			return;
		}

		LOG(INFO) << "Mono module found at address: " << m_mono;

		// Necessary functions to get method addresses
		mono_domain_assembly_open = reinterpret_cast<mono_domain_assembly_open_t>(GetProcAddress(m_mono, "mono_domain_assembly_open"));
		mono_assembly_get_image = reinterpret_cast<mono_assembly_get_image_t>(GetProcAddress(m_mono, "mono_assembly_get_image"));
		mono_class_from_name = reinterpret_cast<mono_class_from_name_t>(GetProcAddress(m_mono, "mono_class_from_name"));
		mono_class_get_method_from_name = reinterpret_cast<mono_class_get_method_from_name_t>(GetProcAddress(m_mono, "mono_class_get_method_from_name"));
		mono_compile_method = reinterpret_cast<mono_compile_method_t>(GetProcAddress(m_mono, "mono_compile_method"));
		mono_runtime_invoke = reinterpret_cast<mono_runtime_invoke_t>(GetProcAddress(m_mono, "mono_runtime_invoke"));

		mono_class_get_field_from_name = reinterpret_cast<mono_class_get_field_from_name_t>(GetProcAddress(m_mono, "mono_class_get_field_from_name"));
		mono_field_get_value = reinterpret_cast<mono_field_get_value_t>(GetProcAddress(m_mono, "mono_field_get_value"));
		mono_field_set_value = reinterpret_cast<mono_field_set_value_t>(GetProcAddress(m_mono, "mono_field_set_value"));
		mono_method_get_class = reinterpret_cast<mono_method_get_class_t>(GetProcAddress(m_mono, "mono_method_get_class"));
		mono_class_vtable = reinterpret_cast<mono_class_vtable_t>(GetProcAddress(m_mono, "mono_class_vtable"));
		mono_vtable_get_static_field_data = reinterpret_cast<mono_vtable_get_static_field_data_t>(GetProcAddress(m_mono, "mono_vtable_get_static_field_data"));
		mono_field_get_offset = reinterpret_cast<mono_field_get_offset_t>(GetProcAddress(m_mono, "mono_field_get_offset"));

		// Attach thread to prevent crashes
		mono_thread_attach = reinterpret_cast<mono_thread_attach_t>(GetProcAddress(m_mono, "mono_thread_attach"));
		mono_get_root_domain = reinterpret_cast<mono_get_root_domain_t>(GetProcAddress(m_mono, "mono_get_root_domain"));

		// Melampirkan thread ini ke domain Mono root agar aman
		mono_thread_attach(mono_get_root_domain());

		this->initalized = true;
	}
	MonoObject* mono::invoke_method_impl(MonoMethod* method, void* obj, void** params) const
	{
		mono_thread_attach(mono_get_root_domain());

		MonoObject* execution;

		return mono_runtime_invoke(method, obj, params, &execution);
	}
	void* mono::get_compile_method_impl(const char* className, const char* methodName, int param_count, const char* assemblyName) const
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

		MonoClass* klass = mono_class_from_name(image, "", className);
		if (klass == nullptr)
		{
			LOG(WARNING) << "Failed to get class: " << klass;

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
			LOG(WARNING) << "Failed to get class: " << klass;

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
		return mono_field_get_offset(field);
	}
	void mono::get_field_value_impl(void* instance, MonoClassField* field, void* out) const
	{
		mono_field_get_value(instance, field, out);
	}
	void mono::set_field_value_impl(MonoObject* obj, MonoClassField* field, void* value)
	{
		mono_field_set_value(obj, field, value);
	}
	MonoVTable* mono::get_vtable_impl(MonoClass* pKlass) const
	{
		return mono_class_vtable(mono_get_root_domain(), pKlass);
	}
	void* mono::get_static_field_data_impl(MonoVTable* pVTable) const
	{
		return mono_vtable_get_static_field_data(pVTable);
	}
	void* mono::get_static_field_data_impl(MonoClass* pKlass) const
	{
		MonoVTable* vtable = get_vtable_impl(pKlass);
		if (vtable == nullptr)
			return nullptr;

		return mono_vtable_get_static_field_data(vtable);
	}
	void* mono::get_static_field_value_impl(const char* className, const char* fieldName)
	{
		MonoClass* klass = get_class_impl(className);
		if (klass == nullptr)
			return nullptr;

		MonoClassField* field = get_field_impl(klass, fieldName);
		if (field == nullptr)
			return nullptr;

		DWORD addr = (DWORD)get_static_field_data_impl(klass);
		uint32_t offset = get_field_offset_impl(field);

		void* value = (void*)(addr + offset);

		return value;
	}
	std::filesystem::path mono::get_assembly_path(const char* assemblyName) const
	{
		return std::filesystem::current_path() / std::filesystem::path(std::format("./Valheim_Data/Managed/{}.dll", assemblyName));
	}
}