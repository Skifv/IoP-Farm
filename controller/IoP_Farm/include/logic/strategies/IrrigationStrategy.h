#pragma once

#include "IActuatorStrategy.h"
#include "actuators/IActuator.h"
#include "sensors/sensors_manager.h"

namespace farm::logic::strategies
{
    class IrrigationStrategy : public IActuatorStrategy
    {
    private:
        // Менеджер датчиков для проверки уровня воды
        std::shared_ptr<sensors::SensorsManager> sensorsManager;
        
        // Параметры стратегии
        float pumpIntervalDays;          // Интервал полива в днях (float для гибкости)
        String pumpStartTime;            // Время начала полива
        float waterVolume;               // Объем воды для полива в мл
        float minWaterLevel;             // Минимальный допустимый уровень воды в %
        
        // Параметры для контроля объема пролитой воды
        uint64_t volumeCheckTaskId;      // ID задачи проверки объема
        bool isIrrigating;               // Флаг активного полива
        
        // Метод проверки текущего объема пролитой воды
        void checkWaterVolume();
        
        void stopIrrigation();
        
    public:
        IrrigationStrategy(std::shared_ptr<log::ILogger> logger, 
                          std::shared_ptr<actuators::IActuator> pump);
        
        bool applyStrategy() override;
        
        void updateFromConfig() override;
        
        // Выполнить полив (вызывается планировщиком)
        void performIrrigation();
        
        void forceTurnOn();    
        void forceTurnOff();
    };
} 