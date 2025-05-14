#include "logic/strategies/IrrigationStrategy.h"
#include <Stamp.h>
#include "config/constants.h"

namespace farm::logic::strategies
{
    using namespace farm::log;
    using namespace farm::config::strategies;
    using namespace farm::config::sensors;
    
    IrrigationStrategy::IrrigationStrategy(
        std::shared_ptr<log::ILogger> logger,
        std::shared_ptr<actuators::IActuator> pump)
        : IActuatorStrategy(logger, pump),
          minWaterLevel   (irrigation::DEFAULT_MIN_WATER_LEVEL),
          sensorsManager  (sensors::SensorsManager::getInstance()),
          volumeCheckTaskId(0),
          isIrrigating    (false)
    {
        updateFromConfig();
    }
    
    void IrrigationStrategy::updateFromConfig()
    {
        bool hasAllKeys = true;
        if (configManager->hasKey(config::ConfigType::System, irrigation::CONFIG_KEY_INTERVAL))
        {
            // Используем float для интервала полива, в ESP32 этого достаточно
            pumpIntervalDays = configManager->getValue<float>(config::ConfigType::System, irrigation::CONFIG_KEY_INTERVAL);
            logger->log(log::Level::Debug, 
                      "%sЗагружен интервал полива: %.2f дн.", 
                      logging::PREFIX_IRRIGATION,
                      pumpIntervalDays);
        }
        else
        {
            hasAllKeys = false;
        }
        
        if (configManager->hasKey(config::ConfigType::System, config::strategies::irrigation::CONFIG_KEY_START_TIME))
        {
            pumpStartTime = configManager->getValue<String>(config::ConfigType::System, irrigation::CONFIG_KEY_START_TIME);
            logger->log(log::Level::Debug, 
                      "%sЗагружено время начала полива: %s", 
                      logging::PREFIX_IRRIGATION,
                      pumpStartTime.c_str());
        }
        else
        {
            hasAllKeys = false;
        }
        
        if (configManager->hasKey(config::ConfigType::System, irrigation::CONFIG_KEY_WATER_VOLUME))
        {
            waterVolume = configManager->getValue<float>(config::ConfigType::System, irrigation::CONFIG_KEY_WATER_VOLUME);
            logger->log(log::Level::Debug, 
                      "%sЗагружен объем воды для полива: %.1f мл", 
                      logging::PREFIX_IRRIGATION,
                      waterVolume);
        }
        else
        {
            hasAllKeys = false;
        }

        if (!hasAllKeys)
        {
            logger->log(log::Level::Error, 
                      "%sНе удалось загрузить все настройки: нет ключей в конфигурации",
                      config::strategies::logging::PREFIX_IRRIGATION);
        }
    }
    
    bool IrrigationStrategy::applyStrategy()
    {
        int pumpStartHour = std::stoi(pumpStartTime.substring(0, 2).c_str());
        int pumpStartMinute = std::stoi(pumpStartTime.substring(3, 5).c_str());
        
        // Создаем Datime для времени полива
        Datime pumpTime(NTP.getUnix()); // Текущий день
        pumpTime.hour = pumpStartHour;
        pumpTime.minute = pumpStartMinute;
        pumpTime.second = 0;
            
        // Планируем периодический полив
        uint64_t pumpTaskId = schedulerManager->schedulePeriodicAt(
            pumpTime,
            static_cast<uint32_t>(pumpIntervalDays * 24.0f * 3600.0f), // Период в секундах (дни * 24 часа * 3600 секунд)
            [this]() { this->performIrrigation(); }
        );

        if (pumpTaskId == 0)
        {
            logger->log(log::Level::Error, 
                      "%sНе удалось запланировать полив", 
                      logging::PREFIX_IRRIGATION);
            return false;
        }
        
        scheduledTaskIds.push_back(pumpTaskId);
        
        logger->log(log::Level::Farm, 
                  "%sЗапланирован полив в %s периодичностью %.2f д", logging::PREFIX_IRRIGATION,
                  pumpStartTime.c_str(), pumpIntervalDays);
        logger->log(log::Level::Farm, "%sTaskID: #%llu", logging::PREFIX_IRRIGATION, pumpTaskId);
        
        return true;
    }
    
