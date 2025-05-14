#pragma once

#include <map>
#include <memory>
#include "sensors/ISensor.h"
#include "config/config_manager.h"
#include "utils/logger_factory.h"
#include "network/mqtt_manager.h"
#include "sensors/DHT22_Temperature.h"
#include "sensors/DHT22_Humidity.h"
#include "sensors/DS18B20.h"
#include "sensors/FC28.h"
#include "sensors/HCSR04.h"
#include "sensors/KY018.h"
#include "sensors/YFS401.h"

namespace farm::sensors
{
    using namespace farm::log;
    using namespace farm::config;
    
    class SensorsManager
    {
    private:
        // Приватный конструктор (паттерн Синглтон)
        explicit SensorsManager(std::shared_ptr<log::ILogger> logger = nullptr);     
        static std::shared_ptr<SensorsManager> instance;
        
        std::shared_ptr<config::ConfigManager> configManager;
        
        std::shared_ptr<log::ILogger> logger;

        std::shared_ptr<net::MQTTManager> mqttManager;

        unsigned long lastReadTime;
        
        // Интервал чтения датчиков (мс)
        // Инициализируется константой, но может быть изменено (закладка на будущее)
        unsigned long readInterval;

        // Флаг включения/выключения ВСЕХ датчиков
        bool enabled;
        
    public:
        // Публичный map датчиков для удобства использования [имя датчика, указатель на датчик]
        std::map<String, std::shared_ptr<ISensor>> sensors;
        
        static std::shared_ptr<SensorsManager> getInstance(std::shared_ptr<log::ILogger> logger = nullptr);
        
        SensorsManager(const SensorsManager&) = delete;
        SensorsManager& operator=(const SensorsManager&) = delete;
            
        ~SensorsManager();
        
        bool initialize();
        
        bool readAllSensors();
        
        bool saveAllMeasurements();
        
        // Метод для периодического выполнения в main.cpp
        bool loop();
        
        float getLastMeasurement(const String& sensorName);

        // Данные после считывания заносятся лишь в переменную класса датчика, не в /data
        float sensorRead(const String& sensorName);

        // Сохранение данных в /data
        bool sensorSave(const String& sensorName);

        float sensorReadAndSave(const String& sensorName);

        String getSensorType(const String& sensorName);

        String getSensorUnit(const String& sensorName);

        void setReadInterval(unsigned long interval);
        
        bool addSensor(const String& sensorName, std::shared_ptr<ISensor> sensor);
        
        bool removeSensor(const String& sensorName);
        
        std::shared_ptr<ISensor> getSensor(const String& sensorName);
        
        bool hasSensor(const String& sensorName) const;
        
        size_t getSensorCount() const;
        
        void clearSensors();

        // Включить/выключить ВСЕ датчики
        void sensorsEnable(bool enable);
        bool isEnabled() const;
     
        // Обобщенный шаблонный метод для создания и инициализации датчика 
        // (реализация должна быть в заголовочном файле)
        template<typename SensorType, typename... Args>
        bool createAndInitSensor(Args&&... args)
        {
            auto sensor = std::make_shared<SensorType>(logger, std::forward<Args>(args)...);

            if (sensor->initialize()) 
            {
                addSensor(sensor->getSensorName(), sensor);
                return true;
            } 
            else 
            {
                logger->log(farm::log::Level::Error, "[Sensors] Не удалось инициализировать датчик %s (%s)", 
                        sensor->getSensorName(), sensor->getMeasurementType().c_str());
                return false;
            }
        }

    };
} 