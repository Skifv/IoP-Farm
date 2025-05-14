#include "logic/strategies/IActuatorStrategy.h"

namespace farm::logic::strategies
{
    IActuatorStrategy::IActuatorStrategy(std::shared_ptr<log::ILogger> logger, 
                                        std::shared_ptr<actuators::IActuator> actuator)
        : logger          (logger),
          actuator        (actuator),
          schedulerManager(utils::Scheduler::getInstance()),
          configManager   (config::ConfigManager::getInstance()),
          scheduledTaskIds()
    {
    }
    
    IActuatorStrategy::~IActuatorStrategy()
    {
        // TODO: надо отменить все запланированные задачи, но чтобы логгер не вызывался
    }
    
    void IActuatorStrategy::cancelScheduledTasks()
    {
        logger->log(log::Level::Debug, "[IActuatorStrategy] Удаление всех запланированных событий");

        forceTurnOff();

        for (auto taskId : scheduledTaskIds)
        {
            schedulerManager->removeScheduledEvent(taskId);
        }
        scheduledTaskIds.clear();
    }
    
    bool IActuatorStrategy::reschedule()
    {
        cancelScheduledTasks();
        updateFromConfig();
        return applyStrategy();
    }
    
    std::shared_ptr<actuators::IActuator> IActuatorStrategy::getActuator() const
    {
        return actuator;
    }
} 