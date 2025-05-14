#include "logic/actuators_manager.h"
#include "utils/logger_factory.h"
#include "config/constants.h"
#include <GyverNTP.h> // Для доступа к NTP

namespace farm::logic
{
    using farm::log::Level;
    using farm::log::LoggerFactory;
    using namespace farm::config::actuators;
    using namespace farm::config::strategies;

    // Инициализация статической переменной (паттерн Singleton)
    std::shared_ptr<ActuatorsManager> ActuatorsManager::instance = nullptr;

    // Получение экземпляра синглтона (паттерн Singleton)
    std::shared_ptr<ActuatorsManager> ActuatorsManager::getInstance(std::shared_ptr<log::ILogger> logger)
    {
        if (!instance)
        {
            instance = std::shared_ptr<ActuatorsManager>(new ActuatorsManager(logger));
        }
        return instance;
    }

    ActuatorsManager::ActuatorsManager(std::shared_ptr<farm::log::ILogger> logger)
        : schedulerManager(utils::Scheduler::getInstance()),
          configManager   (config::ConfigManager::getInstance()),
          sensorsManager  (sensors::SensorsManager::getInstance())
    {
        // Если логгер не передан — создаём дефолтный
        if (!logger)        
        {
            this->logger = LoggerFactory::createSerialLogger(Level::Info);
        } 
        else 
        {
            this->logger = logger;
        }
    }

    ActuatorsManager::~ActuatorsManager()
    {
        logger->log(Level::Info, "[ActuatorsManager] Освобождение ресурсов менеджера актуаторов");
        
        deleteAllStrategies(); // Сначала удаляем все стратегии, чтобы корректно завершить задачи
        actuators.clear();

        logger->log(Level::Debug, "[ActuatorsManager] Ресурсы менеджера актуаторов освобождены");
    }

    bool ActuatorsManager::initialize()
    {
        logger->log(Level::Farm, "[ActuatorsManager] Инициализация менеджера актуаторов");
        deleteAllStrategies();
        actuators.clear();
        bool allInitialized = true;
        int totalAttempted = 0;

        // Инициализация всех актуаторов, если пины заданы
        if (pins::PUMP_R385_FORWARD_PIN  != sensors::calibration::UNINITIALIZED_PIN &&
            pins::PUMP_R385_BACKWARD_PIN != sensors::calibration::UNINITIALIZED_PIN)
        {
            if (!createAndInitActuator<actuators::PumpR385>(pins::PUMP_R385_FORWARD_PIN,    
                                                            pins::PUMP_R385_BACKWARD_PIN))
            {
                allInitialized = false;
            }
            totalAttempted++;
        }
        if (pins::HEATLAMP_PIN != sensors::calibration::UNINITIALIZED_PIN)
        {
            if (!createAndInitActuator<actuators::HeatLamp>(pins::HEATLAMP_PIN))
            {
                allInitialized = false;
            }
           totalAttempted++;
        }
        if (pins::GROWLIGHT_PIN != sensors::calibration::UNINITIALIZED_PIN)
        {
            if (!createAndInitActuator<actuators::GrowLight>(pins::GROWLIGHT_PIN))
            {
                allInitialized = false;
            }
            totalAttempted++;
        }

        // Без синхронизации времени (NTP) работа актуаторов невозможна — предотвращаем запуск
        if (!schedulerManager->isNtpOnline()) 
        {
            logger->log(Level::Warning, 
                      "[ActuatorsManager] Невозможно инициализировать без синхронизации времени по NTP");
            allInitialized = false;
            deleteAllStrategies();
            actuators.clear();
            return allInitialized;
        }
        
        logger->log(Level::Farm, 
                  "[ActuatorsManager] Количество инициализированных актуаторов: %d/%d", 
                  actuators.size(), totalAttempted);
                  
        if (!createStrategies())
        {
            logger->log(Level::Warning, 
                      "[ActuatorsManager] Не все стратегии были успешно созданы");
            allInitialized = false;
        }
        initialized = allInitialized;
        
        if (!allInitialized)
        {
            deleteAllStrategies();
            actuators.clear();
        }
        
        return allInitialized;
    }
    
