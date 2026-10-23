#pragma once
#include "mono/mono.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace big::features
{
	template<typename T, size_t N>
	class field_override
	{
		struct entry
		{
			uintptr_t handle{};
			std::array<T, N> original{};
		};

		const char* m_class_name;
		std::array<const char*, N> m_names;
		std::array<MonoClassField*, N> m_fields{};
		bool m_resolved = false;
		std::vector<entry> m_entries;

		bool resolve()
		{
			if (!m_resolved)
			{
				auto klass = mono::get_class(m_class_name, "assembly_valheim");
				if (!klass)
				{
					return false;
				}
				for (size_t i = 0; i < N; ++i)
				{
					m_fields[i] = mono::get_field(klass, m_names[i]);
				}
				m_resolved = true;
			}
			return std::ranges::all_of(m_fields, [](MonoClassField* field) {
				return field != nullptr;
			});
		}

	public:
		field_override(const char* class_name, std::array<const char*, N> names) :
		    m_class_name(class_name),
		    m_names(names)
		{
		}

		const std::array<T, N>* original(MonoObject* object)
		{
			if (!object || !resolve())
			{
				return nullptr;
			}
			for (const auto& recorded : m_entries)
			{
				if (mono::retained_object(recorded.handle) == object)
				{
					return &recorded.original;
				}
			}

			entry recorded{mono::retain(object)};
			if (!recorded.handle)
			{
				return nullptr;
			}
			for (size_t i = 0; i < N; ++i)
			{
				mono::get_field_value(object, m_fields[i], &recorded.original[i]);
			}
			return &m_entries.emplace_back(recorded).original;
		}

		void write(MonoObject* object, std::array<T, N> values)
		{
			for (size_t i = 0; i < N; ++i)
			{
				mono::set_field_value(object, m_fields[i], &values[i]);
			}
		}

		void restore()
		{
			for (const auto& recorded : m_entries)
			{
				if (auto object = mono::retained_object(recorded.handle))
				{
					write(object, recorded.original);
				}
				mono::release(recorded.handle);
			}
			m_entries.clear();
		}

		void forget_missing(const std::vector<MonoObject*>& alive)
		{
			std::erase_if(m_entries, [&](const entry& recorded) {
				if (std::ranges::find(alive, mono::retained_object(recorded.handle)) != alive.end())
				{
					return false;
				}
				mono::release(recorded.handle);
				return true;
			});
		}
	};
}
