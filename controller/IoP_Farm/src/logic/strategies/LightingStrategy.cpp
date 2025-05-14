#include "logic/strategies/LightingStrategy.h"
#include <Stamp.h>
#include "config/constants.h"

namespace farm::logic::strategies
{
    LightingStrategy::LightingStrategy(
        std::shared_ptr<log::ILogger> logger,
        std::shared_ptr<actuators::IActuator> growLight)
        : IActuatorStrategy(logger, growLight)
    {
        updateFromConfig();
    }
    
    void LightingStrategy::updateFromConfig()
    {
        bool hasAllKeys = true;

        if (configManager->hasKey(config::ConfigType::System, config::strategies::lighting::CONFIG_KEY_LIGHT_ON))
        {
            lightOnTime = configManager->getValue<String>(config::ConfigType::System, config::strategies::lighting::CONFIG_KEY_LIGHT_ON);
            logger->log(log::Level::Debug, 
                      "%sЗагружено время включения: %s", 
                      config::strategies::logging::PREFIX_LIGHTING,
                      lightOnTime.c_str());
        }
        else
        {
            hasAllKeys = false;
        }
        
        if (configManager->hasKey(config::ConfigType::System, config::strategies::lighting::CONFIG_KEY_LIGHT_OFF))
        {
            lightOffTime = configManager->getValue<String>(config::ConfigType::System, config::strategies::lighting::CONFIG_KEY_LIGHT_OFF);
            logger->log(log::Level::Debug, 
                      "%sЗагружено время выключения: %s", 
                      config::strategies::logging::PREFIX_LIGHTING,
                      lightOffTime.c_str());
        }
        else
        {
            hasAllKeys = false;
        }

        if (!hasAllKeys)
        {
            logger->log(log::Level::Error, 
                      "%sНе удалось загрузить все настройки: нет ключей в конфигурации",
                      config::strategies::logging::PREFIX_LIGHTING);
        }
        
    }
    
    bool LightingStrategy::applyStrategy()
    {
        int lightOnHour   = std::stoi(lightOnTime.substring(0, 2).c_str());
        int lightOnMinute = std::stoi(lightOnTime.substring(3, 5).c_str());
        
        int lightOffHour   = std::stoi(lightOffTime.substring(0, 2).c_str());
        int lightOffMinute = std::stoi(lightOffTime.substring(3, 5).c_str());
        
        // Создаем Datime для времени включения
        Datime onTime(NTP.getUnix()); // Текущий день
        onTime.hour = lightOnHour;
        onTime.minute = lightOnMinute;
        onTime.second = 0;
        
        // Создаем Datime для времени выключения
        Datime offTime(NTP.getUnix()); // Текущий день
        offTime.hour = lightOffHour;
        offTime.minute = lightOffMinute;
        offTime.second = 0;
        
        // Планируем ежедневное включение
        uint64_t onTaskId = schedulerManager->schedulePeriodicAt(
            onTime,
            config::strategies::lighting::DEFAULT_LIGHT_CYCLE_PERIOD, // Период в секундах (1 день)
            [this]() { this->turnOnLight(); }
        );
        
        // Планируем ежедневное выключение
        uint64_t offTaskId = schedulerManager->schedulePeriodicAt(
            offTime,
            config::strategies::lighting::DEFAULT_LIGHT_CYCLE_PERIOD, // Период в секундах (1 день)
            [this]() { this->turnOffLight(); }
        );

        if (onTaskId == 0 || offTaskId == 0)
        {
            logger->log(log::Level::Error, 
                      "%sНе удалось запланировать включение или выключение фитоленты", 
                      config::strategies::logging::PREFIX_LIGHTING);
            return false;
        }

        scheduledTaskIds.push_back(onTaskId);
        scheduledTaskIds.push_back(offTaskId);
        
        logger->log(log::Level::Farm, 
                  "%sЗапланировано включение фитоленты в %s и выключение в %s ежедневно", 
                  config::strategies::logging::PREFIX_LIGHTING,
                  lightOnTime.c_str(), lightOffTime.c_str());
        logger->log(log::Level::Farm, 
                  "%sTaskOnID: #%llu, TaskOffID: #%llu", 
                  config::strategies::logging::PREFIX_LIGHTING,
                  onTaskId, offTaskId);
        
        // Проверяем, нужно ли сейчас включить фитоленту
        Datime currentTime(NTP.getUnix());
        int currentHour = currentTime.hour;
        int currentMinute = currentTime.minute;
        
        int currentTimeMinutes = currentHour * 60 + currentMinute;
        int onTimeMinutes = lightOnHour * 60 + lightOnMinute;
        int offTimeMinutes = lightOffHour * 60 + lightOffMinute;
        
        // Если текущее время находится между временем включения и выключения - включаем фитоленту
        if ((onTimeMinutes < offTimeMinutes && 
             currentTimeMinutes >= onTimeMinutes && 
             currentTimeMinutes < offTimeMinutes) ||
            (onTimeMinutes > offTimeMinutes && 
             (currentTimeMinutes >= onTimeMinutes || 
              currentTimeMinutes < offTimeMinutes)))
        {
            logger->log(log::Level::Debug, 
                      "%sНачальное состояние: включение фитоленты (текущее время: %02d:%02d)", 
                      config::strategies::logging::PREFIX_LIGHTING,
                      currentHour, currentMinute);
            actuator->turnOn();
        }
        else 
        {
            logger->log(log::Level::Debug, 
                      "%sНачальное состояние: выключение фитоленты (текущее время: %02d:%02d)", 
                      config::strategies::logging::PREFIX_LIGHTING,
                      currentHour, currentMinute);
            actuator->turnOff();
        }
        
        return true;
    }
    
    void LightingStrategy::turnOnLight()
    {
        logger->log(log::Level::Farm, "%sВключение фитоленты по расписанию", config::strategies::logging::PREFIX_LIGHTING);
        actuator->turnOn();
    }
    
    void LightingStrategy::turnOffLight()
    {
        logger->log(log::Level::Farm, "%sВыключение фитоленты по расписанию", config::strategies::logging::PREFIX_LIGHTING);
        actuator->turnOff();
    }
    
    void LightingStrategy::forceTurnOn()
    {
        logger->log(log::Level::Farm, "%sПринудительное включение фитоленты", config::strategies::logging::PREFIX_LIGHTING);
        
        // Проверяем текущее состояние устройства
        if (actuator->getState())
        {
            logger->log(log::Level::Warning, 
                      "%sПопытка включить фитоленту, но она уже включена", 
                      config::strategies::logging::PREFIX_LIGHTING);
            return;
        }
        
        actuator->turnOn();
        
        logger->log(log::Level::Info, 
                  "%sФитолента принудительно включена. Обычное расписание будет восстановлено при следующем запланированном событии", 
                  config::strategies::logging::PREFIX_LIGHTING);
    }
    
    void LightingStrategy::forceTurnOff()
    {
        logger->log(log::Level::Farm, "%sПринудительное выключение фитоленты", config::strategies::logging::PREFIX_LIGHTING);
        
        // Проверяем текущее состояние устройства
        if (!actuator->getState())
        {
            logger->log(log::Level::Warning, 
                      "%sПопытка выключить фитоленту, но она уже выключена", 
                      config::strategies::logging::PREFIX_LIGHTING);
            return;
        }
        
        actuator->turnOff();
        
        logger->log(log::Level::Info, 
                  "%sФитолента принудительно выключена. Обычное расписание будет восстановлено при следующем запланированном событии", 
                  config::strategies::logging::PREFIX_LIGHTING);
    }
} 