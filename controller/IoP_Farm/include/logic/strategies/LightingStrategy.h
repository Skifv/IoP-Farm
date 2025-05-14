#pragma once

#include "IActuatorStrategy.h"
#include "actuators/IActuator.h"

namespace farm::logic::strategies
{
    class LightingStrategy : public IActuatorStrategy
    {
    private:
        // Параметры стратегии
        String lightOnTime;        // Время включения фитоленты в формате "ЧЧ:ММ"
        String lightOffTime;       // Время выключения фитоленты в формате "ЧЧ:ММ"
        
    public:
        LightingStrategy(std::shared_ptr<log::ILogger> logger, 
                        std::shared_ptr<actuators::IActuator> growLight);
        
        bool applyStrategy() override;
        
        void updateFromConfig() override;
        
        void turnOnLight();
        void turnOffLight();
        
        void forceTurnOn();
        void forceTurnOff();
    };
} 