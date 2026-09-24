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

	minimap minimap::get_instance()
	{
		return minimap(unity::get_minimap());
	}

	void minimap::explore_all()
	{
		if (!m_minimap)
			return;

		static auto method = mono::get_method("Minimap", "ExploreAll", 0, "assembly_valheim");
		if (!method)
			return;

		mono::invoke_method(method, m_minimap, nullptr);
	}

	void minimap::reset()
	{
		if (!m_minimap)
			return;

		static auto method = mono::get_method("Minimap", "Reset", 0, "assembly_valheim");
		if (!method)
			return;

		mono::invoke_method(method, m_minimap, nullptr);
	}

	bool minimap::discover_location(const Vector3& pos, int pin_type, std::string_view pin_name, bool show_map)
	{
		if (!m_minimap)
			return false;

		static auto method = mono::get_method("Minimap", "DiscoverLocation", 4, "assembly_valheim");
		if (!method)
			return false;

		MonoString* ms = mono::to_mono_string(std::string(pin_name));
		bool show = show_map;
		void* args[4] = {(void*)&pos, &pin_type, ms, &show};
		mono::invoke_method(method, m_minimap, args);
		return true;
	}

	bool minimap::is_open()
	{
		if (!m_minimap)
			return false;

		static auto method = mono::get_method("Minimap", "IsOpen", 0, "assembly_valheim");
		if (!method)
			return false;

		auto res = mono::invoke_method(method, m_minimap, nullptr);
		return res ? *static_cast<bool*>(mono::object_unbox(res)) : false;
	}
}
