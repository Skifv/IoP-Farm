#pragma once

#include <Arduino.h>
#include <memory>
#include "utils/logger_factory.h"
#include "config/config_manager.h"
#include "config/constants.h"

namespace farm::actuators
{
    using namespace farm::log;
    using namespace farm::config;

    // Абстрактный класс исполнительного устройства
    class IActuator
    {
    protected:
        String actuatorName;
        String actuatorType;
        
        bool isOn;
        
        std::shared_ptr<log::ILogger> logger; // Передаем из actuatorManager через конструктор реализации
        
        std::shared_ptr<config::ConfigManager> configManager;
        
        // Время последнего обновления состояния (миллисекунды)
        unsigned long lastUpdateTime;
        
        void setActuatorName(const String& name);
        void setActuatorType(const String& type);

        bool initialized;
    public:
        IActuator();

        virtual ~IActuator() = default;

        virtual bool initialize() = 0;
        bool isInitialized() const;
        
        virtual bool turnOn() = 0;
        virtual bool turnOff() = 0;
        
        virtual bool toggle();
        
        bool getState() const;
        
        String getActuatorName() const;
        String getActuatorType() const;
    };
} 