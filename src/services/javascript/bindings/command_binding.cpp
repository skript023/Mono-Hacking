#include "command_binding.hpp"

#include "commands/looped_command.hpp"
#include "../js_lock.hpp"

namespace js::command
{
	using namespace big;

	static std::vector<std::shared_ptr<class bool_command_binding>> g_bool_commands;
	static std::vector<std::shared_ptr<class looped_command_binding>> g_looped_commands;

	static void invoke_callback(const std::optional<qjs::Value>& callback)
	{
		std::lock_guard lock(g_js_mutex);
		if (!callback || !callback->isFunction())
		{
			return;
		}
		try
		{
			callback->call({});
		}
		catch (const qjs::exception&)
		{
			auto error = qjs::Context::get(callback->ctx).getException();
			LOG(WARNING) << "[JS Command Error] " << error.as<std::string>();
		}
	}

	class bool_command_binding final : public bool_command
	{
	public:
		bool_command_binding(std::string name, std::string label, std::string description, bool default_state) :
		    bool_command(std::move(name), std::move(label), std::move(description), default_state)
		{
		}

		bool get_state_value()
		{
			return bool_command::get_state();
		}
		void set_state_value(bool state)
		{
			bool_command::set_state(state);
		}
		void on_enable_callback(qjs::Value callback)
		{
			m_on_enable = std::move(callback);
		}
		void on_disable_callback(qjs::Value callback)
		{
			m_on_disable = std::move(callback);
		}

	protected:
		void on_enable() override
		{
			invoke_callback(m_on_enable);
		}
		void on_disable() override
		{
			invoke_callback(m_on_disable);
		}

	private:
		std::optional<qjs::Value> m_on_enable;
		std::optional<qjs::Value> m_on_disable;
	};

	class looped_command_binding final : public looped_command
	{
	public:
		looped_command_binding(std::string name, std::string label, std::string description) :
		    looped_command(std::move(name), std::move(label), std::move(description))
		{
		}

		bool get_state_value()
		{
			return bool_command::get_state();
		}
		void set_state_value(bool state)
		{
			bool_command::set_state(state);
		}
		void on_tick_callback(qjs::Value callback)
		{
			m_on_tick = std::move(callback);
		}
		void on_disable_callback(qjs::Value callback)
		{
			m_on_disable = std::move(callback);
		}

	protected:
		void on_tick() override
		{
			invoke_callback(m_on_tick);
		}
		void on_disable() override
		{
			invoke_callback(m_on_disable);
		}

	private:
		std::optional<qjs::Value> m_on_tick;
		std::optional<qjs::Value> m_on_disable;
	};

	class command_factory final
	{
	public:
		static std::shared_ptr<bool_command_binding> create_bool(std::string name, std::string label, std::string description, bool default_state)
		{
			auto command = std::make_shared<bool_command_binding>(std::move(name), std::move(label), std::move(description), default_state);
			g_bool_commands.push_back(command);
			return command;
		}

		static std::shared_ptr<looped_command_binding> create_looped(std::string name, std::string label, std::string description)
		{
			auto command = std::make_shared<looped_command_binding>(std::move(name), std::move(label), std::move(description));
			g_looped_commands.push_back(command);
			return command;
		}
	};

	void bind(qjs::Context& context)
	{
		auto& module = context.addModule("command_types");
		module.class_<bool_command_binding>("BoolCommand")
		    .fun<&bool_command_binding::get_state_value>("get_state")
		    .fun<&bool_command_binding::set_state_value>("set_state")
		    .fun<&bool_command_binding::on_enable_callback>("on_enable")
		    .fun<&bool_command_binding::on_disable_callback>("on_disable");
		module.class_<looped_command_binding>("LoopedCommand")
		    .fun<&looped_command_binding::get_state_value>("get_state")
		    .fun<&looped_command_binding::set_state_value>("set_state")
		    .fun<&looped_command_binding::on_tick_callback>("on_tick")
		    .fun<&looped_command_binding::on_disable_callback>("on_disable");
		module.class_<command_factory>("CommandFactory")
		    .constructor<>("CommandFactory")
		    .static_fun<&command_factory::create_bool>("create_bool")
		    .static_fun<&command_factory::create_looped>("create_looped");
	}

	void shutdown()
	{
		g_looped_commands.clear();
		g_bool_commands.clear();
	}
}
