#pragma once
#include <DHT.h>
#include <map>
#include <memory> 
#include "utils/logger_factory.h"
#include "config/constants.h"

namespace farm::sensors
{
    using namespace farm::log;
    using namespace farm::config;
    using namespace farm::config::sensors;
    
    // Статический класс для управления общими ресурсами датчиков DHT22
    class DHT22Common
    {
    private:
        // Статическая карта объектов DHT по пинам
        // Используем int8_t для совместимости с UNINITIALIZED_PIN
        static std::map<int8_t, std::shared_ptr<DHT>> s_dhtInstances;
        
    public:
        // Получение экземпляра DHT по пину
        // Если экземпляр не существует, создаёт его
        static std::shared_ptr<DHT> getInstance(int8_t pin, std::shared_ptr<ILogger> logger);
        
        static void releaseInstance(int8_t pin);        
        static void releaseAll();
    };
} 