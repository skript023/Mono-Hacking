#pragma once
#include "core.hpp"

namespace big
{
    template<typename T>
    class mono_array_iterator
    {
    public:
        mono_array_iterator(MonoArray* arr, int index)
            : m_array(arr), m_index(index)
        {
        }

        T operator*() const
        {
            auto raw = *reinterpret_cast<MonoObject**>(mono_core::mono_array_addr_with_size(m_array, sizeof(MonoObject*), m_index));

            if constexpr (std::is_same_v<T, MonoObject*>)
                return raw;
            else
                return T(raw);
        }

        mono_array_iterator& operator++()
        {
            ++m_index;
            return *this;
        }

        bool operator!=(const mono_array_iterator& other) const
        {
            return m_index != other.m_index;
        }

    private:
        MonoArray* m_array{};
        int m_index{};
    };

    template<typename T>
    class mono_array_view
    {
    public:
        mono_array_view() = default;

        mono_array_view(MonoArray* arr, int size = -1)
            : m_array(arr), m_size(size) {}

        int size() const
        {
            if (!m_array) return 0;
            if (m_size >= 0) return m_size;

            return mono_core::mono_array_length(m_array);
        }

        T operator[](int index) const
        {
            auto raw = *reinterpret_cast<MonoObject**>(
                mono_core::mono_array_addr_with_size(m_array, sizeof(MonoObject*), index)
            );

            if constexpr (std::is_same_v<T, MonoObject*>)
                return raw;
            else
                return T(raw);
        }

        inline T at(int index) const
        {
            if (!m_array || index < 0 || index >= size())
                return T{};
            return (*this)[index];
        }

        auto begin() const
        {
            return mono_array_iterator<T>(m_array, 0);
        }

        auto end() const
        {
            return mono_array_iterator<T>(m_array, size());
        }
    private:
        MonoArray* m_array{};
        int m_size{-1};
    };
}