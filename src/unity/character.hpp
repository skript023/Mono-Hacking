#pragma once
#include "mono/mono.hpp"

namespace big
{
	class character
	{
	protected:
		MonoObject* m_character{};
	public:
		character(MonoObject* character);
		~character() noexcept;

		void set_max_health(float health);
		float get_max_health();
		Vector3 get_center_point();
		Vector3 get_forward();
		Vector3 get_head_point();
		Vector3 get_top_point();
		Vector3 get_velocity();
		Vector3 get_position();
		Vector4 get_rotation();
		Vector3 get_euler_angles();
	};
}