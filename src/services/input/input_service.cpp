#include "input_service.hpp"
#include "task_mgr.hpp"

namespace big
{
	void input_service::show(std::string title)
	{
		strncpy_s(m_title, title.c_str(), _TRUNCATE);
		g_settings.window.input = true;
		g_settings.window.mouse_active = true;
	}
	void input_service::show(std::string const& title, input_callback_t cb)
	{
		strncpy_s(m_title, title.c_str(), _TRUNCATE);
		g_settings.window.input = true;
		g_settings.window.mouse_active = true;

		callback(cb);
	}
	void input_service::hide()
	{
		strcpy(m_title, "Input Here");
		memset(m_input, 0, _ARRAYSIZE(m_input));

		g_settings.window.input = false;
		g_settings.window.mouse_active = false;
	}
	void input_service::callback(input_callback_t callback)
	{
		if (callback)
		{
			m_callback.push(std::move(callback));
		}
	}
	void input_service::execute()
	{
		if (!m_callback.empty())
		{
			input_callback_t job = std::move(m_callback.top());
			m_callback.pop();

			std::string text(m_input);
			this->hide();
			async_run([job = std::move(job), text = std::move(text)] {
				std::invoke(job, text);
			});
		}
	}
	void input_service::cancel()
	{
		m_callback = {};
		this->hide();
	}
}
