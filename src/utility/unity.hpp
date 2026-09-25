#pragma once
#include <optional>
#include "mono/mono.hpp"
#include <pointers.hpp>
#include "class/mono_list.hpp"

namespace big::unity
{
	inline MonoObject* get_local_player()
	{
		// 1. Cari Class Player
		MonoClass* player_class = mono::get_class("Player", "assembly_valheim");
		if (player_class == nullptr)
			return nullptr;

		// 2. Cari Static Field m_localPlayer
		MonoClassField* local_player_field = mono::get_field(player_class, "m_localPlayer");
		if (local_player_field == nullptr)
			return nullptr;

		// 3. Dapatkan Base Address Static Field Data
		void* static_field_data_addr = mono::get_static_field_data(player_class);
		if (static_field_data_addr == nullptr)
			return nullptr;

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
		if (zone_system == nullptr)
			return nullptr;

		// 2. Cari Static Field m_localPlayer
		MonoClassField* zone_system_instance = mono::get_field(zone_system, "m_instance");
		if (zone_system_instance == nullptr)
			return nullptr;

		// 3. Dapatkan Base Address Static Field Data
		void* static_field_data_addr = mono::get_static_field_data(zone_system);
		if (static_field_data_addr == nullptr)
			return nullptr;

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
		if (env_man == nullptr)
			return nullptr;

		// 2. Cari Static Field m_localPlayer
		MonoClassField* env_man_instance = mono::get_field(env_man, "s_instance");
		if (env_man_instance == nullptr)
			return nullptr;

		// 3. Dapatkan Base Address Static Field Data
		void* static_field_data_addr = mono::get_static_field_data(env_man);
		if (static_field_data_addr == nullptr)
			return nullptr;

		// 4. Hitung Offset dan Baca Nilai (MonoObject*)
		uint32_t offset = mono::get_field_offset(env_man_instance);
		void* env_man_ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		// Casting address ke pointer-to-pointer, lalu dereference untuk mendapatkan MonoObject*
		MonoObject* env_man_ptr_instance = *(MonoObject**)env_man_ptr_addr;

		return env_man_ptr_instance;
	}

	inline MonoObject* get_item_drops()
	{
		// 1. Cari Class Player
		MonoClass* item_drop = mono::get_class("ItemDrop", "assembly_valheim");
		if (item_drop == nullptr)
			return nullptr;

		// 2. Cari Static Field m_localPlayer
		MonoClassField* item_drop_instance = mono::get_field(item_drop, "s_instances");
		if (item_drop_instance == nullptr)
			return nullptr;

		// 3. Dapatkan Base Address Static Field Data
		void* static_field_data_addr = mono::get_static_field_data(item_drop);
		if (static_field_data_addr == nullptr)
			return nullptr;

		// 4. Hitung Offset dan Baca Nilai (MonoObject*)
		uint32_t offset = mono::get_field_offset(item_drop_instance);
		void* item_drop_ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		// Casting address ke pointer-to-pointer, lalu dereference untuk mendapatkan MonoObject*
		MonoObject* item_drop_ptr_instance = *(MonoObject**)item_drop_ptr_addr;

		return item_drop_ptr_instance;
	}

	inline MonoObject* get_localization()
	{
		// 1. Cari Class Player
		MonoClass* klass = mono::get_class("Localization", "assembly_guiutils");
		if (klass == nullptr)
			return nullptr;

		// 2. Cari Static Field m_localPlayer
		MonoClassField* field = mono::get_field(klass, "m_instance");
		if (field == nullptr)
			return nullptr;

		// 3. Dapatkan Base Address Static Field Data
		void* static_field_data_addr = mono::get_static_field_data(klass);
		if (static_field_data_addr == nullptr)
			return nullptr;

		// 4. Hitung Offset dan Baca Nilai (MonoObject*)
		uint32_t offset = mono::get_field_offset(field);
		void* ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		// Casting address ke pointer-to-pointer, lalu dereference untuk mendapatkan MonoObject*
		MonoObject* ptr_instance = *(MonoObject**)ptr_addr;

		return ptr_instance;
	}

