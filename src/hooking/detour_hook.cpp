#include "detour_hook.hpp"

#include "memory/handle.hpp"

#include <MinHook.h>

namespace big
{
	detour_hook::detour_hook(const std::string_view name, void* target, void* detour) :
	    detour_base(name),
	    m_target(target),
	    m_detour(detour)
	{
		LOG(INFO) << "Creating hook: " << m_name << " at " << m_target;
		Logger::FlushQueue();
		if (!m_target)
		{
			throw std::runtime_error(std::format("Failed to create hook '{}': target function pointer is null", m_name));
		}

		fix_hook_address();

		if (auto status = MH_CreateHook(m_target, m_detour, &m_original); status != MH_OK)
		{
			throw std::runtime_error(std::format("Failed to create hook '{}' at 0x{:X} (error: {})", m_name, reinterpret_cast<std::uintptr_t>(m_target), MH_StatusToString(status)));
		}
	}

	detour_hook::~detour_hook() noexcept
	{
		if (m_target)
		{
			MH_RemoveHook(m_target);
		}

		LOG(INFO) << "Removed hook '" << m_name << "'.";
	}

	bool detour_hook::enable()
	{
		if (m_enabled)
			return true;
		const auto status = MH_QueueEnableHook(m_target);
		if (status != MH_OK)
			throw std::runtime_error(std::format("Failed to enable hook '{}' ({})", m_name, MH_StatusToString(status)));
		m_enabled = true;
		return true;
	}
	bool detour_hook::disable()
	{
		if (!m_enabled)
			return true;
		const auto status = MH_QueueDisableHook(m_target);
		if (status != MH_OK)
			return false;
		m_enabled = false;
		return true;
	}

	void detour_hook::enable_immediately() const
	{
		if (auto status = MH_EnableHook(m_target); status != MH_OK)
		{
			throw std::runtime_error(std::format("Failed to enable hook 0x{:X} ({})", reinterpret_cast<std::uintptr_t>(m_target), MH_StatusToString(status)));
		}
	}

	void detour_hook::disable_immediately() const
	{
		if (auto status = MH_DisableHook(m_target); status != MH_OK)
		{
			LOG(FATAL) << "Failed to disable hook '" << m_name << "'.";
		}
	}

	void* detour_hook::get_original_ptr()
	{
		return m_original;
	}

	DWORD exp_handler(PEXCEPTION_POINTERS exp, const std::string_view name)
	{
		return exp->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
	}

	void detour_hook::fix_hook_address()
	{
		__try
		{
			auto ptr = memory::handle(m_target);

			while (ptr.as<std::uint8_t&>() == 0xE9)
			{
				ptr = ptr.add(1).rip();
			}

			m_target = ptr.as<void*>();
		}
		__except (exp_handler(GetExceptionInformation(), m_name))
		{
			[this]() {
				throw std::runtime_error(std::format("Failed to fix hook address for '{}'", m_name));
			}();
		}
	}
}
