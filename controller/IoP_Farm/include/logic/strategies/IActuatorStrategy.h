#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include "utils/scheduler.h"
#include "config/config_manager.h"
#include "utils/logger_factory.h"
#include "actuators/IActuator.h"

namespace farm::logic::strategies
{
    // Интерфейс стратегии для исполнительных устройств
    class IActuatorStrategy
    {
    protected:
        std::shared_ptr<log::ILogger> logger;
        
        std::shared_ptr<actuators::IActuator> actuator;
        
        // Идентификаторы текущих запланированных задач
        std::vector<std::uint64_t> scheduledTaskIds;
        
        std::shared_ptr<utils::Scheduler> schedulerManager;
        
        std::shared_ptr<config::ConfigManager> configManager;
        
    public:
        IActuatorStrategy(std::shared_ptr<log::ILogger> logger, 
                          std::shared_ptr<actuators::IActuator> actuator);
        
        virtual ~IActuatorStrategy();

        std::shared_ptr<actuators::IActuator> getActuator() const;
        
        // Применить стратегию - запланировать задачи
        virtual bool applyStrategy() = 0;
        
        virtual void updateFromConfig() = 0;
        
        // Отменить все запланированные задачи, очистить стратегии
        virtual void cancelScheduledTasks();
        
        // Перепланировать задачи (при изменении конфигурации)
        virtual bool reschedule();
        
        virtual void forceTurnOn() = 0;   
        virtual void forceTurnOff() = 0;
    };
} 