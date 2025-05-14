#pragma once

#include <Arduino.h>
#include <memory>
#include "actuators/IActuator.h"
#include "utils/logger_factory.h"
#include "config/constants.h"

namespace farm::actuators
{
    class HeatLamp : public IActuator
    {
    private:
        int8_t controlPin;
        
    public:
        HeatLamp(std::shared_ptr<log::ILogger> logger, int8_t controlPin);
        virtual ~HeatLamp() override;
        
        bool initialize() override;
        
        bool turnOn() override;
        bool turnOff() override;
    };
} 