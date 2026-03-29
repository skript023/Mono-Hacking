#pragma once
#include "mono/mono.hpp"
#include <pointers.hpp>
#include "class/mono_list.hpp"

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

	inline std::vector<MonoObject*> list_to_vector(MonoObject* list)
	{
		std::vector<MonoObject*> out;
		if (!list)
			return out;

		MonoClass* klass = mono::object_get_class(list);

		if (!klass)
		{
			LOG(VERBOSE) << "Failed to get class from list object.";
			return out;
		}

		MonoMethod* getCount = mono::class_get_method_from_name(klass, "get_Count", 0);
		MonoMethod* getItem = mono::class_get_method_from_name(klass, "get_Item", 1);

		if (!getCount || !getItem)
			return out;

		MonoObject* ret = mono::invoke_method(getCount, list);
		if (!ret)
			return out;

		int count = 0;
		void* unboxed = mono::object_unbox(ret);
		if (unboxed)
			count = *(int*)unboxed;

		for (int i = 0; i < count; ++i)
		{
			void* args[1] = {&i};
			MonoObject* item = mono::invoke_method(getItem, list, args);
			if (item)
				out.push_back(item);
		}

		return out;
	}

	inline MonoObject* get_transform(void* player)
	{
		static MonoMethod* method = mono::get_method("Character", "GetTransform", 0, "assembly_valheim");

		if (!method || !player)
			return nullptr;

		auto transform = mono::invoke_method(method, player, nullptr);

		return transform;
	}

	inline Vector3 get_position(void* player)
	{
		static MonoMethod* method = mono::get_method("Character", "GetTransform", 0, "assembly_valheim");
		static MonoMethod* get_position = mono::get_method("Transform", "get_position", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!method || !player || !get_position)
			return Vector3();

		auto transform = mono::invoke_method(method, player, nullptr);

    	auto obj = mono::invoke_method(get_position, transform, nullptr);

		return *(Vector3*)mono::object_unbox(obj);
	}

	inline Vector4 get_rotation(void* player)
	{
		Vector4 rot{0.f, 0.f, 0.f, 1.f}; // default identity

		if (!player)
			return rot;

		static MonoMethod* get_transform =
			mono::get_method("Character", "GetTransform", 0, "assembly_valheim");

		static MonoMethod* get_rotation_method =
			mono::get_method("Transform", "get_rotation", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!get_transform || !get_rotation_method)
			return rot;

		auto transform = mono::invoke_method(get_transform, player, nullptr);
		if (!transform)
			return rot;

		auto obj = mono::invoke_method(get_rotation_method, transform, nullptr);
		if (!obj)
			return rot;

		return *(Vector4*)mono::object_unbox(obj); // Quaternion (x,y,z,w)
	}
	inline Vector3 get_euler_angles(void* player)
	{
		Vector3 euler{};

		if (!player)
			return euler;

		static auto get_transform =
			mono::get_method("Character", "GetTransform", 0, "assembly_valheim");

		static auto get_euler =
			mono::get_method("Transform", "get_eulerAngles", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!get_transform || !get_euler)
			return euler;

		auto transform = mono::invoke_method(get_transform, player, nullptr);
		if (!transform)
			return euler;

		auto obj = mono::invoke_method(get_euler, transform, nullptr);
		if (!obj)
			return euler;

		return *(Vector3*)mono::object_unbox(obj);
	}
	inline Vector3 get_zdo_vec3(void* player, int hash_name)
	{
		Vector3 result{};

		if (!player || !hash_name)
			return result;

		// 🔥 cache semua
		static MonoClass* character_class = mono::get_class("Character", "assembly_valheim");
		static MonoClass* znetview_class  = mono::get_class("ZNetView", "assembly_valheim");
		static MonoClass* zdo_class       = mono::get_class("ZDO", "assembly_valheim");

		static MonoClassField* m_nview_field =
			mono::get_field(character_class, "m_nview");

		static MonoMethod* get_zdo_method =
			mono::get_method("ZNetView", "GetZDO", 0, "assembly_valheim");

		static MonoMethod* get_vec3_method =
			mono::get_method("ZDO", "GetVec3", 2, "assembly_valheim");

		if (!m_nview_field || !get_zdo_method || !get_vec3_method)
			return result;

		// 1. ambil m_nview
		void* nview = nullptr;
		mono::get_field_value(player, m_nview_field, &nview);

		if (!nview)
			return result;

		// 2. ambil ZDO
		void* zdo = mono::invoke_method(get_zdo_method, nview, nullptr);
		if (!zdo)
			return result;

		// 3. prepare arg
		int key_str = hash_name;
		Vector3 default_val{};

		void* args[2];
		args[0] = &key_str;
		args[1] = &default_val;

		// 4. invoke
		auto obj = mono::invoke_method(get_vec3_method, zdo, args);
		if (!obj)
			return result;

		// 5. unbox
		return *(Vector3*)mono::object_unbox(obj);
	}
	inline Vector3 get_center_point(void* player)
	{
		Vector3 pos{};

		if (!player)
			return pos;

		static auto method = mono::get_method("Character", "GetCenterPoint", 0, "assembly_valheim");

		if (!method)
			return pos;

		auto obj = mono::invoke_method(method, player, nullptr);
		if (!obj)
			return pos;

		return *(Vector3*)mono::object_unbox(obj);
	}
	inline Vector3 get_teleport_from()
	{
		Vector3 pos{};

		MonoObject* player = unity::get_local_player();
		if (!player)
			return pos;
			
		pos = get_center_point(player);

		LOG(VERBOSE) << std::format("Player m_teleportFromPos = {:.3f}, {:.3f}, {:.3f}",
		    pos.x,
		    pos.y,
		    pos.z);

		return pos;
	}

	inline void teleport_to(Vector3 const& position, Vector4 const& rotation, bool distantTeleport)
	{
		static MonoObject* player = unity::get_local_player();

		if (!player)
			return;

		static MonoMethod* method = mono::get_method("Player", "TeleportTo", 3, "assembly_valheim");

		bool flashBar = true;

		auto pos = position;
		auto rot = rotation;
		auto distant = distantTeleport;

		void* args[3] = {&pos, &rot, &distant};

		MonoObject* ret = mono::invoke_method(method, player, args);
	}

	inline int get_screen_width()
	{
		static MonoMethod* method = mono::get_method("Screen", "get_width", 0, "UnityEngine.CoreModule");
		auto result = mono::invoke_method(method, nullptr, nullptr);
		return *(int*)mono::object_unbox(result);
	}

	inline int get_screen_height()
	{
		static MonoMethod* method = mono::get_method("Screen", "get_height", 0, "UnityEngine.CoreModule");
		auto result = mono::invoke_method(method, nullptr, nullptr);
		return *(int*)mono::object_unbox(result);
	}

	inline bool world_to_screen(Vector3 const& world, Vector3& out)
	{
		static MonoMethod* get_main = mono::get_method("Camera", "get_main", 0, "UnityEngine.CoreModule", "UnityEngine");
		static MonoMethod* w2s_method = mono::get_method("Camera", "WorldToScreenPoint", 2, "UnityEngine.CoreModule", "UnityEngine");

		if (!get_main || !w2s_method)
			return false;

		auto camera = mono::invoke_method(get_main, nullptr, nullptr);
		if (!camera)
			return false;

		int eye = 2;

		void* args[2];
		args[0] = (void*)&world;
		args[1] = (void*)&eye;

		auto result_obj = mono::invoke_method(w2s_method, camera, args);
		if (!result_obj)
			return false;

		Vector3 result = *(Vector3*)mono::object_unbox(result_obj);

		if (result.z <= 0.1f)
			return false;

		float screen_width = unity::get_screen_width();
		float screen_height = unity::get_screen_height();

		out.x = result.x;
		out.y = screen_height - result.y;
		out.z = result.z;

		return true;
	}

	inline std::vector<MonoObject*> get_all_characters()
	{
		static MonoMethod* method = mono::get_method("Character", "GetAllCharacters", 0, "assembly_valheim");

		if (!method)
			return {};

		MonoObject* result = mono::invoke_method(method, nullptr);

		// MonoClass* klass = mono::object_get_class(result);
		// const char* class_name = mono::class_get_name(klass);
		// const char* namespace_name = mono::class_get_namespace(klass);

		// LOG(INFO) << "Result class: " << namespace_name << "::" << class_name;

		return list_to_vector(result);
	}

	inline std::vector<MonoObject*> get_all_players()
	{
		static MonoMethod* method = mono::get_method("Player", "GetAllPlayers", 0, "assembly_valheim");

		if (!method)
			return {};

		MonoObject* result = mono::invoke_method(method, nullptr);

		// MonoClass* klass = mono::object_get_class(result);
		// const char* class_name = mono::class_get_name(klass);
		// const char* namespace_name = mono::class_get_namespace(klass);

		// LOG(INFO) << "Result class: " << namespace_name << "::" << class_name;

		return list_to_vector(result);
	}

	inline bool is_int(const std::string& s, int64_t& out)
	{
		char* end{};
		out = strtoll(s.c_str(), &end, 10);
		return end && *end == '\0';
	}

	inline bool is_double(const std::string& s, double& out)
	{
		char* end{};
		out = strtod(s.c_str(), &end);
		return end && *end == '\0';
	}

	inline bool is_bool(const std::string& s, bool& out)
	{
		if (s == "true" || s == "True")
		{
			out = true;
			return true;
		}
		if (s == "false" || s == "False")
		{
			out = false;
			return true;
		}
		return false;
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