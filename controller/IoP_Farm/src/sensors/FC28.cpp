// Датчик влажности почвы

#include "sensors/FC28.h"
#include "utils/my_map.h"

namespace farm::sensors
{
    FC28::FC28(std::shared_ptr<log::ILogger> logger, uint8_t pin)
        : ISensor(),
          pin(pin),
          dryValue(calibration::FC28_DRY_VALUE),
          wetValue(calibration::FC28_WET_VALUE)
    {
        this->logger = logger;
        
        setSensorName(names::FC28);
        setMeasurementType(json_keys::SOIL_MOISTURE);
        setUnit(units::PERCENT);
        
        shouldBeRead = true;
        shouldBeSaved = true;
        
        lastMeasurement = calibration::NO_DATA;
    }
    
    bool FC28::initialize()
    {
        if (pin == calibration::UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                     "[FC28] Не задан пин для датчика влажности почвы");
            return false;
        }
        
        pinMode(pin, INPUT);
        
        initialized = true;
        return true;
    }
    
    // Считать влажность почвы в процентах (0-100%)
    float FC28::read()
    {
        if (!initialized)
        {
            logger->log(Level::Error, 
                     "[FC28] Датчик не инициализирован");
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        // Считываем сырое значение АЦП
        int rawValue = getRawValue();
        
        // Проверяем корректность калибровочных значений
        if (dryValue <= wetValue)
        {
            logger->log(Level::Error, 
                      "[FC28] Некорректные калибровочные значения: dryValue=%d, wetValue=%d", 
                      dryValue, wetValue);
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        // Преобразуем в проценты влажности (0% - сухо, 100% - мокро)
        // Используем ограничение на случай, если значения выходят за пределы калибровки
        int constrained = constrain(rawValue, wetValue, dryValue);
        float moisturePercent = utils::MyMap<int, float>(constrained, dryValue, wetValue, 0.0f, 100.0f);

        lastMeasurement = moisturePercent;
        
        return moisturePercent;
    }
    
    // Получить текущее сырое значение АЦП
    int FC28::getRawValue() const
    {
        return analogRead(pin);
    }
    
    void FC28::setCalibration(int dryValue, int wetValue)
    {
        // Проверяем корректность значений (сухое значение должно быть больше влажного)
        if (dryValue <= wetValue)
        {
            logger->log(Level::Error, 
                      "[FC28] Некорректные калибровочные значения: dryValue=%d должно быть > wetValue=%d", 
                      dryValue, wetValue);
            return;
        }
        
        this->dryValue = dryValue;
        this->wetValue = wetValue;
        
        logger->log(Level::Info, 
                  "[FC28] Установлены калибровочные значения: dryValue=%d, wetValue=%d", 
                  dryValue, wetValue);
    }
} 