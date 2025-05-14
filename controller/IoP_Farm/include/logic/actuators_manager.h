#pragma once

#include <memory>
#include <map>
#include <string>
#include <functional>
#include <typeinfo>

#include "utils/logger.h"
#include "config/config_manager.h"
#include "actuators/IActuator.h"
#include "utils/scheduler.h"
#include "logic/strategies/IActuatorStrategy.h"
#include "logic/strategies/IrrigationStrategy.h"
#include "logic/strategies/HeatingStrategy.h"
#include "logic/strategies/LightingStrategy.h"
#include "actuators/HeatLamp.h"
#include "actuators/GrowLight.h"
#include "actuators/PumpR385.h"
#include "config/constants.h"

namespace farm::sensors 
{ 
    class SensorsManager; 
}

namespace farm::logic
{
    class ActuatorsManager
    {
    private:
        // Приватный конструктор (паттерн Синглтон)
        explicit ActuatorsManager(std::shared_ptr<farm::log::ILogger> logger = nullptr);
        static std::shared_ptr<ActuatorsManager> instance;

        std::shared_ptr<log::ILogger> logger;
        
        std::shared_ptr<utils::Scheduler> schedulerManager;
        
        std::shared_ptr<config::ConfigManager> configManager;
        
        std::shared_ptr<farm::sensors::SensorsManager> sensorsManager;
        
        bool initialized = false;
        
        // Время последней проверки инициализации в loop()
        unsigned long lastCheckTime = 0;

        bool createStrategies();

        bool addActuator(const String& name, std::shared_ptr<actuators::IActuator> actuator);
        
        void deleteAllStrategies();
        
        // Метод для безопасной перезагрузки ESP
        void myRestartESP();

    public:
        // Карта актуаторов [имя актуатора, указатель на актуатор]
        std::map<String, std::shared_ptr<actuators::IActuator>> actuators;
        
        // Карта стратегий [имя актуатора, указатель на стратегию]
        std::map<String, std::shared_ptr<strategies::IActuatorStrategy>> strategies;

        // Запрещаем копирование и присваивание (для синглтона)
        ActuatorsManager(const ActuatorsManager&) = delete;
        ActuatorsManager& operator=(const ActuatorsManager&) = delete;
  
        ~ActuatorsManager();

        static std::shared_ptr<ActuatorsManager> getInstance(std::shared_ptr<log::ILogger> logger = nullptr);

        bool initialize();

        // Выключение или включение всех устройств фермы, необходимо для выполнения команды от MQTT
        void syncFarmState(bool enable);
        void actuatorsEnable(bool enable);
        
        // Метод loop для периодической проверки состояния NTP и инициализации при необходимости
        void loop();
        
        bool isInitialized() const;

        std::shared_ptr<actuators::IActuator> getActuator(const String& name);
        
        std::shared_ptr<strategies::IActuatorStrategy> getStrategy(const String& name);

        // Обновить все стратегии (например, после изменения конфигурации)
        bool updateStrategies();

        bool handleMqttCommand(const config::CommandCode& command);
        
        bool forceTurnOn(const String& actuatorName);
        
        bool forceTurnOff(const String& actuatorName);
        
        bool hasActuator(const String& name) const;
        
        bool hasStrategy(const String& name) const;
        
        size_t getActuatorCount() const;
        
        size_t getStrategyCount() const;

        // Реализация шаблонных методов должна быть в заголовочном файле
        template<typename ActuatorType, typename... Args>
        bool createAndInitActuator(Args&&... args)
        {
            auto actuator = std::make_shared<ActuatorType>(logger, std::forward<Args>(args)...);
            
            if (actuator->initialize())
            {
                addActuator(actuator->getActuatorName(), actuator);
                return true;
            }
            else
            {
                logger->log(log::Level::Error, "[ActuatorsManager] Не удалось инициализировать актуатор %s (%s)", 
                          actuator->getActuatorName().c_str(), actuator->getActuatorType().c_str());
                return false;
            }
        }

        template<typename StrategyType>
        bool createAndApplyStrategy(const String& actuatorName)
        {
            auto actuator = getActuator(actuatorName);
            if (!actuator)
            {
                logger->log(log::Level::Error, 
                          "[ActuatorsManager] Не удалось найти актуатор %s для применения стратегии", 
                          actuatorName.c_str());
                return false;
            }

            if (!actuator->isInitialized())
            {
                logger->log(log::Level::Error, 
                          "[ActuatorsManager] Актуатор %s не инициализирован", 
                          actuatorName.c_str());
                return false;
            }

            // Удаляем предыдущую стратегию для этого актуатора, если она существует
            auto it = strategies.find(actuatorName);
            if (it != strategies.end())
            {
                logger->log(log::Level::Debug, 
                          "[ActuatorsManager] Удаление предыдущей стратегии для %s", 
                          actuatorName.c_str());
                it->second->cancelScheduledTasks();
                strategies.erase(it);
            }

            // Создаем новую стратегию
            auto strategy = std::make_shared<StrategyType>(logger, actuator);
            
            // Применяем ее
            if (strategy->applyStrategy())
            {
                strategies[actuatorName] = strategy;
                logger->log(log::Level::Info, 
                          "[ActuatorsManager] Стратегия успешно применена к актуатору %s", 
                          actuatorName.c_str());
                return true;
            }
            else
            {
                logger->log(log::Level::Error, 
                          "[ActuatorsManager] Ошибка применения стратегии к актуатору %s", 
                          actuatorName.c_str());
                return false;
            }
        }
    };
}

        
