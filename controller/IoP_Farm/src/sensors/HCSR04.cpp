// Ультразвуковой датчик уровня воды

#include "sensors/HCSR04.h"

namespace farm::sensors
{
    HCSR04::HCSR04(std::shared_ptr<log::ILogger> logger, uint8_t trigPin, uint8_t echoPin)
        : ISensor(),
          trigPin(trigPin),
          echoPin(echoPin),
          containerDepth(calibration::HCSR04_DEFAULT_CONTAINER_DEPTH),
          A(calibration::HCSR04_A),
          B(calibration::HCSR04_B)
    {
        this->logger = logger;
        
        setSensorName(names::HCSR04);
        setMeasurementType(json_keys::WATER_LEVEL);
        setUnit(units::PERCENT); 
        
        shouldBeRead = true;
        shouldBeSaved = true;
        
        lastMeasurement = calibration::NO_DATA;
    }
    
    bool HCSR04::initialize()
    {
        if (trigPin == calibration::UNINITIALIZED_PIN || echoPin == calibration::UNINITIALIZED_PIN)
        {
            logger->log(Level::Error, 
                     "[HCSR04] Не заданы пины для ультразвукового датчика");
            return false;
        }
        
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
        
        digitalWrite(trigPin, LOW);
        
        initialized = true;

        return true;
    }
    
    // Считать расстояние до поверхности в сантиметрах
    float HCSR04::readDistance()
    {
        if (!initialized)
        {
            logger->log(Level::Error, 
                     "[HCSR04] Датчик не инициализирован");
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        // Считываем время высокого состояния Echo пина в микросекундах
        // Это время соответствует прохождению звука до объекта и обратно
        long duration = pulseIn(echoPin, HIGH, 38000); // Таймаут 38мс (для ~5м дистанции)
        
        // Если время нулевое, значит измерение не удалось
        if (duration == 0)
        {
            logger->log(Level::Error, 
                      "[HCSR04] Не удалось получить сигнал от датчика");
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        // Преобразуем время в расстояние
        // Время включает путь звука туда и обратно, поэтому делим на 2
        float distanceCm = (float)duration * calibration::SOUND_SPEED * 0.5f;
        
        // Проверяем корректность измерения
        if (distanceCm < 2.0f || distanceCm > 400.0f)
        {
            logger->log(Level::Error, 
                      "[HCSR04] Измеренное расстояние вне допустимого диапазона: %.2f см", 
                      distanceCm);
            return calibration::SENSOR_ERROR_VALUE;
        }

        logger->log(Level::Debug, "[HCSR04] Измеренное расстояние: %.2f см", distanceCm);

        distanceCm = static_cast<float>(A * static_cast<double>(distanceCm) + B);

        logger->log(Level::Debug, "[HCSR04] Расстояние до микросхемы: %.2f см", distanceCm);

        return distanceCm;
    }
    
    // Считать уровень воды в процентах от максимума
    float HCSR04::read()
    {
        // Получаем расстояние до поверхности воды в сантиметрах
        float distanceCm = readDistance();
        
        if (distanceCm == calibration::SENSOR_ERROR_VALUE)
        {
            lastMeasurement = calibration::SENSOR_ERROR_VALUE;
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        float waterLevelCm = containerDepth - distanceCm;
        
        // Ограничиваем уровень от 0 до глубины контейнера
        waterLevelCm = constrain(waterLevelCm, 0.0f, containerDepth);
        
        // Вычисляем максимальный уровень воды с учетом константы HCSR04_FULL_TANK_PERCENT
        float maxWaterLevel = containerDepth * (calibration::HCSR04_FULL_TANK_PERCENT / 100.0f);
        
        // Вычисляем процент заполнения
        float waterLevelPercent = (waterLevelCm / maxWaterLevel) * 100.0f;
    
        lastMeasurement = waterLevelPercent;
        
        return waterLevelPercent;
    }
    
    void HCSR04::setContainerDepth(float depth)
    {
        if (depth <= 0.0f)
        {
            logger->log(Level::Error, 
                      "[HCSR04] Некорректная глубина контейнера: %.2f", 
                      depth);
            return;
        }
        
        containerDepth = depth;
        
        logger->log(Level::Info, 
                  "[HCSR04] Установлена глубина контейнера: %.2f см", 
                  containerDepth);
    }
    
    float HCSR04::getContainerDepth() const
    {
        return containerDepth;
    }
} 