    void ActuatorsManager::loop()
    {
        unsigned long currentTime = millis();
        if (currentTime - lastCheckTime >= config::actuators::CHECK_INTERVAL)
        {
            lastCheckTime = currentTime;
            // Автоматическая инициализация при появлении NTP
            if (!initialized && schedulerManager->isNtpOnline())
            {
                logger->log(Level::Debug, 
                          "[ActuatorsManager] NTP синхронизирован, инициализация менеджера актуаторов...");
                initialize();
                if (initialized)
                {
                    logger->log(Level::Farm, 
                              "[ActuatorsManager] Менеджер актуаторов успешно инициализирован");
                }
                else
                {
                    logger->log(Level::Error, 
                              "[ActuatorsManager] Не удалось инициализировать менеджер актуаторов");
                }
            }
        }
        // Здесь можно добавить дополнительные действия, если актуаторы уже инициализированы
    }

    bool ActuatorsManager::isInitialized() const
    {
        return initialized;
    } 
    
    bool ActuatorsManager::addActuator(const String& name, std::shared_ptr<actuators::IActuator> actuator)
    {
        if (actuators.find(name) != actuators.end())
        {
            logger->log(Level::Warning, 
                      "[ActuatorsManager] Актуатор с именем %s уже существует", 
                      name.c_str());
            return false;
        }
        
        actuators[name] = actuator;
        logger->log(Level::Debug, 
                  "[ActuatorsManager] Актуатор %s (%s) добавлен в ActuatorsManager", 
                  name.c_str(), actuator->getActuatorType().c_str());
                  
        return true;
    }
    
    std::shared_ptr<actuators::IActuator> ActuatorsManager::getActuator(const String &name)
    {
        auto it = actuators.find(name);
        if (it != actuators.end())
        {
            auto& actuator = it->second;
            if (!actuator || !actuator->isInitialized())
            {
                logger->log(Level::Warning, 
                          "[ActuatorsManager] Актуатор %s не инициализирован", 
                          name.c_str());
                return nullptr;
            }
            return actuator;
        }
        logger->log(Level::Warning, 
                  "[ActuatorsManager] Актуатор с именем %s не найден", 
                  name.c_str());
        return nullptr;
    }
    
    std::shared_ptr<strategies::IActuatorStrategy> ActuatorsManager::getStrategy(const String& name)
    {
        auto it = strategies.find(name);
        if (it != strategies.end())
        {
            return it->second;
        }
        
        logger->log(Level::Warning, 
                  "[ActuatorsManager] Стратегия для актуатора %s не найдена", 
                  name.c_str());
        return nullptr;
    }
    
    bool ActuatorsManager::hasActuator(const String& name) const
    {
        return actuators.find(name) != actuators.end();
    }
    
    bool ActuatorsManager::hasStrategy(const String& name) const
    {
        return strategies.find(name) != strategies.end();
    }
    
    size_t ActuatorsManager::getActuatorCount() const
    {
        return actuators.size();
    }
    
    size_t ActuatorsManager::getStrategyCount() const
    {
        return strategies.size();
    }
    
    // Принудительное управление актуатором всегда делается через стратегию, чтобы не нарушать бизнес-логику
    bool ActuatorsManager::forceTurnOn(const String& actuatorName)
    {
        auto strategy = getStrategy(actuatorName);
        if (strategy)
        {
            logger->log(Level::Info, 
                      "[ActuatorsManager] Принудительное включение актуатора %s через стратегию", 
                      actuatorName.c_str());
            strategy->forceTurnOn();
            return true;
        }
        else
        {
            logger->log(Level::Error, 
                        "[ActuatorsManager] Не удалось найти актуатор %s для включения", 
                        actuatorName.c_str());
            return false;
        }
    }
    
    bool ActuatorsManager::forceTurnOff(const String& actuatorName)
    {
        auto strategy = getStrategy(actuatorName);
        if (strategy)
        {
            logger->log(Level::Info, 
                      "[ActuatorsManager] Принудительное выключение актуатора %s через стратегию", 
                      actuatorName.c_str());
            strategy->forceTurnOff();
            return true;
        }
        else
        {
            logger->log(Level::Error, 
                        "[ActuatorsManager] Не удалось найти актуатор %s для выключения", 
                        actuatorName.c_str());
            return false;
        }
    }
    