    void IrrigationStrategy::performIrrigation()
    {
        // Проверяем наличие датчика уровня воды
        if (!sensorsManager->hasSensor(names::HCSR04)) 
        {
            logger->log(Level::Error, "%sНевозможно выполнить полив: датчик уровня воды не найден", 
                      logging::PREFIX_IRRIGATION);
            return;
        }

        // Проверяем наличие датчика расхода воды
        if (!sensorsManager->hasSensor(names::YFS401)) 
        {
            logger->log(Level::Error, "%sНевозможно выполнить полив: датчик расхода воды не найден", 
            logging::PREFIX_IRRIGATION);
            return;
        }

        // Если полив уже запущен, то прекращаем выполнение
        if (isIrrigating)
        {
            logger->log(log::Level::Warning, 
                      "%sПолив не запущен: предыдущий полив еще активен", 
                      logging::PREFIX_IRRIGATION);
            return;
        }

        // Проверка уровня воды перед поливом
        float waterLevel = sensorsManager->getLastMeasurement(sensors::names::HCSR04);
        
        if (waterLevel <= minWaterLevel)
        {
            logger->log(log::Level::Warning, 
                      "%sПолив отменен: низкий уровень воды (%.1f%%)", 
                      logging::PREFIX_IRRIGATION,
                      waterLevel);
            return;
        }

        auto flowSensor = sensorsManager->getSensor(sensors::names::YFS401);
        
        if (!flowSensor)
        {
            logger->log(log::Level::Error, 
                      "%sРасходомер воды не найден, полив отменен", 
                      config::strategies::logging::PREFIX_IRRIGATION);
            return;
        }

        // Конвертируем к типу YFS401
        auto yfs401 = std::static_pointer_cast<sensors::YFS401>(flowSensor);
        
        if (!yfs401)
        {
            logger->log(log::Level::Error, 
                      "%sОшибка приведения типа датчика, полив отменен", 
                      config::strategies::logging::PREFIX_IRRIGATION);
            return;
        }

        yfs401->enable(true);
        
        logger->log(log::Level::Farm, 
                  "%sНачало полива, целевой объем: %.1f мл", 
                  config::strategies::logging::PREFIX_IRRIGATION,
                  waterVolume);
                  
        actuator->turnOn();
        
        isIrrigating = true;

        logger->log(log::Level::Debug, 
                    "%sПланирование периодической проверки пропущенной воды", 
                    config::strategies::logging::PREFIX_IRRIGATION);
        
        // Планируем периодическую проверку объема пролитой воды
        volumeCheckTaskId = schedulerManager->schedulePeriodicAfter(
            0,
            config::strategies::irrigation::VOLUME_CHECK_INTERVAL_S,
            [this]() { this->checkWaterVolume(); }
        );
        
        scheduledTaskIds.push_back(volumeCheckTaskId);

        logger->log(log::Level::Debug, 
                    "%sПланирование таймаута отключения насоса", 
                    config::strategies::logging::PREFIX_IRRIGATION);

        // Также добавим защитный таймаут на случай проблем с расходомером
        uint64_t timeoutTaskId = schedulerManager->scheduleOnceAfter(
            config::strategies::irrigation::IRRIGATION_TIMEOUT_SEC,
            [this]() {
                if (isIrrigating)
                {
                    logger->log(log::Level::Warning, 
                              "%sПолив принудительно остановлен по таймауту", 
                              config::strategies::logging::PREFIX_IRRIGATION);
                    stopIrrigation();
                }
            }
        );
        
        scheduledTaskIds.push_back(timeoutTaskId);
    }
    
    void IrrigationStrategy::checkWaterVolume()
    {
        // Если полив не активен, отменяем задачу
        if (!isIrrigating)
        {
            if (volumeCheckTaskId != 0)
            {
                schedulerManager->removeScheduledEvent(volumeCheckTaskId);
                volumeCheckTaskId = 0;
            }
            return;
        }
        
        // Проверяем уровень воды во время полива
        float waterLevel = sensorsManager->getLastMeasurement(sensors::names::HCSR04);
        
        if (waterLevel <= minWaterLevel)
        {
            logger->log(log::Level::Warning, 
                      "%sПолив остановлен: уровень воды слишком низкий (%.1f%%)", 
                      logging::PREFIX_IRRIGATION,
                      waterLevel);
            stopIrrigation();
            return;
        }
        
        // Получаем доступ к расходомеру воды
        auto flowSensor = sensorsManager->getSensor(sensors::names::YFS401);
        
        if (!flowSensor)
        {
            logger->log(log::Level::Error, 
                      "%sРасходомер воды не найден во время полива", 
                      logging::PREFIX_IRRIGATION);
            stopIrrigation();
            return;
        }
        
        // Конвертируем к типу YFS401
        auto yfs401 = std::static_pointer_cast<sensors::YFS401>(flowSensor);
        
        if (!yfs401)
        {
            logger->log(log::Level::Error, 
                      "%sОшибка приведения типа датчика во время полива", 
                      logging::PREFIX_IRRIGATION);
            stopIrrigation();
            return;
        }
        
        // Получаем текущий объем воды в литрах
        float currentVolume = yfs401->getTotalVolume();
        
        // Переводим в миллилитры для сравнения с целевым объемом
        float volumeInMl = currentVolume * 1000.0f;
        
        logger->log(log::Level::Debug, 
                  "%sТекущий объем: %.1f мл / %.1f мл (%.1f%%)", 
                  logging::PREFIX_IRRIGATION,
                  volumeInMl, waterVolume, (volumeInMl / waterVolume) * 100.0f);
        
        if (volumeInMl >= waterVolume)
        {
            logger->log(log::Level::Farm, 
                      "%sДостигнут целевой объем: %.1f мл, завершение полива", 
                      logging::PREFIX_IRRIGATION,
                      volumeInMl);
            stopIrrigation();
        }
    }
    
