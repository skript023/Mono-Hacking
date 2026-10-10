#pragma once
#include "common.hpp"
#include <coroutine>
#include <chrono>
#include <optional>
#include <utility>

namespace big
{
	// 1. Awaiter untuk Jeda Waktu (Yield)
	struct task_yield
	{
		std::optional<std::chrono::high_resolution_clock::duration> m_time;
		explicit task_yield(std::optional<std::chrono::high_resolution_clock::duration> t = std::nullopt) :
		    m_time(t)
		{
		}
	};

	// 2. Deklarasi Awal Template
	template<typename T = void>
	struct task;

	// 3. Spesialisasi untuk task<void> (Root Task / Void Task)
	template<>
	struct task<void>
	{
		struct promise_type
		{
			std::coroutine_handle<> m_continuation; // Untuk menyimpan siapa yang menunggu task ini
			std::optional<std::chrono::high_resolution_clock::time_point> m_wake_time;

			task get_return_object()
			{
				return {std::coroutine_handle<promise_type>::from_promise(*this)};
			}
			std::suspend_always initial_suspend()
			{
				return {};
			}

			// Transfer kontrol kembali ke caller saat selesai
			auto final_suspend() noexcept
			{
				struct final_awaiter
				{
					bool await_ready() noexcept
					{
						return false;
					}
					std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
					{
						if (h.promise().m_continuation)
							return h.promise().m_continuation;
						return std::noop_coroutine();
					}
					void await_resume() noexcept
					{
					}
				};
				return final_awaiter{};
			}

			void return_void()
			{
			}
			void unhandled_exception()
			{
			}

			// Support untuk co_await task_yield(...)
			auto await_transform(task_yield yield_info)
			{
				struct awaiter
				{
					task_yield info;
					bool await_ready()
					{
						return false;
					}
					void await_suspend(std::coroutine_handle<promise_type> h)
					{
						if (info.m_time.has_value())
							h.promise().m_wake_time = std::chrono::high_resolution_clock::now() + info.m_time.value();
						else
							h.promise().m_wake_time = std::nullopt;
					}
					void await_resume()
					{
					}
				};
				return awaiter{yield_info};
			}
		};

		std::coroutine_handle<promise_type> m_handle;

		// --- FUNGSI AGAR BISA DI-AWAIT OLEH TASK LAIN ---
		bool await_ready()
		{
			return !m_handle || m_handle.done();
		}
		std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller)
		{
			m_handle.promise().m_continuation = caller;
			return m_handle;
		}
		void await_resume()
		{
			m_handle.destroy();
		}
	};

	// 4. Implementasi Generic untuk task<T> (Dengan Return Value)
	template<typename T>
	struct task
	{
		struct promise_type
		{
			std::optional<T> m_value;
			std::coroutine_handle<> m_continuation;
			std::optional<std::chrono::high_resolution_clock::time_point> m_wake_time;

			task get_return_object()
			{
				return {std::coroutine_handle<promise_type>::from_promise(*this)};
			}
			std::suspend_always initial_suspend()
			{
				return {};
			}

			auto final_suspend() noexcept
			{
				struct final_awaiter
				{
					bool await_ready() noexcept
					{
						return false;
					}
					std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
					{
						if (h.promise().m_continuation)
							return h.promise().m_continuation;
						return std::noop_coroutine();
					}
					void await_resume() noexcept
					{
					}
				};
				return final_awaiter{};
			}

			void return_value(T val)
			{
				m_value = std::move(val);
			}
			void unhandled_exception()
			{
			}

			// Support untuk co_await task_yield(...)
			auto await_transform(task_yield yield_info)
			{
				struct awaiter
				{
					task_yield info;
					bool await_ready()
					{
						return false;
					}
					void await_suspend(std::coroutine_handle<promise_type> h)
					{
						if (info.m_time.has_value())
							h.promise().m_wake_time = std::chrono::high_resolution_clock::now() + info.m_time.value();
						else
							h.promise().m_wake_time = std::nullopt;
					}
					void await_resume()
					{
					}
				};
				return awaiter{yield_info};
			}
		};

		std::coroutine_handle<promise_type> m_handle;

		// --- FUNGSI AGAR BISA DI-AWAIT OLEH TASK LAIN ---
		bool await_ready()
		{
			return !m_handle || m_handle.done();
		}
		std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller)
		{
			m_handle.promise().m_continuation = caller;
			return m_handle; // Lompat ke sub-task ini
		}
		T await_resume()
		{
			// Kembalikan nilai dan hancurkan memori sub-task
			T result = std::move(m_handle.promise().m_value.value());
			m_handle.destroy();
			return result;
		}
	};

	namespace this_task
	{
		inline task_yield wait(std::optional<std::chrono::high_resolution_clock::duration> t = std::nullopt)
		{
			return task_yield(t);
		}
	}
}
