#pragma once
#include <OneWire.h>
#include <DallasTemperature.h>
#include <map>
#include <memory>
#include "utils/logger_factory.h"
#include "config/constants.h"

namespace farm::sensors
{
    using namespace farm::log;
    using namespace farm::config;
    using namespace farm::config::sensors;
    
    // Структура для хранения пары OneWire и DallasTemperature
    struct DS18B20Resources
    {
        std::shared_ptr<OneWire> oneWire;
        std::shared_ptr<DallasTemperature> sensors;
        
        DS18B20Resources(std::shared_ptr<OneWire> wire, std::shared_ptr<DallasTemperature> temp)
            : oneWire(wire), sensors(temp) {}
    };
    
    // Статический класс для управления общими ресурсами датчиков DS18B20
    class DS18B20Common
    {
    private:
        // Статическая карта ресурсов по пинам
        // Используем int8_t для совместимости с UNINITIALIZED_PIN
        static std::map<int8_t, std::shared_ptr<DS18B20Resources>> s_resourceInstances;
        
    public:
        // Получение ресурсов DS18B20 по пину
        // Если экземпляры не существуют, создаёт их
        static std::shared_ptr<DS18B20Resources> getInstance(int8_t pin, std::shared_ptr<ILogger> logger);
        
        static void releaseInstance(int8_t pin);      
        static void releaseAll();
    };
} 