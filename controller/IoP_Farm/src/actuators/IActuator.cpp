#include "actuators/IActuator.h"

namespace farm::actuators
{
    using namespace farm::log;
    using namespace farm::config;
    
    IActuator::IActuator()
        : isOn(false),
          initialized(false),
          lastUpdateTime(0),
          configManager(ConfigManager::getInstance()) {}
    
    void IActuator::setActuatorName(const String& name)
    {
        actuatorName = name;
    }
    
    void IActuator::setActuatorType(const String& type)
    {
        actuatorType = type;
    }
    
    bool IActuator::toggle()
    {
        isOn ? turnOff() : turnOn();
    }
    
    bool IActuator::getState() const
    {
        return isOn;
    }
    
    String IActuator::getActuatorName() const
    {
        return actuatorName;
    }
    
    String IActuator::getActuatorType() const
    {
        return actuatorType;
    }

    bool farm::actuators::IActuator::isInitialized() const
    {
        return initialized;
    }
}