	inline MonoObject* get_object_db()
	{
		MonoClass* klass = mono::get_class("ObjectDB", "assembly_valheim");
		if (klass == nullptr)
			return nullptr;

		MonoClassField* field = mono::get_field(klass, "m_instance");
		if (field == nullptr)
			return nullptr;

		void* static_field_data_addr = mono::get_static_field_data(klass);
		if (static_field_data_addr == nullptr)
			return nullptr;

		uint32_t offset = mono::get_field_offset(field);
		void* ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		return *(MonoObject**)ptr_addr;
	}

	inline MonoObject* get_znet_scene()
	{
		MonoClass* klass = mono::get_class("ZNetScene", "assembly_valheim");
		if (klass == nullptr)
			return nullptr;

		MonoClassField* field = mono::get_field(klass, "m_instance");
		if (field == nullptr)
			return nullptr;

		void* static_field_data_addr = mono::get_static_field_data(klass);
		if (static_field_data_addr == nullptr)
			return nullptr;

		uint32_t offset = mono::get_field_offset(field);
		void* ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		return *(MonoObject**)ptr_addr;
	}

	inline MonoObject* get_rand_event_system()
	{
		MonoClass* klass = mono::get_class("RandEventSystem", "assembly_valheim");
		if (klass == nullptr)
			return nullptr;

		MonoClassField* field = mono::get_field(klass, "m_instance");
		if (field == nullptr)
			return nullptr;

		void* static_field_data_addr = mono::get_static_field_data(klass);
		if (static_field_data_addr == nullptr)
			return nullptr;

		uint32_t offset = mono::get_field_offset(field);
		void* ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		return *(MonoObject**)ptr_addr;
	}

	inline MonoObject* get_store_gui()
	{
		MonoClass* klass = mono::get_class("StoreGui", "assembly_valheim");
		if (klass == nullptr)
			return nullptr;

		MonoClassField* field = mono::get_field(klass, "m_instance");
		if (field == nullptr)
			return nullptr;

		void* static_field_data_addr = mono::get_static_field_data(klass);
		if (static_field_data_addr == nullptr)
			return nullptr;

		uint32_t offset = mono::get_field_offset(field);
		void* ptr_addr = (void*)((uintptr_t)static_field_data_addr + offset);

		return *(MonoObject**)ptr_addr;
	}

