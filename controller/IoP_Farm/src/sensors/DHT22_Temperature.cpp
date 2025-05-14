#include "sensors/DHT22_Temperature.h"
#include "sensors/DHT22Common.h"

namespace farm::sensors
{
    DHT22_Temperature::DHT22_Temperature(std::shared_ptr<log::ILogger> logger, uint8_t pin)
        : ISensor(),
          pin(pin),
          lastReadTime(0) 
    {
        this->logger = logger;
        
        setSensorName(names::DHT22_TEMPERATURE);
        setMeasurementType(json_keys::TEMPERATURE_DHT22);
        setUnit(units::CELSIUS);
        
        shouldBeRead = true;
        shouldBeSaved = true;
        
        lastMeasurement = calibration::NO_DATA;
        
        // Пока не создаем объект DHT, это делаем в initialize()
        dht = nullptr;
    }
    
    DHT22_Temperature::~DHT22_Temperature()
    {
        // Память будет освобождена автоматически благодаря std::shared_ptr
    }
    
    bool DHT22_Temperature::initialize()
    {
        if (pin == calibration::UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                     "[DHT22_Temperature] Не задан пин для датчика");
            return false;
        }
        
        dht = DHT22Common::getInstance(pin, logger);
        
        if (!dht)
        {
            logger->log(Level::Error, 
                     "[DHT22_Temperature] Ошибка при получении объекта DHT");
            return false;
        }
        
        initialized = true;
        return true;
    }
    
    // Считать температуру в градусах Цельсия
    float DHT22_Temperature::read()
    {
        if (!initialized || !dht)
        {
            logger->log(Level::Error, 
                     "[DHT22_Temperature] Датчик не инициализирован");
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        unsigned long currentTime = millis();
        
        // Проверяем, прошло ли достаточно времени с момента последнего чтения
        if (lastReadTime > 0 && (currentTime - lastReadTime < MIN_READ_INTERVAL))
        {
            // Если прошло меньше 2 секунд, возвращаем последнее измеренное значение
            // без обращения к датчику (специфика работы DHT22)
            return lastMeasurement;
        }
        
        float temperature = dht->readTemperature();
        
        lastReadTime = currentTime;
        
        if (isnan(temperature)) 
        {
            logger->log(Level::Error, 
                      "[DHT22_Temperature] Не удалось считать данные с датчика");
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        lastMeasurement = temperature;
        
        return temperature;
    }
} 