#include "sensors/sensors_manager.h"
#include "sensors/DHT22Common.h"
#include "sensors/DS18B20Common.h"

namespace farm::sensors
{
    using namespace farm::log;
    using namespace farm::config;
    using namespace farm::config::sensors;
    
    // Singleton: гарантирует единственный SensorsManager на всю систему
    std::shared_ptr<SensorsManager> SensorsManager::instance = nullptr;
    
    SensorsManager::SensorsManager(std::shared_ptr<log::ILogger> logger)
        : lastReadTime(0),
          readInterval(timing::DEFAULT_READ_INTERVAL),
          enabled(true) // Датчики включены при старте для немедленного сбора данных
    {
        if (logger == nullptr) 
        {
            this->logger = LoggerFactory::createSerialLogger(Level::Info);
        } 
        else 
        {
            this->logger = logger;
        }
        
        configManager = ConfigManager::getInstance(this->logger);
        
        // Получаем экземпляр MQTTManager для публикации данных
        mqttManager = net::MQTTManager::getInstance(this->logger);
    }
    
    // Получение экземпляра синглтона (гарантирует единственный SensorsManager на всю систему)
    std::shared_ptr<SensorsManager> SensorsManager::getInstance(std::shared_ptr<ILogger> logger)
    {
        if (instance == nullptr) 
        {
            // Используем явное создание вместо make_shared, т.к. конструктор приватный
            instance = std::shared_ptr<SensorsManager>(new SensorsManager(logger));
        }
        return instance;
    }

    SensorsManager::~SensorsManager()
    {
        logger->log(Level::Debug, "[Sensors] Освобождение ресурсов SensorsManager");
        
        clearSensors();
        
        DHT22Common::releaseAll();
        DS18B20Common::releaseAll();
        
        logger->log(Level::Debug, "[Sensors] Ресурсы освобождены");
    }

    
    // Инициализация всех датчиков, если пины настроены
    bool SensorsManager::initialize()
    {
        logger->log(Level::Farm, "[Sensors] Инициализация датчиков");
        
        clearSensors();
        
        bool allInitialized = true;
        
        int totalChecked = 0;

        // Датчик DHT22 (температура)
        if (pins::DHT22_PIN != calibration::UNINITIALIZED_PIN) 
        {
            if (!createAndInitSensor<DHT22_Temperature>(pins::DHT22_PIN)) 
            {
                allInitialized = false;
            }
            totalChecked++;
        }
        
        // Датчик DHT22 (влажность)
        if (pins::DHT22_PIN != calibration::UNINITIALIZED_PIN) 
        {
            if (!createAndInitSensor<DHT22_Humidity>(pins::DHT22_PIN)) 
            {
                allInitialized = false;
            }
            totalChecked++;
        }
        
        // Датчик DS18B20 (температура воды)
        if (pins::DS18B20_PIN != calibration::UNINITIALIZED_PIN) 
        {
            if (!createAndInitSensor<DS18B20>(pins::DS18B20_PIN)) 
            {
                allInitialized = false;
            }
            totalChecked++;
        }
        
        // Датчик HC-SR04 (уровень воды)
        if (pins::HC_SR04_TRIG_PIN != calibration::UNINITIALIZED_PIN && pins::HC_SR04_ECHO_PIN != calibration::UNINITIALIZED_PIN) 
        {
            if (!createAndInitSensor<HCSR04>(pins::HC_SR04_TRIG_PIN, pins::HC_SR04_ECHO_PIN)) 
            {
                allInitialized = false;
            }
            totalChecked++;
        }
        
        // Датчик FC-28 (влажность почвы)
        if (pins::FC28_PIN != calibration::UNINITIALIZED_PIN) 
        {
            if (!createAndInitSensor<FC28>(pins::FC28_PIN)) 
            {
                allInitialized = false;
            }
            totalChecked++;
        }
        
        // Датчик KY-018 (освещённость)
        if (pins::KY018_PIN != calibration::UNINITIALIZED_PIN) 
        {
            if (!createAndInitSensor<KY018>(pins::KY018_PIN)) 
            {
                allInitialized = false;
            }
            totalChecked++;
        }
        
        // Датчик YF-S401 (расходомер)
        if (pins::YFS401_PIN != calibration::UNINITIALIZED_PIN) 
        {
            if (!createAndInitSensor<YFS401>(pins::YFS401_PIN)) 
            {
                allInitialized = false;
            }
            totalChecked++;
        }
        
        logger->log(Level::Farm, 
                  "[Sensors] Количество инициализированных датчиков: %d/%d", 
                  sensors.size(), totalChecked);
        
        return allInitialized;
    }
    