	template<typename T>
	inline T get_field_value(MonoObject* obj, const char* classname, const char* fieldName)
	{
		auto klass = mono::get_class(classname, "assembly_valheim");
		auto field = mono::get_field(klass, fieldName);
		T out{};
		mono::get_field_value(obj, field, &out);
		return out;
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

		if (count <= 0 || count > 100000)
			return out;

		out.reserve(count);
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

	inline Vector3 get_head_point(void* player)
	{
		static MonoMethod* method = mono::get_method("Character", "GetHeadPoint", 0, "assembly_valheim");

		if (!method || !player)
			return Vector3();

		auto obj = mono::invoke_method(method, player, nullptr);
		if (!obj)
			return Vector3();

		auto value = mono::object_unbox(obj);
		return value ? *static_cast<Vector3*>(value) : Vector3{};
	}

	inline Vector3 get_top_point(void* player)
	{
		static MonoMethod* method = mono::get_method("Character", "GetTopPoint", 0, "assembly_valheim");

		if (!method || !player)
			return Vector3();

		auto obj = mono::invoke_method(method, player, nullptr);
		if (!obj)
			return Vector3();

		auto value = mono::object_unbox(obj);
		return value ? *static_cast<Vector3*>(value) : Vector3{};
	}

	inline Vector3 get_velocity(void* player)
	{
		static MonoMethod* method = mono::get_method("Character", "GetVelocity", 0, "assembly_valheim");

		if (!method || !player)
			return Vector3();

		auto obj = mono::invoke_method(method, player, nullptr);
		if (!obj)
			return Vector3();

		auto value = mono::object_unbox(obj);
		return value ? *static_cast<Vector3*>(value) : Vector3{};
	}

	inline Vector3 get_position(void* player)
	{
		static MonoMethod* method = mono::get_method("Component", "get_transform", 0, "UnityEngine.CoreModule", "UnityEngine");
		static MonoMethod* get_position = mono::get_method("Transform", "get_position", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!method || !player || !get_position)
			return Vector3();

		auto transform = mono::invoke_method(method, player, nullptr);

		if (!transform)
			return Vector3();

		auto obj = mono::invoke_method(get_position, transform, nullptr);

		if (!obj)
			return Vector3();

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
	inline Vector3 get_forward(void* player)
	{
		Vector3 euler{};

		if (!player)
			return euler;

		static auto get_transform =
		    mono::get_method("Character", "GetTransform", 0, "assembly_valheim");

		static auto get_euler =
		    mono::get_method("Transform", "get_forward", 0, "UnityEngine.CoreModule", "UnityEngine");

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
	inline Vector3 get_camera_forward()
	{
		static MonoMethod* get_main = mono::get_method(
		    "Camera",
		    "get_main",
		    0,
		    "UnityEngine.CoreModule",
		    "UnityEngine");

		static MonoMethod* get_transform = mono::get_method(
		    "Component",
		    "get_transform",
		    0,
		    "UnityEngine.CoreModule",
		    "UnityEngine");

		static MonoMethod* get_forward = mono::get_method(
		    "Transform",
		    "get_forward",
		    0,
		    "UnityEngine.CoreModule",
		    "UnityEngine");

		if (!get_main || !get_transform || !get_forward)
			return {0, 0, 1};

		auto camera = mono::invoke_method(get_main, nullptr, nullptr);
		if (!camera)
			return {0, 0, 1};

		auto transform = mono::invoke_method(get_transform, camera, nullptr);
		if (!transform)
			return {0, 0, 1};

		auto forward_obj = mono::invoke_method(get_forward, transform, nullptr);
		if (!forward_obj)
			return {0, 0, 1};

		Vector3 forward = *(Vector3*)mono::object_unbox(forward_obj);

		return forward.normalize();
	}
	inline Vector3 get_zdo_vec3(void* player, int hash_name)
	{
		Vector3 result{};

		if (!player || !hash_name)
			return result;

		// 🔥 cache semua
		static MonoClass* character_class = mono::get_class("Character", "assembly_valheim");
		static MonoClass* znetview_class = mono::get_class("ZNetView", "assembly_valheim");
		static MonoClass* zdo_class = mono::get_class("ZDO", "assembly_valheim");

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

		mono::invoke_method(method, player, args);
	}

	inline int get_screen_width()
	{
		static MonoMethod* method = mono::get_method("Screen", "get_width", 0, "UnityEngine.CoreModule", "UnityEngine");
		if (!method)
			return g_pointers ? g_pointers->m_resolution.x : 1920;
		auto result = mono::invoke_method(method, nullptr, nullptr);
		if (!result)
			return g_pointers ? g_pointers->m_resolution.x : 1920;
		auto ptr = mono::object_unbox(result);
		return ptr ? *(int*)ptr : (g_pointers ? g_pointers->m_resolution.x : 1920);
	}

	inline int get_screen_height()
	{
		static MonoMethod* method = mono::get_method("Screen", "get_height", 0, "UnityEngine.CoreModule", "UnityEngine");
		if (!method)
			return g_pointers ? g_pointers->m_resolution.y : 1080;
		auto result = mono::invoke_method(method, nullptr, nullptr);
		if (!result)
			return g_pointers ? g_pointers->m_resolution.y : 1080;
		auto ptr = mono::object_unbox(result);
		return ptr ? *(int*)ptr : (g_pointers ? g_pointers->m_resolution.y : 1080);
	}

	inline bool world_to_screen(Vector3 const& world, Vector3& out)
	{
		static MonoMethod* get_main = mono::get_method("Camera", "get_main", 0, "UnityEngine.CoreModule", "UnityEngine");
		static MonoMethod* w2s_method = mono::get_method("Camera", "WorldToScreenPoint", 1, "UnityEngine.CoreModule", "UnityEngine");

		if (!get_main || !w2s_method)
			return false;

		auto camera = mono::invoke_method(get_main, nullptr, nullptr);
		if (!camera)
			return false;

		int eye = 2;

		auto result_obj = mono::invoke(w2s_method, camera, world);
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

	inline float fov_degrees_to_pixels(float fov_degrees, float screen_h = 0.f)
	{
		if (screen_h <= 0.f)
		{
			if (g_pointers)
				screen_h = (float)g_pointers->m_resolution.y;
			if (screen_h <= 0.f)
				screen_h = 1080.f;
		}

		constexpr float cam_fov = 65.f; // Standard Valheim camera FOV
		constexpr float half_cam_rad = (cam_fov * 0.5f) * (3.14159265359f / 180.f);
		float focal_length = (screen_h * 0.5f) / tanf(half_cam_rad);

		float clamped_fov = std::clamp(fov_degrees, 0.1f, 170.f);
		float half_target_rad = (clamped_fov * 0.5f) * (3.14159265359f / 180.f);

		return focal_length * tanf(half_target_rad);
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

	inline std::string get_name(void* obj)
	{
		if (!obj)
			return "unknown";

		static MonoMethod* get_name_obj = mono::get_method(
		    "Object",
		    "get_name",
		    0,
		    "UnityEngine.CoreModule",
		    "UnityEngine");

		if (!get_name_obj)
			return "unknown";

		// UnityEngine.Object (base of both GameObject and Component) has get_name directly
		auto name_obj = mono::invoke_method(get_name_obj, obj, nullptr);
		if (name_obj)
		{
			std::string str = mono::from_mono_string((MonoString*)name_obj);
			if (!str.empty())
				return str;
		}

		// Fallback in case a component was passed whose Object.get_name did not resolve
		static MonoMethod* get_go = mono::get_method(
		    "Component",
		    "get_gameObject",
		    0,
		    "UnityEngine.CoreModule",
		    "UnityEngine");

		if (get_go)
		{
			auto go = mono::invoke_method(get_go, obj, nullptr);
			if (go)
			{
				auto go_name_obj = mono::invoke_method(get_name_obj, go, nullptr);
				if (go_name_obj)
					return mono::from_mono_string((MonoString*)go_name_obj);
			}
		}

		return "unknown";
	}

	inline std::string get_hover_name(void* character)
	{
		static MonoMethod* method = mono::get_method(
		    "Character",
		    "GetHoverName",
		    0,
		    "assembly_valheim");

		if (!method || !character)
			return "unknown";

		auto name_obj = mono::invoke_method(method, character, nullptr);
		if (!name_obj)
			return "unknown";

		std::string result = mono::from_mono_string((MonoString*)name_obj);

		return result;
	}
	inline float get_health(void* character)
	{
		static MonoMethod* method = mono::get_method(
		    "Character",
		    "GetHealth",
		    0,
		    "assembly_valheim");

		if (!method || !character)
			return 0.f;

		auto obj = mono::invoke_method(method, character, nullptr);
		if (!obj)
			return 0.f;

		return *(float*)mono::object_unbox(obj);
	}
	inline bool is_dead(void* character)
	{
		static MonoMethod* method = mono::get_method(
		    "Character",
		    "IsDead",
		    0,
		    "assembly_valheim");

		if (!method || !character)
			return false;

		auto obj = mono::invoke_method(method, character, nullptr);
		if (!obj)
			return false;

		return *(bool*)mono::object_unbox(obj);
	}
	inline float get_max_health(void* character)
	{
		static MonoMethod* method = mono::get_method(
		    "Character",
		    "GetMaxHealth",
		    0,
		    "assembly_valheim");

		if (!method || !character)
			return 0.f;

		auto obj = mono::invoke_method(method, character, nullptr);
		if (!obj)
			return 0.f;

		return *(float*)mono::object_unbox(obj);
	}
	inline bool get_bounds(void* character, Vector3& top, Vector3& bottom)
	{
		static MonoMethod* get_top = mono::get_method(
		    "Character",
		    "GetTopPoint",
		    0,
		    "assembly_valheim");

		static MonoMethod* get_center = mono::get_method(
		    "Character",
		    "GetCenterPoint",
		    0,
		    "assembly_valheim");

		if (!character || !get_top || !get_center)
			return false;

		auto top_obj = mono::invoke_method(get_top, character, nullptr);
		auto center_obj = mono::invoke_method(get_center, character, nullptr);

		if (!top_obj || !center_obj)
			return false;

		top = *(Vector3*)mono::object_unbox(top_obj);
		Vector3 center = *(Vector3*)mono::object_unbox(center_obj);

		// 🔥 bottom = mirror dari center
		bottom = center - (top - center);

		return true;
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

	inline MonoObject* get_minimap()
	{
		static MonoMethod* get_inst = mono::get_method("Minimap", "get_instance", 0, "assembly_valheim");
		if (get_inst)
		{
			return mono::invoke_method(get_inst, nullptr, nullptr);
		}

		MonoClass* klass = mono::get_class("Minimap", "assembly_valheim");
		if (!klass)
			return nullptr;

		MonoClassField* field = mono::get_field(klass, "s_instance");
		if (!field)
			field = mono::get_field(klass, "m_instance");

		if (field)
		{
			void* static_data = mono::get_static_field_data(klass);
			if (static_data)
			{
				uint32_t offset = mono::get_field_offset(field);
				void* ptr_addr = (void*)((uintptr_t)static_data + offset);
				if (ptr_addr && *(MonoObject**)ptr_addr)
					return *(MonoObject**)ptr_addr;
			}
		}

		return nullptr;
	}

	inline void teleport_to_world_point(Vector3 pos)
	{
		MonoObject* minimap = get_minimap();
		if (minimap)
		{
			static MonoMethod* debug_teleport = mono::get_method("Minimap", "DebugTeleport", 1, "assembly_valheim");
			if (debug_teleport)
			{
				void* args[1] = {&pos};
				mono::invoke_method(debug_teleport, minimap, args);
				return;
			}
		}

		MonoObject* player = get_local_player();
		if (player)
		{
			static MonoMethod* tele_method = mono::get_method("Player", "TeleportTo", 3, "assembly_valheim");
			if (tele_method)
			{
				Vector3 target = Vector3(pos.x, pos.y + 2.0f, pos.z);
				static MonoMethod* get_transform = mono::get_method("Component", "get_transform", 0, "UnityEngine.CoreModule", "UnityEngine");
				static MonoMethod* get_rotation = mono::get_method("Transform", "get_rotation", 0, "UnityEngine.CoreModule", "UnityEngine");
				Quaternions rot(0.f, 0.f, 0.f, 1.f);
				if (get_transform && get_rotation)
				{
					MonoObject* trans = mono::invoke_method(get_transform, player, nullptr);
					if (trans)
					{
						MonoObject* rot_obj = mono::invoke_method(get_rotation, trans, nullptr);
						if (rot_obj)
							rot = *reinterpret_cast<Quaternions*>(mono::object_unbox(rot_obj));
					}
				}
				bool distant = true;
				void* args[3] = {&target, &rot, &distant};
				mono::invoke_method(tele_method, player, args);
			}
		}
	}

	struct map_pin_info
	{
		std::string name;
		Vector3 pos{};
		int type{0};
	};

	inline std::optional<Vector3> get_last_ping()
	{
		MonoObject* minimap = get_minimap();
		if (!minimap)
			return std::nullopt;

		MonoClass* minimap_class = mono::get_class("Minimap", "assembly_valheim");
		if (!minimap_class)
			return std::nullopt;

		MonoClassField* field = mono::get_field(minimap_class, "m_pingPins");
		if (!field)
			return std::nullopt;

		MonoObject* ping_list = nullptr;
		mono::get_field_value(minimap, field, &ping_list);
		if (!ping_list)
			return std::nullopt;

		auto pings = list_to_vector(ping_list);
		if (pings.empty())
			return std::nullopt;

		MonoObject* last_pin = pings.back();
		if (!last_pin)
			return std::nullopt;

		MonoClass* pin_class = mono::object_get_class(last_pin);
		if (!pin_class)
			return std::nullopt;

		MonoClassField* pos_field = mono::get_field(pin_class, "m_pos");
		if (!pos_field)
			return std::nullopt;

		Vector3 pos{};
		mono::get_field_value(last_pin, pos_field, &pos);
		return pos;
	}

	inline std::optional<Vector3> get_death_pin()
	{
		MonoObject* minimap = get_minimap();
		if (!minimap)
			return std::nullopt;

		MonoClass* minimap_class = mono::get_class("Minimap", "assembly_valheim");
		if (!minimap_class)
			return std::nullopt;

		MonoClassField* field = mono::get_field(minimap_class, "m_deathPin");
		if (!field)
			return std::nullopt;

		MonoObject* pin = nullptr;
		mono::get_field_value(minimap, field, &pin);
		if (!pin)
			return std::nullopt;

		MonoClass* pin_class = mono::object_get_class(pin);
		if (!pin_class)
			return std::nullopt;

		MonoClassField* pos_field = mono::get_field(pin_class, "m_pos");
		if (!pos_field)
			return std::nullopt;

		Vector3 pos{};
		mono::get_field_value(pin, pos_field, &pos);
		return pos;
	}

	inline std::optional<Vector3> get_spawn_pin()
	{
		MonoObject* minimap = get_minimap();
		if (!minimap)
			return std::nullopt;

		MonoClass* minimap_class = mono::get_class("Minimap", "assembly_valheim");
		if (!minimap_class)
			return std::nullopt;

		MonoClassField* field = mono::get_field(minimap_class, "m_spawnPointPin");
		if (!field)
			return std::nullopt;

		MonoObject* pin = nullptr;
		mono::get_field_value(minimap, field, &pin);
		if (!pin)
			return std::nullopt;

		MonoClass* pin_class = mono::object_get_class(pin);
		if (!pin_class)
			return std::nullopt;

		MonoClassField* pos_field = mono::get_field(pin_class, "m_pos");
		if (!pos_field)
			return std::nullopt;

		Vector3 pos{};
		mono::get_field_value(pin, pos_field, &pos);
		return pos;
	}

	inline std::vector<map_pin_info> get_all_map_pins()
	{
		std::vector<map_pin_info> result;
		MonoObject* minimap = get_minimap();
		if (!minimap)
			return result;

		MonoClass* minimap_class = mono::get_class("Minimap", "assembly_valheim");
		if (!minimap_class)
			return result;

		MonoClassField* field = mono::get_field(minimap_class, "m_pins");
		if (!field)
			return result;

		MonoObject* pin_list = nullptr;
		mono::get_field_value(minimap, field, &pin_list);
		if (!pin_list)
			return result;

		auto pins = list_to_vector(pin_list);

		static MonoClassField* pos_field = nullptr;
		static MonoClassField* name_field = nullptr;
		static MonoClassField* type_field = nullptr;

		for (auto* pin_obj : pins)
		{
			if (!pin_obj)
				continue;
			if (!pos_field || !name_field || !type_field)
			{
				MonoClass* pin_class = mono::object_get_class(pin_obj);
				if (pin_class)
				{
					pos_field = mono::get_field(pin_class, "m_pos");
					name_field = mono::get_field(pin_class, "m_name");
					type_field = mono::get_field(pin_class, "m_type");
				}
			}

			map_pin_info info{};
			if (pos_field)
				mono::get_field_value(pin_obj, pos_field, &info.pos);
			if (type_field)
				mono::get_field_value(pin_obj, type_field, &info.type);
			if (name_field)
			{
				MonoString* str = nullptr;
				mono::get_field_value(pin_obj, name_field, &str);
				if (str)
				{
					info.name = mono::from_mono_string(str);
				}
			}
			result.push_back(std::move(info));
		}
		return result;
	}

	inline MonoObject* get_game()
	{
		static MonoMethod* get_inst = mono::get_method("Game", "get_instance", 0, "assembly_valheim");
		if (get_inst)
		{
			return mono::invoke_method(get_inst, nullptr, nullptr);
		}

		MonoClass* klass = mono::get_class("Game", "assembly_valheim");
		if (!klass)
			return nullptr;

		MonoClassField* field = mono::get_field(klass, "s_instance");
		if (!field)
			field = mono::get_field(klass, "m_instance");

		if (field)
		{
			void* static_data = mono::get_static_field_data(klass);
			if (static_data)
			{
				uint32_t offset = mono::get_field_offset(field);
				void* ptr_addr = (void*)((uintptr_t)static_data + offset);
				if (ptr_addr && *(MonoObject**)ptr_addr)
					return *(MonoObject**)ptr_addr;
			}
		}

		return nullptr;
	}

	inline void explore_all_map()
	{
		MonoObject* minimap = get_minimap();
		if (minimap)
		{
			static MonoMethod* method = mono::get_method("Minimap", "ExploreAll", 0, "assembly_valheim");
			if (method)
			{
				mono::invoke_method(method, minimap, nullptr);
				LOG(INFO) << "ExploreAll executed successfully";
			}
		}
	}

	inline void reset_map()
	{
		MonoObject* minimap = get_minimap();
		if (minimap)
		{
			static MonoMethod* method = mono::get_method("Minimap", "Reset", 0, "assembly_valheim");
			if (method)
			{
				mono::invoke_method(method, minimap, nullptr);
				LOG(INFO) << "Reset map executed successfully";
			}
		}
	}

	inline void discover_closest_location(std::string_view name, std::string_view pin_name, int pin_type, bool show_map = false, bool discover_all = false)
	{
		MonoObject* game = get_game();
		if (!game)
		{
			LOG(WARNING) << "Game instance not found for discover_closest_location";
			return;
		}

		auto local_player = get_local_player();
		Vector3 pos = local_player ? get_position(local_player) : Vector3{0.f, 0.f, 0.f};

		static MonoMethod* method = mono::get_method("Game", "DiscoverClosestLocation", 6, "assembly_valheim");
		if (!method)
		{
			LOG(WARNING) << "Failed to find Game::DiscoverClosestLocation";
			return;
		}

		MonoString* mono_name = mono::to_mono_string(std::string(name));
		MonoString* mono_pin = mono::to_mono_string(std::string(pin_name));

		void* args[6] = {
		    mono_name,
		    &pos,
		    mono_pin,
		    &pin_type,
		    &show_map,
		    &discover_all};

		mono::invoke_method(method, game, args);
	}

	inline void discover_bosses_and_traders()
	{
		struct boss_entry
		{
			const char* location_name;
			const char* pin_name;
			int pin_type;
		};

		static const boss_entry entries[] = {
		    {"Eikthyrnir", "Eikthyr", 9},
		    {"GDKing", "The Elder", 9},
		    {"Bonemass", "Bonemass", 9},
		    {"Dragonqueen", "Moder", 9},
		    {"GoblinKing", "Yagluth", 9},
		    {"SeekerQueen", "The Queen", 9},
		    {"Fader", "Fader", 9},
		    {"Vendor_BlackForest", "Haldor", 2},
		    {"Hildir_camp", "Hildir", 16},
		    {"Hildir_crypt", "Hildir Crypt", 17},
		    {"Hildir_cave", "Hildir Cave", 17},
		    {"Hildir_tower", "Hildir Tower", 18}};

		for (const auto& entry : entries)
		{
			discover_closest_location(entry.location_name, entry.pin_name, entry.pin_type, false, false);
		}
		LOG(INFO) << "Discover bosses and traders requested";
	}
}