    // Обновление всех стратегий после изменения конфигурации
    bool ActuatorsManager::updateStrategies()
    {
        logger->log(Level::Info, "[ActuatorsManager] Обновление стратегий для всех актуаторов");
        bool result = true;
        for (auto& strategyPair : strategies)
        {
            const String& actuatorName = strategyPair.first;
            auto& strategy = strategyPair.second;
            
            logger->log(Level::Debug, 
                      "[ActuatorsManager] Обновление стратегии для актуатора %s", 
                      actuatorName.c_str());
            
            // Отменяем существующие задачи, обновляем конфигурацию и пересоздаем задачи
            if (!strategy->reschedule())
            {
                logger->log(Level::Error, 
                          "[ActuatorsManager] Ошибка при обновлении стратегии для актуатора %s", 
                          actuatorName.c_str());
                result = false;
            }
            else
            {
                logger->log(Level::Debug, 
                          "[ActuatorsManager] Стратегия для актуатора %s успешно обновлена", 
                          actuatorName.c_str());
            }
        }
        return result;
    }
    
    // Обработка команд, пришедших по MQTT (см. CommandCode)
    bool ActuatorsManager::handleMqttCommand(const config::CommandCode& command)
    {
        logger->log(Level::Debug, 
                  "[ActuatorsManager] Получена команда %d", static_cast<int>(command));
        switch (command)
        {
            case config::CommandCode::PUMP_ON:
                logger->log(Level::Warning, "[ActuatorsManager] Получена команда включения насоса");
                return forceTurnOn(config::actuators::names::PUMP_R385);
                
            case config::CommandCode::PUMP_OFF:
                logger->log(Level::Warning, "[ActuatorsManager] Получена команда выключения насоса");
                return forceTurnOff(config::actuators::names::PUMP_R385);
                
            case config::CommandCode::GROWLIGHT_ON:
                logger->log(Level::Warning, "[ActuatorsManager] Получена команда включения фитоленты");
                return forceTurnOn(config::actuators::names::GROWLIGHT);
                
            case config::CommandCode::GROWLIGHT_OFF:
                logger->log(Level::Warning, "[ActuatorsManager] Получена команда выключения фитоленты");
                return forceTurnOff(config::actuators::names::GROWLIGHT);
                
            case config::CommandCode::HEATLAMP_ON:
                logger->log(Level::Warning, "[ActuatorsManager] Получена команда включения лампы нагрева");
                return forceTurnOn(config::actuators::names::HEATLAMP);
                
            case config::CommandCode::HEATLAMP_OFF:
                logger->log(Level::Warning, "[ActuatorsManager] Получена команда выключения лампы нагрева");
                return forceTurnOff(config::actuators::names::HEATLAMP);

            case config::CommandCode::FARM_ON:
                logger->log(Level::Warning, "[ActuatorsManager] Получена команда включения функциональности фермы");
                syncFarmState(true);
                return true;

            case config::CommandCode::FARM_OFF:
                logger->log(Level::Warning, "[ActuatorsManager] Получена команда выключения функциональности фермы");
                syncFarmState(false);
                return true;

            case config::CommandCode::ESP_RESTART:
                logger->log(Level::Warning, 
                          "[ActuatorsManager] Получена команда перезагрузки ESP");
                myRestartESP(); // безопасная перезагрузка
                return true;
                
            default:
                logger->log(Level::Error, 
                          "[ActuatorsManager] Получена неизвестная команда: %d", 
                          static_cast<int>(command));
                return false;
        }
    }
    
