#pragma once

#include "actuators/IActuator.h"
#include "config/constants.h"

namespace farm::actuators
{
    class PumpR385 : public IActuator
    {
    private:
        // Пин для вращения вперед
        uint8_t forwardPin;
        
        // Пин для вращения назад
        uint8_t backwardPin;
        
        // Текущее направление работы (true - вперед, false - назад)
        bool isForwardDirection;
        
    public:
        PumpR385(std::shared_ptr<log::ILogger> logger, uint8_t forwardPin, uint8_t backwardPin);
   
        ~PumpR385();
        
        bool initialize() override;
        
        // Включить насос (по умолчанию в прямом направлении)
        bool turnOn() override;
        
        // Включить насос в указанном направлении
        bool turnOn(bool forwardDirection);
        
        bool turnOff() override;
        
        bool getDirection() const;
        
        bool setDirection(bool forwardDirection);
    };
} 