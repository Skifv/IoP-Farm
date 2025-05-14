#include "actuators/PumpR385.h"

namespace farm::actuators
{
    using namespace farm::log;
    using namespace farm::config::actuators;
    using namespace farm::config::sensors::calibration; 
    
    PumpR385::PumpR385(std::shared_ptr<log::ILogger> logger, uint8_t forwardPin, uint8_t backwardPin)
        : forwardPin(forwardPin),
          backwardPin(backwardPin),
          isForwardDirection(true)
    {
        this->logger = logger;
        
        setActuatorName(names::PUMP_R385);
        setActuatorType(types::WATER_PUMP);
    }
    
    PumpR385::~PumpR385()
    {
        if (isOn)
        {
            turnOff();
        }
    }
    
    bool PumpR385::initialize()
    {
        if (forwardPin == UNINITIALIZED_PIN || backwardPin == UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                      "[%s] Не заданы пины для насоса", 
                      getActuatorName().c_str());
            return false;
        }
        
        pinMode(forwardPin, OUTPUT);
        pinMode(backwardPin, OUTPUT);
        
        digitalWrite(forwardPin, LOW);
        digitalWrite(backwardPin, LOW);
        
        initialized = true;
        isOn = false;
        
        return true;
    }
    
    // Включить насос по умолчанию
    bool PumpR385::turnOn()
    {
        return turnOn(isForwardDirection);
    }
    
    // Включить насос в указанном направлении
    bool PumpR385::turnOn(bool forwardDirection)
    {
        if (!initialized)
        {
            logger->log(Level::Error, 
                      "[%s] Попытка включить насос до инициализации", 
                      getActuatorName().c_str());
            return false;
        }

        if (isOn)
        {
            logger->log(Level::Warning, 
                      "[%s] Попытка включить насос, но он уже включен", 
                      getActuatorName().c_str());
            return true;
        }
        
        // Запоминаем направление
        isForwardDirection = forwardDirection;
        
        logger->log(Level::Farm, 
                  "[%s] Включение насоса, направление: %s", 
                  getActuatorName().c_str(), 
                  isForwardDirection ? "вперед" : "назад");
        

        digitalWrite(forwardPin, LOW);
        digitalWrite(backwardPin, LOW);
        
        // Включаем нужное направление
        if (isForwardDirection)
        {
            digitalWrite(forwardPin, HIGH);
            digitalWrite(backwardPin, LOW);
        }
        else
        {
            digitalWrite(forwardPin, LOW);
            digitalWrite(backwardPin, HIGH);
        }
        
        isOn = true;
        lastUpdateTime = millis();
        
        return true;
    }
    
    bool PumpR385::turnOff()
    {
        if (!initialized)
        {
            if (logger)
            {
                logger->log(Level::Error, 
                        "[%s] Попытка выключить насос до инициализации", 
                        getActuatorName().c_str());
            }
            return false;
        }

        if (!isOn)
        {
            if (logger)
            {
                logger->log(Level::Warning, 
                        "[%s] Попытка выключить насос, но он уже выключен", 
                        getActuatorName().c_str());
            }
            return true;
        }

        if (logger)
        {
            logger->log(Level::Info, "[%s] Выключение насоса", getActuatorName().c_str());
        }
        
        digitalWrite(forwardPin, LOW);
        digitalWrite(backwardPin, LOW);
        
        isOn = false;
        lastUpdateTime = millis();
        
        return true;
    }
    
    bool PumpR385::getDirection() const
    {
        return isForwardDirection;
    }
    
    // Изменить направление работы
    bool PumpR385::setDirection(bool forwardDirection)
    {
        // Если насос работает и направление меняется, перезапускаем его
        if (isOn && isForwardDirection != forwardDirection)
        {
            turnOff();
            return turnOn(forwardDirection);
        }
        
        // Просто меняем направление, если насос не работает
        isForwardDirection = forwardDirection;
        return true;
    }
} 