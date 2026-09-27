#pragma once
#include "mono/mono.hpp"

namespace big
{
	class character;

	class monster_ai
	{
	private:
		MonoObject* m_monster_ai{};

	public:
		monster_ai(MonoObject* obj = nullptr);
		~monster_ai() noexcept;

		MonoObject* get_object() const
		{
			return m_monster_ai;
		}

		void make_tame();
		void set_alerted(bool alerted);
		bool is_alerted();
		void set_target(character target);
		MonoObject* get_nview();

		bool operator==(const monster_ai& other) const
		{
			return m_monster_ai == other.m_monster_ai;
		}
		operator bool() const
		{
			return m_monster_ai != nullptr;
		}
	};
}