    // Считать значения со всех датчиков (без сохранения)
    bool SensorsManager::readAllSensors()
    {
        bool allSuccess = true;
        int successCount = 0;
        int totalReadable = 0;
        
        for (auto& [name, sensor] : sensors) 
        {
            if (!sensor->shouldBeRead) {
                continue;
            }
            
            totalReadable++;
            
            if (sensor->read() != calibration::SENSOR_ERROR_VALUE) 
            {
                logger->log(Level::Info, 
                          "[Sensors] [%s] %s = %.2f %s", 
                          name.c_str(), 
                          sensor->getMeasurementType().c_str(), 
                          sensor->getLastMeasurement(), 
                          sensor->getUnit().c_str());
                successCount++;
            } 
            else 
            {
                logger->log(Level::Error, 
                          "[Sensors] Ошибка считывания данных с датчика %s (%s)", 
                          name.c_str(), 
                          sensor->getMeasurementType().c_str());
                allSuccess = false;
            }
        }
        
        logger->log(Level::Farm, 
                  "[Sensors] Успешно считано: %d/%d", 
                  successCount, totalReadable);
        
        return allSuccess;
    }
    
    // Сохранить все измеренные значения в оперативную память (Json)
    bool SensorsManager::saveAllMeasurements()
    {
        bool allSuccess = true;
        int successCount = 0;
        int totalSaveable = 0;
        
        for (auto& [name, sensor] : sensors) 
        {
            if (!sensor->shouldBeSaved) {
                continue;
            }
            
            totalSaveable++;

            if (sensor->saveMeasurement()) 
            {
                successCount++;
            } 
            else 
            {
                logger->log(Level::Error, 
                          "[Sensors] Ошибка сохранения данных с датчика %s (%s)", 
                          name.c_str(), 
                          sensor->getMeasurementType().c_str());
                allSuccess = false;
            }
        }
        
        logger->log(Level::Farm, 
                  "[Sensors] Успешно сохранено: %d/%d", 
                  successCount, totalSaveable);
        
        return allSuccess;
    }
    
    // Основной цикл опроса датчиков и публикации данных, вызывается в main.cpp
    bool SensorsManager::loop()
    {
        if (!enabled) {
            return true;
        }

        // Проверяем, прошло ли достаточно времени для нового считывания
        unsigned long currentTime = millis();
        if (currentTime - lastReadTime >= readInterval) 
        {
            lastReadTime = currentTime;
            
            logger->log(Level::Farm, "[Sensors] Запуск цикла считывания данных");
            
            readAllSensors();
            saveAllMeasurements();
            configManager->saveConfig(ConfigType::Data);
            
            // Если подключены к MQTT, публикуем данные
            if (mqttManager && mqttManager->isClientConnected()) 
            {
                bool published = mqttManager->publishData();
                
                if (!published) 
                {
                    logger->log(Level::Error, "[Sensors] Ошибка публикации данных в MQTT");
                }
                
                return true; // Продолжаем работу, даже если не удалось опубликовать данные
            }
            else
            {
                logger->log(Level::Warning, "[Sensors] Нет подключения к MQTT для отправки данных");
                return false;
            }
        }
        
        return true;
    }

