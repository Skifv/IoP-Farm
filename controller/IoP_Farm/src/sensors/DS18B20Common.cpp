#include "sensors/DS18B20Common.h"

namespace farm::sensors
{
    // Инициализация статической переменной
    std::map<int8_t, std::shared_ptr<DS18B20Resources>> DS18B20Common::s_resourceInstances;
    
    std::shared_ptr<DS18B20Resources> DS18B20Common::getInstance(int8_t pin, std::shared_ptr<ILogger> logger)
    {
        if (pin == calibration::UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                     "[DS18B20Common] Ошибка: неинициализированный пин");
            return nullptr;
        }
        
        // Проверяем, существуют ли уже объекты для этого пина
        auto it = s_resourceInstances.find(pin);
        if (it != s_resourceInstances.end()) {
            return it->second;
        }
        
        // Создаем новые объекты, если они не существуют
        try {
            auto oneWire = std::make_shared<OneWire>(pin);        
            auto sensors = std::make_shared<DallasTemperature>(oneWire.get());
            
            // Инициализируем шину 1-Wire и датчики
            sensors->begin();
            
            // Проверяем, есть ли датчики на шине
            if (sensors->getDeviceCount() == 0)
            {
                logger->log(Level::Error, 
                         "[DS18B20Common] Датчики не обнаружены на шине пина %d", pin);
                return nullptr;
            }
            
            logger->log(Level::Info, 
                      "[DS18B20Common] На пине %d обнаружено устройств: %d", 
                      pin, sensors->getDeviceCount());
            
            auto resources = std::make_shared<DS18B20Resources>(oneWire, sensors);
            
            s_resourceInstances[pin] = resources;
            
            return resources;
        } catch (...) {
            logger->log(Level::Error, 
                     "[DS18B20Common] Ошибка при создании ресурсов DS18B20");
            return nullptr;
        }
    }
    
    void DS18B20Common::releaseInstance(int8_t pin)
    {
        auto it = s_resourceInstances.find(pin);
        if (it != s_resourceInstances.end()) {
            s_resourceInstances.erase(it);
        }
    }
    
    void DS18B20Common::releaseAll()
    {
        s_resourceInstances.clear();
    }
} 