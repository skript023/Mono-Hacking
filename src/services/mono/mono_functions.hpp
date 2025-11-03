#pragma once

#include <mono/metadata/threads.h>
#include <mono/metadata/object.h>

namespace big
{
	// --- Fungsi Runtime dan Domain ---

	using mono_thread_attach_t = MonoThread * (*)(MonoDomain* domain);
	using mono_get_root_domain_t = MonoDomain * (*)();
	using mono_domain_assembly_open_t = MonoAssembly * (*)(MonoDomain* doamin, const char* name);
	using mono_assembly_get_image_t = MonoImage * (*)(MonoAssembly* assembly);
	using mono_class_from_name_t = MonoClass * (*)(MonoImage* image, const char* name_space, const char* name);
	using mono_class_get_method_from_name_t = MonoMethod * (*)(MonoClass* klass, const char* name, int param_count);
	using mono_compile_method_t = void* (*)(MonoMethod* method);
	using mono_runtime_invoke_t = MonoObject * (*)(MonoMethod* method, void* obj, void** params, MonoObject** exc);

	// --- Fungsi Class dan Field ---

	using mono_class_get_field_from_name_t = MonoClassField * (*)(MonoClass* klass, const char* name);
	using mono_field_get_value_t = void* (*)(void* obj, MonoClassField* field, void* value);
	using mono_field_set_value_t = void (*)(MonoObject* obj, MonoClassField* field, void* value);
	using mono_method_get_class_t = MonoClass * (*)(MonoMethod* method);
	using mono_class_vtable_t = MonoVTable * (*)(MonoDomain* domain, MonoClass* klass);
	using mono_vtable_get_static_field_data_t = void* (*)(MonoVTable* vt);
	using mono_field_get_offset_t = uint32_t(*)(MonoClassField* field);
}