    float SensorsManager::sensorRead(const String &sensorName)
    {
        return sensors[sensorName]->read();
    }

    bool SensorsManager::sensorSave(const String &sensorName)
    {
        return sensors[sensorName]->saveMeasurement();
    }

    float SensorsManager::sensorReadAndSave(const String &sensorName)
    {
        float value = sensorRead(sensorName);
        sensorSave(sensorName);
        return value;
    }

    String SensorsManager::getSensorType(const String &sensorName)
    {
        return sensors[sensorName]->getMeasurementType();
    }

    String SensorsManager::getSensorUnit(const String &sensorName)
    {
        return sensors[sensorName]->getUnit();
    }
    
    float SensorsManager::getLastMeasurement(const String &sensorName)
    {
        return sensors[sensorName]->getLastMeasurement();
    } 
    
    bool SensorsManager::addSensor(const String& sensorName, std::shared_ptr<ISensor> sensor)
    {
        if (sensors.find(sensorName) != sensors.end()) 
        {
            logger->log(Level::Warning, 
                      "[Sensors] Датчик с именем %s (%s) уже существует", 
                      sensorName.c_str(), 
                      sensor->getMeasurementType().c_str());
            return false;
        }
        if (!sensor->initialized) 
        {
            if (!sensor->initialize()) 
            {
                logger->log(Level::Error, 
                          "[Sensors] Не удалось инициализировать датчик %s (%s)", 
                          sensorName.c_str(), 
                          sensor->getMeasurementType().c_str());
                return false;
            }
        }
        sensors[sensorName] = sensor;
        logger->log(Level::Info, 
                  "[Sensors] Датчик %s (%s) добавлен в SensorsManager", 
                  sensorName.c_str(), 
                  sensor->getMeasurementType().c_str());
        return true;
    }

    void SensorsManager::setReadInterval(unsigned long interval)
    {
        readInterval = interval;
        logger->log(Level::Info, 
                  "[Sensors] Установлен интервал считывания датчиков: %lu мс", 
                  interval);
    }

    bool SensorsManager::removeSensor(const String& sensorName)
    {
        auto it = sensors.find(sensorName);
        if (it == sensors.end()) 
        {
            logger->log(Level::Warning, 
                      "[Sensors] Датчик с именем %s не найден", 
                      sensorName.c_str());
            return false;
        }
        sensors.erase(it);
        logger->log(Level::Debug, 
                  "[Sensors] Датчик %s удален", 
                  sensorName.c_str());
        return true;
    }
    
    std::shared_ptr<ISensor> SensorsManager::getSensor(const String& sensorName)
    {
        auto it = sensors.find(sensorName);
        if (it != sensors.end()) 
        {
            return it->second;
        }
        if (logger)
        {
            logger->log(Level::Warning, 
                      "[Sensors] Датчик с именем %s не найден", 
                      sensorName.c_str());
        }
        return nullptr;
    }
    
    bool SensorsManager::hasSensor(const String& sensorName) const
    {
        return sensors.find(sensorName) != sensors.end();
    }
    
    size_t SensorsManager::getSensorCount() const
    {
        return sensors.size();
    }
    
    void SensorsManager::clearSensors()
    {
        sensors.clear();
        logger->log(Level::Debug, "[Sensors] Все датчики удалены");
    }

    void SensorsManager::sensorsEnable(bool enable)
    {
        if (enabled == enable) {
            logger->log(Level::Debug, "[Sensors] Состояние датчиков не изменилось");
            return; // Если состояние не изменилось, ничего не делаем
        }

        enabled = enable;
        
        if (enable) {
            logger->log(Level::Farm, "[Sensors] Датчики включены");
        } else {
            logger->log(Level::Farm, "[Sensors] Датчики выключены");
        }
    }

    bool SensorsManager::isEnabled() const  
    {
        return enabled;
    }
}


