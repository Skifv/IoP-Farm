// Датчик расхода воды

#include "sensors/YFS401.h"

namespace farm::sensors
{
    // Инициализируем статический указатель
    // Необходим для работы с прерыванием
    YFS401* YFS401::s_instance = nullptr;
    
    // Обработчик прерывания для счета импульсов
    // Помечен атрибутом IRAM_ATTR для быстрого выполнения из RAM
    void IRAM_ATTR flowPulseCounter() 
    {
        // Получаем доступ к единственному экземпляру
        YFS401* sensor = YFS401::getInstance();
        if (sensor) 
        {
            sensor->pulse();
        }
    }

    // Статический метод для получения экземпляра
    // Не паттерн Singleton, используется лишь для работы с прерыванием
    YFS401* YFS401::getInstance()
    {
        return s_instance;
    }

    YFS401::YFS401(std::shared_ptr<log::ILogger> logger, uint8_t pin)
        : ISensor(),
          pin(pin),
          calibrationFactor(calibration::YFS401_CALIBRATION_FACTOR),
          pulseCount(0),
          lastUpdateTime(0) 
    {
        this->logger = logger;
        
        setSensorName(farm::config::sensors::names::YFS401);
        setMeasurementType(farm::config::sensors::json_keys::WATER_FLOW);
        setUnit(farm::config::sensors::units::LITERS);
        
        // Устанавливаем флаги (с расходометром работает только насос)
        shouldBeRead = false;
        shouldBeSaved = false;
        
        // Сохраняем указатель на себя в статической переменной
        s_instance = this;
    }
    
    YFS401::~YFS401()
    {
        if (initialized && pin != farm::config::sensors::calibration::UNINITIALIZED_PIN) 
        {
            detachInterrupt(digitalPinToInterrupt(pin));
            logger->log(farm::log::Level::Debug, 
                      "[YFS401] Прерывание отключено на пине %d", pin);
        }

        enable(false);
        
        if (s_instance == this)
        {
            s_instance = nullptr;
        }
    }

    bool YFS401::initialize()
    {
        if (pin == farm::config::sensors::calibration::UNINITIALIZED_PIN) 
        {
            logger->log(farm::log::Level::Error, 
                      "[YFS401] Не инициализирован пин датчика");
            return false;
        }

        pinMode(pin, INPUT);
        
        resetCounter();

        initialized = true;
        return true;
    }

    void YFS401::resetCounter()
    {
        pulseCount = 0;
        lastUpdateTime = millis();
        lastMeasurement = 0.0f;
    }

    void YFS401::setCalibrationFactor(float factor)
    {
        if (factor > 0) 
        {
            calibrationFactor = factor;
            logger->log(farm::log::Level::Debug, 
                      "[YFS401] Установлен коэффициент калибровки: %.2f", calibrationFactor);
        }
        else 
        {
            logger->log(farm::log::Level::Warning, 
                      "[YFS401] Попытка установить некорректный коэффициент калибровки: %.2f", factor);
        }
    }

    // Зарегистрировать импульс (вызывается из ISR)
    void YFS401::pulse()
    {
        pulseCount++;
    }

    // Считать объем в литрах
    float YFS401::read()
    {
        if (!initialized) 
        {
            logger->log(farm::log::Level::Error, 
                      "[YFS401] Попытка чтения с неинициализированного датчика");
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        lastMeasurement = getTotalVolume();
        
        logger->log(farm::log::Level::Debug, 
                  "[YFS401] Текущий объем: %.3f л (импульсов: %lu)",
                  lastMeasurement, pulseCount);

        return lastMeasurement;
    }

    // Получить общий объем воды в литрах с момента последнего сброса счетчика
    float YFS401::getTotalVolume() const
    {
        if (!initialized) 
        {
            if (logger)
            {
                logger->log(farm::log::Level::Error, 
                          "[YFS401] Попытка получения объема с неинициализированного датчика");
            }
            return calibration::SENSOR_ERROR_VALUE;
        }
        
        // Объем = количество импульсов / калибровочный коэффициент (импульсы на литр)
        if (calibrationFactor <= 0) 
        {   
            if (logger)
            {
                logger->log(farm::log::Level::Error, 
                          "[YFS401] Некорректный коэффициент калибровки: %.2f", calibrationFactor);
            }
            return 0.0f;
        }
        
        // Создаем локальную копию volatile переменной для безопасного чтения
        unsigned long currentPulseCount = pulseCount;
        
        return (float)currentPulseCount / calibrationFactor;
    }
    
    // Переопределение метода saveMeasurement
    bool YFS401::saveMeasurement()
    {
        if (!initialized) 
        {
            logger->log(farm::log::Level::Error, 
                      "[YFS401] Попытка сохранения данных с неинициализированного датчика");
            return false;
        }
        
        // Пустая реализация, так как для этого датчика не требуется сохранение
        logger->log(farm::log::Level::Debug, 
                  "[YFS401] Сохранение данных не реализовано для этого датчика");
        return true;
    }
    
    bool YFS401::enable(bool enable)
    {
        if (!initialized)
        {
            if (logger)
            {
                logger->log(farm::log::Level::Error, 
                          "[YFS401] Попытка включения неинициализированного датчика");
            }
            return false;
        }
        
        if (enable) 
        {
            resetCounter();
            
            attachInterrupt(digitalPinToInterrupt(pin), flowPulseCounter, RISING);
            
            if (logger)
            {
                logger->log(farm::log::Level::Debug,
                            "[YFS401] Датчик включен");
            }
            return true;
        } 
        else 
        {
            detachInterrupt(digitalPinToInterrupt(pin));
            
            float finalVolume = getTotalVolume();
            
            logger->log(farm::log::Level::Debug, 
                       "[YFS401] Датчик выключен. Итоговый объем: %.3f л",
                       finalVolume);
            return true;
        }
    }
} 