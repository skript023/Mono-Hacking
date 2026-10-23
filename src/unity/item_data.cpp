#include "item_data.hpp"

#include <charconv>
#include <format>

namespace big
{
	item_data::item_data(MonoObject* o) :
	    obj(o)
	{
	}
	item_data::~item_data() noexcept
	{
		obj = nullptr;
	}
	MonoObject* item_data::get_object()
	{
		return obj;
	}
	shared_data item_data::get_shared_data()
	{
		return mono::get_field_value<"ItemDrop/ItemData", "m_shared", MonoObject*>(obj);
	}
	void item_data::set_durability(float durability)
	{
		if (!mono::set_field_value<"ItemDrop/ItemData", "m_durability">(obj, durability))
		{
			LOG(FATAL) << "Failed set durability";
		}
	}
	float item_data::get_durability()
	{
		return mono::get_field_value<"ItemDrop/ItemData", "m_durability", float>(obj);
	}
	float item_data::get_max_durability()
	{
		float override_value = 0.f;
		if (get_stat_override("ew_max_durability", override_value))
		{
			return override_value;
		}

		auto shared = get_shared_data();

		auto durability           = shared.get_max_durability();
		auto durability_per_level = shared.get_durability_per_level();
		auto quality              = mono::get_field_value<"ItemDrop/ItemData", "m_quality", int>(obj);

		return durability + (float)std::max(0, quality - 1) * durability_per_level;
	}
	void item_data::set_quality(int quality)
	{
		if (!mono::set_field_value<"ItemDrop/ItemData", "m_quality">(obj, quality))
		{
			LOG(FATAL) << "Failed set quality";
		}
	}
	int item_data::get_quality()
	{
		return mono::get_field_value<"ItemDrop/ItemData", "m_quality", int>(obj);
	}
	void item_data::set_stack(int stack)
	{
		if (!mono::set_field_value<"ItemDrop/ItemData", "m_stack">(obj, stack))
		{
			LOG(FATAL) << "Failed set stack";
		}
	}
	int item_data::get_stack()
	{
		return mono::get_field_value<"ItemDrop/ItemData", "m_stack", int>(obj);
	}
	void item_data::set_variant(int variant)
	{
		if (!mono::set_field_value<"ItemDrop/ItemData", "m_variant">(obj, variant))
		{
			LOG(FATAL) << "Failed set variant";
		}
	}
	int item_data::get_variant()
	{
		return mono::get_field_value<"ItemDrop/ItemData", "m_variant", int>(obj);
	}
	void item_data::set_grid_pos(iVector2 pos)
	{
		if (!mono::set_field_value<"ItemDrop/ItemData", "m_gridPos">(obj, pos))
		{
			LOG(FATAL) << "Failed set grid pos";
		}
	}
	iVector2 item_data::get_grid_pos()
	{
		return mono::get_field_value<"ItemDrop/ItemData", "m_gridPos", iVector2>(obj);
	}
	void item_data::set_cheated(bool cheated)
	{
		if (obj)
		{
			mono::set_field_value<"ItemDrop/ItemData", "m_cheated">(obj, cheated);
		}
	}
	bool item_data::get_cheated()
	{
		if (!obj)
		{
			return false;
		}
		return mono::get_field_value<"ItemDrop/ItemData", "m_cheated", bool>(obj);
	}
	void item_data::set_crafter_id(int64_t crafter_id)
	{
		if (obj)
		{
			mono::set_field_value<"ItemDrop/ItemData", "m_crafterID">(obj, crafter_id);
		}
	}
	int64_t item_data::get_crafter_id()
	{
		if (!obj)
		{
			return 0;
		}
		return mono::get_field_value<"ItemDrop/ItemData", "m_crafterID", int64_t>(obj);
	}
	void item_data::set_crafter_name(const std::string& name)
	{
		if (obj)
		{
			auto ms = mono::to_mono_string(name);
			mono::set_field_value<"ItemDrop/ItemData", "m_crafterName">(obj, ms);
		}
	}
	std::string item_data::get_crafter_name()
	{
		if (!obj)
		{
			return "";
		}
		auto ms = mono::get_field_value<"ItemDrop/ItemData", "m_crafterName", MonoString*>(obj);
		return ms ? mono::from_mono_string(ms) : "";
	}

	bool item_data::get_stat_override(const std::string& name, float& value)
	{
		if (!obj)
		{
			return false;
		}

		auto custom_data = mono::get_field_value<"ItemDrop/ItemData", "m_customData", MonoObject*>(obj);
		if (!custom_data)
		{
			return false;
		}

		struct dictionary_layout
		{
			MonoMethod* try_get_value{};
			uint32_t count{};
			uint32_t free_count{};
		};
		static const dictionary_layout layout = [custom_data] {
			dictionary_layout result{};
			if (auto klass = mono::object_get_class(custom_data))
			{
				result.try_get_value = mono::class_get_method_from_name(klass, "TryGetValue", 2);
				result.count         = mono::get_field_offset(mono::get_field(klass, "_count"));
				result.free_count    = mono::get_field_offset(mono::get_field(klass, "_freeCount"));
			}
			return result;
		}();
		if (!layout.try_get_value)
		{
			return false;
		}
		if (layout.count && layout.free_count)
		{
			const auto base = reinterpret_cast<uintptr_t>(custom_data);
			if (*reinterpret_cast<int*>(base + layout.count) - *reinterpret_cast<int*>(base + layout.free_count) <= 0)
			{
				return false;
			}
		}

		MonoString* raw_value = nullptr;
		void* args[2]         = {mono::to_mono_string(name), &raw_value};
		auto found            = mono::invoke_method(layout.try_get_value, custom_data, args);
		if (!found || !*reinterpret_cast<bool*>(mono::object_unbox(found)) || !raw_value)
		{
			return false;
		}

		auto text         = mono::from_mono_string(raw_value);
		const auto* begin = text.data();
		const auto* end   = begin + text.size();
		auto result       = std::from_chars(begin, end, value);
		return result.ec == std::errc{} && result.ptr == end;
	}

	void item_data::set_stat_override(const std::string& name, float value)
	{
		if (!obj)
		{
			return;
		}

		auto custom_data = mono::get_field_value<"ItemDrop/ItemData", "m_customData", MonoObject*>(obj);
		if (!custom_data)
		{
			return;
		}

		static MonoMethod* set_item = nullptr;
		if (!set_item)
		{
			auto klass = mono::object_get_class(custom_data);
			set_item   = klass ? mono::class_get_method_from_name(klass, "set_Item", 2) : nullptr;
		}
		if (!set_item)
		{
			return;
		}

		const auto value_text = std::format("{:.6g}", value);
		void* args[2]         = {mono::to_mono_string(name), mono::to_mono_string(value_text)};
		mono::invoke_method(set_item, custom_data, args);
	}

	void item_data::remove_stat_override(const std::string& name)
	{
		if (!obj)
		{
			return;
		}

		auto custom_data = mono::get_field_value<"ItemDrop/ItemData", "m_customData", MonoObject*>(obj);
		if (!custom_data)
		{
			return;
		}

		static MonoMethod* remove = nullptr;
		if (!remove)
		{
			auto klass = mono::object_get_class(custom_data);
			remove     = klass ? mono::class_get_method_from_name(klass, "Remove", 1) : nullptr;
		}
		if (!remove)
		{
			return;
		}

		void* args[1] = {mono::to_mono_string(name)};
		mono::invoke_method(remove, custom_data, args);
	}
}
