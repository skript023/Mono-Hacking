#pragma once

#define ASIO_STANDALONE

#include <windows.h>
#include <wincrypt.h>
#include "client_ws.hpp"
#include "client_wss.hpp"
#include <thread>
#include <atomic>
#include "enums/connection_status.hpp"
#include "hooking.hpp"
#include <astra/host/notification.hpp>
#include <nlohmann/json.hpp>

namespace big
{
	class verified_wss_client : public SimpleWeb::SocketClient<SimpleWeb::WSS>
	{
	public:
		explicit verified_wss_client(const std::string& endpoint) :
		    SimpleWeb::SocketClient<SimpleWeb::WSS>(endpoint, true)
		{
			auto store  = SSL_CTX_get_cert_store(context.native_handle());
			auto system = CertOpenSystemStoreW(0, L"ROOT");
			if (!system)
			{
				return;
			}
			PCCERT_CONTEXT certificate = nullptr;
			while (store && (certificate = CertEnumCertificatesInStore(system, certificate)))
			{
				const unsigned char* data = certificate->pbCertEncoded;
				if (auto x509 = d2i_X509(nullptr, &data, static_cast<long>(certificate->cbCertEncoded)))
				{
					X509_STORE_add_cert(store, x509);
					X509_free(x509);
				}
			}
			CertCloseStore(system, 0);
		}
	};

	class socket_client
	{
	public:
		using MessageCallback = std::function<void(std::string const&)>;
		using KickCallback = std::function<void(std::string const&)>;

		socket_client(std::string const& endpoint, bool is_ssl = false, std::string token = {}, std::string hwid = {}, std::string device_name = {}) :
		    m_endpoint(endpoint),
		    m_is_ssl(is_ssl),
		    m_token(std::move(token)),
		    m_hwid(std::move(hwid)),
		    m_device_name(std::move(device_name)),
		    m_connection_status(eConnectionStatus::DISCONNECT)
		{
			connect();
		}

		~socket_client()
		{
			disconnect();
		}

		bool connect()
		{
			std::lock_guard lifecycle_lock(m_lifecycle_mutex);
			if (m_connection_status == eConnectionStatus::CONNECTED)
			{
				LOG(INFO) << "[SocketClient] Already connected.";
				return true;
			}

			disconnect();
			m_shutdown_requested = false;
			m_connection_status = eConnectionStatus::CONNECTING;

			LOG(INFO) << "[SocketClient] Connecting to " << (m_is_ssl ? "wss://" : "ws://") << log_endpoint();

			if (m_is_ssl)
			{
				m_client_wss = std::make_shared<verified_wss_client>(m_endpoint);
				m_client_wss->config.header.emplace("User-Agent", "Valheim/1.0");
				setup_wss_handlers();
				start_client(m_client_wss);
			}
			else
			{
				m_client_ws = std::make_shared<SimpleWeb::SocketClient<SimpleWeb::WS>>(m_endpoint);
				configure_headers(m_client_ws->config.header);
				setup_ws_handlers();
				start_client(m_client_ws);
			}

			return true;
		}

		void disconnect()
		{
			m_shutdown_requested = true;
			std::lock_guard lifecycle_lock(m_lifecycle_mutex);
			if (m_client_wss)
				m_client_wss->stop();
			if (m_client_ws)
				m_client_ws->stop();
			if (m_io_context)
				m_io_context->stop();
			// All callbacks capturing this must finish before client/service destruction.
			if (m_io_thread.joinable())
				m_io_thread.join();
			m_active_conn_wss.reset();
			m_active_conn_ws.reset();
			m_client_wss.reset();
			m_client_ws.reset();
			m_io_context.reset();
			m_connection_status = eConnectionStatus::DISCONNECT;
		}

		bool reconnect()
		{
			disconnect();
			return connect();
		}

		void send_message(const std::string& message)
		{
			std::lock_guard lifecycle_lock(m_lifecycle_mutex);
			if (!m_io_context || !is_connected())
				return;
			// Connection pointers and SSL writes belong to the I/O thread.
			asio::post(*m_io_context, [this, message] {
				if (!is_connected())
					return;
				if (m_is_ssl && m_active_conn_wss)
					m_active_conn_wss->send(message);
				else if (!m_is_ssl && m_active_conn_ws)
					m_active_conn_ws->send(message);
			});
		}

		void on_message_received(MessageCallback callback)
		{
			std::lock_guard lock(m_mutex);
			m_message_callback = std::move(callback);
		}

		void on_force_logout(KickCallback callback)
		{
			std::lock_guard lock(m_mutex);
			m_kick_callback = std::move(callback);
		}

		bool is_connected() const
		{
			return m_connection_status == eConnectionStatus::CONNECTED;
		}

		eConnectionStatus get_connection_status() const
		{
			return m_connection_status;
		}

	private:
		std::string_view log_endpoint() const
		{
			return std::string_view(m_endpoint).substr(0, m_endpoint.find('?'));
		}