    // Создание стратегий для всех актуаторов (каждый тип актуатора — своя стратегия)
    bool ActuatorsManager::createStrategies()
    {
        logger->log(Level::Info, "[ActuatorsManager] Создание стратегий для актуаторов");

        bool allInitialized = true;
        int totalAttempted = 0;
        
        if (hasActuator(config::actuators::names::PUMP_R385))
        {
            if (!createAndApplyStrategy<strategies::IrrigationStrategy>(config::actuators::names::PUMP_R385))
            {
                logger->log(Level::Error, 
                          "[ActuatorsManager] Ошибка при создании стратегии полива");
                allInitialized = false;
            }
            totalAttempted++;
        }
        else
        {
            logger->log(Level::Warning, 
                      "[ActuatorsManager] Не удалось создать стратегию полива: насос не найден");
        }
        
        if (hasActuator(config::actuators::names::HEATLAMP))
        {
            if (!createAndApplyStrategy<strategies::HeatingStrategy>(config::actuators::names::HEATLAMP))
            {
                logger->log(Level::Error, 
                          "[ActuatorsManager] Ошибка при создании стратегии нагрева");
                allInitialized = false;
            }
            totalAttempted++;
        }
        else
        {
            logger->log(Level::Warning, 
                      "[ActuatorsManager] Не удалось создать стратегию нагрева: теплолампа не найдена");
        }
        
        if (hasActuator(config::actuators::names::GROWLIGHT))
        {
            if (!createAndApplyStrategy<strategies::LightingStrategy>(config::actuators::names::GROWLIGHT))
            {
                logger->log(Level::Error, 
                          "[ActuatorsManager] Ошибка при создании стратегии освещения");
                allInitialized = false;
            }
            totalAttempted++;
        }
        else
        {
            logger->log(Level::Warning, 
                      "[ActuatorsManager] Не удалось создать стратегию освещения: фитолента не найдена");
        }

        logger->log(Level::Farm, 
            "[ActuatorsManager] Количество созданных стратегий: %d/%d", 
            strategies.size(), totalAttempted);
        
        return allInitialized;
    }

    // Корректное завершение всех стратегий и отмена связанных задач. Очистка коллекции стратегий
    void ActuatorsManager::deleteAllStrategies()
    {
        logger->log(Level::Debug, "[ActuatorsManager] Остановка и удаление стратегий для актуаторов");
        
        for (auto& strategyPair : strategies)
        {
            const String& actuatorName = strategyPair.first;
            auto& strategy = strategyPair.second;
            
            logger->log(Level::Debug, 
                      "[ActuatorsManager] Отмена задач стратегии для актуатора %s", 
                      actuatorName.c_str());
                      
            if (strategy)
            {
                strategy->cancelScheduledTasks();
            }
        }
        
        strategies.clear();

        logger->log(Level::Debug, "[ActuatorsManager] Все стратегии остановлены и удалены");
    }

    // Безопасная перезагрузка ESP: сначала корректно завершаем все задачи и стратегии
    void ActuatorsManager::myRestartESP()
    {
        logger->log(Level::Warning, "[ActuatorsManager] Выполняется безопасная перезагрузка ESP");
        deleteAllStrategies();
        delay(500);
        logger->log(Level::Warning, "[ActuatorsManager] Перезагрузка ESP...");
        delay(200); // Даем время на отправку логов
        ESP.restart();
    }

    // Включение/выключение всех актуаторов и очистка ресурсов
    void ActuatorsManager::actuatorsEnable(bool enable)
    {
        if (enable && initialized) {
            logger->log(Level::Debug, "[ActuatorsManager] Актуаторы уже включены");
            return;
        }
        if (!enable && !initialized) {
            logger->log(Level::Debug, "[ActuatorsManager] Актуаторы уже выключены");
            return;
        }
        
        if (enable) {
            logger->log(Level::Farm, "[ActuatorsManager] Включение актуаторов");
            
            if (!initialize()) {
                logger->log(Level::Error, "[ActuatorsManager] Не удалось инициализировать актуаторы");
                return;
            }
        } else {
            logger->log(Level::Farm, "[ActuatorsManager] Выключение актуаторов");
            deleteAllStrategies();
            actuators.clear();
            
            if (!strategies.empty() || !actuators.empty()) {
                logger->log(Level::Error, "[ActuatorsManager] Не удалось полностью очистить ресурсы");
                return;
            }
            
            initialized = false;
            logger->log(Level::Farm, "[ActuatorsManager] Актуаторы успешно выключены");
        }
    }

    // Синхронизация состояния фермы: включает/выключает все актуаторы и сенсоры
    void ActuatorsManager::syncFarmState(bool enable)
    {
        actuatorsEnable(enable);
        if (sensorsManager) {
            sensorsManager->sensorsEnable(enable);
        }
    }
}
