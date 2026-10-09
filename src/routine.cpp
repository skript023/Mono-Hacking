#include "routine.hpp"
#include "logger/logger.hpp" // Untuk LOG(WARNING)

namespace big
{
	routine_yield::routine_yield(std::optional<std::chrono::high_resolution_clock::duration> t) : time(t) 
	{
	}

	routine routine::promise_type::get_return_object() 
	{
		return {std::coroutine_handle<promise_type>::from_promise(*this)};
	}

	std::suspend_always routine::promise_type::initial_suspend() 
	{ 
		return {}; 
	}
	
	std::suspend_always routine::promise_type::final_suspend() noexcept 
	{ 
		return {}; 
	}
	
	void routine::promise_type::return_void() 
	{
	}

	void routine::promise_type::unhandled_exception() 
	{
		LOG(WARNING) << "Unhandled exception inside a routine!";
	}
}
