#pragma once

#include <Arduino.h>
#include <memory>
#include "utils/logger_factory.h"
#include "config/config_manager.h"
#include "config/constants.h"

namespace farm::sensors
{
    using namespace farm::log;
    using namespace farm::config;
    using namespace farm::config::sensors;

    // Абстрактный класс датчика
    class ISensor
    {
    protected:
        String sensorName;
        String measurementType;
        String unit;
        
        float lastMeasurement;
        
        std::shared_ptr<log::ILogger> logger;
        
        std::shared_ptr<config::ConfigManager> configManager;
        
        void setSensorName(const String& name);
        void setMeasurementType(const String& type);
        void setUnit(const String& unit);
        
    public:
        // Флаги для определения поведения датчика
        bool shouldBeRead;
        bool shouldBeSaved;
        
        bool initialized;
    public:
        ISensor();

        virtual ~ISensor() = default;

        virtual bool initialize() = 0;
        
        // Считать значение с датчика и записать в lastMeasurement, вернуть его
        virtual float read() = 0;
        
        // Записать значение в оперативную память
        virtual bool saveMeasurement();
        
        float getLastMeasurement() const;
        
        String getSensorName() const;
        
        String getMeasurementType() const;
        
        String getUnit() const;
        
        
    };
} 

