#include "common.hpp"
#include "thread_pool.hpp"
#include "hooking/detour_base.hpp"

void callback()
{
}
struct test_hook : big::detour_base
{
	test_hook(bool fail = false) :
	    detour_base("test")
	{
		if (fail)
			throw std::runtime_error("hook failure");
	}
	bool enable() override
	{
		return true;
	}
	bool disable() override
	{
		return true;
	}
	void* get_original_ptr() override
	{
		return nullptr;
	}
};
int main()
{
	std::set_terminate([] {
		std::_Exit(42);
	});
	try
	{
		big::thread_pool pool;
		throw std::runtime_error("startup failure");
	}
	catch (const std::runtime_error&)
	{
	}
	if (big::g_thread_pool)
		return 1;
	{
		big::thread_pool pool;
		std::promise<void> completed;
		auto ready = completed.get_future();
		pool.push([&] {
			completed.set_value();
		});
		if (ready.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
			return 2;
		pool.destroy();
		pool.destroy();
	}
	try
	{
		test_hook hook(true);
	}
	catch (const std::runtime_error&)
	{
	}
	if (!big::detour_base::hooks().empty())
		return 3;
	{
		test_hook hook;
		big::detour_base::add<callback>(&hook);
	}
	if (!big::detour_base::hooks().empty() || big::detour_base::get<callback, test_hook>())
		return 4;
	return 0;
}
