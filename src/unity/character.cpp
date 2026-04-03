#include "character.hpp"
#include "class/vector.hpp"
#include "utility/unity.hpp"

namespace big
{
    character::character(MonoObject* character): m_character(character)
	{
		
	}
	character::~character() noexcept
	{
		m_character = nullptr;
	}
	MonoObject* character::get_object()
	{
		return m_character;
	}
	void character::set_health(float health)
	{
		auto method = mono::get_method("Character", "SetHealth", 1, "assembly_valheim");
		
		if (!method )
		{
			LOG(WARNING) << "Failed to find method Character::SetHealth";

			return;
		}

		mono::invoke(method, m_character, health);
	}
	void character::set_max_health(float health)
	{
		auto method = mono::get_method("Character", "SetMaxHealth", 1, "assembly_valheim");
		
		if (!method )
		{
			LOG(WARNING) << "Failed to find method Character::SetMaxHealth";

			return;
		}

		mono::invoke(method, m_character, health);
	}
	void character::set_tamed(bool tamed)
	{
		auto method = mono::get_method("Character", "SetTamed", 1, "assembly_valheim");
		
		if (!method )
		{
			LOG(WARNING) << "Failed to find method Character::SetTamed";

			return;
		}

		mono::invoke(method, m_character, tamed);
	}
	float character::get_max_health()
	{
		auto method = mono::get_method("Character", "GetMaxHealth", 0, "assembly_valheim");
		
		if (!method )
		{
			LOG(WARNING) << "Failed to find method Character::GetMaxHealth";

			return 0.f;
		}

		auto result = mono::invoke(method, m_character);

		return *reinterpret_cast<float*>(mono::object_unbox(result));
	}
	std::string character::get_hover_name()
	{
		static MonoMethod* method = mono::get_method(
			"Character",
			"GetHoverName",
			0,
			"assembly_valheim"
		);

		if (!method || !m_character)
			return "unknown";

		auto now = std::chrono::steady_clock::now();

		
		if (auto it = m_hover_name_cache.find(m_character);it != m_hover_name_cache.end())
		{
			if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_update).count() < 2)
			{
				return it->second.name;
			}
		}

		auto name_obj = mono::invoke(method, m_character);

		if (!name_obj)
			return "unknown";

		std::string result = mono::from_mono_string(reinterpret_cast<MonoString*>(name_obj));

		m_hover_name_cache[m_character] = { result, now };

		return result;
	}
	float character::get_health()
	{
		static MonoMethod* method = mono::get_method(
			"Character",
			"GetHealth",
			0,
			"assembly_valheim"
		);

		if (!method || !m_character)
			return 0.f;

		auto obj = mono::invoke(method, m_character);
		if (!obj)
			return 0.f;

		return *reinterpret_cast<float*>(mono::object_unbox(obj));
	}
	bool character::is_dead()
	{
		static MonoMethod* method = mono::get_method(
			"Character",
			"IsDead",
			0,
			"assembly_valheim"
		);

		if (!method || !m_character)
			return false;

		auto obj = mono::invoke(method, m_character);
		if (!obj)
			return false;

		return *reinterpret_cast<bool*>(mono::object_unbox(obj));
	}
	bool character::is_player()
	{
		return false;
	}

	Vector3 character::get_center_point()
	{
		Vector3 pos{};

		if (!m_character)
			return pos;

		static auto method = mono::get_method("Character", "GetCenterPoint", 0, "assembly_valheim");

		if (!method)
			return pos;

		auto obj = mono::invoke(method, m_character);
		if (!obj)
			return pos;

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}
	Vector3 character::get_forward()
	{
		Vector3 euler{};

		if (!m_character)
			return euler;

		static auto get_transform =
			mono::get_method("Character", "GetTransform", 0, "assembly_valheim");

		static auto get_euler =
			mono::get_method("Transform", "get_forward", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!get_transform || !get_euler)
			return euler;

		auto transform = mono::invoke(get_transform, m_character);
		if (!transform)
			return euler;

		auto obj = mono::invoke(get_euler, transform);
		if (!obj)
			return euler;

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}
	Vector3 character::get_head_point()
	{
		static MonoMethod* method = mono::get_method("Character", "GetHeadPoint", 0, "assembly_valheim");

		if (!method || !m_character)
			return Vector3();

		auto obj = mono::invoke(method, m_character);

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector3 character::get_top_point()
	{
		static MonoMethod* method = mono::get_method("Character", "GetTopPoint", 0, "assembly_valheim");

		if (!method || !m_character)
			return Vector3();

		auto obj = mono::invoke(method, m_character);

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector3 character::get_velocity()
	{
		static MonoMethod* method = mono::get_method("Character", "GetVelocity", 0, "assembly_valheim");

		if (!method || !m_character)
			return Vector3();

		auto obj = mono::invoke(method, m_character);

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector3 character::get_position()
	{
		static auto method = mono::get_method("Character", "GetTransform", 0, "assembly_valheim");
		static auto get_position = mono::get_method("Transform", "get_position", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!method || !m_character || !get_position)
			return Vector3();

		auto transform = mono::invoke(method, m_character);

		if (!transform) return Vector3();

    	auto obj = mono::invoke(get_position, transform);

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector4 character::get_rotation()
	{
		Vector4 rot{0.f, 0.f, 0.f, 1.f}; // default identity

		if (!m_character)
			return rot;

		static auto get_transform = mono::get_method("Character", "GetTransform", 0, "assembly_valheim");

		static auto get_rotation_method = mono::get_method("Transform", "get_rotation", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!get_transform || !get_rotation_method)
			return rot;

		auto transform = mono::invoke(get_transform, m_character);
		if (!transform)
			return rot;

		auto obj = mono::invoke(get_rotation_method, transform);
		if (!obj)
			return rot;

		return *reinterpret_cast<Vector4*>(mono::object_unbox(obj)); // Quaternion (x,y,z,w)
	}
	Vector3 character::get_euler_angles()
	{
		Vector3 euler{};

		if (!m_character)
			return euler;

		static auto get_transform = mono::get_method("Character", "GetTransform", 0, "assembly_valheim");

		static auto get_euler = mono::get_method("Transform", "get_eulerAngles", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!get_transform || !get_euler)
			return euler;

		auto transform = mono::invoke(get_transform, m_character);
		if (!transform)
			return euler;

		auto obj = mono::invoke(get_euler, transform);
		if (!obj)
			return euler;

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}
	mono_array_view<character> character::get_all_characters()
	{
		static MonoMethod* method = mono::get_method("Character", "GetAllCharacters", 0, "assembly_valheim");

		if (!method)
			return {};

		MonoObject* result = mono::invoke(method, nullptr);
#ifdef _DEBUG
		MonoClass* klass = mono::object_get_class(result);
		const char* class_name = mono::class_get_name(klass);
		const char* namespace_name = mono::class_get_namespace(klass);

		LOG(INFO) << "Result class: " << namespace_name << "::" << class_name;
#endif
		return mono::list<character>(result);
	}
	mono_array_view<character> character::get_all_scharacters()
	{
		auto result = mono::get_static_field_value<"Character", "s_characters", MonoObject*>();
#ifdef _DEBUG
		MonoClass* klass = mono::object_get_class(result);
		const char* class_name = mono::class_get_name(klass);
		const char* namespace_name = mono::class_get_namespace(klass);

		LOG(INFO) << "Result class: " << namespace_name << "::" << class_name;
#endif
		return mono::list<character>(result);
	}
}