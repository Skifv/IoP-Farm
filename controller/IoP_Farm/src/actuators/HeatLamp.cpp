#include "actuators/HeatLamp.h"

namespace farm::actuators
{
    using namespace farm::log;
    using namespace farm::config::actuators;
    using namespace farm::config::sensors::calibration; 
               
    HeatLamp::HeatLamp(std::shared_ptr<log::ILogger> logger, int8_t controlPin)
        : controlPin(controlPin)
    {
        this->logger = logger;
        
        setActuatorName(names::HEATLAMP);
        setActuatorType(types::HEATER);
    }
    
    HeatLamp::~HeatLamp()
    {
        if (isOn)
        {
            turnOff();
        }
    }
    
    bool HeatLamp::initialize()
    {
        if (controlPin == UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                       "[%s] Не задан пин для лампы нагрева", 
                       getActuatorName().c_str());
            return false;
        }
        
        pinMode(controlPin, OUTPUT);
        
        digitalWrite(controlPin, LOW);
        
        initialized = true;
        isOn = false;
        
        return true;
    }
    
    bool HeatLamp::turnOn()
    {
        if (!initialized)
        {
            logger->log(Level::Error, 
                       "[%s] Попытка включить лампу нагрева до инициализации", 
                       getActuatorName().c_str());
            return false;
        }

        if (isOn)
        {
            logger->log(Level::Warning, 
                       "[%s] Попытка включить лампу нагрева, но она уже включена", 
                       getActuatorName().c_str());
            return true;
        }
        
        logger->log(Level::Farm, 
                   "[%s] Включение лампы нагрева", 
                   getActuatorName().c_str());
        
        digitalWrite(controlPin, HIGH);
        
        isOn = true;
        lastUpdateTime = millis();
        
        return true;
    }
    
    bool HeatLamp::turnOff()
    {
        if (!initialized)
        {
            logger->log(Level::Error, 
                       "[%s] Попытка выключить лампу нагрева до инициализации", 
                       getActuatorName().c_str());
            return false;
        }

        if (!isOn)
        {
            logger->log(Level::Warning, 
                       "[%s] Попытка выключить лампу нагрева, но она уже выключена", 
                       getActuatorName().c_str());
            return true;
        }
        
        logger->log(Level::Farm, 
                   "[%s] Выключение лампы нагрева", 
                   getActuatorName().c_str());
        
        digitalWrite(controlPin, LOW);
        
        isOn = false;
        lastUpdateTime = millis();
        
        return true;
    }
} 