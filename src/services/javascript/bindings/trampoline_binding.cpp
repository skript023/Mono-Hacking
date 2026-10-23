#include "trampoline_binding.hpp"

#include "hooking/detour_hook.hpp"
#include "../js_lock.hpp"

namespace js::trampoline
{
	using namespace big;

	struct AbiContext
	{
		uintptr_t gp[4]{};
		alignas(16) uint8_t xmm[4][16]{};
		uintptr_t entry_stack{};
		void* hook_state{};
		uint32_t argument_count{};
		uint32_t reserved{};
		uintptr_t return_value{};
		uintptr_t* stack_copy{};
		void* original{};
		bool original_called{};
	};

	static_assert(offsetof(AbiContext, gp) == 0x00);
	static_assert(offsetof(AbiContext, xmm) == 0x20);
	static_assert(offsetof(AbiContext, entry_stack) == 0x60);
	static_assert(offsetof(AbiContext, hook_state) == 0x68);
	static_assert(offsetof(AbiContext, argument_count) == 0x70);
	static_assert(offsetof(AbiContext, return_value) == 0x78);
	static_assert(offsetof(AbiContext, stack_copy) == 0x80);

	struct HookState
	{
		qjs::Context* context{};
		std::optional<qjs::Value> callback;
		void* original{};
		uint32_t argument_count{};
		std::atomic<uint32_t> active{};
	};

	struct HookEntry
	{
		std::unique_ptr<detour_hook> hook;
		std::unique_ptr<HookState> state;
		void* entry_stub{};

		~HookEntry()
		{
			if (hook && !hook->disable())
			{
				LOG(WARNING) << "JS hook '" << hook->name() << "' is covered by another program's hook; it stays in place.";
				state->callback.reset();
				(void)hook.release();
				(void)state.release();
				return;
			}
			hook.reset();
			if (entry_stub)
			{
				VirtualFree(entry_stub, 0, MEM_RELEASE);
				entry_stub = nullptr;
			}
		}
	};

	extern "C" uintptr_t __fastcall JsInvokeOriginal(AbiContext* context, void* original, uint32_t argument_count);
	extern "C" uintptr_t __fastcall JsDetourDispatch(AbiContext* raw_context, HookState* state);
	extern "C" void JsDetourEntry();

	class NativeArgument
	{
	public:
		NativeArgument(std::shared_ptr<AbiContext> context, std::shared_ptr<std::vector<uintptr_t>> stack_args, uint32_t index) :
		    m_context(std::move(context)),
		    m_stack_args(std::move(stack_args)),
		    m_index(index)
		{
			if (!m_context || m_index >= m_context->argument_count)
			{
				throw std::out_of_range("native argument index out of range");
			}
		}

		uintptr_t raw() const
		{
			if (m_index < 4)
			{
				return m_context->gp[m_index];
			}
			return (*m_stack_args)[m_index - 4];
		}

		std::string string() const
		{
			const auto value = raw();
			return value ? reinterpret_cast<const char*>(value) : "";
		}

		std::optional<std::string> mono_string() const
		{
			struct MonoStringView
			{
				void* klass;
				void* monitor;
				int32_t length;
				char16_t chars[1];
			};
			auto* value = reinterpret_cast<MonoStringView*>(raw());
			if (!value || value->length < 0 || value->length > 0x2000)
			{
				return std::nullopt;
			}
			const auto* chars = reinterpret_cast<const wchar_t*>(value->chars);
			const int length  = WideCharToMultiByte(CP_UTF8, 0, chars, value->length, nullptr, 0, nullptr, nullptr);
			std::string result(static_cast<size_t>(length), '\0');
			if (length > 0)
			{
				WideCharToMultiByte(CP_UTF8, 0, chars, value->length, result.data(), length, nullptr, nullptr);
			}
			return result;
		}

		double number() const
		{
			return static_cast<double>(raw());
		}
		double float_number() const
		{
			uint32_t bits{};
			if (m_index < 4)
			{
				std::memcpy(&bits, m_context->xmm[m_index], sizeof(bits));
			}
			else
			{
				bits = static_cast<uint32_t>(raw());
			}
			return std::bit_cast<float>(bits);
		}
		double double_number() const
		{
			if (m_index >= 4)
			{
				return std::bit_cast<double>(raw());
			}
			double value{};
			std::memcpy(&value, m_context->xmm[m_index], sizeof(value));
			return value;
		}
		double value_of() const
		{
			return number();
		}

