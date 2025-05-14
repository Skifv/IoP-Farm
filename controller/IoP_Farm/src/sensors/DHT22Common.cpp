#include "sensors/DHT22Common.h"

namespace farm::sensors
{
    // Инициализация статической переменной
    std::map<int8_t, std::shared_ptr<DHT>> DHT22Common::s_dhtInstances;
    
    std::shared_ptr<DHT> DHT22Common::getInstance(int8_t pin, std::shared_ptr<ILogger> logger)
    {
        if (pin == calibration::UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                     "[DHT22Common] Ошибка: неинициализированный пин");
            return nullptr;
        }
        
        // Проверяем, существует ли уже объект для этого пина
        auto it = s_dhtInstances.find(pin);
        if (it != s_dhtInstances.end()) {
            return it->second;
        }
        
        try {
            std::shared_ptr<DHT> dht = std::make_shared<DHT>(pin, DHT22);
            dht->begin();
            s_dhtInstances[pin] = dht;
            
            return dht;
        } catch (...) {
            logger->log(Level::Error, 
                     "[DHT22Common] Ошибка при создании объекта DHT");
            return nullptr;
        }
    }
    
    void DHT22Common::releaseInstance(int8_t pin)
    {
        auto it = s_dhtInstances.find(pin);
        if (it != s_dhtInstances.end()) {
            s_dhtInstances.erase(it);
        }
    }
    
    void DHT22Common::releaseAll()
    {
        s_dhtInstances.clear();
    }
} 