    void IrrigationStrategy::stopIrrigation()
    {
        if (!isIrrigating)
        {
            return;
        }
        
        actuator->turnOff();
        
        auto flowSensor = sensorsManager->getSensor(sensors::names::YFS401);
        if (flowSensor)
        {
            auto yfs401 = std::static_pointer_cast<sensors::YFS401>(flowSensor);
            if (yfs401)
            {
                // Получаем финальный объем воды для логирования
                float finalVolume = yfs401->getTotalVolume() * 1000.0f; // В миллилитрах
                
                yfs401->enable(false);
                
                logger->log(log::Level::Farm, 
                            "%sПолив завершен, итого пролито: %.1f мл", 
                            config::strategies::logging::PREFIX_IRRIGATION,
                            finalVolume);
            }
            else
            {
                logger->log(log::Level::Error,
                            "%sНе удалось привести датчик к типу YFS401",
                            config::strategies::logging::PREFIX_IRRIGATION);
            }
        }
        else
        {
            logger->log(log::Level::Error,
                        "%sНе удалось получить доступ к расходомеру воды для выключения",
                        config::strategies::logging::PREFIX_IRRIGATION);
        }
        
        // Отменяем задачу проверки объема
        if (volumeCheckTaskId != 0)
        {
            schedulerManager->removeScheduledEvent(volumeCheckTaskId);
            volumeCheckTaskId = 0;
        }
        
        isIrrigating = false;
    }

    void IrrigationStrategy::forceTurnOn()
    {
        if (logger)
        {
            logger->log(log::Level::Farm, "%sПринудительный полив запущен", logging::PREFIX_IRRIGATION);
        }
        
        // Просто используем существующий метод полива, который уже содержит всю необходимую логику
        performIrrigation();
    }
    
    void IrrigationStrategy::forceTurnOff()
    {
        logger->log(log::Level::Farm, "[IrrigationStrategy] Принудительное выключение насоса");
        
        if (isIrrigating)
        {
            logger->log(log::Level::Warning, "[IrrigationStrategy] Прерывание активного "
                                           "автоматического полива");
            stopIrrigation();
            return;
        }
        
        // Проверяем, включен ли насос
        if (!actuator->getState())
        {
            logger->log(log::Level::Warning, "[IrrigationStrategy] Попытка выключить насос, "
                                               "но он уже выключен");
            return;
        }

        // По идее, этот код не должен выполняться, но мало ли...
        logger->log(log::Level::Warning, 
                  "%sЭтого сообщения быть не должно", 
                  logging::PREFIX_IRRIGATION);
        
        // Все равно выключаем насос и т.д...
        actuator->turnOff();
        
        auto flowSensor = sensorsManager->getSensor(sensors::names::YFS401);
        if (flowSensor)
        {
            auto yfs401 = std::static_pointer_cast<sensors::YFS401>(flowSensor);
            if (yfs401)
            {
                float finalVolume = yfs401->getTotalVolume() * 1000.0f; // В миллилитрах
                
                yfs401->enable(false);
                
                if (logger)
                {
                    logger->log(log::Level::Farm, 
                              "%sПолив завершен, итого пролито: %.1f мл", 
                              config::strategies::logging::PREFIX_IRRIGATION,
                              finalVolume);
                }
            }
        }

        // Отменяем задачу проверки объема
        if (volumeCheckTaskId != 0)
        {
            schedulerManager->removeScheduledEvent(volumeCheckTaskId);
            volumeCheckTaskId = 0;
        }
        
        if (logger)
        {
            logger->log(log::Level::Info, 
                      "%sНасос принудительно выключен", 
                      logging::PREFIX_IRRIGATION);
        }
    }
}