		template<typename Client>
		void start_client(const std::shared_ptr<Client>& client)
		{
			m_io_context = std::make_shared<asio::io_context>();
			client->io_service = m_io_context;
			// With an external context start() only schedules connection setup.
			// Complete it before disconnect() can stop the context; start() restarts
			// stopped contexts, so racing start against stop can hang a join.
			client->start();
			m_io_thread = std::thread([this, context = m_io_context] {
				try
				{
					auto work = asio::make_work_guard(*context);
					context->run();
				}
				catch (const std::exception& ex)
				{
					if (!m_shutdown_requested)
						LOG(WARNING) << "[SocketClient] I/O error: " << ex.what();
					m_connection_status = eConnectionStatus::DISCONNECT;
				}
				OPENSSL_thread_stop();
			});
		}

		void handle_incoming_text(const std::string& text)
		{
			// Check for FORCE_LOGOUT event from server (Single active device enforcement)
			const auto j       = nlohmann::json::parse(text, nullptr, false);
			bool force_logout  = text == "FORCE_LOGOUT";
			std::string reason = "Account logged in from another device. Mod deactivated.";
			if (j.is_object())
			{
				for (const char* field : {"event", "type", "action"})
				{
					if (j.contains(field) && j[field].is_string())
					{
						auto value = j[field].get<std::string>();
						std::ranges::transform(value, value.begin(), [](unsigned char c) {
							return static_cast<char>(std::toupper(c));
						});
						force_logout |= value == "FORCE_LOGOUT";
					}
				}
				for (const char* field : {"message", "reason"})
				{
					if (force_logout && j.contains(field) && j[field].is_string())
					{
						reason = j[field].get<std::string>();
						break;
					}
				}
			}
			if (force_logout)
			{
				LOG(FATAL) << "[SocketClient] *** FORCE_LOGOUT: " << reason << " ***";
				notification::info("Security Alert", reason);

				KickCallback kick;
				{
					std::lock_guard lock(m_mutex);
					kick = m_kick_callback;
				}
				if (kick)
					kick(reason);

				// Immediate safety shutdown: disable hooks & stop running

				g_running = false;

				return;
			}

			MessageCallback cb;
			{
				std::lock_guard lock(m_mutex);
				cb = m_message_callback;
			}
			if (cb)
			{
				cb(text);
			}
		}

		void setup_wss_handlers()
		{
			m_client_wss->on_open = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection> connection) {
				m_connection_status = eConnectionStatus::CONNECTED;
				m_active_conn_wss = connection;
				LOG(INFO) << "[SocketClient] WSS connected successfully to " << log_endpoint();
			};

			m_client_wss->on_message = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection>, std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::InMessage> in_msg) {
				handle_incoming_text(in_msg->string());
			};

			m_client_wss->on_close = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection>, int status, const std::string& reason) {
				if (!m_shutdown_requested)
					LOG(WARNING) << "[SocketClient] WSS closed, Status: " << status << " Reason: " << reason;
				m_connection_status = eConnectionStatus::DISCONNECT;
			};

			m_client_wss->on_error = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection>, const SimpleWeb::error_code& ec) {
				if (!m_shutdown_requested || ec.value() != 995)
					LOG(WARNING) << "[SocketClient] WSS error: [" << ec.value() << "] " << ec.message();
				m_connection_status = eConnectionStatus::DISCONNECT;
			};
		}

		void setup_ws_handlers()
		{
			m_client_ws->on_open = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection> connection) {
				m_connection_status = eConnectionStatus::CONNECTED;
				m_active_conn_ws = connection;
				LOG(INFO) << "[SocketClient] WS connected successfully to " << log_endpoint();
			};

			m_client_ws->on_message = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection>, std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::InMessage> in_msg) {
				handle_incoming_text(in_msg->string());
			};

			m_client_ws->on_close = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection>, int status, const std::string& reason) {
				if (!m_shutdown_requested)
					LOG(WARNING) << "[SocketClient] WS closed, Status: " << status << " Reason: " << reason;
				m_connection_status = eConnectionStatus::DISCONNECT;
			};

			m_client_ws->on_error = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection>, const SimpleWeb::error_code& ec) {
				if (!m_shutdown_requested || ec.value() != 995)
					LOG(WARNING) << "[SocketClient] WS error: [" << ec.value() << "] " << ec.message();
				m_connection_status = eConnectionStatus::DISCONNECT;
			};
		}

	private:
		std::string m_endpoint;
		bool m_is_ssl{false};
		std::string m_token;
		std::string m_hwid;
		std::string m_device_name;
		std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>> m_client_wss;
		std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>> m_client_ws;
		std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection> m_active_conn_wss;
		std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection> m_active_conn_ws;
		MessageCallback m_message_callback;
		KickCallback m_kick_callback;
		std::recursive_mutex m_mutex;
		std::atomic<eConnectionStatus> m_connection_status;
		std::atomic_bool m_shutdown_requested{false};
		std::recursive_mutex m_lifecycle_mutex;
		std::shared_ptr<asio::io_context> m_io_context;
		std::thread m_io_thread;
	};
}
