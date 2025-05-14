#include "logic/strategies/HeatingStrategy.h"
#include "config/constants.h"

namespace farm::logic::strategies
{
    using namespace config::strategies;

    HeatingStrategy::HeatingStrategy(
        std::shared_ptr<log::ILogger> logger,
        std::shared_ptr<actuators::IActuator> heatLamp)
        : IActuatorStrategy   (logger, heatLamp),
          hysteresis          (heating::DEFAULT_HYSTERESIS),
          checkIntervalSeconds(heating::DEFAULT_CHECK_INTERVAL),
          sensorsManager      (sensors::SensorsManager::getInstance())
    {
        updateFromConfig();
    }
    
    void HeatingStrategy::updateFromConfig()
    {
        if (configManager->hasKey(config::ConfigType::System, config::strategies::heating::CONFIG_KEY_TARGET_TEMP))
        {
            targetTemperature = configManager->getValue<float>(config::ConfigType::System, config::strategies::heating::CONFIG_KEY_TARGET_TEMP);
            logger->log(log::Level::Debug, 
                      "%sЗагружена целевая температура: %.1f°C", 
                      config::strategies::logging::PREFIX_HEATING,
                      targetTemperature);
        }
        else
        {
            logger->log(log::Level::Error, 
                      "%sНе удалось загрузить целевую температуру: нет ключа в конфигурации",
                      config::strategies::logging::PREFIX_HEATING);
        }
    }
    
    bool HeatingStrategy::applyStrategy()
    {
        // Планируем периодическую проверку температуры
        uint64_t taskId = schedulerManager->schedulePeriodicAfter(
            checkIntervalSeconds, 
            checkIntervalSeconds, 
            [this]() { this->controlTemperature(); }
        );

        if (taskId == 0)
        {
            logger->log(log::Level::Error, 
                      "%sНе удалось запланировать проверку температуры", 
                      config::strategies::logging::PREFIX_HEATING);
            return false;
        }
        
        scheduledTaskIds.push_back(taskId);
        
        logger->log(log::Level::Farm, 
                  "%sЗапланирована проверка температуры каждые %d с., целевая температура: %.1f°C", 
                  config::strategies::logging::PREFIX_HEATING, checkIntervalSeconds, targetTemperature);    
        logger->log(log::Level::Farm, 
                  "%sTaskID: #%llu", config::strategies::logging::PREFIX_HEATING, taskId);    

        return true;
    }
    
    void HeatingStrategy::controlTemperature()
    {        
        // Проверяем наличие датчика температуры
        if (!sensorsManager->hasSensor(config::sensors::names::DHT22_TEMPERATURE)) 
        {
            logger->log(log::Level::Warning, 
                      "%sДатчик температуры не найден для контроля за температурой", 
                      config::strategies::logging::PREFIX_HEATING);
            return;
        }
        
        float temperature = sensorsManager->getLastMeasurement(config::sensors::names::DHT22_TEMPERATURE);

        if (temperature == sensors::calibration::SENSOR_ERROR_VALUE || 
            temperature == sensors::calibration::NO_DATA)
        {
            logger->log(log::Level::Warning, 
                      "%sНевозможно проверить температуру: нет доступных показаний. Лампа нагрева отключена.",
                      config::strategies::logging::PREFIX_HEATING);

            // Отключаем лампу нагрева на всякий случай
            if (actuator->getState())
            {
                actuator->turnOff();
            }

            return;
        }
        
        // Если температура ниже целевой - гистерезис - включаем лампу
        if (temperature < (targetTemperature - hysteresis) && !actuator->getState())
        {
            logger->log(log::Level::Info, 
                      "%sВключение лампы нагрева (текущая температура: %.1f°C, целевая: %.1f°C)", 
                      config::strategies::logging::PREFIX_HEATING,
                      temperature, targetTemperature);
            actuator->turnOn();
        }
        // Если температура выше целевой + гистерезис - выключаем лампу
        else if (temperature > (targetTemperature + hysteresis) && actuator->getState())
        {
            logger->log(log::Level::Info, 
                      "%sВыключение лампы нагрева (текущая температура: %.1f°C, целевая: %.1f°C)", 
                      config::strategies::logging::PREFIX_HEATING,
                      temperature, targetTemperature);
            actuator->turnOff();
        }
        else
        {
            // Для отладки
            logger->log(log::Level::Debug, 
                      "%sТекущая температура: %.1f°C, целевая: %.1f°C, лампа: %s", 
                      config::strategies::logging::PREFIX_HEATING,
                      temperature, targetTemperature, 
                      actuator->getState() ? "включена" : "выключена");
        }
    }
    
    void HeatingStrategy::forceTurnOn()
    {
        logger->log(log::Level::Farm, "%sПринудительное включение нагревателя", config::strategies::logging::PREFIX_HEATING);
        
        // Проверяем, включен ли уже нагреватель
        if (actuator->getState())
        {
            logger->log(log::Level::Warning, 
                      "%sПопытка включить нагреватель, но он уже включен", 
                      config::strategies::logging::PREFIX_HEATING);
            return;
        }
        
        actuator->turnOn();
        
        logger->log(log::Level::Info, 
                  "%sНагреватель принудительно включен. Автоматическая регулировка температуры будет восстановлена при следующей проверке", 
                  config::strategies::logging::PREFIX_HEATING);
    }
    
    void HeatingStrategy::forceTurnOff()
    {
        logger->log(log::Level::Farm, "%sПринудительное выключение нагревателя", config::strategies::logging::PREFIX_HEATING);
        
        // Проверяем, выключен ли уже нагреватель
        if (!actuator->getState())
        {
            logger->log(log::Level::Warning, 
                      "%sПопытка выключить нагреватель, но он уже выключен", 
                      config::strategies::logging::PREFIX_HEATING);
            return;
        }
        
        actuator->turnOff();
        
        logger->log(log::Level::Info, 
                  "%sНагреватель принудительно выключен. Автоматическая регулировка температуры будет восстановлена при следующей проверке", 
                  config::strategies::logging::PREFIX_HEATING);
    }
} 