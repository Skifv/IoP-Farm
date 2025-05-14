#include <Arduino.h>
#include <memory>
#include <GyverNTP.h>

#include "network/wifi_manager.h"
#include "network/mqtt_manager.h"

#include "config/config_manager.h"
#include "config/constants.h"

#include "sensors/sensors_manager.h"
#include "logic/actuators_manager.h"

#include "utils/ota_manager.h"
#include "utils/web_server_manager.h"
#include "utils/logger_factory.h"
#include "utils/logger.h"
#include "utils/scheduler.h"

using namespace farm::config;
using namespace farm::config::sensors;
using namespace farm::log;
using namespace farm::net;
using namespace farm::sensors;
using namespace farm::utils;
using namespace farm::logic;

void allActuatorsOff();

// Создаем логгер с нужным уровнем и цветом в зависимости от режима компиляции
#if defined(IOP_DEBUG) && defined(COLOR_SERIAL_LOG)
auto logger = LoggerFactory::createColorSerialMQTTLogger(Level::Debug);
#elif defined(IOP_DEBUG)
auto logger = LoggerFactory::createSerialMQTTLogger(Level::Debug);
#elif defined(COLOR_SERIAL_LOG)
auto logger = LoggerFactory::createColorSerialMQTTLogger(Level::Info);
#else
auto logger = LoggerFactory::createSerialMQTTLogger(Level::Info);
#endif

// Получаем экземпляры синглтонов для всех менеджеров системы
std::shared_ptr<ConfigManager>    configManager    = ConfigManager::getInstance(logger);
std::shared_ptr<MyWiFiManager>    wifiManager      = MyWiFiManager::getInstance(logger);
std::shared_ptr<MQTTManager>      mqttManager      = MQTTManager::getInstance(logger);
std::shared_ptr<SensorsManager>   sensorsManager   = SensorsManager::getInstance(logger);
std::shared_ptr<OTAManager>       otaManager       = OTAManager::getInstance(logger);
std::shared_ptr<WebServerManager> webServerManager = WebServerManager::getInstance(logger);
std::shared_ptr<Scheduler>        schedulerManager = Scheduler::getInstance(logger);  
std::shared_ptr<ActuatorsManager> actuatorsManager = ActuatorsManager::getInstance(logger);

// Флаги для отслеживания инициализации NTP и актуаторов
bool ntpSynchronized = false;
bool actuatorsInitialized = false;
unsigned long startupTime = 0;

void setup() 
{
    allActuatorsOff(); // Гарантируем безопасный старт: все исполнительные устройства выключены

    Serial.begin(115200);
    delay(1000);

    pinMode(pins::LED_PIN, OUTPUT);
    digitalWrite(pins::LED_PIN, HIGH);

    startupTime = millis();
    
    logger->log(Level::Farm, "=== IoP-Farm начало работы ===");

    configManager->initialize();     

#ifdef IOP_DEBUG
    logger->log(Level::Debug, "Текущая файловая система SPIFFS:");
    configManager->printSpiffsInfo(); // Выводим структуру SPIFFS для отладки, печатается лишь в консоль
#endif

    wifiManager     ->initialize();  
    mqttManager     ->initialize();  
    sensorsManager  ->initialize();  
    otaManager      ->initialize();  
    
    webServerManager->enableAuth(true);
    webServerManager->initialize();  
    
    // Примечание: планировщик и менеджер актуаторов инициализируются лишь после синхронизации NTP Clock
    schedulerManager->initialize(time::DEFAULT_GMT_OFFSET);  
    actuatorsManager->initialize(); 
    
    // Демонстрация работы планировщика: событие через 30 секунд
    logger->log(Level::Debug, "[MAIN] Добавление тестового пустого события");
    schedulerManager->scheduleOnceAfter(30, []() {
        logger->log(Level::Info, "[MAIN] Тестовое событие выполнено через 30 секунд после запуска!");
    });

#ifdef USE_FREERTOS
    // Если используется FreeRTOS — запускаем задачу планировщика
    schedulerManager->startSchedulerTask(scheduler::SCHEDULER_TASK_PRIORITY, 
                                         scheduler::SCHEDULER_STACK_SIZE);
#endif
}

void loop() 
{
    wifiManager->maintainConnection();
    mqttManager->maintainConnection();

    // Контролируем синхронизацию времени через NTP
    if (NTP.tick()) 
    {
        if (!ntpSynchronized && schedulerManager->isNtpOnline()) {
            ntpSynchronized = true;
            unsigned long syncTime = (millis() - startupTime) / 1000;
            logger->log(Level::Info, 
                      "[NTP] NTP синхронизирован через %lu с. после запуска", 
                      syncTime);
            logger->log(Level::Info, "[MAIN] Текущее время: %s", NTP.toString().c_str());
        }
        else if (!ntpSynchronized)
        {
            logger->log(Level::Debug, "[MAIN] NTP не синхронизирован, текущее время: %s", NTP.toString().c_str());
            logger->log(Level::Debug, "[MAIN] ntpSynchronized: %d, ntpOnline: %d", ntpSynchronized, schedulerManager->isNtpOnline());
            NTP.updateNow();
        }
    }
    else if (!ntpSynchronized)
    {
        logger->log(Level::Debug, "[MAIN] NTP не синхронизирован, текущее время: %s", NTP.toString().c_str());
        logger->log(Level::Debug, "[MAIN] ntpSynchronized: %d, ntpOnline: %d", ntpSynchronized, schedulerManager->isNtpOnline());
        NTP.updateNow();
    }

#ifndef USE_FREERTOS
    // Планировщик событий вызывается вручную, если не используется FreeRTOS
    schedulerManager->checkSchedule();
#endif

    sensorsManager->loop();    // Опрос всех датчиков
    actuatorsManager->loop();  // Контроль состояния исполнительных устройств

    // Логируем момент инициализации актуаторов (однократно)
    if (!actuatorsInitialized && actuatorsManager->isInitialized()) {
        actuatorsInitialized = true;
        unsigned long initTime = (millis() - startupTime) / 1000;
        logger->log(Level::Info, 
                  "[ActuatorsManager] ActuatorsManager инициализирован через %lu с. после запуска", 
                  initTime);
    }

    otaManager->handle(); 
    webServerManager->handleClient();       

    delay(loop::DEFAULT_DELAY_MS); // Минимальная задержка для стабильности
}

void allActuatorsOff()
{
    // Программно выключаем все исполнительные устройства для предотвращения аварий при старте
    pinMode(actuators::pins::GROWLIGHT_PIN, OUTPUT);
    pinMode(actuators::pins::HEATLAMP_PIN, OUTPUT);
    pinMode(actuators::pins::PUMP_R385_FORWARD_PIN, OUTPUT);
    pinMode(actuators::pins::PUMP_R385_BACKWARD_PIN, OUTPUT);

    digitalWrite(actuators::pins::GROWLIGHT_PIN, LOW);
    digitalWrite(actuators::pins::HEATLAMP_PIN, LOW);
    digitalWrite(actuators::pins::PUMP_R385_FORWARD_PIN, LOW);
    digitalWrite(actuators::pins::PUMP_R385_BACKWARD_PIN, LOW);
}