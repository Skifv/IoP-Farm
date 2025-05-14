#include "sensors/DS18B20.h"

namespace farm::sensors
{
    DS18B20::DS18B20(std::shared_ptr<log::ILogger> logger, uint8_t pin)
        : ISensor(),
          pin(pin),
          resources(nullptr)
    {
        this->logger = logger;
        
        setSensorName(names::DS18B20);
        setMeasurementType(json_keys::TEMPERATURE_DS18B20);
        setUnit(units::CELSIUS);
        
        shouldBeRead = true;
        shouldBeSaved = true;
        
        lastMeasurement = calibration::NO_DATA;
        
        // Установка адреса устройства по умолчанию (все нули - использовать первое найденное устройство)
        memset(deviceAddress, 0, sizeof(deviceAddress));
    }
    
    DS18B20::~DS18B20()
    {
        // Память будет освобождена автоматически благодаря std::shared_ptr
    }
    
    bool DS18B20::initialize()
    {
        if (pin == calibration::UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                     "[DS18B20] Не задан пин для датчика");
            return false;
        }
        
        resources = DS18B20Common::getInstance(pin, logger);
        
        if (!resources)
        {
            logger->log(Level::Error, 
                     "[DS18B20] Ошибка при получении ресурсов датчика");
            return false;
        }
        
        // Если адрес не был установлен, получаем адрес первого устройства
        if (memcmp(deviceAddress, "\0\0\0\0\0\0\0\0", 8) == 0)
        {
            if (!resources->sensors->getAddress(deviceAddress, 0))
            {
                logger->log(Level::Error, 
                         "[DS18B20] Не удалось получить адрес устройства");
                return false;
            }
            
            char addressStr[25] = {0};
            for (uint8_t i = 0; i < 8; i++)
            {
                sprintf(addressStr + i*3, "%02X%s", deviceAddress[i], (i < 7) ? ":" : "");
            }
            
            logger->log(Level::Debug, 
                      "[DS18B20] Адрес устройства: %s", 
                      addressStr);
        }
        
        // Устанавливаем разрешение 12 бит (максимальная точность)
        resources->sensors->setResolution(deviceAddress, 12);
        
        initialized = true;
        return true;
    }
    
    float DS18B20::read()
    {
        if (!initialized || !resources)
        {
            logger->log(Level::Error, 
                     "[DS18B20] Датчик не инициализирован");
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        if (memcmp(deviceAddress, "\0\0\0\0\0\0\0\0", 8) == 0)
        {
            // Используем первый найденный датчик
            resources->sensors->requestTemperatures();
            float temp = resources->sensors->getTempCByIndex(0);
            
            // Проверяем корректность считанного значения
            if (temp == DEVICE_DISCONNECTED_C) 
            {
                logger->log(Level::Warning, 
                          "[DS18B20] Датчик отключен или ошибка чтения");
                lastMeasurement = calibration::SENSOR_ERROR_VALUE;
                return calibration::SENSOR_ERROR_VALUE;
            }
            
            lastMeasurement = temp;
            return temp;
        }
        else
        {
            // Используем датчик по адресу
            resources->sensors->requestTemperaturesByAddress(deviceAddress);
            float temp = resources->sensors->getTempC(deviceAddress);
            
            // Проверяем корректность считанного значения
            if (temp == DEVICE_DISCONNECTED_C) 
            {
                logger->log(Level::Warning, 
                          "[DS18B20] Датчик отключен или ошибка чтения");
                lastMeasurement = calibration::SENSOR_ERROR_VALUE;
                return calibration::SENSOR_ERROR_VALUE;
            }
            
            lastMeasurement = temp;
            return temp;
        }
    }
    
    // Установить адрес устройства
    void DS18B20::setDeviceAddress(const uint8_t* address)
    {
        if (address != nullptr)
        {
            memcpy(deviceAddress, address, 8);
            
            // Выводим адрес для отладки
            char addressStr[25] = {0};
            for (uint8_t i = 0; i < 8; i++)
            {
                sprintf(addressStr + i*3, "%02X%s", deviceAddress[i], (i < 7) ? ":" : "");
            }
            
            logger->log(Level::Debug, 
                      "[DS18B20] Установлен адрес устройства: %s", 
                      addressStr);
        }
        else
        {
            // Если передан nullptr, сбрасываем адрес
            memset(deviceAddress, 0, sizeof(deviceAddress));
            logger->log(Level::Debug, 
                      "[DS18B20] Адрес устройства сброшен");
        }
    }
} 