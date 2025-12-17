#pragma once
#include "../quickjspp.hpp"

namespace js::sig
{
	struct js_scan_entry
	{
		std::string name;
		std::string sig;
		JSValue fn;
	};

	struct scan_event
	{
		size_t index;
		uint64_t address;
	};

	void bind(qjs::Context& ctx);
}