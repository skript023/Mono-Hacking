#include "common.hpp"
#include "logger.hpp"
#include "pointers.hpp"

namespace big
{
	pointers::pointers() : main_batch("pointer_cache"), discord_batch("discord")
	{
		main_batch.add("Return Address", "FF 23", [this](memory::handle ptr)
		{
			m_return_address = ptr.as<void*>();
		});

		discord_batch.add("Discord Overlay Present", "48 8B 05 ? ? ? ? 48 89 D9 89 FA 41 89 F0 FF 15 ? ? ? ? 41 89 C7", [this](memory::handle ptr)
		{
			m_present = ptr.add(3).rip().as<decltype(m_present)>();
		});

		discord_batch.add("Discord Overlay Resize Buffer", "48 8B 05 ? ? ? ? 4C 8B 15 ? ? ? ? 89 4C 24 28 89 54 24 20 4C 89 F1 89 DA 41 89 F8 41 89 F1", [this](memory::handle ptr)
		{
			m_resizebuffer = ptr.add(3).rip().as<decltype(m_resizebuffer)>();
		});

		main_batch.run(memory::module(nullptr));
		//discord_batch.run(memory::module("DiscordHook64.dll"));

		this->m_hwnd = FindWindow(WINDOW_CLASS, WINDOW_NAME);
		if (!this->m_hwnd)
			throw std::runtime_error("Failed to find the game's window.");

		g_pointers = this;
	}

	pointers::~pointers()
	{
		g_pointers = nullptr;
	}

	void pointers::update()
	{
		main_batch.update();
	}
}