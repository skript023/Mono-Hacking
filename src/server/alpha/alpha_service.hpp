#pragma once
#include "data/hardware.hpp"
#include "server/socket_client.hpp"
#include "server/auth_client.hpp"
#include "server/interfaces/message.hpp"
#include "notification/notification_service.hpp"

namespace big
{
	class alpha_service
	{
	public:
		explicit alpha_service()
		{
			init_connection();
		}

		~alpha_service() noexcept
		{
			if (m_alpha_gateway)
			{
				m_alpha_gateway->disconnect();
				m_alpha_gateway.reset();
			}
		}

		void init_connection()
		{
			if (m_alpha_gateway) m_alpha_gateway->disconnect();
			m_auth_invalidated = false;
			m_auth_info = auth_client::authenticate();
			if (!m_auth_info.success)
			{
				LOG(WARNING) << "[AlphaService] Authentication failed: " << m_auth_info.error_message;
				notification::info("Ellohim Server", "Authentication required. Please launch from Loader.");
				return;
			}

			m_alpha_gateway = std::make_shared<socket_client>(m_auth_info.ws_endpoint, m_auth_info.is_ssl);

			m_alpha_gateway->on_message_received([this](const std::string& message) {
				if (message == "pong")
				{
					LOG(INFO) << "[AlphaService] Heartbeat ACK (pong) received.";
				}
				else if (message.find("Connected to server successfully") != std::string::npos)
				{
					LOG(INFO) << "[AlphaService] Server WebSocket handshake success!";
					notification::info("Ellohim Server", "Connected to server successfully");
				}
				else if (message.find("Unauthorized WebSocket connection") != std::string::npos)
				{
					LOG(WARNING) << "[AlphaService] Unauthorized connection. Invalidating session token for re-auth...";
					m_auth_invalidated = true;
				}
				else if (!message.empty())
				{
					LOG(INFO) << "[AlphaService] Received: " << message;
				}
			});

			m_alpha_gateway->on_force_logout([](const std::string& reason) {
				LOG(FATAL) << "[AlphaService] Logged out by server: " << reason;
			});
		}

		void ping()
		{
			if (m_alpha_gateway && m_alpha_gateway->is_connected())
			{
				// Send plain "ping" string as expected by Ellohim-Server auth.cc
				m_alpha_gateway->send_message("ping");
			}
		}

		void send_hardware()
		{
		}

		bool auto_reconnect()
		{
			if (m_auth_invalidated.exchange(false))
			{
				m_auth_info.token.clear();
				m_auth_info.success = false;
			}
			if (!m_auth_info.success || !m_alpha_gateway || !m_alpha_gateway->is_connected())
			{
				if (!m_auth_info.success || m_auth_info.token.empty())
				{
					init_connection();
					return m_alpha_gateway && m_alpha_gateway->is_connected();
				}

				if (m_alpha_gateway && m_alpha_gateway->reconnect())
				{
					return true;
				}

				init_connection();
				return m_alpha_gateway && m_alpha_gateway->is_connected();
			}

			return false;
		}

		bool is_connected() const
		{
			return m_alpha_gateway && m_alpha_gateway->is_connected();
		}

	private:
		std::atomic<bool> m_auth_invalidated{false};
		ServerAuthInfo m_auth_info;
		std::shared_ptr<socket_client> m_alpha_gateway;
		Gateway m_event;
	};
}

