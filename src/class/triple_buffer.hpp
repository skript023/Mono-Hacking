#pragma once

namespace big
{
    template <typename T>
    class TripleBuffer
    {
    public:
        using container_type = std::vector<T>;

        TripleBuffer(size_t reserve_size = 0)
        {
            for (auto& buf : m_buffers)
                buf.reserve(reserve_size);

            m_front_index.store(0, std::memory_order_relaxed);
            m_back_index = 1;
            m_spare_index = 2;
        }

        // Writer: isi buffer ini
        container_type& back() noexcept
        {
            return m_buffers[m_back_index];
        }

        // Writer: publish hasil write
        void publish() noexcept
        {
            // rotate buffers:
            // back -> front
            // front -> spare
            // spare -> back

            const int old_front = m_front_index.load(std::memory_order_relaxed);

            m_front_index.store(m_back_index, std::memory_order_release);

            m_back_index = m_spare_index;
            m_spare_index = old_front;

            // clear buffer baru untuk write berikutnya
            m_buffers[m_back_index].clear();
        }

        // Reader: ambil snapshot (NO LOCK)
        const container_type& view() const noexcept
        {
            return m_buffers[m_front_index.load(std::memory_order_acquire)];
        }

        void clear_all() noexcept
        {
            for (auto buffer : m_buffers)
                buffer.clear();
        }
    private:
        std::array<container_type, 3> m_buffers;

        std::atomic<int> m_front_index;
        int m_back_index;
        int m_spare_index;
    };
}