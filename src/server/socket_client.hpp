#pragma once

#define ASIO_STANDALONE

#include "client_ws.hpp"
#include "client_wss.hpp"
#include "thread_pool.hpp"
#include "enums/connection_status.hpp"
#include "hooking.hpp"
#include "services/notification/notification_service.hpp"
#include <nlohmann/json.hpp>

namespace big
{
    class socket_client
    {
    public:
        using MessageCallback = std::function<void(std::string const&)>;
        using KickCallback = std::function<void(std::string const&)>;

        socket_client(std::string const& endpoint, bool is_ssl = false)
            : m_endpoint(endpoint), m_is_ssl(is_ssl), m_connection_status(eConnectionStatus::DISCONNECT)
        {
            connect();
        }

        ~socket_client()
        {
            disconnect();
        }

        bool connect()
        {
            if (m_connection_status == eConnectionStatus::CONNECTED)
            {
                LOG(INFO) << "[SocketClient] Already connected.";
                return true;
            }

            disconnect();
            m_connection_status = eConnectionStatus::CONNECTING;

            LOG(INFO) << "[SocketClient] Connecting to " << (m_is_ssl ? "wss://" : "ws://") << m_endpoint;

            if (m_is_ssl)
            {
                m_client_wss = std::make_shared<SimpleWeb::SocketClient<SimpleWeb::WSS>>(m_endpoint, false);
                m_client_wss->config.header.emplace("User-Agent", "Valheim/1.0");
                setup_wss_handlers();
                g_thread_pool->push([this] {
                    try {
                        m_client_wss->start();
                    } catch (const std::exception& ex) {
                        LOG(WARNING) << "[SocketClient] WSS error: " << ex.what();
                        m_connection_status = eConnectionStatus::DISCONNECT;
                    }
                });
            }
            else
            {
                m_client_ws = std::make_shared<SimpleWeb::SocketClient<SimpleWeb::WS>>(m_endpoint);
                m_client_ws->config.header.emplace("User-Agent", "Valheim/1.0");
                setup_ws_handlers();
                g_thread_pool->push([this] {
                    try {
                        m_client_ws->start();
                    } catch (const std::exception& ex) {
                        LOG(WARNING) << "[SocketClient] WS error: " << ex.what();
                        m_connection_status = eConnectionStatus::DISCONNECT;
                    }
                });
            }

            return true;
        }

        void disconnect()
        {
            if (m_client_wss)
            {
                m_client_wss->stop();
                m_active_conn_wss.reset();
                m_client_wss.reset();
            }
            if (m_client_ws)
            {
                m_client_ws->stop();
                m_active_conn_ws.reset();
                m_client_ws.reset();
            }
            m_connection_status = eConnectionStatus::DISCONNECT;
        }

        bool reconnect()
        {
            disconnect();
            return connect();
        }

        void send_message(const std::string& message)
        {
            if (m_connection_status != eConnectionStatus::CONNECTED)
            {
                LOG(WARNING) << "[SocketClient] Cannot send message, not connected.";
                return;
            }

            if (m_is_ssl && m_active_conn_wss)
            {
                m_active_conn_wss->send(message);
            }
            else if (!m_is_ssl && m_active_conn_ws)
            {
                m_active_conn_ws->send(message);
            }
        }

        void on_message_received(MessageCallback callback)
        {
            std::lock_guard lock(m_mutex);
            m_message_callback = std::move(callback);
        }

        void on_force_logout(KickCallback callback)
        {
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
        void handle_incoming_text(const std::string& text)
        {
            // Check for FORCE_LOGOUT event from server (Single active device enforcement)
            if (text.find("FORCE_LOGOUT") != std::string::npos)
            {
                std::string reason = "Account logged in from another device. Mod deactivated.";
                try
                {
                    auto j = nlohmann::json::parse(text, nullptr, false);
                    if (!j.is_discarded() && j.contains("message") && j["message"].is_string())
                    {
                        reason = j["message"].get<std::string>();
                    }
                }
                catch (...) {}

                LOG(FATAL) << "[SocketClient] *** FORCE_LOGOUT: " << reason << " ***";
                notification::info("Security Alert", reason);
                
                if (m_kick_callback)
                {
                    m_kick_callback(reason);
                }

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
                LOG(INFO) << "[SocketClient] WSS connected successfully to " << m_endpoint;
            };

            m_client_wss->on_message = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection>, std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::InMessage> in_msg) {
                handle_incoming_text(in_msg->string());
            };

            m_client_wss->on_close = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection>, int status, const std::string& reason) {
                LOG(WARNING) << "[SocketClient] WSS closed, Status: " << status << " Reason: " << reason;
                m_connection_status = eConnectionStatus::DISCONNECT;
            };

            m_client_wss->on_error = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection>, const SimpleWeb::error_code& ec) {
                LOG(WARNING) << "[SocketClient] WSS error: [" << ec.value() << "] " << ec.message();
                m_connection_status = eConnectionStatus::DISCONNECT;
            };
        }

        void setup_ws_handlers()
        {
            m_client_ws->on_open = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection> connection) {
                m_connection_status = eConnectionStatus::CONNECTED;
                m_active_conn_ws = connection;
                LOG(INFO) << "[SocketClient] WS connected successfully to " << m_endpoint;
            };

            m_client_ws->on_message = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection>, std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::InMessage> in_msg) {
                handle_incoming_text(in_msg->string());
            };

            m_client_ws->on_close = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection>, int status, const std::string& reason) {
                LOG(WARNING) << "[SocketClient] WS closed, Status: " << status << " Reason: " << reason;
                m_connection_status = eConnectionStatus::DISCONNECT;
            };

            m_client_ws->on_error = [this](std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection>, const SimpleWeb::error_code& ec) {
                LOG(WARNING) << "[SocketClient] WS error: [" << ec.value() << "] " << ec.message();
                m_connection_status = eConnectionStatus::DISCONNECT;
            };
        }

    private:
        std::string m_endpoint;
        bool m_is_ssl{false};
        std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>> m_client_wss;
        std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>> m_client_ws;
        std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WSS>::Connection> m_active_conn_wss;
        std::shared_ptr<SimpleWeb::SocketClient<SimpleWeb::WS>::Connection> m_active_conn_ws;
        MessageCallback m_message_callback;
        KickCallback m_kick_callback;
        std::recursive_mutex m_mutex;
        eConnectionStatus m_connection_status;
    };
}

