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
            for (auto& buf : buffers_)
                buf.reserve(reserve_size);

            front_index_.store(0, std::memory_order_relaxed);
            back_index_ = 1;
            spare_index_ = 2;
        }

        // Writer: isi buffer ini
        container_type& back() noexcept
        {
            return buffers_[back_index_];
        }

        // Writer: publish hasil write
        void publish() noexcept
        {
            // rotate buffers:
            // back -> front
            // front -> spare
            // spare -> back

            const int old_front = front_index_.load(std::memory_order_relaxed);

            front_index_.store(back_index_, std::memory_order_release);

            back_index_ = spare_index_;
            spare_index_ = old_front;

            // clear buffer baru untuk write berikutnya
            buffers_[back_index_].clear();
        }

        // Reader: ambil snapshot (NO LOCK)
        const container_type& view() const noexcept
        {
            return buffers_[front_index_.load(std::memory_order_acquire)];
        }

    private:
        std::array<container_type, 3> buffers_;

        std::atomic<int> front_index_;
        int back_index_;
        int spare_index_;
    };
}