#pragma once
#include "mono/mono.hpp"

namespace big
{
	class procreation
	{
	private:
		MonoObject* m_procreation{};

	public:
		procreation(MonoObject* obj = nullptr);
		~procreation() noexcept;

		MonoObject* get_object() const
		{
			return m_procreation;
		}

		bool is_pregnant();
		int get_love_points();
		int get_required_love_points();

		bool operator==(const procreation& other) const
		{
			return m_procreation == other.m_procreation;
		}
		operator bool() const
		{
			return m_procreation != nullptr;
		}
	};
}
