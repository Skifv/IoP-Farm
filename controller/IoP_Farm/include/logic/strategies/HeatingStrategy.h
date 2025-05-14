#pragma once

#include "IActuatorStrategy.h"
#include "actuators/IActuator.h"
#include "sensors/sensors_manager.h"

namespace farm::logic::strategies
{
    class HeatingStrategy : public IActuatorStrategy
    {
    private:
        // Менеджер датчиков для проверки температуры
        std::shared_ptr<sensors::SensorsManager> sensorsManager;
        
        // Параметры стратегии
        float targetTemperature;       // Целевая температура в °C
        float hysteresis;              // Гистерезис температуры в °C
        int   checkIntervalSeconds;    // Интервал проверки температуры в секундах
        
    public:
        HeatingStrategy(std::shared_ptr<log::ILogger> logger, 
                       std::shared_ptr<actuators::IActuator> heatLamp);

        bool applyStrategy() override;
        
        void updateFromConfig() override;
        
        // Проверить и регулировать температуру, вызывается планировщиком
        void controlTemperature();
        
        void forceTurnOn();  
        void forceTurnOff();
    };
} 