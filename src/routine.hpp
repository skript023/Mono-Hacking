#pragma once
#include "common.hpp"
#include <coroutine>
#include <chrono>
#include <optional>

namespace big
{
	// Forward declarations
	struct routine_yield;

	struct routine
	{
		struct promise_type
		{
			routine get_return_object();
			std::suspend_always initial_suspend();
			std::suspend_always final_suspend() noexcept;
			void return_void();
			void unhandled_exception();

			std::optional<std::chrono::high_resolution_clock::time_point> m_wake_time;

			// await_transform harus inline karena mengembalikan tipe struct lokal
			auto await_transform(routine_yield yield_info);
		};

		std::coroutine_handle<promise_type> m_handle;
	};

	struct routine_yield
	{
		std::optional<std::chrono::high_resolution_clock::duration> time;
		explicit routine_yield(std::optional<std::chrono::high_resolution_clock::duration> t = std::nullopt);
	};

	// Implementasi await_transform
	inline auto routine::promise_type::await_transform(routine_yield yield_info)
	{
		struct awaiter
		{
			routine_yield info;
			bool await_ready() { return false; }
			void await_suspend(std::coroutine_handle<promise_type> h)
			{
				if (info.time.has_value())
					h.promise().m_wake_time = std::chrono::high_resolution_clock::now() + info.time.value();
				else
					h.promise().m_wake_time = std::nullopt;
			}
			void await_resume() {}
		};
		return awaiter{yield_info};
	}
}
