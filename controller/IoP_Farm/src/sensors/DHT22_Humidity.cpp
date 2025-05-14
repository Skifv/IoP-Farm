#include "sensors/DHT22_Humidity.h"
#include "sensors/DHT22Common.h"

namespace farm::sensors
{
    DHT22_Humidity::DHT22_Humidity(std::shared_ptr<log::ILogger> logger, uint8_t pin)
        : ISensor(),
          pin(pin),
          lastReadTime(0)
    {
        this->logger = logger;
        
        setSensorName(names::DHT22_HUMIDITY);
        setMeasurementType(json_keys::HUMIDITY);
        setUnit(units::PERCENT);
        
        shouldBeRead = true;
        shouldBeSaved = true;
        
        lastMeasurement = calibration::NO_DATA;
        
        // Пока не создаем объект DHT, это делаем в initialize()
        dht = nullptr;
    }
    
    DHT22_Humidity::~DHT22_Humidity()
    {
        // Память будет освобождена автоматически благодаря std::shared_ptr
    }
    
    bool DHT22_Humidity::initialize()
    {
        if (pin == calibration::UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                     "[DHT22_Humidity] Не задан пин для датчика");
            return false;
        }
        
        dht = DHT22Common::getInstance(pin, logger);
        
        if (!dht)
        {
            logger->log(Level::Error, 
                     "[DHT22_Humidity] Ошибка при получении объекта DHT");
            return false;
        }
        
        initialized = true;
        return true;
    }
    
    // Считать влажность в процентах (0-100%)
    float DHT22_Humidity::read()
    {
        if (!initialized || !dht)
        {
            logger->log(Level::Error, 
                     "[DHT22_Humidity] Датчик не инициализирован");
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        unsigned long currentTime = millis();
        
        if (lastReadTime > 0 && (currentTime - lastReadTime < MIN_READ_INTERVAL))
        {
            // Если прошло меньше 2 секунд, возвращаем последнее измеренное значение
            // без обращения к датчику (специфика работы DHT22)
            return lastMeasurement;
        }
        
        float humidity = dht->readHumidity();
        
        lastReadTime = currentTime;
        
        if (isnan(humidity)) 
        {
            logger->log(Level::Error, 
                      "[DHT22_Humidity] Не удалось считать данные с датчика");
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        lastMeasurement = humidity;
        
        return humidity;
    }
} 