// Датчик освещенности

#include "sensors/KY018.h"
#include "utils/my_map.h"

namespace farm::sensors
{
    KY018::KY018(std::shared_ptr<log::ILogger> logger, uint8_t pin)
        : ISensor(),
          pin(pin),
          darkValue(calibration::KY018_DARK_VALUE),
          lightValue(calibration::KY018_LIGHT_VALUE)
    {
        this->logger = logger;
        
        setSensorName(names::KY018);
        setMeasurementType(json_keys::LIGHT_INTENSITY);
        setUnit(units::PERCENT);
        
        shouldBeRead = true;
        shouldBeSaved = true;

        lastMeasurement = calibration::NO_DATA;
    }
    
    bool KY018::initialize()
    {
        if (pin == calibration::UNINITIALIZED_PIN) 
        {
            logger->log(farm::log::Level::Error, 
                      "[KY018] Не инициализирован пин датчика");
            return false;
        }

        pinMode(pin, INPUT);
        
        initialized = true;
        return true;
    }
    
    void KY018::setCalibration(int darkValue, int lightValue)
    {
        if (darkValue > lightValue) 
        {
            this->darkValue = darkValue;
            this->lightValue = lightValue;
            
            logger->log(farm::log::Level::Debug, 
                      "[KY018] Установлены калибровочные значения: темно = %d, светло = %d", 
                      darkValue, lightValue);
        }
        else 
        {
            logger->log(farm::log::Level::Warning, 
                      "[KY018] Некорректные калибровочные значения: темно = %d, светло = %d", 
                      darkValue, lightValue);
        }
    }
    
    // Получить текущее сырое значение АЦП
    int KY018::getRawValue() const
    {
        if (!initialized || pin == calibration::UNINITIALIZED_PIN) 
        {
            logger->log(farm::log::Level::Error, 
                      "[KY018] Попытка чтения с неинициализированного датчика");
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        return analogRead(pin);
    }
    
    // Считать уровень освещенности в процентах (0-100%)
    float KY018::read()
    {
        if (!initialized) 
        {
            logger->log(farm::log::Level::Error, 
                      "[KY018] Попытка чтения с неинициализированного датчика");
            return calibration::SENSOR_ERROR_VALUE;
        }

        // Проверяем, что значения калибровки корректны
        if (darkValue <= lightValue) 
        {
            logger->log(farm::log::Level::Error, 
                      "[KY018] Некорректные калибровочные значения");
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }

        // Получаем сырое значение АЦП
        int rawValue = getRawValue();
        
        // Ограничиваем значение в пределах калибровки
        int constrained = constrain(rawValue, lightValue, darkValue);
        
        // Используем шаблонную функцию MyMap для масштабирования
        // Инвертируем шкалу: высокое значение АЦП (темно) -> 0%, низкое значение АЦП (светло) -> 100%
        lastMeasurement = utils::MyMap<int, float>(constrained, darkValue, lightValue, 0.0f, 100.0f);
        
        return lastMeasurement;
    }
} 