	private:
		std::shared_ptr<AbiContext> m_context;
		std::shared_ptr<std::vector<uintptr_t>> m_stack_args;
		uint32_t m_index;
	};

	class NativeContext
	{
	public:
		NativeContext(std::shared_ptr<AbiContext> context, std::shared_ptr<std::vector<uintptr_t>> stack_args) :
		    m_context(std::move(context)),
		    m_stack_args(std::move(stack_args))
		{
		}

		std::vector<double> args() const
		{
			std::vector<double> result;
			result.reserve(m_context->argument_count);
			for (uint32_t i = 0; i < m_context->argument_count; ++i)
			{
				result.push_back(NativeArgument(m_context, m_stack_args, i).number());
			}
			return result;
		}

		std::shared_ptr<NativeArgument> get_arg(uint32_t index) const
		{
			return std::make_shared<NativeArgument>(m_context, m_stack_args, index);
		}

		void set_arg(uint32_t index, double value) const
		{
			if (index >= m_context->argument_count)
			{
				throw std::out_of_range("native argument index out of range");
			}
			const auto raw = static_cast<uintptr_t>(value);
			if (index < 4)
			{
				m_context->gp[index] = raw;
			}
			else
			{
				(*m_stack_args)[index - 4] = raw;
			}
		}

		void set_float(uint32_t index, double value) const
		{
			if (index >= m_context->argument_count)
			{
				throw std::out_of_range("native argument index out of range");
			}
			const auto bits = std::bit_cast<uint32_t>(static_cast<float>(value));
			if (index < 4)
			{
				std::memcpy(m_context->xmm[index], &bits, sizeof(bits));
			}
			else
			{
				auto& slot = (*m_stack_args)[index - 4];
				slot       = (slot & 0xffffffff00000000ull) | bits;
			}
		}

		void set_double(uint32_t index, double value) const
		{
			if (index >= m_context->argument_count)
			{
				throw std::out_of_range("native argument index out of range");
			}
			if (index < 4)
			{
				std::memcpy(m_context->xmm[index], &value, sizeof(value));
			}
			else
			{
				(*m_stack_args)[index - 4] = std::bit_cast<uint64_t>(value);
			}
		}

		double call_original() const
		{
			if (!m_context->original)
			{
				return 0;
			}
			if (!m_context->original_called)
			{
				m_context->return_value = JsInvokeOriginal(m_context.get(), m_context->original, m_context->argument_count);
				m_context->original_called = true;
			}
			return static_cast<double>(m_context->return_value);
		}

	private:
		std::shared_ptr<AbiContext> m_context;
		std::shared_ptr<std::vector<uintptr_t>> m_stack_args;
	};

	static std::unordered_map<std::string, std::unique_ptr<HookEntry>> g_hooks;
	static std::vector<std::unique_ptr<HookEntry>> g_retired;

	static void prune_retired()
	{
		std::erase_if(g_retired, [](const auto& entry) {
			return entry->state->active == 0;
		});
	}

	static bool hooks_active()
	{
		std::lock_guard lock(big::g_js_mutex);
		const auto active = [](const auto& entry) {
			return entry->state->active > 0;
		};
		return std::ranges::any_of(g_hooks | std::views::values, active) || std::ranges::any_of(g_retired, active);
	}

	static void* make_entry_stub(HookState* state)
	{
		uint8_t code[23]         = {0x49, 0xBA};
		const auto state_address = reinterpret_cast<uint64_t>(state);
		std::memcpy(code + 2, &state_address, sizeof(state_address));
		code[10]                 = 0x49;
		code[11]                 = 0xBB;
		const auto entry_address = reinterpret_cast<uint64_t>(&JsDetourEntry);
		std::memcpy(code + 12, &entry_address, sizeof(entry_address));
		code[20] = 0x41;
		code[21] = 0xFF;
		code[22] = 0xE3;

		auto* memory = VirtualAlloc(nullptr, sizeof(code), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!memory)
		{
			return nullptr;
		}
		std::memcpy(memory, code, sizeof(code));
		FlushInstructionCache(GetCurrentProcess(), memory, sizeof(code));
		return memory;
	}

