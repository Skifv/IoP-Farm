#pragma once

#include <WiFiManager.h>             // https://github.com/tzapu/WiFiManager
#include "utils/logger_factory.h"
#include "config/constants.h"
#include <memory>

namespace farm::net
{
    using namespace farm::config::wifi;
    
    // Обертка над WiFiManager от tzapu
    class MyWiFiManager
    {
    private:
        // Приватный конструктор (паттерн Синглтон)
        explicit MyWiFiManager(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        static std::shared_ptr<MyWiFiManager> instance;
        
        // Экземпляр WiFiManager от tzapu
        WiFiManager wifiManager;
        
        std::shared_ptr<farm::log::ILogger> logger;
        
        // Время последней проверки соединения с WiFi
        unsigned long lastCheckTime = 0;
        
        bool portalActive = false;
        
        uint8_t reconnectAttempts = 0;
        
        unsigned long lastReconnectTime = 0;
        
        String apName     = DEFAULT_AP_NAME;
        String apPassword = DEFAULT_AP_PASSWORD;
        String hostName   = DEFAULT_HOSTNAME;
        
        // Поддержка выставления параметров MQTT в captive portal
        WiFiManagerParameter mqttServerParam;
        WiFiManagerParameter mqttPortParam;
        
        // Колбэк для сохранения параметров MQTT
        void saveParamsCallback();

    public:
        // Получение экземпляра (паттерн Синглтон)
        static std::shared_ptr<MyWiFiManager> getInstance(std::shared_ptr<farm::log::ILogger> logger = nullptr); 
        MyWiFiManager(const MyWiFiManager&) = delete;
        MyWiFiManager& operator=(const MyWiFiManager&) = delete;
        
        ~MyWiFiManager() = default;
        
        void setAccessPointCredentials(const String& name, const String& password = DEFAULT_AP_PASSWORD);
        
        void setHostName(const String& name);

        String getIPAddress() const;
        
        bool checkWifiSaved();
        
        // Инициализация WiFi и попытка первичного подключения
        bool initialize();
        
        bool resetSettings();
        
        // Поддержание WiFi соединения - вызывать в цикле loop()
        void maintainConnection();
        
        bool isConnected() const;
        
        bool isConfigPortalActive();
        
        // Включение/выключение встроенной отладки WiFiManager
        void setDebugOutput(bool enable);
        void setDebugOutput(bool enable, const String& prefix);
    };
}
