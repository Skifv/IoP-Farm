#include "actuators/GrowLight.h"

namespace farm::actuators
{
    using namespace farm::log;
    using namespace farm::config::actuators;
    using namespace farm::config::sensors::calibration; 
    
    GrowLight::GrowLight(std::shared_ptr<log::ILogger> logger, int8_t controlPin)
        : controlPin(controlPin)
    {
        this->logger = logger;
        
        setActuatorName(names::GROWLIGHT);
        setActuatorType(types::LIGHT);
    }
    
    GrowLight::~GrowLight()
    {
        if (isOn)
        {
            turnOff();
        }
    }
    
    bool GrowLight::initialize()
    {
        if (controlPin == UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                       "[%s] Не задан пин для фитоленты", 
                       getActuatorName().c_str());
            return false;
        }
        
        pinMode(controlPin, OUTPUT);
        
        digitalWrite(controlPin, LOW);
        
        initialized = true;
        isOn = false;
        
        return true;
    }
    
    bool GrowLight::turnOn()
    {
        if (!initialized)
        {
            logger->log(Level::Error, 
                       "[%s] Попытка включить фитоленту до инициализации", 
                       getActuatorName().c_str());
            return false;
        }

        if (isOn)
        {
            logger->log(Level::Warning, 
                       "[%s] Попытка включить фитоленту, но она уже включена", 
                       getActuatorName().c_str());
            return true;
        }
        
        logger->log(Level::Farm, 
                   "[%s] Включение фитоленты", 
                   getActuatorName().c_str());
            
        digitalWrite(controlPin, HIGH);
        
        isOn = true;
        lastUpdateTime = millis();
        
        return true;
    }
    
    bool GrowLight::turnOff()
    {
        if (!initialized)
        {
            logger->log(Level::Error, 
                       "[%s] Попытка выключить фитоленту до инициализации", 
                       getActuatorName().c_str());
            return false;
        }

        if (!isOn)
        {
            logger->log(Level::Warning, 
                       "[%s] Попытка выключить фитоленту, но она уже выключена", 
                       getActuatorName().c_str());
            return true;
        }
        
        logger->log(Level::Farm, 
                   "[%s] Выключение фитоленты", 
                   getActuatorName().c_str());
        
        digitalWrite(controlPin, LOW);
        
        isOn = false;
        lastUpdateTime = millis();
        
        return true;
    }
} 