#pragma once
#include "../quickjspp.hpp"

namespace js::trampoline
{
	void bind(qjs::Context& context);
	void shutdown();
} // namespace js::trampoline
