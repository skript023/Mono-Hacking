#pragma once
#include "settings/state_serializer.hpp"

namespace big
{
	// Per-feature toggle keys from settings.json: "hotkeys": { "<command name>": <virtual-key code> }.
	// Astra writes them; the mod only reads them, at start and whenever the file changes.
	class hotkeys : private state_serializer
	{
	private:
		std::unordered_map<std::string, std::uint32_t> m_bindings;
		std::unordered_map<std::string, bool> m_was_down;

		hotkeys();

	public:
		static hotkeys& instance()
		{
			static hotkeys instance{};
			return instance;
		}

		static void tick()
		{
			instance().tick_impl();
		}

	private:
		void tick_impl();
		virtual void save_state_impl(nlohmann::json& state) override;
		virtual void load_state_impl(nlohmann::json& state) override;
	};
}
