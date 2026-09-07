#pragma once
#include "alpha/alpha_service.hpp"
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

namespace big
{
    class server_module;
    inline server_module* g_server_module{};

    class server_module
    {
    public:
        explicit server_module()
        {
            m_alpha_service = std::make_shared<alpha_service>();

            g_server_module = this;

            // Dedicated heartbeat thread running every 15s independently of game state
            m_heartbeat_thread = std::thread([this]() {
                while (g_running)
                {
                    for (int i = 0; i < 15 && g_running; ++i)
                    {
                        std::this_thread::sleep_for(1s);
                    }
                    if (!g_running) break;
                    run();
                }
            });
        }

        ~server_module() noexcept
        {
            g_server_module->shutdown();
        }

        alpha_service* get_alpha() { return m_alpha_service.get(); }

        void run()
        {
            if (m_alpha_service)
            {
                if (!m_alpha_service->auto_reconnect())
                {
                    m_alpha_service->ping();
                    m_alpha_service->send_hardware();
                }
            }
        }
    private:
        void shutdown()
        {
            if (m_heartbeat_thread.joinable())
            {
                m_heartbeat_thread.join();
            }

            m_alpha_service.reset();

            g_server_module = nullptr;
        }
    private:
        std::shared_ptr<alpha_service> m_alpha_service;
        std::thread m_heartbeat_thread;
    };
}

