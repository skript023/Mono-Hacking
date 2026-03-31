#include "character.hpp"
#include "class/vector.hpp"

namespace big
{
    character::character(MonoObject* character): m_character(character)
	{
		
	}
	character::~character() noexcept
	{
		m_character = nullptr;
	}
    void character::set_max_health(float health)
	{
		auto method = mono::get_method("Character", "SetMaxHealth", 1, "assembly_valheim");
		
		if (!method )
		{
			LOG(WARNING) << "Failed to find method Character::SetMaxHealth";

			return;
		}

		float max_hp = health;

		std::array<void*, 1> args{};

		args[0] = &max_hp;

		mono::invoke_method(method, m_character, args.data());
	}
	float character::get_max_health()
	{
		auto method = mono::get_method("Character", "GetMaxHealth", 0, "assembly_valheim");
		
		if (!method )
		{
			LOG(WARNING) << "Failed to find method Character::GetMaxHealth";

			return 0.f;
		}

		auto result = mono::invoke_method(method, m_character, nullptr);

		return *reinterpret_cast<float*>(mono::object_unbox(result));
	}
	Vector3 character::get_center_point()
	{
		Vector3 pos{};

		if (!m_character)
			return pos;

		static auto method = mono::get_method("Character", "GetCenterPoint", 0, "assembly_valheim");

		if (!method)
			return pos;

		auto obj = mono::invoke_method(method, m_character, nullptr);
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

		auto transform = mono::invoke_method(get_transform, m_character, nullptr);
		if (!transform)
			return euler;

		auto obj = mono::invoke_method(get_euler, transform, nullptr);
		if (!obj)
			return euler;

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}
	Vector3 character::get_head_point()
	{
		static MonoMethod* method = mono::get_method("Character", "GetHeadPoint", 0, "assembly_valheim");

		if (!method || !m_character)
			return Vector3();

		auto obj = mono::invoke_method(method, m_character, nullptr);

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector3 character::get_top_point()
	{
		static MonoMethod* method = mono::get_method("Character", "GetTopPoint", 0, "assembly_valheim");

		if (!method || !m_character)
			return Vector3();

		auto obj = mono::invoke_method(method, m_character, nullptr);

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector3 character::get_velocity()
	{
		static MonoMethod* method = mono::get_method("Character", "GetVelocity", 0, "assembly_valheim");

		if (!method || !m_character)
			return Vector3();

		auto obj = mono::invoke_method(method, m_character, nullptr);

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector3 character::get_position()
	{
		static MonoMethod* method = mono::get_method("Character", "GetTransform", 0, "assembly_valheim");
		static MonoMethod* get_position = mono::get_method("Transform", "get_position", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!method || !m_character || !get_position)
			return Vector3();

		auto transform = mono::invoke_method(method, m_character, nullptr);

    	auto obj = mono::invoke_method(get_position, transform, nullptr);

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}

	Vector4 character::get_rotation()
	{
		Vector4 rot{0.f, 0.f, 0.f, 1.f}; // default identity

		if (!m_character)
			return rot;

		static MonoMethod* get_transform = mono::get_method("Character", "GetTransform", 0, "assembly_valheim");

		static MonoMethod* get_rotation_method = mono::get_method("Transform", "get_rotation", 0, "UnityEngine.CoreModule", "UnityEngine");

		if (!get_transform || !get_rotation_method)
			return rot;

		auto transform = mono::invoke_method(get_transform, m_character, nullptr);
		if (!transform)
			return rot;

		auto obj = mono::invoke_method(get_rotation_method, transform, nullptr);
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

		auto transform = mono::invoke_method(get_transform, m_character, nullptr);
		if (!transform)
			return euler;

		auto obj = mono::invoke_method(get_euler, transform, nullptr);
		if (!obj)
			return euler;

		return *reinterpret_cast<Vector3*>(mono::object_unbox(obj));
	}
}