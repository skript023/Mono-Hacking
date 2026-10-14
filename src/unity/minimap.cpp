#include "minimap.hpp"
#include "utility/unity.hpp"

namespace big
{
	minimap::minimap(MonoObject* obj) :
	    m_minimap(obj)
	{
	}

	minimap::~minimap() noexcept
	{
		m_minimap = nullptr;
	}

	minimap minimap::get_instance_impl()
	{
		return minimap(unity::get_minimap());
	}

	void minimap::explore_all()
	{
		if (!m_minimap)
		{
			return;
		}

		static auto method = mono::get_method("Minimap", "ExploreAll", 0, "assembly_valheim");
		if (!method)
		{
			return;
		}

		mono::invoke_method(method, m_minimap, nullptr);
	}

	void minimap::reset()
	{
		if (!m_minimap)
		{
			return;
		}

		static auto method = mono::get_method("Minimap", "Reset", 0, "assembly_valheim");
		if (!method)
		{
			return;
		}

		mono::invoke_method(method, m_minimap, nullptr);
	}

	bool minimap::discover_location(const Vector3& pos, int pin_type, std::string_view pin_name, bool show_map)
	{
		if (!m_minimap)
		{
			return false;
		}

		static auto method = mono::get_method("Minimap", "DiscoverLocation", 4, "assembly_valheim");
		if (!method)
		{
			return false;
		}

		MonoString* ms = mono::to_mono_string(std::string(pin_name));
		bool show      = show_map;
		void* args[4]  = {(void*)&pos, &pin_type, ms, &show};
		mono::invoke_method(method, m_minimap, args);
		return true;
	}

	bool minimap::add_pin(const Vector3& pos, int pin_type, std::string_view pin_name)
	{
		if (!m_minimap || pin_name.empty())
		{
			return false;
		}

		MonoString* name                  = mono::to_mono_string(std::string(pin_name));
		bool save                         = true;
		bool checked                      = false;
		int64_t owner_id                  = 0;
		void* args[6]                     = {const_cast<Vector3*>(&pos), &pin_type, name, &save, &checked, &owner_id};
		static auto add_pin_with_platform = mono::get_method("Minimap", "AddPin", 7, "assembly_valheim");
		if (add_pin_with_platform)
		{
			uint8_t default_platform_user[64]{};
			void* platform_args[7] = {args[0], args[1], args[2], args[3], args[4], args[5], default_platform_user};
			return mono::invoke_method(add_pin_with_platform, m_minimap, platform_args) != nullptr;
		}

		static auto add_pin_legacy = mono::get_method("Minimap", "AddPin", 6, "assembly_valheim");
		return add_pin_legacy && mono::invoke_method(add_pin_legacy, m_minimap, args) != nullptr;
	}

	bool minimap::is_open()
	{
		if (!m_minimap)
		{
			return false;
		}

		static auto method = mono::get_method("Minimap", "IsOpen", 0, "assembly_valheim");
		if (!method)
		{
			return false;
		}

		auto res = mono::invoke_method(method, m_minimap, nullptr);
		return res ? *static_cast<bool*>(mono::object_unbox(res)) : false;
	}
}
