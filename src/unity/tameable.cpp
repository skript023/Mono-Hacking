#include "tameable.hpp"
#include "character.hpp"

namespace big
{
	tameable::tameable(MonoObject* obj) :
	    m_tameable(obj)
	{
	}

	tameable::~tameable() noexcept
	{
		m_tameable = nullptr;
	}

	bool tameable::is_tamed()
	{
		if (!m_tameable)
			return false;

		static auto method = mono::get_method("Tameable", "IsTamed", 0, "assembly_valheim");
		if (method)
		{
			auto res = mono::invoke_method(method, m_tameable, nullptr);
			if (res)
				return *static_cast<bool*>(mono::object_unbox(res));
		}

		static auto klass = mono::get_class("Tameable", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_character") : nullptr;
		if (field)
		{
			MonoObject* char_obj = nullptr;
			mono::get_field_value(m_tameable, field, &char_obj);
			if (char_obj)
				return character(char_obj).is_tamed();
		}
		return false;
	}

	int tameable::get_tameness()
	{
		if (!m_tameable)
			return 0;

		static auto method = mono::get_method("Tameable", "GetTameness", 0, "assembly_valheim");
		if (method)
		{
			auto res = mono::invoke_method(method, m_tameable, nullptr);
			if (res)
			{
				float val = *static_cast<float*>(mono::object_unbox(res));
				return static_cast<int>(val * 100.f);
			}
		}
		return 0;
	}

	float tameable::get_remaining_time()
	{
		if (!m_tameable)
			return 0.f;

		static auto method = mono::get_method("Tameable", "GetRemainingTime", 0, "assembly_valheim");
		if (method)
		{
			auto res = mono::invoke_method(method, m_tameable, nullptr);
			if (res)
				return *static_cast<float*>(mono::object_unbox(res));
		}

		static auto klass = mono::get_class("Tameable", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_tameTimeLeft") : nullptr;
		if (field)
		{
			float time = 0.f;
			mono::get_field_value(m_tameable, field, &time);
			return time;
		}
		return 0.f;
	}

	bool tameable::is_hungry()
	{
		if (!m_tameable)
			return false;

		static auto method = mono::get_method("Tameable", "IsHungry", 0, "assembly_valheim");
		if (!method)
			return false;

		auto res = mono::invoke_method(method, m_tameable, nullptr);
		return res ? *static_cast<bool*>(mono::object_unbox(res)) : false;
	}

	bool tameable::is_alerted()
	{
		if (!m_tameable)
			return false;

		static auto method = mono::get_method("Tameable", "IsAlerted", 0, "assembly_valheim");
		if (!method)
			return false;

		auto res = mono::invoke_method(method, m_tameable, nullptr);
		return res ? *static_cast<bool*>(mono::object_unbox(res)) : false;
	}

	bool tameable::tame()
	{
		if (!m_tameable)
			return false;

		set_tame_time_left(0.f);

		static auto tame_m = mono::get_method("Tameable", "Tame", 0, "assembly_valheim");
		if (tame_m)
		{
			mono::invoke_method(tame_m, m_tameable, nullptr);
			return true;
		}
		return false;
	}

	void tameable::command(character player)
	{
		if (!m_tameable || !player)
			return;

		static auto method = mono::get_method("Tameable", "Command", 1, "assembly_valheim");
		if (!method)
			return;

		void* args[1] = {player.get_object()};
		mono::invoke_method(method, m_tameable, args);
	}

	void tameable::set_tame_time_left(float time)
	{
		if (!m_tameable)
			return;

		static auto klass = mono::get_class("Tameable", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_tameTimeLeft") : nullptr;
		if (field)
			mono::set_field_value(m_tameable, field, &time);
	}

	MonoObject* tameable::get_nview()
	{
		if (!m_tameable)
			return nullptr;

		static auto klass = mono::get_class("Tameable", "assembly_valheim");
		static auto field = klass ? mono::get_field(klass, "m_nview") : nullptr;
		if (!field)
			return nullptr;

		MonoObject* nview = nullptr;
		mono::get_field_value(m_tameable, field, &nview);
		return nview;
	}
}
