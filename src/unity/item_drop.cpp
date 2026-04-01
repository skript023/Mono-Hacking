#include "item_drop.hpp"

namespace big
{
	item_drop::item_drop(MonoObject* o): obj(o)
	{
	}
	item_drop::~item_drop() noexcept
	{
		obj = nullptr;
	}
	MonoObject* item_drop::get_object()
	{
		return obj;
	}
	std::vector<item_drop> item_drop::get_drops()
	{
		MonoClass* drop = mono::get_class("ItemDrop", "assembly_valheim");
		if (drop == nullptr) return {};

		MonoClassField* drop_instance = mono::get_field(drop, "s_instances");
		if (drop_instance == nullptr) return {};

		void* static_field_data_addr = mono::get_static_field_data(drop);
		if (static_field_data_addr == nullptr) return {};

		uint32_t offset = mono::get_field_offset(drop_instance);
		void* drop_ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		MonoObject* drop_ptr_instance = *(MonoObject**)drop_ptr_addr;

		return mono::from_list<item_drop>(drop_ptr_instance);
	}
	void item_drop::set_stack(int stack)
	{
		static auto method = mono::get_method("ItemDrop", "SetStack", 1, "assembly_valheim");

		if (!method)
		{
			LOG(FATAL) << "Failed to find method ItemDrop::SetStack";
			return;
		}

		void* args[1];
		args[0] = &stack;

		mono::invoke_method(method, obj, args);
	}
	void item_drop::set_quality(int quality)
	{
		static auto method = mono::get_method("ItemDrop", "SetQuality", 1, "assembly_valheim");

		if (!method)
		{
			LOG(FATAL) << "Failed to find method ItemDrop::SetQuality";
			return;
		}

		void* args[1];
		args[0] = &quality;

		mono::invoke_method(method, obj, args);
	}
}