	class HookBindings
	{
	public:
		static bool add(std::string const& name, double address, uint32_t argument_count, qjs::Value callback)
		{
			prune_retired();
			auto* target = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
			if (!target || !callback.isFunction() || g_hooks.contains(name) || argument_count > 256)
			{
				return false;
			}

			auto entry            = std::make_unique<HookEntry>();
			entry->state          = std::make_unique<HookState>();
			entry->state->context = &qjs::Context::get(callback.ctx);
			entry->state->callback.emplace(std::move(callback));
			entry->state->argument_count = argument_count;
			entry->entry_stub            = make_entry_stub(entry->state.get());
			if (!entry->entry_stub)
			{
				return false;
			}

			try
			{
				entry->hook            = std::make_unique<detour_hook>(name, target, entry->entry_stub);
				entry->state->original = entry->hook->get_original_ptr();
				g_hooks.emplace(name, std::move(entry));
				return true;
			}
			catch (const std::exception& e)
			{
				LOG(WARNING) << "Failed to create detour hook " << name << ": " << e.what();
				return false;
			}
		}

		static void enable(std::string const& name)
		{
			if (auto it = g_hooks.find(name); it != g_hooks.end())
			{
				it->second->hook->enable();
			}
		}

		static void disable(std::string const& name)
		{
			if (auto it = g_hooks.find(name); it != g_hooks.end())
			{
				auto entry = std::move(it->second);
				g_hooks.erase(it);
				if (entry->state->active > 0)
				{
					entry->hook->disable();
					g_retired.push_back(std::move(entry));
				}
			}
			prune_retired();
		}
	};

	extern "C" uintptr_t __fastcall JsDetourDispatch(AbiContext* raw_context, HookState* state)
	{
		++state->active;
		struct active_scope
		{
			HookState* state;
			~active_scope()
			{
				--state->active;
			}
		} scope{state};

		auto context = std::make_shared<AbiContext>();
		std::memcpy(context.get(), raw_context, offsetof(AbiContext, stack_copy));
		context->argument_count = state->argument_count;
		context->original       = state->original;
		auto stack_args         = std::make_shared<std::vector<uintptr_t>>();
		if (state->argument_count > 4)
		{
			stack_args->resize(state->argument_count - 4);
			for (uint32_t i = 0; i < stack_args->size(); ++i)
			{
				(*stack_args)[i] = *reinterpret_cast<const uintptr_t*>(context->entry_stack + 0x28 + i * sizeof(uintptr_t));
			}
		}
		context->stack_copy = stack_args->data();

		{
			std::lock_guard lock(big::g_js_mutex);
			if (state->callback && state->callback->isFunction())
			{
				try
				{
					auto argument = state->context->newValue(std::make_shared<NativeContext>(context, stack_args));
					auto result   = state->callback->call(std::vector<qjs::Value>{std::move(argument)});
					if (result.isInt() || result.isFloat())
					{
						return static_cast<uintptr_t>(result.as<double>());
					}
					if (context->original_called)
					{
						return context->return_value;
					}
				}
				catch (const qjs::exception&)
				{
					auto error = state->context->getException();
					LOG(WARNING) << "[JS Detour Error] " << error.as<std::string>();
				}
			}
		}

		return static_cast<uintptr_t>(NativeContext(context, stack_args).call_original());
	}

	void bind(qjs::Context& context)
	{
		auto& module = context.addModule("detour");
		module.class_<NativeArgument>("NativeArgument")
		    .fun<&NativeArgument::string>("string")
		    .fun<&NativeArgument::mono_string>("monoString")
		    .fun<&NativeArgument::number>("number")
		    .fun<&NativeArgument::float_number>("float")
		    .fun<&NativeArgument::double_number>("double")
		    .fun<&NativeArgument::value_of>("valueOf");
		module.class_<NativeContext>("NativeContext")
		    .fun<&NativeContext::args>("args")
		    .fun<&NativeContext::get_arg>("getArg")
		    .fun<&NativeContext::set_arg>("setArg")
		    .fun<&NativeContext::set_float>("setFloat")
		    .fun<&NativeContext::set_double>("setDouble")
		    .fun<&NativeContext::call_original>("callOriginal");
		module.class_<HookBindings>("Hook")
		    .constructor<>("Hook")
		    .static_fun<&HookBindings::add>("add")
		    .static_fun<&HookBindings::enable>("enable")
		    .static_fun<&HookBindings::disable>("disable");
		module.function("add", &HookBindings::add);
		module.function("enable", &HookBindings::enable);
		module.function("disable", &HookBindings::disable);
	}

	void shutdown()
	{
		{
			std::lock_guard lock(big::g_js_mutex);
			for (auto& entry : g_hooks | std::views::values)
			{
				entry->hook->disable();
			}
		}
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
		while (hooks_active() && std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		std::lock_guard lock(big::g_js_mutex);
		g_hooks.clear();
		g_retired.clear();
	}
}
