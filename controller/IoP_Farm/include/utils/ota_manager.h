#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "utils/logger_factory.h"
#include "config/constants.h" 
#include <memory>

namespace farm::net
{
    using namespace farm::config; 

    // Класс для управления OTA обновлениями
    class OTAManager 
    {
    private:
        // Приватный конструктор (паттерн Синглтон)
        explicit OTAManager(std::shared_ptr<farm::log::ILogger> logger = nullptr);       
        static std::shared_ptr<OTAManager> instance;
        
        std::shared_ptr<farm::log::ILogger> logger;
        
        bool initialized;
        
        bool lastWifiState;
        
    public:
        static std::shared_ptr<OTAManager> getInstance(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        
        OTAManager(const OTAManager&) = delete;
        OTAManager& operator=(const OTAManager&) = delete;
        
        ~OTAManager() = default;
        
        bool initialize();
        
        void setHostname(const String& name);
        
        void setPassword(const String& password);
        
        // Обработка OTA-запросов, вызывать в цикле loop()
        void handle();
        
        bool isInitialized() const;
    };
}