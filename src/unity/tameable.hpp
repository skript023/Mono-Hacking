#pragma once
#include "mono/mono.hpp"

namespace big
{
	class character;

	class tameable
	{
	private:
		MonoObject* m_tameable{};

	public:
		tameable(MonoObject* obj = nullptr);
		~tameable() noexcept;

		MonoObject* get_object() const
		{
			return m_tameable;
		}

		bool is_tamed();
		int get_tameness();
		float get_remaining_time();
		bool is_hungry();
		bool is_alerted();
		bool tame();
		void command(character player);
		void set_tame_time_left(float time);
		MonoObject* get_nview();

		bool operator==(const tameable& other) const
		{
			return m_tameable == other.m_tameable;
		}
		operator bool() const
		{
			return m_tameable != nullptr;
		}
	};
}
