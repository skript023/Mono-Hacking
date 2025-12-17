#include "sigscanner_binding.hpp"

#include "memory/module.hpp"
#include "memory/pattern.hpp"
#include "memory/pattern_batch.hpp"

namespace js::sig
{
	static std::vector<js_scan_entry> g_entries;
	static std::queue<scan_event> g_events;
	static std::mutex g_event_mutex;
	static qjs::Context* g_ctx = nullptr;

	static void scan_cb(memory::handle ptr, void* user)
	{
		size_t index = reinterpret_cast<size_t>(user);

		std::lock_guard<std::mutex> lock(g_event_mutex);
		g_events.push({index,
		    ptr.as<uint64_t>()});
	}
	static void dispatch_js_events()
	{
		if (!g_ctx)
			return;

		std::queue<scan_event> local;

		{
			std::lock_guard<std::mutex> lock(g_event_mutex);
			std::swap(local, g_events);
		}

		JSContext* ctx = g_ctx->ctx;

		while (!local.empty())
		{
			const auto& ev = local.front();
			const auto& e = g_entries[ev.index];

			if (JS_IsFunction(ctx, e.fn))
			{
				JSValue arg = JS_NewFloat64(ctx, ev.address);

				JS_Call(
				    ctx,
				    e.fn,
				    JS_UNDEFINED,
				    1,
				    &arg);

				JS_FreeValue(ctx, arg);
			}

			local.pop();
		}
	}
	static void bind_main_batch(qjs::Context& ctx, memory::pattern_batch& batch)
	{
		g_ctx = &ctx;

		auto global = ctx.global();

		auto pattern_batch = ctx.newObject();

		pattern_batch.add("add",
		    [&](std::string name, std::string sig, qjs::Value cb) {
			    if (!cb.isFunction())
				    throw std::runtime_error("callback must be function");

			    js_scan_entry e;
			    e.name = std::move(name);
			    e.sig = std::move(sig);
			    e.fn = JS_DupValue(ctx.ctx, cb.v);

			    size_t index = g_entries.size();
			    g_entries.push_back(e);

			    batch.add(e.name, memory::pattern(e.sig), [index](memory::handle h) {
				    std::lock_guard<std::mutex> lock(g_event_mutex);
				    g_events.push({index,
				        h.as<uint64_t>()});
			    });
		});


		pattern_batch.add("run", [&](std::string name) {
			if (name.empty())
				batch.run(memory::module(nullptr));
			else
				batch.run(memory::module(name));
		});

		pattern_batch.add("dispatch", [&]() {
			    dispatch_js_events();
		});

		global["main_batch"] = pattern_batch;
	}
	void bind(qjs::Context& ctx)
	{
		static memory::pattern_batch main("js");
		bind_main_batch(ctx, main);
	}
}