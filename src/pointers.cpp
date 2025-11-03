#include "common.hpp"
#include "logger.hpp"
#include "pointers.hpp"

namespace big
{
	pointers::pointers() : main_batch("pointer_cache")
	{
		main_batch.add("Return Address", "FF 23", [this](memory::handle ptr)
		{
			m_return_address = ptr.as<void*>();
		});

		main_batch.run(memory::module(nullptr));

		this->m_hwnd = FindWindow("UnityWndClass", "Valheim");
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