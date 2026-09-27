#include "monster_ai.hpp"
#include "character.hpp"

namespace big
{
	monster_ai::monster_ai(MonoObject* obj) :
	    m_monster_ai(obj)
	{
	}

	monster_ai::~monster_ai() noexcept
	{
		m_monster_ai = nullptr;
	}

	void monster_ai::make_tame()
	{
		if (!m_monster_ai)
			return;

		static auto method = mono::get_method("MonsterAI", "MakeTame", 0, "assembly_valheim");
		if (method)
			mono::invoke_method(method, m_monster_ai, nullptr);
	}

	void monster_ai::set_alerted(bool alerted)
	{
		if (!m_monster_ai)
			return;

		static auto method = mono::get_method("MonsterAI", "SetAlerted", 1, "assembly_valheim");
		if (method)
		{
			void* args[1] = {&alerted};
			mono::invoke_method(method, m_monster_ai, args);
		}
	}

	bool monster_ai::is_alerted()
	{
		if (!m_monster_ai)
			return false;

		static auto method = mono::get_method("MonsterAI", "IsAlerted", 0, "assembly_valheim");
		if (!method)
			return false;

		auto res = mono::invoke_method(method, m_monster_ai, nullptr);
		return res ? *static_cast<bool*>(mono::object_unbox(res)) : false;
	}

	void monster_ai::set_target(character target)
	{
		if (!m_monster_ai || !target)
			return;

		static auto method = mono::get_method("MonsterAI", "SetTarget", 1, "assembly_valheim");
		if (method)
		{
			void* args[1] = {target.get_object()};
			mono::invoke_method(method, m_monster_ai, args);
		}
	}

	MonoObject* monster_ai::get_nview()
	{
		if (!m_monster_ai)
			return nullptr;

		static auto klass = mono::get_class("MonsterAI", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_nview") : nullptr;
		if (!field)
			return nullptr;

		MonoObject* nview = nullptr;
		mono::get_field_value(m_monster_ai, field, &nview);
		return nview;
	}
}
