#include "utils/ota_manager.h"

namespace farm::net
{
    // Инициализация статической переменной (паттерн Singleton)
    std::shared_ptr<OTAManager> OTAManager::instance = nullptr;
    
    OTAManager::OTAManager(std::shared_ptr<farm::log::ILogger> logger)
        : logger(logger), initialized(false), lastWifiState(false)
    {
        if (!logger) {
            this->logger = farm::log::LoggerFactory::createSerialLogger();
        }
    }
    
    // Получение экземпляра (паттерн Singleton)
    std::shared_ptr<OTAManager> OTAManager::getInstance(std::shared_ptr<farm::log::ILogger> logger)
    {
        if (!instance) {
            instance = std::shared_ptr<OTAManager>(new OTAManager(logger));
        }
        return instance;
    }
    
    bool OTAManager::initialize()
    {
        if (!WiFi.isConnected())
        {
            logger->log(farm::log::Level::Warning, "[ArduinoOTA] Невозможно инициализировать OTA: WiFi не подключен");
            return false;
        }
        
        lastWifiState = true;

        ArduinoOTA.setHostname(wifi::DEFAULT_HOSTNAME);
        
        // Обработчики ArduinoOTA
        ArduinoOTA.onStart([this]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";
            logger->log(farm::log::Level::Warning, "[ArduinoOTA] начало обновления %s", type.c_str());
        });
        
        ArduinoOTA.onEnd([this]() {
            Serial.println("[ArduinoOTA] завершение обновления");
        });
        
        ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
            Serial.printf("[ArduinoOTA] прогресс %u%%\n", (progress / (total / 100)));
        });
        
        ArduinoOTA.onError([this](ota_error_t error) {
            if (error == OTA_AUTH_ERROR) 
                Serial.printf("[ArduinoOTA] ошибка авторизации\n");
            else if (error == OTA_BEGIN_ERROR) 
                Serial.printf("[ArduinoOTA] ошибка начала обновления\n");
            else if (error == OTA_CONNECT_ERROR) 
                Serial.printf("[ArduinoOTA] ошибка соединения\n");
            else if (error == OTA_RECEIVE_ERROR) 
                Serial.printf("[ArduinoOTA] ошибка получения данных\n");
            else if (error == OTA_END_ERROR) 
                Serial.printf("[ArduinoOTA] ошибка завершения\n");
        });
        
        // Запуск самого OTA
        ArduinoOTA.begin();

        setPassword(wifi::DEFAULT_AP_PASSWORD);
        logger->log(farm::log::Level::Farm, "[ArduinoOTA] ArduinoOTA настроен");
        
        initialized = true;
        return true;
    }
    
    void OTAManager::setHostname(const String& name)
    {
        ArduinoOTA.setHostname(name.c_str());
    }
    
    void OTAManager::setPassword(const String& password)
    {
        ArduinoOTA.setPassword(password.c_str());
    }
    
    // Обработка OTA-запросов, вызывается из main.cpp в loop()
    void OTAManager::handle()
    {
        bool currentWifiState = WiFi.isConnected();
        
        // Проверяем, произошло ли переподключение WiFi
        if (currentWifiState && !lastWifiState) 
        {   
            // Если уже был инициализирован ранее, нужно сбросить флаг для реинициализации
            if (initialized) 
            {
                initialized = false;
            }
            
            initialize();
        }
        else if (!currentWifiState && initialized) 
        {
            // WiFi отключился, отмечаем OTA как не инициализированный
            logger->log(farm::log::Level::Warning, "[ArduinoOTA] WiFi отключен, OTA недоступен");
            lastWifiState = false;
            initialized = false;
        }
        // Если OTA не инициализирован и WiFi подключен, инициализируем
        else if (!initialized && currentWifiState)
        {
            logger->log(farm::log::Level::Info, "[ArduinoOTA] WiFi подключен, настройка OTA");
            initialize();
        }
        
        // Обработка OTA запросов происходит только если WiFi подключен
        if (currentWifiState) 
        {
            ArduinoOTA.handle();
        }
    }
    
    bool OTAManager::isInitialized() const
    {
        return initialized;
    }
}