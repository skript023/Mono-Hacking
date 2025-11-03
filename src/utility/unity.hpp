#pragma once
#include "mono/mono.hpp"
#include <pointers.hpp>

namespace big::unity
{
	inline MonoObject* get_local_player()
	{
		// 1. Cari Class Player
		MonoClass* player_class = mono::get_class("Player", "assembly_valheim");
		if (player_class == nullptr) return nullptr;

		// 2. Cari Static Field m_localPlayer
		MonoClassField* local_player_field = mono::get_field(player_class, "m_localPlayer");
		if (local_player_field == nullptr) return nullptr;

		// 3. Dapatkan Base Address Static Field Data
		void* static_field_data_addr = mono::get_static_field_data(player_class);
		if (static_field_data_addr == nullptr) return nullptr;

		// 4. Hitung Offset dan Baca Nilai (MonoObject*)
		uint32_t offset = mono::get_field_offset(local_player_field);
		void* local_player_ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		// Casting address ke pointer-to-pointer, lalu dereference untuk mendapatkan MonoObject*
		MonoObject* local_player_instance = *(MonoObject**)local_player_ptr_addr;

		return local_player_instance;
	}

	inline MonoObject* get_zone_system()
	{
		// 1. Cari Class Player
		MonoClass* zone_system = mono::get_class("ZoneSystem", "assembly_valheim");
		if (zone_system == nullptr) return nullptr;

		// 2. Cari Static Field m_localPlayer
		MonoClassField* zone_system_instance = mono::get_field(zone_system, "m_instance");
		if (zone_system_instance == nullptr) return nullptr;

		// 3. Dapatkan Base Address Static Field Data
		void* static_field_data_addr = mono::get_static_field_data(zone_system);
		if (static_field_data_addr == nullptr) return nullptr;

		// 4. Hitung Offset dan Baca Nilai (MonoObject*)
		uint32_t offset = mono::get_field_offset(zone_system_instance);
		void* zone_system_ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		// Casting address ke pointer-to-pointer, lalu dereference untuk mendapatkan MonoObject*
		MonoObject* zone_system_ptr_instance = *(MonoObject**)zone_system_ptr_addr;

		return zone_system_ptr_instance;
	}

	inline MonoObject* get_env_man()
	{
		// 1. Cari Class Player
		MonoClass* env_man = mono::get_class("EnvMan", "assembly_valheim");
		if (env_man == nullptr) return nullptr;

		// 2. Cari Static Field m_localPlayer
		MonoClassField* env_man_instance = mono::get_field(env_man, "s_instance");
		if (env_man_instance == nullptr) return nullptr;

		// 3. Dapatkan Base Address Static Field Data
		void* static_field_data_addr = mono::get_static_field_data(env_man);
		if (static_field_data_addr == nullptr) return nullptr;

		// 4. Hitung Offset dan Baca Nilai (MonoObject*)
		uint32_t offset = mono::get_field_offset(env_man_instance);
		void* env_man_ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		// Casting address ke pointer-to-pointer, lalu dereference untuk mendapatkan MonoObject*
		MonoObject* env_man_ptr_instance = *(MonoObject**)env_man_ptr_addr;

		return env_man_ptr_instance;
	}

	inline bool is_key_pressed(std::uint16_t key)
	{
		if (GetForegroundWindow() == g_pointers->m_hwnd)
		{
			if (GetAsyncKeyState(key) & 0x8000)
			{
				return true;
			}
		}

		return false;
	}

	inline bool is_controller_pressed(std::uint16_t button)
	{
		XINPUT_STATE state;
		// Zero out the state structure
		ZeroMemory(&state, sizeof(XINPUT_STATE));

		// Get the state of the controller (controller 0)
		if (XInputGetState(0, &state) == ERROR_SUCCESS)
		{
			// Check if the specific button is pressed
			return (state.Gamepad.wButtons & button) != 0;
		}

		// Controller is not connected
		return false;
	}
}