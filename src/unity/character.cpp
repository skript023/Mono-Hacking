#include "character.hpp"
#include "class/vector.hpp"
#include "monster_ai.hpp"
#include "procreation.hpp"
#include "tameable.hpp"
#include "utility/unity.hpp"

namespace big
{
	character::character(MonoObject* character) :
	    m_character(character)
	{
	}
	character::~character() noexcept
	{
		m_character = nullptr;
		m_hover_name_cache.clear();
	}
	MonoObject* character::get_object() const
	{
		return m_character;
	}
	void character::set_health(float health)
	{
		auto method = mono::get_method("Character", "SetHealth", 1, "assembly_valheim");

		if (!method)
		{
			LOG(WARNING) << "Failed to find method Character::SetHealth";

			return;
		}

		mono::invoke(method, m_character, health);
	}
	void character::set_max_health(float health)
	{
		auto method = mono::get_method("Character", "SetMaxHealth", 1, "assembly_valheim");

		if (!method)
		{
			LOG(WARNING) << "Failed to find method Character::SetMaxHealth";

			return;
		}

		mono::invoke(method, m_character, health);
	}
	void character::set_tamed(bool tamed)
	{
		auto method = mono::get_method("Character", "SetTamed", 1, "assembly_valheim");

		if (!method)
		{
			LOG(WARNING) << "Failed to find method Character::SetTamed";

			return;
		}

		mono::invoke(method, m_character, tamed);
	}
	bool character::is_tamed()
	{
		if (!m_character)
			return false;
		static auto method = mono::get_method("Character", "IsTamed", 0, "assembly_valheim");
		if (method)
		{
			auto res = mono::invoke_method(method, m_character, nullptr);
			if (res)
				return *static_cast<bool*>(mono::object_unbox(res));
		}
		static auto klass = mono::get_class("Character", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_tamed") : nullptr;
		if (field)
		{
			bool tamed = false;
			mono::get_field_value(m_character, field, &tamed);
			return tamed;
		}
		return false;
	}
	int character::get_level()
	{
		if (!m_character)
			return 1;
		static auto method = mono::get_method("Character", "GetLevel", 0, "assembly_valheim");
		if (method)
		{
			auto res = mono::invoke_method(method, m_character, nullptr);
			if (res)
				return *static_cast<int*>(mono::object_unbox(res));
		}
		static auto klass = mono::get_class("Character", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_level") : nullptr;
		if (field)
		{
			int lvl = 1;
			mono::get_field_value(m_character, field, &lvl);
			return lvl;
		}
		return 1;
	}
	float character::get_max_health()
	{
		static auto method = mono::get_method("Character", "GetMaxHealth", 0, "assembly_valheim");

		if (!method)
		{
			LOG(WARNING) << "Failed to find method Character::GetMaxHealth";

			return 0.f;
		}

		auto result = mono::invoke(method, m_character);
		if (!result)
			return 0.f;

		return *reinterpret_cast<float*>(mono::object_unbox(result));
	}
	std::string character::get_hover_name()
	{
		static MonoMethod* method = mono::get_method(
		    "Character",
		    "GetHoverName",
		    0,
		    "assembly_valheim");

		if (!method || !m_character)
			return "unknown";

		auto name_obj = mono::invoke(method, m_character);

		if (!name_obj)
			return "unknown";

		auto result = mono::from_mono_string(reinterpret_cast<MonoString*>(name_obj));

		return result;
	}
	float character::get_health()
	{
		static MonoMethod* method = mono::get_method(
		    "Character",
		    "GetHealth",
		    0,
		    "assembly_valheim");

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
		    "assembly_valheim");

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
		if (!obj)
			return Vector3();

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector3 character::get_top_point()
	{
		static MonoMethod* method = mono::get_method("Character", "GetTopPoint", 0, "assembly_valheim");

		if (!method || !m_character)
			return Vector3();

		auto obj = mono::invoke(method, m_character);
		if (!obj)
			return Vector3();

		auto value = mono::object_unbox(obj);
		return value ? *static_cast<Vector3*>(value) : Vector3{};
	}

	Vector3 character::get_velocity()
	{
		static MonoMethod* method = mono::get_method("Character", "GetVelocity", 0, "assembly_valheim");

		if (!method || !m_character)
			return Vector3();

		auto obj = mono::invoke(method, m_character);
		if (!obj)
			return Vector3();

		auto value = mono::object_unbox(obj);
		return value ? *static_cast<Vector3*>(value) : Vector3{};
	}

	Vector3 character::get_position()
	{
		static auto method = mono::get_method("Character", "GetTransform", 0, "assembly_valheim");
		static auto get_position = mono::get_method("Transform", "get_position", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!method || !m_character || !get_position)
			return Vector3();

		auto transform = mono::invoke(method, m_character);

		if (!transform)
			return Vector3();

		auto obj = mono::invoke(get_position, transform);
		if (!obj)
			return Vector3();

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

	MonoObject* character::get_component(const char* class_name, const char* assembly_name, const char* namespace_name)
	{
		if (!m_character)
			return nullptr;
		auto klass = mono::get_class(class_name, assembly_name, namespace_name);
		if (!klass)
			return nullptr;
		auto mono_type = mono::reflection_type(klass);
		if (!mono_type)
			return nullptr;
		static auto comp_method = mono::get_method_overload("Component", "GetComponent", 1, nullptr, "Type", "UnityEngine.CoreModule", "UnityEngine");
		if (!comp_method)
			return nullptr;
		void* args[1] = {mono_type};
		return mono::invoke_method(comp_method, m_character, args);
	}

	MonoObject* character::get_nview()
	{
		if (!m_character)
			return nullptr;
		static auto klass = mono::get_class("Character", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_nview") : nullptr;
		if (field)
		{
			MonoObject* nview = nullptr;
			mono::get_field_value(m_character, field, &nview);
			if (nview)
				return nview;
		}
		return get_component("ZNetView");
	}

	tameable character::get_tameable()
	{
		return tameable(get_component("Tameable"));
	}

	monster_ai character::get_monster_ai()
	{
		return monster_ai(get_component("MonsterAI"));
	}

	procreation character::get_procreation()
	{
		return procreation(get_component("Procreation"));
	}
}
