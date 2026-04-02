#include "item_drop.hpp"

namespace big
{
	item_drop::item_drop(MonoObject* o): obj(o), m_data(o), m_localization(o)
	{}
	item_drop::~item_drop() noexcept
	{
		obj = nullptr;
	}
	MonoObject* item_drop::get_object() const
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

		mono::invoke(method, obj, stack);
	}
	void item_drop::set_quality(int quality)
	{
		static auto method = mono::get_method("ItemDrop", "SetQuality", 1, "assembly_valheim");

		if (!method)
		{
			LOG(FATAL) << "Failed to find method ItemDrop::SetQuality";
			return;
		}

		mono::invoke(method, obj, quality);
	}
	std::string item_drop::get_hover_name()
	{
		static auto method = mono::get_method(
			"ItemDrop",
			"GetHoverName",
			0,
			"assembly_valheim"
		);

		if (!method || !obj)
			return "unknown";

		auto name_obj = mono::invoke(method, obj);

		if (!name_obj)
			return "unknown";

		std::string result = mono::from_mono_string(reinterpret_cast<MonoString*>(name_obj));

		return m_localization.localize(result);
	}
	std::string item_drop::get_hover_text()
	{
		static auto method = mono::get_method(
			"ItemDrop",
			"GetHoverText",
			0,
			"assembly_valheim"
		);

		if (!method || !obj)
			return "unknown";

		auto name_obj = mono::invoke(method, obj);

		if (!name_obj)
			return "unknown";

		std::string result = mono::from_mono_string(reinterpret_cast<MonoString*>(name_obj));

		return result;
	}
	std::optional<Vector3> item_drop::get_position()
	{
		static auto method = mono::get_method("Component", "get_transform", 0, "UnityEngine.CoreModule", "UnityEngine");
		static auto get_position = mono::get_method("Transform", "get_position", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!method || !obj || !get_position)
			return std::nullopt;

		auto transform = mono::invoke(method, obj);

		if (!transform) return std::nullopt;

    	auto ret = mono::invoke(get_position, transform);

		if (!ret) return std::nullopt;

		return *reinterpret_cast<Vector3*>(mono::object_unbox(ret));
	}
	std::optional<Vector3> item_drop::get_bounds_top()
	{
		static auto get_renderer = mono::get_method(
			"Component", "GetComponent", 1,
			"UnityEngine.CoreModule", "UnityEngine"
		);

		static auto get_bounds = mono::get_method(
			"Renderer", "get_bounds", 0,
			"UnityEngine.CoreModule", "UnityEngine"
		);

		if (!obj || !get_renderer || !get_bounds)
			return std::nullopt;

		auto renderer_class = mono::get_class("Renderer", "UnityEngine");

		void* args[1] = { renderer_class };

		MonoObject* renderer = mono::invoke_method(get_renderer, obj, args);
		if (!renderer)
			return std::nullopt;

		MonoObject* bounds_obj = mono::invoke_method(get_bounds, renderer, nullptr);
		if (!bounds_obj)
			return std::nullopt;

		auto bounds = *reinterpret_cast<Bounds*>(mono::object_unbox(bounds_obj));

		auto top = bounds.center;
		top.y += bounds.extents.y;

		return top;
	}
}