#include "sensors/ISensor.h"

namespace farm::sensors
{
    using namespace farm::log;
    using namespace farm::config::sensors;
    using namespace farm::config;
    
    ISensor::ISensor()
        : lastMeasurement(calibration::NO_DATA),
          shouldBeRead(true),
          shouldBeSaved(true),
          initialized(false),
          configManager(ConfigManager::getInstance()) {}
    
    void ISensor::setSensorName(const String& name)
    {
        sensorName = name;
    }
    
    void ISensor::setMeasurementType(const String& type)
    {
        measurementType = type;
    }
    
    void ISensor::setUnit(const String& unitValue)
    {
        unit = unitValue;
    }
    
    bool ISensor::saveMeasurement()
    {
        if (lastMeasurement == calibration::NO_DATA)
        {
            logger->log(Level::Error, "[Sensor] %s: нет данных для сохранения, сохранение для дальнейшего анализа", sensorName.c_str());
            configManager->setValue(ConfigType::Data, measurementType.c_str(), lastMeasurement);
            return false;
        }
        else if (lastMeasurement == calibration::SENSOR_ERROR_VALUE)
        {
            logger->log(Level::Error, "[Sensor] %s: сохранение ошибочного значения для дальнейшего анализа", sensorName.c_str()); 
            configManager->setValue(ConfigType::Data, measurementType.c_str(), lastMeasurement);
            return false;
        }
        else
        {
            configManager->setValue(ConfigType::Data, measurementType.c_str(), lastMeasurement);
        }

        return true;
    }
    
    float ISensor::getLastMeasurement() const
    {
        if (lastMeasurement == calibration::NO_DATA)
        {
            logger->log(Level::Error, "[Sensor] %s: нет данных для чтения", sensorName.c_str());
            return calibration::SENSOR_ERROR_VALUE;
        }
        else if (lastMeasurement == calibration::SENSOR_ERROR_VALUE)
        {
            logger->log(Level::Error, "[Sensor] %s: ошибка чтения данных", sensorName.c_str());
            return calibration::SENSOR_ERROR_VALUE;
        }
        return lastMeasurement;
    }
    
    String ISensor::getSensorName() const
    {
        return sensorName;
    }
    
    String ISensor::getMeasurementType() const
    {
        return measurementType;
    }
    
    String ISensor::getUnit() const
    {
        return unit;
    }
} 